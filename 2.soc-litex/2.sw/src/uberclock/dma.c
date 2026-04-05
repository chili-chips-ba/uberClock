#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>
#include <generated/soc.h>
#include <libbase/uart.h>

#include "libliteeth/udp.h"
#include "ubddr3.h"
#include "uberclock/uberclock_internal.h"

#define UBD3_MAGIC 0x55424433u
#define UBD3_BOARD_IP IPTOINT(192,168,0,123)
#define UBD3_PAYLOAD_MAX 1400u
#define UBD3_SERVICE_EVERY 64u
#define UBD3_PROGRESS_EVERY 0u

static inline void prv_cache_sync(void) {
    flush_cpu_dcache();
    flush_l2_cache();
}

static uint8_t prv_size_to_code(const char *s) {
    if (!s) return 0;
    if (!strcmp(s, "bus")) return 0;
    if (!strcmp(s, "32")) return 1;
    if (!strcmp(s, "16")) return 2;
    if (!strcmp(s, "8")) return 3;
    return 0;
}

static void prv_help(char *args) {
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

static void prv_cmd_ub_info(char *a) {
    int cal;
    (void)a;

    cal = ubddr3_calib_done_read();
#ifdef CSR_UBDDR3_BASE
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d\n",
           (unsigned long)CSR_UBDDR3_BASE, cal);
#else
    printf("UBDDR3 CSR base: <not exported>  calib_done: %d\n", cal);
#endif
}

static void prv_cmd_ub_mode(char *a) {
    unsigned v;
    (void)a;

    v = main_cap_enable_read() & 1u;
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void prv_cmd_ub_setmode(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    uberclock_int_set_cap_enable(v);
    uberclock_int_commit();
    printf("cap_enable = %u (%s)\n", v ? 1u : 0u, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void prv_dma_start(uint64_t addr, uint32_t beats, uint8_t size_code) {
    (void)beats;
    ubddr3_dma_inc_write(1);
    ubddr3_dma_size_write(size_code);
    ubddr3_dma_addr0_write((uint32_t)(addr & 0xffffffffu));
    ubddr3_dma_addr1_write((uint32_t)(addr >> 32));
    ubddr3_dma_req_write(1);
}

static void prv_cmd_ub_start(char *args) {
    char *tok_addr = strtok(args, " \t");
    char *tok_beats = strtok(NULL, " \t");
    char *tok_size = strtok(NULL, " \t");
    uint64_t addr;
    uint32_t beats;
    uint8_t sz;
    unsigned mode;

    if (!tok_addr) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    addr = strtoull(tok_addr, NULL, 0);
    beats = (uint32_t)(tok_beats ? strtoul(tok_beats, NULL, 0) : 256u);
    sz = prv_size_to_code(tok_size);
    mode = main_cap_enable_read() & 1u;

    printf("S2MM start: mode=%s addr=0x%08lx_%08lx beats=%u size=%s\n",
           mode ? "CAPTURE" : "RAMP",
           (unsigned long)(addr >> 32), (unsigned long)(addr & 0xffffffffu),
           (unsigned)beats,
           (sz == 0) ? "bus" : (sz == 1) ? "32" : (sz == 2) ? "16" : "8");

    uberclock_int_set_cap_beats(beats);
    uberclock_int_commit();
    prv_dma_start(addr, beats, sz);
}

static void prv_cmd_ub_ramp(char *args) {
    uberclock_int_set_cap_enable(0);
    uberclock_int_commit();
    prv_cmd_ub_start(args);
}

static void prv_cmd_ub_cap(char *args) {
    uberclock_int_set_cap_enable(1);
    uberclock_int_commit();
    prv_cmd_ub_start(args);
}

static void prv_cmd_ub_wait(char *a) {
    (void)a;
    printf("Waiting for DMA ... ");
    fflush(stdout);
    while (ubddr3_dma_busy_read()) {
    }
    prv_cache_sync();
    puts("done.");
    if (ubddr3_dma_err_read()) {
        puts("DMA error flag is set!");
    }
}

static void prv_cmd_ub_hexdump(char *a) {
    char *tok_addr = strtok(a, " \t");
    char *tok_len = strtok(NULL, " \t");
    uint64_t addr;
    uint32_t len;
    volatile uint8_t *p;
    uint32_t i;

    if (!tok_addr || !tok_len) {
        puts("Usage: ub_hexdump <addr_hex> <bytes>");
        return;
    }

    addr = strtoull(tok_addr, NULL, 0);
    len = (uint32_t)strtoul(tok_len, NULL, 0);
    p = (volatile uint8_t *)(uintptr_t)addr;

    for (i = 0; i < len; i++) {
        if ((i & 0x0f) == 0) {
            printf("\n%08lx: ", (unsigned long)((addr + i) & 0xffffffffu));
        }
        printf("%02x ", p[i]);
    }
    puts("");
}

static void prv_cmd_cap_beats(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "256", NULL, 0);
    if (v == 0u) {
        puts("cap_beats must be >= 1");
        return;
    }
    uberclock_int_set_cap_beats(v);
    uberclock_int_commit();
    printf("cap_beats = %u\n", (unsigned)v);
}

static void prv_u32le_store(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 0);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int prv_parse_ipv4(const char *s, uint32_t *out_ip) {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;

    if (!s) return -1;
    if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return -1;
    if (a > 255 || b > 255 || c > 255 || d > 255) return -1;
    *out_ip = IPTOINT(a, b, c, d);
    return 0;
}

static void prv_cmd_ub_send(char *args) {
    char *tok_addr = strtok(args, " \t");
    char *tok_len = strtok(NULL, " \t");
    char *tok_ip = strtok(NULL, " \t");
    char *tok_port = strtok(NULL, " \t");
    uint64_t addr;
    uint32_t total;
    uint32_t dst_ip = 0;
    uint16_t dst_port;
    uint16_t src_port;
    static const unsigned char board_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0xAB};
    volatile uint8_t *p;
    const uint32_t hdr_sz = 16u;
    const uint32_t max_data = (UBD3_PAYLOAD_MAX > hdr_sz) ? (UBD3_PAYLOAD_MAX - hdr_sz) : 0u;
    uint32_t sent = 0;
    uint32_t seq = 0;
    const uint32_t service_mask =
        (UBD3_SERVICE_EVERY && ((UBD3_SERVICE_EVERY & (UBD3_SERVICE_EVERY - 1u)) == 0u))
            ? (UBD3_SERVICE_EVERY - 1u)
            : 0u;
    int ok = 0;
    unsigned i;

    if (!tok_addr || !tok_len || !tok_ip || !tok_port) {
        puts("Usage: ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
        return;
    }

    addr = strtoull(tok_addr, NULL, 0);
    total = (uint32_t)strtoul(tok_len, NULL, 0);
    if (prv_parse_ipv4(tok_ip, &dst_ip) != 0) {
        puts("Error: bad dst_ip format (use a.b.c.d)");
        return;
    }
    dst_port = (uint16_t)strtoul(tok_port, NULL, 0);
    src_port = dst_port;

    if (total == 0u) {
        puts("Error: bytes must be > 0");
        return;
    }
    if (max_data < 64u) {
        puts("Error: UBD3_PAYLOAD_MAX too small");
        return;
    }

    eth_init();
    udp_set_mac(board_mac);
    udp_set_ip(UBD3_BOARD_IP);
    udp_start(board_mac, UBD3_BOARD_IP);

    for (i = 0; i < 200000; i++) {
        udp_service();
        if (udp_arp_resolve(dst_ip) != 0) {
            ok = 1;
            break;
        }
    }
    if (!ok) {
        puts("No ARP reply.");
        return;
    }

    p = (volatile uint8_t *)(uintptr_t)addr;

    while (sent < total) {
        uint32_t chunk;
        uint8_t *tx;

        if (UBD3_SERVICE_EVERY) {
            if (service_mask) {
                if ((seq & service_mask) == 0u) {
                    udp_service();
                }
            } else if ((seq % UBD3_SERVICE_EVERY) == 0u) {
                udp_service();
            }
        }

        chunk = total - sent;
        if (chunk > max_data) {
            chunk = max_data;
        }

        tx = (uint8_t *)udp_get_tx_buffer();
        if (!tx) {
            return;
        }

        prv_u32le_store(tx + 0, UBD3_MAGIC);
        prv_u32le_store(tx + 4, seq);
        prv_u32le_store(tx + 8, sent);
        prv_u32le_store(tx + 12, total);

        memcpy(tx + hdr_sz, (const void *)(p + sent), chunk);
        (void)udp_send(src_port, dst_port, (unsigned)(hdr_sz + chunk));

        sent += chunk;
        seq++;

        if (UBD3_PROGRESS_EVERY && ((seq % UBD3_PROGRESS_EVERY) == 0u)) {
            printf("sent %lu / %lu\n", (unsigned long)sent, (unsigned long)total);
        }
    }
}

static const struct cmd_entry g_dma_cmds[] = {
    {"prv_help",    prv_help,        "UberDDR3/S2MM help"},
    {"ub_info",    prv_cmd_ub_info,    "Show UBDDR3 info/state"},
    {"ub_mode",    prv_cmd_ub_mode,    "Show current cap_enable mode"},
    {"ub_setmode", prv_cmd_ub_setmode, "Set cap_enable (0=ramp,1=capture)"},
    {"ub_start",   prv_cmd_ub_start,   "Start S2MM using current mode"},
    {"ub_ramp",    prv_cmd_ub_ramp,    "Force ramp mode then start S2MM"},
    {"ub_cap",     prv_cmd_ub_cap,     "Force capture mode then start S2MM"},
    {"ub_wait",    prv_cmd_ub_wait,    "Wait until DMA done"},
    {"ub_hexdump", prv_cmd_ub_hexdump, "Hexdump DDR memory"},
    {"ub_send",    prv_cmd_ub_send,    "Send DDR memory region via UDP"},
    {"cap_beats",  prv_cmd_cap_beats,  "Set capture length in 256-bit beats"},
};

void uberclock_dma_register_cmds(void) {
    console_register(g_dma_cmds, sizeof(g_dma_cmds) / sizeof(g_dma_cmds[0]));
}
