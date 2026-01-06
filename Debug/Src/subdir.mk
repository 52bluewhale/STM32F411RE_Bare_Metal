################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/008_spi_rx_cmd_arduino.c \
../Src/009_spi_message_rcv_it.c \
../Src/010_i2c_master_tx_testing.c \
../Src/011_i2c_master_rx_testing.c \
../Src/012_i2c_master_rx_testingIT.c \
../Src/013_i2c_slave_tx_string.c \
../Src/syscalls.c \
../Src/sysmem.c 

OBJS += \
./Src/008_spi_rx_cmd_arduino.o \
./Src/009_spi_message_rcv_it.o \
./Src/010_i2c_master_tx_testing.o \
./Src/011_i2c_master_rx_testing.o \
./Src/012_i2c_master_rx_testingIT.o \
./Src/013_i2c_slave_tx_string.o \
./Src/syscalls.o \
./Src/sysmem.o 

C_DEPS += \
./Src/008_spi_rx_cmd_arduino.d \
./Src/009_spi_message_rcv_it.d \
./Src/010_i2c_master_tx_testing.d \
./Src/011_i2c_master_rx_testing.d \
./Src/012_i2c_master_rx_testingIT.d \
./Src/013_i2c_slave_tx_string.d \
./Src/syscalls.d \
./Src/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DNUCLEO_F411RE -DSTM32 -DSTM32F4 -DSTM32F411RETx -c -I../Inc -I"F:/MasteringMicroControllerandEmbeddedDriverDevelopment/stm32f4xx_drivers/drivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/008_spi_rx_cmd_arduino.cyclo ./Src/008_spi_rx_cmd_arduino.d ./Src/008_spi_rx_cmd_arduino.o ./Src/008_spi_rx_cmd_arduino.su ./Src/009_spi_message_rcv_it.cyclo ./Src/009_spi_message_rcv_it.d ./Src/009_spi_message_rcv_it.o ./Src/009_spi_message_rcv_it.su ./Src/010_i2c_master_tx_testing.cyclo ./Src/010_i2c_master_tx_testing.d ./Src/010_i2c_master_tx_testing.o ./Src/010_i2c_master_tx_testing.su ./Src/011_i2c_master_rx_testing.cyclo ./Src/011_i2c_master_rx_testing.d ./Src/011_i2c_master_rx_testing.o ./Src/011_i2c_master_rx_testing.su ./Src/012_i2c_master_rx_testingIT.cyclo ./Src/012_i2c_master_rx_testingIT.d ./Src/012_i2c_master_rx_testingIT.o ./Src/012_i2c_master_rx_testingIT.su ./Src/013_i2c_slave_tx_string.cyclo ./Src/013_i2c_slave_tx_string.d ./Src/013_i2c_slave_tx_string.o ./Src/013_i2c_slave_tx_string.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su

.PHONY: clean-Src

