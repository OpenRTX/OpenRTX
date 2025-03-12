#include "imbe_vocoder_impl.h"
#include "imbe_vocoder.h"

imbe_vocoder::imbe_vocoder()
{
        Impl = new imbe_vocoder_impl();
}

imbe_vocoder::~imbe_vocoder()
{
        delete Impl;
}

void imbe_vocoder::imbe_encode(int16_t *frame_vector, int16_t *snd)
{
        Impl->imbe_encode(frame_vector, snd);
}

void imbe_vocoder::imbe_decode(int16_t *frame_vector, int16_t *snd)
{
        Impl->imbe_decode(frame_vector, snd);
}

void imbe_vocoder::encode_4400(int16_t *snd, uint8_t *imbe)
{
	Impl->encode_4400(snd, imbe);
}

void imbe_vocoder::decode_4400(int16_t *snd, uint8_t *imbe)
{
	Impl->decode_4400(snd, imbe);
}

const IMBE_PARAM* imbe_vocoder::param(void)
{
	return Impl->param();
}
