################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: test.elf

# Tool invocations
libcamctrl.so: $(OBJS)
	aarch64-linux-gnu-gcc -o $@ -shared $(OBJS) $(USER_OBJS) -L ${BSP_DIR}/lib/ -Wl,-no-enum-size-warning,-lc,-lxil

test.elf: $(TEST_OBJS)  $(USER_OBJS) libcamctrl.so
	@echo 'Building target: $@'
	@echo 'Invoking: ARM v7 gcc linker'
	aarch64-linux-gnu-gcc -o $@ $(TEST_OBJS) $(USER_OBJS) $(LIBS) -L . -Wl,-no-enum-size-warning
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(S_UPPER_DEPS)$(CC_DEPS)$(TEST_OBJS)$(TEST_DEPS) libcamctrl.so test.elf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
