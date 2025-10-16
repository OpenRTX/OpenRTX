/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LPTIM_H
#define LPTIM_H

/**
 * Handler class for STM32H7 LPTIM peripheral.
 */
class Lptim
{
public:

    /**
     * Constructor.
     *
     * @param tim: base address of timer peripheral to manage.
     * @param baseFreq: timer base input frequency, in Hz.
     */
    constexpr Lptim(const uint32_t tim, const uint32_t baseFreq) : tim(tim),
            baseFreq(baseFreq) {}

    ~Lptim() = default;

    /**
     * Configure timer prescaler and auto-reload registers for a given update
     * frequency.
     *
     * @param updFreq: desidered update frequency, in Hz.
     * @return effective timer update frequency, in Hz.
     */
    uint32_t setUpdateFrequency(const uint32_t updFreq) const
    {
        /*
         * Timer update frequency is given by:
         * Fupd = (Fbus / prescaler) / autoreload
         *
         * In LPTIM the prescaler can only assume values being powers of two up
         * to 128. To find the correct prescaler and autoreload values we start
         * by setting the the prescaler to 1 and proceed iteratively until the
         * autoreload value is less than the maximum allowed.
         */
        uint32_t div;
        uint32_t arr;
        uint32_t psc;
        for(div = 0; div < 8; div += 1)
        {
            psc = 1 << div;
            arr = (baseFreq / psc) / updFreq;
            if(arr < 0xFFFF)
                break;
        }

        // Timer needs to be enabled before configuring the other registers.
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CR   = LPTIM_CR_ENABLE;
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CFGR = div << 9;
        reinterpret_cast< LPTIM_TypeDef * >(tim)->ARR  = arr - 1;

        return (baseFreq / psc) / arr;
    }

    /**
     * Clear and start the timer's counter.
     */
    inline void start() const
    {
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CNT = 0;
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CR |= LPTIM_CR_CNTSTRT;
    }

    /**
     * Stop the timer's counter.
     */
    inline void stop() const
    {
        // It seems that the only way to stop the LPTIM is to turn it off. Then
        // we have to turn it on again to allow writing the configuration registers.
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CR = 0;
        reinterpret_cast< LPTIM_TypeDef * >(tim)->CR = LPTIM_CR_ENABLE;
    }

    /**
     * Get current value of timer's counter.
     *
     * @return value of timer's counter.
     */
    inline uint16_t value() const
    {
        return reinterpret_cast< LPTIM_TypeDef * >(tim)->CNT = 0;
    }

private:

    const uint32_t tim;
    const uint32_t baseFreq;
};

#endif /* LPTIM_H */
