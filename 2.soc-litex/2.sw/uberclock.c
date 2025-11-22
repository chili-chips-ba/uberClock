// uberclock.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include <generated/soc.h>

#include "uberclock.h"
#include "console.h"

/* ----- cache flush wrapper (safe if cache.h not present) ----------------- */
#if defined(__has_include)
#if __has_include(<libbase/cache.h>)
#include <libbase/cache.h>
static inline void ub_cache_sync(void) { flush_cpu_dcache(); flush_l2_cache(); }
#else
static inline void ub_cache_sync(void) { /* no-op */ }
#endif
#else
/* If toolchain lacks __has_include, just make it a no-op or add your own guards */
static inline void ub_cache_sync(void) { /* no-op */ }
#endif

/* === LS DEBUG helpers: tiny binary UART writers ========================== */
static inline void uart_write_bytes(const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < n; ++i) uart_write((char)p[i]);
}
static inline void uart_write_u16_le(uint16_t v) {
    uart_write((char)(v & 0xFF));
    uart_write((char)((v >> 8) & 0xFF));
}
static inline void uart_write_u32_le(uint32_t v) {
    uart_write((char)( v        & 0xFF));
    uart_write((char)((v >> 8 ) & 0xFF));
    uart_write((char)((v >> 16) & 0xFF));
    uart_write((char)((v >> 24) & 0xFF));
}

/* ========================================================================= */
/*                         UberClock (existing path)                          */
/* ========================================================================= */

static volatile int ce_event = 0;
static int16_t  g_mag;
static int32_t  g_phase;

static inline unsigned parse_u(const char *s, unsigned max, const char *what) {
    unsigned v = (unsigned)strtoul(s ? s : "0", NULL, 0);
    if (v >= max) printf("Error: %s must be 0..%u\n", what, max-1);
    return v;
}

/* ---- COMMIT helper ---- */
static inline void uc_commit(void){
    #ifdef CSR_CFG_LINK_BASE
    cfg_link_commit_write(1);
    cfg_link_commit_write(0);
    #endif
}

/* ---- ISR ---- */
#ifdef CSR_EVM_PENDING_ADDR
static void ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    ce_event = 1;
}
#endif

/* ---- Help ---- */
static void uc_help(char *args) {
    (void)args;
    puts_help_header("UberClock commands");
    puts("  phase_nco    <val>      (0..524287)");
    puts("  phase_cpu    <val>      (0..524287)");
    puts("  phase_down_1 <val> ... phase_down_5 <val>  (0..524287)");
    puts("  input_select <val>              (0=ADC, 1=NCO, 2=CPU, 3=TEST RAMP)");
    puts("  upsampler_input_mux <val>       (0=Gain,1=CPU,2=CPU NCO)");
    puts("  output_select_ch1 <0..3>");
    puts("  output_select_ch2 <0..3>");
    puts("  gain1|gain2|gain3|gain4|gain5 <int32>");
    puts("  final_shift  <int32>");
    puts("  phase");
    puts("  magnitude");
    puts("");
}

/* ---- Phase/NCO/downconversion ---- */
static void cmd_phase_nco(char *a) {
    #ifdef CSR_MAIN_PHASE_INC_NCO_ADDR
    unsigned p = parse_u(a, 1u<<19, "phase_nco"); if (p >= (1u<<19)) return;
    main_phase_inc_nco_write(p);
    uc_commit();
    printf("Input NCO phase increment set to %u\n", p);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_phase_cpu(char *a) {
    #ifdef CSR_MAIN_PHASE_INC_CPU_ADDR
    unsigned p = parse_u(a, 1u<<19, "phase_cpu"); if (p >= (1u<<19)) return;
    main_phase_inc_cpu_write(p);
    uc_commit();
    printf("CPU phase increment set to %u\n", p);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_phase_dn(char *a, int ch) {
    #ifdef CSR_MAIN_PHASE_INC_DOWN_1_ADDR
    unsigned p = parse_u(a, 1u<<19, "phase_down"); if (p >= (1u<<19)) return;
    switch (ch) {
        case 1: main_phase_inc_down_1_write(p); break;
        case 2: main_phase_inc_down_2_write(p); break;
        case 3: main_phase_inc_down_3_write(p); break;
        case 4: main_phase_inc_down_4_write(p); break;
        case 5: main_phase_inc_down_5_write(p); break;
        default: return;
    }
    uc_commit();
    printf("Downconversion phase ch%d increment set to %u\n", ch, p);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_phase_down_1(char *a){ cmd_phase_dn(a,1); }
static void cmd_phase_down_2(char *a){ cmd_phase_dn(a,2); }
static void cmd_phase_down_3(char *a){ cmd_phase_dn(a,3); }
static void cmd_phase_down_4(char *a){ cmd_phase_dn(a,4); }
static void cmd_phase_down_5(char *a){ cmd_phase_dn(a,5); }

/* ---- Muxes / gains ---- */
static void cmd_output_sel_ch1(char *a) {
    #ifdef CSR_MAIN_OUTPUT_SELECT_CH1_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x3u;
    main_output_select_ch1_write(v);
    uc_commit();
    printf("output_select_ch1 set to %u\n", v);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_output_sel_ch2(char *a) {
    #ifdef CSR_MAIN_OUTPUT_SELECT_CH2_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x3u;
    main_output_select_ch2_write(v);
    uc_commit();
    printf("output_select_ch2 set to %u\n", v);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_input_select(char *a) {
    #ifdef CSR_MAIN_INPUT_SELECT_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_input_select_write(v);
    uc_commit();
    printf("Main input select register set to %u\n", v);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_ups_in_mux(char *a) {
    #ifdef CSR_MAIN_UPSAMPLER_INPUT_MUX_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_upsampler_input_mux_write(v);
    uc_commit();
    printf("Upsampler input mux register set to %u\n", v);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_gain(char *a, int idx) {
    #ifdef CSR_MAIN_GAIN1_ADDR
    int32_t g = (int32_t)strtol(a ? a : "0", NULL, 0);
    switch(idx){
        case 1: main_gain1_write((uint32_t)g); break;
        case 2: main_gain2_write((uint32_t)g); break;
        case 3: main_gain3_write((uint32_t)g); break;
        case 4: main_gain4_write((uint32_t)g); break;
        case 5: main_gain5_write((uint32_t)g); break;
    }
    uc_commit();
    printf("Gain%d register set to %ld (0x%08lX)\n", idx, (long)g, (unsigned long)g);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_gain1(char *a){ cmd_gain(a,1); }
static void cmd_gain2(char *a){ cmd_gain(a,2); }
static void cmd_gain3(char *a){ cmd_gain(a,3); }
static void cmd_gain4(char *a){ cmd_gain(a,4); }
static void cmd_gain5(char *a){ cmd_gain(a,5); }

static void cmd_final_shift(char *a) {
    #ifdef CSR_MAIN_FINAL_SHIFT_ADDR
    int32_t fs = (int32_t)strtol(a ? a : "0", NULL, 0);
    main_final_shift_write((uint32_t)fs);
    uc_commit();
    printf("S register set to %ld (0x%08lX)\n", (long)fs, (unsigned long)fs);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}
static void cmd_phase_print(char *a){ (void)a; printf("Phase %ld\n", (long)g_phase); }
static void cmd_magnitude  (char *a){ (void)a; printf("Magnitude %d\n", g_mag); }

/* ========================================================================= */
/*                     UberDDR3 + S2MM (ramp-to-DDR) CLI                      */
/* ========================================================================= */

static void ub_help(char *args) {
    (void)args;
    puts_help_header("UberDDR3/S2MM (ramp) commands");
    puts("  ub_info");
    puts("      Print DDR calibration state and base.");
    puts("  ub_ramp <addr_hex> <beats> [seed] [size]");
    puts("      Program ramp generator + S2MM to write `beats` DW-wide words");
    puts("      at DDR <addr_hex> (absolute).");
    puts("        seed: 0..255  (default 0)");
    puts("        size: bus|32|16|8  (default bus = full DW width)");
    puts("  ub_wait");
    puts("      Poll until DMA not busy; prints error if any.");
    puts("  ub_hexdump <addr_hex> <bytes>");
    puts("      Dump memory to verify write.");
    puts("");
}

static inline uint8_t ub_size_to_code(const char *s) {
    if (!s) return 0;             // 00 = bus width
    if (!strcmp(s,"bus")) return 0;
    if (!strcmp(s,"32"))  return 1;
    if (!strcmp(s,"16"))  return 2;
    if (!strcmp(s,"8"))   return 3;
    return 0;
}

static void cmd_ub_info(char *a) {
    (void)a;
    #ifdef CSR_UBDDR3_BASE
    int cal = 0;
    #ifdef CSR_UBDDR3_CALIB_DONE_ADDR
    cal = ubddr3_calib_done_read();
    #endif
    printf("UBDDR3 base: 0x%08lx  calib_done: %d  (UBDDR3_MEM_BASE: 0x%08lx)\n",
           (unsigned long)CSR_UBDDR3_BASE,
           cal
           #ifdef UBDDR3_MEM_BASE
           , (unsigned long)UBDDR3_MEM_BASE
           #else
           , (unsigned long)0
           #endif
    );
    #else
    puts("No ubddr3 CSRs in this build.");
    #endif
}

static void cmd_ub_ramp(char *args) {
    #ifdef CSR_UBDDR3_BASE
    char *p = args;
    char *tok_addr = strtok(p, " \t");
    char *tok_beats= strtok(NULL, " \t");
    char *tok_seed = strtok(NULL, " \t");
    char *tok_size = strtok(NULL, " \t");

    if (!tok_addr || !tok_beats) {
        puts("Usage: ub_ramp <addr_hex> <beats> [seed] [size]");
        return;
    }

    uint64_t addr = strtoull(tok_addr, NULL, 0);
    uint32_t beats= (uint32_t)strtoul(tok_beats, NULL, 0);
    uint8_t  seed = (uint8_t) (tok_seed ? strtoul(tok_seed, NULL, 0) : 0);
    uint8_t  sz   = ub_size_to_code(tok_size);

    #ifdef CSR_UBDDR3_RAMP_LEN_ADDR
    ubddr3_ramp_len_write(beats);
    #endif
    #ifdef CSR_UBDDR3_RAMP_SEED_ADDR
    ubddr3_ramp_seed_write(seed);
    #endif
    #ifdef CSR_UBDDR3_DMA_INC_ADDR
    ubddr3_dma_inc_write(1);
    #endif
    #ifdef CSR_UBDDR3_DMA_SIZE_ADDR
    ubddr3_dma_size_write(sz);   // 0=bus, 1=32b, 2=16b, 3=byte
    #endif
    #ifdef CSR_UBDDR3_DMA_ADDR0_ADDR
    ubddr3_dma_addr0_write((uint32_t)(addr & 0xffffffffu));
    #endif
    #ifdef CSR_UBDDR3_DMA_ADDR1_ADDR
    ubddr3_dma_addr1_write((uint32_t)(addr >> 32));
    #endif

    #ifdef CSR_UBDDR3_DMA_REQ_ADDR
    ubddr3_dma_req_write(1);
    ubddr3_dma_req_write(0);
    #endif

    printf("S2MM ramp: addr=0x%08lx_%08lx beats=%u seed=%u size=%s\n",
           (unsigned long)(addr >> 32), (unsigned long)(addr & 0xffffffffu),
           (unsigned)beats, (unsigned)seed,
           (sz==0)?"bus":(sz==1)?"32":(sz==2)?"16":"8");

    #else
    puts("No ubddr3 CSRs in this build.");
    #endif
}

static void cmd_ub_wait(char *a) {
    (void)a;
    #ifdef CSR_UBDDR3_BASE
    #ifdef CSR_UBDDR3_DMA_BUSY_ADDR
    printf("Waiting for DMA ... "); fflush(stdout);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();    // make DMA writes visible before any CPU reads
    puts("done.");
    #ifdef CSR_UBDDR3_DMA_ERR_ADDR
    if (ubddr3_dma_err_read())
        puts("DMA error flag is set!");
    #endif
    #else
    puts("ubddr3_dma_busy CSR not present.");
    #endif
    #else
    puts("No ubddr3 CSRs in this build.");
    #endif
}

static void cmd_ub_hexdump(char *a) {
    #ifdef UBDDR3_MEM_BASE
    char *tok_addr = strtok(a, " \t");
    char *tok_len  = strtok(NULL, " \t");
    if (!tok_addr || !tok_len) {
        puts("Usage: ub_hexdump <addr_hex> <bytes>");
        return;
    }
    uint64_t addr = strtoull(tok_addr, NULL, 0);
    uint32_t len  = (uint32_t)strtoul(tok_len, NULL, 0);

    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;
    for (uint32_t i = 0; i < len; i++) {
        if ((i & 0x0f) == 0) printf("\n%08lx: ", (unsigned long)((addr + i) & 0xffffffffu));
        printf("%02x ", p[i]);
    }
    puts("");
    #else
    puts("UBDDR3_MEM_BASE not defined; cannot hexdump.");
    #endif
}

static int mem_cmp_u8(uintptr_t addr, const uint8_t *exp, unsigned n) {
    volatile uint8_t *p = (volatile uint8_t*)addr;
    for (unsigned i=0; i<n; ++i) if (p[i] != exp[i]) return (int)i;
    return -1;
}

static void pattern_fill(uint8_t *buf, unsigned n, uint8_t start) {
    for (unsigned i=0;i<n;i++) buf[i] = (uint8_t)(start + i);
}

static void cmd_ub_selftest(char *a) {
    #ifdef CSR_UBDDR3_BASE
    (void)a;
    const uintptr_t base = 0xA0000000u;
    uint8_t exp[128];
    // 1) BUS width @ aligned
    pattern_fill(exp, 64, 0x00);
    ubddr3_ramp_seed_write(0x00);
    ubddr3_ramp_len_write(2);            // 2 beats * 32 bytes = 64 bytes
    ubddr3_dma_size_write(0);            // bus
    ubddr3_dma_addr0_write((uint32_t)base);
    ubddr3_dma_addr1_write(0);
    ubddr3_dma_req_write(1); ubddr3_dma_req_write(0);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    int off = mem_cmp_u8(base, exp, 64);
    if (off >= 0) { printf("BUS/aligned mismatch @+%d\n", off); return; }

    // 2) 32-bit @ unaligned
    pattern_fill(exp, 32, 0x20);
    ubddr3_ramp_seed_write(0x20);
    ubddr3_ramp_len_write(8);            // 8 * 4B
    ubddr3_dma_size_write(1);            // 32-bit
    ubddr3_dma_addr0_write((uint32_t)(base+1));
    ubddr3_dma_req_write(1); ubddr3_dma_req_write(0);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    off = mem_cmp_u8(base+1, exp, 32);
    if (off >= 0) { printf("32b/unaligned mismatch @+%d\n", off); return; }

    // 3) 16-bit @ unaligned
    pattern_fill(exp, 32, 0x40);
    ubddr3_ramp_seed_write(0x40);
    ubddr3_ramp_len_write(16);           // 16 * 2B
    ubddr3_dma_size_write(2);            // 16-bit
    ubddr3_dma_addr0_write((uint32_t)(base+2));
    ubddr3_dma_req_write(1); ubddr3_dma_req_write(0);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    off = mem_cmp_u8(base+2, exp, 32);
    if (off >= 0) { printf("16b/unaligned mismatch @+%d\n", off); return; }

    // 4) 8-bit @ unaligned
    pattern_fill(exp, 16, 0x60);
    ubddr3_ramp_seed_write(0x60);
    ubddr3_ramp_len_write(16);           // 16 * 1B
    ubddr3_dma_size_write(3);            // byte
    ubddr3_dma_addr0_write((uint32_t)(base+3));
    ubddr3_dma_req_write(1); ubddr3_dma_req_write(0);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    off = mem_cmp_u8(base+3, exp, 16);
    if (off >= 0) { printf("8b/unaligned mismatch @+%d\n", off); return; }

    #ifdef CSR_UBDDR3_DMA_ERR_ADDR
    if (ubddr3_dma_err_read()) { puts("DMA error flag set!"); return; }
    #endif
    puts("ub_selftest: PASS");
    #else
    puts("No ubddr3 CSRs in this build.");
    #endif
}

/* ========================================================================= */
/*                 NEW: Downsampled capture (DSCAP) via UART                  */
/* ========================================================================= */
#ifdef CSR_DSCAP_BASE
static void dscap_help(char *args){
    (void)args;
    puts_help_header("Downsampled capture (uc->sys) via UART");
    puts("  dscap_on                  Enable capture (one sample per ce_down).");
    puts("  dscap_off                 Disable capture.");
    puts("  dscap_flush               Drain/clear FIFO and clear valid flag.");
    puts("  dscap_info                Show FIFO level flags, depth, dropped count.");
    puts("  dscap_read                Pop one sample and print as hex Y,X.");
    puts("  dscap_stream <N> [csv]    Stream N samples over UART.");
    puts("                               default: raw little-endian pairs (X16,Y16)");
    puts("                               csv    : text 'idx,x,y' per line");
    puts("");
}

static void cmd_dscap_on(char *a){ (void)a; dscap_enable_write(1); puts("dscap enabled"); }
static void cmd_dscap_off(char *a){ (void)a; dscap_enable_write(0); puts("dscap disabled"); }
static void cmd_dscap_flush(char *a){ (void)a; dscap_flush_write(1); dscap_flush_write(0); puts("dscap flushed"); }

static void cmd_dscap_info(char *a){
    (void)a;
    uint8_t  lvl     = dscap_level_read();   /* bit0=readable, bit1=writable */
    uint16_t depth   = dscap_depth_read();
    uint32_t dropped = dscap_dropped_read();
    printf("level: R=%u W=%u  depth=%u  dropped=%lu\n",
           (unsigned)(lvl & 1u), (unsigned)((lvl>>1)&1u),
           (unsigned)depth, (unsigned long)dropped);
}

/* Pop 1 sample and print hex Y,X for a quick sanity check */
static void cmd_dscap_read(char *a){
    (void)a;
    dscap_pop_write(1); dscap_pop_write(0);
    for (volatile unsigned i=0;i<100000;i++){
        if (dscap_valid_read()) {
            uint32_t w = dscap_data_read();
            uint16_t x = (uint16_t)(w & 0xffffu);
            uint16_t y = (uint16_t)(w >> 16);
            printf("Y=0x%04x  X=0x%04x\n", (unsigned)y, (unsigned)x);
            return;
        }
    }
    puts("No data (FIFO empty?).");
}

/* Stream N samples:
 *   - default: raw LE binary: X(int16 LE), Y(int16 LE) per sample + 0xDEADBEEF trailer
 *   - 'csv'   : prints lines: idx,x,y (signed) */
static void cmd_dscap_stream(char *args){
    char *tokN  = strtok(args, " \t");
    char *tokFmt= strtok(NULL, " \t");
    if (!tokN){ puts("Usage: dscap_stream <N> [csv]"); return; }

    uint32_t N   = (uint32_t)strtoul(tokN, NULL, 0);
    int      csv = (tokFmt && strcmp(tokFmt,"csv")==0);

    if (csv) puts("# idx,x,y");

    for (uint32_t i=0; i<N; i++){
        /* Wait until readable (low-speed) */
        unsigned tries=0;
        while (((dscap_level_read() & 1u)==0u) && tries < 200000) tries++;

        dscap_pop_write(1); dscap_pop_write(0);

        unsigned wait=0;
        while (!dscap_valid_read() && wait < 200000) wait++;

        if (!dscap_valid_read()){
            if (csv) {
                printf("%lu,,\n", (unsigned long)i);
            } else {
                uint16_t zero = 0;
                uart_write_u16_le(zero);
                uart_write_u16_le(zero);
            }
            continue;
        }

        uint32_t w = dscap_data_read();
        int16_t  x = (int16_t)(w & 0xffffu);
        int16_t  y = (int16_t)(w >> 16);

        if (csv) {
            printf("%lu,%d,%d\n", (unsigned long)i, (int)x, (int)y);
        } else {
            uart_write_u16_le((uint16_t)x);
            uart_write_u16_le((uint16_t)y);
        }
    }

    if (!csv) {
        const uint32_t end = 0xDEADBEEF;
        uart_write_u32_le(end);
    }
}
#endif /* CSR_DSCAP_BASE */

/* ---- Set CPU I/Q (X,Y) and commit ---- */
static void cmd_cpu_xy(char *args) {
    #ifdef CSR_MAIN_UPSAMPLER_INPUT_X_ADDR
    char *tx = strtok(args, " \t");
    char *ty = strtok(NULL, " \t");
    if (!tx || !ty) {
        puts("Usage: cpu_xy <x:int16> <y:int16>");
        return;
    }
    int x = (int)strtol(tx, NULL, 0);
    int y = (int)strtol(ty, NULL, 0);
    if (x < -32768) x = -32768; if (x > 32767) x = 32767;
    if (y < -32768) y = -32768; if (y > 32767) y = 32767;

    /* write raw 16-bit signed values */
    main_upsampler_input_x_write((uint16_t)x);
    main_upsampler_input_y_write((uint16_t)y);

    /* push to uc domain */
    #ifdef CSR_CFG_LINK_BASE
    cfg_link_commit_write(1);
    cfg_link_commit_write(0);
    #endif

    printf("CPU I/Q set: X=%d Y=%d\n", x, y);
    #else
    puts("Not built with MAIN_UPSAMPLER_INPUT_{X,Y} CSRs.");
    #endif
}


/* ========================================================================= */
/*                           Command registration                             */
/* ========================================================================= */

static const struct cmd_entry uc_tbl[] = {
    /* UberClock commands */
    {"help_uc",              uc_help,              "UberClock help"},
    {"phase_nco",            cmd_phase_nco,        "Set input CORDIC NCO phase increment"},
    {"phase_cpu",            cmd_phase_cpu,        "Set CPU CORDIC phase increment"},
    {"phase_down_1",         cmd_phase_down_1,     "Set downconversion ch1 phase inc"},
    {"phase_down_2",         cmd_phase_down_2,     "Set downconversion ch2 phase inc"},
    {"phase_down_3",         cmd_phase_down_3,     "Set downconversion ch3 phase inc"},
    {"phase_down_4",         cmd_phase_down_4,     "Set downconversion ch4 phase inc"},
    {"phase_down_5",         cmd_phase_down_5,     "Set downconversion ch5 phase inc"},
    {"output_select_ch1",    cmd_output_sel_ch1,   "Select DAC1 source (0..3)"},
    {"output_select_ch2",    cmd_output_sel_ch2,   "Select DAC2 source (0..3)"},
    {"input_select",         cmd_input_select,     "Set input select register"},
    {"upsampler_input_mux",  cmd_ups_in_mux,       "Set upsampler input mux"},
    {"gain1",                cmd_gain1,            "Set gain1"},
    {"gain2",                cmd_gain2,            "Set gain2"},
    {"gain3",                cmd_gain3,            "Set gain3"},
    {"gain4",                cmd_gain4,            "Set gain4"},
    {"gain5",                cmd_gain5,            "Set gain5"},
    {"final_shift",          cmd_final_shift,      "Set final shift"},
    {"phase",                cmd_phase_print,      "Print current CORDIC phase"},
    {"magnitude",            cmd_magnitude,        "Print current CORDIC magnitude"},
    {"cpu_xy",               cmd_cpu_xy,           "Set CPU input vector <x> <y> and commit"},

    /* UberDDR3 / S2MM commands */
    {"ub_help",              ub_help,              "UberDDR3/S2MM help"},
    {"ub_info",              cmd_ub_info,          "Show UBDDR3 info/state"},
    {"ub_ramp",              cmd_ub_ramp,          "Start ramp S2MM to DDR"},
    {"ub_wait",              cmd_ub_wait,          "Wait until DMA done"},
    {"ub_hexdump",           cmd_ub_hexdump,       "Hexdump DDR memory"},
    {"ub_selftest",          cmd_ub_selftest,      "Run DMA alignment/size selftest"},

    /* NEW: Downsampled capture via UART */
    #ifdef CSR_DSCAP_BASE
    {"dscap_help",           dscap_help,           "Downsampled capture help"},
    {"dscap_on",             cmd_dscap_on,         "Enable capture"},
    {"dscap_off",            cmd_dscap_off,        "Disable capture"},
    {"dscap_flush",          cmd_dscap_flush,      "Flush/clear FIFO"},
    {"dscap_info",           cmd_dscap_info,       "Show FIFO flags/depth/drops"},
    {"dscap_read",           cmd_dscap_read,       "Pop one sample and print hex"},
    {"dscap_stream",         cmd_dscap_stream,     "Stream N samples [csv]"},
    #endif
};

void uberclock_register_cmds(void) {
    console_register(uc_tbl, (unsigned)(sizeof(uc_tbl)/sizeof(uc_tbl[0])));
}

/* ========================================================================= */
/*                            Init / poll functions                           */
/* ========================================================================= */

void uberclock_init(void) {
    /* Defaults for UberClock path */
    #ifdef CSR_MAIN_PHASE_INC_NCO_ADDR
    main_phase_inc_nco_write(80660);
    main_phase_inc_down_1_write(80656); // 500 Hz
    main_phase_inc_down_2_write(80652); // 1000 Hz
    main_phase_inc_down_3_write(80648); // 1500 Hz
    main_phase_inc_down_4_write(80644); // 2000 Hz
    main_phase_inc_down_5_write(80640); // 2500 Hz
    main_phase_inc_cpu_write(52429);
    main_input_select_write(0);
    main_upsampler_input_mux_write(0);
    main_gain1_write (0x40000000);
    main_gain2_write (0x40000000);
    main_gain3_write (0x40000000);
    main_gain4_write (0x40000000);
    main_gain5_write (0x40000000);
    main_output_select_ch1_write(3);
    main_output_select_ch2_write(3);
    main_final_shift_write(2);
    #endif

    #ifdef CSR_EVM_PENDING_ADDR
    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));
    #endif

    /* Defaults for UBDDR3 ramp/S2MM path */
    #ifdef CSR_UBDDR3_BASE
    #ifdef CSR_UBDDR3_RAMP_LEN_ADDR
    ubddr3_ramp_len_write(0);
    #endif
    #ifdef CSR_UBDDR3_RAMP_SEED_ADDR
    ubddr3_ramp_seed_write(0);
    #endif
    #ifdef CSR_UBDDR3_DMA_INC_ADDR
    ubddr3_dma_inc_write(1);
    #endif
    #ifdef CSR_UBDDR3_DMA_SIZE_ADDR
    ubddr3_dma_size_write(0); /* bus width */
    #endif
    #ifdef CSR_UBDDR3_DMA_REQ_ADDR
    ubddr3_dma_req_write(0);
    #endif
    #endif

    /* NEW: dscap defaults */
    #ifdef CSR_DSCAP_BASE
    dscap_enable_write(0);
    dscap_flush_write(1); dscap_flush_write(0);
    #endif

    printf("UberClock init done.\n");
}

void uberclock_poll(void) {
    #ifdef CSR_EVM_PENDING_ADDR
    if (!ce_event) return;

    // Example (if exposed): g_mag   = main_magnitude_read();
    //                        g_phase = main_phase_read();

    ce_event = 0;
    evm_pending_write(1);
    evm_enable_write(1);
    #endif
}
