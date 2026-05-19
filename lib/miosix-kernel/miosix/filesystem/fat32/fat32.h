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

#ifndef FAT32_H
#define	FAT32_H

#include "filesystem/file.h"
#include "kernel/sync.h"
#include "ff.h"
#include "config/miosix_settings.h"

namespace miosix {
    
#ifdef WITH_FILESYSTEM

/**
 * Fat32 Filesystem.
 */
class Fat32Fs : public FilesystemBase
{
public:
    /**
     * Constructor
     */
    Fat32Fs(intrusive_ref_ptr<FileBase> disk);
    
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
    
    /**
     * \return true if the filesystem failed to mount 
     */
    bool mountFailed() const { return failed; }
    
    /**
     * Destructor
     */
    ~Fat32Fs();
    
private:
    
    int unlinkRmdirHelper(StringPart& name, bool delDir);
    
    FATFS filesystem;
    FastMutex mutex;
    bool failed; ///< Failed to mount
};

#endif //WITH_FILESYSTEM

} //namespace miosix

#endif //FAT32_H
