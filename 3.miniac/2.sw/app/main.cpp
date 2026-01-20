#include <stdint.h>
#include "uberclock_libs.h"
#include "uberclock_regs.h"
#include "uart.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- HARDWARE CONSTANTS ---
#define ADC_BUFFER_START_ADDR  (0x10000400)
#define DAC_DPRAM_START_ADDR   (0x20002000) 

#define BUFFER_WORDS           (4096) 
#define DAC_SAMPLES            (65)

volatile uint32_t* adc_buffer = (volatile uint32_t*)ADC_BUFFER_START_ADDR;
volatile uint32_t* dac_buffer = (volatile uint32_t*)DAC_DPRAM_START_ADDR;

/**
 * Generate sine wave table using built-in math functions to avoid 
 * standard library dependencies in freestanding mode.
 */
void prepare_dac_sine(volatile csr_vp_t* csr, int amplitude) {
    uart_send(csr, "DAC: Generating synchronized table for 65MHz Clock...\r\n");
    
    csr->dac_mem_ctrl->en(0);

    // Na 65 uzoraka pri 65MHz: k=3 daje 3MHz, k=11 daje 11MHz
    float k0 = 3.0f;
    float k1 = 11.0f;

    for (int i = 0; i < DAC_SAMPLES; i++) {
        // f = (k * f_clk) / N. Pošto je f_clk = N = 65, f = k.
        float phase0 = 2.0f * (float)M_PI * (k0 / (float)DAC_SAMPLES) * (float)i;
        float phase1 = 2.0f * (float)M_PI * (k1 / (float)DAC_SAMPLES) * (float)i;

        uint16_t val0 = (uint16_t)(amplitude * __builtin_sin(phase0) + 8192);
        uint16_t val1 = (uint16_t)(amplitude * __builtin_sin(phase1) + 8192);
        
        uint32_t packed_sample = ((uint32_t)val1 << 16) | (uint32_t)val0;
        dac_buffer[i] = packed_sample;
    }

    // Postavljamo hardver na 65 uzoraka
    csr->dac_mem_ctrl->len(DAC_SAMPLES);
    csr->dac_mem_ctrl->en(1);
    
    uart_send(csr, "DAC: 3MHz and 11MHz signals active (N=65).\r\n");
}

/**
 * Trigger ADC acquisition and stream data via UART
 */
void run_single_acquisition(volatile csr_vp_t* csr) {
    uart_send(csr, "ADC: Triggering...\r\n");
    csr->adc->start(1); 
    
    for(volatile int i=0; i<1000; i++); 
    csr->adc->start(0); 
    
    uart_send(csr, "ADC: Acquisition in progress...\r\n");
    
    while (csr->adc->done() == 0) {
        // Waiting for hardware 'done' flag
    }
    
    uart_send(csr, "=== BRAM_TRANSFER_START ===\r\n");
    for (int i = 0; i < BUFFER_WORDS; i++) {
        uint32_t data = adc_buffer[i];
        uart_send_hex(csr, data, 8);
        
        if (i < BUFFER_WORDS - 1) {
            uart_send(csr, "\r\n"); 
        } else {
            uart_send(csr, "\n"); 
        }
        for(volatile int j = 0; j < 500; j++);
    }
    uart_send(csr, "=== BRAM_TRANSFER_END ===\r\n");
}

int main(void)
{
    volatile csr_vp_t* csr = new csr_vp_t();
    csr->gpio->led2(1); 

    uart_send(csr, "--- RISC-V ADC/DAC Mixed Signal Demo ---\r\n");

    // Initialize DAC
    prepare_dac_sine(csr, 2000); 

    uart_send(csr, "Press KEY1 to capture ADC snapshot...\r\n");

    while(1){
        if (csr->gpio->key1() == 1) {
            uart_send(csr, "KEY1 Pressed.\r\n");
            run_single_acquisition(csr);
            uart_send(csr, "Ready for next trigger.\r\n");
            while (csr->gpio->key1() == 1) {} 
        }
    }
    return 0;
}
