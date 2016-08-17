$(info configuring for STM32F746)

DEVICE_DIR = $(CMSIS_DIR)/Device/ST/STM32F7xx
HAL_DIR = $(STM_SRC_ROOT)/STM32F7xx_HAL_Driver
BSP_DIR = $(STM_SRC_ROOT)/BSP/STM32746G-Discovery

DEFINES += -DSTM32F746xx

CFLAGS += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16
LD_FLAGS += -Tstm32f746.ld

ASM_SRC += $(DEVICE_DIR)/Source/Templates/gcc/startup_stm32f746xx.s

# always include audio & touchscreen drivers
LL_SRC = $(addprefix $(LL_DIR),/ft5336/ft5336.c /wm8994/wm8994.c)
