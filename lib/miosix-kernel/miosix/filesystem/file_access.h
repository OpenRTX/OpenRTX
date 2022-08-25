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

#ifndef FILE_ACCESS_H
#define FILE_ACCESS_H

#include <map>
#include <list>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include "file.h"
#include "stringpart.h"
#include "devfs/devfs.h"
#include "kernel/sync.h"
#include "kernel/intrusive.h"
#include "config/miosix_settings.h"

#ifdef WITH_FILESYSTEM

namespace miosix {

/**
 * The result of resolvePath().
 */
class ResolvedPath
{
public:
    /**
     * Constructor
     */
    ResolvedPath() : result(-EINVAL), fs(0), off(0) {}
    
    /**
     * Constructor
     * \param result error code
     */
    explicit ResolvedPath(int result) : result(result), fs(0), off(0) {}
    
    /**
     * Constructor
     * \param fs filesystem
     * \param off offset into path where the subpath relative to the current
     * filesystem starts
     */
    ResolvedPath(intrusive_ref_ptr<FilesystemBase> fs, size_t offset)
            : result(0), fs(fs), off(offset) {}
    
    int result; ///< 0 on success, a negative number on failure
    intrusive_ref_ptr<FilesystemBase> fs; ///< pointer to the filesystem to which the file belongs
    /// path.c_str()+off is a string containing the relative path into the
    /// filesystem for the looked up file
    size_t off;
};

/**
 * This class maps file descriptors to file objects, allowing to
 * perform file operations
 */
class FileDescriptorTable
{
public:
    /**
     * Constructor
     */
    FileDescriptorTable();
    
    /**
     * Copy constructor
     * \param rhs object to copy from
     */
    FileDescriptorTable(const FileDescriptorTable& rhs);
    
    /**
     * Operator=
     * \param rhs object to copy from
     * \return *this 
     */
    FileDescriptorTable& operator=(const FileDescriptorTable& rhs);
    
    /**
     * Open a file
     * \param name file name
     * \param flags file open mode
     * \param mode allows to set file permissions
     * \return a file descriptor, or a negative number on error
     */
    int open(const char *name, int flags, int mode);
    
    /**
     * Close a file
     * \param fd file descriptor to close
     * \return 0 on success or a negative number on failure
     */
    int close(int fd);
    
    /**
     * Close all files
     */
    void closeAll();
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    ssize_t write(int fd, const void *data, size_t len)
    {
        if(data==0) return -EFAULT;
        //Important, since len is specified by standard to be unsigned, but the
        //return value has to be signed
        if(static_cast<ssize_t>(len)<0) return -EINVAL;
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->write(data,len);
    }
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    ssize_t read(int fd, void *data, size_t len)
    {
        if(data==0) return -EFAULT;
        //Important, since len is specified by standard to be unsigned, but the
        //return value has to be signed
        if(static_cast<ssize_t>(len)<0) return -EINVAL;
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->read(data,len);
    }
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    off_t lseek(int fd, off_t pos, int whence)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->lseek(pos,whence);
    }
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int fstat(int fd, struct stat *pstat) const
    {
        if(pstat==0) return -EFAULT;
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->fstat(pstat);
    }
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    int isatty(int fd) const
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->isatty();
    }
    
    /**
     * Return file information, follows last symlink
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int stat(const char *name, struct stat *pstat)
    {
        return statImpl(name,pstat,true);
    }
    
    /**
     * Return file information, does not follow last symlink
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    int lstat(const char *name, struct stat *pstat)
    {
        return statImpl(name,pstat,false);
    }
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param opt optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int fcntl(int fd, int cmd, int opt)
    {
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->fcntl(cmd,opt);
    }
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int fd, int cmd, void *arg)
    {
        //arg unchecked here, as some ioctl don't use it
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->ioctl(cmd,arg);
    }
    
    /**
     * List directory content
     * \param dp dp pointer to a memory buffer where one or more struct dirent
     * will be placed. dp must be four words aligned.
     * \param len memory buffer size.
     * \return the number of bytes read on success, or a negative number on
     * failure.
     */
    int getdents(int fd, void *dp, int len)
    {
        if(dp==0) return -EFAULT;
        if(reinterpret_cast<unsigned>(dp) & 0x3) return -EFAULT; //Not aligned
        intrusive_ref_ptr<FileBase> file=getFile(fd);
        if(!file) return -EBADF;
        return file->getdents(dp,len);
    }
    
    /**
     * Return current directory
     * \param buf the current directory is stored here
     * \param len buffer length, if it is not big enough, ERANGE is returned
     * \return 0 on success, or a negative number on failure
     */
    int getcwd(char *buf, size_t len);
    
    /**
     * Change current directory
     * \param path new current directory
     * \return 0 on success, or a negative number on failure
     */
    int chdir(const char *name);
    
    /**
     * Create a directory
     * \param name directory to create
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    int mkdir(const char *name, int mode);
    
    /**
     * Remove a directory if empty
     * \param name directory to create
     * \return 0 on success, or a negative number on failure
     */
    int rmdir(const char *name);
    
    /**
     * Remove a file or directory
     * \param name file or directory to remove
     * \return 0 on success, or a negative number on failure
     */
    int unlink(const char *name);
    
    /**
     * Rename a file or directory
     * \param oldName old file name
     * \param newName new file name
     * \return 0 on success, or a negative number on failure
     */
    int rename(const char *oldName, const char *newName);
    
    /**
     * Retrieves an entry in the file descriptor table
     * \param fd file descriptor, index into the table
     * \return a refcounted poiter to the file at the desired entry
     * (which may be empty), or an empty refcounted pointer if the index is
     * out of bounds 
     */
    intrusive_ref_ptr<FileBase> getFile(int fd) const
    {
        if(fd<0 || fd>=MAX_OPEN_FILES) return intrusive_ref_ptr<FileBase>();
        return atomic_load(files+fd);
    }
    
    /**
     * Destructor
     */
    ~FileDescriptorTable();
    
private:
    /**
     * Append cwd to path if it is not an absolute path
     * \param path an absolute or relative path, must not be null
     * \return an absolute path, or an empty string if the path would exceed
     * PATH_MAX
     */
    std::string absolutePath(const char *path);
    
    /**
     * Return file information (implements both stat and lstat)
     * \param path file to stat
     * \param pstat pointer to stat struct
     * \param f true to follow last synlink (stat),
     * false to not follow it (lstat)
     * \return 0 on success, or a negative number on failure
     */
    int statImpl(const char *name, struct stat *pstat, bool f);
    
    FastMutex mutex; ///< Locks on writes to file object pointers, not on accesses
    
    std::string cwd; ///< Current working directory
    
    /// Holds the mapping between fd and file objects
    intrusive_ref_ptr<FileBase> files[MAX_OPEN_FILES];
};

/**
 * This class contains information on all the mounted filesystems
 */
class FilesystemManager
{
public:
    /**
     * \return the instance of the filesystem manager (singleton)
     */
    static FilesystemManager& instance();
    
    /**
     * Low level mount operation, meant to be used only inside the kernel,
     * and board support packages. It is the only mount operation that can
     * mount the root filesystem.
     * \param path path where to mount the filesystem
     * \param fs filesystem to mount. Ownership of the pointer is transferred
     * to the FilesystemManager class
     * \return 0 on success, a negative number on failure
     */
    int kmount(const char *path, intrusive_ref_ptr<FilesystemBase> fs);
    
    /**
     * Unmounts a filesystem
     * \param path path to a filesytem
     * \param force true to umount the filesystem even if busy
     * \return 0 on success, or a negative number on error
     */
    int umount(const char *path, bool force=false);
    
    /**
     * Umount all filesystems, to be called before system shutdown or reboot
     */
    void umountAll();
    
    #ifdef WITH_DEVFS
    /**
     * \return a pointer to the devfs, useful to add other device files
     */
    intrusive_ref_ptr<DevFs> getDevFs() const { return atomic_load(&devFs); }
    
    /**
     * Called by basicFilesystemSetup() or directly by the BSP to set the
     * pointer returned by getDevFs() 
     * \param dev pointer to the DevFs
     */
    void setDevFs(intrusive_ref_ptr<DevFs> dev)
    {
        atomic_store(&devFs,dev);
    }
    #endif //WITH_DEVFS
    
    /**
     * Resolve a path to identify the filesystem it belongs
     * \param path an absolute path name, that must start with '/'. Note that
     * this is an inout parameter, the string is modified so as to return the
     * full resolved path. In particular, the returned string differs from the
     * passed one by not containing useless path components, such as "/./" and
     * "//", by not containing back path componenets ("/../"), and may be
     * entirely different from the passed one if a symlink was encountered
     * during name resolution. The use of an inout parameter is to minimize
     * the number of copies of the path string, optimizing for speed and size
     * in the common case, but also means that a copy of the original string
     * needs to be made if the original has to be used later.
     * \param followLastSymlink true if the symlink in the last path component
     *(the one that does not end with a /, if it exists, has to be followed)
     * \return the resolved path
     */
    ResolvedPath resolvePath(std::string& path, bool followLastSymlink=true);
    
    /**
     * \internal
     * Helper function to unlink a file or directory. Only meant to be used by
     * FileDescriptorTable::unlink()
     * \param path path of file or directory to unlink
     * \return 0 on success, or a neagtive number on failure
     */
    int unlinkHelper(std::string& path);
    
    /**
     * \internal
     * Helper function to stat a file or directory. Only meant to be used by
     * FileDescriptorTable::statImpl()
     * \param path path of file or directory to stat
     * \param pstat pointer to stat struct
     * \param f f true to follow last synlink (stat),
     * false to not follow it (lstat)
     * \return 0 on success, or a negative number on failure
     */
    int statHelper(std::string& path, struct stat *pstat, bool f);
    
    /**
     * \internal
     * Helper function to unlink a file or directory. Only meant to be used by
     * FileDescriptorTable::unlink()
     * \param oldPath path of file or directory to unlink
     * \param newPath path of file or directory to unlink
     * \return 0 on success, or a neagtive number on failure
     */
    int renameHelper(std::string& oldPath, std::string& newPath);
    
    /**
     * \internal
     * Called by FileDescriptorTable's constructor. Never call this function
     * from user code.
     */
    void addFileDescriptorTable(FileDescriptorTable *fdt)
    {
        #ifdef WITH_PROCESSES
        if(isKernelRunning())
        {
            Lock<FastMutex> l(mutex);
            fileTables.push_back(fdt);
        } else {
            //This function is also called before the kernel is started,
            //and in this case it is forbidden to lock mutexes
            fileTables.push_back(fdt);
        }
        #endif //WITH_PROCESSES
    }
    
    /**
     * \internal
     * Called by FileDescriptorTable's constructor. Never call this function
     * from user code.
     */
    void removeFileDescriptorTable(FileDescriptorTable *fdt)
    {
        #ifdef WITH_PROCESSES
        Lock<FastMutex> l(mutex);
        fileTables.remove(fdt);
        #endif //WITH_PROCESSES
    }
    
    /**
     * \internal
     * \return an unique id used to identify a filesystem, mostly for filling
     * in the st_dev field when stat is called.
     */
    static short int getFilesystemId();
    
private:
    /**
     * Constructor, private as it is a singleton
     */
    FilesystemManager() : mutex(FastMutex::RECURSIVE) {}
    
    FilesystemManager(const FilesystemManager&);
    FilesystemManager& operator=(const FilesystemManager&);
    
    FastMutex mutex; ///< To protect against concurrent access
    
    /// Mounted filesystem
    std::map<StringPart,intrusive_ref_ptr<FilesystemBase> > filesystems;
    
    #ifdef WITH_PROCESSES
    std::list<FileDescriptorTable*> fileTables; ///< Process file tables
    #endif //WITH_PROCESSES
    #ifdef WITH_DEVFS
    intrusive_ref_ptr<DevFs> devFs;
    #endif //WITH_DEVFS

    static int devCount; ///< For assigning filesystemId to filesystems
};

/**
 * This is a simplified function to mount the root and /dev filesystems,
 * meant to be called from bspInit2(). It mounts a MountpointFs as root, then
 * creates a /dev directory, and mounts /dev there. It also takes the passed
 * device and if it is not null it adds the device di DevFs as /dev/sda.
 * Last, it attempts to mount /dev/sda at /sd as a Fat32 filesystem.
 * In case the bsp needs another filesystem setup, such as having a fat32
 * filesystem as /, this function can't be used, but instead the bsp needs to
 * mount the filesystems manually.
 * \param dev disk device that will be added as /dev/sda and mounted on /sd
 * \return a pointer to the DevFs, so as to be able to add other device files,
 * but only if WITH_DEVFS is defined
 */
#ifdef WITH_DEVFS
intrusive_ref_ptr<DevFs> //return value is a pointer to DevFs
#else //WITH_DEVFS
void                     //return value is void
#endif //WITH_DEVFS
basicFilesystemSetup(intrusive_ref_ptr<Device> dev);

/**
 * \return a pointer to the file descriptor table associated with the
 * current process.
 */
FileDescriptorTable& getFileDescriptorTable();

} //namespace miosix

#endif //WITH_FILESYSTEM

#endif //FILE_ACCESS_H
