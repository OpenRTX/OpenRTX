#include <interfaces/platform.h>
#include <cps.h>


channel_t get_default_channel()
{
    channel_t channel;
    channel.mode      = OPMODE_FM;
    channel.bandwidth = BW_25;
    channel.power     = 1.0;

    // Set initial frequency based on supported bands
    const hwInfo_t* hwinfo  = platform_getHwInfo();
    if(hwinfo->uhf_band)
    {
        channel.rx_frequency = 430000000;
        channel.tx_frequency = 430000000;
    }
    else if(hwinfo->vhf_band)
    {
        channel.rx_frequency = 144000000;
        channel.tx_frequency = 144000000;
    }

    channel.fm.rxToneEn = 0; //disabled
    channel.fm.rxTone   = 0; //and no ctcss/dcs selected
    channel.fm.txToneEn = 0;
    channel.fm.txTone   = 0;
    return channel;
}
