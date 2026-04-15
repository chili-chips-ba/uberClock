#ifndef UBERCLOCK_DMA_H
#define UBERCLOCK_DMA_H

#include <stdint.h>

void uberclock_dma_print_help(void);
void uberclock_dma_print_info(void);
void uberclock_dma_print_mode(void);
void uberclock_dma_set_mode(unsigned enabled);
void uberclock_dma_start_current_mode(uint64_t address, uint32_t beats, const char *size_name);
void uberclock_dma_start_ramp(uint64_t address, uint32_t beats, const char *size_name);
void uberclock_dma_start_capture(uint64_t address, uint32_t beats, const char *size_name);
void uberclock_dma_wait(void);
void uberclock_dma_hexdump(uint64_t address, uint32_t length_bytes);
int uberclock_dma_send_udp(uint64_t address, uint32_t length_bytes, const char *dst_ip, uint16_t dst_port);

#endif

