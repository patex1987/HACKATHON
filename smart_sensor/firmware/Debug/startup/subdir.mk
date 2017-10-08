################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../startup/startup_stm32l1xx_mdp.S 

OBJS += \
./startup/startup_stm32l1xx_mdp.o 

S_UPPER_DEPS += \
./startup/startup_stm32l1xx_mdp.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32L1 -DSTM32L100RCTx -DSTM32L100C_DISCO -DSTM32 -DDEBUG -DUSE_STDPERIPH_DRIVER -DSTM32L1XX_MDP -I"/home/jarin/workspace/ht1/inc" -I"/home/jarin/workspace/ht1/CMSIS/core" -I"/home/jarin/workspace/ht1/CMSIS/device" -I"/home/jarin/workspace/ht1/StdPeriph_Driver/inc" -I"/home/jarin/workspace/ht1/Utilities" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


