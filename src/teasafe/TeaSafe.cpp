/*
  Copyright (c) <2014>, <BenHJ>
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

#include "teasafe/EntryType.hpp"
#include "teasafe/TeaSafe.hpp"
#include "teasafe/TeaSafeException.hpp"


namespace teasafe
{

    TeaSafe::TeaSafe(SharedCoreIO const &io)
        : m_io(io)
        , m_rootFolder(io, io->rootBlock, "root")
    {
    }

    TeaSafeFolder
    TeaSafe::getTeaSafeFolder(std::string const &path)
    {
        TeaSafeFolder rootFolder(m_io, m_io->rootBlock, "root");
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash, but only if folder type
        // an entry of file type should never have a trailing
        // slash and is allowed to fail in this case
        if (ch == '/') {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            return m_rootFolder;
        }

        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(path).filename().string());
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        if (childInfo->type() == EntryType::FileType) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        return parentEntry->getTeaSafeFolder(boost::filesystem::path(path).filename().string());


    }

    EntryInfo
    TeaSafe::getInfo(std::string const &path)
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash, but only if folder type
        // an entry of file type should never have a trailing
        // slash and is allowed to fail in this case
        if (ch == '/') {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        std::string fname = boost::filesystem::path(thePath).filename().string();
        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(thePath).filename().string());

        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        return *childInfo;
    }


    bool
    TeaSafe::fileExists(std::string const &path) const
    {
        return doFileExists(path, m_rootFolder);
    }

    bool
    TeaSafe::folderExists(std::string const &path)
    {
        return doFolderExists(path, m_rootFolder);
    }

    void
    TeaSafe::addFile(std::string const &path)
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // file entries with trailing slash should throw
        if (ch == '/') {
            throw TeaSafeException(TeaSafeError::IllegalFilename);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        // throw if already exists
        throwIfAlreadyExists(path, m_rootFolder);

        parentEntry->addTeaSafeFile(boost::filesystem::path(thePath).filename().string());
    }

    void
    TeaSafe::addFolder(std::string const &path) const
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash
        if (ch == '/') {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        // throw if already exists
        throwIfAlreadyExists(path, m_rootFolder);

        parentEntry->addTeaSafeFolder(boost::filesystem::path(thePath).filename().string());
    }

    void
    TeaSafe::renameEntry(std::string const &src, std::string const &dst)
    {
        std::string srcPath(src);
        char ch = *src.rbegin();
        // ignore trailing slash
        if (ch == '/') {
            std::string(src.begin(), src.end() - 1).swap(srcPath);
        }
        std::string dstPath(dst);
        char chDst = *dst.rbegin();
        // ignore trailing slash
        if (chDst == '/') {
            std::string(dst.begin(), dst.end() - 1).swap(dstPath);
        }

        // throw if source parent doesn't exist
        OptionalTeaSafeFolder parentSrc = doGetParentTeaSafeFolder(srcPath, m_rootFolder);
        if (!parentSrc) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        // throw if destination parent doesn't exist
        OptionalTeaSafeFolder parentDst = doGetParentTeaSafeFolder(dstPath, m_rootFolder);
        if (!parentSrc) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        // throw if destination already exists
        throwIfAlreadyExists(dstPath, m_rootFolder);

        // throw if source doesn't exist
        std::string const filename = boost::filesystem::path(srcPath).filename().string();
        OptionalEntryInfo childInfo = parentSrc->getEntryInfo(filename);
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        // do moving / renaming
        // (i) Remove original entry metadata entry
        // (ii) Add new metadata entry with new file name
        parentSrc->putMetaDataOutOfUse(filename);
        std::string dstFilename = boost::filesystem::path(dstPath).filename().string();
        parentDst->writeNewMetaDataForEntry(dstFilename, childInfo->type(), childInfo->firstFileBlock());

    }

    void
    TeaSafe::removeFile(std::string const &path)
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash, but only if folder type
        // an entry of file type should never have a trailing
        // slash and is allowed to fail in this case
        if (ch == '/') {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(thePath).filename().string());
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        if (childInfo->type() == EntryType::FolderType) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        parentEntry->removeTeaSafeFile(boost::filesystem::path(thePath).filename().string());
    }

    void
    TeaSafe::removeFolder(std::string const &path, FolderRemovalType const &removalType)
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash, but only if folder type
        // an entry of file type should never have a trailing
        // slash and is allowed to fail in this case
        if (ch == '/') {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(thePath).filename().string());
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        if (childInfo->type() == EntryType::FileType) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        if (removalType == FolderRemovalType::MustBeEmpty) {

            TeaSafeFolder childEntry = parentEntry->getTeaSafeFolder(boost::filesystem::path(thePath).filename().string());
            if (!childEntry.listAllEntries().empty()) {
                throw TeaSafeException(TeaSafeError::FolderNotEmpty);
            }
        }

        parentEntry->removeTeaSafeFolder(boost::filesystem::path(thePath).filename().string());
    }

    TeaSafeFileDevice
    TeaSafe::openFile(std::string const &path, OpenDisposition const &openMode)
    {
        char ch = *path.rbegin();
        if (ch == '/') {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(path, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(path).filename().string());
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        if (childInfo->type() == EntryType::FolderType) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        TeaSafeFile fe = parentEntry->getTeaSafeFile(boost::filesystem::path(path).filename().string(), openMode);

        return TeaSafeFileDevice(fe);
    }

    void
    TeaSafe::truncateFile(std::string const &path, std::ios_base::streamoff offset)
    {
        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(path, m_rootFolder);
        if (!parentEntry) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        OptionalEntryInfo childInfo = parentEntry->getEntryInfo(boost::filesystem::path(path).filename().string());
        if (!childInfo) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }
        if (childInfo->type() == EntryType::FolderType) {
            throw TeaSafeException(TeaSafeError::NotFound);
        }

        TeaSafeFile fe = parentEntry->getTeaSafeFile(boost::filesystem::path(path).filename().string(), OpenDisposition::buildOverwriteDisposition());
        fe.truncate(offset);
    }

    /**
     * @brief gets file system info; used when a 'df' command is issued
     * @param buf stores the filesystem stats data
     */
    void
    TeaSafe::statvfs(struct statvfs *buf)
    {
        buf->f_bsize   = detail::FILE_BLOCK_SIZE;
        buf->f_blocks  = m_io->blocks;
        buf->f_bfree   = m_io->freeBlocks;
        buf->f_bavail  = m_io->freeBlocks;

        // in teasafe, the concept of an inode doesn't really exist so the
        // number of inodes is set to corresponds to the number of blocks
        buf->f_files   = m_io->blocks;
        buf->f_ffree   = m_io->freeBlocks;
        buf->f_favail  = m_io->freeBlocks;
        buf->f_namemax = detail::MAX_FILENAME_LENGTH;
    }

    void
    TeaSafe::throwIfAlreadyExists(std::string const &path, TeaSafeFolder &fe) const
    {
        // throw if already exists
        boost::filesystem::path processedPath(path);
        if (doFileExists(processedPath.string(), fe)) {
            throw TeaSafeException(TeaSafeError::AlreadyExists);
        }
        if (doFolderExists(processedPath.string(), fe)) {
            throw TeaSafeException(TeaSafeError::AlreadyExists);
        }
    }

    bool
    TeaSafe::doFileExists(std::string const &path, TeaSafeFolder &fe) const
    {
        return doExistanceCheck(path, EntryType::FileType, fe);
    }

    bool
    TeaSafe::doFolderExists(std::string const &path, TeaSafeFolder &fe) const
    {
        return doExistanceCheck(path, EntryType::FolderType, fe);
    }

    TeaSafe::OptionalTeaSafeFolder
    TeaSafe::doGetParentTeaSafeFolder(std::string const &path, TeaSafeFolder &fe) const
    {
        boost::filesystem::path pathToCheck(path);

        if (pathToCheck.parent_path().string() == "/") {
            return OptionalTeaSafeFolder(fe);
        }

        pathToCheck = pathToCheck.relative_path().parent_path();

        // iterate over path parts extracting sub folders along the way
        boost::filesystem::path::iterator it = pathToCheck.begin();
        TeaSafeFolder folderOfInterest = fe;
        boost::filesystem::path pathBuilder;
        for (; it != pathToCheck.end(); ++it) {

            OptionalEntryInfo entryInfo = folderOfInterest.getEntryInfo(it->string());

            if (!entryInfo) {
                return OptionalTeaSafeFolder();
            }

            pathBuilder /= entryInfo->filename();

            if (pathBuilder == pathToCheck) {

                if (entryInfo->type() == EntryType::FolderType) {
                    return OptionalTeaSafeFolder(folderOfInterest.getTeaSafeFolder(entryInfo->filename()));
                } else {
                    return OptionalTeaSafeFolder();
                }
            }
            // recurse deeper
            folderOfInterest = folderOfInterest.getTeaSafeFolder(entryInfo->filename());
        }


        return OptionalTeaSafeFolder();

    }

    bool
    TeaSafe::doExistanceCheck(std::string const &path, EntryType const &entryType, TeaSafeFolder &fe) const
    {
        std::string thePath(path);
        char ch = *path.rbegin();
        // ignore trailing slash, but only if folder type
        // an entry of file type should never have a trailing
        // slash and is allowed to fail in this case
        if (ch == '/' && entryType == EntryType::FolderType) {
            std::string(path.begin(), path.end() - 1).swap(thePath);
        }

        OptionalTeaSafeFolder parentEntry = doGetParentTeaSafeFolder(thePath, fe);
        if (!parentEntry) {
            return false;
        }

        std::string filename = boost::filesystem::path(thePath).filename().string();

        OptionalEntryInfo entryInfo = parentEntry->getEntryInfo(filename);

        if (!entryInfo) {
            return false;
        }

        if (entryInfo->type() == entryType) {
            return true;
        }

        return false;
    }

}
