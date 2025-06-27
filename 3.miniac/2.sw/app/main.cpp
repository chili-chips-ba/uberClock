#include <stdint.h>
#include "uberclock_libs.h"
#include "uberclock_regs.h"
#include "uart.h"
volatile uint32_t samples[1024] = {0};
int main(void)
{
   volatile csr_vp_t* csr = new csr_vp_t();
   char rx_data[UART_RXBUF_SIZE];
   // Turn on LED2
   //csr->gpio->led2(1);
   // Send Hello world to UART
   uart_send(csr, "Hello world!\r\n");
   int i = 0;
   while(1){
          //We read the data from the CS
          csr->dac->ch2(csr->adc->ch2() * 4);
          
          
          //We store the read data data in our circular buffer
          samples[i] = csr->adc->full();
          if (i > 1023) i = 0;
          else i++;
        
        // Set LED1 to the value of KEY1
        //csr->gpio->led1(csr->gpio->key1());
        //csr->dac->ch1(csr->adc->ch1() * 4);
   }
   // Receive (and echo) the text terminated with CRLF
   /*while(!uart_recv(csr, rx_data));
   uart_send(csr, "VENDOR  = ");
   uart_send_hex(csr, csr->hw_id->VENDOR(), 4);
   uart_send(csr, "\r\nPRODUCT = ");
   uart_send_hex(csr, csr->hw_id->PRODUCT(), 4);
   uart_send(csr, "\r\nVERSION = v");
   uart_send_dec(csr, csr->hw_version->MAJOR());
   uart_send_char(csr, '.');
   uart_send_dec(csr, csr->hw_version->MINOR());
   uart_send_char(csr, '.');
   uart_send_dec(csr, csr->hw_version->PATCH());
   uart_send(csr, "\r\n");*/
   return 0;
}
