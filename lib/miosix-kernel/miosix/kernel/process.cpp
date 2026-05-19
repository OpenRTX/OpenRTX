/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include <stdexcept>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#include "sync.h"
#include "process_pool.h"
#include "process.h"
#include "SystemMap.h"

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

/**
 * Used to check if a pointer passed from userspace is aligned
 */
static bool aligned(void *x) { return (reinterpret_cast<unsigned>(x) & 0b11)==0; }

/**
 * This class contains information on all the processes in the system
 */
class Processes
{
public:
    /**
     * \return the instance of this class (singleton)
     */
    static Processes& instance();
    
    ///Used to assign a new pid to a process
    pid_t pidCounter;
    ///Maps the pid to the Process instance. Includes zombie processes
    map<pid_t,ProcessBase *> processes;
    ///Uset to guard access to processes and pidCounter
    Mutex procMutex;
    ///Used to wait on process termination
    ConditionVariable genericWaiting;
    
private:
    Processes()
    {
        ProcessBase *kernel=Thread::getCurrentThread()->getProcess();
        assert(kernel->getPid()==0);
        processes[0]=kernel;
    }
    Processes(const Processes&);
    Processes& operator=(const Processes&);
};

Processes& Processes::instance()
{
    static Processes singleton;
    return singleton;
}

//
// class Process
//

pid_t Process::create(const ElfProgram& program)
{
    Processes& p=Processes::instance();
    ProcessBase *parent=Thread::getCurrentThread()->proc;
    unique_ptr<Process> proc(new Process(program));
    {   
        Lock<Mutex> l(p.procMutex);
        proc->pid=getNewPid();
        proc->ppid=parent->pid;
        parent->childs.push_back(proc.get());
        p.processes[proc->pid]=proc.get();
    }
    Thread *thr;
    thr=Thread::createUserspace(Process::start,0,Thread::DEFAULT,proc.get());
    if(thr==0)
    {
        Lock<Mutex> l(p.procMutex);
        p.processes.erase(proc->pid);
        parent->childs.remove(proc.get());
        throw runtime_error("Thread creation failed");
    }
    //Cannot throw bad_alloc due to the reserve in Process's constructor.
    //This ensures we will never be in the uncomfortable situation where a
    //thread has already been created but there's no memory to list it
    //among the threads of a process
    proc->threads.push_back(thr);
    thr->wakeup(); //Actually start the thread, now that everything is set up
    pid_t result=proc->pid;
    proc.release(); //Do not delete the pointer
    return result;
}

pid_t Process::getppid(pid_t proc)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    map<pid_t,ProcessBase *>::iterator it=p.processes.find(proc);
    if(it==p.processes.end()) return -1;
    return it->second->ppid;
}

pid_t Process::waitpid(pid_t pid, int* exit, int options)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    ProcessBase *self=Thread::getCurrentThread()->proc;
    if(pid<=0)
    {
        //Wait for a generic child process
        if(self->zombies.empty() && (options & WNOHANG)) return 0;
        while(self->zombies.empty())
        {
            if(self->childs.empty()) return -1;
            p.genericWaiting.wait(l);
        }
        Process *joined=self->zombies.front();
        self->zombies.pop_front();
        p.processes.erase(joined->pid);
        if(joined->waitCount!=0) errorHandler(UNEXPECTED);
        if(exit!=0) *exit=joined->exitCode;
        pid_t result=joined->pid;
        delete joined;
        return result;
    } else {
        //Wait on a specific child process
        map<pid_t,ProcessBase *>::iterator it=p.processes.find(pid);
        if(it==p.processes.end() || it->second->ppid!=self->pid
                || pid==self->pid) return -1;
        //Since the case when pid==0 has been singled out, this cast is safe
        Process *joined=static_cast<Process*>(it->second);
        if(joined->zombie==false)
        {
            //Process hasn't terminated yet
            if(options & WNOHANG) return 0;
            joined->waitCount++;
            joined->waiting.wait(l);
            joined->waitCount--;
            if(joined->waitCount<0 || joined->zombie==false)
                errorHandler(UNEXPECTED);
        }
        pid_t result=-1;
        if(joined->waitCount==0)
        {
            result=joined->pid;
            if(exit!=0) *exit=joined->exitCode;
            self->zombies.remove(joined);
            p.processes.erase(joined->pid);
            delete joined;
        }
        return result;
    }
}

Process::~Process() {}

Process::Process(const ElfProgram& program) : program(program), waitCount(0),
        zombie(false)
{
    //This is required so that bad_alloc can never be thrown when the first
    //thread of the process will be stored in this vector
    threads.reserve(1);
    //Done here so if not enough memory the new process is not even created
    image.load(program);
    unsigned int elfSize=program.getElfSize();
    unsigned int roundedSize=elfSize;
    if(elfSize<ProcessPool::blockSize) roundedSize=ProcessPool::blockSize;
    roundedSize=MPUConfiguration::roundSizeForMPU(roundedSize);
    //TODO: Till a flash file system that ensures proper alignment of the
    //programs loaded in flash is implemented, make the whole flash visible as
    //a big MPU region. This allows a program to read and execute parts of
    //other programs but not to write anything.
    extern unsigned char _elf_pool_start asm("_elf_pool_start");
    extern unsigned char _elf_pool_end asm("_elf_pool_end");
    unsigned int *start=reinterpret_cast<unsigned int*>(&_elf_pool_start);
    unsigned int *end=reinterpret_cast<unsigned int*>(&_elf_pool_end);
    unsigned int elfPoolSize=(end-start)*sizeof(int);
    elfPoolSize=MPUConfiguration::roundSizeForMPU(elfPoolSize);
    mpu=MPUConfiguration(start,elfPoolSize,
            image.getProcessBasePointer(),image.getProcessImageSize());
//    mpu=MPUConfiguration(program.getElfBase(),roundedSize,
//            image.getProcessBasePointer(),image.getProcessImageSize());
}

void *Process::start(void *argv)
{
    //This function is never called with a kernel thread, so the cast is safe
    Process *proc=static_cast<Process*>(Thread::getCurrentThread()->proc);
    if(proc==0) errorHandler(UNEXPECTED);
    unsigned int entry=proc->program.getEntryPoint();
    Thread::setupUserspaceContext(entry,proc->image.getProcessBasePointer(),
        proc->image.getProcessImageSize());
    bool running=true;
    do {
        miosix_private::SyscallParameters sp=Thread::switchToUserspace();
        if(proc->fault.faultHappened())
        {
            running=false;
            proc->exitCode=SIGSEGV; //Segfault
            #ifdef WITH_ERRLOG
            iprintf("Process %d terminated due to a fault\n"
                    "* Code base address was 0x%x\n"
                    "* Data base address was %p\n",proc->pid,
                    proc->program.getElfBase(),
                    proc->image.getProcessBasePointer());
            proc->mpu.dumpConfiguration();
            proc->fault.print();
            #endif //WITH_ERRLOG
        } else running=proc->handleSvc(sp);
        if(Thread::testTerminate()) running=false;
    } while(running);
    {
        Processes& p=Processes::instance();
        Lock<Mutex> l(p.procMutex);
        proc->zombie=true;
        list<Process*>::iterator it;
        for(it=proc->childs.begin();it!=proc->childs.end();++it) (*it)->ppid=0;
        for(it=proc->zombies.begin();it!=proc->zombies.end();++it) (*it)->ppid=0;
        ProcessBase *kernel=p.processes[0];
        kernel->childs.splice(kernel->childs.begin(),proc->childs);
        kernel->zombies.splice(kernel->zombies.begin(),proc->zombies);
        
        map<pid_t,ProcessBase *>::iterator it2=p.processes.find(proc->ppid);
        if(it2==p.processes.end()) errorHandler(UNEXPECTED);
        it2->second->childs.remove(proc);
        if(proc->waitCount>0) proc->waiting.broadcast();
        else {
            it2->second->zombies.push_back(proc);
            p.genericWaiting.broadcast();
        }
    }
    return 0;
}

bool Process::handleSvc(miosix_private::SyscallParameters sp)
{
    try {
        switch(sp.getSyscallId())
        {
            case SYS_EXIT:
            {
                exitCode=(sp.getFirstParameter() & 0xff)<<8;
                return false;
            }
            case SYS_WRITE:
            {
                int fd=sp.getFirstParameter();
                void *ptr=reinterpret_cast<void*>(sp.getSecondParameter());
                size_t size=sp.getThirdParameter();
                if(mpu.withinForReading(ptr,size))
                {
                    ssize_t result=fileTable.write(fd,ptr,size);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_READ:
            {
                int fd=sp.getFirstParameter();
                void *ptr=reinterpret_cast<void*>(sp.getSecondParameter());
                size_t size=sp.getThirdParameter();
                if(mpu.withinForWriting(ptr,size))
                {
                    ssize_t result=fileTable.read(fd,ptr,size);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_USLEEP:
            {
                sp.setReturnValue(usleep(sp.getFirstParameter()));
                break;
            }
            case SYS_OPEN:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                int flags=sp.getSecondParameter();
                if(mpu.withinForReading(str))
                {
                    int fd=fileTable.open(str,flags,
                        (flags & O_CREAT) ? sp.getThirdParameter() : 0);
                    sp.setReturnValue(fd);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_CLOSE:
            {
                int result=fileTable.close(sp.getFirstParameter());
                sp.setReturnValue(result);
                break;
            }
            case SYS_LSEEK:
            {
                //FIXME: need to pass and return a 64 bit parameter,
                //now it is truncated to 32 bit but this is wrong
                off_t result=fileTable.lseek(sp.getFirstParameter(),
                    sp.getSecondParameter(),sp.getThirdParameter());
                sp.setReturnValue(result);
                break;
            }
            case SYS_SYSTEM:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                if(mpu.withinForReading(str))
                {
                    std::pair<const unsigned int*,unsigned int> res;
                    res=SystemMap::instance().getElfProgram(str);
                    if(res.first==0 || res.second==0)
                    {
                        sp.setReturnValue(-1);
                    } else {
                        ElfProgram program(res.first,res.second);
                        int ret=0;
                        pid_t child=Process::create(program);
                        Process::waitpid(child,&ret,0);
                        sp.setReturnValue(WEXITSTATUS(ret));
                    }
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_FSTAT:
            {
                struct stat *pstat;
                pstat=reinterpret_cast<struct stat*>(sp.getSecondParameter());
                if(mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.fstat(sp.getFirstParameter(),pstat);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_ISATTY:
            {
                int result=fileTable.isatty(sp.getFirstParameter());
                sp.setReturnValue(result);
                break;
            }
            case SYS_STAT:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                struct stat *pstat;
                pstat=reinterpret_cast<struct stat*>(sp.getSecondParameter());
                if(mpu.withinForReading(str) &&
                   mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.stat(str,pstat);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
            }
            case SYS_LSTAT:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                struct stat *pstat;
                pstat=reinterpret_cast<struct stat*>(sp.getSecondParameter());
                if(mpu.withinForReading(str) &&
                   mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.lstat(str,pstat);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
            }
            case SYS_FCNTL:
            {
                int result=fileTable.fcntl(sp.getFirstParameter(),
                    sp.getSecondParameter(),sp.getThirdParameter());
                sp.setReturnValue(result);
                break;
            }
            case SYS_IOCTL:
            {
                //TODO: need a way to validate ARG
                break;
            }
            case SYS_GETDENTS:
            {
                int fd=sp.getFirstParameter();
                void *ptr=reinterpret_cast<void*>(sp.getSecondParameter());
                size_t size=sp.getThirdParameter();
                if(mpu.withinForWriting(ptr,size))
                {
                    int result=fileTable.getdents(fd,ptr,size);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_GETCWD:
            {
                char *buf=reinterpret_cast<char*>(sp.getFirstParameter());
                size_t size=sp.getSecondParameter();
                if(mpu.withinForWriting(buf,size))
                {
                    int result=fileTable.getcwd(buf,size);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_CHDIR:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.chdir(str);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_MKDIR:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.mkdir(str,sp.getSecondParameter());
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_RMDIR:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.rmdir(str);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_UNLINK:
            {
                const char *str;
                str=reinterpret_cast<const char*>(sp.getFirstParameter());
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.unlink(str);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            case SYS_RENAME:
            {
                const char *oldName, *newName;
                oldName=reinterpret_cast<const char*>(sp.getFirstParameter());
                newName=reinterpret_cast<const char*>(sp.getSecondParameter());
                if(mpu.withinForReading(oldName) &&
                   mpu.withinForReading(newName))
                {
                    int result=fileTable.rename(oldName,newName);
                    sp.setReturnValue(result);
                } else sp.setReturnValue(-EFAULT);
                break;
            }
            default:
                exitCode=SIGSYS; //Bad syscall
                #ifdef WITH_ERRLOG
                iprintf("Unexpected syscall number %d\n",sp.getSyscallId());
                #endif //WITH_ERRLOG
                return false;
        }
    } catch(exception& e) {
        sp.setReturnValue(-ENOMEM);
    }
    return true;
}

pid_t Process::getNewPid()
{
    Processes& p=Processes::instance();
    for(;;p.pidCounter++)
    {
        if(p.pidCounter<0) p.pidCounter=1;
        if(p.pidCounter==0) continue; //Zero is not a valid pid
        map<pid_t,ProcessBase*>::iterator it=p.processes.find(p.pidCounter);
        if(it!=p.processes.end()) continue; //Pid number already used
        return p.pidCounter++;
    }
}

} //namespace miosix

#endif //WITH_PROCESSES
