/**
  ******************************************************************************
  * @file    usb_bsp.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file is responsible to offer board support package and is 
  *          configurable by user.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usb_bsp.h"
#include "usbd_conf.h"
#include "stm32f4xx.h"
#include "gpio.h"

extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);

/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
* @retval None
*/

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
    gpio_setMode(GPIOA, 11, ALTERNATE);
    gpio_setAlternateFunction(GPIOA, 11, 10);
    gpio_setOutputSpeed(GPIOA, 11, HIGH);      // 100MHz output speed

    gpio_setMode(GPIOA, 12, ALTERNATE);
    gpio_setAlternateFunction(GPIOA, 12, 10);
    gpio_setOutputSpeed(GPIOA, 12, HIGH);      // 100MHz output speed

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
}

/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global interrupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
    NVIC_ClearPendingIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 14);
    NVIC_EnableIRQ(OTG_FS_IRQn);
}
/**
* @brief  USB_OTG_BSP_uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
* @retval None
*/
void USB_OTG_BSP_uDelay (const uint32_t usec) {

    uint32_t count = 0;
    const uint32_t utime = (120 * usec / 7);

    do
    {
        if( ++count > utime )
        {
            return ;
        }
    } while (1);
}


/**
* @brief  USB_OTG_BSP_mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void USB_OTG_BSP_mDelay (const uint32_t msec)
{
    USB_OTG_BSP_uDelay(msec * 1000);
}

void OTG_FS_IRQHandler(void)
{
    USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

