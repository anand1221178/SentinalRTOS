#include "tasks.h"
#include "uart.h"
#include "servo.h"
#include "os_queue.h"

extern os_message_queue_t sweep_queue;
extern os_semaphore_t echo_ready;
extern volatile uint32_t time_start;
extern volatile uint32_t time_end;

/* Forward declares */
void hardware_delay_us(uint32_t us);

void os_idle_task(void)
{
    while(1)
    {
        /* Temporarily disabling WFI to keep debugger alive during power issues */
        // __asm volatile ("wfi");
        __NOP();
    }
}

void sweep_task(void)
{
    /* Gradual Homing to prevent current spikes/brownouts */
    uart_print("[Servo] Gradual homing sequence...\r\n");
    for(int i = 90; i >= 0; i -= 5) {
        servo_set_angle(i);
        os_delay(50);
    }
    os_delay(500);

    int16_t current_angle = 0;
    int16_t step = 5; 

    /* Main loop */
    while(1)
    {
        servo_set_angle(current_angle);

        os_queue_send(&sweep_queue, (uint32_t)current_angle, WAIT_FOREVER);
        
        uart_print("[Servo] Angle: ");
        uart_print_number((uint32_t)current_angle);
        uart_print("\r\n");

        current_angle = current_angle + step;
        
        if (current_angle >= 180) 
        {
            current_angle = 180;
            step = -5; 
        }
        else if (current_angle <= 0) 
        {
            current_angle = 0;
            step = 5; 
        }
        
        /* USed to set rate at 50hz */
        os_delay(20);
    }
}

void hardware_delay_us(uint32_t us)
{
    uint32_t iterations = us * 4; 
    for (uint32_t i = 0; i < iterations; i++)
    {
        __NOP(); 
    }
}

void radar_task(void)
{
    uint32_t received_angle = 0;
    uint32_t distance = 0;

    while(1)
    {
        /* We are sending the pulse here:
        - We send a 10us HIGH pulse on the trigger pin
        - The sensor fires 8 pulses at 40khz
        - echo pin stays high until the pulse comes back
        - When the bounce comes back the pin goes low
        need to measure that width — the time between the rising edge (echo goes HIGH) and the falling edge (echo goes LOW)
        */
        GPIOA->ODR |= (1U << TRIG_PIN);  
        hardware_delay_us(10);           
        GPIOA->ODR &= ~(1U << TRIG_PIN); 
        
        if (os_semaphore_acquire(&echo_ready, 100) == OS_SUCCESS)
        {
            distance = (time_end - time_start) / 58;
            
            if (os_queue_receive(&sweep_queue, &received_angle, 0) == OS_SUCCESS)
            {
                uart_print("Angle: ");
                uart_print_number(received_angle);
                uart_print(" | Dist: ");
                uart_print_number(distance);
                uart_print(" cm\r\n");
            }
        }
        
        os_delay(100); 
    }

    /* TO measure the echo pulse width:
    - We configure a timer to count at 1mhz
    - WE conncet the echo pin to a timer channel
    - Hardware auto snapshots the counter value when it sees an edge on the pin
    - Rising edge -> hardware saves counter value into TIM2->CCR1 = time_start
    - Falling edge -> hardware saves counter value into TIM2->CCR1 = time_end
    - Each edge causes an interrupt and ISR needs to read the caputered value
    */
}
