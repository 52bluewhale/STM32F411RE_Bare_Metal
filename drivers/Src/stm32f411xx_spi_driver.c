#include "stm32f411xx_spi_driver.h"


/* ==========================SPI_IRQHandling helper function declaration========================== */
static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

/* ==========================Peripheral clock setup========================== */
/*
 * @fn      - SPI_PeriClockControl
 *
 * @brief   - Enable or disable peripheral clock for the given SPI peripheral
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - EnorDi: ENABLE or DISABLE macros
 *
 * @return  - none
 *
 * @note    - Must be called before initializing the SPI peripheral
 */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pSPIx == SPI1)
        {
            SPI1_PCLK_EN();
        }
        else if (pSPIx == SPI2)
        {
            SPI2_PCLK_EN();
        }
        else if (pSPIx == SPI3)
        {
            SPI3_PCLK_EN();
        }
        else if (pSPIx == SPI4)
        {
            SPI4_PCLK_EN();
        }
        else if (pSPIx == SPI5)
        {
            SPI5_PCLK_EN();
        }
    }
    else
    {
        if (pSPIx == SPI1)
        {
            SPI1_PCLK_DI();
        }
        else if (pSPIx == SPI2)
        {
            SPI2_PCLK_DI();
        }
        else if (pSPIx == SPI3)
        {
            SPI3_PCLK_DI();
        }
        else if (pSPIx == SPI4)
        {
            SPI4_PCLK_DI();
        }
        else if (pSPIx == SPI5)
        {
            SPI5_PCLK_DI();
        }
    }
}

/* ==========================Init and De-Init========================== */
/*
 * @fn      - SPI_Init
 *
 * @brief   - Initialize the SPI peripheral with the configuration specified
 *            in the handle structure
 *
 * @param   - pSPIHandle: pointer to SPI handle structure containing base
 *            address and configuration settings
 *
 * @return  - none
 *
 * @note    - This function configures CR1 register based on user settings.
 *            SPE bit should be set separately using SPI_PeripheralControl()
 */
void SPI_Init(SPI_Handle_t *pSPIHandle)
{
    // Enable peripheral clock
    SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

    // Configure CR1 register
    uint32_t tempreg = 0;

    // 1. Configure the device mode
    tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

    // 2. Configure the bus config
    if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
    {
        // Bi-Di mode should be cleared
        tempreg &= ~(1 << SPI_CR1_BIDIMODE);
    }
    else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
    {
        // Bi-Di mode should be set
        tempreg |= (1 << SPI_CR1_BIDIMODE);
    }
    else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
    {
        // Bi-Di mode should be cleared
        tempreg &= ~(1 << SPI_CR1_BIDIMODE);

        // RXONLY bit must be set
        tempreg |= (1 << SPI_CR1_RXONLY);
    }

    // 3. Configure the SPI serial clock speed (BaudRate)
    tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;

    // 4. Configure the DFF
    tempreg |= pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF;

    // 5. Configure the CPOL
    tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;

    // 6. Configure the CPHA
    tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;

    pSPIHandle->pSPIx->CR1 = tempreg;
}

/*
 * @fn      - SPI_DeInit
 *
 * @brief   - Reset the SPI peripheral registers to their reset state.
 *
 * @param   - pSPIx: base address of the SPI peripheral to be reset
 *
 * @return  - none
 *
 * @note    - This issues a reset pulse via the RCC APBx peripheral reset
 *            register macros (SPIx_PCLK_RESET()). Other peripherals are
 *            unaffected because each macro toggles only the target bit.
 */
void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
    if (pSPIx == SPI1)
    {
        SPI1_PCLK_RESET();
    }
    else if (pSPIx == SPI2)
    {
        SPI2_PCLK_RESET();
    }
    else if (pSPIx == SPI3)
    {
        SPI3_PCLK_RESET();
    }
    else if (pSPIx == SPI4)
    {
        SPI4_PCLK_RESET();
    }
    else if (pSPIx == SPI5)
    {
        SPI5_PCLK_RESET();
    }
}

/* Get flag status */
/*
 * @fn      - SPI_GetFlagStatus
 *
 * @brief   - Check if a specific flag in the SPI status register is set
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - FlagName: flag to check (use SPI_SR_xxx_FLAG macros)
 *
 * @return  - FLAG_SET (1) if flag is set, FLAG_RESET (0) if flag is not set
 *
 * @note    - Useful for polling-based operations to check TXE, RXNE, BSY, etc.
 */
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
    if (pSPIx->SR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}

/* ==========================Polling Data send and receive========================== */
/*
 * @fn      - SPI_SendData
 *
 * @brief   - Send data over SPI using polling method (blocking)
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - pTxBuffer: pointer to transmit buffer
 * @param   - Len: number of bytes to transmit
 *
 * @return  - none
 *
 * @note    - This is a blocking API. Function will wait until TXE flag is set
 *            before writing to DR. For non-blocking operation use interrupt mode.
 */
void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
    while (Len > 0)
    {
        // 1. Wait for TXE is set
        while (SPI_GetFlagStatus(pSPIx, SPI_SR_TXE_FLAG) == FLAG_RESET);

        // 2. Check the DFF bit in CR1
        if (pSPIx->CR1 & (1 << SPI_CR1_DFF))
        {
            /* 16-bit DFF handle */
            // 1. Load the data into the DR
            pSPIx->DR = *((uint16_t*)pTxBuffer);
            Len--;
            Len--;
            (uint16_t*)pTxBuffer++;
        }
        else
        {
            /* 8-bit DFF handle */
            // 1. Load the data into the DR
            pSPIx->DR = *(pTxBuffer);
            Len--;
            pTxBuffer++;
        }
    }
}

/*
 * @fn      - SPI_ReceiveData
 *
 * @brief   - Receive data over SPI using polling method (blocking)
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - pRxBuffer: pointer to receive buffer
 * @param   - Len: number of bytes to receive
 *
 * @return  - none
 *
 * @note    - This is a blocking API. Function will wait until RXNE flag is set
 *            before reading from DR. For non-blocking operation use interrupt mode.
 *            FIXED: removed stray 'c' character that was causing compilation error
 */
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
    while (Len > 0)
    {
        // 1. Wait for RXE is set
        while (SPI_GetFlagStatus(pSPIx, SPI_SR_RXNE_FLAG) == FLAG_RESET);

        // 2. Check the DFF bit in CR1
        if (pSPIx->CR1 & (1 << SPI_CR1_DFF))
        {
            /* 16-bit DFF handle */
            // 1. Load the data from DR to RxBuffer
            *((uint16_t*)pRxBuffer) = pSPIx->DR;
            Len--;
            Len--;
            (uint16_t*)pRxBuffer++;
        }
        else
        {
            /* 8-bit DFF hanlde */
            // 1. Load the data into the DR
            *(pRxBuffer) = pSPIx->DR;
            Len--;
            pRxBuffer++;
        }
    }
}

/* ==========================Interrupt Data send and receive========================== */
/*
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    - SPI 
*/
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
    uint8_t state = pSPIHandle->TxState;

    if (state != SPI_BUSY_IN_TX)
    {
        // 1. Save the TxBuffer address and Len information in some global variable
        pSPIHandle->pTxBuffer = pTxBuffer;
        pSPIHandle->TxLen = Len;

        // 2. Mark the SPI state as busy in transmission so that no other code can 
        // take over same SPI peripheral until transmission is over
        pSPIHandle->TxState = SPI_BUSY_IN_TX;

        // 3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
        pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_TXEIE);
        // 4. Data transmission will be handled by the ISR code (implemented later)
    }

    return state;
}

/*
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    - SPI 
*/
uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    uint8_t state = pSPIHandle->RxState;

    if (state != SPI_BUSY_IN_RX)
    {
        // 1. Save the TxBuffer address and Len information in some global variable
        pSPIHandle->pRxBuffer = pRxBuffer;
        pSPIHandle->TxLen = Len;

        // 2. Mark the SPI state as busy in transmission so that no other code can 
        // take over same SPI peripheral until transmission is over
        pSPIHandle->TxState = SPI_BUSY_IN_RX;

        // 3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
        pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_RXNEIE);
        // 4. Data transmission will be handled by the ISR code (implemented later)
    }

    return state;
}

/* ==========================IRQ configuration and ISR handling========================== */
/*
 * @fn      - SPI_IRQInterruptConfig
 *
 * @brief   - Enable or disable SPI interrupt in the NVIC
 *
 * @param   - IRQNumber: IRQ number of the SPI peripheral (use IRQ_SPIx macros)
 * @param   - EnorDi: ENABLE or DISABLE
 *
 * @return  - none
 *
 * @note    - This function configures the NVIC ISER/ICER registers to enable
 *            or disable the interrupt. The IRQ number determines which NVIC
 *            register and bit position to configure.
 *            
 *            For STM32F411:
 *            - IRQ_SPI1 = 35  (in ISER1, bit 3)
 *            - IRQ_SPI2 = 36  (in ISER1, bit 4)
 *            - IRQ_SPI3 = 51  (in ISER1, bit 19)
 *            - IRQ_SPI4 = 84  (in ISER2, bit 20)
 *            - IRQ_SPI5 = 85  (in ISER2, bit 21)
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (IRQNumber <= 31)
        {
            // Program ISER0 register (IRQ 0-31)
            *NVIC_ISER0 |= (1U << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ISER1 register (IRQ 32-63)
            *NVIC_ISER1 |= (1U << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ISER2 register (IRQ 64-95)
            *NVIC_ISER2 |= (1U << (IRQNumber % 64));
        }
    }
    else 
    {
        if (IRQNumber <= 31)
        {
            // Program ICER0 register (IRQ 0-31)
            *NVIC_ICER0 |= (1U << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ICER1 register (IRQ 32-63)
            *NVIC_ICER1 |= (1U << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ICER2 register (IRQ 64-95)
            *NVIC_ICER2 |= (1U << (IRQNumber % 64));
        }
    }
}

/*
 * @fn      - SPI_IRQPriorityConfig
 *
 * @brief   - Configure the priority for SPI interrupt in the NVIC
 *
 * @param   - IRQNumber: IRQ number of the SPI peripheral (use IRQ_SPIx macros)
 * @param   - IRQPriority: priority value (0-15, where 0 is highest priority)
 *
 * @return  - none
 *
 * @note    - STM32F411 implements only the upper 4 bits of the 8-bit priority
 *            field. This means:
 *            - Valid priority values: 0-15
 *            - Priority 0 is the highest
 *            - Priority 15 is the lowest
 *            
 *            The priority value is shifted left by 4 bits to align with the
 *            implemented bits in the NVIC priority register.
 *            
 *            Each IPR register holds priorities for 4 IRQ numbers:
 *            - IPR[0] = IRQ 0-3
 *            - IPR[1] = IRQ 4-7
 *            - etc.
 */
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
    // 1. Find the correct IPR register 
    uint8_t iprx = IRQNumber / 4;

    // 2. Find the section within the IPR register
    uint8_t iprx_section = IRQNumber % 4;

    // 3. Calculate the shift amount 
    // Each section is 8 bits, but only upper 4 bits are implemented
    uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

    // 4. Clear the priority bits first, then set new priority
    *(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFF << (8 * iprx_section));       // Clear section
    *(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);       // Set priority
}

/*
 * @fn      - SPI_IRQHandling
 *
 * @brief   - Handle SPI interrupt by checking and clearing appropriate flags
 *
 * @param   - pHandle: pointer to SPI handle structure
 *
 * @return  - none
 *
 * @note    - This function should be called from the actual IRQ handler
 *            (e.g., SPI1_IRQHandler). It checks which interrupt occurred
 *            (TXE, RXNE, or Error) and handles it appropriately.
 *            
 *            Current implementation handles basic error flags by clearing them.
 *            For full interrupt-driven TX/RX, additional state variables would
 *            be needed in the SPI_Handle_t structure.
 *            
 *            SPI Interrupt Sources:
 *            - TXE: Transmit buffer empty (ready for next byte)
 *            - RXNE: Receive buffer not empty (data received)
 *            - OVR: Overrun error (data lost)
 *            - MODF: Mode fault (multi-master conflict)
 *            - CRCERR: CRC error
 *            
 *            This is a basic implementation. In a complete driver, you would
 *            maintain state information and buffer pointers to handle
 *            interrupt-driven communication.
 */
void SPI_IRQHandling(SPI_Handle_t *pHandle)
{
    uint8_t temp1, temp2;

    /* Check for TXE flag and TXEIE control bit */
    temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_TXE);
    temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_TXEIE);
    if (temp1 && temp2)
    {
        // TXE interrupt occurred
        // Handle transmission
        spi_txe_interrupt_handle(pHandle);

        // In a complete implementation, you would:
        // 1. Load next byte from TX buffer to DR
        // 2. Decrement byte counter
        // 3. If all bytes sent, disable TXE interrupt
        // 4. Optionally call a callback function
        
        // For now, this is a placeholder
        // User should implement based on their needs
    }

    /* Check for RXNE flag and RXNEIE control bit */
    temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_RXNE);
    temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_RXNEIE);
    if (temp1 && temp2)
    {
        // RXNE interrupt occurred
        // Handle reception
        spi_rxne_interrupt_handle(pHandle);
        // In a complete implementation, you would:
        // 1. Read data from DR to RX buffer
        // 2. Decrement byte counter
        // 3. If all bytes received, disable RXNE interrupt
        // 4. Optionally call a callback function
        
        // For now, this is a placeholder
        // User should implement based on their needs
    }

    /* Check for Overrun error */
    temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_OVR);
    temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_ERRIE);
    if (temp1 && temp2)
    {
        // Handle OVR error
        spi_ovr_err_interrupt_handle(pHandle);
    }

    // /* Check for CRC error (if CRC is enabled )*/
    // temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_CRCERR);
    // if (temp1)
    // {
    //     // CRC error occurred
    //     // Clear the CRCERR flag by writing 0 to it 
    //     pHandle->pSPIx->SR &= ~(1U << SPI_SR_CRCERR);
    // }

    /* Check for Mode fault error */
    temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_MODF);
    if (temp1)
    {
        // Mode fault occurred (multi-master conflict)
        // MODF flag is cleared by reading SR then writing to CR1
        temp1 = pHandle->pSPIx->SR;
        pHandle->pSPIx->CR1 &= ~(1U << SPI_CR1_SPE);
    }
}

/* ==========================SPI_IRQHandling helper function implementation========================== */
static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
    // 2. Check the DFF bit in CR1
    if (pSPIHandle->pSPIx->CR1 & (1 << SPI_CR1_DFF))
    {
        /* 16-bit DFF handle */
        // 1. Load the data into the DR
        pSPIHandle->pSPIx->DR = *((uint16_t*)pSPIHandle->pTxBuffer);
        pSPIHandle->TxLen--;
        pSPIHandle->TxLen--;
        (uint16_t*)pSPIHandle->pTxBuffer++;
    }
    else
    {
        /* 8-bit DFF hanlde */
        // 1. Load the data into the DR
        pSPIHandle->pSPIx->DR = *pSPIHandle->pTxBuffer;;
        pSPIHandle->TxLen--;
        pSPIHandle->pTxBuffer++;
    }

    if (!pSPIHandle->TxLen)
    {
        // TxLen us 0, close SPI transmission
        SPI_CloseTransmission(pSPIHandle);

        // Inform the application that Tx is over
        SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
    }
}

static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
    // 2. Check the DFF bit in CR1
    if (pSPIHandle->pSPIx->CR1 & (1 << SPI_CR1_DFF))
    {
        /* 16-bit DFF handle */
        // 1. Load the data from DR to RxBuffer
        *((uint16_t*)pSPIHandle->pRxBuffer) = pSPIHandle->pSPIx->DR;
        pSPIHandle->RxLen -= 2;
        pSPIHandle->pRxBuffer--;
        pSPIHandle->pRxBuffer--;
    }
    else
    {
        /* 8-bit DFF hanlde */
        // 1. Load the data into the DR
        *(pSPIHandle->pRxBuffer) = (uint8_t) pSPIHandle->pSPIx->DR;
        pSPIHandle->RxLen--;
        pSPIHandle->pRxBuffer++;
    }

    if (!pSPIHandle->RxLen)
    {
        // TxLen us 0, close SPI reception
        SPI_CloseReception(pSPIHandle);

        // Inform the application that Tx is over
        SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
    }
}

static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
    if (pSPIHandle->TxState != SPI_BUSY_IN_TX)
    {
        // Clear the OVR flag
        SPI_ClearOVRFlag(pSPIHandle->pSPIx);
    }
    // void(temp);

    SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);    
}

/* ==========================Other APIs support SPI========================== */
/*
 * @fn      - SPI_PeripheralControl
 *
 * @brief   - Enable or disable the SPI peripheral (SPE bit in CR1)
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - EnOrDi: ENABLE or DISABLE
 *
 * @return  - none
 *
 * @note    - The SPE bit should be set only after configuring all other
 *            settings in CR1 and CR2. Some configuration bits cannot be
 *            changed while SPE=1.
 */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
    if (EnOrDi == ENABLE)
    {
        pSPIx->CR1 |= (1 << SPI_CR1_SPE);
    }
    else
    {
        pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
    }
}

/*
 * @fn      - SPI_SSIConfig
 *
 * @brief   - Configure the SSI bit in CR1 (internal slave select)
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - EnOrDi: ENABLE or DISABLE
 *
 * @return  - none
 *
 * @note    - When SSM=1 (software slave management), the SSI bit value is
 *            used as the NSS signal level internally. Setting SSI=1 prevents
 *            mode fault errors in master mode.
 */
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
    if (EnOrDi == ENABLE)
    {
        pSPIx->CR1 |= (1 << SPI_CR1_SSI);
    }
    else
    {
        pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
    }
}

/*
 * @fn      - SPI_SSOEConfig
 *
 * @brief   - Configure the SSOE bit in CR2 (slave select output enable)
 *
 * @param   - pSPIx: base address of the SPI peripheral
 * @param   - EnOrDi: ENABLE or DISABLE
 *
 * @return  - none
 *
 * @note    - When SSOE=1, the NSS pin is automatically managed by hardware
 *            in master mode. When SSOE=0, NSS output is disabled and can be
 *            used as a regular GPIO.
 *            Only relevant when SSM=0 (hardware slave management).
 */
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
    if (EnOrDi == ENABLE)
    {
        pSPIx->CR2 |= (1 << SPI_CR2_SSOE);
    }
    else
    {
        pSPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
    }
}

/*
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    - SPI 
*/
void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
    uint8_t temp;
    temp = pSPIx->DR;
    temp = pSPIx->SR;
}

/*
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    - SPI 
*/
void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
    pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_TXEIE);       // prevent interrupts from setting up of TXE flag
    pSPIHandle->pTxBuffer = NULL;
    pSPIHandle->TxLen = 0;
    pSPIHandle->TxState = SPI_READY;
}

/*
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    - SPI 
*/
void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
    // TxLen us 0, close SPI transmission
    pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_RXNEIE);       // prevent interrupts from setting up of TXE flag
    pSPIHandle->pRxBuffer = NULL;
    pSPIHandle->RxLen = 0;
    pSPIHandle->RxState = SPI_READY;
}

/* SPI application callback */
__attribute__((weak)) void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{

}
