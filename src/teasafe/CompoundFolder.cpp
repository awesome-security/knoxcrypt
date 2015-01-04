/*
  Copyright (c) <2015>, <BenHJ>
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

#include "teasafe/CompoundFolder.hpp"

#include <sstream>

namespace teasafe
{

    // number of entries a bucket (content) folder is permitted to have
    #define CONTENT_SIZE 50

    CompoundFolder::CompoundFolder(SharedCoreIO const &io,
                                   std::string const &name,
                                   bool const enforceRootBlock)
      : m_compoundFolder(std::make_shared<ContentFolder>(io, name, enforceRootBlock))
      , m_ContentFolders()
      , m_name(name)
      , m_ContentFolderCount(m_compoundFolder->getEntryCount())
      , m_cache()
      , m_cacheShouldBeUpdated(true)
    {
        doPopulateContentFolders();
    }

    CompoundFolder::CompoundFolder(SharedCoreIO const &io,
                                   uint64_t const startBlock,
                                   std::string const &name)
      : m_compoundFolder(std::make_shared<ContentFolder>(io, startBlock, name))
      , m_ContentFolders()
      , m_name(name)
      , m_ContentFolderCount(m_compoundFolder->getEntryCount())
      , m_cache()
      , m_cacheShouldBeUpdated(true)
    {
        doPopulateContentFolders();
    }

    void 
    CompoundFolder::doPopulateContentFolders()
    {
        if(m_ContentFolderCount > 0) {
            auto folderInfos(m_compoundFolder->listFolderEntries());
            for(auto const & f : folderInfos) {
                m_ContentFolders.push_back(m_compoundFolder->getContentFolder(f->filename()));
            }
        }
    }

    void 
    CompoundFolder::doAddContentFolder()
    {
        std::ostringstream ss;
        ss << "index_" << m_ContentFolderCount;
        m_compoundFolder->addContentFolder(ss.str());
        m_ContentFolders.push_back(m_compoundFolder->getContentFolder(ss.str()));
        ++m_ContentFolderCount;
    }

    void 
    CompoundFolder::addFile(std::string const &name)
    {
        // check if compound entries is empty. These are
        // compound 'leaf' sub-folders
        if(m_ContentFolders.empty()) {
            doAddContentFolder();
        }

        // each leaf folder can have CONTENT_SIZE entries
        bool wasAdded = false;
        for(auto & f : m_ContentFolders) {
            if(f->getEntryCount() < CONTENT_SIZE || f->anOldSpaceIsAvailableForNewEntry()) {
                f->addFile(name);
                wasAdded = true;
            }
        }

        // wasn't added. Means that there wasn't room so create
        // another leaf folder
        if(!wasAdded) {
            doAddContentFolder();
            m_ContentFolders.back()->addFile(name);
        }

        m_cacheShouldBeUpdated = true;
    }

    void
    CompoundFolder::addFolder(std::string const &name)
    {
        // check if compound entries is empty. These are
        // compound 'leaf' sub-folders
        if(m_ContentFolders.empty()) {
            doAddContentFolder();
        }

        // each leaf folder can have CONTENT_SIZE entries
        bool wasAdded = false;
        for(auto & f : m_ContentFolders) {
            if(f->getEntryCount() < CONTENT_SIZE || f->anOldSpaceIsAvailableForNewEntry()) {
                f->addCompoundFolder(name);
                wasAdded = true;
            }
        }

        // wasn't added. Means that there wasn't room so create
        // another leaf folder
        if(!wasAdded) {
            doAddContentFolder();
            m_ContentFolders.back()->addCompoundFolder(name);
        }

        m_cacheShouldBeUpdated = true;
    }

    File 
    CompoundFolder::getFile(std::string const &name,
                            OpenDisposition const &openDisposition) const
    {
        for(auto & f : m_ContentFolders) {
            auto file(f->getFile(name, openDisposition));
            if(file) {
                return *file;
            }
        }
        throw std::runtime_error("File not found");
    }

    std::shared_ptr<CompoundFolder>
    CompoundFolder::getFolder(std::string const &name) const
    {
        for(auto & f : m_ContentFolders) {
            auto folder(f->getCompoundFolder(name));
            if(folder) {
                return folder;
            }
        }
        throw std::runtime_error("Compound folder not found");
    }

    std::shared_ptr<ContentFolder>
    CompoundFolder::getCompoundFolder() const
    {
        return m_compoundFolder;
    }

    std::string 
    CompoundFolder::getName() const
    {
        return m_name;
    }

    SharedEntryInfo 
    CompoundFolder::getEntryInfo(std::string const &name) const
    {
        // try and pull out of cache fisrt
        auto it = m_cache.find(name);
        if(it != m_cache.end()) {
            return it->second;
        }

        for(auto const & f : m_ContentFolders) {
            auto info(f->getEntryInfo(name));
            if(info) { 
                if(m_cache.find(name) == m_cache.end()) {
                    m_cache.insert(std::make_pair(name, info));
                }
                return info; 
            }
        }
        return SharedEntryInfo();
    }

    EntryInfoCacheMap &
    CompoundFolder::listAllEntries() const
    {
        if(m_cacheShouldBeUpdated) {
            for(auto const & f : m_ContentFolders) {
                auto & leafEntries(f->listAllEntries());
                for(auto const & entry : leafEntries) {
                    if(m_cache.find(entry.first) == m_cache.end()) {
                        m_cache.insert(entry);
                    }
                }
            }
            m_cacheShouldBeUpdated = false;
        }

        return m_cache;
    }

    std::vector<SharedEntryInfo> 
    CompoundFolder::listFileEntries() const
    {
        std::vector<SharedEntryInfo> infos;
        for(auto const & f : m_ContentFolders) {
            auto leafEntries(f->listFileEntries());
            for(auto const & entry : leafEntries) {
                if(m_cache.find(entry->filename()) == m_cache.end()) {
                    m_cache.insert(std::make_pair(entry->filename(), entry));
                }
                infos.push_back(entry);
            }
        }
        return infos;
    }

    std::vector<SharedEntryInfo> 
    CompoundFolder::listFolderEntries() const
    {
        std::vector<SharedEntryInfo> infos;
        for(auto const & f : m_ContentFolders) {
            auto leafEntries(f->listFolderEntries());
            for(auto const & entry : leafEntries) {
                if(m_cache.find(entry->filename()) == m_cache.end()) {
                    m_cache.insert(std::make_pair(entry->filename(), entry));
                }
                infos.push_back(entry);
            }
        }
        return infos;
    }

    void 
    CompoundFolder::doRemoveEntryFromCache(std::string const &name)
    {
        auto it = m_cache.find(name);
        if(it != m_cache.end()) {
            m_cache.erase(it);
        }
    }

    void 
    CompoundFolder::removeFile(std::string const &name)
    {
        for(auto & f : m_ContentFolders) {
            if(f->removeFile(name)) {                
                // decrement number of entries in leaf
                if(f->getEntryCount() == 0) {
                    m_compoundFolder->removeContentFolder(f->getName());
                }
                doRemoveEntryFromCache(name);
                return; 
            }
        }
        throw std::runtime_error("Error removing: file not found");
    }

    void 
    CompoundFolder::removeFolder(std::string const &name)
    {
        for(auto & f : m_ContentFolders) {
            if(f->removeCompoundFolder(name)) { 
                // decrement number of entries in leaf
                if(f->getEntryCount() == 0) {
                    m_compoundFolder->removeContentFolder(f->getName());
                }
                doRemoveEntryFromCache(name);
                return;
            }
        }
        throw std::runtime_error("Error removing: folder not found");
    }

    void
    CompoundFolder::putMetaDataOutOfUse(std::string const &name)
    {
        for(auto & f : m_ContentFolders) {
            if(f->putMetaDataOutOfUse(name)) { 
                doRemoveEntryFromCache(name);
                return; 
            }
        }
        throw std::runtime_error("Error putting metadata out of use");
    }

    void
    CompoundFolder::writeNewMetaDataForEntry(std::string const &name,
                                             EntryType const &entryType,
                                             uint64_t startBlock)
    {
        // each leaf folder can have CONTENT_SIZE entries
        bool wasAdded = false;
        for(auto & f : m_ContentFolders) {
            if(f->getEntryCount() < CONTENT_SIZE || f->anOldSpaceIsAvailableForNewEntry()) {
                f->writeNewMetaDataForEntry(name, entryType, startBlock);
                wasAdded = true;
            }
        }

        // wasn't added. Means that there wasn't room so create
        // another leaf folder
        if(!wasAdded) {
            doAddContentFolder();
            m_ContentFolders.back()->writeNewMetaDataForEntry(name, entryType, startBlock);
        }
    }
}