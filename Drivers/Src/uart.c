#include "stdint.h"
#include "uart.h"

static void uart_set_baud_rate(uint32_t periph_clk, uint32_t baudrate);
static void uart_write(int ch);
void uart_print(const char *str);

int __io_putchar(int ch)
{
    /* Check if uart is initialised */
    if (!(USART2->CR1 & CR1_UE))
    {
        return -1; /* uart not enabled */
    }

    uart_write(ch);
    return ch; /* success */
}

void uart_init(void)
{
    /* enable clock access to GPIOA */
    RCC->AHB1ENR |= GPIOAEN;

    /* Set mode of PA2 to alternate function mode */
    /* clear bit 4 */
    GPIOA->MODER &=~(1U<<4);     
    //set bt 5
    GPIOA->MODER |= (1U<<5);

    /*Set alternate function type to AF7 (UART2_TX)*/
    //we use [0] here since pin 2 is on Low Reg - which works for the pins from 0-7 if we were using high then we use [1]
    GPIOA->AFR[0] |= (1U<<8);
    GPIOA->AFR[0] |= (1U<<9);
    GPIOA->AFR[0] |= (1U<<10);
    GPIOA->AFR[0] &=~(1U<<11);

    /* enable clock access to uart2*/
    RCC->APB1ENR |= UART2EN; //bit 17

    /* configure the usart BAUD rate*/
    uart_set_baud_rate(APB1_CLK, DBG_UART_BAUDRATE);

    /*Turn on USART - enable*/
    USART2->CR1 |= CR1_UE;
    /* configure UART2 - transmitter enable*/
    USART2->CR1 |= CR1_TE;

}

static void uart_write(int ch)
{
    /*Make sure tranmist data reg is empty*/
    while(!(USART2->SR & SR_TXE)) {}; //spin while not empty

    /* Write to trnamist data reg*/
    //int ch is 32 bits and DR is only 8 bits wide -> so we use the 0XFF mask to extract only the valid data, basically onyl keeping the last 8 bits
    USART2->DR = (ch & 0xFF); 
}

static uint16_t compute_uart_baud(uint32_t periph_clk, uint32_t baud_rate)
{
    // BRR = (16 000 000 + (115 200 / 2)) / 115 200
    //Gives 139.388...
    //= 139
    //hence the actual Baud rate is 16 000 000/139 = 115 107.9

    //Error = [115200 - 115.107.9]/ 115200 = 0.0008 = 0.08%
    return ((periph_clk + (baud_rate/2U))/ baud_rate);
}

static void uart_set_baud_rate(uint32_t periph_clk, uint32_t baud_rate)
{
    USART2->BRR  = compute_uart_baud(periph_clk, baud_rate);
}

void uart_print(const char *str){
    while(*str)
    {
        __io_putchar(*str++);
    }
}

void uart_print_number(uint32_t num) {                                                           
      char buffer[10];                                                                             
      int i = 0;                                                                                   
                                                                                                   
      if (num == 0) {                                                                              
          uart_print("0");                                                                         
          return;                                                                                  
      }                                                                                            
                                                                                                   
      while (num > 0) {                                                                            
          buffer[i++] = '0' + (num % 10);                                                          
          num /= 10;                                                                               
      }                                                                                            
                                                                                                   
      while (i > 0) {                                                                              
          __io_putchar(buffer[--i]);                                                               
      }                                                                                            
  } 