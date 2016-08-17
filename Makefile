.NOTPARALLEL:

CC = arm-none-eabi-gcc
LD = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy

USER_SRC_DIR = src
LIBDIR = lib
BUILD_DIR = obj
BIN_DIR = bin

ifndef STM_CUBE_HOME
$(error STM_CUBE_HOME is not set)
endif

TARGET_NAME = synth

TARGET_ELF = $(BIN_DIR)/$(TARGET_NAME).elf
TARGET_BIN = $(BIN_DIR)/$(TARGET_NAME).bin

STM_SRC_ROOT = $(STM_CUBE_HOME)/Drivers
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

SYS_INCLUDES = $(CMSIS_INCLUDES) $(HAL_INCLUDES) $(BSP_INCLUDES)
SYS_SRC = $(CMSIS_SRC) $(HAL_SRC) $(BSP_SRC)

USER_SRC = $(wildcard $(USER_SRC_DIR)/*.c)
USER_INCLUDES += -I$(USER_SRC_DIR) -Iext

ALL_INCLUDES = $(SYS_INCLUDES) $(USER_INCLUDES)
ALL_SRC = $(SYS_SRC) $(USER_SRC)

ASM_SRC = $(DEVICE_DIR)/Source/Templates/gcc/startup_stm32f746xx.s
ASM_OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRC:.s=.o)))

OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(ALL_SRC:.c=.o)))
DEPS = $(OBJECTS:.o=.d)

DEFINES += -DSTM32F746xx

LIBS = -lm -lc

CFLAGS += -std=c11 -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -funsafe-math-optimizations -fsigned-char -ffunction-sections -fdata-sections -Wall

LD_FLAGS += -T STM32F746NGHx_FLASH.ld -Xlinker --gc-sections -Wl,-Map,$(TARGET_NAME).map -specs=nosys.specs -specs=nano.specs

.SECONDEXPANSION:
PERCENT = %

all: $(BIN_DIR) $(BUILD_DIR) $(TARGET_ELF) $(TARGET_BIN)
	@:

$(BIN_DIR):
	@echo Creating $(BIN_DIR)
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@echo Creating $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)

$(TARGET_BIN): $(TARGET_ELF)
	@echo Creating flash image: $@
	$(OBJCOPY) -O binary $< $@

$(TARGET_ELF): $(OBJECTS) $(ASM_OBJ)
	$(CC) $(CFLAGS) $(LD_FLAGS) -o $@ $^ $(LIBS)

$(OBJECTS): %.o : $$(filter $$(PERCENT)/$$(notdir %).c, $(ALL_SRC))
	@echo compiling: $(notdir $<)
	@$(CC) $(DEFINES) $(ALL_INCLUDES) $(CFLAGS) -o $@ -c $<

$(ASM_OBJ): %.o : $$(filter $$(PERCENT)/$$(notdir %).s, $(ASM_SRC))
	@echo assembling: $(notdir $<)
	@$(CC) $(DEFINES) $(ALL_INCLUDES) $(CFLAGS) -o $@ -c $<

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
	@echo OBJ: $(OBJECTS)
	@echo DEPS: $(DEPS)
