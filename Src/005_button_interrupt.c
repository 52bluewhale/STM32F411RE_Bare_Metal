#include "stm32f411xx.h"
#include "stm32f411xx_gpio_driver.h"
#include <string.h>

#define HIGH            1
#define LOW             0
#define BTN_PRESSED     LOW

int main (void)
{
    GPIO_Handle_t GpioLed;
    memset(&GpioLed, sizeof(GpioLed));

    GpioLed.pGPIOx = GPIOD;
    GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_12;
    GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    GpioLed.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_LOW;
    GpioLed.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    GpioLed.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_PeriClockControl(GPIOD, ENABLE);

    GPIO_Init(&GpioLed);

    GPIO_Handle_t GpioBtn;
    GpioBtn.pGPIOx = GPIOD;
    GpioBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_5;
    GpioBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IT_FT;
    GpioBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    GpioBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    // GpioLed.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_PeriClockControl(GPIOD, ENABLE);

    GPIO_Init(&GpioBtn);

    // IRQ configurations for button
    GPIO_IRQPriorityConfig(IRQ_EXTI9_5, NVIC_IRQ_PRI15);
    GPIO_IRQInterruptConfig(IRQ_EXTI9_5, ENABLE);

    while (1)
    {
        /* code */
    }
    
}

void EXTI9_5_IRQHandler(void)
{
    delay();
    GPIO_IRQHandling(GPIO_PIN_5);
    GPIO_ToggleOutputPin(GPIOD, GPIO_PIN_12);
}