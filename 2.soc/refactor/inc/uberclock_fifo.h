#ifndef UBERCLOCK_FIFO_H
#define UBERCLOCK_FIFO_H

#include <stdint.h>

unsigned uberclock_ds_fifo_flush(unsigned max_samples);
int uberclock_wait_for_ds_sample(const char *phase_name, unsigned sample_index, unsigned total_samples);
int uberclock_ds_fifo_pop_simple(int16_t *sample_x, int16_t *sample_y);
int uberclock_ds_fifo_pop_capture(int16_t *sample_x, int16_t *sample_y);
unsigned uberclock_dsp_pump_step(unsigned max_in, unsigned max_out);

int uberclock_ups_fifo_push(int16_t sample_x, int16_t sample_y);
unsigned uberclock_ds_fifo_flags(void);
unsigned uberclock_ds_fifo_overflow(void);
unsigned uberclock_ds_fifo_underflow(void);
void uberclock_ds_fifo_clear_status(void);
unsigned uberclock_ups_fifo_flags(void);
unsigned uberclock_ups_fifo_overflow(void);
unsigned uberclock_ups_fifo_underflow(void);
void uberclock_ups_fifo_clear_status(void);

#endif

