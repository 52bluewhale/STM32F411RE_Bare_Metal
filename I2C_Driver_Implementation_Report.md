# STM32F411xx I2C Driver Implementation Report

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [I2C Protocol Overview](#i2c-protocol-overview)
3. [Driver Architecture](#driver-architecture)
4. [Implementation Guide](#implementation-guide)
5. [Detailed API Reference](#detailed-api-reference)
6. [Common Pitfalls and How to Avoid Them](#common-pitfalls-and-how-to-avoid-them)
7. [Testing Strategies](#testing-strategies)
8. [Performance Considerations](#performance-considerations)
9. [Appendix](#appendix)

---

## Executive Summary

This report provides a comprehensive analysis of the STM32F411xx I2C driver implementation, detailing the design decisions, implementation strategies, and best practices for developing robust I2C communication drivers for embedded systems. The driver supports both polling and interrupt-driven communication modes, master and slave operations, and handles all critical I2C protocol sequences.

**Key Features Implemented:**
- Master transmit/receive in polling mode
- Master transmit/receive in interrupt mode
- Configurable SCL speeds (Standard Mode: 100kHz, Fast Mode: 400kHz)
- ACK/NACK management
- Repeated start condition support
- Comprehensive error handling
- NVIC interrupt configuration

**Driver Location:**
- Header: `drivers/Inc/stm32f411xx_i2c_driver.h`
- Source: `drivers/Src/stm32f411xx_i2c_driver.c`
- Base definitions: `drivers/Inc/stm32f411xx.h`

---

## I2C Protocol Overview

### What is I2C?

I2C (Inter-Integrated Circuit) is a synchronous, multi-master, multi-slave, packet-switched, single-ended, serial communication bus invented by Philips Semiconductor (now NXP Semiconductors). It is widely used for attaching lower-speed peripheral ICs to processors and microcontrollers in short-distance, intra-board communication.

### Key Characteristics

| Feature | Description |
|---------|-------------|
| **Wires** | 2-wire interface (SDA - Serial Data, SCL - Serial Clock) |
| **Speed** | Standard Mode (100 kHz), Fast Mode (400 kHz), Fast Mode Plus (1 MHz), High Speed (3.4 MHz) |
| **Addressing** | 7-bit or 10-bit addressing |
| **Topology** | Multi-master, multi-slave |
| **Voltage Levels** | Open-drain outputs with pull-up resistors |

### I2C Communication Sequence

```
Master Transmit:
┌─────────────────────────────────────────────────────────────────┐
│ START → ADDR+W → ACK → DATA → ACK → ... → DATA → ACK → STOP    │
└─────────────────────────────────────────────────────────────────┘

Master Receive:
┌─────────────────────────────────────────────────────────────────┐
│ START → ADDR+R → ACK → DATA ← ACK → ... → DATA ← NACK → STOP   │
└─────────────────────────────────────────────────────────────────┘

Repeated Start (No STOP between transactions):
┌─────────────────────────────────────────────────────────────────┐
│ START → ADDR+W → ACK → DATA → ACK → Sr → ADDR+R → ACK → ...    │
└─────────────────────────────────────────────────────────────────┘
```

### I2C Bit Format

```
Address Frame (7-bit addressing):
┌────────────────────────────────┐
│ A6 A5 A4 A3 A2 A1 A0 R/W │ ACK │
└────────────────────────────────┘
   7-bit slave address    │
                          └─── 0=Write, 1=Read
```

---

## Driver Architecture

### Layered Architecture

```
┌─────────────────────────────────────────────────┐
│           Application Layer                      │
│  (010_i2c_master_tx_testing.c, etc.)            │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│        I2C Driver API Layer                      │
│    (stm32f411xx_i2c_driver.c/.h)                │
│  - I2C_Init()                                    │
│  - I2C_MasterSendData()                          │
│  - I2C_MasterReceiveData()                       │
│  - I2C_MasterSendDataIT()                        │
│  - I2C_MasterReceiveDataIT()                     │
│  - I2C_EV_IRQHandling()                          │
│  - I2C_ER_IRQHandling()                          │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│      Hardware Abstraction Layer                  │
│           (stm32f411xx.h)                        │
│  - Register definitions                          │
│  - Bit position definitions                      │
│  - Base address definitions                      │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│         STM32F411 Hardware                       │
│     I2C1, I2C2, I2C3 Peripherals                │
└─────────────────────────────────────────────────┘
```

### Data Structures

#### 1. Configuration Structure (`I2C_Config_t`)
```c
typedef struct
{
    uint32_t I2C_SCLSpeed;        // SCL clock speed
    uint8_t I2C_DeviceAddress;    // Device own address (slave mode)
    uint8_t I2C_ACKControl;       // ACK enable/disable
    uint16_t I2C_FMDutyCycle;     // Fast mode duty cycle (2 or 16/9)
} I2C_Config_t;
```

**Purpose:** Holds user-configurable parameters for initializing the I2C peripheral.

#### 2. Handle Structure (`I2C_Handle_t`)
```c
typedef struct
{
    I2C_RegDef_t *pI2Cx;          // Pointer to I2C peripheral registers
    I2C_Config_t I2C_Config;      // Configuration parameters
    uint8_t *pTxBuffer;           // TX buffer pointer (interrupt mode)
    uint8_t *pRxBuffer;           // RX buffer pointer (interrupt mode)
    uint32_t TxLen;               // TX length remaining
    uint32_t RxLen;               // RX length remaining
    uint8_t TxRxState;            // Current state (READY/BUSY_IN_TX/BUSY_IN_RX)
    uint8_t DeviceAddress;        // Slave address for current transaction
    uint32_t RxSize;              // Original RX size (for single-byte detection)
    uint8_t Sr;                   // Repeated start flag
} I2C_Handle_t;
```

**Purpose:** Complete context for I2C operations, especially critical for interrupt-driven communication.

#### 3. Register Definition Structure (`I2C_RegDef_t`)
```c
typedef struct
{
    volatile uint32_t CR1;        // Control register 1
    volatile uint32_t CR2;        // Control register 2
    volatile uint32_t OAR1;       // Own address register 1
    volatile uint32_t OAR2;       // Own address register 2
    volatile uint32_t DR;         // Data register
    volatile uint32_t SR1;        // Status register 1
    volatile uint32_t SR2;        // Status register 2
    volatile uint32_t CCR;        // Clock control register
    volatile uint32_t TRISE;      // Rise time register
    volatile uint32_t FLTR;       // Filter register
} I2C_RegDef_t;
```

**Purpose:** Direct mapping to hardware registers for type-safe register access.

---

## Implementation Guide

### Step 1: Understanding the Hardware Registers

#### Key I2C Registers

**CR1 (Control Register 1)** - Primary control of I2C peripheral
- **PE (Bit 0):** Peripheral Enable - Must be 0 during configuration
- **START (Bit 8):** Start generation - Triggers START condition
- **STOP (Bit 9):** Stop generation - Triggers STOP condition
- **ACK (Bit 10):** Acknowledge enable - Controls automatic ACK
- **SWRST (Bit 15):** Software reset

**CR2 (Control Register 2)** - Interrupts and clock configuration
- **FREQ[5:0] (Bits 5:0):** Peripheral clock frequency in MHz
- **ITERREN (Bit 8):** Error interrupt enable
- **ITEVTEN (Bit 9):** Event interrupt enable
- **ITBUFEN (Bit 10):** Buffer interrupt enable

**SR1 (Status Register 1)** - Event flags
- **SB (Bit 0):** Start bit - Set after START condition
- **ADDR (Bit 1):** Address sent/matched
- **BTF (Bit 2):** Byte transfer finished
- **TXE (Bit 7):** Transmit buffer empty
- **RXNE (Bit 6):** Receive buffer not empty
- **AF (Bit 10):** Acknowledge failure

**SR2 (Status Register 2)** - Status information
- **MSL (Bit 0):** Master/slave mode
- **BUSY (Bit 1):** Bus busy
- **TRA (Bit 2):** Transmitter/receiver

**CCR (Clock Control Register)** - SCL frequency configuration
- **CCR[11:0] (Bits 11:0):** Clock control value
- **DUTY (Bit 14):** Fast mode duty cycle (Tlow/Thigh = 2 or 16/9)
- **F/S (Bit 15):** Fast/Standard mode selection

**TRISE (Rise Time Register)** - Maximum rise time configuration
- **TRISE[5:0] (Bits 5:0):** Maximum rise time value

### Step 2: Clock Configuration Calculations

The I2C clock speed is one of the most critical aspects to get right.

#### Standard Mode (100 kHz)

For Standard Mode:
- T_high = T_low (50% duty cycle)
- T_scl = 2 × CCR × T_pclk1

**Formula:**
```
CCR = PCLK1 / (2 × SCL_frequency)
```

**Example:**
If PCLK1 = 16 MHz, SCL = 100 kHz:
```
CCR = 16,000,000 / (2 × 100,000) = 80
```

#### Fast Mode (400 kHz)

Fast Mode supports two duty cycles:

**Duty Cycle = 2 (T_low/T_high = 2)**
```
T_scl = 3 × CCR × T_pclk1
CCR = PCLK1 / (3 × SCL_frequency)
```

**Duty Cycle = 16/9 (T_low/T_high = 16/9)**
```
T_scl = 25 × CCR × T_pclk1
CCR = PCLK1 / (25 × SCL_frequency)
```

**Example:**
If PCLK1 = 16 MHz, SCL = 400 kHz, Duty = 2:
```
CCR = 16,000,000 / (3 × 400,000) = 13.33 ≈ 13
```

#### Rise Time Configuration

The TRISE register configures the maximum rise time for the SCL/SDA lines.

**Standard Mode:**
- Maximum rise time = 1000 ns
```
TRISE = (PCLK1_freq_MHz × 1000ns / 1000) + 1
TRISE = (PCLK1 / 1,000,000) + 1
```

**Fast Mode:**
- Maximum rise time = 300 ns
```
TRISE = (PCLK1_freq_MHz × 300ns / 1000) + 1
TRISE = ((PCLK1 × 300) / 1,000,000,000) + 1
```

### Step 3: I2C Initialization Sequence

The initialization must follow a specific sequence as per the STM32 reference manual.

```c
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
    // Ensure peripheral is DISABLED before configuration
    // PE bit in CR1 must be 0

    // Step 1: Enable peripheral clock
    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

    // Step 2: Configure ACK control bit
    tempreg |= pI2CHandle->I2C_Config.I2C_ACKControl << I2C_CR1_ACK;
    pI2CHandle->pI2Cx->CR1 = tempreg;

    // Step 3: Configure FREQ field in CR2 (peripheral clock in MHz)
    tempreg = RCC_GetPCLK1Value() / 1000000U;
    pI2CHandle->pI2Cx->CR2 = (tempreg & 0x3F);

    // Step 4: Configure device own address (for slave mode)
    tempreg = pI2CHandle->I2C_Config.I2C_DeviceAddress << I2C_OAR1_ADD_7_1;
    tempreg |= (1 << 14);  // Bit 14 should be kept at 1 by software
    pI2CHandle->pI2Cx->OAR1 = tempreg;

    // Step 5: Calculate and configure CCR
    if (Standard Mode) {
        ccr_value = (PCLK1 / (2 * I2C_SCLSpeed));
    } else {  // Fast Mode
        if (Duty == 2) {
            ccr_value = (PCLK1 / (3 * I2C_SCLSpeed));
        } else {  // Duty 16/9
            ccr_value = (PCLK1 / (25 * I2C_SCLSpeed));
        }
    }
    pI2CHandle->pI2Cx->CCR = tempreg;

    // Step 6: Configure TRISE
    if (Standard Mode) {
        TRISE = (PCLK1_MHz) + 1;
    } else {  // Fast Mode
        TRISE = ((PCLK1 * 300) / 1000000000) + 1;
    }
    pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3F);

    // Step 7: Enable peripheral (done separately via I2C_PeripheralControl)
}
```

**Critical Points:**
1. Peripheral clock MUST be enabled before accessing registers
2. Configuration MUST occur while PE=0 (peripheral disabled)
3. Bit 14 of OAR1 must always be kept at 1 (reference manual requirement)
4. FREQ field must match actual PCLK1 frequency for correct timing

### Step 4: Master Transmit Implementation (Polling Mode)

Master transmission follows a strict I2C protocol sequence.

```c
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer,
                        uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    // 1. Generate START condition
    //    Sets START bit in CR1, hardware generates START on bus
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

    // 2. Wait for START to complete (SB flag set in SR1)
    //    SB is set when START condition is generated
    //    SCL is stretched (held LOW) until SB is cleared
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_SB));

    // 3. Send slave address with R/W bit = 0 (write)
    //    Address is 7 bits, shifted left, LSB = 0 for write
    //    Writing to DR clears SB flag
    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);

    // 4. Wait for address ACK (ADDR flag set in SR1)
    //    ADDR is set when address is sent and ACK received
    //    SCL is stretched until ADDR is cleared
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_ADDR));

    // 5. Clear ADDR flag by reading SR1 then SR2
    //    This is the only way to clear ADDR (hardware requirement)
    I2C_ClearADDRFlag(pI2CHandle);

    // 6. Send data bytes
    while (Len > 0)
    {
        // Wait for TXE (transmit buffer empty)
        //   TXE=1 means DR is empty, ready for next byte
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_TXE));

        // Write data to DR register
        pI2CHandle->pI2Cx->DR = *pTxBuffer;
        pTxBuffer++;
        Len--;
    }

    // 7. Wait for transmission complete
    //    TXE=1 and BTF=1 means both shift register and DR are empty
    //    This ensures last byte is fully transmitted
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_TXE));
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_BTF));

    // 8. Generate STOP condition (if not repeated start)
    //    Sets STOP bit in CR1, hardware generates STOP on bus
    //    STOP automatically clears BTF
    if (Sr == I2C_SR_DISABLE) {
        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
    }
}
```

**Flag Sequence Diagram:**
```
Time →
START  SB   ADDR TXE  TXE  TXE  BTF  STOP
  │     │     │    │    │    │    │    │
  ▼     ▼     ▼    ▼    ▼    ▼    ▼    ▼
┌───┬─────┬─────┬────┬────┬────┬────┬────┐
│ S │Addr│ ACK │ D1 │ D2 │ D3 │ ... │ P  │
└───┴─────┴─────┴────┴────┴────┴────┴────┘
  │         │         └──Data bytes──┘
  └─Address─┘
```

### Step 5: Master Receive Implementation (Polling Mode)

Master reception is more complex due to ACK/NACK timing requirements.

```c
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer,
                           uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    // 1. Generate START condition
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

    // 2. Wait for START complete (SB flag)
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_SB));

    // 3. Send slave address with R/W bit = 1 (read)
    I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr);

    // 4. Wait for address ACK (ADDR flag)
    while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_ADDR));

    // === SINGLE BYTE RECEPTION ===
    if (Len == 1)
    {
        // CRITICAL: Disable ACK BEFORE clearing ADDR
        //   This ensures NACK is sent after the single byte
        //   If ACK is disabled after ADDR clear, timing may fail
        I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

        // Clear ADDR flag
        I2C_ClearADDRFlag(pI2CHandle);

        // Wait for data (RXNE flag)
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_RXNE));

        // Generate STOP (if not repeated start)
        if (Sr == I2C_SR_DISABLE) {
            I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
        }

        // Read the single byte
        *pRxBuffer = pI2CHandle->pI2Cx->DR;
    }

    // === MULTI-BYTE RECEPTION ===
    if (Len > 1)
    {
        // Clear ADDR flag
        I2C_ClearADDRFlag(pI2CHandle);

        // Read all bytes
        for (uint32_t i = Len; i > 0; i++)
        {
            // Wait for data ready
            while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_RXNE));

            // CRITICAL: When 2 bytes remain
            if (i == 2)
            {
                // Disable ACK to send NACK for last byte
                I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

                // Generate STOP
                if (Sr == I2C_SR_DISABLE) {
                    I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
                }
            }

            // Read data byte
            *pRxBuffer = pI2CHandle->pI2Cx->DR;
            pRxBuffer++;
        }
    }

    // Re-enable ACK if it was enabled in configuration
    if (pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE) {
        I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
    }
}
```

**ACK/NACK Timing for Multi-byte Reception:**
```
Byte N-2      Byte N-1      Byte N (last)
   │             │               │
   ▼             ▼               ▼
┌──────┬──────┬──────┬──────┬──────┬──────┐
│  D   │ ACK  │  D   │ ACK  │  D   │ NACK │
└──────┴──────┴──────┴──────┴──────┴──────┘
          │             │             │
          │             └─── Disable ACK here (when i==2)
          │                  Generate STOP here
          └─── ACK is still sent
```

**Why Disable ACK When i==2?**
- When 2 bytes remain (i==2), we're about to read the second-to-last byte
- We need to send NACK after the LAST byte
- ACK must be disabled BEFORE reading the last byte
- This gives hardware time to prepare NACK for the final byte

### Step 6: Interrupt Mode Implementation

Interrupt mode allows non-blocking I2C communication, freeing the CPU for other tasks.

#### Initiating Interrupt-based Transmission

```c
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer,
                             uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    uint8_t busystate = pI2CHandle->TxRxState;

    // Check if I2C is not already busy
    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        // 1. Save transaction parameters in handle
        pI2CHandle->pTxBuffer = pTxBuffer;
        pI2CHandle->TxLen = Len;
        pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
        pI2CHandle->DeviceAddress = SlaveAddr;
        pI2CHandle->Sr = Sr;

        // 2. Generate START condition
        //    This will trigger SB interrupt
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

        // 3. Enable interrupts
        //    ITBUFEN: Buffer interrupts (TXE, RXNE)
        //    ITEVTEN: Event interrupts (SB, ADDR, BTF, STOPF)
        //    ITERREN: Error interrupts (BERR, ARLO, AF, OVR, TIMEOUT)
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
    }

    return busystate;
}
```

#### Event Interrupt Handler

The event handler manages the state machine for I2C communication.

```c
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t temp1, temp2, temp3;

    // Check if event interrupts are enabled
    temp1 = pI2CHandle->pI2Cx->CR2 & (1 << I2C_CR2_ITEVTEN);
    temp2 = pI2CHandle->pI2Cx->CR2 & (1 << I2C_CR2_ITBUFEN);

    // ===== 1. Handle SB (Start Bit) Event =====
    // SB is set after START condition is generated
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_SB);
    if (temp1 && temp3)
    {
        // Execute address phase based on TX/RX state
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX) {
            I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx,
                                         pI2CHandle->DeviceAddress);
        } else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX) {
            I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx,
                                        pI2CHandle->DeviceAddress);
        }
        // Writing to DR clears SB flag
    }

    // ===== 2. Handle ADDR Event =====
    // ADDR is set when address is sent and ACK received
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_ADDR);
    if (temp1 && temp3)
    {
        // Clear ADDR flag (handles single-byte RX case)
        I2C_ClearADDRFlag(pI2CHandle);
    }

    // ===== 3. Handle BTF (Byte Transfer Finished) Event =====
    // BTF is set when both shift register and DR are empty (TX)
    // or when both shift register and DR are full (RX)
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_BTF);
    if (temp1 && temp3)
    {
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            // Check if TXE is also set
            if (pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TXE))
            {
                // BTF=1, TXE=1: Both registers empty, transmission done
                if (pI2CHandle->TxLen == 0)
                {
                    // Generate STOP if not repeated start
                    if (pI2CHandle->Sr == I2C_SR_DISABLE) {
                        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
                    }

                    // Close transmission
                    I2C_CloseSendData(pI2CHandle);

                    // Notify application
                    I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
                }
            }
        }
    }

    // ===== 4. Handle STOPF Event (Slave Mode) =====
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_STOPF);
    if (temp1 && temp3)
    {
        // Clear STOPF by reading SR1 and writing to CR1
        pI2CHandle->pI2Cx->CR1 |= 0x0000;

        // Notify application
        I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
    }

    // ===== 5. Handle TXE (Transmit Buffer Empty) Event =====
    // TXE is set when DR is empty and ready for next byte
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TXE);
    if (temp1 && temp2 && temp3)  // Requires ITBUFEN enabled
    {
        // Check device is in master mode
        if (pI2CHandle->pI2Cx->SR2 & (1 << I2C_SR2_MSL))
        {
            if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX) {
                I2C_MasterHandleTXEInterrupt(pI2CHandle);
            }
        }
    }

    // ===== 6. Handle RXNE (Receive Buffer Not Empty) Event =====
    // RXNE is set when DR contains received data
    temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_RXNE);
    if (temp1 && temp2 && temp3)  // Requires ITBUFEN enabled
    {
        // Check device is in master mode
        if (pI2CHandle->pI2Cx->SR2 & (1 << I2C_SR2_MSL))
        {
            I2C_MasterHandleRXNEInterrupt(pI2CHandle);
        }
    }
}
```

#### Error Interrupt Handler

```c
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t temp1, temp2;

    // Check if error interrupts are enabled
    temp2 = pI2CHandle->pI2Cx->CR2 & (1 << I2C_CR2_ITERREN);

    // ===== Bus Error =====
    // Misplaced START or STOP condition detected
    temp1 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_BERR);
    if (temp1 && temp2)
    {
        // Clear BERR flag by writing 0
        pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_BERR);

        // Notify application
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
    }

    // ===== Arbitration Lost Error =====
    // Lost arbitration in multi-master scenario
    temp1 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_ARLO);
    if (temp1 && temp2)
    {
        pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_ARLO);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
    }

    // ===== Acknowledge Failure =====
    // Slave did not acknowledge
    temp1 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_AF);
    if (temp1 && temp2)
    {
        pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_AF);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
    }

    // ===== Overrun/Underrun Error =====
    temp1 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_OVR);
    if (temp1 && temp2)
    {
        pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_OVR);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
    }

    // ===== Timeout Error =====
    // SCL held LOW for too long
    temp1 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TIMEOUT);
    if (temp1 && temp2)
    {
        pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_TIMEOUT);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_TIMEOUT);
    }
}
```

### Step 7: NVIC Configuration

NVIC (Nested Vectored Interrupt Controller) must be configured for interrupt mode.

```c
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (IRQNumber <= 31) {
            // ISER0: Interrupts 0-31
            *NVIC_ISER0 |= (1 << IRQNumber);
        } else if (IRQNumber < 64) {
            // ISER1: Interrupts 32-63
            *NVIC_ISER1 |= (1 << (IRQNumber % 32));
        } else if (IRQNumber < 96) {
            // ISER2: Interrupts 64-95
            *NVIC_ISER2 |= (1 << (IRQNumber % 64));
        }
    } else {
        // Similar for ICER registers to disable interrupts
    }
}

void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    // 1. Calculate IPR register index
    uint8_t iprx = IRQNumber / 4;

    // 2. Calculate section within IPR register
    uint8_t iprx_section = IRQNumber % 4;

    // 3. Calculate shift amount
    //    Each section is 8 bits, but only upper 4 bits are implemented
    uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

    // 4. Set priority
    *(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFF << (8 * iprx_section));
    *(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}
```

**I2C IRQ Numbers (STM32F411):**
```
I2C1_EV: 31 (Event interrupt)
I2C1_ER: 32 (Error interrupt)
I2C2_EV: 33
I2C2_ER: 34
I2C3_EV: 72
I2C3_ER: 73
```

---

## Detailed API Reference

### Initialization APIs

#### `void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)`
**Purpose:** Enable or disable peripheral clock for I2C module

**Parameters:**
- `pI2Cx`: Pointer to I2C peripheral (I2C1, I2C2, or I2C3)
- `EnorDi`: ENABLE or DISABLE

**Usage:**
```c
I2C_PeriClockControl(I2C1, ENABLE);
```

---

#### `void I2C_Init(I2C_Handle_t *pI2CHandle)`
**Purpose:** Initialize I2C peripheral with configuration parameters

**Parameters:**
- `pI2CHandle`: Pointer to I2C handle structure containing configuration

**Preconditions:**
- Peripheral clock must be enabled
- Peripheral must be disabled (PE=0)

**Configuration Steps:**
1. Enables peripheral clock
2. Configures ACK control
3. Sets FREQ field in CR2
4. Programs own address (OAR1)
5. Calculates and sets CCR value
6. Configures TRISE register

**Usage:**
```c
I2C_Handle_t I2C1Handle;

I2C1Handle.pI2Cx = I2C1;
I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;  // 100kHz
I2C1Handle.I2C_Config.I2C_DeviceAddress = 0x61;
I2C1Handle.I2C_Config.I2C_ACKControl = I2C_ACK_ENABLE;
I2C1Handle.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;

I2C_Init(&I2C1Handle);
```

---

#### `void I2C_DeInit(I2C_RegDef_t *pI2Cx)`
**Purpose:** Reset I2C peripheral to default state

**Parameters:**
- `pI2Cx`: Pointer to I2C peripheral

**Effect:** Uses RCC reset registers to reset peripheral

---

### Data Transfer APIs (Polling Mode)

#### `void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)`
**Purpose:** Send data to slave device in master mode (blocking)

**Parameters:**
- `pI2CHandle`: Pointer to I2C handle
- `pTxBuffer`: Pointer to transmit buffer
- `Len`: Number of bytes to send
- `SlaveAddr`: 7-bit slave address
- `Sr`: Repeated start flag (I2C_SR_ENABLE or I2C_SR_DISABLE)

**Sequence:**
1. Generate START
2. Send address with write bit
3. Send data bytes
4. Generate STOP (if Sr disabled)

**Usage:**
```c
uint8_t data[] = {0x51, 0x52, 0x53};
I2C_MasterSendData(&I2C1Handle, data, 3, 0x68, I2C_SR_DISABLE);
```

---

#### `void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)`
**Purpose:** Receive data from slave device in master mode (blocking)

**Parameters:**
- `pI2CHandle`: Pointer to I2C handle
- `pRxBuffer`: Pointer to receive buffer
- `Len`: Number of bytes to receive
- `SlaveAddr`: 7-bit slave address
- `Sr`: Repeated start flag

**Special Handling:**
- Single-byte: ACK disabled before clearing ADDR
- Multi-byte: ACK disabled when 2 bytes remain

**Usage:**
```c
uint8_t rxbuf[10];
I2C_MasterReceiveData(&I2C1Handle, rxbuf, 10, 0x68, I2C_SR_DISABLE);
```

---

### Data Transfer APIs (Interrupt Mode)

#### `uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)`
**Purpose:** Send data to slave device in master mode (non-blocking)

**Parameters:** Same as polling mode

**Returns:**
- Current busy state (I2C_READY, I2C_BUSY_IN_TX, I2C_BUSY_IN_RX)

**Effect:**
- Saves parameters in handle
- Generates START
- Enables interrupts
- Returns immediately

**Usage:**
```c
uint8_t data[] = {0x51, 0x52, 0x53};
uint8_t status = I2C_MasterSendDataIT(&I2C1Handle, data, 3, 0x68, I2C_SR_DISABLE);

if (status == I2C_READY) {
    // Transaction initiated successfully
}
```

---

#### `uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)`
**Purpose:** Receive data from slave device in master mode (non-blocking)

**Parameters:** Same as polling mode

**Returns:** Current busy state

---

### Interrupt APIs

#### `void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)`
**Purpose:** Enable/disable I2C interrupt in NVIC

**Parameters:**
- `IRQNumber`: IRQ number (e.g., IRQ_I2C1_EV, IRQ_I2C1_ER)
- `EnorDi`: ENABLE or DISABLE

**Usage:**
```c
I2C_IRQInterruptConfig(IRQ_I2C1_EV, ENABLE);
I2C_IRQInterruptConfig(IRQ_I2C1_ER, ENABLE);
```

---

#### `void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)`
**Purpose:** Set interrupt priority

**Parameters:**
- `IRQNumber`: IRQ number
- `IRQPriority`: Priority level (0-15, where 0 is highest)

**Usage:**
```c
I2C_IRQPriorityConfig(IRQ_I2C1_EV, NVIC_IRQ_PRI0);
I2C_IRQPriorityConfig(IRQ_I2C1_ER, NVIC_IRQ_PRI1);
```

---

#### `void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)`
**Purpose:** Handle I2C event interrupts (SB, ADDR, BTF, TXE, RXNE, STOPF)

**Called From:** Event interrupt service routine

**Usage:**
```c
void I2C1_EV_IRQHandler(void)
{
    I2C_EV_IRQHandling(&I2C1Handle);
}
```

---

#### `void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)`
**Purpose:** Handle I2C error interrupts (BERR, ARLO, AF, OVR, TIMEOUT)

**Called From:** Error interrupt service routine

**Usage:**
```c
void I2C1_ER_IRQHandler(void)
{
    I2C_ER_IRQHandling(&I2C1Handle);
}
```

---

### Utility APIs

#### `void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)`
**Purpose:** Enable/disable I2C peripheral (PE bit)

**Parameters:**
- `pI2Cx`: Pointer to I2C peripheral
- `EnOrDi`: ENABLE or DISABLE

**Usage:**
```c
I2C_PeripheralControl(I2C1, ENABLE);  // Enable before communication
```

---

#### `void I2C_ManageACK(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)`
**Purpose:** Enable/disable ACK bit

**Parameters:**
- `pI2Cx`: Pointer to I2C peripheral
- `EnOrDi`: I2C_ACK_ENABLE or I2C_ACK_DISABLE

**Usage:**
```c
I2C_ManageACK(I2C1, I2C_ACK_ENABLE);
```

---

#### `uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)`
**Purpose:** Check status of I2C flag

**Parameters:**
- `pI2Cx`: Pointer to I2C peripheral
- `FlagName`: Flag to check (e.g., I2C_FLAG_SR1_TXE)

**Returns:** FLAG_SET or FLAG_RESET

**Usage:**
```c
if (I2C_GetFlagStatus(I2C1, I2C_FLAG_SR1_TXE) == FLAG_SET) {
    // Transmit buffer is empty
}
```

---

#### `void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)`
**Purpose:** Close interrupt-based transmission

**Effect:**
- Disables buffer and event interrupts
- Resets state to I2C_READY
- Clears buffer pointers

---

#### `void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)`
**Purpose:** Close interrupt-based reception

**Effect:**
- Disables interrupts
- Resets state
- Re-enables ACK if configured

---

### Application Callback

#### `void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)`
**Purpose:** Application-defined callback for I2C events

**Parameters:**
- `pI2CHandle`: Pointer to I2C handle
- `AppEv`: Event type

**Events:**
```c
I2C_EV_TX_CMPLT      // Transmission complete
I2C_EV_RX_CMPLT      // Reception complete
I2C_EV_STOP          // STOP detected
I2C_ERROR_BERR       // Bus error
I2C_ERROR_ARLO       // Arbitration lost
I2C_ERROR_AF         // Acknowledge failure
I2C_ERROR_OVR        // Overrun/underrun
I2C_ERROR_TIMEOUT    // Timeout error
```

**Implementation (User Application):**
```c
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
    if (AppEv == I2C_EV_TX_CMPLT) {
        // Transmission complete - handle in application
        printf("I2C TX Complete\n");
    } else if (AppEv == I2C_ERROR_AF) {
        // Acknowledge failure - slave not responding
        printf("I2C Error: No ACK from slave\n");
    }
    // Handle other events...
}
```

---

## Common Pitfalls and How to Avoid Them

### 1. Incorrect Clock Configuration

**Problem:**
```c
// WRONG: CCR value too small or zero
ccr_value = 0;  // Results in invalid SCL frequency
```

**Consequence:** I2C communication fails, bus hangs, or operates at wrong speed.

**Solution:**
```c
// Verify PCLK1 frequency first
uint32_t pclk1 = RCC_GetPCLK1Value();
printf("PCLK1 = %lu Hz\n", pclk1);

// Calculate CCR correctly for Standard Mode (100kHz)
uint16_t ccr_value = pclk1 / (2 * 100000);

// Verify CCR is in valid range
if (ccr_value < 4) {
    // CCR too small, PCLK1 might be too low
    // Minimum CCR = 4 for Standard Mode
}
```

**Best Practice:**
- Always verify PCLK1 frequency matches your system clock configuration
- Check that CCR ≥ 4 for Standard Mode, CCR ≥ 1 for Fast Mode
- Use oscilloscope to verify actual SCL frequency

### 2. ADDR Flag Not Cleared Properly

**Problem:**
```c
// WRONG: Only reading SR1
dummy = pI2C->SR1;  // ADDR flag NOT cleared!
```

**Consequence:** SCL remains stretched (held LOW), bus hangs.

**Solution:**
```c
// CORRECT: Read both SR1 and SR2 to clear ADDR
uint32_t dummy;
dummy = pI2C->SR1;
dummy = pI2C->SR2;
(void)dummy;  // Prevent compiler optimization
```

**Why?**
- ADDR flag is cleared by reading SR1 followed by SR2 (hardware requirement)
- This is NOT documented clearly in some reference manuals
- Failure to clear ADDR causes clock stretching indefinitely

### 3. Incorrect ACK Timing in Reception

**Problem:**
```c
// WRONG: ACK disabled too late for single-byte reception
I2C_ClearADDRFlag(pI2C);  // Clear ADDR first
I2C_ManageACK(pI2C, DISABLE);  // TOO LATE! ACK already sent
```

**Consequence:** Master sends ACK instead of NACK, slave may send more data.

**Solution:**
```c
// CORRECT: Disable ACK BEFORE clearing ADDR for single-byte RX
if (Len == 1) {
    I2C_ManageACK(pI2C, DISABLE);  // First disable ACK
    I2C_ClearADDRFlag(pI2C);        // Then clear ADDR
}
```

**Timing Diagram:**
```
WRONG Sequence:
ADDR Clear → ACK sent automatically → Disable ACK (too late!)

CORRECT Sequence:
Disable ACK → ADDR Clear → NACK will be sent
```

### 4. Missing PE (Peripheral Enable) Control

**Problem:**
```c
// WRONG: Configuring while peripheral is enabled
I2C1->CR1 |= (1 << I2C_CR1_PE);  // Enable PE
I2C_Init(&I2CHandle);             // Configure (WRONG!)
```

**Consequence:** Configuration changes may not take effect, undefined behavior.

**Solution:**
```c
// CORRECT: Disable PE before configuration
I2C_Init(&I2CHandle);             // Configure with PE=0
I2C_PeripheralControl(I2C1, ENABLE);  // Enable after config
```

**Best Practice:**
- Always configure I2C with PE=0 (peripheral disabled)
- Enable PE only after complete initialization
- Some registers (like CCR, TRISE) can ONLY be written when PE=0

### 5. Forgetting Bit 14 in OAR1

**Problem:**
```c
// WRONG: Bit 14 of OAR1 not set
I2C1->OAR1 = (0x61 << 1);  // Missing bit 14!
```

**Consequence:** Undefined behavior in address matching (slave mode).

**Solution:**
```c
// CORRECT: Always keep bit 14 at 1 (reference manual requirement)
I2C1->OAR1 = (0x61 << 1) | (1 << 14);
```

**Reference Manual Quote:**
> "Bit 14 should always be kept at 1 by software."

### 6. Buffer Overflow in Interrupt Mode

**Problem:**
```c
// WRONG: Buffer goes out of scope
void send_data(void) {
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    I2C_MasterSendDataIT(&I2CHandle, data, 10, 0x68, I2C_SR_DISABLE);
}  // data[] destroyed here, but interrupt still tries to access it!
```

**Consequence:** Interrupt handler accesses invalid memory, causes hard fault.

**Solution:**
```c
// CORRECT: Use static or global buffer for interrupt mode
static uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void send_data(void) {
    I2C_MasterSendDataIT(&I2CHandle, data, 10, 0x68, I2C_SR_DISABLE);
}
```

**Best Practice:**
- For interrupt mode, buffers must remain valid until transaction completes
- Use global, static, or dynamically allocated buffers
- Wait for I2C_EV_TX_CMPLT callback before reusing buffer

### 7. Not Re-enabling ACK After Reception

**Problem:**
```c
// WRONG: ACK left disabled after reception
I2C_MasterReceiveData(...);  // ACK disabled at end
// Next reception fails because ACK is still disabled!
```

**Consequence:** Subsequent receptions send NACK prematurely.

**Solution:**
```c
// CORRECT: Re-enable ACK if configured
if (pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE) {
    I2C_ManageACK(pI2C, I2C_ACK_ENABLE);
}
```

**Your Driver:** Already handles this correctly in [stm32f411xx_i2c_driver.c:557-560](stm32f411xx_i2c_driver.c#L557-L560)

### 8. Incorrect STOP Timing for Multi-byte Reception

**Problem:**
```c
// WRONG: STOP generated too early
if (i == 1) {  // Last byte
    I2C_GenerateStopCondition(pI2C);  // STOP before reading!
}
*pRxBuffer = pI2C->DR;
```

**Consequence:** STOP interrupts data reception, last byte may be lost.

**Solution:**
```c
// CORRECT: Generate STOP when 2 bytes remain (before last byte is received)
if (i == 2) {
    I2C_ManageACK(pI2C, DISABLE);     // Disable ACK
    I2C_GenerateStopCondition(pI2C);  // Generate STOP
}
// Continue reading...
```

### 9. Unhandled Error Interrupts

**Problem:**
```c
// WRONG: Enabling error interrupts but not handling them
pI2C->CR2 |= (1 << I2C_CR2_ITERREN);
// No I2C_ER_IRQHandler defined!
```

**Consequence:** System hangs in default handler when error occurs.

**Solution:**
```c
// CORRECT: Always implement error handler
void I2C1_ER_IRQHandler(void)
{
    I2C_ER_IRQHandling(&I2C1Handle);
}

// In callback, handle errors appropriately
void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t ev)
{
    switch (ev) {
        case I2C_ERROR_AF:
            // Slave not responding - reset and retry
            I2C_DeInit(pH->pI2Cx);
            I2C_Init(pH);
            break;
        case I2C_ERROR_TIMEOUT:
            // Bus stuck - attempt recovery
            recover_i2c_bus();
            break;
        // Handle other errors...
    }
}
```

### 10. I2C Bus Lockup Recovery Not Implemented

**Problem:**
- Bus gets stuck with SDA held LOW
- No recovery mechanism implemented

**Consequence:** I2C communication permanently fails until power cycle.

**Solution - Manual Bus Recovery:**
```c
void I2C_BusReset(I2C_RegDef_t *pI2Cx)
{
    // Method 1: Software reset
    pI2Cx->CR1 |= (1 << I2C_CR1_SWRST);
    delay_us(10);
    pI2Cx->CR1 &= ~(1 << I2C_CR1_SWRST);

    // Method 2: Clock stretching (send 9 clock pulses)
    // Configure SCL pin as GPIO output
    // Toggle SCL 9 times to allow slave to release SDA
    // Reconfigure pins back to I2C alternate function
}
```

**Best Practice:**
- Implement bus recovery function
- Call on timeout or bus error
- Monitor I2C_SR2_BUSY flag before initiating transactions

### 11. Clock Speed Mismatch

**Problem:**
```c
// System configured for 84 MHz, but driver assumes 16 MHz
uint32_t RCC_GetPCLK1Value(void)
{
    return 16000000;  // Hardcoded! Wrong if using PLL
}
```

**Consequence:** I2C runs at wrong speed, communication fails.

**Solution:**
```c
// CORRECT: Dynamically calculate PCLK1
uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t SystemClk, pclk1;
    uint8_t clksrc = (RCC->CFGR >> 2) & 0x3;

    if (clksrc == 0) {
        SystemClk = 16000000;  // HSI
    } else if (clksrc == 1) {
        SystemClk = 8000000;   // HSE
    } else if (clksrc == 2) {
        SystemClk = RCC_GetPLLOutputClock();  // PLL
    }

    // Apply AHB prescaler
    // Apply APB1 prescaler
    pclk1 = calculate_pclk1(SystemClk);

    return pclk1;
}
```

**Your Driver:** Correctly implemented in [stm32f411xx_i2c_driver.c:218-266](stm32f411xx_i2c_driver.c#L218-L266)

### 12. Repeated Start Implementation Error

**Problem:**
```c
// WRONG: STOP generated before repeated START
I2C_GenerateStopCondition(pI2C);
I2C_GenerateStartCondition(pI2C);  // This is not repeated start!
```

**Consequence:** Bus becomes idle, not a true repeated start condition.

**Solution:**
```c
// CORRECT: Generate START without preceding STOP
I2C_MasterSendData(..., I2C_SR_ENABLE);    // Sr flag enables repeated start
I2C_MasterReceiveData(..., I2C_SR_ENABLE); // No STOP generated

// Sequence: S-ADDR-DATA-Sr-ADDR-DATA-P
```

### 13. Interrupt Priority Configuration Issues

**Problem:**
```c
// WRONG: Event interrupt has lower priority than error interrupt
I2C_IRQPriorityConfig(IRQ_I2C1_EV, 5);
I2C_IRQPriorityConfig(IRQ_I2C1_ER, 2);
```

**Consequence:** Error handling may interrupt normal event processing, causing race conditions.

**Solution:**
```c
// CORRECT: Event and error interrupts should have same or close priority
I2C_IRQPriorityConfig(IRQ_I2C1_EV, 2);
I2C_IRQPriorityConfig(IRQ_I2C1_ER, 2);
```

**Best Practice:**
- Event and error interrupts should have equal priority
- Or error priority slightly higher to catch issues quickly

### 14. GPIO Alternate Function Not Configured

**Problem:**
```c
// WRONG: GPIO pins not configured for I2C
GPIO_Init(&GPIOHandle);  // Configured as GPIO, not I2C!
I2C_Init(&I2CHandle);    // Won't work, pins not connected to I2C
```

**Consequence:** I2C signals don't reach the pins, no communication.

**Solution:**
```c
// CORRECT: Configure GPIO for I2C alternate function
GPIO_Handle_t I2CPins;

I2CPins.pGPIOx = GPIOB;
I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;  // AF4 for I2C
I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;  // Open-drain
I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU; // Pull-up

// SCL - PB6 (I2C1)
I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
GPIO_Init(&I2CPins);

// SDA - PB7 (I2C1)
I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
GPIO_Init(&I2CPins);
```

**Critical Configuration:**
- **Mode:** Alternate Function
- **Output Type:** Open-Drain (required for I2C)
- **Pull-up:** Internal or external pull-ups required
- **Speed:** Typically medium or high

### 15. Missing External Pull-up Resistors

**Problem:**
- Relying only on internal weak pull-ups (30-50kΩ)
- No external pull-up resistors on SDA/SCL lines

**Consequence:**
- Signal rise time too slow, especially for Fast Mode
- Communication unreliable or fails completely

**Solution:**
```
Add external pull-up resistors:

VDD (3.3V or 5V)
    │
    ├─── 4.7kΩ (Standard Mode) or 2.2kΩ (Fast Mode)
    │                │
   SDA              SCL
```

**Pull-up Resistor Calculation:**
```
R_min = (V_DD - V_OL(max)) / I_OL
R_max = t_r / (0.8473 × C_b)

Where:
- V_DD = Supply voltage
- V_OL = LOW level output voltage
- I_OL = LOW level output current (3 mA for I2C)
- t_r = Maximum rise time (1000ns SM, 300ns FM)
- C_b = Bus capacitance
```

**Typical Values:**
- Standard Mode (100kHz): 4.7kΩ - 10kΩ
- Fast Mode (400kHz): 2.2kΩ - 4.7kΩ

---

## Testing Strategies

### 1. Loopback Testing

Connect I2C peripheral to itself in slave mode for basic functionality testing.

### 2. Logic Analyzer Verification

**What to Check:**
- START and STOP conditions properly formed
- Address byte correct (7 bits + R/W)
- ACK/NACK timing correct
- SCL frequency matches configuration
- Rise/fall times within specifications

**Example Logic Analyzer Output:**
```
START | 0x68 | W | ACK | 0x51 | ACK | 0x52 | ACK | STOP
  S   | Addr | 0 | A   | Data | A   | Data | A   |  P
```

### 3. Common Slave Devices for Testing

- **EEPROM (24C256):** Simple read/write operations
- **RTC (DS1307):** Time read/write with register addressing
- **Sensors (MPU6050, BMP280):** Multi-byte read operations
- **I/O Expander (PCF8574):** Basic TX/RX testing

### 4. Test Scenarios

#### Basic Communication Test
```c
// Test 1: Single byte write
uint8_t cmd = 0x51;
I2C_MasterSendData(&I2C1Handle, &cmd, 1, 0x68, I2C_SR_DISABLE);

// Test 2: Multi-byte write
uint8_t data[] = {0x51, 0x52, 0x53, 0x54};
I2C_MasterSendData(&I2C1Handle, data, 4, 0x68, I2C_SR_DISABLE);

// Test 3: Single byte read
uint8_t rxdata;
I2C_MasterReceiveData(&I2C1Handle, &rxdata, 1, 0x68, I2C_SR_DISABLE);

// Test 4: Multi-byte read
uint8_t rxbuf[10];
I2C_MasterReceiveData(&I2C1Handle, rxbuf, 10, 0x68, I2C_SR_DISABLE);
```

#### Register Read/Write Test (Common Pattern)
```c
// Write to register
void I2C_WriteRegister(uint8_t slaveAddr, uint8_t regAddr, uint8_t data)
{
    uint8_t txbuf[2] = {regAddr, data};
    I2C_MasterSendData(&I2C1Handle, txbuf, 2, slaveAddr, I2C_SR_DISABLE);
}

// Read from register
uint8_t I2C_ReadRegister(uint8_t slaveAddr, uint8_t regAddr)
{
    uint8_t data;

    // Write register address with repeated start
    I2C_MasterSendData(&I2C1Handle, &regAddr, 1, slaveAddr, I2C_SR_ENABLE);

    // Read data from register
    I2C_MasterReceiveData(&I2C1Handle, &data, 1, slaveAddr, I2C_SR_DISABLE);

    return data;
}
```

#### Interrupt Mode Test
```c
volatile uint8_t txComplete = 0;

void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t ev)
{
    if (ev == I2C_EV_TX_CMPLT) {
        txComplete = 1;
    }
}

void test_interrupt_mode(void)
{
    static uint8_t data[] = {0x51, 0x52, 0x53};

    // Configure interrupts
    I2C_IRQInterruptConfig(IRQ_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_I2C1_ER, ENABLE);
    I2C_IRQPriorityConfig(IRQ_I2C1_EV, 2);

    // Initiate transmission
    I2C_MasterSendDataIT(&I2C1Handle, data, 3, 0x68, I2C_SR_DISABLE);

    // Wait for completion
    while (!txComplete);

    // Process next task...
}
```

### 5. Error Injection Testing

Test error handling by deliberately causing errors:

```c
// Test 1: No slave present (AF error)
uint8_t data = 0x51;
I2C_MasterSendData(&I2C1Handle, &data, 1, 0xFF, I2C_SR_DISABLE);
// Expected: I2C_ERROR_AF in error callback

// Test 2: Bus busy (attempt transaction while bus is busy)
// Expected: Handle busy state appropriately

// Test 3: Clock stretch timeout
// Configure slave to hold SCL LOW indefinitely
// Expected: I2C_ERROR_TIMEOUT
```

### 6. Stress Testing

```c
// Rapid repeated transmissions
for (int i = 0; i < 1000; i++) {
    uint8_t data = i & 0xFF;
    I2C_MasterSendData(&I2C1Handle, &data, 1, 0x68, I2C_SR_DISABLE);
    delay_ms(1);
}

// Large data transfers
uint8_t largeBuffer[256];
for (int i = 0; i < 256; i++) {
    largeBuffer[i] = i;
}
I2C_MasterSendData(&I2C1Handle, largeBuffer, 256, 0x68, I2C_SR_DISABLE);
```

---

## Performance Considerations

### 1. Clock Speed Selection

**Standard Mode (100 kHz):**
- **Advantages:** More reliable, longer cables, less noise sensitivity
- **Disadvantages:** Slower data transfer
- **Use When:** Long cables, noisy environment, multiple slaves

**Fast Mode (400 kHz):**
- **Advantages:** 4x faster than Standard Mode
- **Disadvantages:** More sensitive to capacitance, shorter cables
- **Use When:** Short traces on PCB, fewer slaves, need higher throughput

**Fast Mode Plus (1 MHz):**
- **Advantages:** 10x faster than Standard Mode
- **Disadvantages:** Strict PCB layout requirements, special drivers needed
- **Use When:** High-speed sensors, same PCB communication

### 2. Polling vs Interrupt Mode

**Polling Mode:**
- **Advantages:** Simple, deterministic, no interrupt overhead
- **Disadvantages:** Blocks CPU, wastes cycles waiting
- **Use When:** Simple applications, single-threaded, short transactions

**Interrupt Mode:**
- **Advantages:** Non-blocking, CPU can do other work
- **Disadvantages:** More complex, interrupt latency affects timing
- **Use When:** RTOS environment, long transactions, multiple peripherals

**Performance Comparison:**
```
Polling Mode:
- CPU Utilization: 100% during transaction
- Response Time: Immediate (no interrupt latency)
- Code Complexity: Low

Interrupt Mode:
- CPU Utilization: ~5-10% (interrupt overhead only)
- Response Time: Interrupt latency + handler time
- Code Complexity: Medium-High
```

### 3. DMA Mode (Future Enhancement)

DMA can further reduce CPU utilization for large transfers:

```
DMA Mode Benefits:
- CPU Utilization: <1% (setup only)
- Throughput: Maximum (limited only by I2C clock)
- Suitable For: Large data transfers (>10 bytes)
```

**Implementation Outline:**
```c
// Configure DMA for I2C TX
DMA_Config.Channel = DMA_Channel_1;
DMA_Config.Direction = DMA_DIR_MemoryToPeripheral;
DMA_Config.PeriphAddr = (uint32_t)&I2C1->DR;
DMA_Config.MemoryAddr = (uint32_t)txBuffer;
DMA_Config.DataSize = bufferSize;
DMA_Init(&DMA_Config);

// Enable I2C DMA request
I2C1->CR2 |= (1 << I2C_CR2_DMAEN);

// Start DMA transfer
DMA_Start(&DMA_Config);
```

### 4. Transaction Timing

**Typical Transaction Times:**

Standard Mode (100 kHz):
```
START: ~50 μs
Address byte (8 bits + ACK): ~90 μs
Data byte (8 bits + ACK): ~90 μs
STOP: ~50 μs

Example: Write 10 bytes
Time = 50 + 90 + (10 × 90) + 50 = 1090 μs ≈ 1.1 ms
```

Fast Mode (400 kHz):
```
Same transaction: ~275 μs (4x faster)
```

### 5. Optimization Tips

**1. Burst Transfers:**
```c
// SLOW: Multiple single-byte transactions
for (int i = 0; i < 10; i++) {
    I2C_WriteRegister(addr, reg + i, data[i]);
    // Each iteration: START-ADDR-REG-DATA-STOP overhead
}

// FAST: Single multi-byte transaction
I2C_WriteRegisterBurst(addr, reg, data, 10);
// One transaction: START-ADDR-REG-DATA[0..9]-STOP
```

**2. Repeated Start Usage:**
```c
// SLOW: Separate transactions
I2C_MasterSendData(..., I2C_SR_DISABLE);  // START-ADDR-DATA-STOP
I2C_MasterReceiveData(..., I2C_SR_DISABLE);  // START-ADDR-DATA-STOP

// FAST: Repeated start
I2C_MasterSendData(..., I2C_SR_ENABLE);   // START-ADDR-DATA-Sr
I2C_MasterReceiveData(..., I2C_SR_DISABLE);  // ADDR-DATA-STOP
// Saves one START and STOP sequence
```

**3. Minimize Clock Configuration Changes:**
```c
// SLOW: Reconfigure for each transaction
I2C_Init(&I2CHandle);  // Time-consuming
I2C_MasterSendData(...);

// FAST: Configure once, use many times
I2C_Init(&I2CHandle);  // Once at startup
for (...) {
    I2C_MasterSendData(...);
}
```

---

## Appendix

### A. I2C Speed Specifications

| Mode | Max Frequency | Rise Time | Fall Time |
|------|---------------|-----------|-----------|
| Standard Mode | 100 kHz | 1000 ns | 300 ns |
| Fast Mode | 400 kHz | 300 ns | 300 ns |
| Fast Mode Plus | 1 MHz | 120 ns | 120 ns |
| High Speed | 3.4 MHz | 80 ns | 80 ns |

### B. STM32F411 I2C Pin Mapping

**I2C1:**
- SCL: PB6 (AF4), PB8 (AF4)
- SDA: PB7 (AF4), PB9 (AF4)

**I2C2:**
- SCL: PB10 (AF4), PF1 (AF4)
- SDA: PB3 (AF9), PB9 (AF9), PF0 (AF4)

**I2C3:**
- SCL: PA8 (AF4)
- SDA: PC9 (AF4)

### C. Common I2C Slave Addresses

| Device | Address (7-bit) |
|--------|-----------------|
| AT24C256 EEPROM | 0x50 - 0x57 |
| DS1307 RTC | 0x68 |
| MPU6050 IMU | 0x68 or 0x69 |
| BMP280 Pressure | 0x76 or 0x77 |
| SSD1306 OLED | 0x3C or 0x3D |
| PCF8574 I/O Exp | 0x20 - 0x27 |

### D. Quick Reference - Flag Clearing Methods

| Flag | Clear Method |
|------|--------------|
| SB | Write to DR |
| ADDR | Read SR1, then read SR2 |
| BTF | Read DR or write DR |
| TXE | Write to DR |
| RXNE | Read from DR |
| STOPF | Read SR1, then write to CR1 |
| AF | Write 0 to SR1 bit |
| BERR | Write 0 to SR1 bit |

### E. Register Reset Values

| Register | Reset Value |
|----------|-------------|
| CR1 | 0x0000 |
| CR2 | 0x0000 |
| OAR1 | 0x0000 |
| CCR | 0x0000 |
| TRISE | 0x0002 |
| SR1 | 0x0000 |
| SR2 | 0x0000 |

### F. Code Examples Location

Your repository contains test applications:
- [010_i2c_master_tx_testing.c](Src/010_i2c_master_tx_testing.c): Master TX example
- [011_i2c_master_rx_testing.c](Src/011_i2c_master_rx_testing.c): Master RX example

### G. References

1. **STM32F411xC/xE Reference Manual (RM0383)**
   - Chapter 19: Inter-integrated circuit (I2C) interface

2. **I2C Bus Specification (NXP)**
   - Version 6.0 (April 2014)

3. **STM32F4 I2C Application Notes**
   - AN4235: Using the I2C interface in STM32F0xx, STM32F3xx and STM32F4xx applications

### H. Identified Issues in Current Implementation

#### 1. Bug in I2C_ExecuteAddressPhaseRead()

**Location:** [stm32f411xx_i2c_driver.c:84](stm32f411xx_i2c_driver.c#L84)

```c
// CURRENT (INCORRECT):
SlaveAddr &= ~(0);  // This does nothing! Should be |= 1

// SHOULD BE:
SlaveAddr |= 1;     // Set LSB to 1 for read
```

**Impact:** Read operations may fail because R/W bit is not set correctly.

**Fix Required:** Change line 84 to set the LSB to 1.

#### 2. Missing I2C_ClearADDRFlag() Parameter

**Location:** [stm32f411xx_i2c_driver.c:453, 512, 531, 772](stm32f411xx_i2c_driver.c)

```c
// CURRENT (INCONSISTENT):
I2C_ClearADDRFlag(pI2CHandle->pI2Cx);  // Line 453 - wrong parameter
I2C_ClearADDRFlag(pI2CHandle);          // Line 512 - correct parameter

// Function signature expects I2C_Handle_t*, not I2C_RegDef_t*
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle);
```

**Impact:** Compilation error or incorrect behavior.

**Fix Required:** Use consistent parameter type (I2C_Handle_t*).

#### 3. Incomplete Clock Disable Implementation

**Location:** [stm32f411xx_i2c_driver.c:291-293](stm32f411xx_i2c_driver.c#L291-L293)

```c
else
{
    // TODO: Implement clock disable functionality
}
```

**Impact:** Cannot disable peripheral clock.

**Fix Required:** Implement clock disable for all three I2C peripherals.

---

## Conclusion

This I2C driver implementation demonstrates a solid understanding of the I2C protocol and STM32 peripheral architecture. The driver correctly implements:

**Strengths:**
- Comprehensive register definitions and bit positions
- Both polling and interrupt modes
- Proper flag clearing sequences (especially ADDR flag)
- Correct ACK/NACK management for different reception scenarios
- Clock configuration for Standard and Fast modes
- Error handling framework

**Areas for Enhancement:**
1. Fix the bug in `I2C_ExecuteAddressPhaseRead()` (line 84)
2. Implement clock disable functionality
3. Add DMA support for large transfers
4. Implement slave mode functionality
5. Add bus recovery mechanism
6. Enhance error recovery strategies

The driver provides a strong foundation for I2C communication in embedded systems and follows industry best practices for peripheral driver development.

---

**Document Version:** 1.0
**Date:** December 26, 2025
**Author:** Based on analysis of stm32f411xx_i2c_driver implementation
