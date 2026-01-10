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
#include "libliteeth/udp.h"   // <-- your UDP stack header

#if defined(__has_include)
#if __has_include(<libbase/cache.h>)
#include <libbase/cache.h>
static inline void ub_cache_sync(void) { flush_cpu_dcache(); flush_l2_cache(); }
#else
static inline void ub_cache_sync(void) { /* no-op */ }
#endif
#else
static inline void ub_cache_sync(void) { /* no-op */ }
#endif

/* ========================================================================= */
/*                             UberClock                                     */
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
static inline void uc_commit(void) {
    #ifdef CSR_CFG_LINK_BASE
    cfg_link_commit_write(1);
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
    puts("  cap_enable   <0|1>      (0=ramp->DDR, 1=capture design->DDR)");
    puts("  cap_beats    <N>        (# of 256-bit beats captured by the gateware)");
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
        default: return;
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
    printf("final_shift set to %ld (0x%08lX)\n", (long)fs, (unsigned long)fs);
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}

static void cmd_cap_enable(char *a) {
    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
    #else
    puts("Not built with UberClock CSRs.");
    #endif
}

static void cmd_phase_print(char *a){ (void)a; printf("Phase %ld\n", (long)g_phase); }
static void cmd_magnitude  (char *a){ (void)a; printf("Magnitude %d\n", g_mag); }

/* ========================================================================= */
/*                     UberDDR3 + S2MM (capture-to-DDR) CLI                   */
/* ========================================================================= */

static inline uint8_t ub_size_to_code(const char *s) {
    if (!s) return 0;             // 00 = bus width
    if (!strcmp(s,"bus")) return 0;
    if (!strcmp(s,"32"))  return 1;
    if (!strcmp(s,"16"))  return 2;
    if (!strcmp(s,"8"))   return 3;
    return 0;
}

static void ub_help(char *args) {
    (void)args;
    puts_help_header("UberDDR3/S2MM commands");
    puts("  ub_info");
    puts("      Print DDR calibration state and CSR base.");
    puts("  ub_mode");
    puts("      Print current capture mode (cap_enable).");
    puts("  ub_setmode <0|1>");
    puts("      Set cap_enable (0=ramp, 1=capture design) and commit.");
    puts("  ub_ramp <addr_hex> [beats] [size]");
    puts("      FORCE ramp mode (cap_enable=0), then start S2MM into DDR.");
    puts("  ub_cap  <addr_hex> [beats] [size]");
    puts("      FORCE capture mode (cap_enable=1), then start S2MM into DDR.");
    puts("  ub_start <addr_hex> [beats] [size]");
    puts("      Start S2MM using CURRENT cap_enable mode.");
    puts("  ub_wait");
    puts("      Poll until DMA not busy; flush caches; print error if any.");
    puts("  ub_hexdump <addr_hex> <bytes>");
    puts("      Dump memory to verify write.");
    puts("  ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
    puts("      Send DDR memory region via UDP to PC.");
    puts("      Example: ub_send 0xA0000000 8192 192.168.0.2 5000");
    puts("");
}

static void cmd_ub_info(char *a) {
    (void)a;
    #ifdef CSR_UBDDR3_BASE
    int cal = 0;
    #ifdef CSR_UBDDR3_CALIB_DONE_ADDR
    cal = ubddr3_calib_done_read();
    #endif
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d",
           (unsigned long)CSR_UBDDR3_BASE, cal);
    #ifdef UBDDR3_MEM_BASE
    printf("  (UBDDR3_MEM_BASE: 0x%08lx)\n", (unsigned long)UBDDR3_MEM_BASE);
    #else
    printf("  (UBDDR3_MEM_BASE: <not defined>)\n");
    #endif
    #else
    puts("No ubddr3 CSRs in this build.");
    #endif
}

static void cmd_ub_mode(char *a) {
    (void)a;
    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    unsigned v = main_cap_enable_read() & 1u;
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
    #else
    puts("cap_enable CSR not present.");
    #endif
}

static void cmd_ub_setmode(char *a) {
    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
    #else
    puts("cap_enable CSR not present.");
    #endif
}

/* Core DMA start helper used by ub_ramp/ub_cap/ub_start */
static void ub_dma_start(uint64_t addr, uint32_t beats, uint8_t size_code) {
    #ifdef CSR_UBDDR3_BASE
    (void)beats;
    (void)size_code;

    #ifdef CSR_UBDDR3_DMA_INC_ADDR
    ubddr3_dma_inc_write(1);
    #endif
    #ifdef CSR_UBDDR3_DMA_SIZE_ADDR
    ubddr3_dma_size_write(size_code);
    #endif
    #ifdef CSR_UBDDR3_DMA_ADDR0_ADDR
    ubddr3_dma_addr0_write((uint32_t)(addr & 0xffffffffu));
    #endif
    #ifdef CSR_UBDDR3_DMA_ADDR1_ADDR
    ubddr3_dma_addr1_write((uint32_t)(addr >> 32));
    #endif

    #ifdef CSR_UBDDR3_RAMP_LEN_ADDR
    ubddr3_ramp_len_write(beats);
    #endif

    #ifdef CSR_UBDDR3_DMA_REQ_ADDR
    ubddr3_dma_req_write(1);
    #else
    puts("ubddr3_dma_req CSR not present.");
    #endif

    #else
    puts("No ubddr3 CSRs in this build.");
    (void)addr; (void)beats; (void)size_code;
    #endif
}

/* ub_start: run DMA using current cap_enable mode */
static void cmd_ub_start(char *args) {
    #ifdef CSR_UBDDR3_BASE
    char *p = args;
    char *tok_addr  = strtok(p, " \t");
    char *tok_beats = strtok(NULL, " \t");
    char *tok_size  = strtok(NULL, " \t");

    if (!tok_addr) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    uint64_t addr  = strtoull(tok_addr, NULL, 0);
    uint32_t beats = (uint32_t)(tok_beats ? strtoul(tok_beats, NULL, 0) : 256);
    uint8_t  sz    = ub_size_to_code(tok_size);

    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    unsigned mode = main_cap_enable_read() & 1u;
    #else
    unsigned mode = 0;
    #endif

    printf("S2MM start: mode=%s addr=0x%08lx_%08lx beats=%u size=%s\n",
           mode ? "CAPTURE" : "RAMP",
           (unsigned long)(addr >> 32), (unsigned long)(addr & 0xffffffffu),
           (unsigned)beats,
           (sz==0)?"bus":(sz==1)?"32":(sz==2)?"16":"8");


    #ifdef CSR_MAIN_CAP_BEATS_ADDR
    main_cap_beats_write(beats);
    uc_commit();
    #endif
    ub_dma_start(addr, beats, sz);
    #else
    (void)args;
    puts("No ubddr3 CSRs in this build.");
    #endif
}

/* ub_ramp: force ramp mode then start */
static void cmd_ub_ramp2(char *args) {
    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    main_cap_enable_write(0);
    uc_commit();
    #endif
    cmd_ub_start(args);
}

/* ub_cap: force capture mode then start */
static void cmd_ub_cap(char *args) {
    #ifdef CSR_MAIN_CAP_ENABLE_ADDR
    main_cap_enable_write(1);
    uc_commit();
    #endif
    cmd_ub_start(args);
}

static void cmd_ub_wait(char *a) {
    (void)a;
    #ifdef CSR_UBDDR3_BASE
    #ifdef CSR_UBDDR3_DMA_BUSY_ADDR
    printf("Waiting for DMA ... "); fflush(stdout);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
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
        if ((i & 0x0f) == 0)
            printf("\n%08lx: ", (unsigned long)((addr + i) & 0xffffffffu));
        printf("%02x ", p[i]);
    }
    puts("");
    #else
    puts("UBDDR3_MEM_BASE not defined; cannot hexdump.");
    #endif
}

static void cmd_cap_beats(char *a) {
    #ifdef CSR_MAIN_CAP_BEATS_ADDR
    uint32_t v = (uint32_t)strtoul(a ? a : "256", NULL, 0);
    if (v == 0) { puts("cap_beats must be >= 1"); return; }
    main_cap_beats_write(v);
    uc_commit();
    printf("cap_beats = %u\n", (unsigned)v);
    #else
    puts("cap_beats CSR not present.");
    #endif
}

/* ========================================================================= */
/*                           UDP DDR streamer                                 */
/* ========================================================================= */

#ifndef UBD3_MAGIC
#define UBD3_MAGIC 0x55424433u /* "UBD3" */
#endif

struct __attribute__((packed)) ubd3_hdr {
    uint32_t magic;
    uint32_t seq;
    uint32_t offset;
    uint32_t total;
};

/* Your board IP (change if you want) */
#ifndef UBD3_BOARD_IP
#define UBD3_BOARD_IP IPTOINT(192,168,0,123)
#endif

/* Keep payload below MTU. 1400 is safe for most stacks. */
#ifndef UBD3_PAYLOAD_MAX
#define UBD3_PAYLOAD_MAX 1400u
#endif

static int parse_ipv4(const char *s, uint32_t *out_ip) {
    unsigned a,b,c,d;
    if (!s) return -1;
    if (sscanf(s, "%u.%u.%u.%u", &a,&b,&c,&d) != 4) return -1;
    if (a>255 || b>255 || c>255 || d>255) return -1;
    *out_ip = IPTOINT(a,b,c,d);
    return 0;
}

static void cmd_ub_send(char *args) {
    char *tok_addr = strtok(args, " \t");
    char *tok_len  = strtok(NULL, " \t");
    char *tok_ip   = strtok(NULL, " \t");
    char *tok_port = strtok(NULL, " \t");

    if (!tok_addr || !tok_len || !tok_ip || !tok_port) {
        puts("Usage: ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
        puts("Example: ub_send 0xA0000000 8192 192.168.0.2 5000");
        return;
    }

    uint64_t addr  = strtoull(tok_addr, NULL, 0);
    uint32_t total = (uint32_t)strtoul(tok_len, NULL, 0);

    uint32_t dst_ip = 0;
    if (parse_ipv4(tok_ip, &dst_ip) != 0) {
        puts("Error: bad dst_ip format (use a.b.c.d)");
        return;
    }

    uint16_t dst_port = (uint16_t)strtoul(tok_port, NULL, 0);
    uint16_t src_port = dst_port; /* simplest */

    if (total == 0) {
        puts("Error: bytes must be > 0");
        return;
    }

    printf("UDP send: addr=0x%08lx_%08lx bytes=%lu dst=%s:%u\n",
           (unsigned long)(addr >> 32),
           (unsigned long)(addr & 0xffffffffu),
           (unsigned long)total,
           tok_ip, (unsigned)dst_port);

    static const unsigned char board_mac[6] = {0x02,0x00,0x00,0x00,0x00,0xAB};

    eth_init();
    udp_set_mac(board_mac);
    udp_set_ip(UBD3_BOARD_IP);
    udp_start(board_mac, UBD3_BOARD_IP);

    /* Wait for ARP to resolve (many tiny stacks require this) */
    printf("ARP resolve %u.%u.%u.%u ... ",
           (dst_ip>>24)&255, (dst_ip>>16)&255, (dst_ip>>8)&255, (dst_ip>>0)&255);
    fflush(stdout);

    int ok = 0;
    for (unsigned i = 0; i < 500000; i++) {
        udp_service();
        if (udp_arp_resolve(dst_ip) != 0) { ok = 1; break; }   /* adjust if your API uses !=0 for success */
    }
    puts(ok ? "ok" : "FAILED");
    if (!ok) {
        puts("No ARP reply. Check link, IPs, subnet, and PC firewall.");
        return;
    }


    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;

    uint32_t sent = 0;
    uint32_t seq  = 0;

    const uint32_t hdr_sz = (uint32_t)sizeof(struct ubd3_hdr);
    const uint32_t max_data = (UBD3_PAYLOAD_MAX > hdr_sz) ? (UBD3_PAYLOAD_MAX - hdr_sz) : 0;

    if (max_data < 64) {
        puts("Error: UBD3_PAYLOAD_MAX too small");
        return;
    }

    while (sent < total) {
        /* keep the network stack alive */
        udp_service();

        uint32_t chunk = total - sent;
        if (chunk > max_data) chunk = max_data;

        uint8_t *tx = (uint8_t*)udp_get_tx_buffer();
        if (!tx) {
            puts("udp_get_tx_buffer() returned NULL");
            return;
        }

        struct ubd3_hdr h;
        h.magic  = UBD3_MAGIC;
        h.seq    = seq;
        h.offset = sent;
        h.total  = total;

        memcpy(tx, &h, hdr_sz);
        memcpy(tx + hdr_sz, (const void*)(p + sent), chunk);

        int rc = udp_send(src_port, dst_port, (unsigned)(hdr_sz + chunk));
        (void)rc;

        sent += chunk;
        seq++;

        /* Optional: print progress occasionally */
        if ((seq & 0x3ffu) == 0) {
            printf("  sent %lu / %lu bytes\n", (unsigned long)sent, (unsigned long)total);
        }
    }

    printf("ub_send done: %lu bytes in %lu packets\n",
           (unsigned long)total, (unsigned long)seq);
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
    {"cap_enable",           cmd_cap_enable,       "0=ramp, 1=capture design to DDR"},
    {"phase",                cmd_phase_print,      "Print current CORDIC phase"},
    {"magnitude",            cmd_magnitude,        "Print current CORDIC magnitude"},

    /* UberDDR3 / S2MM commands */
    {"ub_help",              ub_help,              "UberDDR3/S2MM help"},
    {"ub_info",              cmd_ub_info,          "Show UBDDR3 info/state"},
    {"ub_mode",              cmd_ub_mode,          "Show current cap_enable mode"},
    {"ub_setmode",           cmd_ub_setmode,       "Set cap_enable (0=ramp,1=capture)"},
    {"ub_start",             cmd_ub_start,         "Start S2MM using current mode"},
    {"ub_ramp",              cmd_ub_ramp2,         "Force ramp mode then start S2MM"},
    {"ub_cap",               cmd_ub_cap,           "Force capture mode then start S2MM"},
    {"ub_wait",              cmd_ub_wait,          "Wait until DMA done"},
    {"ub_hexdump",           cmd_ub_hexdump,       "Hexdump DDR memory"},
    {"cap_beats",            cmd_cap_beats,        "Set capture length in 256-bit beats"},
    {"ub_send",              cmd_ub_send,          "Send DDR memory region via UDP" },
};

void uberclock_register_cmds(void) {
    console_register(uc_tbl, (unsigned)(sizeof(uc_tbl)/sizeof(uc_tbl[0])));
}

/* ========================================================================= */
/*                            Init / poll functions                           */
/* ========================================================================= */

void uberclock_init(void) {
    #ifdef CSR_MAIN_PHASE_INC_NCO_ADDR
    main_phase_inc_nco_write(80660);
    main_phase_inc_down_1_write(80656);
    main_phase_inc_down_2_write(80652);
    main_phase_inc_down_3_write(80648);
    main_phase_inc_down_4_write(80644);
    main_phase_inc_down_5_write(80640);
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

    /* Default: ramp mode (safe, deterministic) */
    main_cap_enable_write(0);
    uc_commit();
    #endif

    #ifdef CSR_EVM_PENDING_ADDR
    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));
    #endif

    printf("UberClock init done.\n");
}

void uberclock_poll(void) {
    #ifdef CSR_EVM_PENDING_ADDR
    if (!ce_event) return;

    /* If you later expose magnitude/phase CSRs, read them here. */

    ce_event = 0;
    evm_pending_write(1);
    evm_enable_write(1);
    #endif
}
