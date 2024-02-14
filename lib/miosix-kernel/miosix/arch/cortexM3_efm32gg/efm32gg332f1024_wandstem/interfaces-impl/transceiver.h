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

#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <miosix.h>
#include <limits>
#include "power_manager.h"
#include "spi.h"
#include "timer_interface.h"

namespace miosix {

//Forward declarations
enum class CC2520Register;
enum class CC2520Command;
enum class CC2520State;
using CC2520ExceptionBitmask=unsigned int;
using CC2520StatusBitmask=unsigned char;

/// Pass this to recv() to disable timeout
const long long infiniteTimeout=std::numeric_limits<long long>::max();

/**
 * This class is used to configure the transceiver
 */
class TransceiverConfiguration
{
public:
    TransceiverConfiguration(int frequency=2450, int txPower=0, bool crc=true,
                             bool strictTimeout=true)
        : frequency(frequency), txPower(txPower), crc(crc),
          strictTimeout(strictTimeout) {}
    
    /**
     * Configure the frequency field of this class from a IEEE 802.15.4
     * channel number
     * \param channel IEEE 802.15.4 channel number (from 11 to 26)
     */
    void setChannel(int channel);

    int frequency;      ///< TX/RX frequency, between 2394 and 2507
    int txPower;        ///< TX power in dBm
    bool crc;           ///< True to add CRC during TX and check it during RX
    bool strictTimeout; ///< Used only when receiving. If false and an SFD has
                        ///< been received, prolong the timeout to receive the
                        ///< packet. If true, return upon timeout even if a
                        ///< packet is being received
};

/**
 * This class is returned by the recv member function of the transceiver
 */
class RecvResult
{
public:
    /**
     * Possible outcomes of a receive operation
     */
    enum ErrorCode
    {
        OK,             ///< Receive succeeded
        TIMEOUT,        ///< Receive timed out
        TOO_LONG,       ///< Packet was too long for the given buffer
        CRC_FAIL,       ///< Packet failed CRC check
        UNINITIALIZED   ///< Receive returned exception
    };
    
    RecvResult()
        : timestamp(0), rssi(-128), size(0), error(UNINITIALIZED), timestampValid(false) {}
    
    long long timestamp; ///< Packet timestamp. It is the time point when the
                         ///< first bit of the packet preamble is received
    short rssi;          ///< RSSI of received packet (not valid if CRC disabled)
    short size;          ///< Packet size in bytes (excluding CRC if enabled)
    ErrorCode error;     ///< Possible outcomes of the receive operation
    bool timestampValid; ///< True if timestamp is valid
};

/**
 * This singleton class allows to interact with the radio transceiver
 * This class is not safe to be accessed by multiple threads simultaneously.
 */
class Transceiver
{
public:
    
    enum Unit{
        TICK,
        NS
    };
    
    enum Correct{
        CORR,
        UNCORR
    };
    
    static const int minFrequency=2405; ///< Minimum supported frequency (MHz)
    static const int maxFrequency=2480; ///< Maximum supported frequency (MHz)
    
    /**
     * \return an instance to the transceiver
     */
    static Transceiver& instance();

    /**
     * Turn the transceiver ON (i.e: bring it out of deep sleep)
     */
    void turnOn();
    
    /**
     * \internal this member function is used for restoring the transceiver
     * state during deep sleep
     */
    void configure();

    /**
     * Turn the transceiver OFF (i.e: bring it to deep sleep)
     */
    void turnOff();
    
    /**
     * \return true if the transceiver is turned on
     */
    bool isTurnedOn() const;
    
    /**
     * \return true if the transceiver is turned on
     */
    bool IRQisTurnedOn() const;
    
    /**
     * Put the transceiver to idle state.
     * This function is meant to be called after sending or receiving data to
     * make sure the transceiver is set to the idle state to save some power.
     * Its use is not required for the operation of the transceiver, and if not
     * used the transceiver may remain in receive mode more than necessary.
     */
    void idle();

    /**
     * Configure the transceiver
     * \param config transceiver configuration class
     */
    void configure(const TransceiverConfiguration& config);

    /**
     * Send a packet without checking for a clear channel. Packet transmission
     * time point is managed in software and as such is not very time
     * deterministic
     * \param pkt pointer to the packet bytes
     * \param size packet size in bytes. If CRC is enabled the maximum size is
     * 125 bytes (the packet must not contain the CRC, which is appended
     * by this class). If CRC is disabled, maximum length is 127 bytes
     * \throws exception in case of errors
     */
    void sendNow(const void *pkt, int size);

    /**
     * Send a packet checking for a clear channel. If the receiver is not turned
     * on, at least 192us+128us is spent going in receive mode and checking the
     * radio channel. Then, another 192us are spent going in TX mode.
     * Packet transmission time point is managed in software and as such is not
     * very time deterministic
     * \param pkt pointer to the packet bytes
     * \param size packet size in bytes. If CRC is enabled the maximum size is
     * 125 bytes (the packet must not contain the CRC, which is appended
     * by this class). If CRC is disabled, maximum length is 127 bytes
     * \throws exception in case of errors
     */
    bool sendCca(const void *pkt, int size);

    /**
     * Send a packet at a precise time point. This function needs to be called
     * at least 192us before the time when the packet needs to be sent, because
     * the transceiver takes this time to transition to the TX state.
     * \param pkt pointer to the packet bytes
     * \param size packet size in bytes. If CRC is enabled the maximum size is
     * 125 bytes (the packet must not contain the CRC, which is appended
     * by this class). If CRC is disabled, maximum length is 127 bytes
     * \param when the time point when the first bit of the preamble of the
     * packet is to be transmitted on the wireless channel
     * \throws exception in case of errors
     */
    void sendAt(const void *pkt, int size, long long when, Unit = Unit::NS);
    
    /**
     * \param pkt pointer to a buffer where the packet will be stored
     * \param size size of the buffer
     * \param timeout timeout absolute time after which the function returns
     * \param unit the unit in which timeout is specified and packet is timestamped
     * \param c if the returned timestamp will be corrected by VT or not
     * \return a RecvResult class with the information about the operation
     * outcome
     * \throws exception in case of errors
     * 
     * NOTE: that even if the timeout is in the past, if there is a packet
     * already received, it is returned. So if the caller code has a loop
     * getting packets with recv() and processing, if the processing time is
     * larger than the time it takes to receive a packet, this situation will
     * cause the loop not to end despite the timeout. If it is strictly
     * necessary to stop processing packets received after the timeout, it is
     * responsibility of the caller to check the timeout and discard the
     * packet.
     */
    RecvResult recv(void *pkt, int size, long long timeout, Unit unit=Unit::NS, Correct c=Correct::CORR);

    /**
     * Read the RSSI of the currently selected channel
     * \return RSSI value in dBm
     */
    short readRssi();
    
protected:
    /**
     * Write a register in the transceiver
     * \param reg which register
     * \param data data to write in the register
     */
    void writeReg(CC2520Register reg, unsigned char data);
    
    /**
     * Read a register of the transceiver
     * \param reg which register
     * \return the register content
     */
    unsigned char readReg(CC2520Register reg);
    
    /**
     * Send a command to the transceiver. Only meat to be used for commands
     * that take no arguments, and do not return anything
     * \param cmd command to send
     * \return the status byte
     */
    CC2520StatusBitmask commandStrobe(CC2520Command cmd);
    
private:
    /**
     * Constructor
     */
    Transceiver();
    Transceiver(const Transceiver&)=delete;
    Transceiver& operator= (const Transceiver&)=delete;
    
    /**
     * Start the receiver and wait untile the RSSI is valid
     * \throws runtime_error in case of timeout
     */
    void startRxAndWaitForRssi();

    /**
     * Send a data packet to the transceiver for transmission
     * \param pkt packet to send
     * \param size size of the packet. Maximum length is 125 bytes if CRC is
     * enabled, or 127 bytes if CRC is disabled
     * \throws exception in case of errors
     */
    void writePacketToTxBuffer(const void *pkt, int size);
    
    /**
     * This function is meant to be called after the packet transmission
     * command strobe has been sent to wait for the SFD and TX_FRM_DONE
     * exceptions
     * \param size packet payload size in bytes (excluding length byte and
     * CRC if enabled), used for sizing the timeout
     * \throws runtime_error in case of timeout or errors
     */
    void handlePacketTransmissionEvents(int size);
    
    /**
     * This function is meant to be called when the RX fifo is empty and
     * there is the need to receive a packet. It will wait for the SFD and
     * RX_FRM_DONE exceptions
     * \param timeout timeout for reception
     * \param size maximum packet size in bytes
     * \param result the function will set result.error to TIMEOUT when
     * returning due to a timeout, and will set result.timestampValid if an SFD
     * was detected
     * \return true in case of timeout
     * \throws runtime_error in case of errors
     */
    bool handlePacketReceptionEvents(long long timeout, int size, RecvResult& result, Unit unit, Correct c);
    
    /**
     * Read a data packet from the transceiver
     * \param pkt buffer where the packet will be stored
     * \param size maximum size of the packet
     * \param result additional information that will be returned
     * \throws exception in case of errors
     */
    void readPacketFromRxBuffer(void *pkt, int size, RecvResult& result);
    
    /**
     * Read the current transceiver exception state
     * \param part which part of the exception state to read, as an
     * optimization, bit 0 causes EXCFLAG0 to be read, bit 1 is for EXCFLAG1
     * and bit 2 is for EXCFLAG2
     * \return the current transceiver exception state
     */
    CC2520ExceptionBitmask getExceptions(int part=0b111);
    
    /**
     * Selectively clear some exceptions
     * \param exc bitmask indicating which exceptions to clear
     */
    void clearException(CC2520ExceptionBitmask exc);
    
    /**
     * Clear all pending exceptions of the transceiver
     */
    void clearAllExceptions();
    
    /**
     * When exiting from deep sleep, wait until the transceiver crystal is
     * started
     */
    void waitXosc();
    
    /**
     * Convert between transmission power values in dBm and the value to write
     * in the transceiver register to get the closest possible power level.
     * \param dBm transmission power in dBm
     * \return value to write in the transceiver register
     */
    static unsigned char txPower(int dBm);
    
    /**
     * \param reg the encoded RSSI value
     * \return the RSSI in dBm
     */
    static short decodeRssi(unsigned char reg);

    PowerManager& pm;
    Spi& spi;
    HardwareTimer& timer;
    CC2520State state;
    TransceiverConfiguration config;
    miosix::Thread *waiting;
};

} //namespace miosix

#endif //TRANSCEIVER_H
