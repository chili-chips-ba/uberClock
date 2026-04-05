#ifndef UBERCLOCK_REGS_H
#define UBERCLOCK_REGS_H

#include <stdint.h>

#define UBERCLOCK_CHANNELS 5
#define UBERCLOCK_SIG3_TONES 3

typedef struct {
    int16_t x[UBERCLOCK_CHANNELS];
    int16_t y[UBERCLOCK_CHANNELS];
} iq5_frame_t;

unsigned uberclock_int_parse_u(const char *s, unsigned max, const char *what);
int uberclock_int_parse_s(const char *s, int minv, int maxv, const char *what);

void uberclock_int_commit(void);

void uberclock_int_write_phase_down(unsigned ch, uint32_t value);
void uberclock_int_write_phase_cpu(unsigned ch, uint32_t value);
void uberclock_int_write_mag_cpu(unsigned ch, int value);
void uberclock_int_write_gain(unsigned ch, int32_t value);
void uberclock_int_write_output_select(unsigned ch, unsigned value);
void uberclock_int_write_nco_mag(int value);
void uberclock_int_write_upsampler_inputs_all_x(int16_t value);
void uberclock_int_write_upsampler_inputs_all_y(int16_t value);
void uberclock_int_set_input_select(unsigned value);
void uberclock_int_set_upsampler_input_mux(unsigned value);
void uberclock_int_set_lowspeed_dbg_select(unsigned value);
void uberclock_int_set_highspeed_dbg_select(unsigned value);
void uberclock_int_set_final_shift(int32_t value);
void uberclock_int_set_cap_enable(unsigned value);
void uberclock_int_set_cap_beats(uint32_t value);

void uberclock_int_ups_fifo_write_frame(const iq5_frame_t *frame);
void uberclock_int_ups_fifo_write_replicated(int16_t x, int16_t y);
void uberclock_int_ds_fifo_read_frame(iq5_frame_t *frame);
void uberclock_int_fifo_clear_flags(void);
int uberclock_int_ds_wait_readable(unsigned limit);
void uberclock_int_pulse_cap_arm(void);
unsigned uberclock_int_read_cap_done(void);
int16_t uberclock_int_read_cap_sample(unsigned idx);
unsigned uberclock_int_read_ds_flags(void);
unsigned uberclock_int_read_ds_overflow(void);
unsigned uberclock_int_read_ds_underflow(void);
void uberclock_int_clear_ds_flags(void);
unsigned uberclock_int_read_ups_flags(void);
unsigned uberclock_int_read_ups_overflow(void);
unsigned uberclock_int_read_ups_underflow(void);
void uberclock_int_clear_ups_flags(void);

#endif
