################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.c 

OBJS += \
./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.o 

C_DEPS += \
./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/%.o Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/%.su Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/%.cyclo: ../Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/%.c Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"D:/STM32_CubeIDE_workspaces/SN_Keyboard_assistant/Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Class-2f-MIDI-2f-Src

clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Class-2f-MIDI-2f-Src:
	-$(RM) ./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.cyclo ./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.d ./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.o ./Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Class-2f-MIDI-2f-Src

