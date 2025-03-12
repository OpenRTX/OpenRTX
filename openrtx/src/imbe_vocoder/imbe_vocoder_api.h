/*
 *  * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 *   * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 *    * Version 1.0 (c) Copyright 2009
 *     */
/* -*- c++ -*- */
#ifndef INCLUDED_IMBE_VOCODER_H
#define INCLUDED_IMBE_VOCODER_H

#include <cstdint>

#define FRAME             160   // Number samples in frame
#define NUM_HARMS_MAX      56   // Maximum number of harmonics
#define NUM_HARMS_MIN       9   // Minimum number of harmonics
#define NUM_BANDS_MAX      12   // Maximum number of bands
#define MAX_BLOCK_LEN      10   // Maximum length of block used during spectral amplitude encoding
#define NUM_PRED_RES_BLKS   6   // Number of Prediction Residual Blocks
#define PE_LPF_ORD         21   // Order of Pitch estimation LPF 
#define PITCH_EST_FRAME   301   // Pitch estimation frame size


#define B_NUM           (NUM_HARMS_MAX - 1)


typedef struct 
{
	short e_p;
	short pitch;                 // Q14.2
	short ref_pitch;             // Q8.8 
	int fund_freq;
	short num_harms;
	short num_bands;
	short v_uv_dsn[NUM_HARMS_MAX];
	short b_vec[NUM_HARMS_MAX + 3];
	short bit_alloc[B_NUM + 4];
	short sa[NUM_HARMS_MAX];
	short l_uv;
	short div_one_by_num_harm;
	short div_one_by_num_harm_sh;
} IMBE_PARAM;

typedef struct  
{
	short re;
	short im;
} Cmplx16;

class imbe_vocoder_impl;
class imbe_vocoder
{
public:
    imbe_vocoder(void);	// constructor
    ~imbe_vocoder();   	// destructor
    // imbe_encode compresses 160 samples (in unsigned int format)
    // outputs u[] vectors as frame_vector[]
    void imbe_encode(int16_t *frame_vector, int16_t *snd);
    
    // imbe_decode decodes IMBE codewords (frame_vector),
    // outputs the resulting 160 audio samples (snd)
    void imbe_decode(int16_t *frame_vector, int16_t *snd);
	void encode_4400(int16_t *snd, uint8_t *imbe);
	void decode_4400(int16_t *snd, uint8_t *imbe);
    const IMBE_PARAM* param(void);

private:
    imbe_vocoder_impl *Impl;
};
#endif /* INCLUDED_IMBE_VOCODER_H */
