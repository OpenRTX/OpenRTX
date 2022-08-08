#include <stm32f4xx.h>
#include <kernel/scheduler/scheduler.h>
#include <miosix.h>
#include <hwconfig.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <interfaces/radio.h>
#include <stdint.h>
#include "HR_C5000.h"

extern HR_C5000& C5000;

using namespace miosix;

static Thread *rxWaiting = 0;
static Thread *txWaiting = 0;

/** BEGIN DMR STUFFS **/

void TS_Interrupt_Impl();
void SYS_Interrupt_Impl();
void RF_TX_Interrupt_Impl();
void RF_RX_Interrupt_Impl();

void radio_initVocoder() {
    radio_DMRInterruptsInit();

    // GPIO setup, use alternate function for SPI
    RCC->APB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER|=GPIO_MODER_MODE13_1|GPIO_MODER_MODE14_1|GPIO_MODER_MODE15_1; 
    GPIOB->AFR[1]|=(0x05<<20)|(0x05<<24)|(0x05<<28);
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    __DSB();

    gpio_setMode(V_CS, OUTPUT);
    gpio_setPin(V_CS); // Set CS high/idle

    // Begin setup SPI
    SPI2->CR1 = SPI_CR1_SSM 
              | SPI_CR1_SSI    /* Fclock: 42MHz/32 = 1.3MHz */
              | SPI_CR1_CPHA    /* Fclock: 42MHz/32 = 1.3MHz */
              | SPI_CR1_BR_2    /* Fclock: 42MHz/32 = 1.3MHz */
              | SPI_CR1_MSTR    /* Master mode               */
              | SPI_CR1_SPE;    /* Enable peripheral         */

    // Enable DMA
    SPI2->CR2 = SPI_CR2_TXDMAEN
              | SPI_CR2_RXDMAEN;

    // Finally enable SPI
    SPI2->CR1 |= SPI_CR1_SPE;

    // Begin DMA setup
    // DMA1_Stream3, Channel 0 is SPI2_RX
    // DMA1_Stream4, Channel 0 is SPI2_TX
    RCC->APB1ENR |= RCC_AHB1ENR_DMA1EN;
    __DSB();

    // Make sure not to interrupt any other streams
    DMA1_Stream3->CR &= ~DMA_SxCR_EN;
    while((DMA1_Stream3->CR)&DMA_SxCR_EN){;}
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while((DMA1_Stream4->CR)&DMA_SxCR_EN){;}

    DMA1_Stream3->CR = (0x00<<25)     // Select channel 0
                     | DMA_SxCR_MINC  // Increment memory
                     | DMA_SxCR_DIR_0 // Periph->memory
                     | DMA_SxCR_PL_1  // High priority
                     | DMA_SxCR_TCIE   // Interrupt when buffer full
                     | DMA_SxCR_TEIE;  // Interrupt when transfer error
    DMA1_Stream3->PAR = reinterpret_cast< uint32_t >(&SPI2->DR);

    DMA1_Stream4->CR = (0x00<<25)     // Select channel 0
                     | DMA_SxCR_MINC  // Increment memory
                     | (DMA_SxCR_DIR_0|DMA_SxCR_DIR_1) // memory->Periph
                     | DMA_SxCR_PL    // Very high priority
                     | DMA_SxCR_TCIE  // Interrupt when buffer full
                     | DMA_SxCR_TEIE;  // Interrupt when buffer full
    DMA1_Stream4->PAR = reinterpret_cast< uint32_t >(&SPI2->DR);

    // Enable DMA interrupts
    NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);
    NVIC_SetPriority(DMA1_Stream3_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
    NVIC_SetPriority(DMA1_Stream4_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

void radio_sendVocoder(uint8_t* buf, size_t len) {
    delayUs(2);
    gpio_clearPin(V_CS);
    delayUs(2);

    uint8_t cmdTxBuf[32+2];
    cmdTxBuf[0] = 0x03;
    cmdTxBuf[1] = 0x00;

    // Loop over txBuf and copy it to cmdTxBuf starting at index 2
    for (size_t i = 0; i < len; i++) {
        cmdTxBuf[i + 2] = buf[i];
    }

    DMA1_Stream4->M0AR = reinterpret_cast< uint32_t >(cmdTxBuf);
    DMA1_Stream4->NDTR = 32+2;
    DMA1_Stream4->PAR = reinterpret_cast< uint32_t >(&SPI2->DR);

    DMA1_Stream4->CR |= DMA_SxCR_EN;

    // TX first
    {
        FastInterruptDisableLock dLock;
        txWaiting = Thread::IRQgetCurrentThread();
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }

        }while(txWaiting);
    }

    gpio_setPin(V_CS);
}

void radio_readVocoder(uint8_t* buf, size_t len) {
    delayUs(2);
    gpio_clearPin(V_CS);
    delayUs(2);

    uint8_t rxBuf[32 + 2];
    uint8_t txBuf[32 + 2];

    txBuf[0] = 0x03 | 0x80;
    txBuf[1] = 0x00;

    DMA1_Stream3->M0AR = reinterpret_cast< uint32_t >(rxBuf);
    DMA1_Stream3->NDTR = 32+2;
    DMA1_Stream3->PAR = reinterpret_cast< uint32_t >(&SPI2->DR);

    DMA1_Stream4->M0AR = reinterpret_cast< uint32_t >(txBuf);
    DMA1_Stream4->NDTR = 32+2;
    DMA1_Stream4->PAR = reinterpret_cast< uint32_t >(&SPI2->DR);

    DMA1_Stream3->CR |= DMA_SxCR_EN;
    DMA1_Stream4->CR |= DMA_SxCR_EN;

    // TX first
    {
        FastInterruptDisableLock dLock;
        txWaiting = Thread::IRQgetCurrentThread();
        rxWaiting = Thread::IRQgetCurrentThread();
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }

        }while(txWaiting);
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }

        }while(rxWaiting);
    }

    for (int i = 0; i < len; i++)
    {
        buf[i] = rxBuf[i + 2];
    }
    gpio_setPin(V_CS);
}

void __attribute__((used)) SPI2_RX_Handler() {
    DMA1->LIFCR =  DMA_LIFCR_CTCIF2
                |  DMA_LIFCR_CHTIF2
                |  DMA_LIFCR_CTEIF2;

    NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);

    // Wake up the thread
    if(rxWaiting != 0)
    {
        rxWaiting->IRQwakeup();
        Priority prio = rxWaiting->IRQgetPriority();
        if(prio > Thread::IRQgetCurrentThread()->IRQgetPriority())
            Scheduler::IRQfindNextThread();
        rxWaiting = 0;
    }

    NVIC_SetPriority(DMA1_Stream3_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}

void __attribute__((naked)) DMA1_Stream3_IRQHandler() {
    saveContext();
    asm volatile("bl _Z15SPI2_RX_Handlerv");
    restoreContext();
}

void __attribute__((used)) SPI2_TX_Handler() {
    // Clear interrupt flags
    DMA1->LIFCR =  DMA_LIFCR_CTCIF2
                |  DMA_LIFCR_CHTIF2
                |  DMA_LIFCR_CTEIF2;

    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);

    // Wake up the thread
    if(txWaiting != 0)
    {
        txWaiting->IRQwakeup();
        Priority prio = txWaiting->IRQgetPriority();
        if(prio > Thread::IRQgetCurrentThread()->IRQgetPriority())
            Scheduler::IRQfindNextThread();
        txWaiting = 0;
    }
    NVIC_SetPriority(DMA1_Stream4_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

void __attribute__((naked)) DMA1_Stream4_IRQHandler() {
    saveContext();
    asm volatile("bl _Z15SPI2_TX_Handlerv");
    restoreContext();
}

void radio_stopVocoder() {
    radio_DMRInterruptsStop();
    // Disable DMA (Vocoder SPI)
    DMA1_Stream3->CR &= ~DMA_SxCR_EN;
    while((DMA1_Stream3->CR)&DMA_SxCR_EN){;}
    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while((DMA1_Stream4->CR)&DMA_SxCR_EN){;}

    NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    NVIC_DisableIRQ(DMA1_Stream4_IRQn);

    // Disable SPI
    SPI2->CR1 &= ~SPI_CR1_SPE;
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
    __DSB();

    // Critical section: invalidate (partial) data, wake up
    // thread.
    {
        FastInterruptDisableLock dLock;
        if(txWaiting != 0) txWaiting->IRQwakeup();
        if(rxWaiting != 0) rxWaiting->IRQwakeup();
    }
}

bool radio_isDMRLocked() {
    // ??
    return false;
}

void radio_DMRInterruptsInit()
{
    gpio_setMode(TS_INTER,   INPUT_PULL_UP);
    gpio_setMode(TX_INTER,   INPUT_PULL_UP);
    gpio_setMode(RX_INTER,   INPUT_PULL_UP);
    gpio_setMode(SYS_INTER,  INPUT_PULL_UP);

    /*
     * Configure GPIO interrupts for DMR
     */
    EXTI->IMR  |= EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3;
    EXTI->RTSR |= EXTI_RTSR_TR1 | EXTI_RTSR_TR2 | EXTI_IMR_MR3;
    EXTI->FTSR |= EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | EXTI_IMR_MR3;

    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR1_EXTI0_PC;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR1_EXTI1_PC;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR1_EXTI2_PC;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR1_EXTI3_PC;

    // TS interupt
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI0_IRQn, 10);
    NVIC_EnableIRQ(EXTI0_IRQn);

    // sys interupt
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    NVIC_SetPriority(EXTI1_IRQn, 10);
    NVIC_EnableIRQ(EXTI1_IRQn);

    // RF TX interupt
    NVIC_ClearPendingIRQ(EXTI2_IRQn);
    NVIC_SetPriority(EXTI2_IRQn, 10);
    NVIC_EnableIRQ(EXTI2_IRQn);

    // RF RX interupt
    NVIC_ClearPendingIRQ(EXTI3_IRQn);
    NVIC_SetPriority(EXTI3_IRQn, 10);
    NVIC_EnableIRQ(EXTI3_IRQn);
}

void radio_DMRInterruptsStop()
{
    EXTI->IMR &= ~(EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3);
    // TS interupt
    NVIC_DisableIRQ(EXTI0_IRQn);
    // sys interupt
    NVIC_DisableIRQ(EXTI1_IRQn);
    // RF TX interupt
    NVIC_DisableIRQ(EXTI2_IRQn);
    // RF RX interupt
    NVIC_DisableIRQ(EXTI3_IRQn);
}

void __attribute__((used)) TS_Interrupt_Impl() {
    uint8_t reg0x52 = C5000.readCfgRegister(0x52);
    uint8_t receivedTimeCode = (reg0x52 >> 2) & 1;
    if (receivedTimeCode == 1) {
        
    } else {
        
    }
    C5000.writeCfgRegister(0x41, 0x50);

    EXTI->PR |= EXTI_PR_PR0;
}

void __attribute__((naked)) EXTI0_IRQHandler() {
    saveContext();
    asm volatile("bl _Z17TS_Interrupt_Implv");
    restoreContext();
}

void rxData() {
    uint8_t reg0x51 = C5000.readCfgRegister(0x51);
    uint8_t rxDataType = (reg0x51 >> 4) & 0x0f;
    uint8_t rxSyncClass = (reg0x51 >> 0) & 0x03;
    uint8_t rxCRCisValid = (((reg0x51 >> 2) & 0x01) == 0);
    uint8_t rpi = (reg0x51 >> 3) & 0x01;

    uint8_t reg0x5F = C5000.readCfgRegister(0x5f);
    uint8_t rxSyncType = reg0x5F & 0x03;
    radio_initVocoder();

    if (rxSyncType==BS_SYNC)                                           // if we are receiving from a base station (Repeater)
    {
        //trxDMRModeRx = DMR_MODE_RMO;                               // switch to RMO mode to allow reception
    }
    else
    {
        //trxDMRModeRx = DMR_MODE_DMO;								   // not base station so must be DMO
    }

    if (rpi == 0 && rxCRCisValid) {
        uint8_t rx[27];
        radio_readVocoder(rx, 27);
        gpio_setPin(RED_LED);
    } else {
        // Something went wrong
    }
}

void __attribute__((used)) SYS_Interrupt_Impl() {
    // uint8_t reg0x52 = readReg(M::CONFIG, 0x52);
    // uint8_t reg0x51 = readReg(M::CONFIG, 0x51);
    uint8_t reg0x82 = C5000.readCfgRegister(0x82);
    // int rxColorCode = (reg0x52 >> 4) & 0b1111;

    // if (!transmitting) {
    //     uint8_t LCBuf[12];
    //     readSequence(M::DATA, 0x00, LCBuf, 12); // read the LC from the C5000
    //     bool crc = (((reg0x51 >> 2) & 1) == 0); // CRC is OK if its 0

    //     if (crc && ((LCBuf[0] == TG_CALL_FLAG) || (LCBuf[0] == PC_CALL_FLAG) || ((LCBuf[0] >= FLCO_TALKER_ALIAS_HEADER) && (LCBuf[0] <= FLCO_GPS_INFO))) &&
    //         (memcmp(previousLCBuf, LCBuf, 12) != 0))
    //     {
    //         if (dmrRXMode == DMR_MODE_DMO) // only do this for the selected timeslot, or when in Active mode
    //         {
    //             if ((LCBuf[0] == TG_CALL_FLAG) || (LCBuf[0] == PC_CALL_FLAG))
    //             {
    //                 int receivedTgOrPcId 	= (LCBuf[0] << 24) + (LCBuf[3] << 16) + (LCBuf[4] << 8) + (LCBuf[5] << 0);// used by the call accept filter
    //                 int receivedSrcId 		= (LCBuf[6] << 16) + (LCBuf[7] << 8) + (LCBuf[8] << 0);// used by the call accept filter

    //                 if ((receivedTgOrPcId != 0) && (receivedSrcId != 0)) // only store the data if its actually valid
    //                 {
    //                     // printf("receivedTgOrPcId=%d\n", receivedTgOrPcId);
    //                     // printf("receivedSrcId=%d\n", receivedSrcId);
    //                 }
    //             }
    //             else
    //             {
    //             }
    //         }
    //     }
    //     memcpy(previousLCBuf, LCBuf, 12);
    // }

    // // reg0x82 has the following bits:
    // // MSB: Send request rejected
    // // MSB-1: Start sending
    // // MSB-2: End of delivery
    // // MSB-3: Post-access
    // // MSB-4: Receive data
    // // MSB-5: BB
    // // MSB-6: Abnormal exit
    // // LSB: Physical layer work interrupted

    // // MSB-1, MSB-2, MSB-4, and MSB-5 events need a behavior change

    // // MSB: Send request rejected
    // if ((reg0x82 >> 7) & 1) {
    //     // Do nothing for now, eventually want to inform user
    // }

    // // MSB-1: Start sending
    // if ((reg0x82 >> 6) & 1) {
    //     uint8_t reg0x84 = readReg(M::CONFIG, 0x84);
    //     // Check MSB to see if we should start sending audio
    // }

    // // MSB-2: End of delivery
    // if ((reg0x82 >> 5) & 1) {
    //     uint8_t reg0x84 = readReg(M::CONFIG, 0x84);
    //     // Check MSB-1 to see if we should stop sending audio
    // }

    // // MSB-3: Post-access
    // if ((reg0x82 >> 4) & 1) {
    //     // What is this?
    //     // Late entry into ongoing RX
    // }

    // MSB-4: Receive data
    if ((reg0x82 >> 3) & 1) {
        rxData();
    }

    // // MSB-5: Receive info
    // if ((reg0x82 >> 2) & 1) {
    //     uint8_t reg0x90 = readReg(M::CONFIG, 0x90);
    //     // MSB is CRC check
    //     // MSB-1 is SMS error
    // }

    // // MSB-6: Abnormal exit
    // if ((reg0x82 >> 1) & 1) {
    //     uint8_t reg0x98 = readReg(M::CONFIG, 0x98);
    //     // MSB:MSB-2 is speech abnormal interrupt mask
    //     // MSB-5 TS1 sync lost
    //     // MSB-6 TS2 sync lost
    //     // LSB is abnormal interrupt?
    // }

    // // LSB: Physical layer work interrupted
    // if (reg0x82 & 1) {
    //     // Do nothing for now, eventually want to inform user
    // }

    C5000.writeCfgRegister(0x83, reg0x82);
    EXTI->PR |= EXTI_PR_PR1;
}

void __attribute__((naked)) EXTI1_IRQHandler() {
    saveContext();
    asm volatile("bl _Z18SYS_Interrupt_Implv");
    restoreContext();
}

void __attribute__((used)) RF_TX_Interrupt_Impl() {
    EXTI->PR |= EXTI_PR_PR2;
}

void __attribute__((naked)) EXTI2_IRQHandler() {
    saveContext();
    asm volatile("bl _Z20RF_TX_Interrupt_Implv");
    restoreContext();
}

void __attribute__((used)) RF_RX_Interrupt_Impl() {
    EXTI->PR |= EXTI_PR_PR3;
}

void __attribute__((naked)) EXTI3_IRQHandler() {
    saveContext();
    asm volatile("bl _Z20RF_RX_Interrupt_Implv");
    restoreContext();
}
