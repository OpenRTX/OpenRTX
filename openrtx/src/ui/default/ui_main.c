/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/cps_io.h>
#include <stdio.h>
#include <stdint.h>
#include <ui/ui_default.h>
#include <string.h>
#include <ui/ui_strings.h>

void _ui_drawMainBackground()
{
    // Print top bar line of hline_h pixel height
    gfx_drawHLine(layout.top_h, layout.hline_h, color_grey);
    // Print bottom bar line of 1 pixel height
    gfx_drawHLine(SCREEN_HEIGHT - layout.bottom_h - 1, layout.hline_h, color_grey);
}

void _ui_drawMainTop(ui_state_t * ui_state)
{
#ifdef RTC_PRESENT
    // Print clock on top bar
    datetime_t local_time = utcToLocalTime(last_state.time,
                                           last_state.settings.utc_timezone);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "%02d:%02d:%02d", local_time.hour,
              local_time.minute, local_time.second);
#endif
    // If the radio has no built-in battery, print input voltage
#ifdef BAT_NONE
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_RIGHT,
              color_white,"%.1fV", last_state.v_bat);
#else
    // Otherwise print battery icon on top bar, use 4 px padding
    uint16_t bat_width = SCREEN_WIDTH / 9;
    uint16_t bat_height = layout.top_h - (layout.status_v_pad * 2);
    point_t bat_pos = {SCREEN_WIDTH - bat_width - layout.horizontal_pad,
                       layout.status_v_pad};
    gfx_drawBattery(bat_pos, bat_width, bat_height, last_state.charge);
#endif
    if( ui_state->input_lockedout == true )
      gfx_drawSymbol(layout.top_pos, layout.top_symbol_size, TEXT_ALIGN_LEFT,
                         color_white, SYMBOL_LOCK);
}

void _ui_drawBankChannel()
{
    // Print Bank number, channel number and Channel name
    uint16_t b = (last_state.bank_enabled) ? last_state.bank : 0;
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%01d-%03d: %.12s",
              b, last_state.channel_index + 1, last_state.channel.name);
}

void _ui_drawModeInfo(ui_state_t* ui_state)
{
    char bw_str[8] = { 0 };
    char encdec_str[9] = { 0 };

    rtxStatus_t cfg = rtx_getCurrentStatus();

    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
        // Get Bandwidth string
        if(last_state.channel.bandwidth == BW_12_5)
            snprintf(bw_str, 8, "NFM");
        else if(last_state.channel.bandwidth == BW_20)
            snprintf(bw_str, 8, "FM20");
        else if(last_state.channel.bandwidth == BW_25)
            snprintf(bw_str, 8, "FM");
        // Get encdec string
        bool tone_tx_enable = last_state.channel.fm.txToneEn;
        bool tone_rx_enable = last_state.channel.fm.rxToneEn;
        if (tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, "ED");
        else if (tone_tx_enable && !tone_rx_enable)
            snprintf(encdec_str, 9, " E");
        else if (!tone_tx_enable && tone_rx_enable)
            snprintf(encdec_str, 9, " D");
        else
            snprintf(encdec_str, 9, "  ");

        if (tone_tx_enable || tone_rx_enable)
          // Print Bandwidth, Tone and encdec info
          gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                color_white, "%s %4.1f %s",
                bw_str, 
                ctcss_tone[last_state.channel.fm.txTone]/10.0f,
                encdec_str
                );
        else
          gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                color_white, "%s",
                bw_str
                );
        break;
        case OPMODE_DMR:
        // Print talkgroup
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
              color_white, "DMR TG%s",
              "");
        break;
        case OPMODE_M17:
        {
            // Print M17 Destination ID on line 3 of 3
            const char *dst = NULL;
            if(ui_state->edit_mode)
                dst = ui_state->new_callsign;
            else
                dst = (!strnlen(cfg.destination_address, 10)) ?
                    currentLanguage->broadcast : cfg.destination_address;
            gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, "M17 #%s", dst);
            break;
        }
    }
}

typedef enum band {
  BAND_UNK  = 0,
  BAND_2M   = 1,
  BAND_70CM = 2,
  BAND_1M25 = 3,
  BAND_33CM = 4,
  BAND_23CM = 5,
  BAND_6M   = 6,
  BAND_10M   = 7,
} band_t;
unsigned long std_offset_by_band[] = { //order is of the enums for band_t
        0, //UNKnown band
   600000, //2M
  5000000, //70cm
  1600000, //1.25M
 25000000, //33cm 25MHz offset
 12000000, //23cm 12MHz
  1000000, //6m  1MHz
   100000, //10m  .1MHz
};
band_t freq2band( unsigned long freq ){
  if( freq >= 136000000 && freq <= 174000000 )
    return BAND_2M;
  if( freq >= 400000000 && freq <= 520000000 )
    return BAND_70CM;
  if( freq >= 219000000 && freq <= 225000000 )
    return BAND_1M25;
  if( freq >= 902000000 && freq <= 928000000 )
    return BAND_33CM;
  if( freq >= 1240000000 && freq <= 1300000000 )
    return BAND_23CM;
  if( freq >= 50000000 && freq <= 54000000 )
    return BAND_6M;
  if( freq >= 28000000 && freq <= 29700000 )
    return BAND_10M;
  else
    return BAND_UNK;
}

int cleanLeadingZeros( char * s, int len ){
  int i = 0;
  int n = 0;
  for( ; n < len && s[n] == '0' && s[n+1] != 0; n++ ){
    ;
  }
  for( i = 0; i < len-n; i++ ){
    s[i] = s[i+n];
  }
  return n;
}
int cleanTrailingZerosAndPeriods( char * s, int len ){
  int i = len-1;
  for( ; i >= 0 && (s[i] == '0' || s[i] == '.' || s[i] == 0); i-- ){
    //if( s[i] == '0' )
    s[i] = 0;
  }
  return i+1; //exits from for loop with first character we want to keep
              //but we want to return index of first overwritable spot
}
void snprintFrequency( long f, char * store, size_t maxspace ){
  if( f < 0 ){
    f *= -1;
  }
  long mhz = (long)f/1000000;
  long frac = (long)f%1000000/10;
  snprintf(store, maxspace, "%ld.%ld", mhz, frac);
  cleanTrailingZerosAndPeriods(store,maxspace);
  cleanLeadingZeros(store,maxspace);
}
void convertToCompressedFrequency( unsigned long tx, unsigned long rx, char * store, size_t maxspace){
  /*band_t txband = freq2band(tx);*/
  band_t rxband = freq2band(rx);
  long stdoffset = std_offset_by_band[rxband];
  long offset = tx - rx; //note signed!
  char offset_s[20] = {0};
  snprintFrequency(rx, store, maxspace);
  int nextSpot = strnlen(store, maxspace);
  if( offset != 0 ){
    if( offset > 0 )
      offset_s[0] = '+';
    else{
      offset *= -1;
      offset_s[0] = '-';
    }
    if( offset != stdoffset ){
      snprintFrequency(offset, offset_s+1, 18);
    }
    snprintf( store+nextSpot, maxspace-nextSpot, "%s", offset_s);
  }
}
void _ui_drawFrequency()
{
  unsigned long tx = last_state.channel.tx_frequency;
  unsigned long rx = last_state.channel.rx_frequency;
  band_t txband = freq2band(tx);
  band_t rxband = freq2band(rx);

  if( !platform_getPttStatus() && txband != BAND_UNK && rxband != BAND_UNK && txband == rxband ){
    char freq[20] = {0};
    convertToCompressedFrequency( tx, rx, freq, 19 );
    if( strnlen(freq,19) <= 10 ){ 
      //make sure it will fit sanely on screen -- hardcoded to 10 chars TODO
      gfx_print(layout.line3_large_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
          color_white, freq);
      return;
    }
    //else fall through and print normally

  }
  unsigned long frequency = platform_getPttStatus() ?
    frequency = last_state.channel.tx_frequency : last_state.channel.rx_frequency;
  // Print big numbers frequency
  gfx_print(layout.line3_large_pos, layout.line3_large_font, TEXT_ALIGN_CENTER,
      color_white, "%03lu.%05lu",
      (unsigned long)frequency/1000000,
      (unsigned long)frequency%1000000/10);
}

void _ui_drawVFOMiddleInput(ui_state_t* ui_state)
{
    // Add inserted number to string, skipping "Rx: "/"Tx: " and "."
    uint8_t insert_pos = ui_state->input_position + 3;
    if(ui_state->input_position > 3) insert_pos += 1;
    char input_char = ui_state->input_number + '0';

    if(ui_state->input_set == SET_RX)
    {
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Rx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            // Replace Rx frequency with underscorses
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_rx_freq_buf, ">Rx:___.____");
            ui_state->new_rx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_rx_freq_buf);
        }
        gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Tx:%03lu.%04lu",
                  (unsigned long)last_state.channel.tx_frequency/1000000,
                  (unsigned long)(last_state.channel.tx_frequency%1000000)/100);
    }
    else if(ui_state->input_set == SET_TX)
    {
        gfx_print(layout.line2_pos, layout.input_font, TEXT_ALIGN_CENTER,
                  color_white, " Rx:%03lu.%04lu",
                  (unsigned long)ui_state->new_rx_frequency/1000000,
                  (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        // Replace Rx frequency with underscorses
        if(ui_state->input_position == 0)
        {
            gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ">Tx:%03lu.%04lu",
                      (unsigned long)ui_state->new_rx_frequency/1000000,
                      (unsigned long)(ui_state->new_rx_frequency%1000000)/100);
        }
        else
        {
            if(ui_state->input_position == 1)
                strcpy(ui_state->new_tx_freq_buf, ">Tx:___.____");
            ui_state->new_tx_freq_buf[insert_pos] = input_char;
            gfx_print(layout.line3_large_pos, layout.input_font, TEXT_ALIGN_CENTER,
                      color_white, ui_state->new_tx_freq_buf);
        }
    }
}

void _ui_drawMainBottom()
{
    // Squelch bar
    float rssi = last_state.rssi;
    float squelch = last_state.settings.sqlLevel / 16.0f;
    uint16_t meter_width = SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad,
                          SCREEN_HEIGHT - meter_height - layout.bottom_pad};
    uint8_t mic_level = platform_getMicLevel();
    switch(last_state.channel.mode)
    {
        case OPMODE_FM:
            gfx_drawSmeter(meter_pos,
                           meter_width,
                           meter_height,
                           rssi,
                           squelch,
                           yellow_fab413);
            break;
        case OPMODE_DMR:
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level);
            break;
        case OPMODE_M17:
            gfx_drawSmeterLevel(meter_pos,
                                meter_width,
                                meter_height,
                                rssi,
                                mic_level);
            break;
    }
}

void _ui_drawMainVFO(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawModeInfo(ui_state);
    _ui_drawFrequency();
    _ui_drawMainBottom();
}

void _ui_drawMainVFOInput(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawVFOMiddleInput(ui_state);
    _ui_drawMainBottom();
}

void _ui_drawMainMEM(ui_state_t* ui_state)
{
    gfx_clearScreen();
    _ui_drawMainTop(ui_state);
    _ui_drawBankChannel();
    _ui_drawModeInfo(ui_state);
    _ui_drawFrequency();
    _ui_drawMainBottom();
}
