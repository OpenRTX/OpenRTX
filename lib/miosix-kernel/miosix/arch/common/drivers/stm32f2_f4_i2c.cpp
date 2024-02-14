/***************************************************************************
 *   Copyright (C) 2013-2022 by Terraneo Federico and Silvano Seva         *
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

#include "stm32f2_f4_i2c.h"
#include <miosix.h>
#include <kernel/scheduler/scheduler.h>

using namespace miosix;

static volatile bool error;     ///< Set to true by IRQ on error
static Thread *waiting=nullptr; ///< Thread waiting for an operation to complete

/**
 * DMA I2C rx end of transfer
 */
void __attribute__((naked)) DMA1_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z20I2C1rxDmaHandlerImplv");
    restoreContext();
}

/**
 * DMA I2C rx end of transfer actual implementation
 */
void __attribute__((used)) I2C1rxDmaHandlerImpl()
{
    DMA1->LIFCR=DMA_LIFCR_CTCIF0
              | DMA_LIFCR_CTEIF0
              | DMA_LIFCR_CDMEIF0
              | DMA_LIFCR_CFEIF0;
    if(waiting==nullptr) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=nullptr;
}

/**
 * DMA I2C tx end of transfer
 */
void DMA1_Stream7_IRQHandler()
{
    DMA1->HIFCR=DMA_HIFCR_CTCIF7
              | DMA_HIFCR_CTEIF7
              | DMA_HIFCR_CDMEIF7
              | DMA_HIFCR_CFEIF7;
    //We can't just wake the thread because the I2C is double buffered, and this
    //interrupt is fired at the same time as the second last byte is starting
    //to be sent out of the bus. If we return now, the main code would send a
    //stop condiotion too soon, and the last byte would never be sent. Instead,
    //we change from DMA mode to IRQ mode, so when the second last byte is sent,
    //that interrupt is fired and the last byte is sent out.
    //Note that since no thread is awakened from this IRQ, there's no need for
    //the saveContext(), restoreContext() and __attribute__((naked))
    I2C1->CR2 &= ~I2C_CR2_DMAEN;
    I2C1->CR2 |= I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN;
}

/**
 * I2C address sent interrupt
 */
void __attribute__((naked)) I2C1_EV_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15I2C1HandlerImplv");
    restoreContext();
}

/**
 * I2C address sent interrupt actual implementation
 */
void __attribute__((used)) I2C1HandlerImpl()
{
    //When called to resolve the last byte not sent issue, clearing
    //I2C_CR2_ITBUFEN prevents this interrupt being re-entered forever, as
    //it does not send another byte to the I2C, so the interrupt would remain
    //pending. When called after the start bit has been sent, clearing
    //I2C_CR2_ITEVTEN prevents the same infinite re-enter as this interrupt
    //does not start an address transmission, which is necessary to stop
    //this interrupt from being pending
    I2C1->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN);
    if(waiting==nullptr) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=nullptr;
}

/**
 * I2C error interrupt
 */
void __attribute__((naked)) I2C1_ER_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z18I2C1errHandlerImplv");
    restoreContext();
}

/**
 * I2C error interrupt actual implementation
 */
void __attribute__((used)) I2C1errHandlerImpl()
{
    I2C1->SR1=0; //Clear error flags
    error=true;
    if(waiting==nullptr) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=nullptr;
}

namespace miosix {

//
// class I2C
//

I2C1Master::I2C1Master(GpioPin sda, GpioPin scl, int frequency)
{
    if(checkMultipleInstances) errorHandler(UNEXPECTED);
    checkMultipleInstances=true;

    //I2C devices are connected to APB1, whose frequency is the system clock
    //divided by a value set in the PPRE1 bits of RCC->CFGR
    const int ppre1=(RCC->CFGR & RCC_CFGR_PPRE1)>>10;
    const int divFactor= (ppre1 & 1<<2) ? (2<<(ppre1 & 0x3)) : 1;
    const int fpclk1=SystemCoreClock/divFactor;
    //iprintf("fpclk1=%d\n",fpclk1);
    
    {
        FastInterruptDisableLock dLock;
        // NOTE: GPIOs need to be configured before enabling the peripheral or
        // the first read/write call blocks forever. Smells like a hw bug
        // NOTE: ALTERNATE_OD as the I2C peripheral doesn't enforce open drain
        sda.alternateFunction(4);
        sda.mode(Mode::ALTERNATE_OD);
        scl.alternateFunction(4);
        scl.mode(Mode::ALTERNATE_OD);
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; //Enable clock gating
        RCC_SYNC();
    }
    
    NVIC_SetPriority(DMA1_Stream7_IRQn,10);//Low priority for DMA
    NVIC_ClearPendingIRQ(DMA1_Stream7_IRQn);//DMA1 stream 7 channel 1 = I2C1 TX 
    NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    
    NVIC_SetPriority(DMA1_Stream0_IRQn,10);//Low priority for DMA
    NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);//DMA1 stream 0 channel 1 = I2C1 RX 
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    
    NVIC_SetPriority(I2C1_EV_IRQn,10);//Low priority for I2C
    NVIC_ClearPendingIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
    
    NVIC_SetPriority(I2C1_ER_IRQn,10);
    NVIC_ClearPendingIRQ(I2C1_ER_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);

    I2C1->CR1=I2C_CR1_SWRST;
    I2C1->CR1=0;
    I2C1->CR2=fpclk1/1000000; //Set pclk frequency in MHz

    //Clamp to a reasonable range, but only 100 and 400 are officially supported
    frequency=std::max(10,std::min(1000,frequency));
    if(frequency>100)
    {
        I2C1->CCR=std::max(4,fpclk1/(3000*frequency)) | I2C_CCR_FS;
        /* 
         * TRISE sets the maximum SCL rise time. Reading the I2C specs:
         * 400KHz (2.5us) I2C has maximum rise time 300ns, ratio 8.333
         *   1MHz (1us)   I2C has maximum rise time 120ns, ratio 8.333
         * Although higher frequencies than 400kHz are not officially supported,
         * to allow some overclocking, we'll set TRISE with the "8.333 rule".
         * I2Period[s] = 1 / I2CFrequency[Hz]
         * RISETIME[s] = I2CPeriod[s] / 8.333
         * K = 1 / RISETIME
         * TRISE = (fpclk1/K)+1
         * Putting it all together,
         * K = I2CFrequency[Hz] * 8.333 = I2CFrequency[kHz] * 8333
         */
        I2C1->TRISE=fpclk1/(frequency*8333)+1;
    } else {
        // With full speed mode disabled, we need to divide by 2, not 3
        I2C1->CCR=std::max(4,fpclk1/(2000*frequency));
        // 100kHz I2C has 1000ns rise time, that does not follow the 8.333 rule
        I2C1->TRISE=fpclk1/1000000+1;
    }

    I2C1->CR1=I2C_CR1_PE; //Enable peripheral
}

bool I2C1Master::recv(unsigned char address, void *data, int len)
{
    if(len<=0 || len>0xffff) return false;
    address |= 0x01;
    if(startWorkaround(address,len)==false || I2C1->SR2 & I2C_SR2_TRA)
    {
        fastEnableInterrupts(); //HACK Workaround critical section end
        stop();
        return false;
    }

    error=false;
    waiting=Thread::IRQgetCurrentThread();
    
    I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_LAST | I2C_CR2_ITERREN;
        
    DMA1_Stream0->CR=0;
    DMA1_Stream0->PAR=reinterpret_cast<unsigned int>(&I2C1->DR);
    DMA1_Stream0->M0AR=reinterpret_cast<unsigned int>(data);
    DMA1_Stream0->NDTR=len;
    DMA1_Stream0->FCR=DMA_SxFCR_FEIE
                    | DMA_SxFCR_DMDIS;
    DMA1_Stream0->CR=DMA_SxCR_CHSEL_0 //Channel 1
                   | DMA_SxCR_MINC    //Increment memory pointer
                   | DMA_SxCR_TCIE    //Interrupt on transfer complete
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start DMA

    fastEnableInterrupts(); //HACK Workaround critical section end

    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }

    DMA1_Stream7->CR=0;
    
    I2C1->CR2 &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST | I2C_CR2_ITERREN);
    
    stop();
    return !error;
}

bool I2C1Master::send(unsigned char address, const void *data, int len, bool sendStop)
{
    if(len<=0 || len>0xffff) return false;
    address &= 0xfe; //Mask bit 0, as we are writing
    if(start(address)==false || (I2C1->SR2 & I2C_SR2_TRA)==0)
    {
        stop();
        return false;
    }

    error=false;

    waiting=Thread::getCurrentThread();
    DMA1_Stream7->CR=0;
    DMA1_Stream7->PAR=reinterpret_cast<unsigned int>(&I2C1->DR);
    DMA1_Stream7->M0AR=reinterpret_cast<unsigned int>(data);
    DMA1_Stream7->NDTR=len;
    DMA1_Stream7->FCR=DMA_SxFCR_FEIE
                    | DMA_SxFCR_DMDIS;
    DMA1_Stream7->CR=DMA_SxCR_CHSEL_0 //Channel 1
                   | DMA_SxCR_MINC    //Increment memory pointer
                   | DMA_SxCR_DIR_0   //Memory to peripheral
                   | DMA_SxCR_TCIE    //Interrupt on transfer complete
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start DMA
    
    //Enable DMA in the I2C peripheral *after* having configured the DMA
    //peripheral, or a spurious interrupt is triggered
    I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_ITERREN;
 
    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    
    DMA1_Stream7->CR=0;
    
    //The DMA interrupt routine changes the interrupt flags!
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);

    if(sendStop) stop();
    return !error;
}

I2C1Master::~I2C1Master()
{
    I2C1->CR1=I2C_CR1_SWRST;
    I2C1->CR1=0;

    NVIC_DisableIRQ(DMA1_Stream7_IRQn);
    NVIC_DisableIRQ(DMA1_Stream0_IRQn);
    NVIC_DisableIRQ(I2C1_EV_IRQn);
    NVIC_DisableIRQ(I2C1_ER_IRQn);

    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
        RCC_SYNC();
    }
    checkMultipleInstances=false;
}

bool I2C1Master::start(unsigned char address)
{
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    if(!waitStatus1()) return false;
    //EV5: Must read SR1 to clear SB
    if((I2C1->SR1 & I2C_SR1_SB)==0) return false;
    I2C1->DR=address;
    if(!waitStatus1()) return false;
    //EV6: Must read SR1 and SR2 to clear ADDR
    bool result=true;
    if(I2C1->SR1 & I2C_SR1_AF) result=false;
    if((I2C1->SR2 & I2C_SR2_MSL)==0) result=false;
    return result;
}

bool I2C1Master::startWorkaround(unsigned char address, int len)
{
    /*
     * This function should't exist if this peripheral had been designed in a
     * sane way. Unfortunately, reading from the I2C hides a quirk that made
     * code using this driver deadlock, sometimes. How? When doing an I2C read,
     * as soon as the condition called EV6 in the manual is cleared (a cryptic
     * name for "clear the interrupt flag signaling that the address has been
     * sent"), the peripheral starts reading from the bus. On its own! And the
     * software has to keep up servicing events and setting when to send a NACK
     * because the last byte has been sent, otherwise three bad things happen:
     * 1) The peripheral may read more data from the bus than requested
     * 2) It may be too late to send a NACK at the last byte, and an ACK is sent
     * 3) The peripheral may not send the required interrupts anymore
     * deadlocking the software.
     * But there's the DMA, you may say, shouldn't it automate all this?
     * Now, problem was if after clearing EV6 and before configuring the DMA the
     * OS decided to do a context switch, then the three bad things happened.
     * A clean software solution to cope with this bad hardware design would be
     * to create some sort of interrupt driven FSM that would react to events
     * and prepare for the next one, relying on the atomicity of interrupts.
     * Maybe this solution could even work with the notoriously buggy STM32F1
     * I2C peripheral. But for now, we'll just create a critical section by
     * disabling interrupts.
     * Oh, and another necessary quirk this function implements, when receiving
     * a single byte unless you do clear I2C_CR1_ACK before EV6, everything
     * locks up. Fragile hardware it is...
     */
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    if(!waitStatus1()) return false;
    //EV5: Must read SR1 to clear SB
    if((I2C1->SR1 & I2C_SR1_SB)==0) return false;
    I2C1->DR=address;
    if(!waitStatus1()) return false;
    //If the transfer is single byte, then ACK must be reset before EV6 cleared
    if(len==1) I2C1->CR1 &= ~I2C_CR1_ACK;
    fastDisableInterrupts(); //HACK Workaround critical section start
    bool result=true;
    //EV6: Must read SR1 and SR2 to clear ADDR
    if(I2C1->SR1 & I2C_SR1_AF) result=false;
    if((I2C1->SR2 & I2C_SR2_MSL)==0) result=false;
    return result;
}

bool I2C1Master::waitStatus1()
{
    error=false;
    waiting=Thread::getCurrentThread();
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    return !error;
}

void I2C1Master::stop()
{
    /*
     * The main idea of this driver is to avoid having the processor spinning
     * waiting on some status flag. Why? Because I2C is slow compared to a
     * modern processor. A 120MHz core does 1200 clock cycles in the time it
     * takes to transfer a single bit through an I2C clocked at 100KHz.
     * This time could be better spent doing a context switch and letting
     * another thread do useful work, or (and Miosix does it automatically if
     * there are no ready threads) sleeping the processor core. However,
     * I'm quite disappointed by the STM32 I2C peripheral, as it seems overly
     * complicated to use. To come close to achieving this goal I had to
     * orchestrate among *four* interrupt handlers, two of the DMA, and two
     * of the I2C itself. And in the end, what's even more disappointing, is
     * that I haven't found a way to completely avoid spinning. Why?
     * There's no interrupt that's fired when the stop bit is sent!
     * And what's worse, the documentation says that after you set the stop
     * bit in the CR2 register you can't write to it again (for example, to send
     * a start bit because two i2c api calls are made back to back) until the
     * MSL bit is cleared. But there's no interrupt tied to that event!
     * What's worse, is that the closest interrupt flag I've found when doing
     * an I2C send is fired when the last byte is *beginning* to be sent.
     * Maybe I haven't searched well enough, but the fact is I found nothing,
     * so this code below spins for 8 data bits of the last byte plus the ack
     * bit, plus the stop bit. That's 12000 wasted CPU cycles. Thanks, ST...
     */
    I2C1->CR1 |= I2C_CR1_STOP;
    while(I2C1->SR2 & I2C_SR2_MSL) ; //Wait for stop bit sent
}

bool I2C1Master::checkMultipleInstances=false;

} //namespace miosix
