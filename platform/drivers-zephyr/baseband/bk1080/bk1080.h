#ifndef __BK1080_H__
#define __BK1080_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


enum BK1080_Register_t {
	BK1080_REG_00                       = 0x00U,
	BK1080_REG_02_POWER_CONFIGURATION   = 0x02U,
	BK1080_REG_03_CHANNEL               = 0x03U,
	BK1080_REG_05_SYSTEM_CONFIGURATION2 = 0x05U,
	BK1080_REG_07                       = 0x07U,
	BK1080_REG_10                       = 0x0AU,
	BK1080_REG_25_INTERNAL              = 0x19U,
};

typedef enum BK1080_Register_t BK1080_Register_t;

// REG 07

#define BK1080_REG_07_SHIFT_FREQD		4
#define BK1080_REG_07_SHIFT_SNR			0

#define BK1080_REG_07_MASK_FREQD		(0xFFFU << BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_MASK_SNR			(0x00FU << BK1080_REG_07_SHIFT_SNR)

#define BK1080_REG_07_GET_FREQD(x)		(((x) & BK1080_REG_07_MASK_FREQD) >> BK1080_REG_07_SHIFT_FREQD)
#define BK1080_REG_07_GET_SNR(x)		(((x) & BK1080_REG_07_MASK_SNR) >> BK1080_REG_07_SHIFT_SNR)

// REG 10

#define BK1080_REG_10_SHIFT_AFCRL		12
#define BK1080_REG_10_SHIFT_RSSI		0

#define BK1080_REG_10_MASK_AFCRL		(0x01U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_MASK_RSSI			(0xFFU << BK1080_REG_10_SHIFT_RSSI)

#define BK1080_REG_10_AFCRL_NOT_RAILED	(0U << BK1080_REG_10_SHIFT_AFCRL)
#define BK1080_REG_10_AFCRL_RAILED		(1U << BK1080_REG_10_SHIFT_AFCRL)

#define BK1080_REG_10_GET_RSSI(x)		(((x) & BK1080_REG_10_MASK_RSSI) >> BK1080_REG_10_SHIFT_RSSI)


extern uint16_t BK1080_BaseFrequency;
extern uint16_t BK1080_FrequencyDeviation;

void BK1080_Init(uint32_t Frequency, uint8_t band);
uint16_t BK1080_ReadRegister(BK1080_Register_t Register);
void BK1080_WriteRegister(BK1080_Register_t Register, uint16_t Value);
void BK1080_Mute(bool Mute);
uint32_t BK1080_GetFreqLoLimit(uint8_t band);
uint32_t BK1080_GetFreqHiLimit(uint8_t band);
void BK1080_SetFrequency(uint32_t frequency, uint8_t band);
void BK1080_GetFrequencyDeviation(uint32_t Frequency);

#ifdef __cplusplus
}
#endif

#endif /* BK1080_H */
