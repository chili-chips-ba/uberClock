// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UBERCLOCK_TRACK_H
#define UBERCLOCK_TRACK_H

#include <stdint.h>

void uberclock_track_poll(void);
void uberclock_track_stop(void);
int uberclock_track3_run(uint32_t start_hz,
                         unsigned channel_index,
                         uint32_t step_hz,
                         unsigned max_steps,
                         unsigned sample_count,
                         uint32_t center_hz,
                         uint32_t delta_hz);
int uberclock_trackq_start(uint32_t ch1_hz,
                           uint32_t ch2_hz,
                           uint32_t ch3_hz,
                           unsigned sample_count,
                           uint32_t center_hz,
                           uint32_t delta_ch1_hz,
                           uint32_t delta_ch2_hz,
                           uint32_t delta_ch3_hz);
int uberclock_trackq_probe(unsigned sample_count, uint32_t center_hz, uint32_t delta_hz);

#endif
