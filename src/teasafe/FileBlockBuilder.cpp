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

#include "teasafe/FileBlockBuilder.hpp"
#include "teasafe/TeaSafeImageStream.hpp"
#include "teasafe/detail/DetailTeaSafe.hpp"

namespace teasafe
{

    FileBlock
    FileBlockBuilder::buildWritableFileBlock(SharedCoreIO const &io,
                                             OpenDisposition const &openDisposition,
                                             bool const enforceRootBlock)
    {

        teasafe::TeaSafeImageStream stream(io, std::ios::in | std::ios::out | std::ios::binary);

        // note building a new block to write to should always be in append mode
        uint64_t id;

        // if the starting file block is enforced, set to root block specified in m_io
        if (enforceRootBlock) {
            id = io->rootBlock;
        } else {
            id = *(detail::getNextAvailableBlock(stream, io->blocks));
        }

        stream.close();
        return FileBlock(io, id, id, openDisposition);
    }

    FileBlock
    FileBlockBuilder::buildFileBlock(SharedCoreIO const &io,
                                     uint64_t const index,
                                     OpenDisposition const &openDisposition)
    {
        return FileBlock(io, index, openDisposition);
    }


}
