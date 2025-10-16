#ifndef GPIO_NATIVE_H
#define GPIO_NATIVE_H

#if defined(STM32F405xx)
#include "stm32f4xx.h"
#include "drivers/GPIO/gpio_stm32.h"
#elif defined(STM32H743xx)
#include "stm32h7xx.h"
#include "drivers/GPIO/gpio_stm32.h"
#elif defined(MK22FN512xx)
#include "drivers/GPIO/gpio_mk22.h"
#endif

#endif /* GPIO_NATIVE_H */
