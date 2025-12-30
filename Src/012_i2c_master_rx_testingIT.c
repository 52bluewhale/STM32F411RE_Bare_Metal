#include <stdio.h>
#include <string.h>
#include "stm32f411xx.h"
#include "stm32f411xx_i2c_driver.h"
#include "stm32f411xx_gpio_driver.h"

#define MY_ADDR                             0x61
#define SLAVE_ADDR                          0x68

I2C_Handle_t I2C1Handle;

// Some data
uint8_t rcv_data[32];
uint8_t Rx_Complete = RESET;

extern void initialise_monitor_handles();

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
    uint8_t command_code;
    uint8_t len;

    initialise_monitor_handles();

    printf("Application is running\n");

    I2C1_GPIOInits();
    I2C1_Inits();

    // I2C IRQ configurations 
    I2C_IRQInterruptConfig(IRQ_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_I2C1_ER, ENABLE);

    // Enable I2C peripheral
    I2C_PeripheralControl(I2C1, ENABLE);

    // ACK control bit
    I2C_ManageACK(I2C1, I2C_ACK_ENABLE);

    // Wait for button press
    while (1)
    {
        while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_13)));
        delay();

        command_code = 0x51;

        while(I2C_MasterSendDataIT(&I2C1Handle, &command_code, 1, SLAVE_ADDR, I2C_SR_ENABLE) != I2C_READY);
        while(I2C_MasterReceiveDataIT(&I2C1Handle, &len, 1, SLAVE_ADDR, I2C_SR_ENABLE) != I2C_READY);

        command_code = 0x52;
        while(I2C_MasterSendDataIT(&I2C1Handle, &command_code, 1, SLAVE_ADDR, I2C_SR_ENABLE) != I2C_READY);
        while(I2C_MasterReceiveDataIT(&I2C1Handle, &len, 1, SLAVE_ADDR, I2C_SR_ENABLE) != I2C_READY);

        Rx_Complete = RESET;
        while (Rx_Complete != SET);

        rcv_data[len+1] = '\0';
        printf("Data: %s", rcv_data);
    }  
}

void I2C1_EV_IRQHandler()
{
    I2C_EV_IRQHandling(&I2C1Handle);
}

void I2C1_ER_IRQHandler()
{
    I2C_ER_IRQHandling(&I2C1Handle);
}

void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
    if (AppEv == I2C_EV_TX_CMPLT)
    {
        printf("Tx is completed\n");
    }
    else if (AppEv == I2C_EV_RX_CMPLT)
    {
        printf("Rx is completed\n");
        Rx_Complete = SET;
    }
    else if (AppEv == I2C_ERROR_AF)
    {
        printf("ERROR: ACK failure\n");
        // In master ACK failure happens when slave fails to send ACK
        // for the byte sent from the master
        I2C_CloseSendData(pI2CHandle);

        I2C_GenerateStopCondition(pI2CHandle);

        while(1);
    }
}