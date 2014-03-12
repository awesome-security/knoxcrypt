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

#ifndef TeaSafe_TeaSafe_EXCEPTION_HPP__
#define TeaSafe_TeaSafe_EXCEPTION_HPP__

#include <exception>

namespace teasafe
{

    enum class TeaSafeError { NotFound, AlreadyExists, IllegalFilename, FolderNotEmpty };

    class TeaSafeException : public std::exception
    {
      public:
        TeaSafeException(TeaSafeError const &error)
        : m_error(error)
        {

        }

        virtual const char* what() const throw()
        {
            if (m_error == TeaSafeError::NotFound) {
                return "TeaSafe: Entry not found";
            }

            if (m_error == TeaSafeError::AlreadyExists) {
                return "TeaSafe: Entry already exists";
            }

            if (m_error == TeaSafeError::IllegalFilename) {
                return "TeaSafe: illegal filename";
            }

            if (m_error == TeaSafeError::FolderNotEmpty) {
                return "TeaSafe: Folder not empty";
            }

            return "TeaSafe: Unknown error";
        }

        bool operator==(TeaSafeException const &other) const
        {
            return m_error == other.m_error;
        }

      private:
        TeaSafeError m_error;
    };
}


#endif // TeaSafe_TeaSafe_EXCEPTION_HPP__
