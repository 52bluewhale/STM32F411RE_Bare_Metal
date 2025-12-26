#include <stdio.h>
#include <string.h>
#include "stm32f411xx.h"
#include "stm32f411xx_spi_driver.h"

SPI_Handle_t SPI2handle;

#define MAX_LEN                 500

char RcvBuff[MAX_LEN];

volatile char ReadByte;
volatile uint8_t rcvStop = 0;

// This flag will be set in the interrupt handler of the arrduino interrupt GPIO
volatile uint8_t dataAvailable = 0;

void delay(void)
{
    for (uint32_t i = 0; i < 500000/2; i++);
}

/*
 * PB14 -> SPI2_MISO
 * PB15 -> SPI2_MOSI
 * PB13 ->
 */
