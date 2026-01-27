#include <stdint.h>
#include "uberclock_libs.h"
#include "uberclock_regs.h"
#include "uart.h"

const uint16_t SINE_QUARTER_LUT[512] = {
    0, 25, 50, 75, 101, 126, 151, 176, 201, 226, 251, 276, 301, 327, 352, 377,
    402, 427, 452, 477, 502, 527, 552, 578, 603, 628, 653, 678, 703, 728, 753, 778,
    803, 828, 853, 878, 903, 928, 953, 978, 1003, 1028, 1053, 1077, 1102, 1127, 1152, 1177,
    1202, 1227, 1252, 1276, 1301, 1326, 1351, 1376, 1400, 1425, 1450, 1475, 1499, 1524, 1549, 1573,
    1598, 1623, 1647, 1672, 1696, 1721, 1746, 1770, 1795, 1819, 1844, 1868, 1893, 1917, 1941, 1966,
    1990, 2015, 2039, 2063, 2088, 2112, 2136, 2160, 2185, 2209, 2233, 2257, 2281, 2305, 2330, 2354,
    2378, 2402, 2426, 2450, 2474, 2498, 2522, 2545, 2569, 2593, 2617, 2641, 2665, 2688, 2712, 2736,
    2759, 2783, 2807, 2830, 2854, 2877, 2901, 2924, 2948, 2971, 2995, 3018, 3041, 3065, 3088, 3111,
    3135, 3158, 3181, 3204, 3227, 3250, 3273, 3296, 3319, 3342, 3365, 3388, 3411, 3434, 3457, 3479,
    3502, 3525, 3547, 3570, 3593, 3615, 3638, 3660, 3683, 3705, 3728, 3750, 3772, 3795, 3817, 3839,
    3861, 3883, 3905, 3928, 3950, 3972, 3994, 4015, 4037, 4059, 4081, 4103, 4124, 4146, 4168, 4189,
    4211, 4233, 4254, 4275, 4297, 4318, 4340, 4361, 4382, 4403, 4425, 4446, 4467, 4488, 4509, 4530,
    4551, 4572, 4592, 4613, 4634, 4655, 4675, 4696, 4716, 4737, 4757, 4778, 4798, 4819, 4839, 4859,
    4879, 4900, 4920, 4940, 4960, 4980, 5000, 5020, 5039, 5059, 5079, 5099, 5118, 5138, 5157, 5177,
    5196, 5216, 5235, 5254, 5274, 5293, 5312, 5331, 5350, 5369, 5388, 5407, 5426, 5445, 5463, 5482,
    5501, 5519, 5538, 5556, 5575, 5593, 5612, 5630, 5648, 5666, 5684, 5702, 5720, 5738, 5756, 5774,
    5792, 5810, 5827, 5845, 5863, 5880, 5898, 5915, 5932, 5950, 5967, 5984, 6001, 6018, 6035, 6052,
    6069, 6086, 6103, 6120, 6136, 6153, 6169, 6186, 6202, 6219, 6235, 6251, 6267, 6284, 6300, 6316,
    6332, 6348, 6363, 6379, 6395, 6411, 6426, 6442, 6457, 6473, 6488, 6503, 6519, 6534, 6549, 6564,
    6579, 6594, 6609, 6624, 6638, 6653, 6668, 6682, 6697, 6711, 6726, 6740, 6754, 6768, 6783, 6797,
    6811, 6824, 6838, 6852, 6866, 6880, 6893, 6907, 6920, 6934, 6947, 6960, 6973, 6987, 7000, 7013,
    7026, 7039, 7051, 7064, 7077, 7089, 7102, 7114, 7127, 7139, 7152, 7164, 7176, 7188, 7200, 7212,
    7224, 7236, 7247, 7259, 7271, 7282, 7294, 7305, 7316, 7328, 7339, 7350, 7361, 7372, 7383, 7394,
    7405, 7415, 7426, 7436, 7447, 7457, 7468, 7478, 7488, 7498, 7509, 7519, 7528, 7538, 7548, 7558,
    7567, 7577, 7587, 7596, 7605, 7615, 7624, 7633, 7642, 7651, 7660, 7669, 7678, 7686, 7695, 7704,
    7712, 7721, 7729, 7737, 7745, 7754, 7762, 7770, 7778, 7785, 7793, 7801, 7809, 7816, 7824, 7831,
    7838, 7846, 7853, 7860, 7867, 7874, 7881, 7888, 7894, 7901, 7908, 7914, 7921, 7927, 7933, 7939,
    7946, 7952, 7958, 7964, 7969, 7975, 7981, 7986, 7992, 7997, 8003, 8008, 8013, 8019, 8024, 8029,
    8034, 8038, 8043, 8048, 8053, 8057, 8062, 8066, 8070, 8075, 8079, 8083, 8087, 8091, 8095, 8099,
    8102, 8106, 8110, 8113, 8116, 8120, 8123, 8126, 8129, 8132, 8135, 8138, 8141, 8144, 8146, 8149,
    8152, 8154, 8156, 8159, 8161, 8163, 8165, 8167, 8169, 8171, 8172, 8174, 8176, 8177, 8179, 8180,
    8181, 8182, 8183, 8184, 8185, 8186, 8187, 8188, 8189, 8189, 8190, 8190, 8190, 8191, 8191, 8191
};

// --- HARDWARE CONSTANTS ---
#define ADC_BUFFER_START_ADDR  (0x10000400)
#define DAC_DPRAM_START_ADDR   (0x20002000) 

#define BUFFER_WORDS           (4096) 

volatile uint32_t* adc_buffer = (volatile uint32_t*)ADC_BUFFER_START_ADDR;
volatile uint32_t* dac_buffer = (volatile uint32_t*)DAC_DPRAM_START_ADDR;

// Funkcija za ispis uzoraka na UART
void debug_dump_snapshot(volatile csr_vp_t* csr, volatile uint32_t* buffer, uint32_t n_samples) {
    char log_msg[64];
    uart_send(csr, "\r\n--- SNAPSHOT BUFFER LOG ---\r\n");
    uart_send(csr, "Index | CH1 (Snapshot) | CH0 (Cont)\r\n");
    uart_send(csr, "-------------------------------------\r\n");

    uint32_t limit = (n_samples > 32) ? 32 : n_samples;

    for (uint32_t i = 0; i < limit; i++) {
        uint32_t raw = buffer[i];
        uint16_t ch1 = (uint16_t)(raw >> 16);
        uint16_t ch0 = (uint16_t)(raw & 0xFFFF);
        
        // Ako ti sprintf pravi problem, koristi više uart_send poziva:
        uart_send(csr, "[");
        uart_send_hex(csr, i, 2); 
        uart_send(csr, "]  |  ");
        uart_send_hex(csr, ch1, 4);
        uart_send(csr, "       |  ");
        uart_send_hex(csr, ch0, 4);
        uart_send(csr, "\r\n");
    }
    uart_send(csr, "-------------------------------------\r\n");
}


/**
 * Configure both DAC channels using lookup table for Sine and integer math for others.
 * f0_hz / f1_hz: Target frequencies in Hz.
 * mv0 / mv1: Peak-to-Peak amplitude in millivolts.
 * type0 / type1: 0=Sine (LUT), 1=Sawtooth, 2=Square.
 * mode0 / mode1: 0=Continuous, 1=Snapshot (One-Shot).
 */
void setup_dac_dual_synchronized(volatile csr_vp_t* csr, 
                                 uint32_t f0_hz, uint32_t mv0, int type0, int mode0,
                                 uint32_t f1_hz, uint32_t mv1, int type1, int mode1) {
                                 
    const uint32_t f_clk = 65000000; // 65 MHz
    
    // 1. Disable channels before writing to memory
    csr->dac_mem_ctrl->en_ch0(0);
    csr->dac_mem_ctrl->en_ch1(0);

    // 2. Calculate number of samples per period
    uint32_t n0 = (f0_hz > 0) ? ((f_clk + (f0_hz / 2)) / f0_hz) : 0;
    uint32_t n1 = (f1_hz > 0) ? ((f_clk + (f1_hz / 2)) / f1_hz) : 0;

    // Hardware constraints: Max 2048 samples in DPRAM
    if (n0 > 2048) n0 = 2048; if (n0 < 2 && f0_hz > 0) n0 = 2;
    if (n1 > 2048) n1 = 2048; if (n1 < 2 && f1_hz > 0) n1 = 2;

    // 3. Scale amplitude (Max 8191 for 14-bit DAC with center offset)
    uint32_t amp0 = (mv0 * 8191) / 10000;
    uint32_t amp1 = (mv1 * 8191) / 10000;
    
    if (amp0 > 8191) amp0 = 8191;
    if (amp1 > 8191) amp1 = 8191;

    // 4. Fill DAC DPRAM
    for (uint32_t i = 0; i < 2048; i++) {
        uint16_t v0 = 8192, v1 = 8192; // Default to mid-scale (0V)

        // --- Channel 0 Processing ---
        if (f0_hz > 0 && i < n0) {
            if (type0 == 0) { // Sine wave using LUT
                // Map current sample 'i' to 2048-point phase (4 * 512)
                uint32_t phase = (i * 2048) / n0;
                uint32_t idx = phase % 512;
                uint32_t quadrant = (phase / 512) % 4;
                int16_t s_val;

                if      (quadrant == 0) s_val = SINE_QUARTER_LUT[idx];
                else if (quadrant == 1) s_val = SINE_QUARTER_LUT[511 - idx];
                else if (quadrant == 2) s_val = -SINE_QUARTER_LUT[idx];
                else                    s_val = -SINE_QUARTER_LUT[511 - idx];

                v0 = (uint16_t)(((s_val * (int32_t)amp0) / 8191) + 8192);
            } else if (type0 == 1) { // Sawtooth
                v0 = (uint16_t)((amp0 * 2 * i / n0) + (8192 - amp0));
            } else { // Square
                v0 = (i < n0 / 2) ? (8192 + amp0) : (8192 - amp0);
            }
        }

        // --- Channel 1 Processing ---
        if (f1_hz > 0 && i < n1) {
            if (type1 == 0) { // Sine wave using LUT
                uint32_t phase = (i * 2048) / n1;
                uint32_t idx = phase % 512;
                uint32_t quadrant = (phase / 512) % 4;
                int16_t s_val;
                /*
                if      (quadrant == 0) s_val = SINE_QUARTER_LUT[idx];
                else if (quadrant == 1) s_val = SINE_QUARTER_LUT[511 - idx];
                else if (quadrant == 2) s_val = -SINE_QUARTER_LUT[idx];
                else                    s_val = -SINE_QUARTER_LUT[511 - idx];

                v1 = (uint16_t)(((s_val * (int32_t)amp1) / 8191) + 8192);
                */
                
                int32_t calculate_v;
                if      (quadrant == 0) s_val = SINE_QUARTER_LUT[idx];
                else if (quadrant == 1) s_val = SINE_QUARTER_LUT[511 - idx];
                else if (quadrant == 2) s_val = -((int16_t)SINE_QUARTER_LUT[idx]);      
                else                    s_val = -((int16_t)SINE_QUARTER_LUT[511 - idx]);

                // Ključni dio: Prvo pomnožimo, pa podijelimo, pa tek onda dodamo offset
                calculate_v = ((int32_t)s_val * (int32_t)amp1) / 8191;
                v1 = (uint16_t)(calculate_v + 8192);
            
            } else if (type1 == 1) { // Sawtooth
                v1 = (uint16_t)((amp1 * 2 * i / n1) + (8192 - amp1));
            } else { // Square
                v1 = (i < n1 / 2) ? (8192 + amp1) : (8192 - amp1);
            }
        }

        // Write both channels to 32-bit DPRAM (Ch1 in high 16 bits, Ch0 in low 16 bits)
        dac_buffer[i] = ((uint32_t)v1 << 16) | ((uint32_t)v0 & 0xFFFF);
    }

    // 5. Apply configuration and Synchronized Start
    uint32_t ctrl_reg = 0;
    if (f0_hz > 0) ctrl_reg |= (1 << 30); // Enable CH0
    if (f1_hz > 0) ctrl_reg |= (1 << 31); // Enable CH1
    
    if (mode0) ctrl_reg |= (1 << 28); // Snapshot za CH0
    if (mode1) ctrl_reg |= (1 << 29); // Snapshot za CH1
    
    // Set buffer lengths and trigger
    ctrl_reg |= ((n1 & 0x7FF) << 16) | (n0 & 0x7FF);
    //ctrl_reg |= (2047 << 16) | (n0 & 0x7FF);
    *(volatile uint32_t*)(csr->dac_mem_ctrl->get_addr()) = ctrl_reg;
}

/**
 * Trigger ADC acquisition and stream data via UART
 */
void run_single_acquisition(volatile csr_vp_t* csr) {
  
    uart_send(csr, "ADC: Triggering...\r\n");
    csr->adc->start(1); 
    
    for(volatile int i=0; i<100; i++); 
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
        for(volatile int j = 0; j < 50; j++);
    }
    uart_send(csr, "=== BRAM_TRANSFER_END ===\r\n");
}

int main(void)
{
    volatile csr_vp_t* csr = new csr_vp_t();
    csr->gpio->led2(1); 
    
    // Initialize DAC with synchronized waveforms
    setup_dac_dual_synchronized(csr, 
                                6000000, 4000, 0, 0,   // Channel 0: 6MHz Sine
                                6000000, 3000, 0, 0);  // Channel 1: 6MHz Sine                                 

    uart_send(csr, "Press KEY1 to capture ADC snapshot...\r\n");

    while(1){
        if (csr->gpio->key1() == 1) {
            uart_send(csr, "KEY1 Pressed.\r\n");
            run_single_acquisition(csr);
            while (csr->gpio->key1() == 1) {} 
        }
        if (csr->gpio->key2() == 1) {
            uart_send(csr, "DAC: CH1 One-Shot Triggered!\r\n");

            // 1. Prvo sve ugasi (da budemo sigurni da smo na nuli)
            csr->dac_mem_ctrl->en_ch0(0);
            csr->dac_mem_ctrl->en_ch1(0);

            // 2. Konfiguriši CH0 da stalno radi (Continuous)
            // CH0 = Continuous (mode 0), CH1 = One-shot (mode 1)
            setup_dac_dual_synchronized(csr, 
                                            6500000, 2048, 0, 0,  // CH0
                                            6500000, 2048, 0, 1); // CH1 namješten na Snapshot

            // 3. Pošto setup_dac već postavlja en=1, Snapshot je već okinuo ovdje.
            // Da bismo bili sigurni da je završio snimanje prije čitanja:
            for(volatile int i=0; i<5000; i++); 

            debug_dump_snapshot(csr, dac_buffer, 15);

            while (csr->gpio->key2() == 1); 
        }
    }
    return 0;
}
