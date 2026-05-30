EXEC     = app.elf
LINKER   = stm32f4.ld
CPU      = cortex-m4
ARCH     = armv7e-m
FPU      = fpv4-sp-d16
SPECS    = nosys.specs

CC       = arm-none-eabi-gcc

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) \
            -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

CFLAGS   = -g -O0 -std=c99 -Wall -Werror \
           -D__FPU_PRESENT=1 -D__FPU_USED=1 \
           $(ARCHFLAGS)

INCLUDES = -ICore/Inc -ICMSIS

SRCS     = startup.c \
           Core/Src/main.c \
           Core/Src/clock.c \
           Core/Src/gpio.c \
           Core/Src/uart.c \
           Core/Src/adc.c \
           Core/Src/i2c.c \
           Core/Src/oled.c

OBJS     = $(SRCS:.c=.o)
LDFLAGS  = -nostdlib -T $(LINKER)

%.o : %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

.PHONY: all
all: $(EXEC)

.PHONY: build
build: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@

.PHONY: flash
flash:
	openocd -f board/st_nucleo_f4.cfg -c "program $(EXEC) verify reset" -c shutdown

.PHONY: clean
clean:
	rm -f $(OBJS) $(EXEC)
