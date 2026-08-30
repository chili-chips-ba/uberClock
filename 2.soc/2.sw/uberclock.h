// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

#ifndef UBERCLOCK_H
#define UBERCLOCK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
    #endif

    typedef struct {
        int16_t x[5];
        int16_t y[5];
    } iq5_frame_t;

    typedef struct {
        int16_t x[6];
        int16_t y[6];
    } iq6_frame_t;

    /* Pure arithmetic, no CSR dependency -- shared as static inline so every
     * translation unit gets its own copy without a cross-file linkage dance. */
    static inline unsigned parse_u(const char *s, unsigned max, const char *what) {
        unsigned v = (unsigned)strtoul(s ? s : "0", NULL, 0);
        if (v >= max) printf("Error: %s must be 0..%u\n", what, max - 1);
        return v;
    }

    static inline int parse_s(const char *s, int minv, int maxv, const char *what) {
        long v = strtol(s ? s : "0", NULL, 0);
        if (v < minv || v > maxv) {
            printf("Error: %s must be %d..%d\n", what, minv, maxv);
        }
        return (int)v;
    }

    static inline uint32_t uc_phase_inc_from_hz(uint32_t f_hz, uint32_t fs_hz) {
        return (uint32_t)(((uint64_t)f_hz << 26) / (uint64_t)fs_hz);
    }

    static inline uint32_t uc_phase_inc_to_hz(uint32_t phase_inc, uint32_t fs_hz) {
        return (uint32_t)((((uint64_t)phase_inc * (uint64_t)fs_hz) + (1u << 25)) >> 26);
    }

    static inline int64_t uc_phase_inc_to_mhz(uint32_t phase_inc, uint32_t fs_hz) {
        return (int64_t)((((uint64_t)phase_inc * (uint64_t)fs_hz * 1000ull) + (1u << 25)) >> 26);
    }

    /* Shared ISR-tick state: ce_ticks/ce_event are defined (and incremented,
     * in ce_down_isr) in uberclock.c; trackq.c reads/decrements them too. */
    extern volatile uint32_t ce_ticks;
    extern volatile uint32_t ce_event;

    /* Defined in uberclock.c, touch CSR macros the header shouldn't need to
     * pull in, so kept as regular (non-inline) functions. */
    void uc_commit(void);
    void ds_fifo_read_frame(iq6_frame_t *frame);
    void ups_fifo_write_frame(const iq5_frame_t *frame);
    void service_one_ce_event(void);

    void uberclock_register_cmds(void);
    void uberclock_init(void);
    void uberclock_poll(void);

    #ifdef __cplusplus
}
#endif

#endif
