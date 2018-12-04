################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/caminit.cc ../src/camcfg.cc ../src/ov5640.cc ../src/imx219.cc

CC_DEPS += \
./src/caminit.d ./src/camcfg.d ./src/ov5640.d ./src/imx219.d 

OBJS += \
./src/caminit.o ./src/camcfg.o ./src/ov5640.o ./src/imx219.o 

MAIN_DEPS += \
./src/main.d 

MAIN_OBJS += \
./src/main.o 

BSP_DIR = ../../../../ultra96_design/ultra96_design.sdk/standalone_bsp_0/psu_cortexa53_0/

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	aarch64-linux-gnu-g++ -Wall -O0 -g3 -std=c++11 -c -fmessage-length=0 -fpermissive -MT"$@" -I${BSP_DIR}/include -I ../src/ -g -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
