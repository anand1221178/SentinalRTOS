.syntax unified
.thumb
.text

@ External symbols
.extern current_tcb
.extern os_scheduler

@ Global symbols
.global os_kernel_launch_asm
.global os_start_first_task
.global PendSV_Handler

.thumb_func
os_kernel_launch_asm:
    LDR     R0, =current_tcb    
    LDR     R2, [R0]            
    LDR     SP, [R2]            
    LDMIA   SP!, {R4-R11}       
    LDMIA   SP!, {R0-R3}        
    LDMIA   SP!, {R12, LR}      
    LDMIA   SP!, {PC}           

.thumb_func
PendSV_Handler:
    @ --- 1. SAVE CONTEXT ---
    MRS     R0, PSP             @ Get current task's stack pointer
    STMDB   R0!, {R4-R11}       @ Push registers R4-R11 onto task stack
    
    @ Store the updated SP back into the TCB
    LDR     R1, =current_tcb    
    LDR     R1, [R1]            @ R1 = address of current_tcb
    STR     R0, [R1]            @ current_tcb->stackPtr = R0

    @ --- 2. SELECT NEXT TASK ---
    PUSH    {LR}                @ Save EXC_RETURN
    BL      os_scheduler        @ Call the C scheduler
    POP     {LR}                @ Restore EXC_RETURN

    @ --- 3. RESTORE CONTEXT ---
    @ Load the new task's stack pointer
    LDR     R1, =current_tcb    
    LDR     R1, [R1]            @ R1 = address of new current_tcb
    LDR     R0, [R1]            @ R0 = new_tcb->stackPtr

    @ Pop registers R4-R11 from the new task's stack
    LDMIA   R0!, {R4-R11}       
    MSR     PSP, R0             @ Update PSP with new stack pointer

    BX      LR                  @ Return to new task

.thumb_func
os_start_first_task:
    @ 1. Get current_tcb->stackPtr
    LDR     R0, =current_tcb    
    LDR     R1, [R0]            
    LDR     R0, [R1]            @ R0 = stackPtr (points to R4)

    @ 2. Pop the software frame (R4-R11) manually from R0
    LDMIA   R0!, {R4-R11}

    @ 3. Pop the hardware frame (R0-R3, R12, LR, PC, xPSR)
    @ We need the PC to jump, and the rest to clear the stack
    LDMIA   R0!, {R1-R3}        @ R1=R0, R2=R1, R3=R2 (dummy)
    LDMIA   R0!, {R2}           @ R2=R3 (dummy)
    LDMIA   R0!, {R3}           @ R3=R12 (dummy)
    LDMIA   R0!, {R12}          @ R12=LR (dummy)
    LDMIA   R0!, {R1}           @ R1 = PC (Actual task function)
    @ xPSR is left on stack, but we'll set PSP to the final R0 value
    ADD     R0, R0, #4          @ Skip xPSR

    @ 4. Set PSP to the cleaned up stack pointer
    MSR     PSP, R0             

    @ 5. Switch to using PSP (Set bit 1 of CONTROL register)
    MOV     R2, #2
    MSR     CONTROL, R2
    ISB                         @ Instruction Synchronization Barrier

    @ 6. Global enable interrupts
    CPSIE   I
    
    @ 7. Jump to the task!
    BX      R1
