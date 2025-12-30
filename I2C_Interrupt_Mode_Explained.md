# I2C Interrupt Mode - Complete Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Key Concepts](#key-concepts)
3. [Polling vs Interrupt Mode Comparison](#polling-vs-interrupt-mode-comparison)
4. [How I2C Interrupt Functions Work](#how-i2c-interrupt-functions-work)
5. [Complete Execution Flow](#complete-execution-flow)
6. [Visual Illustrations](#visual-illustrations)
7. [Common Misconceptions](#common-misconceptions)
8. [Practical Examples](#practical-examples)
9. [Best Practices](#best-practices)

---

## Introduction

This document explains **I2C Interrupt Mode** in detail, clarifying how interrupt-based I2C communication works in STM32 microcontrollers. The focus is on understanding the **purpose**, **mechanism**, and **benefits** of using interrupt mode over polling mode.

### What You'll Learn
- What `I2C_MasterSendDataIT()` and `I2C_MasterReceiveDataIT()` actually do
- The real purpose of interrupt mode (hint: it's NOT about speed!)
- How CPU time is utilized efficiently
- Complete execution flow with timeline

---

## Key Concepts

### What Does "IT" Mean?

**IT = Interrupt**

Functions ending with "IT" are **interrupt-driven** versions of their polling counterparts:
- `I2C_MasterSendData()` → **Polling mode** (blocking)
- `I2C_MasterSendDataIT()` → **Interrupt mode** (non-blocking)

### Critical Understanding: The Real Purpose

**IMPORTANT:** Interrupt mode does NOT make I2C communication faster!

The I2C hardware transmits data at the **same speed** regardless of mode:
- Speed is determined by **SCL clock frequency** (100 kHz, 400 kHz, etc.)
- Hardware sends bits at the configured rate

**What interrupt mode DOES:**
- **Frees the CPU** to do other work while I2C transfer happens
- **Prevents busy-waiting** - CPU isn't stuck in loops
- **Enables multitasking** - handle multiple operations "simultaneously"
- **Improves responsiveness** - system can respond to other events immediately

---

## Polling vs Interrupt Mode Comparison

### Polling Mode (Blocking)

```c
void application_task(void)
{
    uint8_t data[100] = {...};

    // This function BLOCKS for the entire duration
    I2C_MasterSendData(&I2CHandle, data, 100, SLAVE_ADDR, DISABLE);
    // ↑ CPU stuck here for ~10ms at 100kHz

    // This code doesn't run until I2C completes
    UpdateDisplay();
    CheckButtons();
}
```

**CPU Activity:**
```
┌─────────────────────────────────────────────────┐
│         I2C TRANSMISSION (10ms)                 │
│  CPU: Waiting... Waiting... Waiting...          │
│  while(!SB); while(!ADDR); while(!TXE);...     │
│         DOING NOTHING USEFUL                    │
└─────────────────────────────────────────────────┘
                                                  ↓
                                         UpdateDisplay()
                                         CheckButtons()
```

**Characteristics:**
- ✅ Simple code, easy to understand
- ✅ Deterministic timing
- ❌ CPU wastes cycles in busy-waiting
- ❌ Cannot do other work during transfer
- ❌ System appears "frozen" to user

---

### Interrupt Mode (Non-blocking)

```c
void application_task(void)
{
    static uint8_t data[100] = {...};

    // This function RETURNS IMMEDIATELY (within microseconds)
    I2C_MasterSendDataIT(&I2CHandle, data, 100, SLAVE_ADDR, DISABLE);
    // ↑ Only setup, returns instantly

    // This code runs RIGHT AWAY while I2C happens in background
    UpdateDisplay();      // ← Runs immediately
    CheckButtons();       // ← Runs immediately
    ProcessSensorData();  // ← Runs immediately

    // Later, when I2C completes, callback is triggered
}

// Callback executed when I2C finishes
void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    if (Event == I2C_EV_TX_CMPLT)
    {
        // I2C transmission complete!
        // Handle completion here
    }
}
```

**CPU Activity:**
```
Setup  Application Code Running................................ Callback
(5μs)  UpdateDisplay() → CheckButtons() → ProcessData()...    (5μs)
  ↓           ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓  ↓              ↓
  │          [ISR][ISR][ISR][ISR][ISR][ISR][ISR]...           │
  │           5μs  5μs  5μs  5μs  5μs  5μs  5μs               │
  └────────────────── 10ms total ──────────────────────────────┘

Legend:
  Setup    = I2C_MasterSendDataIT() setup (5μs)
  [ISR]    = Brief interrupt to send one byte (5μs each)
  App Code = Your application doing useful work
  Callback = I2C_ApplicationEventCallback() when done
```

**Characteristics:**
- ✅ CPU free to do other work (90-95% of the time)
- ✅ System remains responsive
- ✅ Can handle multiple tasks
- ❌ More complex code
- ❌ Need to manage callbacks

---

## How I2C Interrupt Functions Work

### `I2C_MasterSendDataIT()` Function

**Location:** [stm32f411xx_i2c_driver.c:576-603](../drivers/Src/stm32f411xx_i2c_driver.c#L576-L603)

#### What It Does

```c
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle,
                             uint8_t *pTxBuffer,
                             uint8_t Len,
                             uint8_t SlaveAddr,
                             uint8_t Sr)
{
    // 1. Check if I2C is already busy
    uint8_t busystate = pI2CHandle->TxRxState;
    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        // 2. Save transaction parameters in handle structure
        pI2CHandle->pTxBuffer = pTxBuffer;
        pI2CHandle->TxLen = Len;
        pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
        pI2CHandle->DeviceAddress = SlaveAddr;
        pI2CHandle->Sr = Sr;

        // 3. Generate START condition
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

        // 4. Enable interrupts
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);  // Buffer interrupts
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);  // Event interrupts
        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);  // Error interrupts
    }

    // 5. Return immediately
    return busystate;
}
```

#### Step-by-Step Breakdown

**Step 1: Check Current State**
- Verify I2C is not already busy with another transaction
- If busy, return current state without doing anything

**Step 2: Save Transaction Parameters**
- `pTxBuffer` → Save buffer pointer
- `TxLen` → Save number of bytes to send
- `TxRxState` → Set state to `I2C_BUSY_IN_TX`
- `DeviceAddress` → Save slave address
- `Sr` → Save repeated start flag

**Why save these?**
The ISR (Interrupt Service Routine) needs this information to know:
- Where to read data from (buffer pointer)
- How many bytes to send (length)
- Which slave to communicate with (address)

**Step 3: Generate START Condition**
- Sets START bit in CR1 register
- Hardware generates START condition on the bus
- This will trigger the **SB (Start Bit) interrupt**

**Step 4: Enable Three Types of Interrupts**

1. **ITBUFEN (Bit 10)** - Buffer Interrupts
   - TXE (Transmit buffer Empty)
   - RXNE (Receive buffer Not Empty)

2. **ITEVTEN (Bit 9)** - Event Interrupts
   - SB (Start Bit)
   - ADDR (Address sent)
   - BTF (Byte Transfer Finished)
   - STOPF (Stop detected)

3. **ITERREN (Bit 8)** - Error Interrupts
   - BERR (Bus Error)
   - ARLO (Arbitration Lost)
   - AF (Acknowledge Failure)
   - OVR (Overrun/Underrun)
   - TIMEOUT (SCL timeout)

**Step 5: Return Immediately**
- Function exits in **microseconds**
- Does NOT wait for transmission to complete
- CPU is now free to do other work

#### What It Does NOT Do

❌ Does NOT send the slave address
❌ Does NOT send any data bytes
❌ Does NOT wait for completion
❌ Does NOT generate STOP condition

**All actual data transmission happens in the ISR!**

---

### `I2C_MasterReceiveDataIT()` Function

**Location:** [stm32f411xx_i2c_driver.c:614-642](../drivers/Src/stm32f411xx_i2c_driver.c#L614-L642)

Very similar to transmit, with one key difference:

```c
// Additional parameter saved for RX
pI2CHandle->RxSize = Len;  // Original length needed for single-byte detection
```

**Why save RxSize?**
- For **single-byte reception**, ACK must be disabled BEFORE clearing ADDR flag
- The ISR needs to know if we're receiving 1 byte or multiple bytes
- `RxLen` decrements as data is read, but `RxSize` stays constant

---

## Complete Execution Flow

### Transmit (TX) Flow

#### 1. Application Calls `I2C_MasterSendDataIT()`

```c
static uint8_t txdata[5] = {0x11, 0x22, 0x33, 0x44, 0x55};

// Setup and return immediately
I2C_MasterSendDataIT(&I2CHandle, txdata, 5, 0x68, DISABLE);
```

**What happens (5μs):**
- Save: `pTxBuffer = txdata`, `TxLen = 5`, `DeviceAddress = 0x68`
- Generate START
- Enable interrupts
- **Return**

---

#### 2. START Condition Generated → SB Interrupt

**Hardware:** START condition generated on bus
**Interrupt:** SB flag set in SR1

```c
void I2C1_EV_IRQHandler(void)  // Called by hardware
{
    I2C_EV_IRQHandling(&I2CHandle);
}

// Inside I2C_EV_IRQHandling()
if (SR1 & I2C_SR1_SB)  // SB interrupt
{
    if (TxRxState == I2C_BUSY_IN_TX)
    {
        // Send slave address with write bit
        I2C_ExecuteAddressPhaseWrite(pI2Cx, DeviceAddress);
        // Writes: (0x68 << 1) | 0 = 0xD0 to DR
    }
}
```

**ISR Duration:** ~5μs
**Action:** Send slave address
**Flag Cleared:** Writing to DR clears SB flag

---

#### 3. Address Sent → ADDR Interrupt

**Hardware:** Address sent, slave ACKed
**Interrupt:** ADDR flag set in SR1

```c
if (SR1 & I2C_SR1_ADDR)  // ADDR interrupt
{
    // Clear ADDR flag by reading SR1 and SR2
    I2C_ClearADDRFlag(pI2CHandle);
}
```

**ISR Duration:** ~5μs
**Action:** Clear ADDR flag
**Flag Cleared:** Read SR1, then SR2

---

#### 4. Buffer Empty → TXE Interrupt (Repeated for Each Byte)

**Hardware:** Transmit buffer empty, ready for data
**Interrupt:** TXE flag set in SR1

```c
if (SR1 & I2C_SR1_TXE)  // TXE interrupt
{
    I2C_MasterHandleTXEInterrupt(pI2CHandle);
}

// Inside I2C_MasterHandleTXEInterrupt()
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pH)
{
    if (pH->TxLen > 0)
    {
        // Send next byte
        pH->pI2Cx->DR = *(pH->pTxBuffer);  // Send 0x11, then 0x22, ...
        pH->TxLen--;                        // 5 → 4 → 3 → 2 → 1 → 0
        pH->pTxBuffer++;                    // Move to next byte
    }
}
```

**ISR Duration:** ~5μs per byte
**Action:** Send one data byte
**Flag Cleared:** Writing to DR clears TXE

**This interrupt fires 5 times** (once for each byte: 0x11, 0x22, 0x33, 0x44, 0x55)

---

#### 5. Transfer Complete → BTF Interrupt

**Hardware:** Byte transfer finished, both shift register and DR empty
**Interrupt:** BTF flag set in SR1

```c
if (SR1 & I2C_SR1_BTF)  // BTF interrupt
{
    if (TxRxState == I2C_BUSY_IN_TX)
    {
        if ((SR1 & I2C_SR1_TXE) && (TxLen == 0))
        {
            // All data sent!
            // 1. Generate STOP condition
            if (Sr == I2C_SR_DISABLE)
            {
                I2C_GenerateStopCondition(pI2Cx);
            }

            // 2. Close transmission (disable interrupts, reset state)
            I2C_CloseSendData(pI2CHandle);

            // 3. Notify application
            I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
        }
    }
}
```

**ISR Duration:** ~5μs
**Actions:**
1. Generate STOP condition
2. Disable interrupts
3. Reset state to `I2C_READY`
4. Call application callback

---

#### 6. Application Callback

```c
void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    if (Event == I2C_EV_TX_CMPLT)
    {
        // Transmission complete!
        // All 5 bytes sent successfully
        // Can now reuse the buffer or start new operation
        printf("I2C TX Complete!\n");
    }
}
```

---

### Complete Timeline for 5-Byte Transmission

```
Time     Event                    What Happens                      CPU State
─────────────────────────────────────────────────────────────────────────────
0.0ms    App calls SendDataIT()   Save params, enable interrupts   Setup (5μs)
         Function returns         ─────────────────────────────→   App runs

0.1ms    [SB Interrupt]           Send slave address               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.15ms   [ADDR Interrupt]         Clear ADDR flag                  ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.2ms    [TXE Interrupt]          Send byte 1 (0x11)               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.3ms    [TXE Interrupt]          Send byte 2 (0x22)               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.4ms    [TXE Interrupt]          Send byte 3 (0x33)               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.5ms    [TXE Interrupt]          Send byte 4 (0x44)               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.6ms    [TXE Interrupt]          Send byte 5 (0x55)               ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

0.65ms   [BTF Interrupt]          STOP, close, callback            ISR (5μs)
         ISR returns              ─────────────────────────────→   App runs

Total Time: ~0.7ms
CPU Time in ISR: ~40μs (8 interrupts × 5μs)
CPU Time for App: ~660μs (94% of total time!)
```

**Key Point:** During the 0.7ms transmission, the CPU spends:
- **40μs** handling I2C (6% of time)
- **660μs** running your application (94% of time)

---

## Visual Illustrations

### CPU Utilization Graph

```
POLLING MODE - CPU Blocked
══════════════════════════════════════════════════════════
CPU Usage
100% │████████████████████████████████████████│
 80% │████████████████████████████████████████│ ← CPU stuck waiting
 60% │████████████████████████████████████████│
 40% │████████████████████████████████████████│
 20% │████████████████████████████████████████│
  0% │                                        │
     0.0ms                                 0.7ms

     All CPU time wasted in while() loops!
     Cannot do ANY other work!


INTERRUPT MODE - CPU Free
══════════════════════════════════════════════════════════
CPU Usage
100% │█ █ █ █ █ █ █ █                        │
 80% │█ █ █ █ █ █ █ █                        │ ← Brief ISR spikes
 60% │█ █ █ █ █ █ █ █                        │
 40% │█ █ █ █ █ █ █ █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│ ← App running
 20% │█ █ █ █ █ █ █ █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│
  0% │                                        │
     0.0ms                                 0.7ms

Legend: █ = ISR (very brief, 5μs)
        ▓ = Application code (useful work!)

94% of CPU time available for your application!
```

---

### The Microwave Analogy

#### Polling Mode = Standing in Front of Microwave

```
You:  "I need to heat food for 2 minutes"

      [Put food in microwave]
      [Press START]

      [Stand there watching for 2 minutes]
      [Stare at rotating plate...]
      [Wait... wait... wait...]
      [Still waiting...]
      [Almost done...]

      [DING! Food ready]

      [Finally can do laundry]

Result: Wasted 2 minutes doing NOTHING
```

#### Interrupt Mode = Microwave with Timer

```
You:  "I need to heat food for 2 minutes"

      [Put food in microwave]
      [Press START]
      [Walk away immediately]

      [Do laundry while food heats]
      [Check phone]
      [Clean dishes]
      [Organize desk]

      [DING! Microwave beeps - interrupt!]

      [Quickly get food - 5 seconds]
      [Return to laundry]

Result: Did 2 minutes of useful work while food heated!
```

**This is EXACTLY how interrupt mode works!**

---

## Common Misconceptions

### ❌ Misconception 1: "Interrupts make I2C faster"

**Reality:** NO! I2C hardware sends data at the same speed regardless of mode.

Speed is determined by:
- SCL clock frequency (100 kHz, 400 kHz, etc.)
- Bus capacitance, pull-up resistors
- Hardware limitations

**Polling:** I2C sends 5 bytes in 0.7ms
**Interrupt:** I2C sends 5 bytes in 0.7ms

**Same speed!** The difference is what the CPU does during that 0.7ms.

---

### ❌ Misconception 2: "Interrupts notify CPU to do other tasks"

**Reality:** Interrupts are FOR the I2C events themselves!

The interrupts are not saying "go do other work." They're saying:
- "START condition complete, send address now!"
- "Address sent, clear ADDR flag!"
- "Buffer empty, send next byte!"

The CPU is naturally free because `I2C_MasterSendDataIT()` returns immediately.

---

### ❌ Misconception 3: "CPU returns to transmit remaining data"

**Reality:** CPU doesn't "return" to I2C work. The ISR is CALLED by hardware.

```
Wrong Mental Model:
  App → I2C function → Return to app → Return to I2C → Return to app...

Correct Mental Model:
  App runs continuously
     ↓ (Interrupted briefly by hardware)
  [ISR executes for 5μs]
     ↓ (Returns immediately)
  App continues running
     ↓ (Interrupted again)
  [ISR executes for 5μs]
     ↓ (Returns)
  App continues running...
```

The CPU doesn't decide to "go back" to I2C. The hardware FORCES the CPU to execute the ISR when an event occurs.

---

### ❌ Misconception 4: "`I2C_MasterSendDataIT()` sends data"

**Reality:** It only PREPARES for sending. The ISR does the actual work.

```c
// This function:
I2C_MasterSendDataIT(&I2CHandle, data, 5, 0x68, DISABLE);

// Does this:
✅ Save parameters
✅ Enable interrupts
✅ Generate START
✅ Return

// Does NOT do this:
❌ Send slave address
❌ Send data bytes
❌ Generate STOP
❌ Wait for completion
```

---

## Practical Examples

### Example 1: Weather Station

```c
// Polling Mode - Everything blocks
void weather_station_polling(void)
{
    while(1)
    {
        // Read temperature from I2C sensor (5ms)
        I2C_MasterReceiveData(&I2C_Handle, temp_buf, 2, TEMP_ADDR, DISABLE);
        // ↑ BLOCKS for 5ms - nothing else can run!

        // Read humidity (5ms)
        I2C_MasterReceiveData(&I2C_Handle, hum_buf, 2, HUM_ADDR, DISABLE);
        // ↑ BLOCKS for 5ms

        // Update LCD display
        UpdateDisplay();

        // Check button presses - missed if pressed during I2C!
        CheckButtons();

        delay_ms(1000);
    }
}
```

**Problem:** If user presses button during I2C read, it's missed!

---

```c
// Interrupt Mode - System remains responsive
void weather_station_interrupt(void)
{
    static uint8_t state = 0;

    while(1)
    {
        // Can ALWAYS check buttons, even during I2C
        CheckButtons();      // Responds immediately

        // Update display
        UpdateDisplay();     // Always updates

        // Blink LED
        BlinkStatusLED();    // Precise timing

        // Log data
        LogToSD();

        // I2C happens in background via interrupts
    }
}

// Callback when I2C completes
void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    static uint8_t state = 0;

    if (Event == I2C_EV_RX_CMPLT)
    {
        if (state == 0)
        {
            // Temperature read complete, start humidity read
            I2C_MasterReceiveDataIT(&I2C_Handle, hum_buf, 2, HUM_ADDR, DISABLE);
            state = 1;
        }
        else
        {
            // Both reads complete, process data
            ProcessWeatherData();
            state = 0;
        }
    }
}
```

**Benefit:** System always responsive, never "freezes"

---

### Example 2: Multi-Sensor System

```c
void multi_sensor_system(void)
{
    // Configure I2C interrupts
    I2C_IRQInterruptConfig(IRQ_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_I2C1_ER, ENABLE);

    // Start first sensor read (returns immediately)
    static uint8_t accel_data[6];
    I2C_MasterReceiveDataIT(&I2C_Handle, accel_data, 6, ACCEL_ADDR, DISABLE);

    while(1)
    {
        // Main loop continues running
        ProcessGyroData();
        UpdateMotorControl();
        CheckSafetyLimits();
        UpdateTelemetry();

        // All these functions run smoothly while I2C reads happen in background
    }
}

void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    if (Event == I2C_EV_RX_CMPLT)
    {
        // Process received data
        ProcessAccelData();

        // Start next sensor read
        static uint8_t gyro_data[6];
        I2C_MasterReceiveDataIT(&I2C_Handle, gyro_data, 6, GYRO_ADDR, DISABLE);
    }
}
```

---

### Example 3: Error Handling

```c
volatile uint8_t i2c_error = 0;

void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    switch (Event)
    {
        case I2C_EV_TX_CMPLT:
            // Transmission successful
            printf("TX Complete\n");
            break;

        case I2C_ERROR_AF:
            // Slave didn't acknowledge - not responding
            printf("Error: Slave not responding\n");
            i2c_error = 1;

            // Try recovery
            I2C_DeInit(pH->pI2Cx);
            I2C_Init(pH);
            break;

        case I2C_ERROR_BERR:
            // Bus error - misplaced START/STOP
            printf("Error: Bus error detected\n");
            i2c_error = 1;
            break;

        case I2C_ERROR_TIMEOUT:
            // Timeout - bus stuck
            printf("Error: Timeout\n");
            i2c_error = 1;

            // Attempt bus recovery
            I2C_BusReset(pH->pI2Cx);
            break;
    }
}
```

---

## Best Practices

### 1. Buffer Lifetime Management

**CRITICAL:** Buffers must remain valid until callback!

```c
// ❌ WRONG - Buffer destroyed when function exits
void send_data(void)
{
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    I2C_MasterSendDataIT(&I2CHandle, data, 10, 0x68, DISABLE);
}  // data[] destroyed here, but ISR will try to access it!

// ✅ CORRECT - Use static or global buffer
static uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

void send_data(void)
{
    I2C_MasterSendDataIT(&I2CHandle, data, 10, 0x68, DISABLE);
}  // data[] still valid
```

---

### 2. Check Return Status

```c
uint8_t status;

status = I2C_MasterSendDataIT(&I2CHandle, data, 10, 0x68, DISABLE);

if (status == I2C_READY)
{
    // Transaction initiated successfully
}
else
{
    // I2C busy, try again later
    printf("I2C busy, state = %d\n", status);
}
```

---

### 3. Configure Interrupts

```c
void i2c_interrupt_init(void)
{
    // Enable both Event and Error interrupts
    I2C_IRQInterruptConfig(IRQ_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_I2C1_ER, ENABLE);

    // Set same priority for both
    I2C_IRQPriorityConfig(IRQ_I2C1_EV, NVIC_IRQ_PRI2);
    I2C_IRQPriorityConfig(IRQ_I2C1_ER, NVIC_IRQ_PRI2);
}

// Implement both handlers
void I2C1_EV_IRQHandler(void)
{
    I2C_EV_IRQHandling(&I2C1Handle);
}

void I2C1_ER_IRQHandler(void)
{
    I2C_ER_IRQHandling(&I2C1Handle);
}
```

---

### 4. Always Implement Callback

```c
// MUST implement this function in your application
void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    // Handle ALL events
    switch (Event)
    {
        case I2C_EV_TX_CMPLT:
            // TX complete
            break;

        case I2C_EV_RX_CMPLT:
            // RX complete
            break;

        case I2C_ERROR_AF:
        case I2C_ERROR_BERR:
        case I2C_ERROR_ARLO:
        case I2C_ERROR_OVR:
        case I2C_ERROR_TIMEOUT:
            // Handle errors
            break;
    }
}
```

---

### 5. Use State Machine for Complex Sequences

```c
typedef enum {
    STATE_IDLE,
    STATE_READ_SENSOR1,
    STATE_READ_SENSOR2,
    STATE_WRITE_DISPLAY,
    STATE_ERROR
} I2C_State_t;

static I2C_State_t currentState = STATE_IDLE;

void I2C_ApplicationEventCallback(I2C_Handle_t *pH, uint8_t Event)
{
    if (Event == I2C_EV_TX_CMPLT || Event == I2C_EV_RX_CMPLT)
    {
        switch (currentState)
        {
            case STATE_READ_SENSOR1:
                // Sensor 1 read complete, read sensor 2
                I2C_MasterReceiveDataIT(pH, sensor2_buf, 4, SENSOR2_ADDR, DISABLE);
                currentState = STATE_READ_SENSOR2;
                break;

            case STATE_READ_SENSOR2:
                // Sensor 2 read complete, update display
                I2C_MasterSendDataIT(pH, display_buf, 16, DISPLAY_ADDR, DISABLE);
                currentState = STATE_WRITE_DISPLAY;
                break;

            case STATE_WRITE_DISPLAY:
                // Display updated, done
                currentState = STATE_IDLE;
                break;
        }
    }
    else  // Error events
    {
        currentState = STATE_ERROR;
        // Handle error
    }
}
```

---

## Summary

### Key Takeaways

1. **Purpose of Interrupt Mode:**
   - NOT to make I2C faster
   - To FREE the CPU for other work
   - To enable multitasking on single-core MCU

2. **What `I2C_MasterSendDataIT()` Does:**
   - ✅ Save transaction parameters
   - ✅ Enable interrupts
   - ✅ Generate START
   - ✅ Return immediately
   - ❌ Does NOT send data itself

3. **Actual Data Transfer:**
   - Happens in ISR (Interrupt Service Routine)
   - ISR called automatically by hardware
   - Brief execution (~5μs per interrupt)
   - CPU returns to application between interrupts

4. **CPU Utilization:**
   - Polling: 100% blocked during I2C
   - Interrupt: ~5-10% for I2C, 90-95% for application

5. **When to Use Each:**
   - **Polling:** Simple apps, quick transfers, debugging
   - **Interrupt:** Complex apps, long transfers, multitasking

---

### The Bottom Line

> **Interrupt mode doesn't make I2C communication faster.**
>
> **It makes YOUR APPLICATION faster by letting the CPU do useful work instead of waiting!**

The I2C hardware transmits at the same speed either way. The magic of interrupt mode is that **your CPU is free to do other things while I2C hardware handles communication autonomously**.

---

## Related Documents

- [I2C Driver Implementation Report](I2C_Driver_Implementation_Report.md) - Complete driver documentation
- [stm32f411xx_i2c_driver.c](../drivers/Src/stm32f411xx_i2c_driver.c) - Driver source code
- [stm32f411xx_i2c_driver.h](../drivers/Inc/stm32f411xx_i2c_driver.h) - Driver header

---

**Document Version:** 1.0
**Date:** December 27, 2025
**Based on:** Discussion about I2C interrupt mode operation
