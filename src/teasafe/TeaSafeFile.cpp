/*
  Copyright (c) <2013-2014>, <BenHJ>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the names of its contributors
  may be used to endorse or promote products derived from this software without
  specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "teasafe/TeaSafeImageStream.hpp"
#include "teasafe/TeaSafeFile.hpp"
#include "teasafe/FileBlockBuilder.hpp"
#include "teasafe/FileBlockIterator.hpp"
#include "teasafe/FileEntryException.hpp"
#include "teasafe/detail/DetailTeaSafe.hpp"
#include "teasafe/detail/DetailFileBlock.hpp"

#include <boost/make_shared.hpp>

#include <stdexcept>

namespace teasafe
{
    // for writing a brand new entry where start block isn't known
    TeaSafeFile::TeaSafeFile(SharedCoreIO const &io,
                             std::string const &name,
                             bool const enforceStartBlock)
        : m_io(io)
        , m_name(name)
        , m_enforceStartBlock(enforceStartBlock)
        , m_fileSize(0)
        , m_workingBlock()
        , m_buffer()
        , m_startVolumeBlock(0)
        , m_blockIndex(0)
        , m_openDisposition(OpenDisposition::buildAppendDisposition())
        , m_pos(0)
        , m_blockCount(0)
        , m_stream()
    {
    }

    // for appending or overwriting
    TeaSafeFile::TeaSafeFile(SharedCoreIO const &io,
                             std::string const &name,
                             uint64_t const startBlock,
                             OpenDisposition const &openDisposition)
        : m_io(io)
        , m_name(name)
        , m_enforceStartBlock(false)
        , m_fileSize(0)
        , m_workingBlock()
        , m_buffer()
        , m_startVolumeBlock(startBlock)
        , m_blockIndex(0)
        , m_openDisposition(openDisposition)
        , m_pos(0)
        , m_blockCount(0)
        , m_stream()
    {
        // counts number of blocks and sets file size
        enumerateBlockStats();

        // sets the current working block to the very first file block
        m_workingBlock = boost::make_shared<FileBlock>(io, startBlock, openDisposition, m_stream);

        m_stream = m_workingBlock->getStream();

        // set up for specific write-mode
        if (m_openDisposition.readWrite() != ReadOrWriteOrBoth::ReadOnly) {

            // if in trunc, unlink
            if (m_openDisposition.trunc() == TruncateOrKeep::Truncate) {
                this->unlink();

            } else {
                // only if in append mode do we seek to end.
                if (m_openDisposition.append() == AppendOrOverwrite::Append) {
                    this->seek(0, std::ios::end);
                }
            }
        }
    }

    namespace detail
    {

        uint32_t blockWriteSpace()
        {
            return FILE_BLOCK_SIZE - FILE_BLOCK_META;
        }

    }

    std::string
    TeaSafeFile::filename() const
    {
        return m_name;
    }

    uint64_t
    TeaSafeFile::fileSize() const
    {
        return m_fileSize;
    }

    uint64_t
    TeaSafeFile::getCurrentVolumeBlockIndex()
    {
        if (!m_workingBlock) {
            checkAndUpdateWorkingBlockWithNew();
        }
        return m_workingBlock->getIndex();
    }

    uint64_t
    TeaSafeFile::getStartVolumeBlockIndex() const
    {
        if (!m_workingBlock) {
            checkAndUpdateWorkingBlockWithNew();
            m_startVolumeBlock = m_workingBlock->getIndex();
        }
        return m_startVolumeBlock;
    }

    std::streamsize
    TeaSafeFile::readWorkingBlockBytes()
    {
        // need to take into account the currently seeked-to position and
        // subtract that because we then only want to read from the told position
        uint32_t size =  m_workingBlock->getDataBytesWritten() - m_workingBlock->tell();

        std::vector<uint8_t>().swap(m_buffer);
        m_buffer.resize(size);
        (void)m_workingBlock->read((char*)&m_buffer.front(), size);

        if (m_blockIndex + 1 < m_blockCount) {
            ++m_blockIndex;
            m_workingBlock = boost::make_shared<FileBlock>(m_io,
                                                           m_workingBlock->getNextIndex(),
                                                           m_openDisposition,
                                                           m_stream);
        }

        return size;
    }

    void TeaSafeFile::newWritableFileBlock() const
    {
        FileBlock block(m_io->blockBuilder->buildWritableFileBlock(m_io,
                                                                   teasafe::OpenDisposition::buildAppendDisposition(),
                                                                   m_stream,
                                                                   m_enforceStartBlock));
        if (m_enforceStartBlock) { m_enforceStartBlock = false; }

        block.registerBlockWithVolumeBitmap();

        if (m_workingBlock) {
            m_workingBlock->setNextIndex(block.getIndex());
        }

        ++m_blockCount;
        m_blockIndex = m_blockCount - 1;
        m_workingBlock = boost::make_shared<FileBlock>(block);
    }

    void TeaSafeFile::enumerateBlockStats()
    {
        // find very first block
        FileBlockIterator block(m_io,
                                m_startVolumeBlock,
                                m_openDisposition,
                                m_stream);
        FileBlockIterator end;
        for (; block != end; ++block) {
            m_fileSize += block->getDataBytesWritten();
            ++m_blockCount;
        }
    }

    void
    TeaSafeFile::writeBufferedDataToWorkingBlock(uint32_t const bytes)
    {
        m_workingBlock->write((char*)&m_buffer.front(), bytes);
        std::vector<uint8_t>().swap(m_buffer);

        // stream would have been initialized in block's write function
        if(!m_stream) {
            m_stream = m_workingBlock->getStream();
        }
    }

    bool
    TeaSafeFile::workingBlockHasAvailableSpace() const
    {
        // use tell to get bytes written so far as the read/write head position
        // is always updates after reads/writes
        uint32_t const bytesWritten = m_workingBlock->tell();

        if (bytesWritten < detail::blockWriteSpace()) {
            return true;
        }
        return false;

    }

    void
    TeaSafeFile::checkAndUpdateWorkingBlockWithNew() const
    {
        // first case no file blocks so absolutely need one to write to
        if (!m_workingBlock) {
            newWritableFileBlock();

            // when writing the file, the working block will be empty
            // and the start volume block will be unset so need to set now
            m_startVolumeBlock = m_workingBlock->getIndex();
            return;
        }

        // in this case the current block is exhausted so we need a new one
        if (!workingBlockHasAvailableSpace()) {

            // EDGE case: if overwrite causes us to go over end, need to
            // switch to append mode
            if (this->tell() >= m_fileSize) {
                m_openDisposition = OpenDisposition::buildAppendDisposition();
            }

            // if in overwrite mode, maybe we want to overwrite current bytes
            if (m_openDisposition.append() == AppendOrOverwrite::Overwrite) {

                // if the reported stream position in the block is less that
                // the block's total capacity, then we don't create a new block
                // we simply overwrite
                if (m_workingBlock->tell() < detail::blockWriteSpace()) {
                    return;
                }

                // edge case; if right at the very end of the block, need to
                // iterate the block index and return if possible
                if (m_workingBlock->tell() == detail::blockWriteSpace()) {
                    ++m_blockIndex;
                    m_workingBlock = boost::make_shared<FileBlock>(m_io,
                                                                   m_workingBlock->getNextIndex(),
                                                                   m_openDisposition,
                                                                   m_stream);
                    return;
                }

            }
            newWritableFileBlock();

            return;
        }

    }

    uint32_t
    TeaSafeFile::getBytesLeftInWorkingBlock()
    {
        // if given the stream position no more bytes can be written
        // then write out buffer
        uint32_t streamPosition(m_workingBlock->tell());

        // the stream position is subtracted since block may have already
        // had bytes written to it in which case the available size left
        // is approx. block size - stream position
        return (detail::blockWriteSpace()) - streamPosition;
    }

    std::streamsize
    TeaSafeFile::read(char* s, std::streamsize n)
    {
        if (m_openDisposition.readWrite() == ReadOrWriteOrBoth::WriteOnly) {
            throw FileEntryException(FileEntryError::NotReadable);
        }

        // read block data
        uint32_t read(0);
        uint64_t offset(0);
        while (read < n) {

            uint32_t count = readWorkingBlockBytes();

            // check that we don't read too much!
            if (read + count >= n) {
                count -= (read + count - n);
            }

            read += count;

            // optimization; use this rather than for loop
            // copies from m_buffer to s + offset
            std::copy(&m_buffer[0], &m_buffer[count], s + offset);

            offset += count;

            // edge case bug fix
            if (count == 0) { break; }
        }

        // update stream position
        m_pos += n;

        return n;
    }

    uint32_t
    TeaSafeFile::bufferBytesForWorkingBlock(const char* s, std::streamsize n, uint32_t offset)
    {

        uint32_t const spaceAvailable = getBytesLeftInWorkingBlock();

        // if n is smaller than space available, just copy in to buffer
        if (n < spaceAvailable) {
            m_buffer.resize(n);
            std::copy(&s[offset], &s[offset + n], &m_buffer[0]);
            return n;
        }

        // if space available > 0 copy in to the buffer spaceAvailable bytes
        if (spaceAvailable > 0) {
            m_buffer.resize(spaceAvailable);
            std::copy(&s[offset], &s[offset + spaceAvailable], &m_buffer[0]);
            return spaceAvailable;
        }
        return 0;
    }

    std::streamsize
    TeaSafeFile::write(const char* s, std::streamsize n)
    {

        if (m_openDisposition.readWrite() == ReadOrWriteOrBoth::ReadOnly) {
            throw FileEntryException(FileEntryError::NotWritable);
        }

        std::streamsize wrote(0);
        while (wrote < n) {

            // check if the working block needs to be updated with a new one
            checkAndUpdateWorkingBlockWithNew();

            // buffers the data that will be written to the working block
            // computed as a function of the data left to write and the
            // working block's available space
            uint32_t actualWritten = bufferBytesForWorkingBlock(s, (n-wrote), wrote);

            // does what it says
            writeBufferedDataToWorkingBlock(actualWritten);
            wrote += actualWritten;

            // update stream position
            m_pos += actualWritten;

            if (m_openDisposition.append() == AppendOrOverwrite::Append) {
                m_fileSize+=actualWritten;
            }
        }

        return wrote;
    }

    void
    TeaSafeFile::truncate(std::ios_base::streamoff newSize)
    {
        // compute number of block required
        uint16_t const blockSize = detail::blockWriteSpace();

        // edge case
        if (newSize < blockSize) {
            FileBlock zeroBlock = getBlockWithIndex(0);
            zeroBlock.setSize(newSize);
            zeroBlock.setNextIndex(zeroBlock.getIndex());
            return;
        }

        boost::iostreams::stream_offset const leftOver = newSize % blockSize;

        boost::iostreams::stream_offset const roundedDown = newSize - leftOver;

        uint64_t blocksRequired = roundedDown / blockSize;

        // edge case
        SharedFileBlock block;
        if (leftOver == 0) {
            --blocksRequired;
            block = boost::make_shared<FileBlock>(getBlockWithIndex(blocksRequired));
            block->setSize(blockSize);
        } else {
            block = boost::make_shared<FileBlock>(getBlockWithIndex(blocksRequired));
            block->setSize(leftOver);
        }

        block->setNextIndex(block->getIndex());

        m_blockCount = blocksRequired;

    }

    typedef std::pair<int64_t, boost::iostreams::stream_offset> SeekPair;
    SeekPair
    getPositionFromBegin(boost::iostreams::stream_offset off)
    {
        // find what file block the offset would relate to and set extra offset in file block
        // to that position
        uint16_t const blockSize = detail::blockWriteSpace();
        boost::iostreams::stream_offset casted = off;
        boost::iostreams::stream_offset const leftOver = casted % blockSize;
        int64_t block = 0;
        boost::iostreams::stream_offset blockPosition = 0;
        if (off > blockSize) {
            if (leftOver > 0) {

                // round down casted
                casted -= leftOver;

                // set the position of the stream in block to leftOver
                blockPosition = leftOver;

                ++block;

            } else {
                blockPosition = 0;
            }

            // get exact number of blocks after round-down
            block = off / blockSize;

        } else {
            // offset is smaller than the first block so keep block
            // index at 0 and the position for the zero block at offset
            blockPosition = off;
        }
        return std::make_pair(block, blockPosition);
    }

    SeekPair
    getPositionFromEnd(boost::iostreams::stream_offset off, int64_t endBlockIndex,
                       boost::iostreams::stream_offset bytesWrittenToEnd)
    {
        // treat like begin and then 'inverse'
        SeekPair treatLikeBegin = getPositionFromBegin(abs(off));

        int64_t block = endBlockIndex - treatLikeBegin.first;
        boost::iostreams::stream_offset blockPosition = bytesWrittenToEnd - treatLikeBegin.second;

        if (blockPosition < 0) {
            uint16_t const blockSize = detail::blockWriteSpace();
            blockPosition = blockSize + blockPosition;
            --block;
        }

        return std::make_pair(block, blockPosition);

    }

    SeekPair
    getPositionFromCurrent(boost::iostreams::stream_offset off,
                           int64_t blockIndex,
                           boost::iostreams::stream_offset indexedBlockPosition)
    {

        // find what file block the offset would relate to and set extra offset in file block
        // to that position
        uint16_t const blockSize = detail::blockWriteSpace();
        boost::iostreams::stream_offset const casted = off;

        boost::iostreams::stream_offset addition = casted + indexedBlockPosition;

        if (addition >= 0 && addition <= blockSize) {
            return std::make_pair(blockIndex, addition);
        } else {

            boost::iostreams::stream_offset const leftOver = abs(addition) % blockSize;

            int64_t sumValue = 0;

            boost::iostreams::stream_offset roundedDown = addition - leftOver;

            if (abs(roundedDown) > (blockSize)) {
                sumValue = abs(roundedDown) / blockSize;

                // hacky bit to get working
                if ((addition < 0) && ((blockSize - leftOver) > indexedBlockPosition)) {
                    sumValue++;
                }
            } else {
                sumValue = 1;
            }

            uint16_t const theBlock = (addition < 0) ? (blockIndex - sumValue) :
                (blockIndex + sumValue);
            boost::iostreams::stream_offset offset = (addition < 0) ? (blockSize - leftOver) :
                leftOver;

            return std::make_pair(theBlock, offset);
        }
    }

    boost::iostreams::stream_offset
    TeaSafeFile::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way)
    {
        // reset any offset values to zero but only if not seeking from the current
        // position. When seeking from the current position, we need to keep
        // track of the original block offset
        if (way != std::ios_base::cur) {
            if (m_workingBlock) {
                m_workingBlock->seek(0);
            }
        }

        // if at end just seek right to end and don't do anything else
        SeekPair seekPair;
        if (way == std::ios_base::end) {

            size_t endBlock = m_blockCount - 1;
            seekPair = getPositionFromEnd(off, endBlock,
                                          getBlockWithIndex(endBlock).getDataBytesWritten());

        }

        // pass in the current offset, the current block and the current
        // block position. Note both of these latter two params will be 0
        // if seeking from the beginning

        if (way == std::ios_base::beg) {
            seekPair = getPositionFromBegin(off);
        }
        // seek relative to the current position
        if (way == std::ios_base::cur) {
            seekPair = getPositionFromCurrent(off, m_blockIndex,
                                              m_workingBlock->tell());
        }

        // check bounds and error if too big
        if (seekPair.first >= m_blockCount || seekPair.first < 0) {
            return -1; // fail
        } else {

            // update block where we start reading/writing from
            m_blockIndex = seekPair.first;
            m_workingBlock = boost::make_shared<FileBlock>(this->getBlockWithIndex(m_blockIndex));

            // set the position to seek to for given block
            // this will be the point from which we read or write
            m_workingBlock->seek(seekPair.second);

            switch (way) {
              case std::ios_base::cur:
                m_pos = m_pos + off;
                break;
              case std::ios_base::end:
                m_pos = m_fileSize + off;
                break;
              case std::ios_base::beg:
                m_pos = off;
                break;
              default:
                break;
            }

        }

        return off;
    }

    boost::iostreams::stream_offset
    TeaSafeFile::tell() const
    {
        return m_pos;
    }

    void
    TeaSafeFile::flush()
    {
        writeBufferedDataToWorkingBlock(m_buffer.size());
        if (m_optionalSizeCallback) {
            (*m_optionalSizeCallback)(m_fileSize);
        }
    }

    void
    TeaSafeFile::unlink()
    {
        // loop over all file blocks and update the volume bitmap indicating
        // that block is no longer in use
        FileBlockIterator it(m_io, m_startVolumeBlock, m_openDisposition, m_stream);
        FileBlockIterator end;

        for (; it != end; ++it) {
            it->unlink();
            m_io->freeBlocks++;
        }

        m_fileSize = 0;
        m_blockCount = 0;
        m_workingBlock = SharedFileBlock();
    }

    void
    TeaSafeFile::setOptionalSizeUpdateCallback(SetEntryInfoSizeCallback callback)
    {
        m_optionalSizeCallback = OptionalSizeCallback(callback);
    }

    FileBlock
    TeaSafeFile::getBlockWithIndex(uint64_t n) const
    {
        FileBlockIterator it(m_io, m_startVolumeBlock, m_openDisposition, m_stream);
        FileBlockIterator end;
        uint64_t c(0);
        for (; it != end; ++it) {
            if (c==n) {
                return *it;
            }
            ++c;
        }

        throw std::runtime_error("Whoops! Something went wrong in TeaSafeFile::getBlockWithIndex");
    }
}
