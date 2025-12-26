/* SPI2 pin documentation 
 * Alternate Function: AF05
 * PB9  - SPI2_NSS/I2S2_WS
 * PB10 - SPI2_SCK/I2S2_CK
 * PB11 - I2S2_CKIN
 * PB12 - SPI2_NSS/I2S2_WS
 * PB13 - SPI2_SCK/I2S2_CK
 * PB14 - SPI2_MISO
 * PB15 - SPI2_MOSI/I2S2_SD
 */

#include <string.h>
#include "stm32f411xx.h"
#include "stm32f411xx_gpio_driver.h"
#include "stm32f411xx_spi_driver.h"

#define CMD_LED_CTRL                    0x50
#define CMD_SENSOR_READ                 0x51
#define CMD_LED_READ                    0x52
#define CMD_PRINT                       0x53
#define CMD_ID_READ                     0x54

#define LED_ON                          1
#define LED_OFF                         0

#define ANALOG_PIN0                     0
#define ANALOG_PIN1                     1
#define ANALOG_PIN2                     2
#define ANALOG_PIN3                     3
#define ANALOG_PIN4                     4

#define LED_PIN                         9

void SPI2_GPIOInits(void)
{
    GPIO_Handle_t SPIPins;

    SPIPins.pGPIOx = GPIOB;
    SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;
    SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    // PB13 - SPI2_SCK/I2S2_CK
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_13;
    GPIO_Init(&SPIPins);

    // PB15 - SPI2_MOSI/I2S2_SD
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_15;
    GPIO_Init(&SPIPins);

    // PB14 - SPI2_MISO
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_14;
    GPIO_Init(&SPIPins);

    // PB9  - SPI2_NSS/I2S2_WS / PB12 - SPI2_NSS/I2S2_WS
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_12;
    GPIO_Init(&SPIPins);
}

void SPI2_Inits(void)
{
    SPI_Handle_t SPI2Handle;

    SPI2Handle.pSPIx = SPI2;
    SPI2Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
    SPI2Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MOD_MASTER;
    SPI2Handle.SPIConfig.SPI_SclkSpeed = SPI_CLK_SPEED_DIV8;        // 2 MHz
    SPI2Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
    SPI2Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
    SPI2Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
    SPI2Handle.SPIConfig.SPI_SSM = SPI_SSM_DI;                      // Hw slave management enabled for NSS

    SPI_Init(&SPI2Handle);
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

void delay (void)
{
    for (uint32_t i = 0; i < 500000 / 2; i++);
}

uint8_t SPI_VerifyResponse(uint8_t ackbyte)
{
    if (ackbyte == 0xF5)
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

int main(void)
{
    // char user_data[] = "Hello World";

    uint8_t dummy_write = 0xff;
    uint8_t dummy_read;

    GPIO_ButtonInit();

    SPI2_GPIOInits();

    SPI2_Inits();

    SPI_SSOEConfig(SPI2, ENABLE);

    while (1)
    {

        while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_13)));
        delay();

        // SPI_SSIConfig(SPI2, ENABLE);

        SPI_PeripheralControl(SPI2, ENABLE);

        /* -----------------Start of CMD_LED_CTRL----------------- */
        // 1. CMD_LED_CTRL <pin no(1)>  <value(1)>
        uint8_t cmdcode = CMD_LED_CTRL;
        uint8_t ackbyte;
        uint8_t args[2];

        // send command code 
        SPI_SendData(SPI2, &cmdcode, 1);
        // do dummy read to clear off the RXNE
        SPI_ReceiveData(SPI2, &dummy_read, 1);

        // send some dummy bit (1 byte) fetch the response from the slave
        SPI_SendData(SPI2, &dummy_write, 1);
        // read the ack byte received
        SPI_ReceiveData(SPI2, &ackbyte, 1);
        if (SPI_VerifyResponse(ackbyte))
        {
            args[0] = LED_PIN;
            args[1] = LED_ON;

            // send arguments 
            SPI_SendData(SPI2, args, 2);
        }
        /* -----------------End of CMD_LED_CTRL----------------- */

        /* -----------------Start of CMD_SENSOR_READ----------------- */
        // 2. CMD_SENSOR_READ
        while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_13)));
        delay();

        cmdcode = CMD_SENSOR_READ;

        // send command code 
        SPI_SendData(SPI2, &cmdcode, 1);
        // do dummy read to clear off the RXNE
        SPI_ReceiveData(SPI2, &dummy_read, 1);

        // send some dummy bit (1 byte) fetch the response from the slave
        SPI_SendData(SPI2, &dummy_write, 1);
        // read the ack byte received
        SPI_ReceiveData(SPI2, &ackbyte, 1);
        if (SPI_VerifyResponse(ackbyte))
        {
            args[0] = ANALOG_PIN0;

            // send arguments
            SPI_SendData(SPI2, args, 1);

            // Do dummy read to clear off the RXNE
            SPI_ReceiveData(SPI2, &dummy_read, 1);

            // delay for slave can ready with data
            delay();

            // send some dummy bit (1 byte) fetch the response from the slave
            SPI_SendData(SPI2, &dummy_write, 1);

            uint8_t analog_read;
            SPI_ReceiveData(SPI2, &analog_read, 1);
        }

        
        /* -----------------End of CMD_SENSOR_READ----------------- */

        // SPI_SendData(SPI2, (uint8_t*)user_data, strlen(user_data));

        while (SPI_GetFlagStatus(SPI2, SPI_SR_BSY));

        SPI_PeripheralControl(SPI2, DISABLE);
    }

    return 0;
}
