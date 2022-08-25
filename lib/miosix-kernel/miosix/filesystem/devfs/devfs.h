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

#ifndef DEVFS_H
#define	DEVFS_H

#include <map>
#include "filesystem/file.h"
#include "filesystem/stringpart.h"
#include "kernel/sync.h"
#include "config/miosix_settings.h"

namespace miosix {

/**
 * Instances of this class are devices inside DevFs. When open is called, a
 * DevFsFile is returned, which has its own seek point so that multiple files
 * can be opened on the same device retaining an unique seek point. A DevFsFile
 * then calls readBlock() and writeBlock() on this class. These functions have a
 * third argument which is the seek point, making them stateless.
 * 
 * Individual devices must subclass Device and reimplement readBlock(),
 * writeBlock() and ioctl() as needed. A mutex may be required as multiple
 * concurrent readBlock(), writeBlock() and ioctl() can occur.
 * 
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 * 
 * This class is defined also if WITH_DEVFS is not defined as it is used by the
 * Console interface, but in this case the interface is reduced to a minimum
 */
class Device : public IntrusiveRefCounted,
        public IntrusiveRefCountedSharedFromThis<Device>
{
public:
    /**
     * Possible device types
     */
    enum DeviceType
    {
        STREAM, ///< Not seekable device, like /dev/random
        BLOCK,  ///< Seekable block device
        TTY     ///< Like STREAM, but additionally is a TTY
    };
    /**
     * Constructor
     * \param d device type
     */
    Device(DeviceType d) : seekable(d==BLOCK), block(d==BLOCK), tty(d==TTY)
    {}
    
    /**
     * Return an instance of the file type managed by this Device
     * \param file the file object will be stored here, if the call succeeds
     * \param fs pointer to the DevFs
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    int open(intrusive_ref_ptr<FileBase>& file,
            intrusive_ref_ptr<FilesystemBase> fs, int flags, int mode);
    
    /**
     * Obtain information for the file type managed by this Device
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    int fstat(struct stat *pstat) const;
    
    /**
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    #ifdef WITH_DEVFS
    
    /**
     * \internal
     * Called be DevFs to assign a device and inode to the Device
     */
    void setFileInfo(unsigned int st_ino, short st_dev)
    {
        this->st_ino=st_ino;
        this->st_dev=st_dev;
    }
    
    #endif //WITH_DEVFS
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure
     */
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    /**
     * Write a string.
     * An extension to the Device interface that adds a new member function,
     * which is used by the kernel on console devices to write debug information
     * before the kernel is started or in case of serious errors, right before
     * rebooting.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt. This default implementation ignores writes.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);
    
    /**
     * Destructor
     */
    virtual ~Device();

protected:
    unsigned int st_ino; ///< inode of device file
    short st_dev;        ///< device (unique id of the filesystem) of device file
    const bool seekable; ///< If true, device is seekable
    const bool block;    ///< If true, it is a block device
    const bool tty;      ///< If true, it is a tty
};

#ifdef WITH_DEVFS

/**
 * DevFs is a special filesystem meant to access devices as they were files.
 * For this reason, it is a little different from other filesystems. Normal
 * filesystems create FileBase objects ondemand, to answer an open() call. Such
 * files have a parent pointer that points to the filesystem. On the contrary,
 * DevFs is a collection of both pre-existing DeviceFiles (for stateless files),
 * or DeviceFileGenerators for stateful ones. Each device file is a different
 * subclass of FileBase that overrides some of its member functions to access
 * the handled device. These FileBase subclasses do not have a parent pointer
 * into DevFs, and as such umounting DevFs should better be avoided, as it's
 * not possible to detect if some of its files are currently opened by some
 * application. What will happen is that the individual files (and
 * DeviceFileGenerators) won't be deleted until the processes that have them
 * opened close them.
 */
class DevFs : public FilesystemBase
{
public:
    /**
     * Constructor
     */
    DevFs();
    
    /**
     * Add a device file to DevFs
     * \param name File name, must not start with a slash
     * \param df Device file. Every open() call will return the same file
     * \return true if the file was successfully added
     */
    bool addDevice(const char *name, intrusive_ref_ptr<Device> dev);
    
    /**
     * Remove a device. This prevents the device from being opened again,
     * but if at the time this member function is called the file is already
     * opened, it won't be deallocated till the application closes it, thanks
     * to the reference counting scheme.
     * \param name name of file to remove
     * \return true if the file was successfully removed
     */
    bool remove(const char *name);
    
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
            int flags, int mode);
    
    /**
     * Obtain information on a file, identified by a path name. Does not follow
     * symlinks
     * \param name path name, relative to the local filesystem
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(StringPart& name, struct stat *pstat);
    
    /**
     * Remove a file or directory
     * \param name path name of file or directory to remove
     * \return 0 on success, or a negative number on failure
     */
    virtual int unlink(StringPart& name);
    
    /**
     * Rename a file or directory
     * \param oldName old file name
     * \param newName new file name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rename(StringPart& oldName, StringPart& newName);
     
    /**
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode);
    
    /**
     * Remove a directory if empty
     * \param name directory name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rmdir(StringPart& name);
    
private:
    
    FastMutex mutex;
    std::map<StringPart,intrusive_ref_ptr<Device> > files;
    int inodeCount;
    static const int rootDirInode=1;
};

#endif //WITH_DEVFS

} //namespace miosix

#endif //DEVFS_H
