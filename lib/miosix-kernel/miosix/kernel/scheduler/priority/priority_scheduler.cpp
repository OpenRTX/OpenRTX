/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

#include "priority_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"

#ifdef SCHED_TYPE_PRIORITY

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *cur;
extern volatile int kernel_running;

//
// class PriorityScheduler
//

bool PriorityScheduler::PKaddThread(Thread *thread,
        PrioritySchedulerPriority priority)
{
    thread->schedData.priority=priority;
    if(thread_list[priority.get()]==NULL)
    {
        thread_list[priority.get()]=thread;
        thread->schedData.next=thread;//Circular list
    } else {
        thread->schedData.next=thread_list[priority.get()]->schedData.next;
        thread_list[priority.get()]->schedData.next=thread;
    }
    return true;
}

bool PriorityScheduler::PKexists(Thread *thread)
{
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(thread_list[i]==NULL) continue;
        Thread *temp=thread_list[i];
        for(;;)
        {
            if((temp==thread)&&(! (temp->flags.isDeleted())))
            {
                //Found
                return true;
            }
            temp=temp->schedData.next;
            if(temp==thread_list[i]) break;
        }
    }
    return false;
}

void PriorityScheduler::PKremoveDeadThreads()
{
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(thread_list[i]==NULL) continue;
        bool first=false;//If false the tail of the list hasn't been calculated
        Thread *tail=NULL;//Tail of the list
        //Special case: removing first element in the list
        while(thread_list[i]->flags.isDeleted())
        {
            if(thread_list[i]->schedData.next==thread_list[i])
            {
                //Only one element in the list
                //Call destructor manually because of placement new
                void *base=thread_list[i]->watermark;
                thread_list[i]->~Thread();
                free(base); //Delete ALL thread memory
                thread_list[i]=NULL;
                break;
            }
            //If it is the first time the tail of the list hasn't
            //been calculated
            if(first==false)
            {
                first=true;
                tail=thread_list[i];
                while(tail->schedData.next!=thread_list[i])
                    tail=tail->schedData.next;
            }
            Thread *d=thread_list[i];//Save a pointer to the thread
            thread_list[i]=thread_list[i]->schedData.next;//Remove from list
            //Fix the tail of the circular list
            tail->schedData.next=thread_list[i];
            //Call destructor manually because of placement new
            void *base=d->watermark;
            d->~Thread();
            free(base);//Delete ALL thread memory
        }
        if(thread_list[i]==NULL) continue;
        //If it comes here, the first item is not NULL, and doesn't have
        //to be deleted General case: removing items not at the first
        //place
        Thread *temp=thread_list[i];
        for(;;)
        {
            if(temp->schedData.next==thread_list[i]) break;
            if(temp->schedData.next->flags.isDeleted())
            {
                Thread *d=temp->schedData.next;//Save a pointer to the thread
                //Remove from list
                temp->schedData.next=temp->schedData.next->schedData.next;
                //Call destructor manually because of placement new
                void *base=d->watermark;
                d->~Thread();
                free(base);//Delete ALL thread memory
            } else temp=temp->schedData.next;
        }
    }
}

void PriorityScheduler::PKsetPriority(Thread *thread,
        PrioritySchedulerPriority newPriority)
{
    PrioritySchedulerPriority oldPriority=getPriority(thread);
    //First set priority to the new value
    thread->schedData.priority=newPriority;
    //Then remove the thread from its old list
    if(thread_list[oldPriority.get()]==thread)
    {
        if(thread_list[oldPriority.get()]->schedData.next==
                thread_list[oldPriority.get()])
        {
            //Only one element in the list
            thread_list[oldPriority.get()]=NULL;
        } else {
            Thread *tail=thread_list[oldPriority.get()];//Tail of the list
            while(tail->schedData.next!=thread_list[oldPriority.get()])
                tail=tail->schedData.next;
            //Remove
            thread_list[oldPriority.get()]=
                    thread_list[oldPriority.get()]->schedData.next;
            //Fix tail of the circular list
            tail->schedData.next=thread_list[oldPriority.get()];
        }
    } else {
        //If it comes here, the first item doesn't have to be removed
        //General case: removing item not at the first place
        Thread *temp=thread_list[oldPriority.get()];
        for(;;)
        {
            if(temp->schedData.next==thread_list[oldPriority.get()])
            {
                //After walking all elements in the list the thread wasn't found
                //This should never happen
                errorHandler(UNEXPECTED);
            }
            if(temp->schedData.next==thread)
            {
                //Remove from list
                temp->schedData.next=temp->schedData.next->schedData.next;
                break;
            } else temp=temp->schedData.next;
        }
    }
    //Last insert the thread in the new list
    if(thread_list[newPriority.get()]==NULL)
    {
        thread_list[newPriority.get()]=thread;
        thread->schedData.next=thread;//Circular list
    } else {
        thread->schedData.next=thread_list[newPriority.get()]->schedData.next;
        thread_list[newPriority.get()]->schedData.next=thread;
    }
}

void PriorityScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle=idleThread;
}

void PriorityScheduler::IRQfindNextThread()
{
    if(kernel_running!=0) return;//If kernel is paused, do nothing
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(thread_list[i]==NULL) continue;
        Thread *temp=thread_list[i]->schedData.next;
        for(;;)
        {
            if(temp->flags.isReady())
            {
                //Found a READY thread, so run this one
                cur=temp;
                #ifdef WITH_PROCESSES
                if(const_cast<Thread*>(cur)->flags.isInUserspace()==false)
                {
                    ctxsave=cur->ctxsave;
                    MPUConfiguration::IRQdisable();
                } else {
                    ctxsave=cur->userCtxsave;
                    //A kernel thread is never in userspace, so the cast is safe
                    static_cast<Process*>(cur->proc)->mpu.IRQenable();
                }
                #else //WITH_PROCESSES
                ctxsave=temp->ctxsave;
                #endif //WITH_PROCESSES
                //Rotate to next thread so that next time the list is walked
                //a different thread, if available, will be chosen first
                thread_list[i]=temp;
                return;
            } else temp=temp->schedData.next;
            if(temp==thread_list[i]->schedData.next) break;
        }
    }
    //No thread found, run the idle thread
    cur=idle;
    ctxsave=idle->ctxsave;
    #ifdef WITH_PROCESSES
    MPUConfiguration::IRQdisable();
    #endif //WITH_PROCESSES
}

Thread *PriorityScheduler::thread_list[PRIORITY_MAX]={0};
Thread *PriorityScheduler::idle=0;

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
