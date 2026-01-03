#include <stdint.h>
#include "uberclock_libs.h"
#include "uberclock_regs.h"
#include "uart.h"

// --- HARDVERSKE KONSTANTE ---
// Pocetna adresa
#define ADC_BUFFER_START_ADDR (0x10000400)
// Velicina buffera u rijecima (32-bit/4 bajta)
#define BUFFER_WORDS          (4096) 
// Pokazivac na bafer u BRAM-u
volatile uint32_t* adc_buffer = (volatile uint32_t*)ADC_BUFFER_START_ADDR;

// Funkcija za pokretanje i čekanje akvizicije
void run_single_acquisition(volatile csr_vp_t* csr) {
    
    // 1. SIGNALIZIRAJ HARDVERU DA POKRENE AKVIZICIJU
    uart_send(csr, "ADC: Priprema...\r\n");
    csr->adc->start(1); // START puls (postavi na 1)
    
    // Potrebno je spustiti start signal da bi se FSM prebacio u RUNNING
    for(int i=0; i<1000; i++); // Mala pauza (moze i bez nje, ali je sigurnije)
    csr->adc->start(0); // Spusti start signal
    
    uart_send(csr, "ADC: Akvizicija zapoceta...\r\n");
    
    // 2. ČEKAJ NA DONE FLAG
    uart_send(csr, "ADC: Cekamo done...\r\n");
    while (csr->adc->done() == 0) {
        uart_send_char(csr, '.'); 
    }
    
    // 3. DONE FLAG JE POSTAVLJEN - CITAJ I SALJI PODATKE
    uart_send(csr, "ADC: Bafer popunjen. Pokrecem transfer...\r\n");
    
    // --- START TRANSFERA - SIGNAL ZA PYTHON ---
    uart_send(csr, "=== BRAM_TRANSFER_START ===\r\n");
    
    // Čitanje bafera i slanje
    for (int i = 0; i < BUFFER_WORDS; i++) {
        uint32_t data = adc_buffer[i];
        // Šalji 32-bitnu riječ kao 8 heksadecimalnih karaktera
        uart_send_hex(csr, data, 8);
        //uart_send(csr, "\r\n"); // Novi red
        if (i < BUFFER_WORDS - 1) { // Šalji \r\n samo ako nije zadnji
            uart_send(csr, "\r\n"); 
        } else {
            // Dodajte samo \n za zadnji uzorak ili čak nista
            uart_send(csr, "\n"); 
        }
        for(volatile int j = 0; j < 2000; j++);
    }
    
    // --- KRAJ TRANSFERA - SIGNAL ZA PYTHON ---
    uart_send(csr, "=== BRAM_TRANSFER_END ===\r\n");
    
    uart_send(csr, "ADC: Transfer zavrsen.\r\n");
}


int main(void)
{
    volatile csr_vp_t* csr = new csr_vp_t();
    
    csr->gpio->led2(1);

    uart_send(csr, "--- RISC-V ADC Snapshot Acquisition Demo ---\r\n");
    uart_send(csr, "Buffer adresa: 0x");
    uart_send_hex(csr, ADC_BUFFER_START_ADDR, 8);
    uart_send(csr, ", Velicina: ");
    uart_send_dec(csr, BUFFER_WORDS);
    uart_send(csr, " uzoraka.\r\n");
    uart_send(csr, "Pritisnite KEY1 na FPGA za akviziciju...\r\n");

    while(1){
        
        // 1. Čekaj da se pritisne KEY1 (za pokretanje akvizicije)
        if (csr->gpio->key1() == 1) {
        
            uart_send(csr, "Pritisnut KEY1.\r\n");

            // Pokreni akviziciju, cekaj DONE, i posalji bafer
            run_single_acquisition(csr);
            
            uart_send(csr, "Spreman za novu akviziciju (Cekam KEY1).\r\n");

            // Čekaj da se taster otpusti
            while (csr->gpio->key1() == 1) {} 
        }
        
    }
    
    return 0;
}
