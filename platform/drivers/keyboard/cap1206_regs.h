/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CAP1206_REGS_H
#define CAP1206_REGS_H

#define CAP1206_ADDR                            0x28    /* 7 bits address */
#define CAP1206_PRODUCT_ID                      0xFD
#define CAP1206_VENDOR_ID                       0x5D
#define CAP1206_REVISION                        0xFF

/* Registers */
#define CAP1206_MAIN_CTRL                       0x00
#define CAP1206_MAIN_CTRL_STBY                  (1 << 5)
#define CAP1206_MAIN_CTRL_DSLEEP                (1 << 4)
#define CAP1206_MAIN_CTRL_INT                   (1 << 0)

#define CAP1206_GEN_STATUS                      0x02
#define CAP1206_GEN_STATUS_BC_OUT               (1 << 6)
#define CAP1206_GEN_STATUS_ACAL_FAIL            (1 << 5)
#define CAP1206_GEN_STATUS_PWR                  (1 << 4)
#define CAP1206_GEN_STATUS_MULT                 (1 << 2)
#define CAP1206_GEN_STATUS_MTP                  (1 << 1)
#define CAP1206_GEN_STATUS_TOUCH                (1 << 0)

#define CAP1206_SENSOR_IN_STATUS                0x03
#define CAP1206_SENSOR_IN_STATUS_CS(x)          (1 << (x-1))

#define CAP1206_NOISE_FLAG                      0x0A
#define CAP1206_NOISE_FLAG_CS_NOISE(x)          (1 << (x-1))

#define CAP1206_IN_dCNT(x)                      (0x0F + x)

#define CAP1206_SENS_CTRL                       0x1F
#define CAP1206_SENS_CTRL_dSENSE_x128           (0 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x64            (1 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x32            (2 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x16            (3 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x8             (4 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x4             (5 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x2             (6 << 4)
#define CAP1206_SENS_CTRL_dSENSE_x1             (7 << 4)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x1         (0 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x2         (1 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x4         (2 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x8         (3 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x16        (4 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x32        (5 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x64        (6 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x128       (7 << 0)
#define CAP1206_SENS_CTRL_BASE_SHIFT_x256       (8 << 0)

#define CAP1206_CONFIG_1                        0x20
#define CAP1206_CONFIG_1_TIMEOUT                (1 << 7)
#define CAP1206_CONFIG_1_DIS_DIG_NOISE          (1 << 5)
#define CAP1206_CONFIG_1_DIS_ANA_NOISE          (1 << 4)
#define CAP1206_CONFIG_1_MAX_DUR_EN             (1 << 3)

#define CAP1206_CONFIG_2                        0x44
#define CAP1206_CONFIG_2_BC_OUT_RECAL           (1 << 6)
#define CAP1206_CONFIG_2_BLK_PWR_CTRL           (1 << 5)
#define CAP1206_CONFIG_2_BC_OUT_INT             (1 << 4)
#define CAP1206_CONFIG_2_SHOW_RF_NOISE          (1 << 3)
#define CAP1206_CONFIG_2_DIS_RF_NOISE           (1 << 2)
#define CAP1206_CONFIG_2_ACAL_FAIL_INT          (1 << 1)
#define CAP1206_CONFIG_2_INT_REL_n              (1 << 0)

#define CAP1206_SENS_IN_EN                      0x21
#define CAP1206_SENS_IN_EN_CS_EN(x)             (1 << (x-1))

#define CAP1206_SENS_IN_CFG                     0x22
#define CAP1206_SENS_IN_CFG_MAX_DUR_560ms       (0 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_840ms       (1 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_1120ms      (2 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_1400ms      (3 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_1680ms      (4 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_2240ms      (5 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_2800ms      (6 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_3360ms      (7 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_3920ms      (8 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_4480ms      (9 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_5600ms      (10 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_6720ms      (11 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_7840ms      (12 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_8906ms      (13 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_10080ms     (14 << 4)
#define CAP1206_SENS_IN_CFG_MAX_DUR_11200ms     (15 << 4)
#define CAP1206_SENS_IN_CFG_RPT_RATE_35ms       (0 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_70ms       (1 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_105ms      (2 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_140ms      (3 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_175ms      (4 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_210ms      (5 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_245ms      (6 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_280ms      (7 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_315ms      (8 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_350ms      (9 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_385ms      (10 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_420ms      (11 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_455ms      (12 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_490ms      (13 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_525ms      (14 << 0)
#define CAP1206_SENS_IN_CFG_RPT_RATE_560ms      (15 << 0)

#define CAP1206_SENS_IN_CFG2                    0x23
#define CAP1206_SENS_IN_CFG2_M_PRESS_35ms       (0 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_70ms       (1 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_105ms      (2 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_140ms      (3 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_175ms      (4 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_210ms      (5 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_245ms      (6 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_280ms      (7 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_315ms      (8 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_350ms      (9 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_385ms      (10 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_420ms      (11 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_455ms      (12 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_490ms      (13 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_525ms      (14 << 0)
#define CAP1206_SENS_IN_CFG2_M_PRESS_560ms      (15 << 0)

#define CAP1206_AVG_SAMP_CFG                    0x24
#define CAP1206_AVG_SAMP_CFG_AVG_1              (0 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_2              (1 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_4              (2 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_8              (3 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_16             (4 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_32             (5 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_64             (6 << 4)
#define CAP1206_AVG_SAMP_CFG_AVG_128            (7 << 4)
#define CAP1206_AVG_SAMP_CFG_SAMP_TIM_320us     (0 << 2)
#define CAP1206_AVG_SAMP_CFG_SAMP_TIM_640us     (1 << 2)
#define CAP1206_AVG_SAMP_CFG_SAMP_TIM_1ms28     (2 << 2)
#define CAP1206_AVG_SAMP_CFG_SAMP_TIM_2ms56     (4 << 2)
#define CAP1206_AVG_SAMP_CFG_CYCL_TIM_35ms      (0 << 0)
#define CAP1206_AVG_SAMP_CFG_CYCL_TIM_70ms      (1 << 0)
#define CAP1206_AVG_SAMP_CFG_CYCL_TIM_105ms     (2 << 0)
#define CAP1206_AVG_SAMP_CFG_CYCL_TIM_140ms     (3 << 0)

#define CAP1206_CAL_ACTIVE_STATUS               0x26
#define CAP1206_CAL_ACTIVE_STATUS_CS_CAL(x)     (1 << (x-1))

#define CAP1206_INT_EN                          0x27
#define CAP1206_INT_EN_CS(x)                    (1 << (x-1))

#define CAP1206_RPT_EN                          0x28
#define CAP1206_RPT_EN_CS(x)                    (1 << (x-1))

#define CAP1206_MULT_TOUCH_CFG                  0x2A
#define CAP1206_MULT_TOUCH_CFG_MULT_BLK_EN      (1 << 7)
#define CAP1206_MULT_TOUCH_CFG_B_MULT_T_1       (0 << 2)
#define CAP1206_MULT_TOUCH_CFG_B_MULT_T_2       (1 << 2)
#define CAP1206_MULT_TOUCH_CFG_B_MULT_T_3       (2 << 2)
#define CAP1206_MULT_TOUCH_CFG_B_MULT_T_4       (3 << 2)

#define CAP1206_MULT_TOUCH_PTRN                 0x2B
#define CAP1206_MULT_TOUCH_PTRN_MTP_EN          (1 << 7)
#define CAP1206_MULT_TOUCH_PTRN_MTP_TH_12_5     (0 << 2)
#define CAP1206_MULT_TOUCH_PTRN_MTP_TH_25       (1 << 2)
#define CAP1206_MULT_TOUCH_PTRN_MTP_TH_37_5     (2 << 2)
#define CAP1206_MULT_TOUCH_PTRN_MTP_TH_100      (3 << 2)
#define CAP1206_MULT_TOUCH_PTRN_COMP_PTRN       (1 << 1)
#define CAP1206_MULT_TOUCH_PTRN_MTP_ALERT       (1 << 0)

#define CAP1206_BASE_CNT_OUT_OF_LIMIT           0x2E
#define CAP1206_BASE_CNT_OUT_OF_LIMIT_CS(x)     (1 << (x-1))

#define CAP1206_RECALIB_CFG                     0x2F
#define CAP1206_RECALIB_CFG_BUT_LD_TH           (1 << 7)
#define CAP1206_RECALIB_CFG_NO_CLR_INTD         (1 << 6)
#define CAP1206_RECALIB_CFG_NO_CLR_NEG          (1 << 5)
#define CAP1206_RECALIB_CFG_NEG_dCNT_8          (0 << 3)
#define CAP1206_RECALIB_CFG_NEG_dCNT_16         (1 << 3)
#define CAP1206_RECALIB_CFG_NEG_dCNT_32         (2 << 3)
#define CAP1206_RECALIB_CFG_NEG_dCNT_NONE       (3 << 3)
#define CAP1206_RECALIB_CFG_CAL_CFG_16_16       (0 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_32_32       (1 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_64_64       (2 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_128_128     (3 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_256_256     (4 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_256_1024    (5 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_256_2048    (6 << 0)
#define CAP1206_RECALIB_CFG_CAL_CFG_256_4096    (7 << 0)

#define CAP1206_SENS_IN_TH(x)                   (0x2F + x)

#define CAP1206_SENS_IN_NOISE_TH                0x38
#define CAP1206_SENS_IN_NOISE_TH_CS_BN_TH_25    (0 << 0)
#define CAP1206_SENS_IN_NOISE_TH_CS_BN_TH_37_5  (1 << 0)
#define CAP1206_SENS_IN_NOISE_TH_CS_BN_TH_50    (2 << 0)
#define CAP1206_SENS_IN_NOISE_TH_CS_BN_TH_62_5  (3 << 0)

#define CAP1206_STBY_CHAN                       0x40
#define CAP1206_STBY_CHAN_CS(x)                 (1 << (x-1))

#define CAP1206_STBY_CFG                        0x41
#define CAP1206_STBY_CFG_AVG_SUM                (1 << 7)
#define CAP1206_STBY_CFG_STBY_AVG_1             (0 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_2             (1 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_4             (2 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_8             (3 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_16            (4 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_32            (5 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_64            (6 << 4)
#define CAP1206_STBY_CFG_STBY_AVG_128           (7 << 4)
#define CAP1206_STBY_CFG_SAMP_TIM_320u          (0 << 2)
#define CAP1206_STBY_CFG_SAMP_TIM_640u          (0 << 2)
#define CAP1206_STBY_CFG_SAMP_TIM_1_28m         (0 << 2)
#define CAP1206_STBY_CFG_SAMP_TIM_2_56m         (0 << 2)
#define CAP1206_STBY_CFG_CY_TIM_35m             (0 << 0)
#define CAP1206_STBY_CFG_CY_TIM_70m             (1 << 0)
#define CAP1206_STBY_CFG_CY_TIM_105m            (2 << 0)
#define CAP1206_STBY_CFG_CY_TIM_140m            (3 << 0)

#define CAP1206_STBY_SENS                       0x42
#define CAP1206_STBY_SENS_STBY_SENS_128         (0 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_64          (1 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_32          (2 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_16          (3 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_8           (4 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_4           (5 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_2           (6 << 0)
#define CAP1206_STBY_SENS_STBY_SENS_1           (7 << 0)

#define CAP1206_STBY_TH                         0x43

#define CAP1206_SENS_IN_BASE_CNT(x)             (0x4F + x)

#define CAP1206_PWR_BTN                         0x60
#define CAP1206_PWR_BTN_CS(x)                   (x-1)

#define CAP1206_PWR_BTN_CFG                     0x61
#define CAP1206_PWR_BTN_CFG_STBY_PWR_EN         (1 << 6)
#define CAP1206_PWR_BTN_CFG_STBY_PWR_TIME_280m  (0 << 4)
#define CAP1206_PWR_BTN_CFG_STBY_PWR_TIME_560m  (1 << 4)
#define CAP1206_PWR_BTN_CFG_STBY_PWR_TIME_1_12s (2 << 4)
#define CAP1206_PWR_BTN_CFG_STBY_PWR_TIME_2_24s (3 << 4)

#define CAP1206_SENS_IN_CAL(x)                  (0xB0 + x)

#endif /* CAP1206_REGS_H */

