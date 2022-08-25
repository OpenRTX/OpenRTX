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

#include "console_device.h"
#include "filesystem/ioctl.h"
#include <errno.h>
#include <termios.h>

using namespace std;

namespace miosix {

//
// class TerminalDevice
//

TerminalDevice::TerminalDevice(intrusive_ref_ptr<Device> device)
        : FileBase(intrusive_ref_ptr<FilesystemBase>()), device(device),
          mutex(), echo(true), binary(false), skipNewline(false) {}

ssize_t TerminalDevice::write(const void *data, size_t length)
{
    if(binary) return device->writeBlock(data,length,0);
    //No mutex here to avoid blocking writes while reads are in progress
    const char *buffer=static_cast<const char*>(data);
    const char *start=buffer;
    //Try to write data in chunks, stop at every \n to replace with \r\n
    //Although it may be tempting to call echoBack() from here since it performs
    //a similar task, it is not possible, as echoBack() uses a class field,
    //chunkStart, and write is not mutexed to allow concurrent writing
    for(size_t i=0;i<length;i++,buffer++)
    {
        if(*buffer!='\n') continue;
        if(buffer>start)
        {
            ssize_t r=device->writeBlock(start,buffer-start,0);
            if(r<=0) return r;
        }
        ssize_t r=device->writeBlock("\r\n",2,0);//Add \r\n
        if(r<=0) return r;
        start=buffer+1;
    }
    if(buffer>start)
    {
        ssize_t r=device->writeBlock(start,buffer-start,0);
        if(r<=0) return r;
    }
    return length;
}

ssize_t TerminalDevice::read(void *data, size_t length)
{
    if(binary)
    {
        ssize_t result=device->readBlock(data,length,0);
        if(echo && result>0) device->writeBlock(data,result,0);//Ignore write errors
        return result;
    }
    Lock<FastMutex> l(mutex); //Reads are serialized
    char *buffer=static_cast<char*>(data);
    size_t readBytes=0;
    for(;;)
    {
        ssize_t r=device->readBlock(buffer+readBytes,length-readBytes,0);
        if(r<0) return r;
        pair<size_t,bool> result=normalize(buffer,readBytes,readBytes+r);
        readBytes=result.first;
        if(readBytes==length || result.second) return readBytes;
    }
}

#ifdef WITH_FILESYSTEM

off_t TerminalDevice::lseek(off_t pos, int whence)
{
    return -EBADF;
}

int TerminalDevice::fstat(struct stat *pstat) const
{
    return device->fstat(pstat);
}

int TerminalDevice::isatty() const { return device->isatty(); }

#endif //WITH_FILESYSTEM

int TerminalDevice::ioctl(int cmd, void *arg)
{
    if(int result=device->ioctl(cmd,arg)!=0) return result;
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_TCGETATTR:
            if(echo) t->c_lflag |= ECHO; else t->c_lflag &= ~ECHO;
            if(binary==false) t->c_lflag |= ICANON; else t->c_lflag &= ~ICANON;
            break;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            echo=(t->c_lflag & ECHO) ? true : false;
            binary=(t->c_lflag & ICANON) ? false : true;
            break;
        default:
            break;
    }
    return 0;
}

pair<size_t,bool> TerminalDevice::normalize(char *buffer, ssize_t begin,
        ssize_t end)
{
    bool newlineFound=false;
    buffer+=begin;
    chunkStart=buffer;
    for(ssize_t i=begin;i<end;i++,buffer++)
    {
        switch(*buffer)
        {
            //Trying to be compatible with terminals that output \r, \n or \r\n
            //When receiving \r skipNewline is set to true so we skip the \n
            //if it comes right after the \r
            case '\r':
                *buffer='\n';
                echoBack(buffer,"\r\n",2);
                skipNewline=true;
                newlineFound=true;
                break;
            case '\n':
                if(skipNewline)
                {
                    skipNewline=false;
                    //Discard the \n because comes right after \r
                    memmove(buffer,buffer+1,end-i-1);
                    end--;
                    i--; //Note that i may become -1, but it is acceptable.
                    buffer--;
                } else {
                    echoBack(buffer,"\r\n",2);
                    newlineFound=true;
                }
                break;
            case 0x7f: //Unix backspace
            case 0x08: //DOS backspace
            {
                //Need to echo before moving buffer data
                echoBack(buffer,"\033[1D \033[1D",9);
                ssize_t backward= i==0 ? 1 : 2;
                memmove(buffer-(backward-1),buffer+1,end-i);
                end-=backward;
                i-=backward; //Note that i may become -1, but it is acceptable.
                buffer-=backward;
                chunkStart=buffer+1; //Fixup chunkStart after backspace 
                break;
            }
            default:
                skipNewline=false;
        }
    }
    echoBack(buffer);
    return make_pair(end,newlineFound);
}

void TerminalDevice::echoBack(const char *chunkEnd, const char *sep, size_t sepLen)
{
    if(!echo) return;
    if(chunkEnd>chunkStart) device->writeBlock(chunkStart,chunkEnd-chunkStart,0);
    chunkStart=chunkEnd+1;
    if(sep) device->writeBlock(sep,sepLen,0); //Ignore write errors
}

//
// class DefaultConsole 
//

DefaultConsole& DefaultConsole::instance()
{
    static DefaultConsole singleton;
    return singleton;
}

void DefaultConsole::IRQset(intrusive_ref_ptr<Device> console)
{
    //Note: should be safe to be called also outside of IRQ as set() calls
    //IRQset()
    atomic_store(&this->console,console);
    #ifndef WITH_FILESYSTEM
    atomic_store(&terminal,
        intrusive_ref_ptr<TerminalDevice>(new TerminalDevice(console)));
    #endif //WITH_FILESYSTEM
}

DefaultConsole::DefaultConsole() : console(new Device(Device::STREAM))
#ifndef WITH_FILESYSTEM
, terminal(new TerminalDevice(console))
#endif //WITH_FILESYSTEM      
{}

} //namespace miosix
