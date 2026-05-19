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

#include <dirent.h>
#include <sys/stat.h>
#include "kernel/intrusive.h"
#include "config/miosix_settings.h"

#ifndef FILE_H
#define	FILE_H

namespace miosix {

// Forward decls
class FilesystemBase;
class StringPart;

/**
 * The unix file abstraction. Also some device drivers are seen as files.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class FileBase : public IntrusiveRefCounted
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    FileBase(intrusive_ref_ptr<FilesystemBase> parent);
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len)=0;
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len)=0;
    
    #ifdef WITH_FILESYSTEM
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence)=0;
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const=0;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param opt optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int fcntl(int cmd, int opt);
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);
    
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
     * \return a pointer to the parent filesystem
     */
    const intrusive_ref_ptr<FilesystemBase> getParent() const { return parent; }
    
    #endif //WITH_FILESYSTEM
    
    /**
     * File destructor
     */
    virtual ~FileBase();
    
private:
    FileBase(const FileBase&);
    FileBase& operator=(const FileBase&);
    
    intrusive_ref_ptr<FilesystemBase> parent; ///< Files may have a parent fs
};

/**
 * Directories are a special kind of files that implement the getdents() call
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<DirectoryBase>
 */
class DirectoryBase : public FileBase
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    DirectoryBase(intrusive_ref_ptr<FilesystemBase> parent) : FileBase(parent) {}
    
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
    
protected:
    /**
     * Helper function to add a directory entry to a buffer
     * \param pos position where to add the entry (four word aligned). Pointer
     * is incremented.
     * \param end end of buffer (one char past the last), for bound checking
     * \param ino inode of file
     * \param type file type
     * \param name file name to append after the DirentHeader
     * \return the number of bytes written or -1 on failure (no space in buffer)
     */
    static int addEntry(char **pos, char *end, int ino, char type,
            const StringPart& n);
    
    /**
     * Helper function to add the default directory entries . and .. to a buffer
     * \param pos position where to add the entry (four word aligned). Pointer
     * is incremented. The caller is responsible to guarantee that there is at
     * least space for 2*direntHeaderSize
     * \param thisIno inode number of .
     * \param upInode inode number of ..
     * \return the number of bytes written
     */
    static int addDefaultEntries(char **pos, int thisIno, int upIno);
    
    /**
     * Add an entry with d_reclen=0 which is used to terminate directory listing
     * \param pos position where to add the entry (four word aligned). Pointer
     * is incremented. The caller is responsible to guarantee that there is at
     * least space for direntHeaderSize, including padding
     * \param end end of buffer (one char past the last), for bound checking
     * \return the number of bytes written or -1 on failure (no space in buffer)
     */
    static int addTerminatingEntry(char **pos, char *end);
    
    ///Size of struct dirent excluding d_name. That is, the size of d_ino,
    ///d_off, d_reclen and d_type.  Notice that there are 4 bytes of padding
    ///between d_ino and d_off as d_off is a 64 bit number. Should be 19.
    static const int direntHeaderSizeNoPadding=offsetof(struct dirent,d_name);
    
    ///Size of struct dirent including room for the "." and ".." string in
    ///d_name, including terminating \0 and padding for 4-word alignment.
    ///First +3: make room for '..\0', 3 bytes
    ///Second +3 and /4*4: four word alignment
    static const int direntHeaderSize=(direntHeaderSizeNoPadding+3+3)/4*4;
    
    ///Minimum buffer accepted by getdents, two for . and .., plus terminating
    static const int minimumBufferSize=3*direntHeaderSize;
};

/**
 * All filesystems derive from this class. Classes of this type are reference
 * counted, must be allocated on the heap and managed through
 * intrusive_ref_ptr<FilesystemBase>
 */
class FilesystemBase : public IntrusiveRefCounted,
        public IntrusiveRefCountedSharedFromThis<FilesystemBase>
{
public:
    /**
     * Constructor
     */
    FilesystemBase();
    
    /**
     * Open a file
     * \param file the file object will be stored here, if the call succeeds
     * \param name the name of the file to open, relative to the local
     * filesystem
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
            int flags, int mode)=0;
    
    /**
     * Obtain information on a file, identified by a path name. Does not follow
     * symlinks
     * \param name path name, relative to the local filesystem
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(StringPart& name, struct stat *pstat)=0;
    
    /**
     * Remove a file or directory
     * \param name path name of file or directory to remove
     * \return 0 on success, or a negative number on failure
     */
    virtual int unlink(StringPart& name)=0;
    
    /**
     * Rename a file or directory
     * \param oldName old file name
     * \param newName new file name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rename(StringPart& oldName, StringPart& newName)=0;
    
    /**
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode)=0;
    
    /**
     * Remove a directory if empty
     * \param name directory name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rmdir(StringPart& name)=0;
    
    /**
     * Follows a symbolic link
     * \param path path identifying a symlink, relative to the local filesystem
     * \param target the link target is returned here if the call succeeds.
     * Note that the returned path is not relative to this filesystem, and can
     * be either relative or absolute.
     * \return 0 on success, a negative number on failure
     */
    virtual int readlink(StringPart& name, std::string& target);
    
    /**
     * \return true if the filesystem supports symbolic links.
     * In this case, the filesystem should override readlink
     */
    virtual bool supportsSymlinks() const;
    
    /**
     * \internal
     * \return true if all files belonging to this filesystem are closed 
     */
    bool areAllFilesClosed() { return openFileCount==0; }
    
    /**
     * \internal
     * Called by file constructor whenever a file belonging to this
     * filesystem is opened. Never call this function from user code.
     */
    void newFileOpened();
    
    /**
     * \internal
     * Called by file destructors whenever a file belonging to this
     * filesystem is closed. Never call this function from user code.
     */
    void fileCloseHook();
    
    /**
     * \internal
     * This is used to inform a filesystem of the inode of the directory in the
     * parent fs where it is mounted. It is used for directory listing, to
     * resolve the inode of the .. entry of the filesystem's root directory
     * \param inode inode of the directory where the fs is mounted
     */
    void setParentFsMountpointInode(int inode) { parentFsMountpointInode=inode; }
    
    /**
     * \return filesystem id
     */
    short int getFsId() const { return filesystemId; }
            
    /**
     * Destructor
     */
    virtual ~FilesystemBase();
    
protected:
    
    const short int filesystemId; ///< The unique filesystem id, used by lstat
    int parentFsMountpointInode; ///< The inode of the directory in the parent fs
    
private:
    FilesystemBase(const FilesystemBase&);
    FilesystemBase& operator= (const FilesystemBase&);
    
    volatile int openFileCount; ///< Number of open files
};

} //namespace miosix

#endif //FILE_H
