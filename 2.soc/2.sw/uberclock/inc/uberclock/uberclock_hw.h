#ifndef UBERCLOCK_HW_H
#define UBERCLOCK_HW_H

#include <stdint.h>

void uberclock_commit_config(void);
uint32_t uberclock_phase_inc_from_hz(uint32_t frequency_hz, uint32_t sample_rate_hz);
uint32_t uberclock_phase_inc_to_hz(uint32_t phase_increment, uint32_t sample_rate_hz);

void uberclock_set_nco_phase_increment(uint32_t phase_increment);
void uberclock_set_nco_magnitude(int16_t magnitude);
void uberclock_set_phase_down_reference(uint32_t phase_increment);
void uberclock_set_input_select(uint32_t input_select);
void uberclock_set_upsampler_input_mux(uint32_t mux_select);
void uberclock_set_output_select_ch1(uint32_t output_select);
void uberclock_set_output_select_ch2(uint32_t output_select);
void uberclock_set_lowspeed_debug_select(uint32_t debug_select);
void uberclock_set_highspeed_debug_select(uint32_t debug_select);
void uberclock_set_final_shift(int32_t final_shift);
void uberclock_set_upsampler_input_x(int16_t sample);
void uberclock_set_upsampler_input_y(int16_t sample);

void uberclock_set_capture_enable(unsigned enabled);
unsigned uberclock_get_capture_enable(void);
void uberclock_set_capture_beats(uint32_t beats);
void uberclock_capture_arm_pulse(void);
unsigned uberclock_capture_done(void);
int16_t uberclock_capture_read_sample(unsigned index);

#endif

