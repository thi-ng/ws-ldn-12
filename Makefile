ifndef STM_CUBE_HOME
$(error STM_CUBE_HOME is not set)
endif

STM_SRC_ROOT = $(STM_CUBE_HOME)/Drivers
CMSIS_DIR = $(STM_SRC_ROOT)/CMSIS
LL_DIR = $(STM_SRC_ROOT)/BSP/Components
USBH_DIR = $(STM_CUBE_HOME)/Middlewares/ST/STM32_USB_Host_Library
FATFS_DIR = $(STM_CUBE_HOME)/Middlewares/Third_Party/FatFs/src

ifndef device
device = stm32f746
endif

ifndef module
module = ex01
endif

USER_SRC_DIR = src
EXT_SRC_DIR = ext
LIB_DIR = lib
BUILD_DIR = build/$(module)
BIN_DIR = bin/$(module)

include make/devices/$(device).make

$(info building module: $(module))
include make/modules/$(module).make

ifndef catalog
	catalog = make/sources.txt
endif

$(info using source catalog: $(catalog))
SRC_FILTER = $(shell grep -v '\#' $(catalog))

CC = arm-none-eabi-gcc
LD = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

ifndef TARGET_NAME
TARGET_NAME = app
endif

TARGET_ELF = $(BIN_DIR)/$(TARGET_NAME).elf
TARGET_BIN = $(BIN_DIR)/$(TARGET_NAME).bin
TARGET_SIZE = $(BIN_DIR)/$(TARGET_NAME).siz

CMSIS_INCLUDES = -I$(CMSIS_DIR)/Include -I$(DEVICE_DIR)/Include
CMSIS_SRC = $(filter $(SRC_FILTER), $(wildcard $(DEVICE_DIR)/Source/Templates/*.c))

HAL_INCLUDES = -I$(HAL_DIR)/Inc
HAL_SRC = $(filter $(SRC_FILTER), $(wildcard $(HAL_DIR)/Src/*.c))

BSP_INCLUDES = -I$(BSP_DIR)
BSP_SRC = $(filter $(SRC_FILTER), $(wildcard $(BSP_DIR)/*.c))

ifeq ($(USE_USBH),1)
$(info including USBH...)
USBH_INCLUDES += -I$(USBH_DIR)/Core/Inc -I$(USBH_DIR)/Class/AUDIO/Inc -I$(USBH_DIR)/Class/MSC/Inc
USBH_SRC += $(filter $(SRC_FILTER), $(wildcard $(USBH_DIR)/Core/Src/*.c))
USBH_SRC += $(wildcard $(USBH_DIR)/Class/AUDIO/Src/*.c)
USBH_SRC += $(wildcard $(USBH_DIR)/Class/MSC/Src/*.c)
endif

ifeq ($(USE_FATFS),1)
$(info including FatFS...)
FATFS_INCLUDES += -I$(FATFS_DIR) -I$(FATFS_DIR)/drivers
FATFS_SRC += $(wildcard $(FATFS_DIR)/*.c)
FATFS_SRC += $(filter $(SRC_FILTER), $(wildcard $(FATFS_DIR)/drivers/*.c))
FATFS_SRC += $(filter $(SRC_FILTER), $(wildcard $(FATFS_DIR)/option/*.c))
endif

SYS_INCLUDES = $(CMSIS_INCLUDES) $(HAL_INCLUDES) $(BSP_INCLUDES) $(USBH_INCLUDES) $(FATFS_INCLUDES)
SYS_SRC = $(CMSIS_SRC) $(HAL_SRC) $(BSP_SRC) $(LL_SRC) $(USBH_SRC) $(FATFS_SRC)

USER_INCLUDES += -I$(USER_SRC_DIR) -Iext
USER_SRC += $(wildcard $(USER_SRC_DIR)/common/*.c) $(wildcard $(USER_SRC_DIR)/$(module)/*.c)

ALL_INCLUDES = $(USER_INCLUDES) $(SYS_INCLUDES)
ALL_SRC = $(SYS_SRC) $(USER_SRC)

ASM_OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRC:.s=.o)))

C_OBJ += $(addprefix $(BUILD_DIR)/, $(notdir $(ALL_SRC:.c=.o)))

DEPS += $(ASM_OBJ:.o=.d) $(C_OBJ:.o=.d)

LIBS += -lm

CFLAGS += -std=c11 -fsigned-char -ffunction-sections -fdata-sections -Wall

LD_FLAGS += -Lmake/devices -L$(LIB_DIR)
LD_FLAGS += -Xlinker --gc-sections -Wl,-Map,$(BIN_DIR)/$(TARGET_NAME).map
LD_FLAGS += -specs=nosys.specs -specs=nano.specs

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
	@echo Linking target: $(TARGET_ELF)
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
	@echo SRC_FILTER: $(SRC_FILTER)
	@echo BSP: $(BSP_SRC)
	@echo HAL: $(HAL_SRC)
	@echo ALL_INCL: $(ALL_INCLUDES)
	@echo ALL_SRC: $(ALL_SRC) $(ASM_SRC)
	@echo OBJ: $(C_OBJ) $(ASM_OBJ)
	@echo DEPS: $(DEPS)
