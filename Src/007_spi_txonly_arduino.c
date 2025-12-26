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

    // // PB14 - SPI2_MISO
    // SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_14;
    // GPIO_Init(&SPIPins);

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
    GPIO_BUTTON.pGPIOx = GPIOA;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_5;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    GPIO_BUTTON.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

    GPIO_Init(&GPIO_BUTTON);
}

void delay (void)
{
    for (uint32_t i = 0; i < 500000 / 2; i++);
}

int main(void)
{
    char user_data[] = "Hello World";

    GPIO_ButtonInit();

    SPI2_GPIOInits();

    SPI2_Inits();

    SPI_SSOEConfig(SPI2, ENABLE);

    while (1)
    {

        while (!(GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_5)));

        delay();

        // SPI_SSIConfig(SPI2, ENABLE);

        SPI_PeripheralControl(SPI2, ENABLE);

        uint8_t dataLen = strlen(user_data);
        SPI_SendData(SPI2, &dataLen, 1);

        SPI_SendData(SPI2, (uint8_t*)user_data, strlen(user_data));

        while (SPI_GetFlagStatus(SPI2, SPI_SR_BSY));

        SPI_PeripheralControl(SPI2, DISABLE);
    }

    return 0;
}
