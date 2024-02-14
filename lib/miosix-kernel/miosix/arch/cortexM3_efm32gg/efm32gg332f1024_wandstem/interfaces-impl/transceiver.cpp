/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015, 2016 by Terraneo Federico and   *
 *      Luigi Rinaldi                                                      *
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

#include "transceiver.h"
#include "cc2520_constants.h"
#include "gpioirq.h"
#include "config/miosix_settings.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <kernel/scheduler/scheduler.h>

using namespace std;

namespace miosix {

/**
 * NOTE: after implementing the driver for this transceiver, it was clear that
 * the hardware is designed assuming a reactive or event-driven paradigm,
 * where the handling of events or "exceptions", as called in the datasheet,
 * is separated from the individual actions, such as transmit or receive.
 * This driver, which is meant for deterministic time-slotted protocols, offers
 * instead an imperative interface with individual transmit and receive
 * operations depending on the task to be performed in a given time slot.
 * Due to the mismatch between what the hardware provides and what the software
 * requires, the driver is a bit complicated, and feels like fitting a square
 * peg in a round hole.
 */

using complExcChB=transceiver::gpio2; ///< Function of this pin is missing in bsp

//
// Time constants computation
//

/// Wait time for the RSSI to become valid
const auto rssiWait=80000;

/// Slack added to all constants ending in "timeout"
const auto slack=128000;

/// Transceiver activation time (time from idle to TX or RX, and from RX to TX)
/// Measured STXON to SFD_TX delay (turnaround+preambleSfdTime) is 352.370us
const auto turnaround=192370;

/// Measured SFD_TX to SFD_RX delay 
const auto rxSfdLag=3374;

/// Time to send one byte as float
const auto timePerByte=32000;

/// Time to send the first part of each packet (4 bytes preamble + 1 byte SFD)
const auto preambleSfdTime=timePerByte*5;

/// Timeout from sending STXON to when the SFD should be sent, around 500us
const auto sfdTimeout=slack+turnaround+preambleSfdTime;

/// Timeout from an SFD to when the maximum length packet should end
const auto maxPacketTimeout=slack+timePerByte*128;

/**
 * Used by the SPI code to respect tcscks, csckh, tcsnh
 * defined at page 34 of the CC2520 datasheet
 */
static inline void spiDelay()
{
    asm volatile("           nop    \n"
                 "           nop    \n"
                 "           nop    \n");
}

/**
 * RAII class to keep the transceiver SPI chip select low in a scope
 */
class TransceiverChipSelect
{
public:
    TransceiverChipSelect()
    {
        transceiver::cs::low();
        spiDelay();
    }
    
    ~TransceiverChipSelect()
    {
        spiDelay();
        transceiver::cs::high();
        spiDelay();
    }
};

//
// class TransceiverConfiguration
//

void TransceiverConfiguration::setChannel(int channel)
{
    if(channel<11 || channel>26) throw range_error("Channel not in range");
    frequency=2405+5*(channel-11);
}

//
// class Transceiver
//

Transceiver& Transceiver::instance()
{
    static Transceiver singleton;
    return singleton;
}

void Transceiver::turnOn()
{
    if(state!=CC2520State::DEEPSLEEP) return;

    //Enable the transceiver power domain
    pm.enableTransceiverPowerDomain();
    transceiver::reset::high();
    
    {
        TransceiverChipSelect cs;
        //wait until SO=1 (clock stable and running)
        waitXosc();
    }
    
    configure();
}

void Transceiver::configure()
{
    //
    // Configure transceiver as per given configuration class
    //
    
    writeReg(CC2520Register::FREQCTRL,config.frequency-2394);
    writeReg(CC2520Register::FRMCTRL0,0x40*config.crc); //automatically add FCS
    writeReg(CC2520Register::TXPOWER,txPower(config.txPower));
    
    //
    // GPIO andexception configuration
    //

    //Setting SFD, TX_FRM_DONE, RX_FRM_DONE, RX_OVERFLOW on exception channel B
    writeReg(CC2520Register::EXCMASKB0,   CC2520Exception::TX_FRM_DONE
                                        | CC2520Exception::RX_OVERFLOW);
    writeReg(CC2520Register::EXCMASKB1,(  CC2520Exception::RX_FRM_DONE
                                        | CC2520Exception::SFD)>>8);
    writeReg(CC2520Register::EXCMASKB2,0x0);
    
    //Setting gpio0 as no function input to save power (breaks bed of nail test)
    writeReg(CC2520Register::GPIOCTRL0,0x80 | 0x10);
    //Setting gpio2 as output complementary exception channel B
    writeReg(CC2520Register::GPIOCTRL2,0x24);
    //Setting gpio3 as output exception channel B
    writeReg(CC2520Register::GPIOCTRL3,0x22);
    //Setting gpio5 as input on rising edge send command strobe STXON
    writeReg(CC2520Register::GPIOCTRL5,0x80 | 0x08);
    
    //
    // MISC configuration
    //
    
    //With the crystal and capacitor selection in the node, this is required
    //to trim the crystal frequency as close as possible to 32MHz
    writeReg(CC2520Register::FREQTUNE,14);
    
    writeReg(CC2520Register::FRMFILT0,0x00); //disable frame filtering
    writeReg(CC2520Register::FRMFILT1,0x00); //disable frame filtering
    writeReg(CC2520Register::SRCMATCH,0x00); //disable source matching

    //
    // Register that need to update from their default value (as in datasheet)
    //
    
    writeReg(CC2520Register::CCACTRL0,0xF8);
    writeReg(CC2520Register::MDMCTRL0,0x85); //controls modem
    writeReg(CC2520Register::MDMCTRL1,0x14); //controls modem
    writeReg(CC2520Register::RXCTRL,0x3f);
    writeReg(CC2520Register::FSCTRL,0x5a);
    writeReg(CC2520Register::FSCAL1,0x2b);
    writeReg(CC2520Register::AGCCTRL1,0x11);
    writeReg(CC2520Register::ADCTEST0,0x10);
    writeReg(CC2520Register::ADCTEST1,0x0E);
    writeReg(CC2520Register::ADCTEST2,0x03);

    state=CC2520State::IDLE;
    idle();
}

void Transceiver::turnOff()
{
    if(state==CC2520State::DEEPSLEEP) return;
    state=CC2520State::DEEPSLEEP;
    
    transceiver::reset::low();
    pm.disableTransceiverPowerDomain();
}

bool Transceiver::isTurnedOn() const { return state!=CC2520State::DEEPSLEEP; }

bool Transceiver::IRQisTurnedOn() const { return state!=CC2520State::DEEPSLEEP; }

void Transceiver::idle()
{
    if(state==CC2520State::DEEPSLEEP) return;
    
    //In case of timeout, abort current transmission
    commandStrobe(CC2520Command::SRFOFF);
    //See diagram on page 69 of datasheet
    commandStrobe(CC2520Command::SFLUSHTX);
    //See errata sheet about SRFOFF possibly leaving unwanted byte in RX fifo
    commandStrobe(CC2520Command::SFLUSHRX);
    clearAllExceptions();
    state=CC2520State::IDLE;
}

void Transceiver::configure(const TransceiverConfiguration& config)
{
    if(config.frequency<minFrequency || config.frequency>maxFrequency)
        throw range_error("config.frequency");
    
    this->config=config;

    if(state==CC2520State::DEEPSLEEP) return;
    if(state!=CC2520State::IDLE) idle();
    
    writeReg(CC2520Register::FREQCTRL,config.frequency-2394);
    writeReg(CC2520Register::FRMCTRL0,0x40*config.crc);
    writeReg(CC2520Register::TXPOWER,txPower(config.txPower));
}

void Transceiver::sendNow(const void* pkt, int size)
{
    if(state==CC2520State::DEEPSLEEP)
        throw runtime_error("Transceiver::sendNow while in deep sleep");
    
    //Can't leave RX on, would cause an RX_FRM_ABORTED if a packet being
    //received is interrupted, we should check that it does not interfere with
    //the RX flow diagram. Moreover STXON does not re-enable the receiver on
    //exit, so we would not be in RX anyway
    if(state==CC2520State::RX) idle();
    
    writePacketToTxBuffer(pkt,size);
    commandStrobe(CC2520Command::STXON);
    handlePacketTransmissionEvents(size);
}

bool Transceiver::sendCca(const void* pkt, int size)
{
    if(state==CC2520State::DEEPSLEEP)
        throw runtime_error("Transceiver::sendCca while in deep sleep");
    
    startRxAndWaitForRssi();
    writePacketToTxBuffer(pkt,size);
    commandStrobe(CC2520Command::STXONCCA);
    if((readReg(CC2520Register::FSMSTAT1) & (1<<3))==0)
    {
        //See diagram on page 69 of datasheet
        commandStrobe(CC2520Command::SFLUSHTX);
        return false;
    }
    handlePacketTransmissionEvents(size);
    return true;
}

void Transceiver::sendAt(const void* pkt, int size, long long when, Unit unit)
{
    if(state==CC2520State::DEEPSLEEP)
        throw runtime_error("Transceiver::sendAt while in deep sleep");
    
    //Can't leave RX on, would cause an ambiguity with the SFD exeption
    if(state==CC2520State::RX) idle();
    
    writePacketToTxBuffer(pkt,size);
    
    long long w;
    if(unit==Unit::NS){
        w=timer.ns2tick(when-turnaround);
    }else{
        w=when-timer.ns2tick(turnaround);
    }
    //NOTE: when is the time where the first byte of the packet should be sent,
    //while the cc2520 requires the turnaround from STXON to sending
    if(timer.absoluteWaitTrigger(w)==true)
    {
	//See diagram on page 69 of datasheet
        commandStrobe(CC2520Command::SFLUSHTX);
        throw runtime_error("Transceiver::sendAt too late to send");
    }
    handlePacketTransmissionEvents(size);
}

RecvResult Transceiver::recv(void *pkt, int size, long long timeout, Unit unit, Correct c)
{
    if(state==CC2520State::DEEPSLEEP)
        throw runtime_error("Transceiver::recv while in deep sleep");
    
    RecvResult result;
    if(state!=CC2520State::RX)
    {
        commandStrobe(CC2520Command::SRXON);
        state=CC2520State::RX;
    } else {
        //If we are already receiving, we must first check if there is already
        //at least a packet in the fifo, as it can contain multiple full packets
        int inFifo=readReg(CC2520Register::RXFIFOCNT);
        if(inFifo>0)
        {
            //Ok, we have bytes in the fifo, does that mean that there is a
            //full packet? No, we may have an incomplete packet we are receiving
            //just now, so check that there is a full packet
            int lengthByte=readReg(CC2520Register::RXFIRST);
            //Length byte may be wrong, so check it
            if(lengthByte>127 || lengthByte<(config.crc ? 3 : 1))
            {
                idle();
                throw range_error("Transceiver::recv length error");
            }
            //lengthByte+1 is the number of bytes that should be in the fifo
            //for the full packet to be there (+1 because the length byte counts
            //the bytes excluding the length byte itself)
            if(inFifo>=lengthByte+1)
            {
                //Timestamp is wrong and we know it, so we don't set valid
                
                if(unit==Unit::NS){
                    result.timestamp=timer.tick2ns(timer.getExtEventTimestamp((HardwareTimer::Correct) c))-
                        (preambleSfdTime+rxSfdLag);
                }else{
                    result.timestamp=timer.getExtEventTimestamp((HardwareTimer::Correct) c)-
                        timer.ns2tick(preambleSfdTime+rxSfdLag);
                }
                
                //We may still be in the middle of another packet reception, so
                //this may cause FRM_DONE to occur without a previous SFD,
                //handlePacketReceptionEvents() has to account for that
                clearException(  CC2520Exception::SFD
                            | CC2520Exception::RX_FRM_DONE);
                readPacketFromRxBuffer(pkt,size,result);
                return result;
            }
        }
    }
    
    if(handlePacketReceptionEvents(timeout,size,result,unit,c)==false)
         readPacketFromRxBuffer(pkt,size,result);
    return result;
}

short Transceiver::readRssi()
{
    startRxAndWaitForRssi();
    return decodeRssi(readReg(CC2520Register::RSSI));
}

void Transceiver::writeReg(CC2520Register reg, unsigned char data)
{
    unsigned char r=toByte(reg);
    assert(r<=0x7e);
    
    TransceiverChipSelect cs;
    if(r<0x40)
    {
        spi.sendRecv(toByte(CC2520Command::REGISTER_WRITE) | r);
    } else {
        spi.sendRecv(toByte(CC2520Command::MEMORY_WRITE));
        spi.sendRecv(r);
    }
    spi.sendRecv(data);
}
    
unsigned char Transceiver::readReg(CC2520Register reg)
{
    unsigned char r=toByte(reg);
    assert(r<=0x7e);
    
    TransceiverChipSelect cs;
    if(r<0x40)
    {
        spi.sendRecv(toByte(CC2520Command::REGISTER_READ) | r);
    } else {
        spi.sendRecv(toByte(CC2520Command::MEMORY_READ));
        spi.sendRecv(r);
    }
    return spi.sendRecv();
}

CC2520StatusBitmask Transceiver::commandStrobe(CC2520Command cmd)
{
    TransceiverChipSelect cs;
    return spi.sendRecv(toByte(cmd));
}

Transceiver::Transceiver()
    : pm(PowerManager::instance()), spi(Spi::instance()),
      timer(getTransceiverTimer()), state(CC2520State::DEEPSLEEP),
      waiting(nullptr)
{
    registerGpioIrq(internalSpi::miso::getPin(),GpioIrqEdge::RISING,
    [this]{
        if(!waiting) return;
        waiting->IRQwakeup();
        if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
            Scheduler::IRQfindNextThread();
        waiting=nullptr;
    });
}

void Transceiver::startRxAndWaitForRssi()
{
    if(state!=CC2520State::RX)
    {
        commandStrobe(CC2520Command::SRXON);
        state=CC2520State::RX;
    }
    //We should do this even if we are already in RX, as we may be in RX for
    //such a short time (such as two sendCca() back to back, that the RSSI is
    //not yet valid
    
    //The datasheet says the maximum time for the RSSI to become valid is
    //the time for the receiver to start (192us) plus the time to receive
    //8 symbols (128us), the total is 320us, so we wait for a maximum of
    //8 * 80us, or 640us
    const int retryTimes=8;
    for(int i=0;i<retryTimes;i++)
    {
        CC2520StatusBitmask status=commandStrobe(CC2520Command::SNOP);
        if(status & CC2520Status::RSSI_VALID) break;
        if(i==retryTimes-1)
            throw runtime_error("Transceiver::startRxAndWaitForRssi timeout");
        
        timer.wait(timer.ns2tick(rssiWait));
    }
}

void Transceiver::writePacketToTxBuffer(const void* pkt, int size)
{
    //Input param validation
    if(size<1)
        throw range_error("Transceiver::writePacketToTxBuffer packet too short");
    if(size>(config.crc ? 125 : 127))
        throw range_error("Transceiver::writePacketToTxBuffer packet too long");
    
    //Send packet to the transceiver
    {
        TransceiverChipSelect cs;
        spi.sendRecv(toByte(CC2520Command::TXBUF));
        spi.sendRecv(size+(config.crc ? 2 : 0));
        const unsigned char *ptr=reinterpret_cast<const unsigned char *>(pkt);
        for(int i=0;i<size;i++) spi.sendRecv(*ptr++);
    }
    
    //Look for unexpected exceptions
    if(complExcChB::value())
    {
        unsigned int exc=getExceptions(0b001);
        if(exc & CC2520Exception::TX_OVERFLOW)
        {
            //See diagram on page 69 of datasheet
            commandStrobe(CC2520Command::SFLUSHTX);
            clearException(CC2520Exception::TX_OVERFLOW);
            throw runtime_error("Transceiver::writePacketToTxBuffer tx overflow");
        }
    }
}

void Transceiver::handlePacketTransmissionEvents(int size)
{
    bool oneRestart=false;
    bool silentError=false;
    restart:
    //Wait for the first event to occur (SFD)
    if(timer.waitTimeoutOrEvent(timer.ns2tick(sfdTimeout))==true)
    {
        //In case of timeout, abort current transmission
        idle();
        throw runtime_error("Transceiver::handlePacketTransmissionEvents timeout 1");
    }
    
    unsigned int exc=getExceptions(0b111);
    if(exc & CC2520Exception::SFD)
    {
        clearException(CC2520Exception::SFD);
    }
    //Both exception can occur simultaneously if a context switch occurs causing
    //an unexpected delay
    if(exc & CC2520Exception::TX_FRM_DONE)
    {
        //If both exceptions have occurred, then clear also the second one and
        //return without waiting any longer
        clearException(CC2520Exception::TX_FRM_DONE);
        if(silentError) idle();
        return;
    }
    //These are the odd ones caused by a race condition happening when we
    //transmit without first turning off the receiver, which for STXONCCA is
    //a necessity. If an unread packet is in the RX fifo, the RX_FRM_DONE
    //is set and this keeps exc ch B active, so we need to clear the RX
    //exception even though we are in TX. Same for RX_OVERFLOW, but we also
    //need to recover from the error, and do so AFTER we have sent the packet
    if(exc & CC2520Exception::RX_FRM_DONE)
    {
        if(oneRestart)
        {
            idle();
            throw runtime_error("Transceiver::handlePacketTransmissionEvents rx frm done");
        }
        oneRestart=true;
        clearException(CC2520Exception::RX_FRM_DONE);
        goto restart;
    }
    if(exc & CC2520Exception::RX_OVERFLOW)
    {
        if(oneRestart)
        {
            idle();
            throw runtime_error("Transceiver::handlePacketTransmissionEvents rx overflow");
        }
        oneRestart=true;
        clearException(CC2520Exception::RX_OVERFLOW);
        silentError=true;
        goto restart;
    }
    
    //Wait for the second event to occur (TX_FRM_DONE)
    bool timeout=timer.waitTimeoutOrEvent(timer.ns2tick(maxPacketTimeout));
    exc=getExceptions(0b001);
    if(timeout==true || (exc & CC2520Exception::TX_FRM_DONE)==0)
    {
        //In case of timeout, abort current transmission
        idle();
        //One cause (that should not happen anyway) of hitting the timeout
        //is the tx underflow exception
        if(exc & CC2520Exception::TX_UNDERFLOW)
            throw runtime_error("Transceiver::handlePacketTransmissionEvents tx underflow");
        if(timeout)
            throw runtime_error("Transceiver::handlePacketTransmissionEvents timeout 2");
        throw runtime_error("Transceiver::handlePacketTransmissionEvents no frm done");
    }
    clearException(CC2520Exception::TX_FRM_DONE);
    if(silentError) idle();
}

bool Transceiver::handlePacketReceptionEvents(long long timeout, int size, RecvResult& result, Unit unit, Correct c)
{
    if(unit==Unit::NS)
    {
        timeout=timer.ns2tick(timeout);
    }
    //Wait for the first event to occur (SFD), or timeout
    if(timer.absoluteWaitTimeoutOrEvent(timeout)==true)
    {
        result.error=RecvResult::TIMEOUT;
        return true;
    }
    //NOTE: the returned timestamp is the time where the first byte of the
    //packet is received, while the cc2520 allows timestamping at the SFD
    
    if(unit==Unit::NS){
        result.timestamp=timer.tick2ns(timer.getExtEventTimestamp((HardwareTimer::Correct) c))-
                        (preambleSfdTime+rxSfdLag);
    }else{
        result.timestamp=timer.getExtEventTimestamp((HardwareTimer::Correct) c)-
                        timer.ns2tick(preambleSfdTime+rxSfdLag);
    }
    
    unsigned int exc=getExceptions(0b011);
    if(exc & CC2520Exception::RX_OVERFLOW)
    {
        //In case of overflow, abort current reception
        idle();
        throw runtime_error("Transceiver::handlePacketReceptionEvents overflow");
    }
    if(exc & CC2520Exception::SFD)
    {
        //We actually have observed an SFD exception, so the timestamp is valid.
        //Even if a context switch may have caused a delay and multiple packets
        //have queued up, the SFD will be correct for the first packet, and
        //the remaining ones will (correctly) have the timestampValid flag
        //set to to false
        result.timestampValid=true;
        clearException(CC2520Exception::SFD);
    }
    //Both SFD and RX_FRM_DONE exceptions can occur simultaneously if a context
    //switch occurs causing an unexpected delay. An RX_FRM_DONE not preceded by
    //an SFD exception can also happen if multiple packets have queued up and
    //the exception clearing code has cleared the SFD exception while a packet
    //was being received, after the SFD but before the RX_FRM_DONE
    if(exc & CC2520Exception::RX_FRM_DONE)
    {
        //If both exceptions have occurred, then clear also the second one and
        //return without waiting any longer
        clearException(CC2520Exception::RX_FRM_DONE);
        return false;
    }
    
    //Wait for the second event to occur (RX_FRM_DONE)
    long long tt=timer.getValue()+timer.ns2tick(slack+timePerByte*size);
    auto secondTimeout=config.strictTimeout ? min(timeout,tt) : max(timeout,tt);
    if(timer.absoluteWaitTimeoutOrEvent(secondTimeout))
    {
        if(timer.getValue()<timeout)
        {
            //We didn't hit the caller set timeout, something is wrong
            idle();
            throw runtime_error("Transceiver::handlePacketReceptionEvents unexpected timeout");
        }
        result.error=RecvResult::TIMEOUT;
        return true;
    }
    exc=getExceptions(0b010);
    if((exc & CC2520Exception::RX_FRM_DONE)==0)
    {
        //Unexpected situation, didn't get the frm done
        idle();
        throw runtime_error("Transceiver::handlePacketReceptionEvents no frm done");
    }
    clearException(CC2520Exception::RX_FRM_DONE);
    return false;
}

void Transceiver::readPacketFromRxBuffer(void* pkt, int size, RecvResult& result)
{
    {
        TransceiverChipSelect cs;
        spi.sendRecv(toByte(CC2520Command::RXBUF));
        int bytesInBuffer=spi.sendRecv();
        if(bytesInBuffer>127 || bytesInBuffer<(config.crc ? 3 : 1))
        {
            idle();
            throw range_error("Transceiver::readPacketFromRxBuffer length error");
        }
        if(config.crc) bytesInBuffer-=2;
        unsigned char *ptr=reinterpret_cast<unsigned char *>(pkt);
        int bytesToRead=min(bytesInBuffer,size);
        result.size=bytesToRead;
        for(int i=0;i<bytesToRead;i++) *ptr++=spi.sendRecv();
        if(bytesInBuffer>size)
        {
            result.error=RecvResult::TOO_LONG;
            //Read the extra bytes from the transceiver and discard them
            for(int i=0;i<bytesInBuffer-size;i++) spi.sendRecv();
        }
        if(config.crc)
        {
            result.rssi=decodeRssi(spi.sendRecv());   
            unsigned char crcCheck=spi.sendRecv();
            result.error = crcCheck & 0x80? RecvResult::OK : RecvResult::CRC_FAIL;
        } else
            result.error = RecvResult::OK;
    }
    
    //FIFOP is raised at the end of each valid packet, so clear it
    clearException(CC2520Exception::FIFOP);
    
    //Look for unexpected exceptions
    if(complExcChB::value())
    {
        unsigned int exc=getExceptions(0b101);
        if(exc & CC2520Exception::RX_UNDERFLOW)
        {
            idle();
            throw runtime_error("Transceiver::readPacketFromRxBuffer underflow");
        }
        if(exc & CC2520Exception::RX_FRM_ABORTED)
        {
            idle();
            throw runtime_error("Transceiver::readPacketFromRxBuffer frm aborted");
        }
    }
}

CC2520ExceptionBitmask Transceiver::getExceptions(int part)
{
    unsigned int result=0;
    if(part & 0b001)
        result|=static_cast<unsigned int>(readReg(CC2520Register::EXCFLAG0));
    if(part & 0b010)
        result|=static_cast<unsigned int>(readReg(CC2520Register::EXCFLAG1))<<8;
    if(part & 0b100)
        result|=static_cast<unsigned int>(readReg(CC2520Register::EXCFLAG2))<<16;
    return result;
}

void Transceiver::clearException(CC2520ExceptionBitmask exc)
{
    if(exc & 0xff)     writeReg(CC2520Register::EXCFLAG0,~( exc      & 0xff));
    if(exc & 0xff00)   writeReg(CC2520Register::EXCFLAG1,~((exc>>8)  & 0xff));
    if(exc & 0xff0000) writeReg(CC2520Register::EXCFLAG2,~((exc>>16) & 0xff));
}

void Transceiver::clearAllExceptions()
{
    //Exception channel B : SFD | TX_FRM_DONE | RX_FRM_DONE
    //These are the "expected" exceptions
    //Complementary exception channel B : all other exceptions
    //These are the "unexpected" exceptions, something went wrong
    if(transceiver::excChB::value() || complExcChB::value())
    {
        writeReg(CC2520Register::EXCFLAG0,0);
        writeReg(CC2520Register::EXCFLAG1,0);
        writeReg(CC2520Register::EXCFLAG2,0);
    }
}

void Transceiver::waitXosc()
{
    //The simplest possible implementation is
    //while(internalSpi::miso::value()==0) ;
    //but it is too energy hungry
    
    auto misoPin=internalSpi::miso::getPin();
    FastInterruptDisableLock dLock;
    waiting=Thread::IRQgetCurrentThread();
    IRQenableGpioIrq(misoPin);
    do {
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    } while(waiting);
    IRQdisableGpioIrq(misoPin);
}

unsigned char Transceiver::txPower(int dBm)
{
    CC2520TxPower result;
    if(dBm>=5)       result=CC2520TxPower::P_5;
    else if(dBm>=3)  result=CC2520TxPower::P_3;
    else if(dBm==2)  result=CC2520TxPower::P_2;
    else if(dBm==1)  result=CC2520TxPower::P_1;
    else if(dBm==0)  result=CC2520TxPower::P_0;
    else if(dBm>=-2) result=CC2520TxPower::P_m2;
    else if(dBm>=-7) result=CC2520TxPower::P_m7;
    else             result=CC2520TxPower::P_m18;
    return toByte(result);
}

short int Transceiver::decodeRssi(unsigned char reg)
{
    int rawRssi=static_cast<int>(reg);
    if(rawRssi & 0x80) rawRssi-=256; //8 bit 2's complement to int
    return rawRssi-76; //See datasheet
}

} //namespace miosix
