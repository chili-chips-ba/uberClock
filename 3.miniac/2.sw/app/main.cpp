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

void setup_dac_dual_sine(volatile csr_vp_t* csr, 
                         float f0, int amp0, 
                         float f1, int amp1) {
    csr->dac_mem_ctrl->en(0);

    float f_clk = 65000000.0f;
    uint32_t best_n = 0;
    float min_error = 1e10;

    // Tražimo optimalno N (između 64 i 2048 uzoraka)
    // Cilj je da N * (f/f_clk) bude što bliže cijelom broju za OBJE frekvencije
    for (uint32_t n = 64; n <= 2048; n++) {
        float k0 = (f0 * (float)n) / f_clk;
        float k1 = (f1 * (float)n) / f_clk;

        // Računamo koliko smo daleko od cijelog broja perioda
        float err0 = k0 - (uint32_t)(k0 + 0.5f);
        float err1 = k1 - (uint32_t)(k1 + 0.5f);
        float total_err = (err0 * err0) + (err1 * err1);

        if (total_err < min_error) {
            min_error = total_err;
            best_n = n;
        }
        // Ako nađemo savršeno N, prekidamo potragu
        if (total_err < 0.00001f) break;
    }

    // Sada popunjavamo tabelu sa pronađenim best_n
    for (uint32_t i = 0; i < best_n; i++) {
        // Koristimo fazu koja se savršeno zatvara na best_n
        // phase = 2 * PI * (broj_perioda / best_n) * trenutni_uzorak
        float k0_final = (float)((uint32_t)((f0 * (float)best_n) / f_clk + 0.5f));
        float k1_final = (float)((uint32_t)((f1 * (float)best_n) / f_clk + 0.5f));

        float phase0 = 2.0f * (float)M_PI * (k0_final / (float)best_n) * (float)i;
        float phase1 = 2.0f * (float)M_PI * (k1_final / (float)best_n) * (float)i;

        uint16_t val0 = (uint16_t)(amp0 * __builtin_sin(phase0) + 8192);
        uint16_t val1 = (uint16_t)(amp1 * __builtin_sin(phase1) + 8192);
        
        dac_buffer[i] = ((uint32_t)val1 << 16) | (uint32_t)val0;
    }

    csr->dac_mem_ctrl->len(best_n);
    csr->dac_mem_ctrl->en(1);
    
    // uart_send(csr, "DAC: Selected N = "); uart_send_hex(csr, best_n, 4); uart_send(csr, "\r\n");
}

void setup_dac_shapes(volatile csr_vp_t* csr, float freq, int amp0, int amp1, int type) {
    csr->dac_mem_ctrl->en(0);
    uint32_t n = (uint32_t)(65000000.0f / freq);
    if (n > 2048) n = 2048;

    for (uint32_t i = 0; i < n; i++) {
        uint16_t v0, v1;
        if (type == 0) { // Sawtooth
            v0 = (uint16_t)((amp0 * 2 * (float)i / (float)n) + (8192 - amp0));
            v1 = (uint16_t)((amp1 * 2 * (float)i / (float)n) + (8192 - amp1));
        } else { // Square
            v0 = (i < n/2) ? (8192 + amp0) : (8192 - amp0);
            v1 = (i < n/2) ? (8192 + amp1) : (8192 - amp1);
        }
        dac_buffer[i] = ((uint32_t)v1 << 16) | (uint32_t)v0;
    }
    csr->dac_mem_ctrl->len(n);
    csr->dac_mem_ctrl->en(1);
}

/**
 * Trigger ADC acquisition and stream data via UART
 */
void run_single_acquisition(volatile csr_vp_t* csr) {
    int timeout;
    uart_send(csr, "\r\n[DEBUG] 1. Ulaz u funkciju\r\n");
    
    uart_send(csr, "ADC: Triggering...\r\n");
    csr->adc->start(1); 
    
    for(volatile int i=0; i<100; i++); 
    csr->adc->start(0); 
    
    uart_send(csr, "[DEBUG] 2. ADC Startovan, cekam 'done'...\r\n");
    
    uart_send(csr, "ADC: Acquisition in progress...\r\n");
    
    while (csr->adc->done() == 0) {
        // Waiting for hardware 'done' flag
        timeout++;
        if (timeout > 20000000) { // Cekaj par sekundi
            //uart_send(csr, "[ALARM] ADC hardver ne odgovara (TIMEOUT)!\r\n");
        } 
    }
    uart_send(csr, "[DEBUG] 3. Hardver javio 'done'. Pokusavam citanje memorije...\r\n");
    uart_send(csr, "=== BRAM_TRANSFER_START ===\r\n");
    for (int i = 0; i < BUFFER_WORDS; i++) {
        uint32_t data = adc_buffer[i];
        uart_send_hex(csr, data, 8);
        
        if (i < BUFFER_WORDS - 1) {
            uart_send(csr, "\r\n"); 
        } else {
            uart_send(csr, "\n"); 
        }
        for(volatile int j = 0; j < 50; j++);
    }
    uart_send(csr, "=== BRAM_TRANSFER_END ===\r\n");
    
    uart_send(csr, "[DEBUG] 4. Kraj transfera uspjesan.\r\n");
}

int main(void)
{
    volatile csr_vp_t* csr = new csr_vp_t();
    csr->gpio->led2(1); 

    // Initialize DAC
    // -- Primjer 1: Sinus --
    setup_dac_dual_sine(csr, 4000000.0f, 1000, 4000000.0f, 1000);  // ampl: od 0 do 8192 
    
    // -- Primjer 2: Pila (0) ili Četvrtka (1) --
    //setup_dac_shapes(csr, 6000000.0f, 2000, 5000, 1);

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
