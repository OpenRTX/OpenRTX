/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "mountpointfs.h"
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include "filesystem/stringpart.h"

using namespace std;

namespace miosix {
    
#ifdef WITH_FILESYSTEM

/**
 * Directory class for MountpointFs 
 */
class MountpointFsDirectory : public DirectoryBase
{
public:
    /**
     * \param parent parent filesystem
     * \param mutex mutex to lock when accessing the file map
     * \param dirs file map
     * \param root true if we're listing the root directory
     * \param currentInode inode of the directory we're listing
     * \param parentInode inode of the parent directory
     */
    MountpointFsDirectory(intrusive_ref_ptr<FilesystemBase> parent,
            FastMutex& mutex, map<StringPart,int>& dirs, bool root,
            int currentInode, int parentInode)
            : DirectoryBase(parent), mutex(mutex), dirs(dirs),
              currentInode(currentInode), parentInode(parentInode),
              first(true), last(false)
    {
        Lock<FastMutex> l(mutex);
        if(root && dirs.empty()==false) currentItem=dirs.begin()->first.c_str();
    }
    
    /**
     * Also directories can be opened as files. In this case, this system call
     * allows to retrieve directory entries.
     * \param dp pointer to a memory buffer where one or more struct dirent
     * will be placed. dp must be four words aligned.
     * \param len memory buffer size.
     * \return the number of bytes read on success, or a negative number on
     * failure.
     */
    virtual int getdents(void *dp, int len);
    
private:
    FastMutex& mutex;                 ///< Mutex of parent class
    std::map<StringPart,int>& dirs;   ///< Directory entries of parent class
    string currentItem;               ///< First unhandled item in directory
    int currentInode,parentInode;     ///< Inodes of . and ..

    bool first; ///< True if first time getdents is called
    bool last;  ///< True if directory has ended
};

//
// class MountpointFsDirctory
//

int MountpointFsDirectory::getdents(void *dp, int len)
{
    if(len<minimumBufferSize) return -EINVAL;
    if(last) return 0;
    
    Lock<FastMutex> l(mutex);
    char *begin=reinterpret_cast<char*>(dp);
    char *buffer=begin;
    char *end=buffer+len;
    if(first)
    {
        first=false;
        addDefaultEntries(&buffer,currentInode,parentInode);
    }
    if(currentItem.empty()==false)
    {
        map<StringPart,int>::iterator it=dirs.find(StringPart(currentItem));
        //Someone deleted the exact directory entry we had saved (unlikely)
        if(it==dirs.end()) return -EBADF;
        for(;it!=dirs.end();++it)
        {
            if(addEntry(&buffer,end,it->second,DT_DIR,it->first)>0) continue;
            //Buffer finished
            currentItem=it->first.c_str();
            return buffer-begin;
        }
    }
    addTerminatingEntry(&buffer,end);
    last=true;
    return buffer-begin;
}

//
// class MountpointFs
//

int MountpointFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    if(flags & (O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC))
        return -EACCES;
    
    Lock<FastMutex> l(mutex);
    int currentInode=rootDirInode;
    int parentInode=parentFsMountpointInode;
    if(name.empty()==false)
    {
        map<StringPart,int>::iterator it=dirs.find(name);
        if(it==dirs.end()) return -EACCES;
        parentInode=currentInode;
        currentInode=it->second;
    }
    file=intrusive_ref_ptr<FileBase>(
        new MountpointFsDirectory(
            shared_from_this(),mutex,dirs,name.empty(),currentInode,parentInode));
    return 0;
}

int MountpointFs::lstat(StringPart& name, struct stat *pstat)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,int>::iterator it;
    if(name.empty()==false)
    {
        it=dirs.find(name);
        if(it==dirs.end()) return -ENOENT;
    }
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=filesystemId;
    pstat->st_ino=name.empty() ? rootDirInode : it->second;
    pstat->st_mode=S_IFDIR | 0755; //drwxr-xr-x
    pstat->st_nlink=1;
    pstat->st_blksize=512;
    return 0;
}

int MountpointFs::unlink(StringPart& name)
{
    return -ENOENT;
}

int MountpointFs::rename(StringPart& oldName, StringPart& newName)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,int>::iterator it=dirs.find(oldName);
    if(it==dirs.end()) return -ENOENT;
    for(unsigned int i=0;i<newName.length();i++)
        if(newName[i]=='/')
            return -EACCES; //MountpointFs does not support subdirectories
    dirs[newName]=it->second;
    dirs.erase(it);
    return 0;
}

int MountpointFs::mkdir(StringPart& name, int mode)
{
    for(unsigned int i=0;i<name.length();i++)
        if(name[i]=='/')
            return -EACCES; //MountpointFs does not support subdirectories
    Lock<FastMutex> l(mutex);
    if(dirs.insert(make_pair(name,inodeCount)).second==false) return -EEXIST;
    inodeCount++;
    return 0;
}

int MountpointFs::rmdir(StringPart& name)
{
    Lock<FastMutex> l(mutex);
    if(dirs.erase(name)==1) return 0;
    return -ENOENT;
}

#endif //WITH_FILESYSTEM

} //namespace miosix
