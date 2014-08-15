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

#ifndef TeaSafe_FILE_BLOCK_BUILDER_HPP__
#define TeaSafe_FILE_BLOCK_BUILDER_HPP__

#include "teasafe/CoreTeaSafeIO.hpp"
#include "teasafe/FileBlock.hpp"
#include "teasafe/OpenDisposition.hpp"

#include <boost/shared_ptr.hpp>

#include <deque>

namespace teasafe
{

    class FileBlockBuilder;
    typedef boost::shared_ptr<FileBlockBuilder> SharedBlockBuilder;
    typedef std::deque<uint64_t> BlockDeque;

    class FileBlockBuilder
    {
      public:
        FileBlockBuilder();
        FileBlockBuilder(SharedCoreIO const &io);

        FileBlock buildWritableFileBlock(SharedCoreIO const &io,
                                         OpenDisposition const &openDisposition,
                                         SharedImageStream &stream,
                                         bool const enforceRootBlock = false);

        FileBlock buildFileBlock(SharedCoreIO const &io,
                                 uint64_t const index,
                                 OpenDisposition const &openDisposition,
                                 SharedImageStream &stream);

        void putBlockBack(uint64_t const);

      private:

        BlockDeque m_blockDeque;

        /// store how many blocks have actually been written
        /// when we get a block to use if it is greater than the number
        /// of blocks written then image is probably sparse in which case
        /// the block needs to be written
        uint64_t m_blocksWritten;

    };

}

#endif
