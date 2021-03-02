################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/background.c \
../src/game.c \
../src/lcd.c \
../src/square.c \
../src/square2.c \
../src/syscalls.c \
../src/system_stm32f0xx.c 

OBJS += \
./src/background.o \
./src/game.o \
./src/lcd.o \
./src/square.o \
./src/square2.o \
./src/syscalls.o \
./src/system_stm32f0xx.o 

C_DEPS += \
./src/background.d \
./src/game.d \
./src/lcd.d \
./src/square.d \
./src/square2.d \
./src/syscalls.d \
./src/system_stm32f0xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F091RCTx -DDEBUG -DSTM32F091 -DUSE_STDPERIPH_DRIVER -I"D:/Programs/362/workspace/PROJECT/StdPeriph_Driver/inc" -I"D:/Programs/362/workspace/PROJECT/inc" -I"D:/Programs/362/workspace/PROJECT/CMSIS/device" -I"D:/Programs/362/workspace/PROJECT/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


