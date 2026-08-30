// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

#ifndef SIG3_H
#define SIG3_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DDS math, reused directly by trackq.c's Goertzel correlator. */
uint32_t sig3_phase_inc(uint32_t f_hz, uint32_t fs_hz);
uint32_t sig3_phase_inc_from_mhz(int64_t f_hz_milli, uint32_t fs_hz);
int16_t  sig3_sin_u32(uint32_t ph);

/* Lifecycle + per-channel control, used by cmd_track3/cmd_trackq_start and
 * by uberclock_init()/service_one_ce_event(). */
void sig3_start(void);
void sig3_push_one(void);
void sig3_set_channel_symmetric(unsigned channel, uint32_t center_hz, uint32_t delta_hz);
void sig3_update_increments(void);
void sig3_enable_channel(unsigned channel);

/* What tone frequency is this sig3 channel currently centered on, if it's
 * actively driving fallback_center_hz's target tone? Falls back to
 * fallback_center_hz (as milli-Hz) otherwise. Used by trackq.c's
 * trackq_center_tone_hz_milli() instead of reaching into sig3 internals. */
int64_t sig3_channel_center_hz_milli(unsigned channel, uint32_t fallback_center_hz);

/* Console commands, referenced by uberclock.c's command table. */
void cmd_sig3_amp(char *a);
void cmd_sig3_freqs(char *args);
void cmd_sig3_enable_ch(char *a);
void cmd_sig3_disable_ch(char *a);
void cmd_sig3_start(char *a);
void cmd_sig3_stop(char *a);

#ifdef __cplusplus
}
#endif

#endif
