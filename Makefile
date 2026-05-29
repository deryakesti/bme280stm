##############################################################
# STM32F407VGT6 — BME280 CMSIS-only build
##############################################################

TARGET  = bme280stm
BUILD   = build

TC      = arm-none-eabi-
CC      = $(TC)gcc
AS      = $(TC)gcc -x assembler-with-cpp
CP      = $(TC)objcopy
SZ      = $(TC)size

CPU     = -mcpu=cortex-m4
FPU     = -mfpu=fpv4-sp-d16
FLOAT   = -mfloat-abi=hard
ARCH    = $(CPU) $(FPU) $(FLOAT) -mthumb

DEFS    = -DSTM32F407xx -DUSE_FULL_LL_DRIVER

INCS    = -IInc \
          -ICMSIS/Include \
          -ICMSIS/Device/ST/STM32F4xx/Include

CFLAGS  = $(ARCH) $(DEFS) $(INCS) \
          -Wall -Wextra -O2 -g3 \
          -ffunction-sections -fdata-sections \
          -fno-common --specs=nano.specs \
          -u _printf_float

ASFLAGS = $(ARCH) $(DEFS) $(INCS)

LDFLAGS = $(ARCH) \
          -TLD/stm32f407vgtx.ld \
          --specs=nano.specs \
          -u _printf_float \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD)/$(TARGET).map,--cref

C_SRCS  = Src/main.c \
          Src/i2c.c \
          Src/bme280.c \
          Src/usart.c \
          Src/system_stm32f4xx.c

AS_SRCS = Startup/startup_stm32f407vgtx.s

C_OBJS  = $(addprefix $(BUILD)/,$(C_SRCS:.c=.o))
AS_OBJS = $(addprefix $(BUILD)/,$(AS_SRCS:.s=.o))
OBJS    = $(C_OBJS) $(AS_OBJS)

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin
	$(SZ) $<

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(CP) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(CP) -O binary -S $< $@

$(BUILD)/Src/%.o: Src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/Startup/%.o: Startup/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)

flash: $(BUILD)/$(TARGET).bin
	st-flash write $< 0x08000000

.PHONY: all clean flash
