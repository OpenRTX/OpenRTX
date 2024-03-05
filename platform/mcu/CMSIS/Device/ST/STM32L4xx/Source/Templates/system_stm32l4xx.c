//
// NOTE: this file contains some modifications by Silvano Seva (silseva) to make
// available system clock frequencies greater than 4MHz
//

/**
  ******************************************************************************
  * @file    system_stm32l4xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32l4xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *   After each device reset the MSI (4 MHz) is used as system clock source.
  *   Then SystemInit() function is called, in "startup_stm32l4xx.s" file, to
  *   configure the system clock before to branch to main program.
  *
  *   This file configures the system clock as follows:
  *=============================================================================
  *-----------------------------------------------------------------------------
  *        System Clock source                    | MSI
  *-----------------------------------------------------------------------------
  *        SYSCLK(Hz)                             | 4000000
  *-----------------------------------------------------------------------------
  *        HCLK(Hz)                               | 4000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB1 Prescaler                         | 1
  *-----------------------------------------------------------------------------
  *        APB2 Prescaler                         | 1
  *-----------------------------------------------------------------------------
  *        PLL_M                                  | 1
  *-----------------------------------------------------------------------------
  *        PLL_N                                  | 8
  *-----------------------------------------------------------------------------
  *        PLL_P                                  | 7
  *-----------------------------------------------------------------------------
  *        PLL_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL_R                                  | 2
  *-----------------------------------------------------------------------------
  *        PLLSAI1_P                              | NA
  *-----------------------------------------------------------------------------
  *        PLLSAI1_Q                              | NA
  *-----------------------------------------------------------------------------
  *        PLLSAI1_R                              | NA
  *-----------------------------------------------------------------------------
  *        PLLSAI2_P                              | NA
  *-----------------------------------------------------------------------------
  *        PLLSAI2_Q                              | NA
  *-----------------------------------------------------------------------------
  *        PLLSAI2_R                              | NA
  *-----------------------------------------------------------------------------
  *        Require 48MHz for USB OTG FS,          | Disabled
  *        SDIO and RNG clock                     |
  *-----------------------------------------------------------------------------
  *=============================================================================
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Apache License, Version 2.0,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/Apache-2.0
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32l4xx_system
  * @{
  */

/** @addtogroup STM32L4xx_System_Private_Includes
  * @{
  */

#include <stdint.h>

//By Silvano Seva: was #include "stm32l4xx.h"
#include "interfaces/arch_registers.h"

#if !defined  (HSE_VALUE)
  #define HSE_VALUE    8000000U  /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (MSI_VALUE)
  #define MSI_VALUE    4000000U  /*!< Value of the Internal oscillator in Hz*/
#endif /* MSI_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    16000000U /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

// Added by silseva
#define HSE_STARTUP_TIMEOUT   ((uint16_t)0x0500)

/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_Defines
  * @{
  */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field.
                                   This value must be a multiple of 0x200. */
/******************************************************************************/
/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_Variables
  * @{
  */
  /* The SystemCoreClock variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
  
  // Added by silseva
  #ifdef SYSCLK_FREQ_HSE
  uint32_t SystemCoreClock         = SYSCLK_FREQ_HSE;        /*!< System Clock Frequency (Core Clock) */
  #elif defined SYSCLK_FREQ_24MHz
  uint32_t SystemCoreClock         = SYSCLK_FREQ_24MHz;        /*!< System Clock Frequency (Core Clock) */
  #elif defined SYSCLK_FREQ_36MHz
  uint32_t SystemCoreClock         = SYSCLK_FREQ_36MHz;        /*!< System Clock Frequency (Core Clock) */
  #elif defined SYSCLK_FREQ_48MHz
  uint32_t SystemCoreClock         = SYSCLK_FREQ_48MHz;        /*!< System Clock Frequency (Core Clock) */
  #elif defined SYSCLK_FREQ_56MHz
  uint32_t SystemCoreClock         = SYSCLK_FREQ_56MHz;        /*!< System Clock Frequency (Core Clock) */
  #elif defined SYSCLK_FREQ_80MHz
  uint32_t SystemCoreClock         = SYSCLK_FREQ_80MHz;        /*!< System Clock Frequency (Core Clock) */
  #else /*!< MSI Selected as System Clock source */
  uint32_t SystemCoreClock         = MSI_VALUE;        /*!< System Clock Frequency (Core Clock) */
  #endif
  
  //uint32_t SystemCoreClock = 4000000U;

  const uint8_t  AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
  const uint8_t  APBPrescTable[8] =  {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
  const uint32_t MSIRangeTable[12] = {100000U,   200000U,   400000U,   800000U,  1000000U,  2000000U, \
                                      4000000U, 8000000U, 16000000U, 24000000U, 32000000U, 48000000U};
/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_FunctionPrototypes
  * @{
  */

// Added by silseva
static void SetSysClock(void);

#ifdef SYSCLK_FREQ_HSE
  static void SetSysClockToHSE(void);
#elif defined SYSCLK_FREQ_24MHz
  static void SetSysClockTo24(void);
#elif defined SYSCLK_FREQ_36MHz
  static void SetSysClockTo36(void);
#elif defined SYSCLK_FREQ_48MHz
  static void SetSysClockTo48(void);
#elif defined SYSCLK_FREQ_56MHz
  static void SetSysClockTo56(void);
#elif defined SYSCLK_FREQ_80MHz
  static void SetSysClockTo80(void);
#endif

/**
  * @}
  */

/** @addtogroup STM32L4xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system.
  * @param  None
  * @retval None
  */

void SystemInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif

  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set MSION bit */
  RCC->CR |= RCC_CR_MSION;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000U;

  /* Reset HSEON, CSSON , HSION, and PLLON bits */
  RCC->CR &= 0xEAF6FFFFU;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x00001000U;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Disable all interrupts */
  RCC->CIER = 0x00000000U;

  /* By silseva:
   * Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers
   * Configure the Flash Latency cycles and enable prefetch buffer */
  SetSysClock();
  
  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is MSI, SystemCoreClock will contain the MSI_VALUE(*)
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
  *             or HSI_VALUE(*) or MSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (*) MSI_VALUE is a constant defined in stm32l4xx_hal.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *
  *         (**) HSI_VALUE is a constant defined in stm32l4xx_hal.h file (default value
  *              16 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  *
  *         (***) HSE_VALUE is a constant defined in stm32l4xx_hal.h file (default value
  *              8 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp = 0U, msirange = 0U, pllvco = 0U, pllr = 2U, pllsource = 0U, pllm = 2U;

  /* Get MSI Range frequency--------------------------------------------------*/
  if((RCC->CR & RCC_CR_MSIRGSEL) == RESET)
  { /* MSISRANGE from RCC_CSR applies */
    msirange = (RCC->CSR & RCC_CSR_MSISRANGE) >> 8U;
  }
  else
  { /* MSIRANGE from RCC_CR applies */
    msirange = (RCC->CR & RCC_CR_MSIRANGE) >> 4U;
  }
  /*MSI frequency range in HZ*/
  msirange = MSIRangeTable[msirange];

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR & RCC_CFGR_SWS)
  {
    case 0x00:  /* MSI used as system clock source */
      SystemCoreClock = msirange;
      break;

    case 0x04:  /* HSI used as system clock source */
      SystemCoreClock = HSI_VALUE;
      break;

    case 0x08:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;

    case 0x0C:  /* PLL used as system clock  source */
      /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI_VALUE/ PLLM) * PLLN
         SYSCLK = PLL_VCO / PLLR
         */
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
      pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> 4U) + 1U ;

      switch (pllsource)
      {
        case 0x02:  /* HSI used as PLL clock source */
          pllvco = (HSI_VALUE / pllm);
          break;

        case 0x03:  /* HSE used as PLL clock source */
          pllvco = (HSE_VALUE / pllm);
          break;

        default:    /* MSI used as PLL clock source */
          pllvco = (msirange / pllm);
          break;
      }
      pllvco = pllvco * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 8U);
      pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> 25U) + 1U) * 2U;
      SystemCoreClock = pllvco/pllr;
      break;

    default:
      SystemCoreClock = msirange;
      break;
  }
  /* Compute HCLK clock frequency --------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4U)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
}

// Added by silseva

#ifdef RUN_WITH_HSE

/**
 * @brief Activates HSE oscillator and waits until is ready
 * @param None
 * @retval 1 if HSE starts, 0 on failure
 */
static uint32_t EnableHSE()
{
    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;
  
  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
  /* Enable HSE */    
  RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
  /* Wait till HSE is ready and if Time out is reached exit */
  do
  {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;  
  } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != 0x0)
  {
    HSEStatus = (uint32_t)0x01;
  }
  else
  {
    HSEStatus = (uint32_t)0x00;
  }
  
  return HSEStatus;
}

#elif defined(RUN_WITH_HSI)

/**
 * @brief Activates HSI oscillator and waits until is ready
 * @param None
 * @retval 1 if HSI starts, 0 on failure
 */
static uint32_t EnableHSI()
{
    __IO uint32_t StartUpCounter = 0, HSIStatus = 0;
  
  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
  /* Enable HSE */    
  RCC->CR |= ((uint32_t)RCC_CR_HSION);
 
  /* Wait till HSE is ready and if Time out is reached exit */
  do
  {
    HSIStatus = RCC->CR & RCC_CR_HSIRDY;
    StartUpCounter++;  
  } while((HSIStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSIRDY) != 0x0)
  {
    HSIStatus = (uint32_t)0x01;
  }
  else
  {
    HSIStatus = (uint32_t)0x00;
  }
  
  return HSIStatus;
}

#endif //RUN_WITH_HSI


/**
  * @brief  Configures the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers.
  * @param  None
  * @retval None
  */
static void SetSysClock(void)
{
#ifdef SYSCLK_FREQ_HSE
  SetSysClockToHSE();
#elif defined SYSCLK_FREQ_24MHz
  SetSysClockTo24();
#elif defined SYSCLK_FREQ_36MHz
  SetSysClockTo36();
#elif defined SYSCLK_FREQ_48MHz
  SetSysClockTo48();
#elif defined SYSCLK_FREQ_56MHz
  SetSysClockTo56();  
#elif defined SYSCLK_FREQ_80MHz
  SetSysClockTo80();
#endif
 
 /* If none of the define above is enabled, the HSI is used as System clock
    source (default after reset) */ 
}


#ifdef SYSCLK_FREQ_HSE
/**
  * @brief  Selects HSE as System clock source and configure HCLK, PCLK2
  *         and PCLK1 prescalers.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockToHSE(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0;
  
  /* Enable HSE and wait till is ready and if Time out is reached exit */
  HSEStatus = EnableHSE();

  if (HSEStatus == (uint32_t)0x01)
  {

    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash 0 wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);

    /* See RM0394 at page 79 */
	if ((HSE_VALUE > 16000000) && (HSE_VALUE <= 32000000))
	{
      FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1WS; // one wait states
	}
	else if ((HSE_VALUE > 32000000) && (HSE_VALUE <= 48000000))
    {
      FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2WS; // two wait states
	}
    else if ((HSE_VALUE > 48000000) && (HSE_VALUE <= 64000000))
    {
      FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_3WS  // three wait states
	}
    else if ((HSE_VALUE > 64000000) && (HSE_VALUE <= 80000000))
    {
      FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_4WS; // four wait states
	}
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /* Select HSE as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSE;    

    /* Wait till HSE is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  }  
}
#elif defined SYSCLK_FREQ_24MHz
/**
  * @brief  Sets System clock frequency to 24MHz and configure HCLK, PCLK2 
  *         and PCLK1 prescalers.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockTo24(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0x01; //By TFT: was 0
  (void)StartUpCounter;

  #ifdef RUN_WITH_HSE //By silseva
  HSEStatus = EnableHSE();
  #elif defined(RUN_WITH_HSI)
  HSEStatus = EnableHSI();
  #endif //RUN_WITH_HSI

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash one wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1WS;
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /*
     * PLL configuration:
     *
     * - Fpll = (Fin/M)*N
     * - System clock: Fpll/R
     * - 48MHz clock:  Fpll/Q
     * 
     * To go @24MHz we set:
     * 
     * - Fpll = 96MHz
     * - R = 4
     * - Q = 2
     * - N = 24 -> PLL input frequency has to be 4MHz
     */

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (uint32_t)(24 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= (uint32_t)RCC_PLLCFGR_PLLR_0;   /* PLLR = 10 -> divide by four */
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLQEN | RCC_PLLCFGR_PLLREN;

    #ifdef RUN_WITH_HSE
    /* 8MHz HSE -> M = 2 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSE);
    #elif defined RUN_WITH_HSI
    /* 16MHz HSI -> M = 4 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_1)
                 |  (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSI);
    #else
    /* 4MHz MSI -> M = 1 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_MSI);
    #endif

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  } 
}
#elif defined SYSCLK_FREQ_36MHz
/**
  * @brief  Sets System clock frequency to 36MHz and configure HCLK, PCLK2 
  *         and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockTo36(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0x01; //By TFT: was 0
  (void)StartUpCounter;
  
  #ifdef RUN_WITH_HSE //By silseva
  HSEStatus = EnableHSE();
  #elif defined RUN_WITH_HSI
  HSEStatus = EnableHSI();
  #endif //RUN_WITH_HSI

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash two wait states */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2WS;
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /*
     * PLL configuration:
     *
     * - Fpll = (Fin/M)*N
     * - System clock: Fpll/R
     * - 48MHz clock:  Fpll/Q
     * 
     * To go @36MHz we set:
     * 
     * - Fpll = 72MHz
     * - R = 2
     * - Q -> 48MHz clock shut down
     * - N = 18 -> PLL input frequency has to be 4MHz
     */

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (uint32_t)(18 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

    #ifdef RUN_WITH_HSE
    /* 8MHz HSE -> M = 2 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSE);
    #elif defined RUN_WITH_HSI
    /* 16MHz HSI -> M = 4 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_1)
                 |  (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSI);
    #else
    /* 4MHz MSI -> M = 1 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_MSI);
    #endif

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  } 
}
#elif defined SYSCLK_FREQ_48MHz
/**
  * @brief  Sets System clock frequency to 48MHz and configure HCLK, PCLK2 
  *         and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockTo48(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0x01; //By TFT: was 0
  (void)StartUpCounter;
  
  #ifdef RUN_WITH_HSE //By silseva
  HSEStatus = EnableHSE();
  #elif defined RUN_WITH_HSI
  HSEStatus = EnableHSI();
  #endif //RUN_WITH_HSI

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash two wait states */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2WS;
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /*
     * PLL configuration:
     *
     * - Fpll = (Fin/M)*N
     * - System clock: Fpll/R
     * - 48MHz clock:  Fpll/Q
     * 
     * To go @48MHz we set:
     * 
     * - Fpll = 96MHz
     * - R = 2
     * - Q = 2
     * - N = 24 -> PLL input frequency has to be 4MHz
     */

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (uint32_t)(24 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLQEN | RCC_PLLCFGR_PLLREN;

    #ifdef RUN_WITH_HSE
    /* 8MHz HSE -> M = 2 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSE);
    #elif defined RUN_WITH_HSI
    /* 16MHz HSI -> M = 4 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_1)
                 |  (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSI);
    #else
    /* 4MHz MSI -> M = 1 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_MSI);
    #endif

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  } 
}

#elif defined SYSCLK_FREQ_56MHz
/**
  * @brief  Sets System clock frequency to 56MHz and configure HCLK, PCLK2 
  *         and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockTo56(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0x01; //By TFT: was 0
  (void)StartUpCounter;
  
  #ifdef RUN_WITH_HSE //By silseva
  HSEStatus = EnableHSE();
  #elif defined RUN_WITH_HSI
  HSEStatus = EnableHSI();
  #endif //RUN_WITH_HSI

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash three wait states */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_3WS;
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /*
     * PLL configuration:
     *
     * - Fpll = (Fin/M)*N
     * - System clock: Fpll/R
     * - 48MHz clock:  Fpll/Q
     * 
     * To go @56MHz we set:
     * 
     * - Fpll = 112MHz
     * - R = 2
     * - Q -> 48MHz clock cannot be used
     * - N = 28 -> PLL input frequency has to be 4MHz
     */

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (uint32_t)(28 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

    #ifdef RUN_WITH_HSE
    /* 8MHz HSE -> M = 2 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSE);
    #elif defined RUN_WITH_HSI
    /* 16MHz HSI -> M = 4 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_1)
                 |  (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSI);
    #else
    /* 4MHz MSI -> M = 1 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_MSI);
    #endif

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  } 
}

#elif defined SYSCLK_FREQ_80MHz
/**
  * @brief  Sets System clock frequency to 80MHz and configure HCLK, PCLK2 
  *         and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
static void SetSysClockTo80(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0x01; //By TFT: was 0
  (void)StartUpCounter;
  
  #ifdef RUN_WITH_HSE //By silseva
  HSEStatus = EnableHSE();
  #elif defined RUN_WITH_HSI
  HSEStatus = EnableHSI();
  #endif //RUN_WITH_HSI

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    /* Flash four wait states */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_4WS;
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
    /*
     * PLL configuration:
     *
     * - Fpll = (Fin/M)*N
     * - System clock: Fpll/R
     * - 48MHz clock:  Fpll/Q
     * 
     * To go @80MHz we set:
     * 
     * - Fpll = 160MHz
     * - R = 2
     * - Q -> 48MHz clock cannot be used
     * - N = 40 -> PLL input frequency has to be 4MHz
     */

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (uint32_t)(40 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

    #ifdef RUN_WITH_HSE
    /* 8MHz HSE -> M = 2 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSE);
    #elif defined RUN_WITH_HSI
    /* 16MHz HSI -> M = 4 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLM_1)
                 |  (uint32_t)(RCC_PLLCFGR_PLLM_0);
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_HSI);
    #else
    /* 4MHz MSI -> M = 1 */
    RCC->PLLCFGR |= (uint32_t)(RCC_PLLCFGR_PLLSRC_MSI);
    #endif

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }
    
    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
  }
}
#endif

// end addition

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
