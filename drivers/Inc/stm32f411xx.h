/*
 * stm32f411xx.h
 *
 *  Created on: Oct 15, 2025
 *      Author: phanc
 */

#ifndef INC_STM32F411XX_H_
#define INC_STM32F411XX_H_

#include <stdint.h>
#include "stm32f411xx_gpio_driver.h"
#include "stm32f411xx_spi_driver.h"

/* Base addresses of FLASH and SRAM memories */
#define FLASH_BASEADDR                  0x08000000U
#define SRAM1_BASEADDR                  0x20000000U
#define SRAM                            SRAM1_BASEADDR

/* AHBx and APBx bus peripheral base addresses */
#define PERIPH_BASE                     0x40000000U
#define APB1PERIPH_BASE                 PERIPH_BASE
#define APB2PERIPH_BASE                 0x40010000U
#define AHB1PERIPH_BASE                 0x40020000U
#define AHB2PERIPH_BASE                 0x50000000U

/* ARM Cortex Mx Processor number of priority bits implemented in Priority Register */
#define NO_PR_BITS_IMPLEMENTED          4

/* AHB1 bus peripherals base addresses */
#define GPIOA_BASEADDR                  (AHB1PERIPH_BASE + 0x0000U)
#define GPIOB_BASEADDR                  (AHB1PERIPH_BASE + 0x0400U)
#define GPIOC_BASEADDR                  (AHB1PERIPH_BASE + 0x0800U)
#define GPIOD_BASEADDR                  (AHB1PERIPH_BASE + 0x0C00U)
#define GPIOE_BASEADDR                  (AHB1PERIPH_BASE + 0x1000U)
#define GPIOH_BASEADDR                  (AHB1PERIPH_BASE + 0x1C00U)
#define CRC_BASEADDR                    (AHB1PERIPH_BASE + 0x3000U)
#define RCC_BASEADDR                    (AHB1PERIPH_BASE + 0x3800U)
#define DMA1_BASEADDR                   (AHB1PERIPH_BASE + 0x6000U)
#define DMA2_BASEADDR                   (AHB1PERIPH_BASE + 0x6400U)

/* APB1 bus peripherals base addresses */
#define TIM2_BASEADDR                   (APB1PERIPH_BASE + 0x0000U)
#define TIM3_BASEADDR                   (APB1PERIPH_BASE + 0x0400U)
#define TIM4_BASEADDR                   (APB1PERIPH_BASE + 0x0800U)
#define TIM5_BASEADDR                   (APB1PERIPH_BASE + 0x0C00U)
#define WWDG_BASEADDR                   (APB1PERIPH_BASE + 0x2C00U)
#define IWDG_BASEADDR                   (APB1PERIPH_BASE + 0x3000U)
#define I2S2EXT_BASEADDR                (APB1PERIPH_BASE + 0x3400U)
#define SPI2_I2S2_BASEADDR              (APB1PERIPH_BASE + 0x3800U)
#define SPI3_I2S3_BASEADDR              (APB1PERIPH_BASE + 0x3C00U)
#define I2S3EXT_BASEADDR                (APB1PERIPH_BASE + 0x4000U)
#define USART2_BASEADDR                 (APB1PERIPH_BASE + 0x4400U)
#define I2C1_BASEADDR                   (APB1PERIPH_BASE + 0x5400U)
#define I2C2_BASEADDR                   (APB1PERIPH_BASE + 0x5800U)
#define I2C3_BASEADDR                   (APB1PERIPH_BASE + 0x5C00U)
#define PWR_BASEADDR                    (APB1PERIPH_BASE + 0x7000U)

/* APB2 bus peripherals base addresses */
#define TIM1_BASEADDR                   (APB2PERIPH_BASE + 0x0000U)
#define USART1_BASEADDR                 (APB2PERIPH_BASE + 0x1000U)
#define USART6_BASEADDR                 (APB2PERIPH_BASE + 0x1400U)
#define ADC1_BASEADDR                   (APB2PERIPH_BASE + 0x2000U)
#define SDIO_BASEADDR                   (APB2PERIPH_BASE + 0x2C00U)
#define SPI1_I2S1_BASEADDR              (APB2PERIPH_BASE + 0x3000U)
#define SPI4_I2S4_BASEADDR              (APB2PERIPH_BASE + 0x3400U)
#define SYSCFG_BASEADDR                 (APB2PERIPH_BASE + 0x3800U)
#define EXTI_BASEADDR                   (APB2PERIPH_BASE + 0x3C00U)
#define TIM9_BASEADDR                   (APB2PERIPH_BASE + 0x4000U)
#define TIM10_BASEADDR                  (APB2PERIPH_BASE + 0x4400U)
#define TIM11_BASEADDR                  (APB2PERIPH_BASE + 0x4800U)
#define SPI5_I2S5_BASEADDR              (APB2PERIPH_BASE + 0x5000U)

/* ARM Cortex Mx Processor NVIC ISERx register addresses */
#define NVIC_ISER0                      ((volatile uint32_t *)0xE000E100)
#define NVIC_ISER1                      ((volatile uint32_t *)0xE000E104)
#define NVIC_ISER2                      ((volatile uint32_t *)0xE000E108)
#define NVIC_ISER3                      ((volatile uint32_t *)0xE000E10C)
#define NVIC_ICER0                      ((volatile uint32_t *)0XE000E180)
#define NVIC_ICER1                      ((volatile uint32_t *)0XE000E184)
#define NVIC_ICER2                      ((volatile uint32_t *)0XE000E188)
#define NVIC_ICER3                      ((volatile uint32_t *)0XE000E18C)

/* ARM Cortex Mx Processor Priority register address calculation*/
#define NVIC_PR_BASE_ADDR               ((volatile uint32_t *)0xE000E400)

/* IRQ number of STM32F411RE */
#define IRQ_EXTI0                       6
#define IRQ_EXTI1                       7
#define IRQ_EXTI2                       8
#define IRQ_EXTI3                       9
#define IRQ_EXTI4                       10
#define IRQ_EXTI9_5                     23
#define IRQ_EXTI15_10                   40

/* NVIC priority levels */
#define NVIC_IRQ_PRI0                   0
#define NVIC_IRQ_PRI15                  15

/* Peripheral register definition structures */

typedef struct 
{
    volatile uint32_t MODER;         // Offset: 0x00 - GPIO port mode register 
    volatile uint32_t OTYPER;        // Offset: 0x04 - GPIO port output type register
    volatile uint32_t OSPEEDR;       // Offset: 0x08 - GPIO port output speed register
    volatile uint32_t PUPDR;         // Offset: 0x0C - GPIO port pull-up/pull-down register
    volatile uint32_t IDR;           // Offset: 0x10 - GPIO port input data register
    volatile uint32_t ODR;           // Offset: 0x14 - GPIO port output data register
    volatile uint32_t BSRR;          // Offset: 0x18 - GPIO port bit set/reset register
    volatile uint32_t LCKR;          // Offset: 0x1C - GPIO port configuration lock register
    volatile uint32_t AFR[2];        // Offset: 0x20 - GPIO alternate function low register
                                     // Offset: 0x24 - GPIO alternate function high register
} GPIO_RegDef_t;

typedef struct
{
    volatile uint32_t CR;           // Offset: 0x00 - RCC clock control register
    volatile uint32_t PLLCFGR;      // Offset: 0x04 - PLL (Phase-Clock Loop) configuration register
    volatile uint32_t CFGR;         // Offset: 0x08 - Clock configuration register 
    volatile uint32_t CIR;          // Offset: 0x0C - Clock interrupt register
    volatile uint32_t AHB1RSTR;     // Offset: 0x10 - AHB1 peripheral reset register
    volatile uint32_t AHB2RSTR;     // Offset: 0x14 - AHB2 peripheral reset register
    uint32_t RESERVED0[2];          
    volatile uint32_t APB1RSTR;     // Offset: 0x20 - APB1 peripheral reset register
    volatile uint32_t APB2RSTR;     // Offset: 0x24 - APB2 peripheral reset register
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;      // Offset: 0x30 - AHB1 peripheral clock enable register
    volatile uint32_t AHB2ENR;      // Offset: 0x34 - AHB2 peripheral clock enable register
    uint32_t RESERVED2[2];
    volatile uint32_t APB1ENR;      // Offset: 0x40 - APB1 peripheral clock enable register
    volatile uint32_t APB2ENR;      // Offset: 0x44 - APB2 peripheral clock enable register
    uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;    // Offset: 0x50 - AHB1 peripheral clock enable in low power mode register
    volatile uint32_t AHB2LPENR;    // Offset: 0x54 - AHB2 peripheral clock ebable in low power mode register
    uint32_t RESERVED4[2];
    volatile uint32_t APB1LPENR;    // Offset: 0x60 - APB1 peripheral clock enable in low power mode register
    volatile uint32_t APB2LPENR;    // Offset: 0x64 - APB2 peripheral clock enable in low power mode register
    uint32_t RESERVED5[2];
    volatile uint32_t BDCR;         // Offset: 0x70 - Backup Domain Control Register
    volatile uint32_t CSR;          // Offset: 0x74 - Clock control & Status Register
    uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;        // Offset: 0x80 - Spread spectrum clock generation register
    volatile uint32_t PLLI2SCFGR;   // Offset: 0x84 - PLLI2S configuration register
    uint32_t RESERVED7[1];
    volatile uint32_t DCKCFGR;      // Offset: 0x8C - Dedicated clocks configuration register
} RCC_RegDef_t;

/* Peripheral register for EXTI*/
typedef struct 
{
    volatile uint32_t IMR;          // Offset: 0x00 - Interrupt mask register
    volatile uint32_t EMR;          // Offset: 0x04 - Event mask register
    volatile uint32_t RTSR;         // Offset: 0x08 - Rising trigger selection register
    volatile uint32_t FTSR;         // Offset: 0x0C - Falling trigger selection register
    volatile uint32_t SWIER;        // Offset: 0x10 - Sw interrupt event register
    volatile uint32_t PR;           // Offset: 0x14 - Pending register
} EXTI_RegDef_t;

/* Peripheral register for SYSCFG */
typedef struct 
{
    volatile uint32_t MEMRMP;       // Offset: 0x00 - memory remap register
    volatile uint32_t PMC;          // Offset: 0x04 - peripheral mode configuration register
    volatile uint32_t EXTICR[4];    // Offset: 0x08 - external interrupt configuration register 1
    uint32_t RESERVED[2];
    volatile uint32_t CMPCR;        // Offset: 0x20 - Compensation cell control register 
} SYSCFG_RegDef_t;

/* Peripheral register for SPI */
typedef struct
{
    volatile uint32_t CR1;          // Offset: 0x00 - SPI control register 1
    volatile uint32_t CR2;          // Offset: 0x04 - SPI control register 2
    volatile uint32_t SR;           // Offset: 0x08 - SPI status register
    volatile uint32_t DR;           // Offset: 0x0C - SPI data register 
    volatile uint32_t CRCPR;        // Offset: 0x10 - SPI CRC polynomial register
    volatile uint32_t RXCRCR;       // Offset: 0x14 - SPI RX CRC register
    volatile uint32_t TXCRCR;       // Offset: 0x18 - SPI TX CRC register
    volatile uint32_t I2SCFGR;      // Offset: 0x1C - SPI_I2S configuration register
    volatile uint32_t I2SPR;        // Offset: 0x20 - SPI_I2S prescaler register
} SPI_RegDef_t;


/* RCC definition */
#define RCC         ((RCC_RegDef_t *)RCC_BASEADDR)

/* GPIOx enable bit definitions */
#define GPIOA_PCLK_EN()       (RCC->AHB1ENR |= (1U<<0))
#define GPIOB_PCLK_EN()       (RCC->AHB1ENR |= (1U<<1))
#define GPIOC_PCLK_EN()       (RCC->AHB1ENR |= (1U<<2))
#define GPIOD_PCLK_EN()       (RCC->AHB1ENR |= (1U<<3))
#define GPIOE_PCLK_EN()       (RCC->AHB1ENR |= (1U<<4))
#define GPIOH_PCLK_EN()       (RCC->AHB1ENR |= (1U<<7))

/* GPIOx disable bit definitions */
#define GPIOA_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<0))
#define GPIOB_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<1))
#define GPIOC_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<2))
#define GPIOD_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<3))
#define GPIOE_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<4))
#define GPIOH_PCLK_DI()       (RCC->AHB1ENR &= ~(1U<<7))

/* GPIOx reset bit definitions */
#define GPIOA_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<0)); (RCC->AHB1RSTR &= ~(1U<<0));} while(0)
#define GPIOB_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<1)); (RCC->AHB1RSTR &= ~(1U<<1));} while(0)
#define GPIOC_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<2)); (RCC->AHB1RSTR &= ~(1U<<2));} while(0)  
#define GPIOD_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<3)); (RCC->AHB1RSTR &= ~(1U<<3));} while(0)  
#define GPIOE_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<4)); (RCC->AHB1RSTR &= ~(1U<<4));} while(0)  
#define GPIOH_PCLK_RESET() \
    do {(RCC->AHB1RSTR |= (1U<<7)); (RCC->AHB1RSTR &= ~(1U<<7));} while(0)  

/* I2Cx enable bit definitions */
#define I2C1_PCLK_EN()        (RCC->APB1ENR |= (1U<<21))
#define I2C2_PCLK_EN()        (RCC->APB1ENR |= (1U<<22))
#define I2C3_PCLK_EN()        (RCC->APB1ENR |= (1U<<23))

/* I2Cx disable bit definitions */
#define I2C1_PCLK_DI()        (RCC->APB1ENR &= ~(1U<<21))
#define I2C2_PCLK_DI()        (RCC->APB1ENR &= ~(1U<<22))
#define I2C3_PCLK_DI()        (RCC->APB1ENR &= ~(1U<<23))

/* SPIx enable bit definitions */
#define SPI1_PCLK_EN()        (RCC->APB2ENR |= (1U<<12))
#define SPI2_PCLK_EN()        (RCC->APB1ENR |= (1U<<14))
#define SPI3_PCLK_EN()        (RCC->APB1ENR |= (1U<<15))
#define SPI4_PCLK_EN()        (RCC->APB2ENR |= (1U<<13))
#define SPI5_PCLK_EN()        (RCC->APB2ENR |= (1U<<20))

/* SPIx disable bit definitions */
#define SPI1_PCLK_DI()        (RCC->APB2ENR &= ~(1U<<12))
#define SPI2_PCLK_DI()        (RCC->APB1ENR &= ~(1U<<14))
#define SPI3_PCLK_DI()        (RCC->APB1ENR &= ~(1U<<15))
#define SPI4_PCLK_DI()        (RCC->APB2ENR &= ~(1U<<13))
#define SPI5_PCLK_DI()        (RCC->APB2ENR &= ~(1U<<20))

/* SPIx reset bit definitions */
#define SPI1_PCLK_RESET() \
    do {(RCC->APB2RSTR |= (1U << 12)); (RCC->APB2RSTR &= ~(1U << 12));} while(0)
#define SPI2_PCLK_RESET() \
    do {(RCC->APB1RSTR |= (1U << 14)); (RCC->APB1RSTR &= ~(1U << 14));} while(0)
#define SPI3_PCLK_RESET() \
    do {(RCC->APB1RSTR |= (1U << 15)); (RCC->APB1RSTR &= ~(1U << 15));} while(0)
#define SPI4_PCLK_RESET() \
    do {(RCC->APB2RSTR |= (1U << 13)); (RCC->APB2RSTR &= ~(1U << 13));} while(0)
#define SPI5_PCLK_RESET() \
    do {(RCC->APB2RSTR |= (1U << 20)); (RCC->APB2RSTR &= ~(1U << 20));} while(0)

/* USARTx enable bit definitions */
#define USART1_PCLK_EN()      (RCC->APB2ENR |= (1U<<4))
#define USART2_PCLK_EN()      (RCC->APB1ENR |= (1U<<17))
#define USART6_PCLK_EN()      (RCC->APB2ENR |= (1U<<5))

/* USARTx disable bit definitions */
#define USART1_PCLK_DI()      (RCC->APB2ENR &= ~(1U<<4))
#define USART2_PCLK_DI()      (RCC->APB1ENR &= ~(1U<<17))
#define USART6_PCLK_DI()      (RCC->APB2ENR &= ~(1U<<5))

/* SYSCFG enable bit definition */
#define SYSCFG_PCLK_EN()      (RCC->APB2ENR |= (1U<<14))

/* SYSCFG disable bit definition */
#define SYSCFG_PCLK_DI()      (RCC->APB2ENR &= ~(1U<<14))

#define GPIO_BASEADDR_TO_CODE(x)       ((x == GPIOA) ? 0 : \
                                        (x == GPIOB) ? 1 : \
                                        (x == GPIOC) ? 2 : \
                                        (x == GPIOD) ? 3 : \
                                        (x == GPIOE) ? 4 : \
                                        (x == GPIOH) ? 7 : 0)

/* Interrupt definitions*/
#define EXTI        ((EXTI_RegDef_t *)EXTI_BASEADDR)
#define SYSCFG      ((SYSCFG_RegDef_t *)SYSCFG_BASEADDR)

/* Generic macros */
#define ENABLE              1
#define DISABLE             0
#define SET                 ENABLE
#define RESET               DISABLE
#define GPIO_PIN_SET        SET
#define GPIO_PIN_RESET      RESET
#define FLAG_SET            SET
#define FLAG_RESET          RESET


#endif /* INC_STM32F411XX_H_ */



