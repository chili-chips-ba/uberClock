// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file uberclock_fifo.h
 * @brief FIFO interface for DSP streaming.
 */

#ifndef UBERCLOCK_FIFO_H
#define UBERCLOCK_FIFO_H

#include <stdint.h>
#include "uberclock/uberclock_types.h"

/** Flush DS FIFO */
unsigned uberclock_ds_fifo_flush(unsigned max_samples);

/** Wait for sample */
int uberclock_wait_for_ds_sample(const char *phase_name, unsigned sample_index, unsigned total_samples);

/** Pop DS sample */
int uberclock_ds_fifo_pop_simple(int16_t *sample_x, int16_t *sample_y);

/** Pop capture sample */
int uberclock_ds_fifo_pop_capture(int16_t *sample_x, int16_t *sample_y);

int uberclock_ds_fifo_pop_frame(struct uberclock_iq_frame *frame);

/** Perform DSP pump step */
unsigned uberclock_dsp_pump_step(unsigned max_in, unsigned max_out);

/** Push sample to upsampler */
int uberclock_ups_fifo_push(int16_t sample_x, int16_t sample_y);

int uberclock_ups_fifo_push_frame(const struct uberclock_iq_frame *frame);

/** FIFO status */
unsigned uberclock_ds_fifo_flags(void);
unsigned uberclock_ds_fifo_overflow(void);
unsigned uberclock_ds_fifo_underflow(void);
void uberclock_ds_fifo_clear_status(void);

unsigned uberclock_ups_fifo_flags(void);
unsigned uberclock_ups_fifo_overflow(void);
unsigned uberclock_ups_fifo_underflow(void);
void uberclock_ups_fifo_clear_status(void);

#endif
