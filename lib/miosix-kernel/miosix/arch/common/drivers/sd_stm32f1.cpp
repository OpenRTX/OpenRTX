/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#include "sd_stm32f1.h"
#include "interfaces/bsp.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/scheduler.h"
#include "board_settings.h" //For sdVoltage
#include <cstdio>
#include <cstring>
#include <errno.h>

/*
 * This driver is quite a bit complicated, due to a silicon errata in the
 * STM32F1 microcontrollers, that prevents concurrent access to the FSMC
 * (i.e., the external memory controller) by both the CPU and DMA.
 * Therefore, if __ENABLE_XRAM is defined, the SDIO peripheral is used in
 * polled mode, otherwise in DMA mode. The use in polled mode is further
 * complicated by the fact that the SDIO peripheral does not halt the clock
 * to the SD card if its internal fifo is full. Therefore, when using the
 * SDIO in polled mode the only solution is to disable interrupts during
 * the data transfer. To optimize reading and writing speed this code
 * automatically chooses the best transfer speed using a binary search during
 * card initialization. Also, other sources of mess are the requirement for
 * word alignment of pointers when doing DMA transfers or writing to the SDIO
 * peripheral. Because of that, tryng to fwrite() large bloks of data is faster
 * if they are word aligned. An easy way to do so is to allocate them on the
 * heap (and not doing any pointer arithmetic on the value returned by
 * malloc/new)
 */

//Note: enabling debugging might cause deadlock when using sleep() or reboot()
//The bug won't be fixed because debugging is only useful for driver development
///\internal Debug macro, for normal conditions
//#define DBG iprintf
#define DBG(x,...) do {} while(0)
///\internal Debug macro, for errors only
//#define DBGERR iprintf
#define DBGERR(x,...) do {} while(0)

#ifndef __ENABLE_XRAM
/**
 * \internal
 * DMA2 Channel4 interrupt handler
 */
void __attribute__((naked)) DMA2_Channel4_5_IRQHandler()
{
    saveContext();
    asm volatile("bl _ZN6miosix19DMA2channel4irqImplEv");
    restoreContext();
}

/**
 * \internal
 * SDIO interrupt handler
 */
void __attribute__((naked)) SDIO_IRQHandler()
{
    saveContext();
    asm volatile("bl _ZN6miosix11SDIOirqImplEv");
    restoreContext();
}
#endif //__ENABLE_XRAM

namespace miosix {

#ifndef __ENABLE_XRAM
static volatile bool transferError; ///< \internal DMA or SDIO transfer error
static Thread *waiting;             ///< \internal Thread waiting for transfer
static unsigned int dmaFlags;       ///< \internal DMA status flags
static unsigned int sdioFlags;      ///< \internal SDIO status flags

/**
 * \internal
 * DMA2 Channel4 interrupt handler actual implementation
 */
void __attribute__((used)) DMA2channel4irqImpl()
{
    dmaFlags=DMA2->ISR;
    if(dmaFlags & DMA_ISR_TEIF4) transferError=true;
    
    DMA2->IFCR=DMA_IFCR_CGIF4;
    
    if(!waiting) return;
    waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * \internal
 * DMA2 Channel4 interrupt handler actual implementation
 */
void __attribute__((used)) SDIOirqImpl()
{
    sdioFlags=SDIO->STA;
    if(sdioFlags & (SDIO_STA_STBITERR | SDIO_STA_RXOVERR  |
                    SDIO_STA_TXUNDERR | SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL))
        transferError=true;
    
    SDIO->ICR=0x7ff;//Clear flags
    
    if(!waiting) return;
    waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    waiting=0;
}
#endif //__ENABLE_XRAM

/*
 * Operating voltage of device. It is sent to the SD card to check if it can
 * work at this voltage. Range *must* be within 28..36
 * Example 33=3.3v
 */
//const unsigned char sdVoltage=33; //Is defined in board_settings.h
const unsigned int sdVoltageMask=1<<(sdVoltage-13); //See OCR register in SD spec

/**
 * \internal
 * Possible state of the cardType variable.
 */
enum CardType
{
    Invalid=0, ///<\internal Invalid card type
    MMC=1<<0,  ///<\internal if(cardType==MMC) card is an MMC
    SDv1=1<<1, ///<\internal if(cardType==SDv1) card is an SDv1
    SDv2=1<<2, ///<\internal if(cardType==SDv2) card is an SDv2
    SDHC=1<<3  ///<\internal if(cardType==SDHC) card is an SDHC
};

///\internal Type of card.
static CardType cardType=Invalid;

//SD card GPIOs
typedef Gpio<GPIOC_BASE,8>  sdD0;
typedef Gpio<GPIOC_BASE,9>  sdD1;
typedef Gpio<GPIOC_BASE,10> sdD2;
typedef Gpio<GPIOC_BASE,11> sdD3;
typedef Gpio<GPIOC_BASE,12> sdCLK;
typedef Gpio<GPIOD_BASE,2>  sdCMD;

//
// Class BufferConverter
//

/**
 * \internal
 * Convert a single buffer of *fixed* and predetermined size to and from
 * word-aligned. To do so, if the buffer is already word aligned a cast is made,
 * otherwise a new buffer is allocated.
 * Note that this class allocates at most ONE buffer at any given time.
 * Therefore any call to toWordAligned(), toWordAlignedWithoutCopy(),
 * toOriginalBuffer() or deallocateBuffer() invalidates the buffer previousy
 * returned by toWordAligned() and toWordAlignedWithoutCopy()
 */
class BufferConverter
{
public:
    /**
     * \internal
     * The buffer will be of this size only.
     */
    static const int BUFFER_SIZE=512;

    /**
     * \internal
     * \return true if the pointer is word aligned
     */
    static bool isWordAligned(const void *x)
    {
        return (reinterpret_cast<const unsigned int>(x) & 0x3)==0;
    }

    /**
     * \internal
     * Convert from a constunsigned char* buffer of size BUFFER_SIZE to a
     * const unsigned int* word aligned buffer.
     * If the original buffer is already word aligned it only does a cast,
     * otherwise it copies the data on the original buffer to a word aligned
     * buffer. Useful if subseqent code will read from the buffer.
     * \param a buffer of size BUFFER_SIZE. Can be word aligned or not.
     * \return a word aligned buffer with the same data of the given buffer
     */
    static const unsigned int *toWordAligned(const unsigned char *buffer);

    /**
     * \internal
     * Convert from an unsigned char* buffer of size BUFFER_SIZE to an
     * unsigned int* word aligned buffer.
     * If the original buffer is already word aligned it only does a cast,
     * otherwise it returns a new buffer which *does not* contain the data
     * on the original buffer. Useful if subseqent code will write to the
     * buffer. To move the written data to the original buffer, use
     * toOriginalBuffer()
     * \param a buffer of size BUFFER_SIZE. Can be word aligned or not.
     * \return a word aligned buffer with undefined content.
     */
    static unsigned int *toWordAlignedWithoutCopy(unsigned char *buffer);

    /**
     * \internal
     * Convert the buffer got through toWordAlignedWithoutCopy() to the
     * original buffer. If the original buffer was word aligned, nothing
     * happens, otherwise a memcpy is done.
     * Note that this function does not work on buffers got through
     * toWordAligned().
     */
    static void toOriginalBuffer();

    /**
     * \internal
     * Can be called to deallocate the buffer
     */
    static void deallocateBuffer();

private:
    static unsigned char *originalBuffer;
    static unsigned int *wordAlignedBuffer;
};

const unsigned int *BufferConverter::toWordAligned(const unsigned char *buffer)
{
    originalBuffer=0; //Tell toOriginalBuffer() that there's nothing to do
    if(isWordAligned(buffer))
    {
        return reinterpret_cast<const unsigned int*>(buffer);
    } else {
        if(wordAlignedBuffer==0)
            wordAlignedBuffer=new unsigned int[BUFFER_SIZE/sizeof(unsigned int)];
        std::memcpy(wordAlignedBuffer,buffer,BUFFER_SIZE);
        return wordAlignedBuffer;
    }
}

unsigned int *BufferConverter::toWordAlignedWithoutCopy(
    unsigned char *buffer)
{
    if(isWordAligned(buffer))
    {
        originalBuffer=0; //Tell toOriginalBuffer() that there's nothing to do
        return reinterpret_cast<unsigned int*>(buffer);
    } else {
        originalBuffer=buffer; //Save original pointer for toOriginalBuffer()
        if(wordAlignedBuffer==0)
            wordAlignedBuffer=new unsigned int[BUFFER_SIZE/sizeof(unsigned int)];
        return wordAlignedBuffer;
    }
}

void BufferConverter::toOriginalBuffer()
{
    if(originalBuffer==0) return;
    std::memcpy(originalBuffer,wordAlignedBuffer,BUFFER_SIZE);
    originalBuffer=0;
}

void BufferConverter::deallocateBuffer()
{
    originalBuffer=0; //Invalidate also original buffer
    if(wordAlignedBuffer!=0)
    {
        delete[] wordAlignedBuffer;
        wordAlignedBuffer=0;
    }
}

unsigned char *BufferConverter::originalBuffer=0;
unsigned int *BufferConverter::wordAlignedBuffer=0;

//
// Class CmdResult
//

/**
 * \internal
 * Contains the result of an SD/MMC command
 */
class CmdResult
{
public:

    /**
     * \internal
     * Possible outcomes of sending a command
     */
    enum Error
    {
        Ok=0,        /// No errors
        Timeout,     /// Timeout while waiting command reply
        CRCFail,     /// CRC check failed in command reply
        RespNotMatch,/// Response index does not match command index
        ACMDFail     /// Sending CMD55 failed
    };

    /**
     * \internal
     * Default constructor
     */
    CmdResult(): cmd(0), error(Ok), response(0) {}

    /**
     * \internal
     * Constructor,  set the response data
     * \param cmd command index of command that was sent
     * \param result result of command
     */
    CmdResult(unsigned char cmd, Error error): cmd(cmd), error(error),
            response(SDIO->RESP1) {}

    /**
     * \internal
     * \return the 32 bit of the response.
     * May not be valid if getError()!=Ok or the command does not send a
     * response, such as CMD0
     */
    unsigned int getResponse() { return response; }

    /**
     * \internal
     * \return command index
     */
    unsigned char getCmdIndex() { return cmd; }

    /**
     * \internal
     * \return the error flags of the response
     */
    Error getError() { return error; }

    /**
     * \internal
     * Checks if errors occurred while sending the command.
     * \return true if no errors, false otherwise
     */
    bool validateError();

    /**
     * \internal
     * interprets this->getResponse() as an R1 response, and checks if there are
     * errors, or everything is ok
     * \return true on success, false on failure
     */
    bool validateR1Response();

    /**
     * \internal
     * Same as validateR1Response, but can be called with interrupts disabled.
     * \return true on success, false on failure
     */
    bool IRQvalidateR1Response();

    /**
     * \internal
     * interprets this->getResponse() as an R6 response, and checks if there are
     * errors, or everything is ok
     * \return true on success, false on failure
     */
    bool validateR6Response();

    /**
     * \internal
     * \return the card state from an R1 or R6 resonse
     */
    unsigned char getState();

private:
    unsigned char cmd; ///<\internal Command index that was sent
    Error error; ///<\internal possible error that occurred
    unsigned int response; ///<\internal 32bit response
};

bool CmdResult::validateError()
{
    switch(error)
    {
        case Ok:
            return true;
        case Timeout:
            DBGERR("CMD%d: Timeout\n",cmd);
            break;
        case CRCFail:
            DBGERR("CMD%d: CRC Fail\n",cmd);
            break;
        case RespNotMatch:
            DBGERR("CMD%d: Response does not match\n",cmd);
            break;
        case ACMDFail:
            DBGERR("CMD%d: ACMD Fail\n",cmd);
            break;
    }
    return false;
}

bool CmdResult::validateR1Response()
{
    if(error!=Ok) return validateError();
    //Note: this number is obtained with all the flags of R1 which are errors
    //(flagged as E in the SD specification), plus CARD_IS_LOCKED because
    //locked card are not supported by this software driver
    if((response & 0xfff98008)==0) return true;
    DBGERR("CMD%d: R1 response error(s):\n",cmd);
    if(response & (1<<31)) DBGERR("Out of range\n");
    if(response & (1<<30)) DBGERR("ADDR error\n");
    if(response & (1<<29)) DBGERR("BLOCKLEN error\n");
    if(response & (1<<28)) DBGERR("ERASE SEQ error\n");
    if(response & (1<<27)) DBGERR("ERASE param\n");
    if(response & (1<<26)) DBGERR("WP violation\n");
    if(response & (1<<25)) DBGERR("card locked\n");
    if(response & (1<<24)) DBGERR("LOCK_UNLOCK failed\n");
    if(response & (1<<23)) DBGERR("command CRC failed\n");
    if(response & (1<<22)) DBGERR("illegal command\n");
    if(response & (1<<21)) DBGERR("ECC fail\n");
    if(response & (1<<20)) DBGERR("card controller error\n");
    if(response & (1<<19)) DBGERR("unknown error\n");
    if(response & (1<<16)) DBGERR("CSD overwrite\n");
    if(response & (1<<15)) DBGERR("WP ERASE skip\n");
    if(response & (1<<3)) DBGERR("AKE_SEQ error\n");
    return false;
}

bool CmdResult::IRQvalidateR1Response()
{
    if(error!=Ok) return false;
    if(response & 0xfff98008) return false;
    return true;
}

bool CmdResult::validateR6Response()
{
    if(error!=Ok) return validateError();
    if((response & 0xe008)==0) return true;
    DBGERR("CMD%d: R6 response error(s):\n",cmd);
    if(response & (1<<15)) DBGERR("command CRC failed\n");
    if(response & (1<<14)) DBGERR("illegal command\n");
    if(response & (1<<13)) DBGERR("unknown error\n");
    if(response & (1<<3)) DBGERR("AKE_SEQ error\n");
    return false;
}

unsigned char CmdResult::getState()
{
    unsigned char result=(response>>9) & 0xf;
    DBG("CMD%d: State: ",cmd);
    switch(result)
    {
        case 0:  DBG("Idle\n");  break;
        case 1:  DBG("Ready\n"); break;
        case 2:  DBG("Ident\n"); break;
        case 3:  DBG("Stby\n"); break;
        case 4:  DBG("Tran\n"); break;
        case 5:  DBG("Data\n"); break;
        case 6:  DBG("Rcv\n"); break;
        case 7:  DBG("Prg\n"); break;
        case 8:  DBG("Dis\n"); break;
        case 9:  DBG("Btst\n"); break;
        default: DBG("Unknown\n"); break;
    }
    return result;
}

//
// Class Command
//

/**
 * \internal
 * This class allows sending commands to an SD or MMC
 */
class Command
{
public:

    /**
     * \internal
     * SD/MMC commands
     * - bit #7 is @ 1 if a command is an ACMDxx. send() will send the
     *   sequence CMD55, CMDxx
     * - bit from #0 to #5 indicate command index (CMD0..CMD63)
     * - bit #6 is don't care
     */
    enum CommandType
    {
        CMD0=0,           //GO_IDLE_STATE
        CMD2=2,           //ALL_SEND_CID
        CMD3=3,           //SEND_RELATIVE_ADDR
        ACMD6=0x80 | 6,   //SET_BUS_WIDTH
        CMD7=7,           //SELECT_DESELECT_CARD
        ACMD41=0x80 | 41, //SEND_OP_COND (SD)
        CMD8=8,           //SEND_IF_COND
        CMD9=9,           //SEND_CSD
        CMD12=12,         //STOP_TRANSMISSION
        CMD13=13,         //SEND_STATUS
        CMD16=16,         //SET_BLOCKLEN
        CMD17=17,         //READ_SINGLE_BLOCK
        CMD18=18,         //READ_MULTIPLE_BLOCK
        ACMD23=0x80 | 23, //SET_WR_BLK_ERASE_COUNT (SD)
        CMD24=24,         //WRITE_BLOCK
        CMD25=25,         //WRITE_MULTIPLE_BLOCK
        CMD55=55          //APP_CMD
    };

    /**
     * \internal
     * Send a command.
     * \param cmd command index (CMD0..CMD63) or ACMDxx command
     * \param arg the 32 bit argument to the command
     * \return a CmdResult object
     */
    static CmdResult send(CommandType cmd, unsigned int arg)
    {
        if(static_cast<unsigned char>(cmd) & 0x80)
        {
            DBG("ACMD%d\n",static_cast<unsigned char>(cmd) & 0x3f);
        } else {
            DBG("CMD%d\n",static_cast<unsigned char>(cmd) & 0x3f);
        }
        return IRQsend(cmd,arg);
    }

    /**
     * \internal
     * Send a command. Can be called with interrupts disabled as it does not
     * print any debug information.
     * \param cmd command index (CMD0..CMD63) or ACMDxx command
     * \param arg the 32 bit argument to the command
     * \return a CmdResult object
     */
    static CmdResult IRQsend(CommandType cmd, unsigned int arg);

    /**
     * \internal
     * Set the relative card address, obtained during initialization.
     * \param r the card's rca
     */
    static void setRca(unsigned short r) { rca=r; }

    /**
     * \internal
     * \return the card's rca, as set by setRca
     */
    static unsigned int getRca() { return static_cast<unsigned int>(rca); }

private:
    static unsigned short rca;///<\internal Card's relative address
};

CmdResult Command::IRQsend(CommandType cmd, unsigned int arg)
{
    unsigned char cc=static_cast<unsigned char>(cmd);
    //Handle ACMDxx as CMD55, CMDxx
    if(cc & 0x80)
    {
        CmdResult r=IRQsend(CMD55,(static_cast<unsigned int>(rca))<<16);
        if(r.IRQvalidateR1Response()==false)
            return CmdResult(cc & 0x3f,CmdResult::ACMDFail);
        //Bit 5 @ 1 = next command will be interpreted as ACMD
        if((r.getResponse() & (1<<5))==0)
            return CmdResult(cc & 0x3f,CmdResult::ACMDFail);
    }

    //Send command
    cc &= 0x3f;
    unsigned int command=SDIO_CMD_CPSMEN | static_cast<unsigned int>(cc);
    if(cc!=CMD0) command |= SDIO_CMD_WAITRESP_0; //CMD0 has no response
    if(cc==CMD2) command |= SDIO_CMD_WAITRESP_1; //CMD2 has long response
    if(cc==CMD9) command |= SDIO_CMD_WAITRESP_1; //CMD9 has long response
    SDIO->ARG=arg;
    SDIO->CMD=command;

    //CMD0 has no response, so wait until it is sent
    if(cc==CMD0)
    {
        for(int i=0;i<500;i++)
        {
            if(SDIO->STA & SDIO_STA_CMDSENT)
            {
                SDIO->ICR=0x7ff;//Clear flags
                return CmdResult(cc,CmdResult::Ok);
            }
            delayUs(1);
        }
        SDIO->ICR=0x7ff;//Clear flags
        return CmdResult(cc,CmdResult::Timeout);
    }

    //Command is not CMD0, so wait a reply
    for(int i=0;i<500;i++)
    {
        unsigned int status=SDIO->STA;
        if(status & SDIO_STA_CMDREND)
        {
            SDIO->ICR=0x7ff;//Clear flags
            if(SDIO->RESPCMD==cc) return CmdResult(cc,CmdResult::Ok);
            else return CmdResult(cc,CmdResult::RespNotMatch);
        }
        if(status & SDIO_STA_CCRCFAIL)
        {
            SDIO->ICR=SDIO_ICR_CCRCFAILC;
            return CmdResult(cc,CmdResult::CRCFail);
        }
        if(status & SDIO_STA_CTIMEOUT) break;
        delayUs(1);
    }
    SDIO->ICR=SDIO_ICR_CTIMEOUTC;
    return CmdResult(cc,CmdResult::Timeout);
}

unsigned short Command::rca=0;

//
// Class DataResult
//

/**
 * \internal
 * Contains the result of sending/receiving a data block
 */
class DataResult
{
public:

    /**
     * \internal
     * Possible outcomes of sending or receiving data
     */
    enum Error
    {
        Ok=0,
        Timeout,
        CRCFail,
        RXOverrun,
        TXUnderrun,
        StartBitFail
    };

    /**
     * \internal
     * Default constructor
     */
    DataResult(): error(Ok) {}

    /**
     * \internal
     * Constructor,  set the result.
     * \param error error type
     */
    DataResult(Error error): error(error) {}

    /**
     * \internal
     * \return the error flags
     */
    Error getError() { return error; }

    /**
     * \internal
     * Checks if errors occurred while sending/receiving data.
     * \return true if no errors, false otherwise
     */
    bool validateError();
    
private:
    Error error;
};


bool DataResult::validateError()
{
    switch(error)
    {
        case Ok:
            return true;
        case Timeout:
            DBGERR("Data Timeout\n");
            break;
        case CRCFail:
            DBGERR("Data CRC Fail\n");
            break;
        case RXOverrun:
            DBGERR("Data overrun\n");
            break;
        case TXUnderrun:
            DBGERR("Data underrun\n");
            break;
        case StartBitFail:
            DBGERR("Data start bit Fail\n");
            break;
    }
    return false;
}

//
// Class ClockController
//

/**
 * \internal
 * This class controls the clock speed of the SDIO peripheral. The SDIO
 * peripheral, when used in polled mode, requires two timing critical pieces of
 * code: the one to send and the one to receive a data block. This because
 * the peripheral has a 128 byte fifo while the block size is 512 byte, and
 * if fifo underrun/overrun occurs the peripheral does not pause communcation,
 * instead it simply aborts the data transfer. Since the speed of the code to
 * read/write a data block depends on too many factors, such as compiler
 * optimizations, code running from internal flash or external ram, and the
 * cpu clock speed, a dynamic clocking approach was chosen.
 */
class ClockController
{
public:

    /**
     * \internal. Set a low clock speed of 400KHz or less, used for
     * detecting SD/MMC cards. This function as a side effect enables 1bit bus
     * width, and disables clock powersave, since it is not allowed by SD spec.
     */
    static void setLowSpeedClock()
    {
        clockReductionAvailable=0;
        // No hardware flow control, SDIO_CK generated on rising edge, 1bit bus
        // width, no clock bypass, no powersave.
        // Set low clock speed 400KHz, 72MHz/400KHz-2=178
        SDIO->CLKCR=CLOCK_400KHz | SDIO_CLKCR_CLKEN;
        SDIO->DTIMER=240000; //Timeout 600ms expressed in SD_CK cycles
    }

    /**
     * \internal
     * Automatically select the data speed.
     * Since the maximum speed depends on many factors, such as code running in
     * internal or external RAM, compiler optimizations etc. this routine
     * selects the highest sustainable data transfer speed.
     * This is done by binary search until the highest clock speed that causes
     * no errors is found.
     * This function as a side effect enables 4bit bus width, and clock
     * powersave.
     */
    static void calibrateClockSpeed(SDIODriver *sdio);
    
    /**
     * \internal
     * Since clock speed is set dynamically by bynary search at runtime, a
     * corner case might be that of a clock speed which results in unreliable
     * data transfer, that sometimes succeeds, and sometimes fail.
     * For maximum robustness, this function is provided to reduce the clock
     * speed slightly in case a data transfer should fail after clock
     * calibration. To avoid inadvertently considering other kind of issues as
     * clock issues, this function can be called only MAX_ALLOWED_REDUCTIONS
     * times after clock calibration, subsequent calls will fail. This will
     * avoid other issues causing an ever decreasing clock speed.
     * \return true on success, false on failure
     */
    static bool reduceClockSpeed() { return IRQreduceClockSpeed(); }

    /**
     * \internal
     * Same as reduceClockSpeed(), can be called with interrupts disabled.
     * \return true on success, false on failure 
     */
    static bool IRQreduceClockSpeed();

    /**
     * \internal
     * Read and write operation do retry during normal use for robustness, but
     * during clock claibration they must not retry for speed reasons. This
     * member function returns 1 during clock claibration and MAX_RETRY during
     * normal use.
     */
    static unsigned char getRetryCount() { return retries; }

private:
    /**
     * Set SDIO clock speed
     * \param clkdiv speed is SDIOCLK/(clkdiv+2) 
     */
    static void setClockSpeed(unsigned int clkdiv);
    
    /**
     * \internal
     * Value of SDIO->CLKCR that will give a 400KHz clock, depending on cpu
     * clock speed.
     */
    #ifdef SYSCLK_FREQ_72MHz
    static const unsigned int CLOCK_400KHz=178;
    #elif SYSCLK_FREQ_56MHz
    static const unsigned int CLOCK_400KHz=138;
    #elif SYSCLK_FREQ_48MHz
    static const unsigned int CLOCK_400KHz=118;
    #elif SYSCLK_FREQ_36MHz
    static const unsigned int CLOCK_400KHz=88;
    #elif SYSCLK_FREQ_24MHz
    static const unsigned int CLOCK_400KHz=58;
    #else
    static const unsigned int CLOCK_400KHz=18;
    #endif

    ///\internal Clock enabled, bus width 4bit, clock powersave enabled.
    static const unsigned int CLKCR_FLAGS=SDIO_CLKCR_CLKEN |
        SDIO_CLKCR_WIDBUS_0 | SDIO_CLKCR_PWRSAV;
    
    ///\internal Maximum number of calls to IRQreduceClockSpeed() allowed
    ///When using polled mode this is a critical parameter, if SDIO driver
    ///starts to fail, it might be a good idea to increase this
    static const unsigned char MAX_ALLOWED_REDUCTIONS=7;

    ///\internal value returned by getRetryCount() while *not* calibrating clock.
    ///When using polled mode this is a critical parameter, if SDIO driver
    ///starts to fail, it might be a good idea to increase this
    static const unsigned char MAX_RETRY=10;

    ///\internal Used to allow only one call to reduceClockSpeed()
    static unsigned char clockReductionAvailable;

    static unsigned char retries;
};

void ClockController::calibrateClockSpeed(SDIODriver *sdio)
{
    //During calibration we call readBlock which will call reduceClockSpeed()
    //so not to invalidate calibration clock reduction must not be available
    clockReductionAvailable=0;
    retries=1;

    DBG("Automatic speed calibration\n");
    unsigned int buffer[512/sizeof(unsigned int)];
    unsigned int minFreq=CLOCK_400KHz; //400KHz, independent of CPU clock
    unsigned int maxFreq=1;            //24MHz  with CPU running @ 72MHz
    unsigned int selected;
    while(minFreq-maxFreq>1)
    {
        selected=(minFreq+maxFreq)/2;
        DBG("Trying CLKCR=%d\n",selected);
        setClockSpeed(selected);
        if(sdio->readBlock(reinterpret_cast<unsigned char*>(buffer),512,0)==512)
            minFreq=selected;
        else maxFreq=selected;
    }
    //Last round of algorithm
    setClockSpeed(maxFreq);
    if(sdio->readBlock(reinterpret_cast<unsigned char*>(buffer),512,0)==512)
    {
        DBG("Optimal CLKCR=%d\n",maxFreq);
    } else {
        setClockSpeed(minFreq);
        DBG("Optimal CLKCR=%d\n",minFreq);
    }

    //Make clock reduction available
    clockReductionAvailable=MAX_ALLOWED_REDUCTIONS;
    retries=MAX_RETRY;
}

bool ClockController::IRQreduceClockSpeed()
{
    //Ensure this function can be called only twice per calibration
    if(clockReductionAvailable==0) return false;
    clockReductionAvailable--;

    unsigned int currentClkcr=SDIO->CLKCR & 0xff;
    if(currentClkcr==CLOCK_400KHz) return false; //No lower than this value

    //If the value of clockcr is low, increasing it by one is enough since
    //frequency changes a lot, otherwise increase by 2.
    if(currentClkcr<10) currentClkcr++;
    else currentClkcr+=2;
    
    setClockSpeed(currentClkcr);
    return true;
}

void ClockController::setClockSpeed(unsigned int clkdiv)
{
    SDIO->CLKCR=clkdiv | CLKCR_FLAGS;
    //Timeout 600ms expressed in SD_CK cycles
    SDIO->DTIMER=(6*SystemCoreClock)/((clkdiv+2)*10);
}

unsigned char ClockController::clockReductionAvailable=false;
unsigned char ClockController::retries=ClockController::MAX_RETRY;

//
// Data send/receive functions
//

/**
 * \internal
 * Wait until the card is ready for data transfer.
 * Can be called independently of the card being selected.
 * \return true on success, false on failure
 */
static bool waitForCardReady()
{
    const int timeout=1500; //Timeout 1.5 second
    const int sleepTime=2;
    for(int i=0;i<timeout/sleepTime;i++) 
    {
        CmdResult cr=Command::send(Command::CMD13,Command::getRca()<<16);
        if(cr.validateR1Response()==false) return false;
        //Bit 8 in R1 response means ready for data.
        if(cr.getResponse() & (1<<8)) return true;
        Thread::sleep(sleepTime);
    }
    DBGERR("Timeout waiting card ready\n");
    return false;
}

#ifdef __ENABLE_XRAM

/**
 * \internal
 * Receive a data block. The end of the data block must be told to the SDIO
 * peripheral in SDIO->DLEN and must match the size parameter given to this
 * function.
 * \param buffer buffer where to store received data. Its size must be >=size
 * \param buffer size, which *must* be multiple of 8 words (32bytes)
 * Note that the size parameter must be expressed in word (4bytes), while
 * the value in SDIO->DLEN is expressed in bytes.
 * \return a DataResult object
 */
static DataResult IRQreceiveDataBlock(unsigned int *buffer, unsigned int size)
{
    // A note on speed.
    // Due to the auto calibration of SDIO clock speed being done with
    // IRQreceiveDataBlock(), the speed of this function must be comparable
    // with the speed of IRQsendDataBlock(), otherwise IRQsendDataBlock()
    // will fail because of data underrun.
    const unsigned int *bufend=buffer+size;
    unsigned int status;
    for(;;)
    {
        status=SDIO->STA;
        if(status & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL |
            SDIO_STA_DTIMEOUT | SDIO_STA_STBITERR | SDIO_STA_DBCKEND)) break;
        if((status & SDIO_STA_RXFIFOHF) && (buffer!=bufend))
        {
            //Read 8 words from the fifo, loop entirely unrolled for speed
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
            *buffer=SDIO->FIFO; buffer++;
        }
    }
    SDIO->ICR=0x7ff;//Clear flags
    if(status & SDIO_STA_RXOVERR) return DataResult(DataResult::RXOverrun);
    if(status & SDIO_STA_DCRCFAIL) return DataResult(DataResult::CRCFail);
    if(status & SDIO_STA_DTIMEOUT) return DataResult(DataResult::Timeout);
    if(status & SDIO_STA_STBITERR) return DataResult(DataResult::StartBitFail);
    //Read eventual data left in the FIFO
    for(;;)
    {
        if((SDIO->STA & SDIO_STA_RXDAVL)==0) break;
        *buffer=SDIO->FIFO; buffer++;
    }
    return DataResult(DataResult::Ok);
}

/**
 * \internal
 * Send a data block. The end of the data block must be told to the SDIO
 * peripheral in SDIO->DLEN and must match the size parameter given to this
 * function.
 * \param buffer buffer where to store received data. Its size must be >=size
 * \param buffer size, which *must* be multiple of 8 words (32bytes).
 * Note that the size parameter must be expressed in word (4bytes), while
 * the value in SDIO->DLEN is expressed in bytes.
 * \return a DataResult object
 */
static DataResult IRQsendDataBlock(const unsigned int *buffer, unsigned int size)
{
    // A note on speed.
    // Due to the auto calibration of SDIO clock speed being done with
    // IRQreceiveDataBlock(), the speed of this function must be comparable
    // with the speed of IRQreceiveDataBlock(), otherwise this function
    // will fail because of data underrun.
    const unsigned int *bufend=buffer+size;
    unsigned int status;
    for(;;)
    {
        status=SDIO->STA;
        if(status & (SDIO_STA_TXUNDERR | SDIO_STA_DCRCFAIL |
            SDIO_STA_DTIMEOUT | SDIO_STA_STBITERR | SDIO_STA_DBCKEND)) break;
        if((status & SDIO_STA_TXFIFOHE) && (buffer!=bufend))
        {
            //Write 8 words to the fifo, loop entirely unrolled for speed
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
            SDIO->FIFO=*buffer; buffer++;
        }
    }
    SDIO->ICR=0x7ff;//Clear flags
    if(status & SDIO_STA_TXUNDERR) return DataResult(DataResult::TXUnderrun);
    if(status & SDIO_STA_DCRCFAIL) return DataResult(DataResult::CRCFail);
    if(status & SDIO_STA_DTIMEOUT) return DataResult(DataResult::Timeout);
    if(status & SDIO_STA_STBITERR) return DataResult(DataResult::StartBitFail);
    return DataResult(DataResult::Ok);
}

/**
 * \internal
 * Read a single block of 512 bytes from an SD/MMC card.
 * Card must be selected prior to caling this function.
 * \param buffer, a buffer whose size is >=512 bytes, word aligned
 * \param lba logical block address of the block to read.
 */
static bool singleBlockRead(unsigned int *buffer, unsigned int lba)
{
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC

    if(waitForCardReady()==false) return false;

    CmdResult cr;
    DataResult dr;
    bool failed=true;
    for(;;)
    {
        // Since we read with polling, a context switch or interrupt here
        // would cause a fifo overrun, so we disable interrupts.
        FastInterruptDisableLock dLock;

        SDIO->DLEN=512;
        //Block size 512 bytes, block data xfer, from card to controller
        SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;

        cr=Command::IRQsend(Command::CMD17,lba);
        if(cr.IRQvalidateR1Response())
        {
            dr=IRQreceiveDataBlock(buffer,512/sizeof(unsigned int));
            SDIO->DCTRL=0; //Disable data path state machine
            
            //If failed because too slow check if it is possible to reduce speed
            if(dr.getError()==DataResult::RXOverrun)
            {
                if(ClockController::IRQreduceClockSpeed())
                {
                    //Disabling interrupts for too long is bad
                    FastInterruptEnableLock eLock(dLock);
                    //After an error during data xfer the card might be a little
                    //confused. So send STOP_TRANSMISSION command to reassure it
                    cr=Command::send(Command::CMD12,0);
                    if(cr.validateR1Response()) continue;
                }
            }

            if(dr.getError()==DataResult::Ok) failed=false;
        }
        break;
    }
    if(failed)
    {
        cr.validateR1Response();
        dr.validateError();
        //After an error during data xfer the card might be a little
        //confused. So send STOP_TRANSMISSION command to reassure it
        cr=Command::send(Command::CMD12,0);
        cr.validateR1Response();
        return false;
    }
    return true;
}

/**
 * \internal
 * Write a single block of 512 bytes to an SD/MMC card
 * Card must be selected prior to caling this function.
 * \param buffer, a buffer whose size is >=512 bytes
 * \param lba logical block address of the block to write.
 */
static bool singleBlockWrite(const unsigned int *buffer, unsigned int lba)
{
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC

    if(waitForCardReady()==false) return false;

    bool failed=true;
    CmdResult cr;
    DataResult dr;
    for(;;)
    {
        // Since we write with polling, a context switch or interrupt here
        // would cause a fifo overrun, so we disable interrupts.
        FastInterruptDisableLock dLock;

        cr=Command::IRQsend(Command::CMD24,lba);
        if(cr.IRQvalidateR1Response())
        {
            SDIO->DLEN=512;
            //Block size 512 bytes, block data xfer, from controller to card
            SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DTEN;

            dr=IRQsendDataBlock(buffer,512/sizeof(unsigned int));
            SDIO->DCTRL=0; //Disable data path state machine

            //If failed because too slow check if it is possible to reduce speed
            if(dr.getError()==DataResult::TXUnderrun)
            {
                if(ClockController::IRQreduceClockSpeed())
                {
                    //Disabling interrupts for too long is bad
                    FastInterruptEnableLock eLock(dLock);
                    //After an error during data xfer the card might be a little
                    //confused. So send STOP_TRANSMISSION command to reassure it
                    cr=Command::send(Command::CMD12,0);
                    if(cr.validateR1Response()) continue;
                }
            }

            if(dr.getError()==DataResult::Ok) failed=false;
        }
        break;
    }
    if(failed)
    {
        cr.validateR1Response();
        dr.validateError();
        //After an error during data xfer the card might be a little
        //confused. So send STOP_TRANSMISSION command to reassure it
        cr=Command::send(Command::CMD12,0);
        cr.validateR1Response();
        return false;
    }
    return true;
}

#else //__ENABLE_XRAM

/**
 * \internal
 * Prints the errors that may occur during a DMA transfer
 */
static void displayBlockTransferError()
{
    DBGERR("Block transfer error\n");
    if(dmaFlags & DMA_ISR_TEIF4)      DBGERR("* DMA Transfer error\n");
    if(sdioFlags & SDIO_STA_STBITERR) DBGERR("* SDIO Start bit error\n");
    if(sdioFlags & SDIO_STA_RXOVERR)  DBGERR("* SDIO RX Overrun\n");
    if(sdioFlags & SDIO_STA_TXUNDERR) DBGERR("* SDIO TX Underrun error\n");
    if(sdioFlags & SDIO_STA_DCRCFAIL) DBGERR("* SDIO Data CRC fail\n");
    if(sdioFlags & SDIO_STA_DTIMEOUT) DBGERR("* SDIO Data timeout\n");
}

/**
 * \internal
 * Read a given number of contiguous 512 byte blocks from an SD/MMC card.
 * Card must be selected prior to calling this function.
 * \param buffer, a buffer whose size is 512*nblk bytes, word aligned
 * \param nblk number of blocks to read.
 * \param lba logical block address of the first block to read.
 */
static bool multipleBlockRead(unsigned int *buffer, unsigned int nblk,
    unsigned int lba)
{
    if(nblk==0) return true;
    while(nblk>511)
    {
        if(multipleBlockRead(buffer,511,lba)==false) return false;
        buffer+=511*512;
        nblk-=511;
        lba+=511;
    }
    if(waitForCardReady()==false) return false;
    
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC
    
    //Clear both SDIO and DMA interrupt flags
    SDIO->ICR=0x7ff;
    DMA2->IFCR=DMA_IFCR_CGIF4;
    
    transferError=false;
    dmaFlags=sdioFlags=0;
    waiting=Thread::getCurrentThread();
    
    //Data transfer is considered complete once the DMA transfer complete
    //interrupt occurs, that happens when the last data was written in the
    //buffer. Both SDIO and DMA error interrupts are active to catch errors
    SDIO->MASK=SDIO_MASK_STBITERRIE | //Interrupt on start bit error
               SDIO_MASK_RXOVERRIE  | //Interrupt on rx underrun
               SDIO_MASK_TXUNDERRIE | //Interrupt on tx underrun
               SDIO_MASK_DCRCFAILIE | //Interrupt on data CRC fail
               SDIO_MASK_DTIMEOUTIE;  //Interrupt on data timeout
    DMA2_Channel4->CPAR=reinterpret_cast<unsigned int>(&SDIO->FIFO);
    DMA2_Channel4->CMAR=reinterpret_cast<unsigned int>(buffer);
	DMA2_Channel4->CNDTR=nblk*512/sizeof(unsigned int);
    DMA2_Channel4->CCR=DMA_CCR4_PL_1      | //High priority DMA stream
                       DMA_CCR4_MSIZE_1   | //Write 32bit at a time to RAM
					   DMA_CCR4_PSIZE_1   | //Read 32bit at a time from SDIO
				       DMA_CCR4_MINC      | //Increment RAM pointer
			           0                  | //Peripheral to memory direction
			           DMA_CCR4_TCIE      | //Interrupt on transfer complete
                       DMA_CCR4_TEIE      | //Interrupt on transfer error
			  	       DMA_CCR4_EN;         //Start the DMA
    
    SDIO->DLEN=nblk*512;
    if(waiting==0)
    {
        DBGERR("Premature wakeup\n");
        transferError=true;
    }
    CmdResult cr=Command::send(nblk>1 ? Command::CMD18 : Command::CMD17,lba);
    if(cr.validateR1Response())
    {
        //Block size 512 bytes, block data xfer, from card to controller
        SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    } else transferError=true;
    DMA2_Channel4->CCR=0;
    while(DMA2_Channel4->CCR & DMA_CCR4_EN) ; //DMA may take time to stop
    SDIO->DCTRL=0; //Disable data path state machine
    SDIO->MASK=0;

    // CMD12 is sent to end CMD18 (multiple block read), or to abort an
    // unfinished read in case of errors
    if(nblk>1 || transferError) cr=Command::send(Command::CMD12,0);
    if(transferError || cr.validateR1Response()==false)
    {
        displayBlockTransferError();
        ClockController::reduceClockSpeed();
        return false;
    }
    return true;
}

/**
 * \internal
 * Write a given number of contiguous 512 byte blocks to an SD/MMC card.
 * Card must be selected prior to calling this function.
 * \param buffer, a buffer whose size is 512*nblk bytes, word aligned
 * \param nblk number of blocks to write.
 * \param lba logical block address of the first block to write.
 */
static bool multipleBlockWrite(const unsigned int *buffer, unsigned int nblk,
    unsigned int lba)
{
    if(nblk==0) return true;
    while(nblk>511)
    {
        if(multipleBlockWrite(buffer,511,lba)==false) return false;
        buffer+=511*512;
        nblk-=511;
        lba+=511;
    }
    if(waitForCardReady()==false) return false;
    
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC
    if(nblk>1)
    {
        CmdResult cr=Command::send(Command::ACMD23,nblk);
        if(cr.validateR1Response()==false) return false;
    }
    
    //Clear both SDIO and DMA interrupt flags
    SDIO->ICR=0x7ff;
    DMA2->IFCR=DMA_IFCR_CGIF4;
    
    transferError=false;
    dmaFlags=sdioFlags=0;
    waiting=Thread::getCurrentThread();
    
    //Data transfer is considered complete once the SDIO transfer complete
    //interrupt occurs, that happens when the last data was written to the SDIO
    //Both SDIO and DMA error interrupts are active to catch errors
    SDIO->MASK=SDIO_MASK_DATAENDIE  | //Interrupt on data end
               SDIO_MASK_STBITERRIE | //Interrupt on start bit error
               SDIO_MASK_RXOVERRIE  | //Interrupt on rx underrun
               SDIO_MASK_TXUNDERRIE | //Interrupt on tx underrun
               SDIO_MASK_DCRCFAILIE | //Interrupt on data CRC fail
               SDIO_MASK_DTIMEOUTIE;  //Interrupt on data timeout
	DMA2_Channel4->CPAR=reinterpret_cast<unsigned int>(&SDIO->FIFO);
	DMA2_Channel4->CMAR=reinterpret_cast<unsigned int>(buffer);
	DMA2_Channel4->CNDTR=nblk*512/sizeof(unsigned int);
	DMA2_Channel4->CCR=DMA_CCR4_PL_1      | //High priority DMA stream
                       DMA_CCR4_MSIZE_1   | //Read 32bit at a time from RAM
					   DMA_CCR4_PSIZE_1   | //Write 32bit at a time to SDIO
				       DMA_CCR4_MINC      | //Increment RAM pointer
			           DMA_CCR4_DIR       | //Memory to peripheral direction
                       DMA_CCR4_TEIE      | //Interrupt on transfer error
                       DMA_CCR4_EN;         //Start the DMA
    
    SDIO->DLEN=nblk*512;
    if(waiting==0)
    {
        DBGERR("Premature wakeup\n");
        transferError=true;
    }
    CmdResult cr=Command::send(nblk>1 ? Command::CMD25 : Command::CMD24,lba);
    if(cr.validateR1Response())
    {
        //Block size 512 bytes, block data xfer, from card to controller
        SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    } else transferError=true;
    DMA2_Channel4->CCR=0;
    while(DMA2_Channel4->CCR & DMA_CCR4_EN) ; //DMA may take time to stop
    SDIO->DCTRL=0; //Disable data path state machine
    SDIO->MASK=0;

    // CMD12 is sent to end CMD25 (multiple block write), or to abort an
    // unfinished write in case of errors
    if(nblk>1 || transferError) cr=Command::send(Command::CMD12,0);
    if(transferError || cr.validateR1Response()==false)
    {
        displayBlockTransferError();
        ClockController::reduceClockSpeed();
        return false;
    }
    return true;
}
#endif //__ENABLE_XRAM

//
// Class CardSelector
//

/**
 * \internal
 * Simple RAII class for selecting an SD/MMC card an automatically deselect it
 * at the end of the scope.
 */
class CardSelector
{
public:
    /**
     * \internal
     * Constructor. Selects the card.
     * The result of the select operation is available through its succeded()
     * member function
     */
    explicit CardSelector()
    {
        success=Command::send(
                Command::CMD7,Command::getRca()<<16).validateR1Response();
    }

    /**
     * \internal
     * \return true if the card was selected, false on error
     */
    bool succeded() { return success; }

    /**
     * \internal
     * Destructor, ensures that the card is deselected
     */
    ~CardSelector()
    {
        Command::send(Command::CMD7,0); //Deselect card. This will timeout
    }

private:
    bool success;
};

//
// Initialization helper functions
//

/**
 * \internal
 * Initialzes the SDIO peripheral in the STM32
 */
static void initSDIOPeripheral()
{
    {
        //Doing read-modify-write on RCC->APBENR2 and gpios, better be safe
        FastInterruptDisableLock lock;
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN;
        RCC_SYNC();
        #ifdef __ENABLE_XRAM
        RCC->AHBENR |= RCC_AHBENR_SDIOEN;
        #else //__ENABLE_XRAM
        RCC->AHBENR |= RCC_AHBENR_SDIOEN | RCC_AHBENR_DMA2EN;
        #endif //__ENABLE_XRAM
        RCC_SYNC();
        sdD0::mode(Mode::ALTERNATE);
        sdD1::mode(Mode::ALTERNATE);
        sdD2::mode(Mode::ALTERNATE);
        sdD3::mode(Mode::ALTERNATE);
        sdCLK::mode(Mode::ALTERNATE);
        sdCMD::mode(Mode::ALTERNATE);
    }
    #ifndef __ENABLE_XRAM
    NVIC_SetPriority(DMA2_Channel4_5_IRQn,15);//Low priority for DMA
    NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);
    NVIC_SetPriority(SDIO_IRQn,15);//Low priority for SDIO
    NVIC_EnableIRQ(SDIO_IRQn);
    #endif //__ENABLE_XRAM

    SDIO->POWER=0; //Power off state
    delayUs(1);
    SDIO->CLKCR=0;
    SDIO->CMD=0;
    SDIO->DCTRL=0;
    SDIO->ICR=0xc007ff;
    SDIO->POWER=SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0; //Power on state
    //This delay is particularly important: when setting the POWER register a
    //glitch on the CMD pin happens. This glitch has a fast fall time and a slow
    //rise time resembling an RC charge with a ~6us rise time. If the clock is
    //started too soon, the card sees a clock pulse while CMD is low, and
    //interprets it as a start bit. No, setting POWER to powerup does not
    //eliminate the glitch.
    delayUs(10);
    ClockController::setLowSpeedClock();
}

/**
 * \internal
 * Detect if the card is an SDHC, SDv2, SDv1, MMC
 * \return Type of card: (1<<0)=MMC (1<<1)=SDv1 (1<<2)=SDv2 (1<<2)|(1<<3)=SDHC
 * or Invalid if card detect failed.
 */
static CardType detectCardType()
{
    const int INIT_TIMEOUT=200; //200*10ms= 2 seconds
    CmdResult r=Command::send(Command::CMD8,0x1aa);
    if(r.validateError())
    {
        //We have an SDv2 card connected
        if(r.getResponse()!=0x1aa)
        {
            DBGERR("CMD8 validation: voltage range fail\n");
            return Invalid;
        }
        for(int i=0;i<INIT_TIMEOUT;i++)
        {
            //Bit 30 @ 1 = tell the card we like SDHCs
            r=Command::send(Command::ACMD41,(1<<30) | sdVoltageMask);
            //ACMD41 sends R3 as response, whose CRC is wrong.
            if(r.getError()!=CmdResult::Ok && r.getError()!=CmdResult::CRCFail)
            {
                r.validateError();
                return Invalid;
            }
            if((r.getResponse() & (1<<31))==0) //Busy bit
            {
                Thread::sleep(10);
                continue;
            }
            if((r.getResponse() & sdVoltageMask)==0)
            {
                DBGERR("ACMD41 validation: voltage range fail\n");
                return Invalid;
            }
            DBG("ACMD41 validation: looped %d times\n",i);
            if(r.getResponse() & (1<<30))
            {
                DBG("SDHC\n");
                return SDHC;
            } else {
                DBG("SDv2\n");
                return SDv2;
            }
        }
        DBGERR("ACMD41 validation: looped until timeout\n");
        return Invalid;
    } else {
        //We have an SDv1 or MMC
        r=Command::send(Command::ACMD41,sdVoltageMask);
        //ACMD41 sends R3 as response, whose CRC is wrong.
        if(r.getError()!=CmdResult::Ok && r.getError()!=CmdResult::CRCFail)
        {
            //MMC card
            DBG("MMC card\n");
            return MMC;
        } else {
            //SDv1 card
            for(int i=0;i<INIT_TIMEOUT;i++)
            {
                //ACMD41 sends R3 as response, whose CRC is wrong.
                if(r.getError()!=CmdResult::Ok &&
                        r.getError()!=CmdResult::CRCFail)
                {
                    r.validateError();
                    return Invalid;
                }
                if((r.getResponse() & (1<<31))==0) //Busy bit
                {
                    Thread::sleep(10);
                    //Send again command
                    r=Command::send(Command::ACMD41,sdVoltageMask);
                    continue;
                }
                if((r.getResponse() & sdVoltageMask)==0)
                {
                    DBGERR("ACMD41 validation: voltage range fail\n");
                    return Invalid;
                }
                DBG("ACMD41 validation: looped %d times\nSDv1\n",i);
                return SDv1;
            }
            DBGERR("ACMD41 validation: looped until timeout\n");
            return Invalid;
        }
    }
}

//
// class SDIODriver
//

intrusive_ref_ptr<SDIODriver> SDIODriver::instance()
{
    static FastMutex m;
    static intrusive_ref_ptr<SDIODriver> instance;
    Lock<FastMutex> l(m);
    if(!instance) instance=new SDIODriver();
    return instance;
}

ssize_t SDIODriver::readBlock(void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    Lock<FastMutex> l(mutex);
    DBG("SDIODriver::readBlock(): nSectors=%d\n",nSectors);
    bool aligned=BufferConverter::isWordAligned(buffer);
    if(aligned==false) DBG("Buffer misaligned\n");

    for(int i=0;i<ClockController::getRetryCount();i++)
    {
        //Select card
        CardSelector selector;
        if(selector.succeded()==false) continue;
        bool error=false;
        
        #ifdef __ENABLE_XRAM
        // In the XRAM fallback code multiple sector read is implemented as
        // a sequence of single block read operations
        unsigned char *tempBuffer=reinterpret_cast<unsigned char*>(buffer);
        unsigned int tempLba=lba;
        for(unsigned int j=0;j<nSectors;j++)
        {
            unsigned int* b=BufferConverter::toWordAlignedWithoutCopy(tempBuffer);
            if(singleBlockRead(b,tempLba)==false)
            {
                error=true;
                break;
            }
            BufferConverter::toOriginalBuffer();
            tempBuffer+=512;
            tempLba++;
        }
        #else //__ENABLE_XRAM
        // If XRAM is not enabled, then check pointer alignment, and if it is
        // aligned use a single multipleBlockRead(), else use the buffer
        // converter and read a sector at a time
        if(aligned)
        {
            if(multipleBlockRead(reinterpret_cast<unsigned int*>(buffer),
                nSectors,lba)==false) error=true;
        } else {
            unsigned char *tempBuffer=reinterpret_cast<unsigned char*>(buffer);
            unsigned int tempLba=lba;
            for(unsigned int j=0;j<nSectors;j++)
            {
                unsigned int* b=BufferConverter::toWordAlignedWithoutCopy(tempBuffer);
                if(multipleBlockRead(b,1,tempLba)==false)
                {
                    error=true;
                    break;
                }
                BufferConverter::toOriginalBuffer();
                tempBuffer+=512;
                tempLba++;
            }
        }
        #endif //__ENABLE_XRAM

        if(error==false)
        {
            if(i>0) DBGERR("Read: required %d retries\n",i);
            return size;
        }
    }
    return -EBADF;
}

ssize_t SDIODriver::writeBlock(const void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    Lock<FastMutex> l(mutex);
    DBG("SDIODriver::writeBlock(): nSectors=%d\n",nSectors);
    bool aligned=BufferConverter::isWordAligned(buffer);
    if(aligned==false) DBG("Buffer misaligned\n");

    for(int i=0;i<ClockController::getRetryCount();i++)
    {
        //Select card
        CardSelector selector;
        if(selector.succeded()==false) continue;
        bool error=false;

        #ifdef __ENABLE_XRAM
        // In the XRAM fallback code multiple sector write is implemented as
        // a sequence of single block write operations
        const unsigned char *tempBuffer=
            reinterpret_cast<const unsigned char*>(buffer);
        unsigned int tempLba=lba;
        for(unsigned int j=0;j<nSectors;j++)
        {
            const unsigned int* b=BufferConverter::toWordAligned(tempBuffer);
            if(singleBlockWrite(b,tempLba)==false)
            {
                error=true;
                break;
            }
            tempBuffer+=512;
            tempLba++;
        }
        #else //__ENABLE_XRAM
        // If XRAM is not enabled, then check pointer alignment, and if it is
        // aligned use a single multipleBlockWrite(), else use the buffer
        // converter and write a sector at a time
        if(aligned)
        {
            if(multipleBlockWrite(reinterpret_cast<const unsigned int*>(buffer),
                nSectors,lba)==false) error=true;
        } else {
            const unsigned char *tempBuffer=
                reinterpret_cast<const unsigned char*>(buffer);
            unsigned int tempLba=lba;
            for(unsigned int j=0;j<nSectors;j++)
            {
                const unsigned int* b=BufferConverter::toWordAligned(tempBuffer);
                if(multipleBlockWrite(b,1,tempLba)==false)
                {
                    error=true;
                    break;
                }
                tempBuffer+=512;
                tempLba++;
            }
        }
        #endif //__ENABLE_XRAM
        
        if(error==false)
        {
            if(i>0) DBGERR("Write: required %d retries\n",i);
            return size;
        }
    }
    return -EBADF;
}

int SDIODriver::ioctl(int cmd, void* arg)
{
    DBG("SDIODriver::ioctl()\n");
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<FastMutex> l(mutex);
    //Note: no need to select card, since status can be queried even with card
    //not selected.
    return waitForCardReady() ? 0 : -EFAULT;
}

SDIODriver::SDIODriver() : Device(Device::BLOCK)
{
    initSDIOPeripheral();

    // This is more important than it seems, since CMD55 requires the card's RCA
    // as argument. During initalization, after CMD0 the card has an RCA of zero
    // so without this line ACMD41 will fail and the card won't be initialized.
    Command::setRca(0);

    //Send card reset command
    CmdResult r=Command::send(Command::CMD0,0);
    if(r.validateError()==false) return;

    cardType=detectCardType();
    if(cardType==Invalid) return; //Card detect failed
    if(cardType==MMC) return; //MMC cards currently unsupported

    // Now give an RCA to the card. In theory we should loop and enumerate all
    // the cards but this driver supports only one card.
    r=Command::send(Command::CMD2,0);
    //CMD2 sends R2 response, whose CMDINDEX field is wrong
    if(r.getError()!=CmdResult::Ok && r.getError()!=CmdResult::RespNotMatch)
    {
        r.validateError();
        return;
    }
    r=Command::send(Command::CMD3,0);
    if(r.validateR6Response()==false) return;
    Command::setRca(r.getResponse()>>16);
    DBG("Got RCA=%u\n",Command::getRca());
    if(Command::getRca()==0)
    {
        //RCA=0 can't be accepted, since it is used to deselect cards
        DBGERR("RCA=0 is invalid\n");
        return;
    }

    //Lastly, try selecting the card and configure the latest bits
    {
        CardSelector selector;
        if(selector.succeded()==false) return;

        r=Command::send(Command::CMD13,Command::getRca()<<16);//Get status
        if(r.validateR1Response()==false) return;
        if(r.getState()!=4) //4=Tran state
        {
            DBGERR("CMD7 was not able to select card\n");
            return;
        }

        r=Command::send(Command::ACMD6,2);   //Set 4 bit bus width
        if(r.validateR1Response()==false) return;

        if(cardType!=SDHC)
        {
            r=Command::send(Command::CMD16,512); //Set 512Byte block length
            if(r.validateR1Response()==false) return;
        }
    }

    // Now that card is initialized, perform self calibration of maximum
    // possible read/write speed. This as a side effect enables 4bit bus width.
    ClockController::calibrateClockSpeed(this);

    DBG("SDIO init: Success\n");
}

} //namespace miosix
