/*
 * stm32f411xx_gpio_driver_new.h
 *
 *  Created on: Nov 11, 2025
 *      Author: phanc
 */

#ifndef INC_STM32F411XX_GPIO_DRIVER_H_
#define INC_STM32F411XX_GPIO_DRIVER_H_

#include "stm32f411xx.h"

/* Peripheral definitions for GPIO instances */
#define GPIOA       ((GPIO_RegDef_t *)GPIOA_BASEADDR)
#define GPIOB       ((GPIO_RegDef_t *)GPIOB_BASEADDR)
#define GPIOC       ((GPIO_RegDef_t *)GPIOC_BASEADDR)
#define GPIOD       ((GPIO_RegDef_t *)GPIOD_BASEADDR)
#define GPIOE       ((GPIO_RegDef_t *)GPIOE_BASEADDR)
#define GPIOH       ((GPIO_RegDef_t *)GPIOH_BASEADDR)

/* ========================= GPIO HANDLER - CONFIGURATION STRUCTURES ========================= */
/* GPIO configuration structure */
typedef struct
{
    uint8_t GPIO_PinNumber;
    uint8_t GPIO_PinMode;           /* GPIO Pin Mode*/
    uint8_t GPIO_PinSpeed;          /* GPIO Output Speed */
    uint8_t GPIO_PinPuPdControl;    /* GPIO Pin PUPD */
    uint8_t GPIO_PinOPType;         /* GPIO Output Types */
    uint8_t GPIO_PinAltFunMode;
} GPIO_PinConfig_t;


/* GPIO handler structure */
typedef struct
{
    /*Pointer to hold the base address of GPIO peripheral*/
    GPIO_RegDef_t *pGPIOx;       // This hold the  base address of the GPIO port to which the pin belongs
    GPIO_PinConfig_t GPIO_PinConfig;    // This hold GPIO pin configuration settings
} GPIO_Handle_t;

/* ========================= GPIO CONFIGURATION MACROS ========================= */
/* GPIO Pin Number*/
#define GPIO_PIN_0           0
#define GPIO_PIN_1           1
#define GPIO_PIN_2           2
#define GPIO_PIN_3           3
#define GPIO_PIN_4           4
#define GPIO_PIN_5           5
#define GPIO_PIN_6           6
#define GPIO_PIN_7           7
#define GPIO_PIN_8           8
#define GPIO_PIN_9           9
#define GPIO_PIN_10          10
#define GPIO_PIN_11          11
#define GPIO_PIN_12          12
#define GPIO_PIN_13          13
#define GPIO_PIN_14          14
#define GPIO_PIN_15          15

/* GPIO Pin Mode*/
#define GPIO_MODE_IN                    0   // Input
#define GPIO_MODE_OUT                   1   // General purpose output mode
#define GPIO_MODE_ALTFN                 2   // Alternate Function mode
#define GPIO_MODE_ANALOG                3   // Analog mode
#define GPIO_MODE_IT_FT                 4   // Interrupt Input Falling edge Trigger
#define GPIO_MODE_IT_RT                 5   // Interrupt Input Rising edge Trigger
#define GPIO_MODE_IT_RFT                6   // Interrupt Input Rising-Falling edge Trigger

/* GPIO Output Types */
#define GPIO_OP_TYPE_PP                 0   // Push-Pull
#define GPIO_OP_TYPE_OD                 1   // Open-drain

/* GPIO Output Speed */
#define GPIO_SPEED_LOW                  0   // Low speed
#define GPIO_SPEED_MEDIUM               1   // Medium speed
#define GPIO_SPEED_FAST                 2   // High speed
#define GPIO_SPEED_HIGH                 3   // Very high speed

/* GPIO Pin PUPD */
#define GPIO_NO_PUPD                    0   // No PUPD
#define GPIO_PIN_PU                     1   // Pull Up
#define GPIO_PIN_PD                     2   // Pull Down



/* ========================= GPIO APIs ========================= */

/* Peripheral clock control */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi);

/* Init and DeInit */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle);
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx);

/* Read Data */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber);
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx);

/* Write Data */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value);
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint8_t Value);
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber);

/* IRQ configuration and ISR handling */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void GPIO_IRQHandling(uint8_t PinNumber);

#endif /* INC_STM32F411XX_GPIO_DRIVER_H_ */
