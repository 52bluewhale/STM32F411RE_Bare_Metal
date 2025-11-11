#ifndef INC_STM32F411XX_SPI_DRIVER_H_
#define INC_STM32F411XX_SPI_DRIVER_H_

#include "stm32f411xx.h"

/* Peripheral definitions for SPI instances */
#define SPI1        ((SPI_RegDef_t *)SPI1_I2S1_BASEADDR)
#define SPI2        ((SPI_RegDef_t *)SPI2_I2S2_BASEADDR)
#define SPI3        ((SPI_RegDef_t *)SPI3_I2S3_BASEADDR)
#define SPI4        ((SPI_RegDef_t *)SPI4_I2S4_BASEADDR)
#define SPI5        ((SPI_RegDef_t *)SPI5_I2S5_BASEADDR)

/* ========================= SPI HANDLING STRUCTURE ========================= */

/* Configuration structure for SPIx peripheral */
typedef struct 
{
    uint8_t SPI_DeviceMode;
    uint8_t SPI_BusConfig;
    uint8_t SPI_SclkSpeed;
    uint8_t SPI_DFF;
    uint8_t SPI_CPOL;
    uint8_t SPI_CPHA;
    uint8_t SPI_SSM;
} SPI_Config_t;

/* Handle structure for SPIx peripheral */
typedef struct 
{
    SPI_RegDef_t *pSPIx;
    SPI_Config_t SPIConfig;
} SPI_Handle_t;

/* ========================= SPI MACROS ========================= */

// SPI_DeviceMode
#define SPI_DEVICE_MOD_MASTER               1
#define SPI_DEVICE_MOD_SLAVE                0

// SPI_BusConfig
#define SPI_BUS_CONFIG_FD                   1
#define SPI_BUS_CONFIG_HD                   2
#define SPI_BUS_CONFIG_SIMPLEX_RXONLY       3

// SPI_SclkSpeed
#define SPI_CLK_SPEED_DIV2                  0
#define SPI_CLK_SPEED_DIV4                  1
#define SPI_CLK_SPEED_DIV8                  2
#define SPI_CLK_SPEED_DIV16                 3
#define SPI_CLK_SPEED_DIV32                 4
#define SPI_CLK_SPEED_DIV64                 5
#define SPI_CLK_SPEED_DIV128                6
#define SPI_CLK_SPEED_DIV256                7

// SPI_DFF
#define SPI_DFF_8BITS                       0
#define SPI_DFF_16BITS                      1

// SPI_CPOL
#define SPI_CPOL_HIGH                       1
#define SPI_CPOL_LOW                        0

// SPI_CPHA
#define SPI_CPHA_HIGH                       1
#define SPI_CPHA_LOW                        0

// SPI_SSM
#define SPI_SSM_EN                          1
#define SPI_SSM_DI                          0

// SPI control register 1 (SPI_CR1)
#define SPI_CR1_CPHA                        0       // Bit 0 CPHA: Clock phase
#define SPI_CR1_CPOL                        1       // Bit1 CPOL: Clock polarity
#define SPI_CR1_MSTR                        2       // Bit 2 MSTR: Master selection
#define SPI_CR1_BR                          3       // Bits 5:3 BR[2:0]: Baud rate control
#define SPI_CR1_SPE                         6       // Bit 6 SPE: SPI enable
#define SPI_CR1_LSBFIRST                    7       // Bit 7 LSBFIRST: Frame format
#define SPI_CR1_SSI                         8       // Bit 8 SSI: Internal slave select
#define SPI_CR1_SSM                         9       // Bit 9 SSM: Software slave management
#define SPI_CR1_RXONLY                      10      // Bit 10 RXONLY: Receive only
#define SPI_CR1_DFF                         11      // Bit 11 DFF: Data frame format
#define SPI_CR1_CRCNEXT                     12      // Bit 12 CRCNEXT: CRC transfer next
#define SPI_CR1_CRCEN                       13      // Bit 13 CRCEN: Hardware CRC calculation enable
#define SPI_CR1_BIDIOE                      14      // Bit 14 BIDIOE: Output enable in bidirectional mode
#define SPI_CR1_BIDIMODE                    15      // Bit 15 BIDIMODE: Bidirectional data mode enable

// SPI control register 2 (SPI_CR2)
#define SPI_CR2_RXDMAEN                     0       // Bit 0 RXDMAEN: Rx buffer DMA enable
#define SPI_CR2_TXDMAEN                     1       // Bit 1 TXDMAEN: Tx buffer DMA enable
#define SPI_CR2_SSOE                        2       // Bit 2 SSOE: SS output enable
#define SPI_CR2_FRF                         4       // Bit 4 FRF: Frame format
#define SPI_CR2_ERRIE                       5       // Bit 5 ERRIE: Error interrupt enable
#define SPI_CR2_RXNEIE                      6       // Bit 6 RXNEIE: RX buffer not empty interrupt enable
#define SPI_CR2_TXEIE                       7       // Bit 7 TXEIE: Tx buffer empty interrupt enable

// SPI status register (SPI_SR)
#define SPI_SR_RXNE                         0       // Bit 0 RXNE: Receive buffer not empty
#define SPI_SR_TXE                          1       // Bit 1 TXE: Transmit buffer empty
#define SPI_SR_CHSIDE                       2       // Bit 2 CHSIDE: Channel side
#define SPI_SR_UDR                          3       // Bit 3 UDR: Underrun flag
#define SPI_SR_CRCERR                       4       // Bit 4 CRCERR: CRC error flag
#define SPI_SR_MODF                         5       // Bit 5 MODF: Mode fault
#define SPI_SR_OVR                          6       // Bit 6 OVR: Overrun flag
#define SPI_SR_BSY                          7       // Bit 7 BSY: Busy flag
#define SPI_SR_FRE                          8       // Bit 8 FRE: Frame format error

// SPI flag for status register (SPI_SR)
#define SPI_SR_RXNE                         (1 << SPI_SR_RXNE)       
#define SPI_SR_TXE                          (1 << SPI_SR_TXE)      
#define SPI_SR_CHSIDE                       (2 << SPI_SR_CHSIDE)       
#define SPI_SR_UDR                          (3 << SPI_SR_UDR)     
#define SPI_SR_CRCERR                       (4 << SPI_SR_CRCERR)
#define SPI_SR_MODF                         (5 << SPI_SR_MODF)
#define SPI_SR_OVR                          (6 << SPI_SR_OVR)
#define SPI_SR_BSY                          (7 << SPI_SR_BSY)
#define SPI_SR_FRE                          (8 << SPI_SR_FRE)    

/* ========================= SPI APIs ========================= */

/* Peripheral clock setup */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi);

/* Init and De-Init */
void SPI_Init(SPI_Handle_t *pSPIHandle);
void SPI_DeInit(SPI_RegDef_t *pSPIx);

/* Data send and receive */
void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len);
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len);

/* IRQ configuration and ISR handling */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority);
void SPI_IRQHandling(SPI_Handle_t *pHandle);

/* Other APIs support SPI */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi);

#endif /* INC_STM32F411XX_SPI_DRIVER_H_ */
