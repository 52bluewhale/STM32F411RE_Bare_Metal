#include "stm32f411xx_spi_driver.h"

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
 * @note    -
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
        // TODO
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
 * @note    -
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
* @note    -
*/
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
    if (pSPIx->SR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
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
* @note    - Polling API send data
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
            /* 8-bit DFF hanlde */
            // 1. Load the data into the DR
            pSPIx->DR = *(pTxBuffer);
            Len--;
            pTxBuffer++;
        }
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
* @note    -
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
* @note    -
*/
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{

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
* @note    -
*/
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{

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
* @note    -
*/
void SPI_IRQHandling(SPI_Handle_t *pHandle)
{

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
* @note    -
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
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    -
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
* @fn      -
*
* @brief   -
*
* @param   -
* @param   -
*
* @return  -
*
* @note    -
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