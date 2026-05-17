#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <generated/soc.h>
#include "console.h"
#include "ubddr3.h"

/* ===== UberDDR3 bring-up helpers ===== */

/* ---- Clock Hz ---- */
#if defined(CONFIG_CLOCK_FREQUENCY)
#  define CLK_HZ CONFIG_CLOCK_FREQUENCY
#elif defined(SYSTEM_CLOCK_FREQUENCY)
#  define CLK_HZ SYSTEM_CLOCK_FREQUENCY
#else
#  define CLK_HZ 0
#endif

#ifndef UBDDR3_MEM_BASE
#  define UBDDR3_MEM_BASE 0xA0000000u
#endif

/* ---- timers: prefer TIMER1, else rdcycle, else TIMER0 ---- */
#if defined(CSR_TIMER1_BASE)
static inline void t_start(void){
    timer1_en_write(0);
    timer1_reload_write(0);
    timer1_load_write(0xFFFFFFFFu);
    timer1_en_write(1);
}
static inline uint32_t t_lap_ticks(void){
    timer1_update_value_write(1);
    return 0xFFFFFFFFu - timer1_value_read();
}
static inline const char* t_source(void){ return "TIMER1"; }
#elif defined(__riscv)
static uint64_t rdcycle64(void){
    #if (__riscv_xlen == 64)
    uint64_t x; __asm__ volatile("rdcycle %0":"=r"(x)); return x;
    #else
    uint32_t hi, lo, hi2;
    __asm__ volatile(
        "1:\n"
        "  rdcycleh %0\n"
        "  rdcycle  %1\n"
        "  rdcycleh %2\n"
        "  bne      %0, %2, 1b\n"
        : "=&r"(hi), "=&r"(lo), "=&r"(hi2));
    return ((uint64_t)hi<<32)|lo;
    #endif
}
static uint64_t g_t0;
static inline void t_start(void){ g_t0 = rdcycle64(); }
static inline uint32_t t_lap_ticks(void){
    uint64_t d = rdcycle64() - g_t0;
    return (uint32_t)(d > 0xFFFFFFFFu ? 0xFFFFFFFFu : d);
}
static inline const char* t_source(void){ return "RDCYCLE"; }
#elif defined(CSR_TIMER0_BASE)
static inline void t_start(void){
    timer0_en_write(0);
    timer0_reload_write(0);
    timer0_load_write(0xFFFFFFFFu);
    timer0_en_write(1);
}
static inline uint32_t t_lap_ticks(void){
    timer0_update_value_write(1);
    return 0xFFFFFFFFu - timer0_value_read();
}
static inline const char* t_source(void){ return "TIMER0"; }
#else
static inline void t_start(void){}
static inline uint32_t t_lap_ticks(void){ return 0; }
static inline const char* t_source(void){ return "NONE"; }
#endif

static inline uint32_t ticks_to_us(uint32_t t){
    #if CLK_HZ == 0
    (void)t; return 0;
    #else
    return (uint32_t)((uint64_t)t * 1000000ull / (uint64_t)CLK_HZ);
    #endif
}

static inline void membar(void){ __asm__ volatile("" ::: "memory"); }

/* ---- DDR calib helpers ---- */
static int ddr_calibrated(void){
    #ifdef CSR_UBDDR3_CALIB_DONE_ADDR
    return (ubddr3_calib_done_read() & 1);
    #else
    return 1;
    #endif
}
static int ddr_wait_calib(unsigned max_polls){
    #ifdef CSR_UBDDR3_CALIB_DONE_ADDR
    while (max_polls--) {
        if (ddr_calibrated()) return 1;
        for (volatile int i = 0; i < 50000; i++) __asm__ volatile("" ::: "memory");
    }
    return 0;
    #else
    (void)max_polls; return 1;
    #endif
}

/* ---- patterns ---- */
typedef uint32_t (*pat32_fn)(uintptr_t base, uint32_t i, uint32_t seed);
static uint32_t pat_const(uintptr_t b, uint32_t i, uint32_t s){ (void)b;(void)i; return s; }
static uint32_t pat_xor  (uintptr_t b, uint32_t i, uint32_t s){ (void)b; return s ^ i; }
static uint32_t pat_addr (uintptr_t b, uint32_t i, uint32_t s){ (void)s; return (uint32_t)(b + 4u*i); }

static uint32_t xorshift32(uint32_t *st){
    uint32_t x = *st;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *st = x ? x : 1u;
    return *st;
}
static uint32_t pat_prbs (uintptr_t b, uint32_t i, uint32_t s){
    (void)b; static uint32_t st = 1u; if (i==0) st = s ? s : 1u; return xorshift32(&st);
}
static uint32_t pat_walk1(uintptr_t b, uint32_t i, uint32_t s){ (void)b;(void)s; return 1u << (i & 31); }
static uint32_t pat_walk0(uintptr_t b, uint32_t i, uint32_t s){ return ~pat_walk1(b,i,s); }

static pat32_fn pick_pattern(const char* name, const char **pretty, uint32_t *def_seed){
    *def_seed = 0;
    if (!name || !*name || !strcmp(name,"a5xor")){ *pretty="A5A5 XOR index"; *def_seed=0xA5A50000u; return pat_xor; }
    if (!strcmp(name,"const")) { *pretty="Constant 32-bit value"; return pat_const; }
    if (!strcmp(name,"addr"))  { *pretty="Physical address (32b)"; return pat_addr; }
    if (!strcmp(name,"prbs"))  { *pretty="PRBS (xorshift32)"; *def_seed=1u; return pat_prbs; }
    if (!strcmp(name,"walk1")) { *pretty="Walking 1s"; return pat_walk1; }
    if (!strcmp(name,"walk0")) { *pretty="Walking 0s"; return pat_walk0; }
    *pretty="A5A5 XOR index"; *def_seed=0xA5A50000u; return pat_xor;
}

/* ---- progress (coarse) ---- */
#ifndef PROGRESS_WORD_STEP
#define PROGRESS_WORD_STEP (1u<<12)
#endif
#ifndef PROGRESS_BYTE_STEP
#define PROGRESS_BYTE_STEP (1u<<14)
#endif

static void progress_line(const char *phase, unsigned pct, char frame){
    printf("\r%s %3u%% %c", phase, pct, frame);
    fflush(stdout);
}
static void progress_done(void){
    printf("\r%*s\r", 20, "");
    fflush(stdout);
    #ifdef CSR_LEDS_BASE
    leds_out_write(0);
    #endif
}
static inline void progress_update_words(const char *phase, uint32_t i, uint32_t total){
    static const char frames[]="|/-\\";
    static uint8_t f=0;
    if (!total) return;
    if ((i % PROGRESS_WORD_STEP)!=0 && i!=0) return;
    unsigned pct=(unsigned)((uint64_t)i*100ull/total);
    progress_line(phase,pct,frames[(f++)&3]);
    #ifdef CSR_LEDS_BASE
    leds_out_write(1u << (f & 3));
    #endif
}
static inline void progress_update_bytes(const char *phase, uint32_t i, uint32_t total){
    static const char frames[]="|/-\\";
    static uint8_t f=0;
    if (!total) return;
    if ((i % PROGRESS_BYTE_STEP)!=0 && i!=0) return;
    unsigned pct=(unsigned)((uint64_t)i*100ull/total);
    progress_line(phase,pct,frames[(f++)&3]);
    #ifdef CSR_LEDS_BASE
    leds_out_write(1u << (f & 3));
    #endif
}

/* ---- utilities ---- */
static unsigned parse_kib(const char* s, unsigned def_kib){
    if (!s || !*s) return def_kib;
    char *end=NULL; unsigned long v=strtoul(s,&end,0);
    if (end && *end){
        if (*end=='K'||*end=='k') return (unsigned)v;
        if (*end=='M'||*end=='m') return (unsigned)(v*1024u);
        if (*end=='G'||*end=='g') return (unsigned)(v*1024u*1024u);
    }
    return (unsigned)v;
}

/* ---- command implementations ---- */

static void cmd_help_ddr(char *a){
    (void)a;
    puts_help_header("DDR commands");
    puts("  ddrinfo             - Print DDR base + calib CSR state");
    puts("  ddrwait             - Wait for UberDDR3 calibration to complete");
    puts("  ddrprobe            - One 32-bit store/load at base");
    puts("  ddrbyte             - Byte-lane sanity (0..31 at base)");
    puts("  ddrtest  [KiB]      - 32-bit test, pattern=A5A5^index (default 4 KiB)");
    puts("  ddrtestb [KiB]      - Byte test,  pattern=A5^index     (default 4 KiB)");
    puts("  ddrpat   [size] [pattern] [seed]  - Pattern test:");
    puts("       patterns: a5xor(default) | const | addr | prbs | walk1 | walk0");
    puts("       size suffix: K/M/G (e.g., 4M). Seed is optional.");
    puts("  timertest           - 100 ms sanity (prints source, ticks, us)");
    puts("  timeinfo            - Show timing source and CLK_HZ");
}

static void cmd_ddrinfo(char *a){
    (void)a;
    puts_help_header("DDR Info");
    #ifdef MAIN_RAM_BASE
    printf("MAIN_RAM_BASE: 0x%08lx\n", (unsigned long)MAIN_RAM_BASE);
    #endif
    #ifdef CSR_UBDDR3_CALIB_DONE_ADDR
    printf("UBDDR3 calib CSR present: %u\n", (unsigned)ubddr3_calib_done_read());
    #else
    puts("UBDDR3 calib CSR not present (assuming ready).");
    #endif
}

static void cmd_ddrwait(char *a){
    (void)a;
    puts_help_header("DDR Calibration");
    printf("Waiting for DDR calibration... ");
    if (ddr_wait_calib(10000)) puts("\e[32;1mOK\e[0m");
    else                       puts("\e[31;1mTIMEOUT\e[0m");
}

static void cmd_ddrprobe(char *a){
    (void)a;
    puts_help_header("DDR 32-bit Probe");
    volatile uint32_t *p=(volatile uint32_t *)UBDDR3_MEM_BASE;
    p[0]=0x11223344; membar();
    printf("Wrote  0x11223344 @ %p\n",(void*)&p[0]);
    printf("Read   0x%08x\n", p[0]);
}

static void cmd_ddrbyte(char *a){
    (void)a;
    puts_help_header("DDR Byte-Lane Sanity (first 32 bytes)");
    if (!ddr_wait_calib(10000)){ puts("\e[31;1mCalibration TIMEOUT\e[0m"); return; }
    volatile uint8_t *b=(volatile uint8_t *)UBDDR3_MEM_BASE;
    for (int i=0;i<32;i++) b[i]=i;
    membar();
    for (int i=0;i<32;i++) printf("%02x%s", b[i], (i%16==15) ? "\n":" ");
}

static void run_ddr_test32(uint32_t kib, pat32_fn fn, const char* pname, uint32_t seed){
    if (!ddr_wait_calib(10000)){ puts("\e[31;1mCalibration TIMEOUT\e[0m"); return; }
    const uintptr_t base=(uintptr_t)UBDDR3_MEM_BASE;
    volatile uint32_t *p=(volatile uint32_t *)base;
    const uint32_t words=(kib*1024u)/4u;
    uint32_t errs=0, shown=0, show_max=12;

    printf("\e[33;1mTest:\e[0m %s  | \e[33;1mBase:\e[0m 0x%08lx  | \e[33;1mSize:\e[0m %u KiB  | \e[33;1mSeed:\e[0m 0x%08x\n",
           pname, (unsigned long)base, kib, seed);

    t_start();
    progress_update_words("Write ",0,words);
    for (uint32_t i=0;i<words;i++){
        p[i]=fn(base,i,seed);
        progress_update_words("Write ",i,words);
    }
    membar();
    uint32_t t_fill_us=ticks_to_us(t_lap_ticks());
    progress_done();

    t_start();
    progress_update_words("Verify",0,words);
    for (uint32_t i=0;i<words;i++){
        uint32_t exp=fn(base,i,seed);
        uint32_t got=p[i];
        if (got!=exp){
            if (shown++<show_max) printf("\e[31;1mERR\e[0m @%p exp=%08x got=%08x\n",(void*)&p[i],exp,got);
            ++errs;
        }
        progress_update_words("Verify",i,words);
    }
    uint32_t t_verify_us=ticks_to_us(t_lap_ticks());
    progress_done();

    const uint32_t bytes=words*4u;
    if (errs==0){
        printf("\e[32;1mPASS\e[0m — wrote %lu bytes in %u us, verified in %u us\n",
               (unsigned long)bytes, t_fill_us, t_verify_us);
    } else {
        printf("\e[31;1mFAIL\e[0m — %u mismatches over %lu bytes\n",
               errs, (unsigned long)bytes);
    }
}

static void cmd_ddrtest(char *args){
    unsigned kib = parse_kib(args && *args ? get_token(&args) : NULL, 4);
    puts_help_header("DDR 32-bit Test (A5A5^index)");
    run_ddr_test32(kib, pat_xor, "A5A5 XOR index", 0xA5A50000u);
}

static void cmd_ddrtestb(char *args){
    unsigned kib = parse_kib(args && *args ? get_token(&args) : NULL, 4);
    puts_help_header("DDR Byte Test (A5^index)");
    if (!ddr_wait_calib(10000)){ puts("\e[31;1mCalibration TIMEOUT\e[0m"); return; }

    volatile uint8_t *b=(volatile uint8_t *)UBDDR3_MEM_BASE;
    const uint32_t bytes=kib*1024u;
    uint32_t errs=0, shown=0, show_max=16;

    t_start();
    progress_update_bytes("Write ",0,bytes);
    for (uint32_t i=0;i<bytes;i++){
        b[i]=(uint8_t)(i ^ 0xA5);
        progress_update_bytes("Write ",i,bytes);
    }
    membar();
    uint32_t t_fill_us=ticks_to_us(t_lap_ticks());
    progress_done();

    t_start();
    progress_update_bytes("Verify",0,bytes);
    for (uint32_t i=0;i<bytes;i++){
        uint8_t exp=(uint8_t)(i ^ 0xA5), got=b[i];
        if (got!=exp){
            if (shown++<show_max) printf("\e[31;1mERR\e[0m @%p exp=%02x got=%02x\n",(void*)&b[i],exp,got);
            ++errs;
        }
        progress_update_bytes("Verify",i,bytes);
    }
    uint32_t t_verify_us=ticks_to_us(t_lap_ticks());
    progress_done();

    if (errs==0){
        printf("\e[32;1mPASS\e[0m — wrote %lu bytes in %u us, verified in %u us\n",
               (unsigned long)bytes, t_fill_us, t_verify_us);
    } else {
        printf("\e[31;1mFAIL\e[0m — %u mismatches over %lu bytes\n",
               errs, (unsigned long)bytes);
    }
}

static void cmd_ddrmap(char *a){
    (void)a;
    puts_help_header("DDR Lane Map (one 256-bit beat @ base)");
    volatile uint32_t *w = (volatile uint32_t *)UBDDR3_MEM_BASE;
    for (int i=0;i<8;i++) w[i]=0;
    membar();

    uint32_t tag[8] = {0x11111111,0x22222222,0x33333333,0x44444444,
        0x55555555,0x66666666,0x77777777,0x88888888};
        for (int i=0;i<8;i++){ w[i]=tag[i]; membar(); }

        uint32_t rd[8];
        for (int i=0;i<8;i++) rd[i]=w[i];

        printf("Wrote lanes:  [0..7] = 11 22 33 44 55 66 77 88\n");
    printf("Read lanes :  [0..7] = ");
    for (int i=0;i<8;i++) printf("%02x ", (unsigned)(rd[i]>>28));
    printf("\n");

    volatile uint32_t *w2 = (volatile uint32_t *)(UBDDR3_MEM_BASE + 32);
    for (int i=0;i<8;i++) w2[i]=0;
    membar();
    for (int i=0;i<8;i++){ w2[i]=tag[i]; membar(); }
    for (int i=0;i<8;i++) rd[i]=w2[i];
    printf("Read lanes+1: [0..7] = ");
    for (int i=0;i<8;i++) printf("%02x ", (unsigned)(rd[i]>>28));
    printf("\n");
}


static void cmd_ddrpat(char *args){
    unsigned kib=4;
    const char *pname=NULL, *pretty=NULL;
    pat32_fn fn; uint32_t seed, def_seed;
    char *tok=NULL;

    if (args && *args){ tok=get_token(&args); if (tok && *tok) kib = parse_kib(tok,4); }
    if (args && *args){ tok=get_token(&args); if (tok && *tok) pname = tok; }

    fn = pick_pattern(pname, &pretty, &def_seed);
    seed = def_seed;

    if (args && *args){ tok=get_token(&args); if (tok && *tok) seed = (uint32_t)strtoul(tok,NULL,0); }

    puts_help_header("DDR Pattern Test");
    run_ddr_test32(kib, fn, pretty, seed);
}

static void cmd_timertest(char *a){
    (void)a;
    puts_help_header("Timer sanity");
    const unsigned sleep_ms=100;
    t_start();
    busy_wait(sleep_ms);
    uint32_t ticks=t_lap_ticks();
    uint32_t us=ticks_to_us(ticks);
    printf("source=%s  CLK_HZ=%u  ticks=%u  -> ~%u us (expected ~100000 us)\n",
           t_source(), (unsigned)CLK_HZ, ticks, us);
}

static void cmd_timeinfo(char *a){
    (void)a;
    puts_help_header("Timer info");
    printf("Timing source: %s\n", t_source());
    printf("CLK_HZ      : %u\n", (unsigned)CLK_HZ);
    #if defined(CSR_TIMER1_BASE)
    printf("TIMER1 base : 0x%08lx\n", (unsigned long)CSR_TIMER1_BASE);
    #endif
    #if defined(CSR_TIMER0_BASE)
    printf("TIMER0 base : 0x%08lx\n", (unsigned long)CSR_TIMER0_BASE);
    #endif
}

static const struct cmd_entry g_ddr_cmds[] = {
    { "help_ddr", cmd_help_ddr },
    { "ddrinfo",  cmd_ddrinfo  },
    { "ddrwait",  cmd_ddrwait  },
    { "ddrprobe", cmd_ddrprobe },
    { "ddrbyte",  cmd_ddrbyte  },
    { "ddrtest",  cmd_ddrtest  },
    { "ddrtestb", cmd_ddrtestb },
    { "ddrmap",   cmd_ddrmap  },
    { "ddrpat",   cmd_ddrpat   },
    { "timertest",cmd_timertest},
    { "timeinfo", cmd_timeinfo },
};

void ubddr3_register_cmds(void){
    console_register(g_ddr_cmds, sizeof(g_ddr_cmds)/sizeof(g_ddr_cmds[0]));
}
