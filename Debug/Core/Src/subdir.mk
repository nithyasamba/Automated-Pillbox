################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/User_Elena.c \
../Core/Src/User_George.c \
../Core/Src/User_Kevin.c \
../Core/Src/User_Nithya.c \
../Core/Src/fingerprint.c \
../Core/Src/fingerprint1.c \
../Core/Src/fingerprint2.c \
../Core/Src/fingerprint3.c \
../Core/Src/fingerprint4.c \
../Core/Src/gpio.c \
../Core/Src/happyturtle.c \
../Core/Src/ili9488.c \
../Core/Src/ili9488_gfx.c \
../Core/Src/main.c \
../Core/Src/menu.c \
../Core/Src/newmenu.c \
../Core/Src/pond_bg.c \
../Core/Src/sadturtle.c \
../Core/Src/sd_benchmark.c \
../Core/Src/sd_diskio_spi.c \
../Core/Src/sd_functions.c \
../Core/Src/sd_spi.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/turtle.c \
../Core/Src/user_manasa.c \
../Core/Src/xpt2046.c 

OBJS += \
./Core/Src/User_Elena.o \
./Core/Src/User_George.o \
./Core/Src/User_Kevin.o \
./Core/Src/User_Nithya.o \
./Core/Src/fingerprint.o \
./Core/Src/fingerprint1.o \
./Core/Src/fingerprint2.o \
./Core/Src/fingerprint3.o \
./Core/Src/fingerprint4.o \
./Core/Src/gpio.o \
./Core/Src/happyturtle.o \
./Core/Src/ili9488.o \
./Core/Src/ili9488_gfx.o \
./Core/Src/main.o \
./Core/Src/menu.o \
./Core/Src/newmenu.o \
./Core/Src/pond_bg.o \
./Core/Src/sadturtle.o \
./Core/Src/sd_benchmark.o \
./Core/Src/sd_diskio_spi.o \
./Core/Src/sd_functions.o \
./Core/Src/sd_spi.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/turtle.o \
./Core/Src/user_manasa.o \
./Core/Src/xpt2046.o 

C_DEPS += \
./Core/Src/User_Elena.d \
./Core/Src/User_George.d \
./Core/Src/User_Kevin.d \
./Core/Src/User_Nithya.d \
./Core/Src/fingerprint.d \
./Core/Src/fingerprint1.d \
./Core/Src/fingerprint2.d \
./Core/Src/fingerprint3.d \
./Core/Src/fingerprint4.d \
./Core/Src/gpio.d \
./Core/Src/happyturtle.d \
./Core/Src/ili9488.d \
./Core/Src/ili9488_gfx.d \
./Core/Src/main.d \
./Core/Src/menu.d \
./Core/Src/newmenu.d \
./Core/Src/pond_bg.d \
./Core/Src/sadturtle.d \
./Core/Src/sd_benchmark.d \
./Core/Src/sd_diskio_spi.d \
./Core/Src/sd_functions.d \
./Core/Src/sd_spi.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/turtle.d \
./Core/Src/user_manasa.d \
./Core/Src/xpt2046.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L4R5xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/User_Elena.cyclo ./Core/Src/User_Elena.d ./Core/Src/User_Elena.o ./Core/Src/User_Elena.su ./Core/Src/User_George.cyclo ./Core/Src/User_George.d ./Core/Src/User_George.o ./Core/Src/User_George.su ./Core/Src/User_Kevin.cyclo ./Core/Src/User_Kevin.d ./Core/Src/User_Kevin.o ./Core/Src/User_Kevin.su ./Core/Src/User_Nithya.cyclo ./Core/Src/User_Nithya.d ./Core/Src/User_Nithya.o ./Core/Src/User_Nithya.su ./Core/Src/fingerprint.cyclo ./Core/Src/fingerprint.d ./Core/Src/fingerprint.o ./Core/Src/fingerprint.su ./Core/Src/fingerprint1.cyclo ./Core/Src/fingerprint1.d ./Core/Src/fingerprint1.o ./Core/Src/fingerprint1.su ./Core/Src/fingerprint2.cyclo ./Core/Src/fingerprint2.d ./Core/Src/fingerprint2.o ./Core/Src/fingerprint2.su ./Core/Src/fingerprint3.cyclo ./Core/Src/fingerprint3.d ./Core/Src/fingerprint3.o ./Core/Src/fingerprint3.su ./Core/Src/fingerprint4.cyclo ./Core/Src/fingerprint4.d ./Core/Src/fingerprint4.o ./Core/Src/fingerprint4.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/happyturtle.cyclo ./Core/Src/happyturtle.d ./Core/Src/happyturtle.o ./Core/Src/happyturtle.su ./Core/Src/ili9488.cyclo ./Core/Src/ili9488.d ./Core/Src/ili9488.o ./Core/Src/ili9488.su ./Core/Src/ili9488_gfx.cyclo ./Core/Src/ili9488_gfx.d ./Core/Src/ili9488_gfx.o ./Core/Src/ili9488_gfx.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/menu.cyclo ./Core/Src/menu.d ./Core/Src/menu.o ./Core/Src/menu.su ./Core/Src/newmenu.cyclo ./Core/Src/newmenu.d ./Core/Src/newmenu.o ./Core/Src/newmenu.su ./Core/Src/pond_bg.cyclo ./Core/Src/pond_bg.d ./Core/Src/pond_bg.o ./Core/Src/pond_bg.su ./Core/Src/sadturtle.cyclo ./Core/Src/sadturtle.d ./Core/Src/sadturtle.o ./Core/Src/sadturtle.su ./Core/Src/sd_benchmark.cyclo ./Core/Src/sd_benchmark.d ./Core/Src/sd_benchmark.o ./Core/Src/sd_benchmark.su ./Core/Src/sd_diskio_spi.cyclo ./Core/Src/sd_diskio_spi.d ./Core/Src/sd_diskio_spi.o ./Core/Src/sd_diskio_spi.su ./Core/Src/sd_functions.cyclo ./Core/Src/sd_functions.d ./Core/Src/sd_functions.o ./Core/Src/sd_functions.su ./Core/Src/sd_spi.cyclo ./Core/Src/sd_spi.d ./Core/Src/sd_spi.o ./Core/Src/sd_spi.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/turtle.cyclo ./Core/Src/turtle.d ./Core/Src/turtle.o ./Core/Src/turtle.su ./Core/Src/user_manasa.cyclo ./Core/Src/user_manasa.d ./Core/Src/user_manasa.o ./Core/Src/user_manasa.su ./Core/Src/xpt2046.cyclo ./Core/Src/xpt2046.d ./Core/Src/xpt2046.o ./Core/Src/xpt2046.su

.PHONY: clean-Core-2f-Src

