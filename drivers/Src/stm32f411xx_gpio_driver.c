/*
 * stm32f411xx_gpio_driver.c
 *
 *  Created on: Oct 17, 2025
 *      Author: phanc
 */


#include "stm32f411xx_gpio_driver.h"

/*GPIO APIs*/

/* Peripheral clock control
 * @fn      - GPIO_PeriClockControl
 *
 * @brief   - this function enable/disable peripheral clock for the given GPIO port
 *
 * @param   -   pGPIOx: base address of the GPIO peripheral
 * @param   -   EnorDi: ENABLE or DISABLE macros
 *
 * @return  - none
 *
 * @note    - none
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pGPIOx == GPIOA)
        {
            GPIOA_PCLK_EN();
        }
        else if (pGPIOx == GPIOB)
        {
            GPIOB_PCLK_EN();
        }
        else if (pGPIOx == GPIOC)
        {
            GPIOC_PCLK_EN();
        }
        else if (pGPIOx == GPIOD)
        {
            GPIOD_PCLK_EN();
        }
        else if (pGPIOx == GPIOE)
        {
            GPIOE_PCLK_EN();
        }
        else if (pGPIOx == GPIOH)
        {
            GPIOH_PCLK_EN();
        }
    }
    else
    {
        //to do
    }
}

/*
 * @fn      - GPIO_Init
 *
 * @brief   - Initialize a GPIO pin according to the configuration in the
 *            supplied GPIO_Handle_t. Configures mode (including AF),
 *            output type, speed, pull-up/pull-down and alternate function.
 *
 * @param   - pGPIOHandle: pointer to a GPIO_Handle_t that contains
 *                        the peripheral base address and pin settings.
 *
 * @return  - none
 *
 * @note    - The peripheral clock for the GPIO port must be enabled prior to
 *            calling this function. Interrupt mode handling is not implemented
 *            in this function (placeholder present in code).
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    uint32_t temp;      // temp register

    // Enable the peripheral clock
    GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

    /* Step 1: Configure the mode of GPIO pin */
    if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG)
    {
        // non-interrupt mode
        temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinMode << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        pGPIOHandle->pGPIOx->MODER |= temp;
    }
    else
    {
        if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT)
        {
            // 1. Configure the FTSR
            EXTI->FTSR |= (1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->RTSR &= ~(1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT)
        {
            // 1. Configure the RTSR
            EXTI->RTSR |= (1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->FTSR &= ~(1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT)
        {
            // 1. Configure both FTSR and RTSR
            EXTI->FTSR |= (1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->RTSR |= (1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }

        // 2. Configure the GPIO port selection in SYSCFG_EXTICR
        uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4;
        uint8_t temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;
        uint8_t portcode = GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);

        SYSCFG_PCLK_EN();
        SYSCFG->EXTICR[temp1] = portcode << (temp2 * 4);

        // 3. Enable EXTI interrupt delivery using IMR
        EXTI->IMR |= (1U << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    }
    temp = 0;

    /* Step 2: Configure the speed */
    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->OSPEEDR &= ~(0x3 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->OSPEEDR |= temp;
    temp = 0;

    /* Step 3: Configure the PUPD */
    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->PUPDR |= temp;
    temp = 0;

    /* Step 4: Configure the Output Type */
    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinOPType << (pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
    pGPIOHandle->pGPIOx->OTYPER |= temp;
    temp = 0;

    /* Step 5: Configure the Alternate Functionality */
    if (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode == GPIO_MODE_ALTFN)
    {
        // Configure the AF register
        uint32_t temp1, temp2;
        temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 8;
        temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;
        pGPIOHandle->pGPIOx->AFR[temp1] &= ~(0xF << 4 * temp2);
        pGPIOHandle->pGPIOx->AFR[temp1] |= pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode << (4 * temp2);
    }
}

/*
 * @fn      - GPIO_DeInit
 *
 * @brief   - Reset the GPIO peripheral registers to their reset state.
 *
 * @param   - pGPIOx: base address of the GPIO peripheral to be reset
 *
 * @return  - none
 *
 * @note    - This issues a reset pulse via the RCC AHB1 peripheral reset
 *            register macros (GPIOx_PCLK_RESET()). Other peripherals are
 *            unaffected because each macro toggles only the target bit.
 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
    if (pGPIOx == GPIOA)
    {
        GPIOA_PCLK_RESET();
    }
    else if (pGPIOx == GPIOB)
    {
        GPIOB_PCLK_RESET();
    }
    else if (pGPIOx == GPIOC)
    {
        GPIOC_PCLK_RESET();
    }
    else if (pGPIOx == GPIOD)
    {
        GPIOD_PCLK_RESET();
    }
    else if (pGPIOx == GPIOE)
    {
        GPIOE_PCLK_RESET();
    }
    else if (pGPIOx == GPIOH)
    {
        GPIOH_PCLK_RESET();
    }
}

/*
 * @fn      - GPIO_ReadFromInputPin
 *
 * @brief   - Read the logic level present on a single input pin.
 *
 * @param   - pGPIOx:    base address of the GPIO peripheral
 * @param   - PinNumber: pin index (0..15)
 *
 * @return  - uint8_t: 0 if pin is low, 1 if pin is high
 *
 * @note    - Returns the current state from the IDR register. This is a
 *            read-only operation and is safe for single-bit reads. For
 *            guaranteed atomic access in concurrent environments the
 *            bit-banding or BSRR/BRR mechanisms can be considered.
 */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    uint8_t value;
    value = (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x00000001);

    return value;
}

/*
 * @fn      - GPIO_ReadFromInputPort
 *
 * @brief   - Read the entire 16-bit input data register for a GPIO port.
 *
 * @param   - pGPIOx: base address of the GPIO peripheral
 *
 * @return  - uint16_t: lower 16 bits reflect pin states [15:0]
 *
 * @note    - Useful when reading parallel buses or multiple inputs at once.
 */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx)
{
    uint16_t value;
    value = (uint16_t) pGPIOx->IDR;

    return value;
}

/*
 * @fn      - GPIO_WriteToOutputPin
 *
 * @brief   - Set or reset a single GPIO output pin.
 *
 * @param   - pGPIOx:    base address of the GPIO peripheral
 * @param   - PinNumber: pin index (0..15)
 * @param   - Value:     GPIO_PIN_SET (1) to set, GPIO_PIN_RESET (0) to clear
 *
 * @return  - none
 *
 * @note    - Current implementation uses ODR read-modify-write. For atomic
 *            set/reset without read-modify-write use the BSRR register if
 *            available (BSRR can set and reset individual pins atomically).
 */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value)
{
    if (Value == GPIO_PIN_SET)
    {
        // Write 1 to output data register at corresponding pin number
        pGPIOx->ODR |= (1<<PinNumber);
    }
    else
    {
        // Write 0 to output data register at corresponding pin number
        pGPIOx->ODR &= ~(1<<PinNumber);
    }
}

/*
 * @fn      - GPIO_WriteToOutputPort
 *
 * @brief   - Write a 16-bit value to the port output data register (ODR).
 *
 * @param   - pGPIOx: base address of the GPIO peripheral
 * @param   - Value:  lower 16 bits written to ODR
 *
 * @return  - none
 *
 * @note    - Overwrites the entire ODR. To change single bits without
 *            affecting others prefer BSRR (set/reset) or per-pin operations.
 */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint8_t Value)
{
    pGPIOx->ODR = Value;
}

/*
 * @fn      - GPIO_ToggleOutputPin
 *
 * @brief   - Toggle the specified output pin (invert its current state).
 *
 * @param   - pGPIOx:    base address of the GPIO peripheral
 * @param   - PinNumber: pin index (0..15)
 *
 * @return  - none
 *
 * @note    - Uses XOR on ODR. This is a read-modify-write operation and may
 *            not be atomic; for atomic toggle consider disabling interrupts
 *            briefly or using hardware features if the MCU provides them.
 */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    pGPIOx->ODR ^= (1<<PinNumber);
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
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (IRQNumber <= 31)
        {
            // Program ISER0 register
            *NVIC_ISER0 |= (1U << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ISER1 register
            *NVIC_ISER1 |= (1U << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)     // Needed to be reviewed later
        {
            // Program ISER2 register
            *NVIC_ISER3 |= (1U << (IRQNumber % 64));
        }
    }
    else
    {
        if (IRQNumber <= 31)
        {
            // Program ICER0 register
            *NVIC_ICER0 |= (1U << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ICER1 register
            *NVIC_ICER1 |= (1U << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)     // Needed to be reviewed later
        {
            // Program ICER2 register
            *NVIC_ICER3 |= (1U << (IRQNumber % 64));
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
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    // Finding the correct IPR register
    uint8_t iprx = IRQNumber / 4;

    // Finding the section within the IPR
    uint8_t iprx_section = IRQNumber % 4;

    // Calculating the shift amount
    uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

    // Setting the priority
    *(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << (shift_amount));
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
void GPIO_IRQHandling(uint8_t PinNumber)
{
    // Clear EXTI PR register corresponding to pin number
    if (EXTI->PR & (1U << PinNumber))
    {
        // Clear
        EXTI->PR |= (1U << PinNumber);
    }
}
