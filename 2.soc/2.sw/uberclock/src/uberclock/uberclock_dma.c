#include <stdio.h>
#include <string.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include <libliteeth/udp.h>
#include "../../../console.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_parse.h"
#include "uberclock/uberclock_dma.h"

#define UBD3_MAGIC 0x55424433u
#define UBD3_BOARD_IP IPTOINT(192,168,0,123)
#define UBD3_PAYLOAD_MAX 1400u
#define UBD3_SERVICE_EVERY 64u
#define UBD3_PROGRESS_EVERY 0u

static uint8_t size_name_to_code(const char *size_name) {
    if (!size_name || !strcmp(size_name, "bus")) {
        return 0u;
    }
    if (!strcmp(size_name, "32")) {
        return 1u;
    }
    if (!strcmp(size_name, "16")) {
        return 2u;
    }
    if (!strcmp(size_name, "8")) {
        return 3u;
    }
    return 0u;
}

static const char *size_code_name(uint8_t size_code) {
    switch (size_code) {
        case 0u: return "bus";
        case 1u: return "32";
        case 2u: return "16";
        default: return "8";
    }
}

static void dma_start(uint64_t address, uint32_t beats, uint8_t size_code) {
    (void)beats;
    ubddr3_dma_inc_write(1);
    ubddr3_dma_size_write(size_code);
    ubddr3_dma_addr0_write((uint32_t)(address & 0xffffffffu));
    ubddr3_dma_addr1_write((uint32_t)(address >> 32));
    ubddr3_dma_req_write(1);
}

static void store_u32le(uint8_t *buffer, uint32_t value) {
    buffer[0] = (uint8_t)(value >> 0);
    buffer[1] = (uint8_t)(value >> 8);
    buffer[2] = (uint8_t)(value >> 16);
    buffer[3] = (uint8_t)(value >> 24);
}

void uberclock_dma_print_help(void) {
    puts("UberDDR3/S2MM commands");
    puts("  ub_info");
    puts("  ub_mode");
    puts("  ub_setmode <0|1>");
    puts("  ub_ramp <addr_hex> [beats] [size]");
    puts("  ub_cap  <addr_hex> [beats] [size]");
    puts("  ub_start <addr_hex> [beats] [size]");
    puts("  ub_wait");
    puts("  ub_hexdump <addr_hex> <bytes>");
    puts("  ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
}

void uberclock_dma_print_info(void) {
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d  (UBDDR3_MEM_BASE: 0x%08lx)\n",
           (unsigned long)CSR_UBDDR3_BASE,
           ubddr3_calib_done_read(),
           (unsigned long)UBDDR3_MEM_BASE);
}

void uberclock_dma_print_mode(void) {
    unsigned mode = uberclock_get_capture_enable();
    printf("cap_enable = %u (%s)\n", mode, mode ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

void uberclock_dma_set_mode(unsigned enabled) {
    uberclock_set_capture_enable(enabled);
    uberclock_commit_config();
    uberclock_dma_print_mode();
}

void uberclock_dma_start_current_mode(uint64_t address, uint32_t beats, const char *size_name) {
    uint8_t size_code = size_name_to_code(size_name);
    unsigned mode = uberclock_get_capture_enable();

    printf("S2MM start: mode=%s addr=0x%08lx_%08lx beats=%u size=%s\n",
           mode ? "CAPTURE" : "RAMP",
           (unsigned long)(address >> 32),
           (unsigned long)(address & 0xffffffffu),
           (unsigned)beats,
           size_code_name(size_code));

    uberclock_set_capture_beats(beats);
    uberclock_commit_config();
    dma_start(address, beats, size_code);
}

void uberclock_dma_start_ramp(uint64_t address, uint32_t beats, const char *size_name) {
    uberclock_set_capture_enable(0u);
    uberclock_commit_config();
    uberclock_dma_start_current_mode(address, beats, size_name);
}

void uberclock_dma_start_capture(uint64_t address, uint32_t beats, const char *size_name) {
    uberclock_set_capture_enable(1u);
    uberclock_commit_config();
    uberclock_dma_start_current_mode(address, beats, size_name);
}

void uberclock_dma_wait(void) {
    printf("Waiting for DMA ... ");
    fflush(stdout);
    while (ubddr3_dma_busy_read()) {
    }
    uberclock_cache_sync();
    puts("done.");
    if (ubddr3_dma_err_read()) {
        puts("DMA error flag is set!");
    }
}

void uberclock_dma_hexdump(uint64_t address, uint32_t length_bytes) {
    volatile uint8_t *memory = (volatile uint8_t *)(uintptr_t)address;
    uint32_t index;

    for (index = 0u; index < length_bytes; ++index) {
        if ((index & 0x0fu) == 0u) {
            printf("\n%08lx: ", (unsigned long)((address + index) & 0xffffffffu));
        }
        printf("%02x ", memory[index]);
    }
    puts("");
}

int uberclock_dma_send_udp(uint64_t address, uint32_t length_bytes, const char *dst_ip, uint16_t dst_port) {
    static const unsigned char board_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0xAB};
    volatile uint8_t *memory = (volatile uint8_t *)(uintptr_t)address;
    uint32_t dst_ip_value = 0u;
    uint16_t src_port = dst_port;
    uint32_t sent = 0u;
    uint32_t sequence = 0u;
    uint32_t header_size = 16u;
    uint32_t max_data = (UBD3_PAYLOAD_MAX > header_size) ? (UBD3_PAYLOAD_MAX - header_size) : 0u;
    uint32_t service_mask =
    (UBD3_SERVICE_EVERY && ((UBD3_SERVICE_EVERY & (UBD3_SERVICE_EVERY - 1u)) == 0u)) ? (UBD3_SERVICE_EVERY - 1u) : 0u;
    unsigned retry;

    if (length_bytes == 0u) {
        puts("Error: bytes must be > 0");
        return -1;
    }
    if (uberclock_parse_ipv4(dst_ip, &dst_ip_value) != 0) {
        puts("Error: bad dst_ip format (use a.b.c.d)");
        return -1;
    }
    if (max_data < 64u) {
        puts("Error: UBD3_PAYLOAD_MAX too small");
        return -1;
    }

    eth_init();
    udp_set_mac(board_mac);
    udp_set_ip(UBD3_BOARD_IP);
    udp_start(board_mac, UBD3_BOARD_IP);

    for (retry = 0u; retry < 200000u; ++retry) {
        udp_service();
        if (udp_arp_resolve(dst_ip_value) != 0) {
            break;
        }
    }
    if (retry == 200000u) {
        puts("No ARP reply.");
        return -1;
    }

    while (sent < length_bytes) {
        uint8_t *tx_buffer;
        uint32_t chunk = length_bytes - sent;

        if (UBD3_SERVICE_EVERY) {
            if (service_mask != 0u) {
                if ((sequence & service_mask) == 0u) {
                    udp_service();
                }
            } else if ((sequence % UBD3_SERVICE_EVERY) == 0u) {
                udp_service();
            }
        }

        if (chunk > max_data) {
            chunk = max_data;
        }

        tx_buffer = (uint8_t *)udp_get_tx_buffer();
        if (!tx_buffer) {
            return -1;
        }

        store_u32le(tx_buffer + 0u, UBD3_MAGIC);
        store_u32le(tx_buffer + 4u, sequence);
        store_u32le(tx_buffer + 8u, sent);
        store_u32le(tx_buffer + 12u, length_bytes);
        memcpy(tx_buffer + header_size, (const void *)(memory + sent), chunk);
        (void)udp_send(src_port, dst_port, (unsigned)(header_size + chunk));

        sent += chunk;
        ++sequence;

        if (UBD3_PROGRESS_EVERY && ((sequence % UBD3_PROGRESS_EVERY) == 0u)) {
            printf("sent %lu / %lu\n", (unsigned long)sent, (unsigned long)length_bytes);
        }
    }

    return 0;
}
