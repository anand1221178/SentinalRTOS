; External variable from our C code
.extern current_tcb

; Make the function visible to our C code
.global os_kernel_launch

os_kernel_launch:
    ; 1. Get the current_tcb address
    LDR     R0, =current_tcb    
    LDR     R2, [R0]            ; R2 now holds the address of the TCB
    
    ; 2. Get the stackPtr from the TCB (it's the first element, so offset 0)
    LDR     SP, [R2]            
    
    ; 3. Unpack the "Suitcase" manually since we aren't in an interrupt yet
    LDMIA   SP!, {R4-R11}       ; Pop software-saved registers
    LDMIA   SP!, {R0-R3}        ; Pop part of the hardware frame
    LDMIA   SP!, {R12, LR}      ; Pop more hardware frame
    
    ; 4. The Final Jump
    LDMIA   SP!, {PC}           ; Pop the Program Counter! 
                                ; The CPU jumps to the task's function address.