#include <stdint.h>
#include "uberclock_libs.h"
#include "uberclock_regs.h"
#include "uart.h"

// --- HARDVERSKE KONSTANTE ---
// Word adresa (Word Index 512) koja odgovara Byte adresi 0x10000800
#define ADC_BUFFER_START_ADDR (0x10000800)
// Veličina buffera u riječima (32-bit/4 bajta)
#define BUFFER_WORDS          (4096) 
// POKAZIVAČ NA BAFER U BRAM-u
volatile uint32_t* adc_buffer = (volatile uint32_t*)ADC_BUFFER_START_ADDR;

// Signal spremnosti za Python
#define READY_SIGNAL "R"

// Funkcija za pokretanje i čekanje akvizicije
void run_single_acquisition(volatile csr_vp_t* csr) {
    
    // 1. SIGNALIZIRAJ HARDVERU DA POKRENE AKVIZICIJU
    // SW piše 1 u csr.adc.start
    csr->adc->start(1);
    uart_send(csr, "ADC: Akvizicija zapoceta...\r\n");
    
    // 2. ČEKAJ NA DONE FLAG
    // SW čita csr.adc.done dok ne postane 1 (blokirajuće čekanje)
    while (csr->adc->done() == 0) {
        // CPU čeka ovdje dok hardver ne završi punjenje bafera
    }
    
    // 3. DONE FLAG JE POSTAVLJEN - CITAJ PODATKE (Host preuzima)
    uart_send(csr, "ADC: Bafer popunjen. Podaci spremni za host.\r\n");
    uart_send(csr, "Pritisnite 'r' na laptopu za citanje.\r\n");
    
    // Opcionalno slanje signala 'R' za automatizaciju, ali je manje bitno sada.
    // uart_send(csr, READY_SIGNAL); 
    
    // NAPOMENA: Ovdje C kod stoji i ceka novi trigger (taster), 
    // pa su podaci sigurni u BRAM-u od prepisivanja.
}


int main(void)
{
    volatile csr_vp_t* csr = new csr_vp_t();
    char rx_data[UART_RXBUF_SIZE]; // Varijabla za prijem podataka (trenutno neiskorištena)
    
    // Turn on LED2 - signalizira da je sistem spreman
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
        if (csr->gpio->key1() == 1) { // Taster je aktivan na nuli (low active)
            
            // Debounce (jednostavni)
            for(volatile int i=0; i<100000; i++); 

            // Pokreni akviziciju, cekaj DONE, i posalji poruku hostu
            run_single_acquisition(csr);
            
            uart_send(csr, "Spreman za novu akviziciju (Cekam KEY1).\r\n");

            // Čekaj da se taster otpusti
            while (csr->gpio->key1() == 1) {} 
        }
        
        // Mala pauza da se smanji CPU opterećenje dok se čeka taster
        for(volatile int i=0; i<1000; i++);
    }
    
    return 0;
}
