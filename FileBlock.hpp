#ifndef BFS_FILE_BLOCK_HPP__
#define BFS_FILE_BLOCK_HPP__

#include "DetailBFS.hpp"
#include "DetailFileBlock.hpp"

#include <vector>

#include <boost/iostreams/categories.hpp>  // sink_tag
#include <iosfwd>                          // streamsize
#include <string>
#include <fstream>


namespace bfs
{

    class FileBlock
    {
    public:
    	/**
    	 * @brief for when a file block needs to be written use this constructor
    	 * @param imagePath the path of the bfs image
    	 * @param totalBlocks the total number of blocks in the fs image
    	 * @param index the index of this file block
    	 * @param next the index of the next file block that makes up the file
    	 */
        FileBlock(std::string const &imagePath,
        		  uint64_t const totalBlocks,
        	      uint64_t const index,
        		  uint64_t const next)
    		: m_imagePath(imagePath)
    		, m_totalBlocks(totalBlocks)
    	    , m_index(index)
    		, m_size(0)
    		, m_next(next)
    		, m_offset(0)
            , m_extraOffset(0)
        {
        	// set m_offset
        	std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
        	m_offset = detail::getOffsetOfFileBlock(m_index, m_totalBlocks);
        	(void)stream.seekp(m_offset);

        	// write m_size (set size to default can later be updated if necessary)
        	uint8_t sizeDat[4];
        	uint32_t size = detail::FILE_BLOCK_SIZE - detail::FILE_BLOCK_META;
        	detail::convertInt32ToInt4Array(size, sizeDat);
        	(void)stream.write((char*)sizeDat, 4);

        	// write m_next
        	uint8_t nextDat[8];
        	detail::convertUInt64ToInt8Array(m_next, nextDat);
        	assert(m_next == detail::convertInt8ArrayToInt64(nextDat));
        	(void)stream.write((char*)nextDat, 8);
        	stream.flush();
        	stream.close();
        }

        /**
         * @brief for when a file block needs to be read use this constructor
         * @param index the index of the file block
         * @note other params like size and next will be initialized when
         * the block is actually read
         */
        FileBlock(std::string const &imagePath,
        		  uint64_t const totalBlocks,
        		  uint64_t const index)
    		: m_imagePath(imagePath)
            , m_totalBlocks(totalBlocks)
    	    , m_index(index)
    		, m_size(0)
    		, m_next(0)
    		, m_offset(0)
            , m_extraOffset(0)
        {
        	// set m_offset
			std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			m_offset = detail::getOffsetOfFileBlock(m_index, m_totalBlocks);
			(void)stream.seekg(m_offset);

			// read m_size
        	uint8_t sizeDat[4];
        	(void)stream.read((char*)sizeDat, 4);
        	m_size = detail::convertInt4ArrayToInt32(sizeDat);

        	// read m_next
        	uint8_t nextDat[8];
        	(void)stream.read((char*)nextDat, 8);
        	m_next = detail::convertInt8ArrayToInt64(nextDat);

        	stream.close();

        }

        /**
         * @brief in case needing to skip some bytes
         * @param extraOffset the amount to update m_offset by
         */
        void setExtraOffset(uint64_t const extraOffset)
        {
        	m_extraOffset = extraOffset;
        }

        std::streamsize read(char * const buf, std::streamsize const n) const
        {
        	// open the image stream for reading
        	std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			(void)stream.seekg(m_offset + detail::FILE_BLOCK_META + m_extraOffset);
			(void)stream.read((char*)buf, n);

			stream.close();
			return n;
        }

        std::streamsize write(char const * const buf, std::streamsize const n) const
        {
        	// open the image stream for writing
        	std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			(void)stream.seekp(m_offset + detail::FILE_BLOCK_META + m_extraOffset);
			(void)stream.write((char*)buf, n);

			// check if we need to update m_size and m_next. This will be different
			// probably if the number of bytes read is not consistent with the
			// reported size stored in m_size or if the stream has been moved
			// to a position past its start as indicated by m_extraOffset
			if(m_extraOffset > 0) {
				m_size += uint32_t(n);
			}
			if(n < detail::FILE_BLOCK_SIZE - detail::FILE_BLOCK_META || m_extraOffset > 0) {
				if(m_extraOffset == 0) {
					m_size = uint32_t(n); // this already been updated
				}
				m_next = m_index;
	        	(void)stream.seekp(m_offset);

	        	// update m_size
	        	uint8_t sizeDat[4];
	        	detail::convertInt32ToInt4Array(m_size, sizeDat);
	        	(void)stream.write((char*)sizeDat, 4);

	        	// update m_next
	        	uint8_t nextDat[8];
	        	detail::convertUInt64ToInt8Array(m_next, nextDat);
	        	(void)stream.write((char*)nextDat, 8);
			}

            stream.flush();
			stream.close();

			return n;
        }

        uint32_t getDataBytesWritten() const
        {
        	return m_size;
        }

        uint64_t getNextIndex() const
        {
        	return m_next;
        }

        uint64_t getBlockOffset() const
        {
        	return m_offset;
        }

        uint64_t getIndex() const
		{
        	return m_index;
		}

        void setNext(uint64_t const next)
        {
        	m_next = next;
        	std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			(void)stream.seekp(m_offset + 4); // 4 indicate bytes written
			// update m_next
			uint8_t nextDat[8];
			detail::convertUInt64ToInt8Array(m_next, nextDat);
			(void)stream.write((char*)nextDat, 8);
			stream.flush();
			stream.close();
        }

        void registerBlockWithVolumeBitmap()
        {
    		std::fstream stream(m_imagePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    		detail::updateVolumeBitmapWithOne(stream, m_index, m_totalBlocks);
    		stream.close();
        }

    private:

        std::string m_imagePath;
        uint64_t m_totalBlocks;
        uint64_t m_index;
        mutable uint32_t m_size;
        mutable uint64_t m_next;
        mutable uint64_t m_offset;
        mutable uint64_t m_extraOffset;

    };

}

#endif // BFS_FILE_BLOCK_HPP__
