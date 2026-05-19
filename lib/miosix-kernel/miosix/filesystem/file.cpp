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

#include "file.h"
#include <cstdio>
#include <string>
#include <fcntl.h>
#include "file_access.h"
#include "config/miosix_settings.h"

using namespace std;

namespace miosix {

//This is a static assert. The filesystem code assumes off_t is 64bit.
typedef char check_sizeof_off_t[sizeof(off_t)==8 ? 1 : -1];

//
// class FileBase
//

FileBase::FileBase(intrusive_ref_ptr<FilesystemBase> parent) : parent(parent)
{
    if(parent) parent->newFileOpened();
}

#ifdef WITH_FILESYSTEM

int FileBase::isatty() const
{
    return 0;
}

int FileBase::fcntl(int cmd, int opt)
{
    //Newlib makes some calls to fcntl, for example in opendir(). CLOEXEC isn't
    //supported, but for now we lie and return 0
    if(cmd==F_SETFD && (opt==FD_CLOEXEC || opt==0)) return 0;
    return -EBADF;
}

int FileBase::ioctl(int cmd, void *arg)
{
    return -ENOTTY; //Means the operation does not apply to this descriptor
}

int FileBase::getdents(void *dp, int len)
{
    return -EBADF;
}

#endif //WITH_FILESYSTEM

FileBase::~FileBase()
{
    if(parent) parent->fileCloseHook();
}

//
// class DirectoryBase
//

ssize_t DirectoryBase::write(const void *data, size_t len)
{
    return -EBADF;
}
    
ssize_t DirectoryBase::read(void *data, size_t len)
{
    return -EBADF;
}

off_t DirectoryBase::lseek(off_t pos, int whence)
{
    return -EBADF;
}

int DirectoryBase::fstat(struct stat *pstat) const
{
    return -EBADF;
}

int DirectoryBase::addEntry(char **pos, char *end, int ino, char type,
        const StringPart& n)
{
    int reclen=direntHeaderSizeNoPadding+n.length()+1;
    reclen=(reclen+3) & ~0x3; //Align to 4 bytes
    if(reclen>end-*pos) return -1;
    
    struct dirent *data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=ino;
    data->d_off=0;
    data->d_reclen=reclen;
    data->d_type=type;
    strcpy(data->d_name,n.c_str());
    
    (*pos)+=reclen;
    return reclen;
}

int DirectoryBase::addDefaultEntries(char **pos, int thisIno, int upIno)
{
    struct dirent *data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=thisIno;
    data->d_off=0;
    data->d_reclen=direntHeaderSize;
    data->d_type=DT_DIR;
    strcpy(data->d_name,".");
    
    (*pos)+=direntHeaderSize;
    data=reinterpret_cast<struct dirent*>(*pos);
    data->d_ino=upIno;
    data->d_off=0;
    data->d_reclen=direntHeaderSize;
    data->d_type=DT_DIR;
    strcpy(data->d_name,"..");
    
    (*pos)+=direntHeaderSize;
    return 2*direntHeaderSize;
}

int DirectoryBase::addTerminatingEntry(char **pos, char *end)
{
    if(direntHeaderSize>end-*pos) return -1;
    //This sets everything to zero, including d_reclen, terminating the
    //directory listing loop in readdir.c
    memset(*pos,0,direntHeaderSize);
    (*pos)+=direntHeaderSize;
    return direntHeaderSize;
}

//
// class FilesystemBase
//

FilesystemBase::FilesystemBase() :
#ifdef WITH_FILESYSTEM
        filesystemId(FilesystemManager::getFilesystemId()), 
#else //WITH_FILESYSTEM
        filesystemId(0),
#endif //WITH_FILESYSTEM
        parentFsMountpointInode(1), openFileCount(0) {}

int FilesystemBase::readlink(StringPart& name, string& target)
{
    return -EINVAL; //Default implementation, for filesystems without symlinks
}

bool FilesystemBase::supportsSymlinks() const { return false; }

void FilesystemBase::newFileOpened() { atomicAdd(&openFileCount,1); }

void FilesystemBase::fileCloseHook()
{
    #ifdef WITH_ERRLOG
    int result=atomicAddExchange(&openFileCount,-1);
    assert(result>=0);
    #else //WITH_ERRLOG
    atomicAdd(&openFileCount,-1);
    #endif //WITH_ERRLOG
}

FilesystemBase::~FilesystemBase() {}

} //namespace miosix
