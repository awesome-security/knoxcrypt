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

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#ifndef BFS_FILE_BLOCK_EXCEPTION_HPP__
#define BFS_FILE_BLOCK_EXCEPTION_HPP__

#include <exception>

namespace bfs
{

    enum class FileBlockError { NotReadable, NotWritable };

    class FileBlockException : public std::exception
    {
      public:
        FileBlockException(FileBlockError const &error)
            : m_error(error)
        {

        }

        virtual const char* what() const throw()
        {
            if (m_error == FileBlockError::NotReadable) {
                return "File block not readable";
            }

            if (m_error == FileBlockError::NotWritable) {
                return "File block not writable";
            }

            return "File block unknown error";
        }

        bool operator==(FileBlockException const &other) const
        {
            return m_error == other.m_error;
        }

      private:
        FileBlockError m_error;
    };
}


#endif // BFS_FILE_BLOCK_EXCEPTION_HPP__
