/*
 *  * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 *   * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 *    * Version 1.0 (c) Copyright 2009
 *     */
/* -*- c++ -*- */
#ifndef INCLUDED_IMBE_VOCODER_H
#define INCLUDED_IMBE_VOCODER_H

#include <cstdint>

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
