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

#include "fat32.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <cstdio>
#include "filesystem/stringpart.h"
#include "filesystem/ioctl.h"
#include "util/unicode.h"

using namespace std;

namespace miosix {
    
#ifdef WITH_FILESYSTEM

/**
 * Translate between FATFS error codes and POSIX ones
 * \param ec FATS error code
 * \return POSIX error code
 */
static int translateError(int ec)
{
    switch(ec)
    {
        case FR_OK:
            return 0;
        case FR_NO_FILE:
        case FR_NO_PATH:
            return -ENOENT;
        case FR_DENIED:
            return -ENOSPC;
        case FR_EXIST:
            return -EEXIST;
        case FR_WRITE_PROTECTED:
            return -EROFS;
        case FR_LOCKED:
            return -EBUSY;
        case FR_NOT_ENOUGH_CORE:
            return -ENOMEM;
        case FR_TOO_MANY_OPEN_FILES:
            return -ENFILE;
        default:
            return -EACCES;
    }
}

/**
 * Directory class for Fat32Fs
 */
class Fat32Directory : public DirectoryBase
{
public:
    /**
     * \param parent parent filesystem
     * \param mutex mutex to lock when accessing the fiesystem
     * \param currentInode inode value for '.' entry
     * \param parentInode inode value for '..' entry
     */
    Fat32Directory(intrusive_ref_ptr<FilesystemBase> parent, FastMutex& mutex,
            int currentInode, int parentInode) : DirectoryBase(parent),
            mutex(mutex), currentInode(currentInode), parentInode(parentInode),
            first(true), unfinished(false)
    {
        //Make sure a closedir of an uninitialized dir won't do any damage
        dir.fs=0;
        fi.lfname=lfn;
        fi.lfsize=sizeof(lfn);
    }
    
    /**
     * \return the underlying directory object 
     */
    DIR_ *directory() { return &dir; }
    
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
    
    /**
     * Destructor
     */
    virtual ~Fat32Directory();
    
private:
    FastMutex& mutex;  ///< Parent filesystem's mutex
    DIR_ dir;          ///< Directory object
    FILINFO fi;        ///< Information on a file
    int currentInode;  ///< Inode of '.'
    int parentInode;   ///< Inode of '..'
    bool first;        ///< To display '.' and '..' entries
    bool unfinished;   ///< True if fi contains unread data
    char lfn[(_MAX_LFN+1)*2]; ///< Long file name
};

//
// class Fat32Directory
//

int Fat32Directory::getdents(void *dp, int len)
{
    if(len<minimumBufferSize) return -EINVAL;
    char *begin=reinterpret_cast<char*>(dp);
    char *buffer=begin;
    char *end=buffer+len;
    
    Lock<FastMutex> l(mutex);
    if(first)
    {
        first=false;
        addDefaultEntries(&buffer,currentInode,parentInode);
    }
    if(unfinished)
    {
        unfinished=false;
        char type=fi.fattrib & AM_DIR ? DT_DIR : DT_REG;
        StringPart name(fi.lfname);
        if(addEntry(&buffer,end,fi.inode,type,name)<0) return -EINVAL;
    }
    for(;;)
    {
        if(int res=translateError(f_readdir(&dir,&fi))) return res;
        if(fi.fname[0]=='\0')
        {
            addTerminatingEntry(&buffer,end);
            return buffer-begin;
        }
        char type=fi.fattrib & AM_DIR ? DT_DIR : DT_REG;
        StringPart name(fi.lfname);
        if(addEntry(&buffer,end,fi.inode,type,name)<0)
        {
            unfinished=true;
            return buffer-begin;
        }
    }
}

Fat32Directory::~Fat32Directory()
{
    Lock<FastMutex> l(mutex);
    f_closedir(&dir);
}

/**
 * Files of the Fat32Fs filesystem
 */
class Fat32File : public FileBase
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    Fat32File(intrusive_ref_ptr<FilesystemBase> parent, FastMutex& mutex);
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len);
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len);
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence);
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);
    
    /**
     * \return the FatFs FIL object 
     */
    FIL *fil() { return &file; }
    
    /**
     * \param inode file inode
     */
    void setInode(int inode) { this->inode=inode; }
    
    /**
     * Destructor
     */
    ~Fat32File();
    
private:
    FIL file;
    FastMutex& mutex;
    int inode;
};

//
// class Fat32File
//

Fat32File::Fat32File(intrusive_ref_ptr<FilesystemBase> parent, FastMutex& mutex)
        : FileBase(parent), mutex(mutex), inode(0) {}

ssize_t Fat32File::write(const void *data, size_t len)
{
    Lock<FastMutex> l(mutex);
    unsigned int bytesWritten;
    if(int res=translateError(f_write(&file,data,len,&bytesWritten))) return res;
    #ifdef SYNC_AFTER_WRITE
    if(f_sync(&file)!=FR_OK) return -EIO;
    #endif //SYNC_AFTER_WRITE    
    return static_cast<int>(bytesWritten);
}

ssize_t Fat32File::read(void *data, size_t len)
{
    Lock<FastMutex> l(mutex);
    unsigned int bytesRead;
    if(int res=translateError(f_read(&file,data,len,&bytesRead))) return res;
    return static_cast<int>(bytesRead);
}

off_t Fat32File::lseek(off_t pos, int whence)
{
    Lock<FastMutex> l(mutex);
    off_t offset;
    switch(whence)
    {
        case SEEK_CUR:
            offset=static_cast<off_t>(f_tell(&file))+pos;
            break;
        case SEEK_SET:
            offset=pos;
            break;
        case SEEK_END:
            offset=static_cast<off_t>(f_size(&file))+pos;
            break;
        default:
            return -EINVAL;
    }
    //We don't support seek past EOF for Fat32
    if(offset<0 || offset>static_cast<off_t>(f_size(&file))) return -EOVERFLOW;
    if(int result=translateError(
        f_lseek(&file,static_cast<unsigned long>(offset)))) return result;
    return offset;
}

int Fat32File::fstat(struct stat *pstat) const
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=getParent()->getFsId();
    pstat->st_ino=inode;
    pstat->st_mode=S_IFREG | 0755; //-rwxr-xr-x
    pstat->st_nlink=1;
    pstat->st_size=f_size(&file);
    pstat->st_blksize=512;
    pstat->st_blocks=(static_cast<off_t>(f_size(&file))+511)/512;
    return 0;
}

int Fat32File::ioctl(int cmd, void *arg)
{
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<FastMutex> l(mutex);
    return translateError(f_sync(&file));
}

Fat32File::~Fat32File()
{
    Lock<FastMutex> l(mutex);
    if(inode) f_close(&file); //TODO: what to do with error code?
}

//
// class Fat32Fs
//

Fat32Fs::Fat32Fs(intrusive_ref_ptr<FileBase> disk)
        : mutex(FastMutex::RECURSIVE), failed(true)
{
    filesystem.drv=disk;
    failed=f_mount(&filesystem,1,false)!=FR_OK;
}

int Fat32Fs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
        int flags, int mode)
{
    if(failed) return -ENOENT;
    flags++; //To convert from O_RDONLY, O_WRONLY, ... to _FREAD, _FWRITE, ...
    
    // Code path checklist:
    // Not existent | Regular file | Directory |
    //      ok      |      ok      |    ok     | _FREAD
    //      ok      |      ok      |    ok     | _FWRITE
    //      ok      |      ok      |    ok     | _FWRITE | _FCREAT
    
    struct stat st;
    bool statFailed=false;
    if(int result=lstat(name,&st))
    {
        //If _FCREAT the file may not yet exist as we are asked to create it
        if((flags & (_FWRITE | _FCREAT)) != (_FWRITE | _FCREAT)) return result;
        else statFailed=true;
    }
        
    //Using if short circuit, as st is not initialized if statFailed
    if(statFailed || !S_ISDIR(st.st_mode))
    {
        //About to open a file
        BYTE openflags=0;
        if(flags & _FREAD)  openflags|=FA_READ;
        if(flags & _FWRITE) openflags|=FA_WRITE;
        if(flags & _FTRUNC) openflags|=FA_CREATE_ALWAYS;//Truncate
        else if(flags & _FCREAT) openflags|=FA_OPEN_ALWAYS;//If !exists create
        else openflags|=FA_OPEN_EXISTING;//If not exists fail

        intrusive_ref_ptr<Fat32File> f(new Fat32File(shared_from_this(),mutex));
        Lock<FastMutex> l(mutex);
        if(int res=translateError(f_open(&filesystem,f->fil(),name.c_str(),openflags)))
            return res;
        if(statFailed)
        {
            //If we didn't stat before, stat now to get the inode
            if(int result=lstat(name,&st)) return result;
        }
        f->setInode(st.st_ino);

        //Can't open files larger than INT_MAX
        if(static_cast<int>(f_size(f->fil()))<0) return -EOVERFLOW;

        #ifdef SYNC_AFTER_WRITE
        if(f_sync(f->fil())!=FR_OK) return -EFAULT;
        #endif //SYNC_AFTER_WRITE

        //If file opened for appending, seek to end of file
        if(flags & _FAPPEND)
            if(f_lseek(f->fil(),f_size(f->fil()))!=FR_OK) return -EFAULT;

        file=f;
        return 0;
    } else {
        //About to open a directory
        if(flags & (_FWRITE | _FAPPEND | _FCREAT | _FTRUNC)) return -EISDIR;
        
        int parentInode;
        if(name.empty()==false)
        {
            unsigned int lastSlash=name.findLastOf('/');
            if(lastSlash!=string::npos)
            {
                StringPart parent(name,lastSlash);
                struct stat st2;
                if(int result=lstat(parent,&st2)) return result;
                parentInode=st2.st_ino;
            } else parentInode=1; //Asked to list subdir of root
        } else parentInode=parentFsMountpointInode; //Asked to list root dir
        
        
        intrusive_ref_ptr<Fat32Directory> d(
            new Fat32Directory(shared_from_this(),mutex,st.st_ino,parentInode));
         
        Lock<FastMutex> l(mutex);
        if(int res=translateError(f_opendir(&filesystem,d->directory(),name.c_str())))
            return res;
         
         file=d;
         return 0;
    }
}

int Fat32Fs::lstat(StringPart& name, struct stat *pstat)
{
    if(failed) return -ENOENT;
    memset(pstat,0,sizeof(struct stat));
    pstat->st_dev=filesystemId;
    pstat->st_nlink=1;
    pstat->st_blksize=512;
    
    Lock<FastMutex> l(mutex);
    if(name.empty())
    {
        //We are asked to stat the filesystem's root directory
        //By convention, we use 1 for root dir inode, see INODE() macro in ff.c
        pstat->st_ino=1;
        pstat->st_mode=S_IFDIR | 0755;  //drwxr-xr-x
        return 0;
    }
    FILINFO info;
    info.lfname=0; //We're not interested in getting the lfname
    info.lfsize=0;
    if(int result=translateError(f_stat(&filesystem,name.c_str(),&info))) return result;
    
    pstat->st_ino=info.inode;
    pstat->st_mode=(info.fattrib & AM_DIR) ?
        S_IFDIR | 0755  //drwxr-xr-x
      : S_IFREG | 0755; //-rwxr-xr-x
    pstat->st_size=info.fsize;
    pstat->st_blocks=(info.fsize+511)/512;
    return 0;
}

int Fat32Fs::unlink(StringPart& name)
{
    return unlinkRmdirHelper(name,false);
}

int Fat32Fs::rename(StringPart& oldName, StringPart& newName)
{
    if(failed) return -ENOENT;
    Lock<FastMutex> l(mutex);
    return translateError(f_rename(&filesystem,oldName.c_str(),newName.c_str()));
}

int Fat32Fs::mkdir(StringPart& name, int mode)
{
    if(failed) return -ENOENT;
    Lock<FastMutex> l(mutex);
    return translateError(f_mkdir(&filesystem,name.c_str()));
}

int Fat32Fs::rmdir(StringPart& name)
{
    return unlinkRmdirHelper(name,true);
}

Fat32Fs::~Fat32Fs()
{
    if(failed) return;
    f_mount(&filesystem,0,true); //TODO: what to do with error code?
    filesystem.drv->ioctl(IOCTL_SYNC,0);
    filesystem.drv.reset();
}

int Fat32Fs::unlinkRmdirHelper(StringPart& name, bool delDir)
{
    if(failed) return -ENOENT;
    Lock<FastMutex> l(mutex);
    struct stat st;
    if(int result=lstat(name,&st)) return result;
    if(delDir)
    {
        if(!S_ISDIR(st.st_mode)) return -ENOTDIR;
    } else if(S_ISDIR(st.st_mode)) return -EISDIR;
    return translateError(f_unlink(&filesystem,name.c_str()));
}

#endif //WITH_FILESYSTEM

} //namespace miosix
