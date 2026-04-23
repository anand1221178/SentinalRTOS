#include "servo.h"
#include "os_kernel.h"
#include "uart.h"
#include "os_queue.h"

extern os_message_queue_t sweep_queue;

void servo_init(void)
{
    /* Enable clock acess to gipia  and TIM3*/
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* FOnciggure PA6 for alternate function (AF2 = TIM3_CH1) so set bits 27-24 to 0010*/
    /* THis sets PA6 to AF mode */
    GPIOA->MODER |= (1U<<13); /* Set 13 to 1 */ 
    GPIOA->MODER &=~(1U<<12); /* Set 12 to 0 */
    /* Set the AF mode of PA6 to 0010 for TIM3 */
    GPIOA->AFR[0] &=~(1U<<27);
    GPIOA->AFR[0] &=~(1U<<26);
    GPIOA->AFR[0] |=(1U<<25);
    GPIOA->AFR[0] &=~(1U<<24);

    /* configure the TIM3 timer (1 tick = 1 us, period = 20ms) */
    TIM3->PSC = 15; /* 16mhz -> 15 + 1 */
    TIM3->ARR = 19999; /* 20000us 50hz */

    /* Configure TIM3 Channel 1 for PWM mode 1 */
    /* The Output Compare Mode (OC1M) is controlled by bits 6, 5, and 4 in CCMR1 PWM Mode 1 is defined as binary '110'. */    
    TIM3->CCMR1 |=  (1U << 6); /* Set bit 6 to 1 */
    TIM3->CCMR1 |=  (1U << 5); /* Set bit 5 to 1 */
    TIM3->CCMR1 &= ~(1U << 4); /* Set bit 4 to 0 */

    /* Enable the Output Compare 1 Preload (OC1PE). 
       This is bit 3 in CCMR1. */
    TIM3->CCMR1 |=  (1U << 3); /* Set bit 3 to 1 */

    /* Enable the Output on Channel 1 */
    /* The Capture/Compare 1 Output Enable (CC1E) is bit 0 in CCER. */
    TIM3->CCER |=   (1U << 0); /* Set bit 0 to 1 */

    /* Start with servo at 90 degrees (1.5ms pulse) */
    /* The Capture/Compare Register 1 (CCR1) holds the actual counter value.
       We don't shift bits here; we just give it the raw integer value (1500 ticks = 1500 us). */
    TIM3->CCR1 = 1500;

    /* Enable Timer 3 Counter */
    /* The Counter Enable (CEN) is bit 0 in the Control Register 1 (CR1). */
    TIM3->CR1 |=    (1U << 0); /* Set bit 0 to 1 */

    
}

/* Since i have never worked with the SG90 servo motor this is what the big GEMINI had to say:
    The motor cannot track degrees and only knows for how long the power is on
    According to the spec sheet for the motor:
    - If we turn on the power for the motor for 1 ms, the motor moves all the way to the left (0 degrees)
    - If we turn on the power for the motor for 1.5ms, the motor moves to the center (90 degrees)
    - If we turn on the power for the motor for 2ms, the motor moves all the way to the right (180 degrees)

    TIM3->CCR1 = 1500: CCR1 is like the power off switch.
    When the cycle starts at 0, the STM32 turns the power pin HIGH. The timer starts counting: 1, 2, 3...
    When the timer hits the number stored in CCR1 (which is 1500), the hardware automatically flips the power pin LOW. 
    The timer keeps counting in the dark until it hits 20,000, and then the whole thing repeats.

    By writing TIM3->CCR1 = 1500;, you are literally telling the hardware: "Keep the pin ON for 1500 ticks, then turn it OFF." 
    Because our ticks are 1 microsecond each, that sends the exact 1.5ms pulse the motor needs to find the 90-degree center point.
*/

void servo_set_angle(uint16_t degrees) 
{
    /* Clamp the input*/
    if (degrees > 180) {
        degrees = 180;
    }

    /* * Map 0-180 degrees to 1000-2000 microseconds:
     * Pulse Width = 1000 + ( (degrees * 1000) / 180 )
     */
    uint32_t pulse_width_us = 1000 + ((degrees * 1000) / 180);

    /* Update the hardware register to change the duty cycle immediately */
    TIM3->CCR1 = pulse_width_us;
}

void sweep_task(void)
{
    /* Homing phase to get the servo motor to a known angle */
    servo_set_angle(0);
    os_delay(500);

    int16_t current_angle = 0;
    int16_t step = 1;

    /* Main loop -> runs forver at 50hz */
    while(1)
    {
        /* Update the hardware */
        servo_set_angle(current_angle);

        /*Broadcast the angle to the rest of the OS safely */
        os_queue_send(&sweep_queue, (uint32_t)current_angle);

        /* Print every 10 degrees to verify timing and logic */
        if (current_angle % 10 == 0) {
            uart_print("[Servo] Angle: ");
            uart_print_number(current_angle);
            uart_print(" | Step: ");
            if (step > 0) uart_print("+1\r\n");
            else uart_print("-1\r\n");
        }
        
        /* Calculate the next position */
        current_angle = current_angle + step;
        
        /* Check boundaries and reverse direction if needed */
        if (current_angle >= 180 && step == +1) 
        {
            step = -1; 
        }
        else if ( current_angle <= 0 && step == -1) 
        {
            step = 1; 
        }
        
        os_delay(20);
    }
}