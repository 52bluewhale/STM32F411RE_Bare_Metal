/*
 * stm32f411xx_i2c_driver.h
 *
 *  Created on: Dec 8, 2025
 *      Author: phanc
 */

#ifndef INC_STM32F411XX_I2C_DRIVER_H_
#define INC_STM32F411XX_I2C_DRIVER_H_

#include "stm32f411xx.h"

/* Peripheral definitions for I2C instances */
#define I2C1                        ((I2C_RegDef_t *)I2C1_BASEADDR)
#define I2C2                        ((I2C_RegDef_t *)I2C2_BASEADDR)
#define I2C3                        ((I2C_RegDef_t *)I2C3_BASEADDR)

/* ========================= I2C HANDLING STRUCTURE ========================= */

/* Configuration structure for I2Cx peripheral */
typedef struct 
{
    uint32_t I2C_SCLSpeed;
    uint8_t I2C_DeviceAddress;
    uint8_t I2C_ACKControl;
    uint16_t I2C_FMDutyCycle;
} I2C_Config_t;

/* Handle structure for I2Cx peripheral */
typedef struct 
{
    I2C_RegDef_t *pI2Cx;
    I2C_Config_t I2C_Config;
    uint8_t *pTxBuffer;             // To store application Tx Buffer address
    uint8_t *pRxBuffer;             // To store application Rx Buffer address
    uint32_t TxLen;                 // To store Tx len
    uint32_t RxLen;                 // To store Rx len
    uint8_t TxRxState;              // To store communication state
    uint8_t DeviceAddress;          // To store slave/device address
    uint32_t RxSize;                // To store Rx size
    uint8_t Sr;                     // To store repeated start value
} I2C_Handle_t;

/* ========================= I2C CONFIGURATION MACROS DEFINITIONS ========================= */
#define I2C_SCL_SPEED_SM            100000
#define I2C_SCL_SPEED_FM4K          400000
#define I2C_SCL_SPEED_FM2K          200000
#define I2C_ACK_ENABLE              1
#define I2C_ACK_DISABLE             0
#define I2C_FM_DUTY_2               0
#define I2C_FM_DUTY_16_9            1

#define I2C_SR_DISABLE              RESET
#define I2C_SR_ENABLE               SET

#define I2C_READY                   0
#define I2C_BUSY_IN_RX              1
#define I2C_BUSY_IN_TX              2

/* I2C application events macros */
#define I2C_EV_TX_CMPLT             0
#define I2C_EV_RX_CMPLT             1
#define I2C_EV_STOP                 2
#define I2C_ERROR_BERR              3
#define I2C_ERROR_ARLO              4
#define I2C_ERROR_AF                5
#define I2C_ERROR_OVR               6
#define I2C_ERROR_TIMEOUT           7
#define I2C_EV_DATA_REQ             8
#define I2C_EV_DATA_RCV             9

/* ========================= I2C REGISTER BIT POSITION DEFINITIONS ========================= */

// I2C Control register 1 (I2C_CR1)
#define I2C_CR1_PE                  0           // Bit 0 PE: Peripheral enable
#define I2C_CR1_SMBUS               1           // Bit 1 SMBUS: SMBus mode
#define I2C_CR1_SMBTYPE             3           // Bit 3 SMBTYPE: SMBus type
#define I2C_CR1_ENARP               4           // Bit 4 ENARP: ARP enable
#define I2C_CR1_ENPEC               5           // Bit 5 ENPEC: PEC enable
#define I2C_CR1_ENGC                6           // Bit 6 ENGC: General call enable
#define I2C_CR1_NOSTRETCH           7           // Bit 7 NOSTRETCH: Clock stretching disable (Slave mode)
#define I2C_CR1_START               8           // Bit 8 START: Start generation
#define I2C_CR1_STOP                9           // Bit 9 STOP: Stop generation
#define I2C_CR1_ACK                 10          // Bit 10 ACK: Acknowledge enable
#define I2C_CR1_POS                 11          // Bit 11 POS: Acknowledge/PEC Position (for data reception)
#define I2C_CR1_PEC                 12          // Bit 12 PEC: Packet error checking
#define I2C_CR1_ALERT               13          // Bit 13 ALERT: SMBus alert
#define I2C_CR1_SWRST               15          // Bit 15 SWRST: Software reset

// I2C Control register 2 (I2C_CR2)
#define I2C_CR2_FREQ                0           // Bits 5:0 FREQ[5:0]: Peripheral clock frequency
#define I2C_CR2_ITERREN             8           // Bit 8 ITERREN: Error interrupt enable
#define I2C_CR2_ITEVTEN             9           // Bit 9 ITEVTEN: Event interrupt enable
#define I2C_CR2_ITBUFEN             10          // Bit 10 ITBUFEN: Buffer interrupt enable
#define I2C_CR2_DMAEN               11          // Bit 11 DMAEN: DMA requests enable
#define I2C_CR2_LAST                12          // Bit 12 LAST: DMA last transfer

// I2C Own address register 1 (I2C_OAR1)
#define I2C_OAR1_ADD0               0           // Bit 0 ADD0: Interface address
#define I2C_OAR1_ADD_7_1            1           // Bits 7:1 ADD[7:1]: Interface address
#define I2C_OAR1_ADD_9_8            8           // Bits 9:8 ADD[9:8]: Interface address
#define I2C_OAR1_ADDMODE            15          // Bit 15 ADDMODE Addressing mode (slave mode)

// I2C Own address register 2 (I2C_OAR2)
#define I2C_OAR2_ENDUAL             0           // Bit 0 ENDUAL: Dual addressing mode enable
#define I2C_OAR2_ADD2               1           // Bits 7:1 ADD2[7:1]: Interface address

// I2C Status register 1 (I2C_SR1)
#define I2C_SR1_SB                  0           // Bit 0 SB: Start bit (Master mode)
#define I2C_SR1_ADDR                1           // Bit 1 ADDR: Address sent (master mode)/matched (slave mode)
#define I2C_SR1_BTF                 2           // Bit 2 BTF: Byte transfer finished
#define I2C_SR1_ADD10               3           // Bit 3 ADD10: 10-bit header sent (Master mode)
#define I2C_SR1_STOPF               4           // Bit 4 STOPF: Stop detection (slave mode)
#define I2C_SR1_RXNE                6           // Bit 6 RxNE: Data register not empty (receivers)
#define I2C_SR1_TXE                 7           // Bit 7 TxE: Data register empty (transmitters)
#define I2C_SR1_BERR                8           // Bit 8 BERR: Bus error
#define I2C_SR1_ARLO                9           // Bit 9 ARLO: Arbitration lost (master mode)
#define I2C_SR1_AF                  10          // Bit 10 AF: Acknowledge failure
#define I2C_SR1_OVR                 11          // Bit 11 OVR: Overrun/Underrun
#define I2C_SR1_PECERR              12          // Bit 12 PECERR: PEC Error in reception
#define I2C_SR1_TIMEOUT             14          // Bit 14 TIMEOUT: Timeout or Tlow error
#define I2C_SR1_SMBALERT            15          // Bit 15 SMBALERT: SMBus alert

// I2C Status register 2 (I2C_SR2)
#define I2C_SR2_MSL                 0           // Bit 0 MSL: Master/slave
#define I2C_SR2_BUSY                1           // Bit 1 BUSY: Bus busy
#define I2C_SR2_TRA                 2           // Bit 2 TRA: Transmitter/receiver
#define I2C_SR2_MSL                 4           // Bit 4 GENCALL: General call address (Slave mode)
#define I2C_SR2_SMBDEFAULT          5           // Bit 5 SMBDEFAULT: SMBus device default address (Slave mode)
#define I2C_SR2_SMBHOST             6           // Bit 6 SMBHOST: SMBus host header (Slave mode)
#define I2C_SR2_DUALF               7           // Bit 7 DUALF: Dual flag (Slave mode)
#define I2C_SR2_PEC                 8           // Bits 15:8 PEC[7:0] Packet error checking register

// I2C Clock control register (I2C_CCR)
#define I2C_CCR_CCR                 0           // Bits 11:0 CCR[11:0]: Clock control register in Fm/Sm mode (Master mode)
#define I2C_CCR_DUTY                14          // Bit 14 DUTY: Fm mode duty cycle
#define I2C_CCR_FS                  15          // Bit 15 F/S: I2C master mode selection

// I2C TRISE register (I2C_TRISE)
#define I2C_TRISE_TRISE             0           // Bits 5:0: Maximum rise time in Fast/Standard mode (Master mode)

// I2C FLTR register (I2C_FLTR)
#define I2C_TRISE_DNF               0           // Bits 3:0 DNF[3:0]: Digital noise filter
#define I2C_TRISE_ANOFF             4           // Bit 4 ANOFF: Analog noise filter OFF

/* ========================= I2C FLAG DEFINITIONS ========================= */

// I2C Status register 1 flag definition (I2C_SR1)
#define I2C_FLAG_SR1_SB             (1U << I2C_SR1_SB)          // Start bit flag
#define I2C_FLAG_SR1_ADDR           (1U << I2C_SR1_ADDR)        // Address sent/matched flag
#define I2C_FLAG_SR1_BTF            (1U << I2C_SR1_BTF)         // Byte transfer finished flag
#define I2C_FLAG_SR1_ADD10          (1U << I2C_SR1_ADD10)       // 10-bit header sent flag
#define I2C_FLAG_SR1_STOPF          (1U << I2C_SR1_STOPF)       // Stop detection flag
#define I2C_FLAG_SR1_RXNE           (1U << I2C_SR1_RXNE)        // Data register not empty flag
#define I2C_FLAG_SR1_TXE            (1U << I2C_SR1_TXE)         // Data register empty flag
#define I2C_FLAG_SR1_BERR           (1U << I2C_SR1_BERR)        // Bus error flag
#define I2C_FLAG_SR1_ARLO           (1U << I2C_SR1_ARLO)        // Arbitration lost flag
#define I2C_FLAG_SR1_AF             (1U << I2C_SR1_AF)          // Acknowledge failure flag
#define I2C_FLAG_SR1_OVR            (1U << I2C_SR1_OVR)         // Overrun/Underrun flag
#define I2C_FLAG_SR1_PECERR         (1U << I2C_SR1_PECERR)      // PEC error flag
#define I2C_FLAG_SR1_TIMEOUT        (1U << I2C_SR1_TIMEOUT)     // Timeout flag
#define I2C_FLAG_SR1_SMBALERT       (1U << I2C_SR1_SMBALERT)    // SMBus alert flag

// I2C Status register 2 flag definition (I2C_SR2)
#define I2C_FLAG_SR2_MSL            (1U << I2C_SR2_MSL)         // Master/Slave flag
#define I2C_FLAG_SR2_BUSY           (1U << I2C_SR2_BUSY)        // Bus busy flag
#define I2C_FLAG_SR2_TRA            (1U << I2C_SR2_TRA)         // Transmitter/Receiver flag
#define I2C_FLAG_SR2_MSL            (1U << I2C_SR2_GENCALL)     // General call flag
#define I2C_FLAG_SR2_SMBDEFAULT     (1U << I2C_SR2_SMBDEFAULT)  // SMBus device default flag
#define I2C_FLAG_SR2_SMBHOST        (1U << I2C_SR2_SMBHOST)     // SMBus host header flag
#define I2C_FLAG_SR2_DUALF          (1U << I2C_SR2_DUALF)       // Dual flag

/* ========================= I2C APIs ========================= */

/* Peripheral clock setup */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/* Init and De-Init */
void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_RegDef_t *pI2Cx);

/* Get flag status */
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName);

/* Data send and receive */
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr);

/* Data send and receive with Interrupt*/
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr);
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr);

/* IRQ configuration and ISR handling */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);

/* Other APIs support I2C */
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
void I2C_ManageACK(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);

/* I2C application callback */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv);

#endif /* INC_STM32F411XX_I2C_DRIVER_H_ */
