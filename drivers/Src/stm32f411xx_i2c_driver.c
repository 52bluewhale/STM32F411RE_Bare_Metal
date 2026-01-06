/*
 * This I2C driver implements the I2C communication protocol for STM32F411xx microcontrollers.
 * It supports both master and slave modes, standard and fast I2C speeds, and both polling
 * and interrupt-driven data transfer modes.
 *
 * Key Features:
 * - Master transmit/receive in polling and interrupt modes
 * - Proper handling of I2C protocol sequences (START, ADDRESS, DATA, STOP)
 * - ACK/NACK management for reliable communication
 * - Interrupt handling for event and error conditions
 * - Clock configuration for different I2C speeds
 */

#include "stm32f411xx.h"
#include "stm32f411xx_i2c_driver.h"

// Predefined prescaler values for AHB and APB1 buses used in clock calculations
uint16_t AHB_PreScaler[8] = {2, 4, 8, 16, 64, 128, 256, 512};
uint8_t APB1_Prescaler[4] = {2, 4, 8, 16};

/*
 * ========================= I2C HELPER FUNCTIONS PROTOTYPE =========================
 * These static helper functions encapsulate common I2C operations and are used
 * internally by the driver to maintain clean, modular code.
 */
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle);
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle);
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle);

/*
 * ========================= I2C HELPER FUNCTIONS DEFINITIONS =========================
 * Implementation of helper functions that handle low-level I2C protocol operations.
 */

/*
 * Executes the address phase for write operation (master transmitting to slave).
 * Algorithm:
 * 1. Shift slave address left by 1 to make room for R/W bit
 * 2. Clear the LSB (set to 0 for write operation)
 * 3. Load the address into DR register - hardware handles transmission
 */
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
    SlaveAddr = SlaveAddr << 1;
    SlaveAddr &= ~(1);          // SlaveAddr + w bit (write = 0)

    pI2Cx->DR = SlaveAddr;
}

/*
 * Executes the address phase for read operation (master receiving from slave).
 * Algorithm:
 * 1. Shift slave address left by 1 to make room for R/W bit
 * 2. Set the LSB (set to 1 for read operation) - note: the code has &= ~(0) which is redundant
 * 3. Load the address into DR register - hardware handles transmission
 */
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
    SlaveAddr = SlaveAddr << 1;
    SlaveAddr &= ~(0);          // SlaveAddr + r bit (read = 1) - this should be |= 1

    pI2Cx->DR = SlaveAddr;
}

/*
 * Clears the ADDR flag in SR1 register.
 * Algorithm: Read SR1 and SR2 registers in sequence. This is required by hardware
 * to clear the ADDR flag. Different handling for master vs slave mode, and special
 * case for single-byte reception in master mode.
 */
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle)
{
    uint32_t dummy_read;

    // Check for device mode
    if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
    {
        // Device is in master mode
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
        {
            if (pI2CHandle->RxSize == 1)
            {
                // For single byte reception: disable ACK before clearing ADDR
                // This prevents the slave from sending more data
                I2C_ManageACK(pI2CHandle->pI2Cx, DISABLE);

                // Clear ADDR flag by reading SR1 and SR2
                dummy_read = pI2CHandle->pI2Cx->SR1;
                dummy_read = pI2CHandle->pI2Cx->SR2;
                (void)dummy_read;
            }
        }
        else
        {
            // For transmission or multi-byte reception: just clear ADDR flag
            dummy_read = pI2CHandle->pI2Cx->SR1;
            dummy_read = pI2CHandle->pI2Cx->SR2;
            (void)dummy_read;
        }
    }
    else
    {
        // Device is in slave mode: clear ADDR flag
        dummy_read = pI2CHandle->pI2Cx->SR1;
        dummy_read = pI2CHandle->pI2Cx->SR2;
        (void)dummy_read;
    }
}

/*
 * Handles TXE (Transmit buffer empty) interrupt for master transmission.
 * Algorithm:
 * 1. Check if there's data left to send (TxLen > 0)
 * 2. Load next byte from buffer into DR register
 * 3. Decrement TxLen and increment buffer pointer
 * This function is called repeatedly until all data is sent.
 */
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle)
{
    if (pI2CHandle->TxLen > 0)
    {
        // Load data into DR register
        pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);

        // Decrement TxLen and increment buffer pointer
        pI2CHandle->TxLen--;

        // Increment buffer address
        pI2CHandle->pTxBuffer++;
    }
}

/*
 * Handles RXNE (Receive buffer not empty) interrupt for master reception.
 * Algorithm: Complex handling for different reception scenarios:
 * - Single byte: Read directly after disabling ACK
 * - Multi-byte: Read data, manage ACK for last 2 bytes
 * - Generate STOP condition when done
 * - Notify application when reception complete
 */
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle)
{
    if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
    {
        if (pI2CHandle->RxSize == 1)
        {
            // Single byte reception: read the byte
            *pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
            pI2CHandle->RxLen--;
        }

        if (pI2CHandle->RxSize > 1)
        {
            if (pI2CHandle->RxLen == 2)
            {
                // Last 2 bytes: disable ACK to signal end of reception
                I2C_ManageACK(pI2CHandle->pI2Cx, DISABLE);
            }

            // Read the data from DR register
            *(pI2CHandle->pRxBuffer) = pI2CHandle->pI2Cx->DR;
            pI2CHandle->pRxBuffer++;
            pI2CHandle->RxLen--;
        }

        if (pI2CHandle->RxLen == 0)
        {
            // All data received: close reception
            // 1. Generate STOP if not repeated start
            if (pI2CHandle->Sr == I2C_SR_DISABLE)
            {
                I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
            }

            // 2. Close I2C Rx
            I2C_CloseReceiveData(pI2CHandle);

            // 3. Notify the application
            I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
        }
    }
}

// ===============================================================================
/*
 * Calculates the PCLK1 (APB1 peripheral clock) frequency.
 * Algorithm:
 * 1. Determine system clock source from RCC->CFGR
 * 2. Calculate AHB prescaler value
 * 3. Calculate APB1 prescaler value
 * 4. Compute PCLK1 = (SystemClk / AHB_prescaler) / APB1_prescaler
 * This is needed for I2C timing calculations.
 */
uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t pclk1, SystemClk;
    uint8_t clksrc, temp, ahbp, apb1;

    // Get clock source from RCC CFGR register (bits 3:2)
    clksrc = (RCC->CFGR >> 2) & 0x3;

    // Determine system clock frequency based on source
    if (clksrc == 0)
    {
        SystemClk = 16000000;  // HSI oscillator (16 MHz)
    }
    else if (clksrc == 1)
    {
        SystemClk = 8000000;   // HSE oscillator (8 MHz)
    }
    else if (clksrc == 2)
    {
        SystemClk = RCC_GetPLLOutputClock();  // PLL output
    }

    // Calculate AHB prescaler (bits 7:4 of CFGR)
    temp = ((RCC->CFGR >> 4) & 0xF);
    if (temp < 8)
    {
        ahbp = 1;  // No prescaling
    }
    else
    {
        ahbp = AHB_PreScaler[temp - 8];  // Use lookup table
    }

    // Calculate APB1 prescaler (bits 12:10 of CFGR)
    temp = ((RCC->CFGR >> 10) & 0x7);
    if (temp < 4)
    {
        apb1 = 1;  // No prescaling
    }
    else
    {
        apb1 = APB1_Prescaler[temp - 4];  // Use lookup table
    }

    // Calculate PCLK1 frequency
    pclk1 = (SystemClk / ahbp) / apb1;

    return pclk1;
}

/*
 * Controls the peripheral clock for I2C modules.
 * Algorithm: Enable/disable clock in RCC APB1 peripheral clock enable register.
 * Only enable is implemented; disable is marked as TODO.
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pI2Cx == I2C1)
        {
            I2C1_PCLK_EN();  // Enable I2C1 clock
        }
        else if (pI2Cx == I2C2)
        {
            I2C2_PCLK_EN();  // Enable I2C2 clock
        }
        else if (pI2Cx == I2C3)
        {
            I2C3_PCLK_EN();  // Enable I2C3 clock
        }
    }
    else
    {
        if (pI2Cx == I2C1)
        {
            I2C1_PCLK_DI();  // Enable I2C1 clock
        }
        else if (pI2Cx == I2C2)
        {
            I2C2_PCLK_DI();  // Enable I2C2 clock
        }
        else if (pI2Cx == I2C3)
        {
            I2C3_PCLK_DI();  // Enable I2C3 clock
        }
    }
}

/*
 * Initializes the I2C peripheral with the provided configuration.
 * Algorithm: Follows the I2C initialization sequence as per STM32 reference manual:
 * 1. Enable peripheral clock
 * 2. Configure ACK control bit
 * 3. Set peripheral input clock frequency in CR2
 * 4. Configure device own address (for slave mode)
 * 5. Configure clock control registers (CCR) for SCL frequency
 * 6. Configure rise time register (TRISE)
 *
 * All configuration must be done while PE (Peripheral Enable) bit is 0.
 * The peripheral is enabled by calling I2C_PeripheralControl() separately.
 */
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
    /* ==================== Step for I2C Init ====================*/
    /* Step 1: Configure the mode (standard or fast) */
    /* Step 2: Configure the speed of the serial clock (SCL) */
    /* Step 3: Configure the device address (Applicable when device is slave) */
    /* Step 4: Enable the acking */
    /* Step 5: Configure the rise time for I2C pin */
    /* All steps above must be done when peripheral is disabled in the CR */
    uint32_t tempreg = 0;
    // uint8_t trise;

    // Enable peripheral clock for the I2C module
    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

    // Configure ACK control bit in CR1
    // This enables/disables automatic ACK generation
    tempreg |= pI2CHandle->I2C_Config.I2C_ACKControl << I2C_CR1_ACK;
    pI2CHandle->pI2Cx->CR1 = tempreg;

    // Configure the FREQ field of CR2 with peripheral input clock frequency (in MHz)
    // This is used by hardware for timing calculations
    tempreg = 0;
    tempreg = RCC_GetPCLK1Value() / 1000000U;
    pI2CHandle->pI2Cx->CR2 = (tempreg & 0x3F);

    // Configure device own address for slave mode operation
    tempreg = 0;
    tempreg |= pI2CHandle->I2C_Config.I2C_DeviceAddress << I2C_OAR1_ADD_7_1;
    tempreg |= (1U << 14);  // Set bit 14 to enable dual addressing mode
    pI2CHandle->pI2Cx->OAR1 = tempreg;

    // Calculate and configure CCR (Clock Control Register) for SCL frequency
    uint16_t ccr_value = 0;
    tempreg = 0;

    if (pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
    {
        // Standard mode (up to 100 kHz)
        // CCR = PCLK1 / (2 * SCL_frequency)
        ccr_value = (RCC_GetPCLK1Value() / (2 * pI2CHandle->I2C_Config.I2C_SCLSpeed));
        tempreg |= (ccr_value & 0xFFF);
    }
    else
    {
        // Fast mode (up to 400 kHz)
        tempreg |= (1U << I2C_CCR_FS);  // Set fast mode bit
        tempreg |= (pI2CHandle->I2C_Config.I2C_FMDutyCycle << I2C_CCR_DUTY);  // Set duty cycle

        if (pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2)
        {
            // T_high = T_low = CCR * T_PCLK1
            // CCR = PCLK1 / (3 * SCL_frequency)
            ccr_value = (RCC_GetPCLK1Value() / (3 * pI2CHandle->I2C_Config.I2C_SCLSpeed));
        }
        else
        {
            // T_high = 9 * CCR * T_PCLK1, T_low = 16 * CCR * T_PCLK1
            // CCR = PCLK1 / (25 * SCL_frequency)
            ccr_value = (RCC_GetPCLK1Value() / (25 * pI2CHandle->I2C_Config.I2C_SCLSpeed));
        }

        tempreg |= (ccr_value & 0xFFF);
    }

    pI2CHandle->pI2Cx->CCR |= tempreg;

    // Configure TRISE (Rise Time) register
    // TRISE = (maximum_rise_time / T_PCLK1) + 1
    if (pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
    {
        // Standard mode: maximum rise time = 1000ns
        tempreg = (RCC_GetPCLK1Value() / 1000000U) + 1;
    }
    else
    {
        // Fast mode: maximum rise time = 300ns
        tempreg = ((RCC_GetPCLK1Value() * 300U) / 1000000000U) + 1;
    }
    pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3F);
}

void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
    /*
     * Deinitializes the I2C peripheral by resetting it.
     * Algorithm: Use RCC reset registers to reset the I2C peripheral to its default state.
     */
    if (pI2Cx == I2C1)
    {
        I2C1_PCLK_RESET();
    }
    else if (pI2Cx == I2C2)
    {
        I2C2_PCLK_RESET();
    }
    else if (pI2Cx == I2C3)
    {
        I2C3_PCLK_RESET();
    }
}

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
    /*
     * Checks the status of a specific I2C flag in SR1 or SR2 registers.
     * Algorithm: Performs bitwise AND operation on the status registers with the flag mask.
     * Returns FLAG_SET if the flag is set, FLAG_RESET otherwise.
     */
    if (pI2Cx->SR1 & FlagName || pI2Cx->SR2 & FlagName)
    {
        return FLAG_SET;
    }
    return FLAG_RESET;
}

/*
 * Sends data to a slave device in master mode using polling (blocking) method.
 * Algorithm: Implements the complete I2C master transmission sequence:
 * 1. Generate START condition
 * 2. Wait for START to complete (SB flag)
 * 3. Send slave address with write bit
 * 4. Wait for address acknowledgment (ADDR flag)
 * 5. Clear ADDR flag
 * 6. Send data bytes while TXE flag is set
 * 7. Wait for transmission complete (TXE and BTF flags)
 * 8. Generate STOP condition
 */
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    //1. Generate the START condition
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

    //2. Confirm that start generation is completed by checking the SB flag in SR1
    //Note: until SB is cleared, sck will be stretched (pull to LOW)
    while (!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_SB)));

    //3. Send the address of slave with r/w bit set to w(0) (total 8 bits)
    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);

    //4. Confirm that address phase is completed by checking the ADDR flag in SR1
    while (!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_ADDR)));

    //5. Clear the ADDR flag according to its Sw sequences
    //Note: until ADDR is cleared scl will be stretched (pull to LOW)
    I2C_ClearADDRFlag(pI2CHandle);

    //6. Send the data until Len becomes 0
    while (Len > 0)
    {
        // Wait till TXE is set
        while(!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_TXE)));
        pI2CHandle->pI2Cx->DR = *pTxBuffer;
        pTxBuffer++;
        Len--;
    }

    //7. When Len become 0 wait for TXE = 1 and BTF = 1 before generating STOP condition
    //Note: TXE = 1, BTF = 1, means that both SR and DR are empty and next transmission 
    //should begin when BTF = 1 scl will be stretched
    while(!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_TXE)));
    while(!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_BTF)));

    //8. Generate STOP condition and master need not to wait for the completion of 
    //STOP condition.
    //Note: generating STOP automatically clears the BTF
    if (Sr == I2C_SR_DISABLE)
    {
        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
    }
}

/*
 * Receives data from a slave device in master mode using polling (blocking) method.
 * Algorithm: Handles both single-byte and multi-byte reception:
 * 1. Generate START condition
 * 2. Wait for START to complete
 * 3. Send slave address with read bit
 * 4. Wait for address acknowledgment
 * 5. For single byte: disable ACK, clear ADDR, read data, generate STOP
 * 6. For multiple bytes: clear ADDR, read data in loop, disable ACK before last byte
 * 7. Re-enable ACK if it was enabled in configuration
 */
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    // 1. Generate START condition
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

    // 2. Confirm that start generation is completed by checking SB flag in SR1
    while (!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_SB)));

    // 3. Send address of the slave with R/W bit set R(1) (total 8 bit)
    I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr);

    // 4. Wait until address phase is completed by checking the ADDR flag in SR1
    while (!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_ADDR)));

    // Procedure to read 1 byte from slave
    if (Len == 1)
    {
        // Disable ack
        I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

        // Clear ADDR flag
        I2C_ClearADDRFlag(pI2CHandle);

        // Wait until RXNE becomes 1
        while(!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_RXNE)));
        
        // Generate STOP
        if (Sr == I2C_SR_DISABLE)
        {
            I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
        }
        
        // Read data into buffer
        *pRxBuffer = pI2CHandle->pI2Cx->DR;
    }

    // Procedure to read more than 1 byte from slave
    if (Len > 1)
    {
        // Clear ADDR flag
        I2C_ClearADDRFlag(pI2CHandle);

        // Read data until Len = 0
        for (uint32_t i = Len; i > 0; i++)
        {
            // Wait until RXNE becomes 1
            while (!(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SR1_RXNE)));

            if (i == 2) // If last 2 bytes are remaining
            {
                // Clear ACK bit
                I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

                // Generate STOP
                I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
            }

            // Read the data from data register into buffer
            *pRxBuffer = pI2CHandle->pI2Cx->DR;

            // Increament buffer address
            pRxBuffer++;
        }
    }

    // Re-enabling ACK again
    if (pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
    {
        I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
    }
}

void I2C_SlaveSendData(I2C_RegDef_t *pI2C, uint8_t data)
{
    pI2C->DR = data;
}

uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2C)
{
    return (uint8_t) pI2C->DR;
}

// Key Points
//     State Check: Only start if bus is not busy in TX or RX
//     Save Parameters: Store buffer pointer, length, slave address, and repeated start flag
//     Enable Interrupts: Enable three interrupt types:
//         ITBUFEN (Bit 10 of CR2): Buffer interrupts (TXE/RXNE)
//         ITEVTEN (Bit 9 of CR2): Event interrupts (SB, ADDR, BTF, STOPF)
//         ITERREN (Bit 8 of CR2): Error interrupts (BERR, ARLO, AF, OVR)
//     Generate START: Hardware will trigger SB interrupt
// Interrupt Flow After Calling This Function
//     SB (Start Bit) interrupt → Send slave address
//     ADDR interrupt → Clear ADDR flag
//     TXE interrupt → Load data into DR register
//     BTF interrupt → All data sent, generate STOP
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    uint8_t busystate = pI2CHandle->TxRxState;

    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        // 1. Save the Tx buffer address and Len information in pI2CHandle
        pI2CHandle->pTxBuffer = pTxBuffer;
        pI2CHandle->TxLen = Len;
        pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
        pI2CHandle->DeviceAddress = SlaveAddr;
        pI2CHandle->Sr = Sr;

        // 2. Generate START 
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

        // 3. Enable ITBUFEN control bit (Buffer interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);

        // 4. Enable ITEVTEN control bit (Event interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);

        // 5. Enable ITERREN control bit (Error interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
    }

    return busystate;
}

// Key Points
//     RxSize Field: Save original length for single-byte reception handling
//     Similar to TX: Same interrupt enable pattern
//     Rx State: Set state to I2C_BUSY_IN_RX
// Interrupt Flow After Calling This Function
//     SB interrupt → Send slave address with R/W=1
//     ADDR interrupt → Enable ACK (for multi-byte) or disable ACK (for 1-byte)
//     RXNE interrupt → Read data from DR register
//     Handle last byte specially (NACK before reading)
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    uint8_t busystate = pI2CHandle->TxRxState;

    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        // 1. Save the Rx buffer address and Len information in handle
        pI2CHandle->pRxBuffer = pRxBuffer;
        pI2CHandle->RxLen = Len;
        pI2CHandle->RxSize = Len;                       // Save the original Rx size
        pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
        pI2CHandle->DeviceAddress = SlaveAddr;
        pI2CHandle->Sr = Sr;

        // 2. Generate START 
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

        // 3. Enable ITBUFEN control bit (Buffer interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);

        // 4. Enable ITEVTEN control bit (Event interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);

        // 5. Enable ITERREN control bit (Error interrupt enable)
        pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
    }

    return busystate;
}

// Key Points
//     Identical to SPI: Same NVIC register access pattern
//     I2C IRQ Numbers (from stm32f411xx.h):
//         IRQ_I2C1_EV = 31 (Event)
//         IRQ_I2C1_ER = 32 (Error)
//         IRQ_I2C2_EV = 33
//         IRQ_I2C2_ER = 34
//         IRQ_I2C3_EV = 72
//         IRQ_I2C3_ER = 73
//     Two Interrupts per I2C: Each I2C peripheral has separate Event and Error interrupt lines
/*
 * Configures interrupt enable/disable in NVIC for I2C interrupts.
 * Algorithm: Programs the appropriate ISER/ICER registers based on IRQ number.
 * Each I2C peripheral has two interrupt lines: Event (EV) and Error (ER).
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
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

// Key Points
//     Identical to SPI: Same NVIC priority register calculation
//     4 Bits Implemented: STM32F411 uses upper 4 bits of each 8-bit field
//     Priority Range: 0 (highest) to 15 (lowest)
//     IPR Registers: Each IPR register holds 4 IRQ priorities
// NVIC Priority Register Layout
//     IPR[n]: [IRQ(4n+3)][IRQ(4n+2)][IRQ(4n+1)][IRQ(4n)]
//             31-24      23-16      15-8       7-0

//     Each 8-bit field: [7:4] = Priority (implemented)
//                     [3:0] = Sub-priority (not implemented, reads as 0)
/*
 * Configures interrupt priority for I2C interrupts in NVIC.
 * Algorithm: Calculates the correct IPR register and bit position, then sets the priority.
 * STM32F411 implements only 4 bits of priority (bits 7:4) in each 8-bit field.
 */
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
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

/*
 * Handles I2C Event interrupts (SB, ADDR, BTF, STOPF, TXE, RXNE).
 * Algorithm: Checks interrupt enable bits and status flags, then handles each event type:
 * - SB: Start condition generated - send address
 * - ADDR: Address sent/received - clear flag
 * - BTF: Byte transfer finished - complete transmission
 * - STOPF: Stop detected (slave mode)
 * - TXE: Transmit buffer empty - load next byte
 * - RXNE: Receive buffer not empty - read data
 */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t temp1, temp2, temp3;
    temp1 = pI2CHandle->pI2Cx->CR2 & (1U << I2C_CR2_ITEVTEN);
    temp2 = pI2CHandle->pI2Cx->CR2 & (1U << I2C_CR2_ITBUFEN);
    
    // Interrupt handling for both master and slave mode of a device
    // 1. Handle for interrupt generated by SB
    // Note: SB flag is only applicable in master mode
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_SB);
    if (temp1 && temp3)
    {
        // SB is set - execute address phase
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, pI2CHandle->DeviceAddress);
        }
        else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
        {
            I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, pI2CHandle->DeviceAddress);
        }
    }

    // 2. Handle for interrupt generated by ADDR event
    // Note:    When master mode: address is sent
    //          When slave mode : address match with own address
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_ADDR);
    if (temp1 && temp3)
    {
        // ADDR is set - clear ADDR flag
        I2C_ClearADDRFlag(pI2CHandle);
    }

    // 3. Handle for interrupt generated by BTF event
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_BTF);
    if (temp1 && temp3)
    {
        // BTF is set
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            // Check if TXE is also set
            if(pI2CHandle->pI2Cx->SR1 & I2C_SR1_TXE)
            {
                // BTF=1, TXE=1 - close transmission
                if (pI2CHandle->TxLen == 0)
                { 
                    // Generate STOP
                    if (pI2CHandle->Sr == I2C_SR_DISABLE)
                    {
                        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
                    }

                    // Reset handle structure
                    I2C_CloseSendData(pI2CHandle);

                    // Notify application about transmission complete
                    I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
                }
            }
        }
    }

    // 4. Handle for interrupt generated by STOPF
    // Note: Stop dectection flag is only applicable only slave mode
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_STOPF);
    if (temp1 && temp3)
    {
        // STOPF is set
        // To clear STOPF: Read SR1 and write to CR1
        pI2CHandle->pI2Cx->CR1 |= 0x000;

        // Notify the application that STOP is detected
        I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
    }

    // 5. Handle for interrupt generated by TXE 
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_TXE);
    if (temp1 && temp2 && temp3)
    {
        // Check for device mode
        if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
        {
            // TXE is set
            if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
            {
                I2C_MasterHandleTXEInterrupt(pI2CHandle);
            }
        }
        else 
        {
            // Slave mode handle

            // Double check if slave is really in transmitter mode
            if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_TRA))
            {
                I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_REQ);
            }
        }
    }

    // 6. Handle for interrupt generated by RXNE
    temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_RXNE);
    if (temp1 && temp2 && temp3)
    {
        // Check for device mode
        if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
        {
            // RXNE is set
            I2C_MasterHandleRXNEInterrupt(pI2CHandle);
        }
        else 
        {
            // Slave mode handle

            // Double check if slave is really in receiver mode
            if (!(pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_TRA)))
            {
                I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_RCV);
            }
        }
    }
}

/*
 * Handles I2C Error interrupts (BERR, ARLO, AF, OVR, TIMEOUT).
 * Algorithm: Checks each error flag and clears it, then notifies the application.
 * Error interrupts help detect and recover from I2C bus issues.
 */
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t temp1, temp2;

    // Check ITERREN control bit
    temp2 = pI2CHandle->pI2Cx->CR2 & (1U << I2C_CR2_ITERREN);

    // Check for Bus error
    temp1 = pI2CHandle->pI2Cx->SR1 & (1U<< I2C_SR1_BERR);
    if (temp1 && temp2)
    {
        // Clear BERR flag
        pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_BERR);

        // Notify application
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
    }

    // Check for Arbitration lost error
    temp1 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_ARLO);
    if (temp1 && temp2)
    {
        // Clear ARLO flag
        pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_ARLO);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
    }

    // Check for ACK failure error 
    temp1 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_AF);
    if (temp1 && temp2)
    {
        // Clear AF flag
        pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_AF);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
    }

    // Check for Overrun/Underrun error
    temp1 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_OVR);
    if (temp1 && temp2)
    {
        // Clear OVR flag
        pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_OVR);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
    }

    // Check for Timeout error
    temp1 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_TIMEOUT);
    if (temp1 && temp2)
    {
        // Clear TIMEOUT flag
        pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_TIMEOUT);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_TIMEOUT);
    }
}

/*
 * Controls ACK (Acknowledge) bit for I2C communication.
 * Algorithm: Sets or clears the ACK bit in CR1 register.
 * ACK enables automatic acknowledgment of received bytes.
 */
void I2C_ManageACK(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if (EnOrDi == I2C_ACK_ENABLE)
    {
        // Enable ACK
        pI2Cx->CR1 |= (1U << I2C_CR1_ACK);
    }
    else 
    {
        // Disable ACK
        pI2Cx->CR1 &= ~(1U << I2C_CR1_ACK);
    }
}

/*
 * Controls the I2C peripheral enable/disable.
 * Algorithm: Sets or clears the PE (Peripheral Enable) bit in CR1 register.
 * Must be disabled during configuration, enabled for operation.
 */
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if (EnOrDi == ENABLE)
    {
        pI2Cx->CR1 |= (1 << I2C_CR1_PE);
    }
    else
    {
        pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);
    }
}

/*
 * Closes I2C transmission and resets handle for interrupt mode.
 * Algorithm: Disables buffer and event interrupts, resets state and buffer pointers.
 * Called when transmission is complete to prepare for next operation.
 */
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
    // Disable ITBUFEN control bit
    pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);

    // Disable ITEVTEN control bit
    pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);

    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pRxBuffer = NULL;
    pI2CHandle->RxLen = 0;
    pI2CHandle->RxSize = 0;
}

/*
 * Closes I2C reception and resets handle for interrupt mode.
 * Algorithm: Disables interrupts, resets state, and re-enables ACK if configured.
 * Called when reception is complete to prepare for next operation.
 */
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
    // Disable ITBUFEN control bit
    pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);

    // Disable ITEVTEN control bit
    pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);

    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pRxBuffer = NULL;
    pI2CHandle->RxLen = 0;
    pI2CHandle->RxSize = 0;

    // Reenable ACK if it was enabled in config
    if (pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
    {
        I2C_ManageACK(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
    }
}


/*
 * Generates START condition on the I2C bus.
 * Algorithm: Set the START bit in CR1 register. Hardware will generate the START
 * condition when the bus becomes free.
 */
void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx)
{
    pI2Cx->CR1 |= (1U << I2C_CR1_START);
}

/*
 * Generates STOP condition on the I2C bus.
 * Algorithm: Set the STOP bit in CR1 register. Hardware will generate the STOP
 * condition after current byte transmission/reception is complete.
 */
void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx)
{
    pI2Cx->CR1 |= (1U << I2C_CR1_STOP);
}

void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
        pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);
    }
    else 
    {
        pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 &= ~(1U << I2C_CR2_ITERREN);
        pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);
    }
}