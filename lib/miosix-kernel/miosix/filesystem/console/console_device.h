/***************************************************************************
 *   Copyright (C) 2013, 2014 by Terraneo Federico                         *
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

#ifndef CONSOLE_DEVICE_H
#define	CONSOLE_DEVICE_H

#include "config/miosix_settings.h"
#include "filesystem/devfs/devfs.h"
#include "kernel/sync.h"

namespace miosix {

/**
 * Teriminal device, proxy object supporting additional terminal-specific
 * features
 */
class TerminalDevice : public FileBase
{
public:
    /**
     * Constructor
     * \param device proxed device.
     */
    TerminalDevice(intrusive_ref_ptr<Device> device);
    
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param length the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t length);
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param length the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t length);
    
    #ifdef WITH_FILESYSTEM
    
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
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    #endif //WITH_FILESYSTEM
    
    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);
    
    /**
     * Enables or disables echo of commands on the terminal
     * \param echo true to enable echo, false to disable it
     */
    void setEcho(bool echoMode) { echo=echoMode; }
    
    /**
     * \return true if echo is enabled 
     */
    bool isEchoEnabled() const { return echo; }
    
    /**
     * Selects whether the terminal sholud be transparent to non ASCII data
     * \param rawMode true if raw mode is required
     */
    void setBinary(bool binaryMode) { binary=binaryMode; }
    
    /**
     * \return true if the terminal allows binary data 
     */
    bool isBinary() const { return binary; }
    
private:
    /**
     * Perform normalization of a read buffer (\r\n conversion to \n, backspace)
     * \param buffer pointer to read buffer
     * \param begin buffer[begin] is the first character to normalize
     * \param end buffer[end] is one past the las character to normalize
     * \return a pair with the number of valid character in the buffer (staring
     * from buffer[0], not from buffer[begin], and a bool that is true if at
     * least one \n was found. 
     */
    std::pair<size_t,bool> normalize(char *buffer, ssize_t begin, ssize_t end);
    
    /**
     * Perform echo when reading a buffer
     * \param chunkEnd one past the last character to echo back. The first
     * character is chunkStart. As a side effect, this member function modifies
     * chunkStart to be equal to chunkEnd+1, if echo is enabled
     * \param sep optional line separator, printed after the chunk
     * \param sepLen separator length
     */
    void echoBack(const char *chunkEnd, const char *sep=0, size_t sepLen=0);
    
    intrusive_ref_ptr<Device> device; ///< Underlying TTY device
    FastMutex mutex;                  ///< Mutex to serialze concurrent reads
    const char *chunkStart;           ///< First character to echo in echoBack()
    bool echo;                        ///< True if echo enabled
    bool binary;                      ///< True if binary mode enabled
    bool skipNewline;                 ///< Used by normalize()
};

/**
 * This class holds the file object related to the console, that is set by
 * the board support package, and used to populate /dev/console in DevFs
 */
class DefaultConsole
{
public:
    /**
     * \return an instance of this class (singleton) 
     */
    static DefaultConsole& instance();
    
    /**
     * Called by the board support package, in particular IRQbspInit(), to pass
     * to the kernel the console device. This device file is used as the default
     * one for stdin/stdout/stderr.
     * Notes: this has to be called in IRQbspInit(), since if it's called too
     * late the console gets initialized with a NullFile.
     * Also, calling this a second time to dynamically change the console device
     * is probably a bad idea, as the device is cached around in the filesystem
     * code and will result in some processes using the old device and some
     * other the new one.
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void IRQset(intrusive_ref_ptr<Device> console);
    
    /**
     * Same as IRQset(), but can be called with interrupts enabled
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void set(intrusive_ref_ptr<Device> console) { IRQset(console); }
    
    /**
     * \return the currently installed console device, wrapped in a
     * TerminalDevice
     */
    intrusive_ref_ptr<Device> get() { return console; }
    
    /**
     * \return the currently installed console device.
     * Can be called with interrupts disabled or within an interrupt routine.
     */
    intrusive_ref_ptr<Device> IRQget() { return console; }
    
    #ifndef WITH_FILESYSTEM
    /**
     * \return the terminal device, when filesystem support is disabled.
     * If filesystem is enabled, the terminal device can be found in the
     * FileDescriptorTable
     */
    intrusive_ref_ptr<TerminalDevice> getTerminal() { return terminal; }
    #endif //WITH_FILESYSTEM
    
private:    
    /**
     * Constructor, private as it is a singleton
     */
    DefaultConsole();
    
    DefaultConsole(const DefaultConsole&);
    DefaultConsole& operator= (const DefaultConsole&);
    
    intrusive_ref_ptr<Device> console; ///< The raw console device
    #ifndef WITH_FILESYSTEM
    intrusive_ref_ptr<TerminalDevice> terminal; ///< The wrapped console device
    #endif //WITH_FILESYSTEM
};

} //namespace miosix

#endif //CONSOLE_DEVICE_H
