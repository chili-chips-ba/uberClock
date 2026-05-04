#include <generated/csr.h>
#include "uberclock/uberclock_hw.h"

void uberclock_commit_config(void) {
    cfg_link_commit_write(1);
}

uint32_t uberclock_phase_inc_from_hz(uint32_t frequency_hz, uint32_t sample_rate_hz) {
    return (uint32_t)((((uint64_t)frequency_hz << 26) + (sample_rate_hz / 2u)) / (uint64_t)sample_rate_hz);
}

uint32_t uberclock_phase_inc_to_hz(uint32_t phase_increment, uint32_t sample_rate_hz) {
    return (uint32_t)((((uint64_t)phase_increment * (uint64_t)sample_rate_hz) + (1u << 25)) >> 26);
}

void uberclock_set_nco_phase_increment(uint32_t phase_increment) {
    main_phase_inc_nco_write(phase_increment);
}

void uberclock_set_nco_magnitude(int16_t magnitude) {
    main_nco_mag_write((uint32_t)((int32_t)magnitude & 0x0fff));
}

void uberclock_set_phase_down_reference(uint32_t phase_increment) {
    main_phase_inc_down_ref_write(phase_increment);
}

void uberclock_set_input_select(uint32_t input_select) {
    main_input_select_write(input_select);
}

void uberclock_set_upsampler_input_mux(uint32_t mux_select) {
    main_upsampler_input_mux_write(mux_select);
}

void uberclock_set_output_select_ch1(uint32_t output_select) {
    main_output_select_ch1_write(output_select & 0x0fu);
}

void uberclock_set_output_select_ch2(uint32_t output_select) {
    main_output_select_ch2_write(output_select & 0x0fu);
}

void uberclock_set_lowspeed_debug_select(uint32_t debug_select) {
    main_lowspeed_dbg_select_write(debug_select);
}

void uberclock_set_highspeed_debug_select(uint32_t debug_select) {
    main_highspeed_dbg_select_write(debug_select);
}

void uberclock_set_final_shift(int32_t final_shift) {
    main_final_shift_write((uint32_t)final_shift);
}

void uberclock_set_upsampler_input_x(int16_t sample) {
    main_upsampler_input_x_write((uint32_t)((int32_t)sample & 0xffff));
}

void uberclock_set_upsampler_input_y(int16_t sample) {
    main_upsampler_input_y_write((uint32_t)((int32_t)sample & 0xffff));
}

void uberclock_set_capture_enable(unsigned enabled) {
    main_cap_enable_write(enabled ? 1u : 0u);
}

unsigned uberclock_get_capture_enable(void) {
    return main_cap_enable_read() & 1u;
}

void uberclock_set_capture_beats(uint32_t beats) {
    main_cap_beats_write(beats);
}

void uberclock_capture_arm_pulse(void) {
    main_cap_arm_write(0);
    uberclock_commit_config();
    main_cap_arm_write(1);
    uberclock_commit_config();
    main_cap_arm_write(0);
    uberclock_commit_config();
}

unsigned uberclock_capture_done(void) {
    return main_cap_done_read() & 1u;
}

int16_t uberclock_capture_read_sample(unsigned index) {
    main_cap_idx_write(index);
    uberclock_commit_config();
    (void)main_cap_data_read();
    return (int16_t)(main_cap_data_read() & 0xffffu);
}
