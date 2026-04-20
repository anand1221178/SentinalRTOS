# --- Variables ---
CC = arm-none-eabi-gcc
MACH = cortex-m4
CFLAGS = -c -mcpu=$(MACH) -mthumb -std=gnu11 -DSTM32F411xE -g -O0
LDFLAGS = -mcpu=$(MACH) -mthumb -nostartfiles -T stm32_ls.ld --specs=nano.specs --specs=nosys.specs -Wl,-Map=all.map

# Updated Includes
INCLUDES = -IInc \
           -ITasks \
           -IKernel \
           -IDrivers/Inc \
           -I/Users/anand/stm32_dev/CMSIS/Include \
           -I/Users/anand/stm32_dev/CMSIS/Device/ST/STM32F4xx/Include

# --- Build Rules ---
all: all.elf

main.o: Src/main.c Inc/os_kernel.h Tasks/tasks.h Inc/lock.h Drivers/Inc/uart.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

uart.o: Drivers/Src/uart.c Drivers/Inc/uart.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

os_kernel.o: Kernel/os_kernel.c Inc/os_kernel.h Inc/lock.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

os_tests.o: Kernel/os_tests.c Inc/os_kernel.h Drivers/Inc/uart.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

os_kernel_asm.o: Kernel/os_kernel_asm.s
	$(CC) $(CFLAGS) $< -o $@

tasks.o: Tasks/tasks.c Tasks/tasks.h Inc/os_kernel.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

stm32f411_startup.o: stm32f411_startup.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

all.elf: main.o os_kernel.o os_kernel_asm.o os_tests.o tasks.o stm32f411_startup.o uart.o
	$(CC) $(LDFLAGS) $^ -o $@

flash:
	arm-none-eabi-gdb -batch \
	-ex "target remote localhost:3333" \
	-ex "monitor reset init" \
	-ex "monitor flash write_image erase all.elf" \
	-ex "monitor reset halt" \
	-ex "monitor resume"

load:
	openocd -f board/st_nucleo_f4.cfg

clean:
	rm -f *.o *.elf *.map
