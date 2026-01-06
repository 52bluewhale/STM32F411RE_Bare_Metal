/*
 * stm32f411xx_usart_driver.h
 *
 *  Created on: Jan 2, 2026
 *      Author: phanc
 */

#ifndef INC_STM32F411XX_USART_DRIVER_H_
#define INC_STM32F411XX_USART_DRIVER_H_

#include "stm32f411xx.h"

/* Peripheral definitions for I2C instances */
#define USART1                        ((USART_RegDef_t *)USART1_BASEADDR)
#define USART2                        ((USART_RegDef_t *)USART2_BASEADDR)
#define USART6                        ((USART_RegDef_t *)USART6_BASEADDR)

/* ========================= I2C HANDLING STRUCTURE ========================= */

/* Configuration structure for I2Cx peripheral */
typedef struct 
{
    uint8_t USART_Mode;
    uint32_t USART_Baud;
    uint8_t USART_NoOfStopBits;
    uint8_t USART_WordLength;
    uint8_t USART_ParityControl;
    uint8_t USART_HWFlowControl;
} USART_Config_t;

/* Handle structure for I2Cx peripheral */
typedef struct 
{
    USART_RegDef_t *pUSARTx;
    USART_Config_t USART_Config;
    uint8_t *pTxBuffer;
    uint8_t *pRxBuffer;
    uint32_t TxLen;
    uint32_t RxLen;
    uint8_t TxState;
    uint8_t RxState;
} USART_Handle_t;

/* ========================= I2C CONFIGURATION MACROS DEFINITIONS ========================= */

// USART_Mode configuration macros
#define USART_MODE_ONLY_TX              0
#define USART_MODE_ONLY_RX              1
#define USART_MODE_TXRX                 2

// USART_Baud configuration macros
#define USART_STD_BAUD_1200             1200
#define USART_STD_BAUD_2400             2400
#define USART_STD_BAUD_9600             9600
#define USART_STD_BAUD_19200            19200
#define USART_STD_BAUD_38400            38400
#define USART_STD_BAUD_57600            57600
#define USART_STD_BAUD_115200           115200
#define USART_STD_BAUD_230400           230400
#define USART_STD_BAUD_460800           460800
#define USART_STD_BAUD_921600           921600
#define USART_STD_BAUD_2M               2000000
#define USART_STD_BAUD_3M               3000000

// USART_ParityControl configuration macros
#define USART_PARITY_EN_ODD             2
#define USART_PARITY_EN_EVEN            1
#define USART_PARITY_DISABLE            0

// USART_WordLength configuration macros
#define USART_WORDLEN_8BIT              0
#define USART_WORDLEN_9BIT              1

// USART_NoOfStopBits configuration macros
#define USART_STOPBITS_1                0
#define USART_STOPBITS_0_5              1
#define USART_STOPBITS_2                2
#define USART_STOPBITS_1_5              3

// USART_HWFlowControl configuration macros
#define USART_HW_FLOW_CTRL_NONE    	    0
#define USART_HW_FLOW_CTRL_CTS    	    1
#define USART_HW_FLOW_CTRL_RTS    	    2
#define USART_HW_FLOW_CTRL_CTS_RTS	    3

// USART state
#define USART_READY                     0
#define USART_BUSY_IN_TX                1
#define USART_BUSY_IN_RX                2

// USART application events
#define USART_EVENT_TX_CMPLT            0
#define USART_EVENT_RX_CMPLT            1
#define USART_EVENT_CTS                 2
#define USART_EVENT_IDLE                3
#define USART_EVENT_ORE                 4  

/* ========================= I2C REGISTER BIT POSITION DEFINITIONS ========================= */

// Status register (USART_SR)
#define USART_SR_PE                     0       // Bit 0 PE: Parity error
#define USART_SR_FE                     1       // Bit 1 FE: Framing error
#define USART_SR_NF                     2       // Bit 2 NF: Noise detected flag
#define USART_SR_ORE                    3       // Bit 3 ORE: Overrun error
#define USART_SR_IDLE                   4       // Bit 4 IDLE: IDLE line detected
#define USART_SR_RXNE                   5       // Bit 5 RXNE: Read data register not empty
#define USART_SR_TC                     6       // Bit 6 TC: Transmission complete
#define USART_SR_TXE                    7       // Bit 7 TXE: Transmit data register empty
#define USART_SR_LBD                    8       // Bit 8 LBD: LIN break detection flag
#define USART_SR_CTS                    9       // Bit 9 CTS: CTS flag

// Baud rate register (USART_BRR)
#define USART_BRR_DIV_FRACTION          0       // Bits 3:0 DIV_Fraction[3:0]: fraction of USARTDIV
#define USART_BRR_DIV_MANTISSA          4       // Bits 15:4 DIV_Mantissa[11:0]: mantissa of USARTDIV

// Control register 1 (USART_CR1)
#define USART_CR1_SBK                   0       // Bit 0 SBK: Send break
#define USART_CR1_RWU                   1       // Bit 1 RWU: Receiver wakeup
#define USART_CR1_RE                    2       // Bit 2 RE: Receiver enable
#define USART_CR1_TE                    3       // Bit 3 TE: Transmitter enable
#define USART_CR1_IDLEIE                4       // Bit 4 IDLEIE: IDLE interrupt enable
#define USART_CR1_RXNEIE                5       // Bit 5 RXNEIE: RXNE interrupt enable
#define USART_CR1_TCIE                  6       // Bit 6 TCIE: Transmission complete interrupt enable
#define USART_CR1_TXEIE                 7       // Bit 7 TXEIE: TXE interrupt enable
#define USART_CR1_PEIE                  8       // Bit 8 PEIE: PE interrupt enable
#define USART_CR1_PS                    9       // Bit 9 PS: Parity selection
#define USART_CR1_PCE                   10      // Bit 10 PCE: Parity control enable
#define USART_CR1_WAKE                  11      // Bit 11 WAKE: Wakeup method
#define USART_CR1_M                     12      // Bit 12 M: Word length
#define USART_CR1_UE                    13      // Bit 13 UE: USART enable
#define USART_CR1_OVER8                 15      // Bit 15 OVER8: Oversampling mode

// Control register 2 (USART_CR2)
#define USART_CR2_ADD                   0       // Bits 3:0 ADD[3:0]: Address of the USART node
#define USART_CR2_LBDL                  5       // Bit 5 LBDL: lin break detection leng
#define USART_CR2_LBDIE                 6       // Bit 6 LBDIE: LIN break detection interrupt enable
#define USART_CR2_LBCL                  8       // Bit 8 LBCL: Last bit clock pulse
#define USART_CR2_CPHA                  9       // Bit 9 CPHA: Clock phase
#define USART_CR2_CPOL                  10      // Bit 10 CPOL: Clock polarity
#define USART_CR2_CLKEN                 11      // Bit 11 CLKEN: Clock enable
#define USART_CR2_STOP                  12      // Bits 13:12 STOP: STOP bits
#define USART_CR2_LINEN                 14      // Bit 14 LINEN: LIN mode enable

// Control register 3 (USART_CR3)
#define USART_CR3_EIE                   0       // Bit 0 EIE: Error interrupt enable
#define USART_CR3_IREN                  1       // Bit 1 IREN: IrDA mode enable
#define USART_CR3_IRLP                  2       // Bit 2 IRLP: IrDA low-power
#define USART_CR3_HDSEL                 3       // Bit 3 HDSEL: Half-duplex selection
#define USART_CR3_NACK                  4       // Bit 4 NACK: Smartcard NACK enable
#define USART_CR3_SCEN                  5       // Bit 5 SCEN: Smartcard mode enable
#define USART_CR3_DMAR                  6       // Bit 6 DMAR: DMA enable receiver
#define USART_CR3_DMAT                  7       // Bit 7 DMAT: DMA enable transmitter
#define USART_CR3_RTSE                  8       // Bit 8 RTSE: RTS enable
#define USART_CR3_CTSE                  9       // Bit 9 CTSE: CTS enable
#define USART_CR3_CTSIE                 10      // Bit 10 CTSIE: CTS interrupt enable
#define USART_CR3_ONEBIT                11      // Bit 11 ONEBIT: One sample bit method enable

// Guard time and prescaler register (USART_GTPR)
#define USART_GTPR_PSC                  0       // Bits 7:0 PSC[7:0]: Prescaler value
#define USART_GTPR_GT                   8       // Bits 15:8 GT[7:0]: Guard time value

/* ========================= I2C FLAG DEFINITIONS ========================= */
#define USART_FLAG_SR_PE                (1U << 0)       // Bit 0 PE: Parity error
#define USART_FLAG_SR_FE                (1U << 1)       // Bit 1 FE: Framing error
#define USART_FLAG_SR_NF                (1U << 2)       // Bit 2 NF: Noise detected flag
#define USART_FLAG_SR_ORE               (1U << 3)       // Bit 3 ORE: Overrun error
#define USART_FLAG_SR_IDLE              (1U << 4)       // Bit 4 IDLE: IDLE line detected
#define USART_FLAG_SR_RXNE              (1U << 5)       // Bit 5 RXNE: Read data register not empty
#define USART_FLAG_SR_TC                (1U << 6)       // Bit 6 TC: Transmission complete
#define USART_FLAG_SR_TXE               (1U << 7)       // Bit 7 TXE: Transmit data register empty
#define USART_FLAG_SR_LBD               (1U << 8)       // Bit 8 LBD: LIN break detection flag
#define USART_FLAG_SR_CTS               (1U << 9)       // Bit 9 CTS: CTS flag

/* ========================= I2C APIs ========================= */

/* Peripheral clock setup */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi);

/* Init and De-Init */
void USART_Init(USART_Handle_t *pUSARTHandle);
void USART_DeInit(USART_RegDef_t *pUSARTx);

/* Data send and receive */
void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len);
void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len);
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len);
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len);

/* IRQ configuration and ISR handling */
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void USART_IRQHandling(USART_Handle_t *pHandle);

/* Other APIs support USART */
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnOrDi);
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx , uint32_t FlagName);
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t StatusFlagName);

/* USART application callback */
void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle,uint8_t AppEv);

#endif /* INC_STM32F411XX_USART_DRIVER_H_ */
