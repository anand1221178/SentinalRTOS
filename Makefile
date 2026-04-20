# --- Variables ---
CC = arm-none-eabi-gcc
CFLAGS = -c -mcpu=cortex-m4 -mthumb -std=gnu11 -DSTM32F411xE -g -O0

# Updated Includes to match your new tree structure
INCLUDES = -IInc \
           -ITasks \
           -IKernel \
           -IDrivers/Inc \
           -I/Users/anand/stm32_dev/CMSIS/Include \
           -I/Users/anand/stm32_dev/CMSIS/Device/ST/STM32F4xx/Include

# --- Build Rules ---
all: all.elf

# Rule for main.o
main.o: Src/main.c Inc/os_kernel.h Tasks/tasks.h Inc/lock.h Drivers/Inc/uart.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Rule for UART driver
uart.o: Drivers/Src/uart.c Drivers/Inc/uart.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Rule for kernel C code
os_kernel.o: Kernel/os_kernel.c Inc/os_kernel.h Inc/lock.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Rule for kernel Assembly code 
os_kernel_asm.o: Kernel/os_kernel_asm.s
	$(CC) $(CFLAGS) $< -o $@

# Rule for tasks
tasks.o: Tasks/tasks.c Tasks/tasks.h Inc/os_kernel.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Rule for startup file (Make sure you copy this to your new folder!)
stm32f411_startup.o: stm32f411_startup.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Linking everything together
# Using $^ automatically inserts all the prerequisites (the .o files)
all.elf: main.o os_kernel.o os_kernel_asm.o tasks.o stm32f411_startup.o uart.o
	$(CC) -nostartfiles -T stm32_ls.ld $^ -o $@ --specs=nano.specs --specs=nosys.specs -Wl,-Map=all.map

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