/**
 * @file uberclock_dma.h
 * @brief DDR / DMA control interface.
 */

#ifndef UBERCLOCK_DMA_H
#define UBERCLOCK_DMA_H

#include <stdint.h>

/** Print DMA help */
void uberclock_dma_print_help(void);

/** Print DMA configuration */
void uberclock_dma_print_info(void);

/** Print current DMA mode */
void uberclock_dma_print_mode(void);

/** Enable or disable DMA */
void uberclock_dma_set_mode(unsigned enabled);

/**
 * @brief Start DMA using current mode.
 */
void uberclock_dma_start_current_mode(uint64_t address, uint32_t beats, const char *size_name);

/**
 * @brief Start ramp pattern DMA.
 */
void uberclock_dma_start_ramp(uint64_t address, uint32_t beats, const char *size_name);

/**
 * @brief Start capture DMA.
 */
void uberclock_dma_start_capture(uint64_t address, uint32_t beats, const char *size_name);

/** Wait for DMA completion */
void uberclock_dma_wait(void);

/** Dump memory region */
void uberclock_dma_hexdump(uint64_t address, uint32_t length_bytes);

/**
 * @brief Send memory over UDP.
 */
int uberclock_dma_send_udp(uint64_t address, uint32_t length_bytes,
                           const char *dst_ip, uint16_t dst_port);

#endif
