/*
  The MIT License (MIT)

  Copyright (c) 2013 Ben H.D. Jones

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef BFS_FILE_ENTRY_EXCEPTION_HPP__
#define BFS_FILE_ENTRY_EXCEPTION_HPP__

#include <exception>

namespace bfs
{

    enum class FileEntryError { NotReadable, NotWritable };

    class FileEntryException : public std::exception
    {
      public:
        FileEntryException(FileEntryError const &error)
            : m_error(error)
        {

        }

        virtual const char* what() const throw()
        {
            if (m_error == FileEntryError::NotReadable) {
                return "File entry not readable";
            }

            if (m_error == FileEntryError::NotWritable) {
                return "File entry not writable";
            }

            return "File entry unknown error";
        }

        bool operator==(FileEntryException const &other) const
        {
            return m_error == other.m_error;
        }

      private:
        FileEntryError m_error;
    };
}


#endif // BFS_FILE_ENTRY_EXCEPTION_HPP__
