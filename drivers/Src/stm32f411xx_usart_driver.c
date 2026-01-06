/*
 * stm32f411xx_usart_driver.c
 *
 *  Created on: Jan 2, 2026
 *      Author: phanc
 */

#include "stm32f411xx.h"
#include "stm32f411xx_usart_driver.h"

/* ========================= USART HELPER FUNCTIONS PROTOTYPE ========================= */
static uint16_t AHB_Prescaler[8] = {2, 4, 8, 16, 64, 128, 256, 512};
static uint8_t APB1_Prescaler[4] = {2, 4, 8, 16};

static uint32_t RCC_GetPLLOutputClock(void);

static uint32_t RCC_GetPCLK1Value(void);
static uint32_t RCC_GetPCLK2Value(void);

static void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate);

/* ========================= USART HELPER FUNCTIONS DEFINITIONS ========================= */
static uint32_t RCC_GetPLLOutputClock(void)
{
    uint32_t pllm, plln, pllp, pllsource;
    uint32_t vco_input, vco_output, pllclk;

    pllm = RCC->PLLCFGR & 0x3F;
    plln = (RCC->PLLCFGR >> 6) & 0x1FF;
    pllp = ((RCC->PLLCFGR >> 16) & 0x3);
    pllsource = (RCC->PLLCFGR >> 22) & 0x1;

    if (pllsource == 0)
    {
        vco_input = 16000000 / pllm;
    }
    else
    {
        vco_input = 8000000 / pllm;
    }

    vco_output = vco_input * plln;
    pllclk = vco_output / (2 * (pllp + 1));

    return pllclk;
}

static uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t pclk1, SystemClk;
    uint8_t clksrc, temp, ahbp, apb1p;

    // 1. Get clock source from RCC_CFGR (bit 3:2)
    // SWS[1:0] tell us which clock source is currently used
    clksrc = (RCC->CFGR >> 2) & 0x3;
    
    // 2. Determine syustem clock frequency based on source
    if (clksrc == 0)
    {
        // HSI selected
        SystemClk = 16000000;
    }
    else if (clksrc == 1)
    {
        // HSE selected
        SystemClk = 8000000;
    }
    else if (clksrc == 2)
    {
        // PLL selected
        SystemClk = RCC_GetPLLOutputClock();
    }
    else 
    {
        // Reserved - should not reach here
        SystemClk = 16000000;
    }

    // 3. Calculate AHB prescaler from RCC_CFGR (bit 7:4)
    // HPRE[3:0] determines AHB prescaler value 
    temp = (RCC->CFGR >> 4) & 0xF;
    if (temp < 8)
    {
        // 0xxx: SYSCLK not divided
        ahbp = 1;
    }
    else 
    {
        // 1xxx: SYSCLK divided by 2, 4, 7, 16, 64, 128, 256 or 512
        // Use lookup table: index = temp - 8
        ahbp = AHB_Prescaler[temp - 8];
    }

    // 4. Calculate APB1 prescaler from RCC_CFGR (bit 12:10)
    // PPRE1[2:0] determines APB1 prescaler value
    temp = (RCC->CFGR >> 10) & 0x7;
    if (temp < 4)
    {
        // 0xx: HCLK not divided
        apb1p = 1;
    }
    else 
    {
        // 1xx: HCLK divided by 2, 4, 8, 16
        // Use lookup table: index = temp - 4
        apb1p = APB1_Prescaler[temp - 4];
    }

    // 5. Calculate PCLK1 frequency
    // PCLK1 = SYSCLK / AHB_Prescaler / APB1_Prescaler
    pclk1 = (SystemClk / ahbp) / apb1p;

    return pclk1;
}

static uint32_t RCC_GetPCLK2Value(void)
{
    uint32_t pclk2, SystemClk;
    uint8_t clksrc, temp, ahbp, apb2p;

    // 1. Get clock source from RCC_CFGR (bit 3:2)
    clksrc = (RCC->CFGR >> 2) & 0x3;

    // 2. Determine system clock frequency based on source
    if (clksrc == 0)
    {
        SystemClk = 16000000;
    }
    else if (clksrc == 1)
    {
        SystemClk = 8000000;
    }
    else if (clksrc == 2)
    {
        SystemClk = RCC_GetPLLOutputClock();
    }
    else 
    {
        SystemClk = 16000000;
    }

    // 3. Calculate AHB prescaler (bit 7:4 of CFGR)
    temp = (RCC->CFGR >> 4) & 0xF;
    if (temp < 8)
    {
        ahbp = 1;
    }
    else 
    {
        ahbp = AHB_Prescaler[temp - 8];
    }

    // 4. Calculate AOB2 prescaler (bit 15: 13 of CFGR)
    temp = (RCC->CFGR >> 13) & 0x7;
    if (temp < 4)
    {
        apb2p = 1;
    }
    else 
    {
        apb2p = APB1_Prescaler[temp - 4];
    }

    // 5. Calculate PCLK2 frequency
    pclk2 = (SystemClk / ahbp) / apb2p;

    return pclk2;
}

static void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{
    uint32_t PCLKx;
    uint32_t usart_div;
    uint32_t M_part, F_part;
    uint32_t tempreg = 0;

    // 1. Get peripheral clock value
    if (pUSARTx == USART1 || pUSARTx == USART6)
    {
        // USART1 and USART6 are on APB2
        PCLKx = RCC_GetPCLK2Value();
    }
    else 
    {
        // USART2 is on APB1
        PCLKx = RCC_GetPCLK1Value();
    }

    // 2. Check for OVER8 configuration (sampling by 8)
    if (pUSARTx->CR1 & (1U << USART_CR1_OVER8))
    {
        // OVER8 = 1, oversampling by 8
        usart_div = ((25 * PCLKx) / (2 * BaudRate));
    }
    else 
    {
        // OVER8 = 0, oversampling by 16
        usart_div = ((25 * PCLKx) / (4 * BaudRate));
    }

    // 3. Calculate the Mantissa part
    M_part = usart_div / 100;

    // 4. Place the Mantissa part in appropriate bit position
    tempreg |= M_part << 4;

    // 5. Extract the fraction part
    F_part = (usart_div - (M_part * 100));

    // 6. Calculate the final fractional part
    if (pUSARTx->CR1 & (1U << USART_CR1_OVER8))
    {
        // OVER8 = 1, oversampling by 8, only 3 bits for fraction
        F_part = (((F_part * 8) + 50) / 100) & ((uint8_t)0x07);
    }
    else 
    {
        // OVER8 = 0, oversampling by 16, 4 bits for fraction
        F_part = (((F_part * 16) + 50) / 100) & ((uint8_t)0x0F);
    }

    // 7. Place the fractional part in appropriate bit position
    tempreg |= F_part;

    // 8. Copy the value of tempreg into BRR
    pUSARTx->BRR = tempreg;
}

/* ========================= USART APIs DEFINITIONS ========================= */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pUSARTx == USART1)
        {
            USART1_PCLK_EN();  // Enable USART1 clock
        }
        else if (pUSARTx == USART2)
        {
            USART2_PCLK_EN();  // Enable USART2 clock
        }
        else if (pUSARTx == USART6)
        {
            USART6_PCLK_EN();  // Enable USART6 clock
        }
    }
    else
    {
        if (pUSARTx == USART1)
        {
            USART1_PCLK_DI();  // Enable USART1 clock
        }
        else if (pUSARTx == USART2)
        {
            USART2_PCLK_DI();  // Enable USART2 clock
        }
        else if (pUSARTx == USART6)
        {
            USART6_PCLK_DI();  // Enable USART6 clock
        }
    }
}

void USART_Init(USART_Handle_t *pUSARTHandle)
{
    uint32_t tempreg = 0;

    // 1. Enable peripheral clock
    USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);

    // 2. Configure CR1
    // Enable USART Tx and Rx based on mode
    if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
    {
        // Enable receiver
        tempreg |= (1U << USART_CR1_RE);
    }
    else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
    {
        // Enable transmitter
        tempreg |= (1U << USART_CR1_TE);
    }
    else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
    {
        tempreg |= ((1U << USART_CR1_RE) | (1U << USART_CR1_TE));
    }

    // Configure word length
    tempreg |= pUSARTHandle->USART_Config.USART_WordLength << USART_CR1_M;

    // Configure parity bit
    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
    {
        // Enable parity, even parity (PS = 0)
        tempreg |= (1U << USART_CR1_PCE);
    }
    else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD)
    {
        // Enable parity, odd parity (PS = 1)
        tempreg |= (1U << USART_CR1_PCE);
        tempreg |= (1U << USART_CR1_PS);
    }

    // Write to CR1
    pUSARTHandle->pUSARTx->CR1 = tempreg;

    // 3. Configure CR2 register
    tempreg = 0;
    
    // Configure stop bit
    tempreg |= pUSARTHandle->USART_Config.USART_NoOfStopBits << USART_CR2_STOP;
    
    // Write to CR2
    pUSARTHandle->pUSARTx->CR2 = tempreg;

    // 4. Configure CR3 register
    tempreg = 0;

    if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
    {
        tempreg |= (1U << USART_CR3_CTSE);
    }
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
    {
        tempreg |= (1U << USART_CR3_RTSE);
    }
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
    {
        tempreg |= ((1U << USART_CR3_CTSE) | (1U << USART_CR3_RTSE));
    }

    // Write to CR3
    pUSARTHandle->pUSARTx->CR3 = tempreg;

    // 5. Configure Baudrate
    USART_SetBaudRate(pUSARTHandle->pUSARTx, pUSARTHandle->USART_Config.USART_Baud);
}

void USART_DeInit(USART_RegDef_t *pUSARTx)
{
    if (pUSARTx == USART1)
    {
        USART1_PCLK_RESET();
    }
    else if (pUSARTx == USART2)
    {
        USART2_PCLK_RESET();
    }
    else if (pUSARTx == USART6)
    {
        USART6_PCLK_RESET();
    }
}

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len)
{
    uint16_t *pdata;

    // Loop over until Len of bytes are all transferred
    for (uint32_t i = 0; i < Len; i++)
    {
        // Wait TXE flag is set in SR
        while (!(USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_SR_TXE)));

        // Check USART_WordLength items for 9bit or 8bit frame
        if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BIT)
        {
            // 9bit frame, load DR with 2 bytes masking the bits other than first 9 bit
            pdata = (uint16_t*)pTxBuffer;
            pUSARTHandle->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

            // Check for USART_ParityControl
            if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity is used, 9bit data will be sent
                pTxBuffer++;
                pTxBuffer++;
            }
            else 
            {
                // Parity bit is used in this transfer, 8bit of data will be sent
                // 9th bit will be replaced by parity bit by Hw
                pTxBuffer++;
            }
        }
        else 
        {
            // 8bit frame data transfer
            pUSARTHandle->pUSARTx->DR = (*pTxBuffer & (uint8_t)0xFF);
            pTxBuffer++;
        }
    }

    // Wait until TC flag is set in the SR
    while (!(USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_SR_TC)));
}

void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    // Loop over until Len of bytes are all transferred
    for (uint32_t i = 0; i < Len; i++) 
    {
        // Wait RXNE flag is set in SR
        while (!(USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_SR_RXNE)));

        // Check USART_WordLength items for 9bit or 8bit frame
        if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BIT)
        {
            // Check for parity bit control
            if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity bit, 9bit data
                *((uint16_t*)pRxBuffer) = (pUSARTHandle->pUSARTx->DR & (uint16_t)0x01FF);
                pRxBuffer++;
                pRxBuffer++;
            }
            else 
            {
                // Parity bit is used, 8bit data and 1 bit parity
                *pRxBuffer = (pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
                pRxBuffer++;
            }
        }
        else 
        {
            // Check for parity bit control
            if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity bit, 8bit data
                *pRxBuffer = (pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
            }
            else 
            {
                // Parity bit is used, 7bit data and 1 bit parity
                *pRxBuffer = (pUSARTHandle->pUSARTx->DR & (uint8_t)0x7F);
            }

            pRxBuffer++;
        }
    }
}

uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len)
{
    uint8_t tx_state = pUSARTHandle->TxState;

    if (tx_state != USART_BUSY_IN_TX)
    {
        // 1. Save buffer address and length
        pUSARTHandle->pTxBuffer = pTxBuffer;
        pUSARTHandle->TxLen = Len;
        
        // 2. Mark USART as busy in transmission
        pUSARTHandle->TxState = USART_BUSY_IN_TX;

        // 3. Enable TXE interrupt
        pUSARTHandle->pUSARTx->CR1 |= (1U << USART_CR1_TXEIE);

        // 4. Enable TC interrupt
        pUSARTHandle->pUSARTx->CR1 |= (1U << USART_CR1_TCIE);
    }

    return tx_state;
}

uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    uint8_t rx_state = pUSARTHandle->RxState;

    if (rx_state != USART_BUSY_IN_RX)
    {
        // 1. Save buffer address and length
        pUSARTHandle->pRxBuffer = pRxBuffer;
        pUSARTHandle->RxLen = Len;

        // 2. Mark USART as busy in reception
        pUSARTHandle->RxState = USART_BUSY_IN_RX;

        // 3. Enable RXNE interrupt
        pUSARTHandle->pUSARTx->CR1 |= (1U << USART_CR1_RXNEIE);
    }

    return rx_state;
}

void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
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

void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    // 1. Find the correct IPR register
    uint8_t iprx = IRQNumber / 4;

    // 2. Find the section within the IPR register
    uint8_t iprx_section = IRQNumber % 4;

    // 3. Calculate the shift amount
    // Each section is 8bit, but only upper 4bit are implemented
    uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

    // 4. Clear the priority bits first, then set new priority
    *(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFF << (8 * iprx_section));
    *(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

void USART_IRQHandling(USART_Handle_t *pUSARTHandle)
{
    uint32_t temp1, temp2, temp3;

    // Check for TXE flag
    temp1 = pUSARTHandle->pUSARTx->SR & (1U << USART_SR_TXE);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1U << USART_CR1_RXNEIE);

    if (temp1 && temp2)
    {
        // TXE is set - handle transmission
        if (pUSARTHandle->TxState == USART_BUSY_IN_TX)
        {
            if (pUSARTHandle->TxLen > 0)
            {
                // Check for 9bit or 8bit data frame
                if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BIT)
                {
                    uint16_t *pData = (uint16_t*)pUSARTHandle->pTxBuffer;
                    pUSARTHandle->pUSARTx->DR = (*pData & (uint16_t)0x01FF);

                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        pUSARTHandle->pTxBuffer += 2;
                    }
                    else 
                    {
                        pUSARTHandle->pTxBuffer++;
                    }

                    pUSARTHandle->TxLen--;
                }
                else 
                {
                    pUSARTHandle->pUSARTx->DR = (*pUSARTHandle->pTxBuffer & (uint8_t)0xFF);
                    pUSARTHandle->pTxBuffer++;
                    pUSARTHandle->TxLen--;
                }
            }

            if (pUSARTHandle->TxLen == 0)
            {
                // All data sent, disable TXE interrupt
                pUSARTHandle->pUSARTx->CR1 &= ~(1U << USART_CR1_TXEIE);
            }
        }
    }

    // Check for TC flag
    temp1 = pUSARTHandle->pUSARTx->SR & (1U << USART_SR_TC);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1U << USART_CR1_TCIE);

    if (temp1 && temp2)
    {
        // Transmission complete
        if (pUSARTHandle->TxLen == 0)
        {
            // Clear TC flag
            pUSARTHandle->pUSARTx->SR &= ~(1U << USART_SR_TC);

            // Disable TC interrupt
            pUSARTHandle->pUSARTx->CR1 &= ~(1U << USART_CR1_TCIE);

            // Reset state
            pUSARTHandle->TxState = USART_READY;
            pUSARTHandle->pTxBuffer = NULL;
            pUSARTHandle->TxLen = 0;

            // Notify application
            USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_TX_CMPLT);
        }
    }

    // Check for RXNE flag
    temp1 = pUSARTHandle->pUSARTx->SR & (1U << USART_SR_RXNE);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1U << USART_CR1_RXNEIE);

    if (temp1 && temp2)
    {
        // RXNE is set - handle reception
        if (pUSARTHandle->RxState == USART_BUSY_IN_RX)
        {
            if (pUSARTHandle->RxLen > 0)
            {
                if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BIT)
                {
                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        *((uint16_t*)pUSARTHandle->pRxBuffer) = (pUSARTHandle->pUSARTx->DR & (uint16_t)0x01FF);
                        pUSARTHandle->pRxBuffer += 2;
                    }
                    else 
                    {
                        *pUSARTHandle->pRxBuffer = (pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
                        pUSARTHandle->pRxBuffer++;
                    }
                    pUSARTHandle->RxLen--;
                }
                else 
                {
                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        *pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & 0xFF);
                    }
                    else 
                    {
                        *pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR &0x7F);
                    }
                    pUSARTHandle->pRxBuffer++;
                    pUSARTHandle->RxLen--;
                }
            }

            if (pUSARTHandle->RxLen == 0)
            {
                // Disable RXNE interrupt
                pUSARTHandle->pUSARTx->CR1 &= ~(1U << USART_CR1_RXNEIE);

                // Reset state
                pUSARTHandle->RxState = USART_READY;
                pUSARTHandle->pRxBuffer = NULL;
                pUSARTHandle->RxLen = 0;

                // Notify application
                USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_RX_CMPLT);
            }
        }        
    }

    // Check for error flag (Overrun, Noise, Framing, Parity)
    temp3 = pUSARTHandle->pUSARTx->SR;
    if (temp3 & (USART_FLAG_SR_ORE | USART_FLAG_SR_NF | USART_FLAG_SR_FE | USART_FLAG_SR_PE))
    {
        // Clear flag
        temp1 = pUSARTHandle->pUSARTx->SR;
        temp1 = pUSARTHandle->pUSARTx->DR;
        (void)temp1;
    }
}

void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnOrDi)
{
    if (EnOrDi == ENABLE)
    {
        pUSARTx->CR1 |= (1 << USART_CR1_UE);
    }
    else
    {
        pUSARTx->CR1 &= ~(1 << USART_CR1_UE);
    }
}

uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx , uint32_t FlagName)
{
    if (pUSARTx->SR & FlagName)
    {
        return FLAG_SET;
    }
    return FLAG_RESET;
}

void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t StatusFlagName)
{
    if (StatusFlagName & (USART_FLAG_SR_PE | USART_FLAG_SR_FE | USART_FLAG_SR_NF \
                            | USART_FLAG_SR_ORE | USART_FLAG_SR_IDLE))
    {
        // Read SR followed by DR to clear
        (void)pUSARTx->SR;
        (void)pUSARTx->DR;
    }
    else 
    {
        pUSARTx->SR &= ~(StatusFlagName);
    }
}