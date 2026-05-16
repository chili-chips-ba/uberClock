// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UBERCLOCK_SIGGEN_H
#define UBERCLOCK_SIGGEN_H

#include <stdint.h>
#include "uberclock/uberclock_types.h"

void uberclock_siggen_start(void);
void uberclock_siggen_stop(void);
int uberclock_siggen_step(int16_t *sample_x, int16_t *sample_y);
int uberclock_siggen_step_frame(struct uberclock_iq_frame *frame);
void uberclock_siggen_service_push(void);
void uberclock_siggen_set_amplitude_all(int16_t amplitude);
void uberclock_siggen_set_channel_amplitude(unsigned channel_index, int16_t amplitude);
int16_t uberclock_siggen_channel_amplitude(unsigned channel_index);
void uberclock_siggen_set_channel_frequencies(unsigned channel_index,
                                              uint32_t f1_hz,
                                              uint32_t f2_hz,
                                              uint32_t f3_hz);
void uberclock_siggen_set_channel_symmetric(unsigned channel_index, uint32_t center_hz, uint32_t delta_hz);
void uberclock_siggen_enable_channel(unsigned channel_index);
void uberclock_siggen_disable_channel(unsigned channel_index);
int uberclock_siggen_channel_enabled(unsigned channel_index);
uint32_t uberclock_siggen_channel_frequency(unsigned channel_index, unsigned tone_index);
uint32_t uberclock_siggen_channel_increment(unsigned channel_index, unsigned tone_index);
int64_t uberclock_siggen_increment_to_mhz(uint32_t phase_increment, uint32_t sample_rate_hz);

#endif
