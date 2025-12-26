#include <stdio.h>
#include <string.h>
#include "stm32f411xx.h"
#include "stm32f411xx_i2c_driver.h"
#include "stm32f411xx_gpio_driver.h"

#define MY_ADDR                             0x61
#define SLAVE_ADDR                          0x68

I2C_Handle_t I2C1Handle;

// Some data
uint8_t some_data[] = "I2C Tx API testing\n";

void delay (void)
{
    for (uint32_t i = 0; i < 500000 / 2; i++);
}

/*
 * PB6 -> SCL
 * PB9 -> SDA
 */
void I2C1_GPIOInits(void)
{
    GPIO_Handle_t I2CPins;

    I2CPins.pGPIOx = GPIOB;
    I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
    I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
    I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    // SCL
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_6;
    GPIO_Init(&I2CPins);
}

void I2C1_Inits(void)
{
    I2C1Handle.I2C_Config.I2C_ACKControl = I2C_ACK_ENABLE;
    I2C1Handle.I2C_Config.I2C_DeviceAddress = MY_ADDR;
    I2C1Handle.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;
    I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;

    I2C_Init(&I2C1Handle);
}

void GPIO_ButtonInit(void)
{
    GPIO_Handle_t GPIO_BUTTON;
    GPIO_BUTTON.pGPIOx = GPIOC;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_13;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

    GPIO_Init(&GPIO_BUTTON);
}

int main()
{
    I2C1_GPIOInits();
    I2C1_Inits();

    // Enable I2C peripheral
    I2C_PeripheralControl(I2C1, ENABLE);

    // Wait for button press
    while (1)
    {
        while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_13)));
        delay();

        // Send some data to slave
        I2C_MasterSendData(&I2C1Handle, some_data, strlen((char*)some_data), SLAVE_ADDR);    
    }  
}

