.NOTPARALLEL:

ifndef STM_CUBEF7_HOME
$(error STM_CUBEF7_HOME is not set)
endif

ifndef module
module = ex01
endif

CC = arm-none-eabi-gcc
LD = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

USER_SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build/$(module)
BIN_DIR = bin/$(module)

TARGET_NAME = app

TARGET_ELF = $(BIN_DIR)/$(TARGET_NAME).elf
TARGET_BIN = $(BIN_DIR)/$(TARGET_NAME).bin
TARGET_SIZE = $(BIN_DIR)/$(TARGET_NAME).siz

STM_SRC_ROOT = $(STM_CUBEF7_HOME)/Drivers
CMSIS_DIR = $(STM_SRC_ROOT)/CMSIS
DEVICE_DIR = $(CMSIS_DIR)/Device/ST/STM32F7xx
HAL_DIR = $(STM_SRC_ROOT)/STM32F7xx_HAL_Driver
BSP_DIR = $(STM_SRC_ROOT)/BSP/STM32746G-Discovery
LL_DIR = $(STM_SRC_ROOT)/BSP/Components

ifndef src_index
	src_index = sources.txt
endif

SRC_FILTER = $(shell grep -v '\#' $(src_index))

CMSIS_INCLUDES = -I$(CMSIS_DIR)/Include -I$(DEVICE_DIR)/Include
CMSIS_SRC = $(filter $(SRC_FILTER), $(wildcard $(DEVICE_DIR)/Source/Templates/*.c))

HAL_INCLUDES = -I$(HAL_DIR)/Inc
HAL_SRC = $(filter $(SRC_FILTER), $(wildcard $(HAL_DIR)/Src/*.c))

BSP_INCLUDES = -I$(BSP_DIR)
BSP_SRC = $(filter $(SRC_FILTER), $(wildcard $(BSP_DIR)/*.c))

LL_SRC = $(addprefix $(LL_DIR),/ft5336/ft5336.c /wm8994/wm8994.c)

SYS_INCLUDES = $(CMSIS_INCLUDES) $(HAL_INCLUDES) $(BSP_INCLUDES)
SYS_SRC = $(CMSIS_SRC) $(HAL_SRC) $(BSP_SRC) $(LL_SRC)

USER_SRC = $(wildcard $(USER_SRC_DIR)/common/*.c) $(wildcard $(USER_SRC_DIR)/$(module)/*.c)
USER_INCLUDES += -I$(USER_SRC_DIR) -Iext

ALL_INCLUDES = $(USER_INCLUDES) $(SYS_INCLUDES)
ALL_SRC = $(SYS_SRC) $(USER_SRC)

ASM_SRC = $(DEVICE_DIR)/Source/Templates/gcc/startup_stm32f746xx.s
ASM_OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRC:.s=.o)))

C_OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(ALL_SRC:.c=.o)))

DEPS = $(ASM_OBJ:.o=.d) $(C_OBJ:.o=.d)

DEFINES += -DSTM32F746xx

LIBS = -lm

CFLAGS += -std=c11 -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -funsafe-math-optimizations -fsigned-char -ffunction-sections -fdata-sections -Wall

LD_FLAGS += -L$(LIB_DIR) -Tldscripts/STM32F746NGHx_FLASH.ld
LD_FLAGS += -Xlinker --gc-sections -Wl,-Map,$(BIN_DIR)/$(TARGET_NAME).map
LD_FLAGS += -specs=nosys.specs -specs=nano.specs

$(info Building module: $(module))

# Used to enable rules below, allows building from given source list
# http://stackoverflow.com/a/24967712/294515
.SECONDEXPANSION:
PERCENT = %

all: $(BUILD_DIR) $(BIN_DIR) $(TARGET_ELF) $(TARGET_BIN) $(TARGET_SIZE)
	@:

$(BUILD_DIR):
	@echo Creating $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@echo Creating $(BIN_DIR)
	@mkdir -p $(BIN_DIR)

$(TARGET_ELF): $(C_OBJ) $(ASM_OBJ)
	$(CC) $(CFLAGS) $(LD_FLAGS) -o $@ $^ $(LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	@echo Creating flash image: $@
	@$(OBJCOPY) -O binary $< $@

$(TARGET_SIZE): $(TARGET_ELF)
	@$(SIZE) --format=berkeley -x --totals $(TARGET_ELF)

$(C_OBJ): %.o : $$(filter $$(PERCENT)/$$(notdir %).c, $(ALL_SRC))
	@echo compiling: $(notdir $<)
	@$(CC) $(DEFINES) $(ALL_INCLUDES) $(CFLAGS) -o $@ -c $< -MMD -MP -MF "$(@:%.o=%.d)"

$(ASM_OBJ): %.o : $$(filter $$(PERCENT)/$$(notdir %).s, $(ASM_SRC))
	@echo assembling: $(notdir $<)
	@$(CC) $(DEFINES) $(ALL_INCLUDES) $(CFLAGS) -o $@ -c $< -MMD -MP -MF "$(@:%.o=%.d)"

-include $(DEPS)

.PHONY: clean trace

clean:
	@echo Cleaning...
	rm -rf $(BUILD_DIR) $(BIN_DIR)

trace:
	@echo Using source index: $(src_index)
	@echo SRC_FILTER: $(SRC_FILTER)
	@echo BSP: $(BSP_SRC)
	@echo HAL: $(HAL_SRC)
	@echo ALL_INCL: $(ALL_INCLUDES)
	@echo ALL_SRC: $(ALL_SRC)
	@echo OBJ: $(C_OBJ)
	@echo DEPS: $(DEPS)
