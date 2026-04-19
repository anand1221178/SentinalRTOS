.syntax unified
.thumb
.text

@ External variable from our C code
.extern current_tcb

@ Make the functions visible to our C code
.global os_kernel_launch_asm
.global os_start_first_task

.thumb_func
os_kernel_launch_asm:
    @ 1. Get the current_tcb address
    LDR     R0, =current_tcb    
    LDR     R2, [R0]            @ R2 now holds the address of the TCB
    
    @ 2. Get the stackPtr from the TCB (it's the first element, so offset 0)
    LDR     SP, [R2]            
    
    @ 3. Unpack the "Suitcase" manually since we aren't in an interrupt yet
    LDMIA   SP!, {R4-R11}       @ Pop software-saved registers
    LDMIA   SP!, {R0-R3}        @ Pop part of the hardware frame
    LDMIA   SP!, {R12, LR}      @ Pop more hardware frame
    
    @ 4. The Final Jump
    LDMIA   SP!, {PC}           @ Pop the Program Counter! 
                                @ The CPU jumps to the task's function address.

.thumb_func
os_start_first_task:
    LDR     R0, =current_tcb    @ Get address of pointer
    LDR     R1, [R0]            @ Get address of actual TCB
    LDR     R0, [R1]            @ Get stackPtr (offset 0)

    @ 1. Set PSP to the task's stack pointer
    MSR     PSP, R0             

    @ 2. Switch to using PSP (Set bit 1 of CONTROL register)
    MOV     R0, #2
    MSR     CONTROL, R0
    ISB                         @ Instruction Synchronization Barrier

    @ 3. Pop the Software Frame (R4-R11) from PSP
    @ We must use R0 to point to PSP to do the popping
    MRS     R0, PSP
    LDMIA   R0!, {R4-R11}
    MSR     PSP, R0             @ Update PSP after popping

    @ 4. Set the magic return value
    LDR     LR, =0xFFFFFFFD
    
    @ 5. Global enable interrupts before we jump
    CPSIE   I
    
    @ 6. Return! Hardware handles popping R0-R3, R12, LR, PC, xPSR
    BX      LR
