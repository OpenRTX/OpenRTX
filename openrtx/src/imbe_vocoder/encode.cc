/*
 * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 * Version 1.0 (c) Copyright 2009
 * 
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * The software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */


#include "typedef.h"
#include "globals.h"
#include "imbe.h"
#include "basic_op.h"
#include "dsp_sub.h"
#include "aux_sub.h"
#include "tbls.h"
#include "encode.h"
#include "dc_rmv.h"
#include "pe_lpf.h"
#include "pitch_est.h"
#include "pitch_ref.h"
#include "v_uv_det.h"
#include "sa_encode.h"
#include "ch_encode.h"
#include "imbe_vocoder_impl.h"

const uint8_t  BIT_MASK_TABLE8[]  = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };
#define WRITE_BIT(p,i,b)   p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE8[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE8[(i)&7])


void imbe_vocoder_impl::encode_init(void)
{
	v_zap(pitch_est_buf, PITCH_EST_BUF_SIZE);
	v_zap(pitch_ref_buf, PITCH_EST_BUF_SIZE);
	v_zap(pe_lpf_mem, PE_LPF_ORD);
	pitch_est_init();
	fft_init();
	dc_rmv_mem = 0;
	sa_encode_init();
	pitch_ref_init();
}


void imbe_vocoder_impl::encode(IMBE_PARAM *imbe_param, Word16 *frame_vector, Word16 *snd)
{
	Word16 i;
	Word16 *wr_ptr, *sig_ptr;
	
	for(i = 0; i < PITCH_EST_BUF_SIZE - FRAME; i++)
	{
		pitch_est_buf[i] = pitch_est_buf[i + FRAME];
		pitch_ref_buf[i] = pitch_ref_buf[i + FRAME];
	}
		
	dc_rmv(snd, &pitch_ref_buf[PITCH_EST_BUF_SIZE - FRAME], &dc_rmv_mem, FRAME);
	pe_lpf(&pitch_ref_buf[PITCH_EST_BUF_SIZE - FRAME], &pitch_est_buf[PITCH_EST_BUF_SIZE - FRAME], pe_lpf_mem, FRAME);

	pitch_est(imbe_param, pitch_est_buf);

    //
	// Speech windowing and FFT calculation
	//
	wr_ptr  = (Word16 *)wr;
	sig_ptr = &pitch_ref_buf[40];
	for(i = 146; i < 256; i++) 
	{
		fft_buf[i].re = mult(*sig_ptr++, *wr_ptr++); 
		fft_buf[i].im = 0;
	}
	fft_buf[0].re = *sig_ptr++;
	fft_buf[0].im = 0;
	wr_ptr--;
	for(i = 1; i < 111; i++) 
	{
		fft_buf[i].re = mult(*sig_ptr++, *wr_ptr--); 
		fft_buf[i].im = 0;
	}
	for(i = 111; i < 146; i++) 
		fft_buf[i].re = fft_buf[i].im = 0;

	fft((Word16 *)&fft_buf, FFTLENGTH, 1);

	pitch_ref(imbe_param, fft_buf);
	v_uv_det(imbe_param, fft_buf);
	sa_encode(imbe_param);
	encode_frame_vector(imbe_param, frame_vector);
}

void imbe_vocoder_impl::encode_4400(int16_t *pcm, uint8_t *imbe)
{
	int16_t frame_vector[8];
	memset(imbe, 0, 11);

	imbe_encode(frame_vector, pcm);
	uint32_t offset = 0U;
	int16_t mask = 0x0800;

	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[0U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[1U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[2U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[3U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[4U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[5U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[6U] & mask) != 0);

	mask = 0x0040;
	for (uint32_t i = 0U; i < 7U; i++, mask >>= 1, offset++)
		WRITE_BIT(imbe, offset, (frame_vector[7U] & mask) != 0);
}

