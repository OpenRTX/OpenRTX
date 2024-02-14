/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <miosix.h>
#include "spi.h"
#include "hrtb.h"
#include "rtc.h"
#include "vht.h"
#include "virtual_clock.h"

namespace miosix {

//Forward decl
class Transceiver;

/**
 * This class allows to handle the power management of the entire node.
 * This class is internally mutexed, and is thus safe to be accessed
 * concurrently by multiple htreads.
 */
class PowerManager
{
public:
    
    enum Unit{
        NS,
        TICK
    };
    /**
     * \return the PowerManager instance (singleton)
     */
    static PowerManager& instance();
    
    /**
     * Go to deep sleep until the specified time point
     * \param when absolute time point of the Rtc when to wake up
     * \param the default is in NS, you can force the TICK unit 
     */
    void deepSleepUntil(long long when/*, Unit unit=Unit::NS*/);
    
    /**
     * The transceiver power domain is handled using a reference count.
     * Drivers may enable it, incrementing the reference count. When drivers
     * disable it, it is really disabled only if this is the last driver that
     * has requested it.
     * 
     * This member function enables the power domain if it was disabled, and
     * increments the reference count.
     */
    void enableTransceiverPowerDomain();
    
    /**
     * The transceiver power domain is handled using a reference count.
     * Drivers may enable it, incrementing the reference count. When drivers
     * disable it, it is really disabled only if this is the last driver that
     * has requested it.
     * 
     * This member function decrements the reference count, and disables the
     * power domain if the reference count is zero.
     */
    void disableTransceiverPowerDomain();
    
    /**
     * \return the state of the transceiver power domain
     */
    bool isTransceiverPowerDomainEnabled() const;
    
    /**
     * The sensor power domain is handled using a reference count.
     * Drivers may enable it, incrementing the reference count. When drivers
     * disable it, it is really disabled only if this is the last driver that
     * has requested it.
     * 
     * This member function enables the power domain if it was disabled, and
     * increments the reference count.
     */
    void enableSensorPowerDomain();
    
    /**
     * The sensor power domain is handled using a reference count.
     * Drivers may enable it, incrementing the reference count. When drivers
     * disable it, it is really disabled only if this is the last driver that
     * has requested it.
     * 
     * This member function decrements the reference count, and disables the
     * power domain if the reference count is zero.
     */
    void disableSensorPowerDomain();
    
    /**
     * \return the state of the transceiver power domain
     */
    bool isSensorPowerDomainEnabled() const;
    
    /**
     * The node voltage regulator allows to select two voltages for the node,
     * HIGH (3.1V) or LOW (2.3V). This trnasition is handled using a reference
     * count.
     * Drivers may request the high voltage state, incrementing the reference
     * count. When drivers disable it, it is really disabled only if this is
     * the last driver that has requested it.
     * 
     * This member function selects the high regulator voltage if it was
     * disabled, and increments the reference count.
     */
    void enableHighRegulatorVoltage();
    
    /**
     * The node voltage regulator allows to select two voltages for the node,
     * HIGH (3.1V) or LOW (2.3V). This trnasition is handled using a reference
     * count.
     * Drivers may request the high voltage state, incrementing the reference
     * count. When drivers disable it, it is really disabled only if this is
     * the last driver that has requested it.
     * 
     * This member function decrements the reference count, and selects the low
     * voltage state if the reference count is zero.
     */
    void disableHighRegulatorVoltage();
    
    /**
     * \return true if the voltage regulator is set to 3.1V, failse if it is set
     * to 2.3V
     */
    bool isRegulatorVoltageHigh();
    
private:
    /*
     * Constructor
     */
    PowerManager();
    PowerManager(const PowerManager&)=delete;
    PowerManager& operator= (const PowerManager&)=delete;
    
    /**
     * Called before entering deep sleep
     */
    void IRQpreDeepSleep(Transceiver& rtx);
    
    /**
     * Called before WFI to implement the parallel HFXO and cc2520 vreg startup
     */
    void IRQmakeSureTransceiverPowerDomainIsDisabled();
    
    /**
     * Called after WFI to implement the parallel HFXO and cc2520 vreg startup
     */
    void IRQrestartHFXOandTransceiverPowerDomainEnable();
    
    /**
     * Called after exiting deep sleep 
     */
    void IRQpostDeepSleep(Transceiver& rtx);
    
    void IRQresyncClock();
    
    int transceiverPowerDomainRefCount;
    int sensorPowerDomainRefCount;
    int regulatorVoltageRefCount;
    FastMutex powerMutex;
    bool wasTransceiverTurnedOn;
    bool transceiverPowerDomainExplicitDelayNeeded;
    Spi& spi;

    HRTB& b;
    Rtc& rtc;
    VHT& vht;
    VirtualClock& vt;
    TimeConversion tc;
};

/**
 * RAII class to enable the transceiver power domain in a scope
 */
class TransceiverPowerDomainLock
{
public:
    TransceiverPowerDomainLock(PowerManager& pm) : pm(pm)
    {
        pm.enableTransceiverPowerDomain();
    }
    
    ~TransceiverPowerDomainLock()
    {
        pm.disableTransceiverPowerDomain();
    }
    
private:
    PowerManager& pm;
};

/**
 * RAII class to enable the sensor power domain in a scope
 */
class SensorPowerDomainLock
{
public:
    SensorPowerDomainLock(PowerManager& pm) : pm(pm)
    {
        pm.enableSensorPowerDomain();
    }
    
    ~SensorPowerDomainLock()
    {
        pm.disableSensorPowerDomain();
    }
    
private:
    PowerManager& pm;
};

/**
 * RAII class to set the node regulator to high voltage mode in a scope
 */
class HighRegulatorVoltageLock
{
public:
    HighRegulatorVoltageLock(PowerManager& pm) : pm(pm)
    {
        pm.enableHighRegulatorVoltage();
    }
    
    ~HighRegulatorVoltageLock()
    {
        pm.disableHighRegulatorVoltage();
    }
    
private:
    PowerManager& pm;
};

} //namespace miosix

#endif //POWERMANAGER_H
