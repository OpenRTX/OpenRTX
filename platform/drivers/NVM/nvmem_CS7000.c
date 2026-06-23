/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cps.h"
#include "core/vfo.h"
#include "interfaces/delays.h"
#include "interfaces/nvmem.h"
#include "calibration/calibInfo_CS7000.h"
#include "core/nvmem_access.h"
#include "core/utils.h"
#include "core/crc.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/NVM/W25Qx.h"
#include "drivers/NVM/eeep.h"
#include "core/settings.h"

#include "interfaces/platform.h"
#include "rtx/rtx.h"
#include <stdint.h>
#include <string.h>

static const struct W25QxCfg cfg =
{
    .spi = (const struct spiDevice *) &flash_spi,
    .cs  = { FLASH_CS }
};

W25Qx_DEVICE_DEFINE(eflash, cfg)
EEEP_DEVICE_DEFINE(eeep)

const struct nvmPartition memPartitions[] =
{
    {
        .offset = 0x0000,   // First partition, calibration and other OEM data
        .size   = 32768     // 32kB
    },
#ifdef PLATFORM_CS7000P
    {
        .offset = 0x1000000,// Second partition EEEP storage
        .size   = 16384     // 16kB
    },
    {
        .offset = 0x1004000,// Third partition, settings part A
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0x1005000,// Fourth partition, settings part B
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0x1006000,// Fifth partition, available memory
        .size   = 0xFF2000
    }
#else
    {
        .offset = 0x8000,   // Second partition EEEP storage
        .size   = 16384     // 16kB
    },
    {
        .offset = 0xC000,   // Third partition, settings part A
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0xD000,   // Fourth partition, settings part B
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0xE000,   // Fifth partition, available memory
        .size   = 0xFF2000
    }
#endif
};

const struct nvmPartition eeepPartitions[] =
{
    {
        .offset = 0x0000,   // Legacy partition, old vfo (0x1) and settings (0x2)
        .size   = 100         // (entries 0x00 -> 0x02)
    },
    {
        .offset = 100,        // Current partition, vfo stored as slices
        .size   = 0xFFFF-100    // To the end of the EEEP address space
    }
};

static const struct nvmDescriptor extMem[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .baseAddr   = 0x00000000,
#ifdef PLATFORM_CS7000P
        .size       = 0x2000000,        // 32 MB, 256 Mbit
#else
        .size       = 0x1000000,        // 16 MB, 128 Mbit
#endif
        .nbPart     = ARRAY_SIZE(memPartitions),
        .partitions = memPartitions
    },
    {
        .name       = "Virtual EEPROM",
        .dev        = &eeep,
        .baseAddr   = 0x00000000,
        .size       = 0xFFFF,       // Virtual address is 16 bits
        .nbPart     = ARRAY_SIZE(eeepPartitions),
        .partitions = eeepPartitions
    }
};

const struct nvmTable nvmTab = {
    .areas = extMem,
    .nbAreas = ARRAY_SIZE(extMem),
};

static struct vfo_storage vfo_storage;
static struct settings_storage settings_storage;

/**
 * In order to transparently migrate vfo and settings from v0.4.4,
 * previous implementations of vfo and settings are replicated here.
 * Those variables and functions should be deleted once OpenRTX reaches v0.6.x.
 */

typedef struct
{
    uint8_t brightness;           // Display brightness
    uint8_t contrast;             // Display contrast
    uint8_t sqlLevel;             // Squelch level
    uint8_t voxLevel;             // Vox level
    int8_t  utc_timezone;         // Timezone, in units of half hours
    bool    gps_enabled;          // GPS active
    char    callsign[10];         // Plaintext callsign
    uint8_t display_timer   : 4,  // Standby timer
            m17_can         : 4;  // M17 CAN
    uint8_t vpLevel         : 3,  // Voice prompt level
            vpPhoneticSpell : 1,  // Phonetic spell enabled
            macroMenuLatch  : 1,  // Automatic latch of macro menu
            _reserved       : 3;
    bool    m17_can_rx;           // Check M17 CAN on RX
    char    m17_dest[10];         // M17 destination
    bool    showBatteryIcon;      // Battery display true: icon, false: percentage
    bool    gpsSetTime;           // Use GPS to ajust RTC time
    char    M17_meta_text[53];    // M17 Meta Text to send
}
__attribute__((packed)) old_settings_t;

typedef struct
{
    uint8_t mode;                  //< Operating mode

    uint8_t bandwidth      : 2,    //< Bandwidth
            rx_only        : 1,    //< 1 means RX-only channel
            _unused        : 5;    //< Padding to 8 bits

    uint32_t power;                //< Tx power, in mW

    freq_t  rx_frequency;          //< RX Frequency, in Hz
    freq_t  tx_frequency;          //< TX Frequency, in Hz

    uint8_t scanList_index;        //< Scan List: None, ScanList1...250
    uint8_t groupList_index;       //< Group List: None, GroupList1...128

    char    name[CPS_STR_SIZE];    //< Channel name
    char    descr[CPS_STR_SIZE];   //< Description of the channel
    geo_t   ch_location;           //< Transmitter geolocation

    union
    {
        fmInfo_t  fm;              //< Information block for FM channels
        dmrInfo_t dmr;             //< Information block for DMR channels
        m17Info_t m17;             //< Information block for M17 channels
    };
}
__attribute__((packed)) old_vfo_t; // 59B


static bool read_old_settings(old_settings_t *old_settings)
{
    memset(old_settings, 0x00, sizeof(old_settings_t));
    return (nvm_read(1, 1, 0x0002, old_settings, sizeof(old_settings_t)) == 0);
}

/**
 * Try to read the vfo stored according to the old system (OpenRTX 0.4.4)
 *
 * @param old_vfo pointer to an old_vfo_t structure
 * @return true if read was successful, false if read failed
 */
static bool read_old_vfo(old_vfo_t *old_vfo)
{
    memset(old_vfo, 0x00, sizeof(old_vfo_t));
    return (nvm_read(1, 1, 0x0001, old_vfo, sizeof(old_vfo_t)) == 0);
}

static void migrate_settings_vfo(settings_t *new_settings, channel_t *new_vfo,
                                 const old_settings_t *old_settings, const old_vfo_t *old_vfo)
{
    *new_vfo = cps_getDefaultChannel();
    *new_settings = default_settings;

    new_vfo->mode = old_vfo->mode;
    new_vfo->bandwidth = old_vfo->bandwidth;
    new_vfo->rx_only = old_vfo->rx_only;
    new_vfo->m17_can = old_settings->m17_can;
    new_vfo->sqlLevel = old_settings->sqlLevel;
    new_vfo->power = old_vfo->power;
    new_vfo->rx_frequency = old_vfo->rx_frequency;
    new_vfo->tx_frequency = old_vfo->tx_frequency;
    new_vfo->scanList_index = old_vfo->scanList_index;
    new_vfo->groupList_index = old_vfo->groupList_index;
    strncpy(new_vfo->name, old_vfo->name, 32);
    strncpy(new_vfo->descr, old_vfo->descr, 32);
    new_vfo->ch_location = old_vfo->ch_location;
    strncpy(new_vfo->m17_dest, old_settings->m17_dest, 10);
    strncpy(new_vfo->M17_meta_text, old_settings->M17_meta_text, 53);

    switch(new_vfo->mode)
    {
        case OPMODE_M17:
            new_vfo->m17 = old_vfo->m17;
            break;
        case OPMODE_FM:
            new_vfo->fm = old_vfo->fm;
            break;
        case OPMODE_DMR:
            new_vfo->dmr = old_vfo->dmr;
            break;
    }

    new_settings->brightness = old_settings->brightness;
    new_settings->contrast = old_settings->contrast;
    new_settings->voxLevel = old_settings->voxLevel;
    new_settings->utc_timezone = old_settings->utc_timezone;
    strncpy(new_settings->callsign, old_settings->callsign, 10);
    new_settings->display_timer = old_settings->display_timer;
    new_settings->vpLevel = old_settings->vpLevel;
    new_settings->vpPhoneticSpell = old_settings->vpPhoneticSpell;
    new_settings->macroMenuLatch = old_settings->macroMenuLatch;
    new_settings->gps_enabled = old_settings->gps_enabled;
    new_settings->gpsSetTime = old_settings->gpsSetTime;
    new_settings->m17_can_rx = old_settings->m17_can_rx;
    new_settings->showBatteryIcon = old_settings->showBatteryIcon;
}

void nvm_init()
{
#ifdef PLATFORM_CS7000P
    gpio_setMode(FLASH_CLK, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDI, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDO, ALTERNATE | ALTERNATE_FUNC(5));
    spiStm32_init(&flash_spi, 25000000, 0);
#else
    spiBitbang_init(&flash_spi);
#endif
    W25Qx_init(&eflash);
    eeep_init(&eeep, 0, 2);
    vfo_initStorage(&vfo_storage, 1, 2, 16, CPS_VERSION_NUMBER);
    settings_initStorage(&settings_storage, 0, 3, 4);

    // Check if we need to migrate settings/vfo
    old_settings_t old_settings;
    old_vfo_t old_vfo;

    if( read_old_vfo(&old_vfo) && read_old_settings(&old_settings) )
    {
        channel_t vfo;
        settings_t settings;

        vfo_load(&vfo_storage, &vfo);
        settings_load(&settings_storage, &settings);
        migrate_settings_vfo(&settings, &vfo, &old_settings, &old_vfo);

        if( (vfo_save(&vfo_storage, &vfo) == 0) && (settings_save(&settings_storage, &settings) == 0) )
        {
            nvm_erase(1, 1, 0, 100);
        }
    }
}

void nvm_terminate()
{
    eeep_terminate(&eeep);
    W25Qx_terminate(&eflash);
#ifdef PLATFORM_CS7000P
    spiStm32_terminate(&flash_spi);
#else
    spiBitbang_terminate(&flash_spi);
#endif
}

void nvm_readCalibData(void *buf)
{
    struct CS7000Calib *calData = (struct CS7000Calib *) buf;

    nvm_read(0, 1, 0x1000, &(calData->txCalFreq),      sizeof(calData->txCalFreq));
    nvm_read(0, 1, 0x1020, &(calData->rxCalFreq),      sizeof(calData->rxCalFreq));
    nvm_read(0, 1, 0x1044, &(calData->rxSensitivity),  sizeof(calData->rxSensitivity));
    nvm_read(0, 1, 0x106C, &(calData->txHighPwr),      sizeof(calData->txHighPwr));
    nvm_read(0, 1, 0x1074, &(calData->txMiddlePwr),    sizeof(calData->txMiddlePwr));
    nvm_read(0, 1, 0x10C4, &(calData->txDigitalPathQ), sizeof(calData->txDigitalPathQ));
    nvm_read(0, 1, 0x10CC, &(calData->txAnalogPathI),  sizeof(calData->txAnalogPathI));
    nvm_read(0, 1, 0x10DC, &(calData->errorRate),      sizeof(calData->errorRate));

    for(int i = 0; i < 8; i++)
    {
        calData->txCalFreq[i] = __builtin_bswap32(calData->txCalFreq[i]);
        calData->rxCalFreq[i] = __builtin_bswap32(calData->rxCalFreq[i]);
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return vfo_load(&vfo_storage, channel);
}

int nvm_readSettings(settings_t *settings)
{
    return settings_load(&settings_storage, settings);
}

int nvm_writeSettings(const settings_t *settings)
{
    return settings_save(&settings_storage, settings);
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = nvm_writeSettings(settings);
    if(ret < 0)
        return ret;
    return vfo_save(&vfo_storage, vfo);
}
