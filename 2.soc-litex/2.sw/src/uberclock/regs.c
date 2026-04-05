#include <stdio.h>
#include <stdlib.h>

#include <generated/csr.h>

#include "uberclock/uberclock_internal.h"

typedef void (*uberclock_int_write_u32_fn)(uint32_t);
typedef uint32_t (*uberclock_int_read_u32_fn)(void);

static const uberclock_int_write_u32_fn g_phase_down_write[UBERCLOCK_CHANNELS] = {
    main_phase_inc_down_1_write,
    main_phase_inc_down_2_write,
    main_phase_inc_down_3_write,
    main_phase_inc_down_4_write,
    main_phase_inc_down_5_write,
};

static const uberclock_int_write_u32_fn g_phase_cpu_write[UBERCLOCK_CHANNELS] = {
    main_phase_inc_cpu1_write,
    main_phase_inc_cpu2_write,
    main_phase_inc_cpu3_write,
    main_phase_inc_cpu4_write,
    main_phase_inc_cpu5_write,
};

static const uberclock_int_write_u32_fn g_mag_cpu_write[UBERCLOCK_CHANNELS] = {
    main_mag_cpu1_write,
    main_mag_cpu2_write,
    main_mag_cpu3_write,
    main_mag_cpu4_write,
    main_mag_cpu5_write,
};

static const uberclock_int_write_u32_fn g_gain_write[UBERCLOCK_CHANNELS] = {
    main_gain1_write,
    main_gain2_write,
    main_gain3_write,
    main_gain4_write,
    main_gain5_write,
};

static const uberclock_int_write_u32_fn g_output_select_write[2] = {
    main_output_select_ch1_write,
    main_output_select_ch2_write,
};

static const uberclock_int_write_u32_fn g_ups_x_write[UBERCLOCK_CHANNELS] = {
    main_upsampler_input_x1_write,
    main_upsampler_input_x2_write,
    main_upsampler_input_x3_write,
    main_upsampler_input_x4_write,
    main_upsampler_input_x5_write,
};

static const uberclock_int_write_u32_fn g_ups_y_write[UBERCLOCK_CHANNELS] = {
    main_upsampler_input_y1_write,
    main_upsampler_input_y2_write,
    main_upsampler_input_y3_write,
    main_upsampler_input_y4_write,
    main_upsampler_input_y5_write,
};

static const uberclock_int_write_u32_fn g_ups_fifo_x_write[UBERCLOCK_CHANNELS] = {
    main_ups_fifo_x1_write,
    main_ups_fifo_x2_write,
    main_ups_fifo_x3_write,
    main_ups_fifo_x4_write,
    main_ups_fifo_x5_write,
};

static const uberclock_int_write_u32_fn g_ups_fifo_y_write[UBERCLOCK_CHANNELS] = {
    main_ups_fifo_y1_write,
    main_ups_fifo_y2_write,
    main_ups_fifo_y3_write,
    main_ups_fifo_y4_write,
    main_ups_fifo_y5_write,
};

static const uberclock_int_read_u32_fn g_ds_fifo_x_read[UBERCLOCK_CHANNELS] = {
    main_ds_fifo_x1_read,
    main_ds_fifo_x2_read,
    main_ds_fifo_x3_read,
    main_ds_fifo_x4_read,
    main_ds_fifo_x5_read,
};

static const uberclock_int_read_u32_fn g_ds_fifo_y_read[UBERCLOCK_CHANNELS] = {
    main_ds_fifo_y1_read,
    main_ds_fifo_y2_read,
    main_ds_fifo_y3_read,
    main_ds_fifo_y4_read,
    main_ds_fifo_y5_read,
};

static uint32_t pack_s16(int value) {
    return (uint32_t)((int32_t)value & 0xffff);
}

static uint32_t pack_s12(int value) {
    return (uint32_t)((int32_t)value & 0x0fff);
}

unsigned uberclock_int_parse_u(const char *s, unsigned max, const char *what) {
    unsigned v = (unsigned)strtoul(s ? s : "0", NULL, 0);
    if (v >= max) {
        printf("Error: %s must be 0..%u\n", what, max - 1);
    }
    return v;
}

int uberclock_int_parse_s(const char *s, int minv, int maxv, const char *what) {
    long v = strtol(s ? s : "0", NULL, 0);
    if (v < minv || v > maxv) {
        printf("Error: %s must be %d..%d\n", what, minv, maxv);
    }
    return (int)v;
}

void uberclock_int_commit(void) {
    cfg_link_commit_write(1);
}

void uberclock_int_write_phase_down(unsigned ch, uint32_t value) {
    if (ch >= 1 && ch <= UBERCLOCK_CHANNELS) {
        g_phase_down_write[ch - 1](value);
    }
}

void uberclock_int_write_phase_cpu(unsigned ch, uint32_t value) {
    if (ch >= 1 && ch <= UBERCLOCK_CHANNELS) {
        g_phase_cpu_write[ch - 1](value);
    }
}

void uberclock_int_write_mag_cpu(unsigned ch, int value) {
    if (ch >= 1 && ch <= UBERCLOCK_CHANNELS) {
        g_mag_cpu_write[ch - 1](pack_s12(value));
    }
}

void uberclock_int_write_gain(unsigned ch, int32_t value) {
    if (ch >= 1 && ch <= UBERCLOCK_CHANNELS) {
        g_gain_write[ch - 1]((uint32_t)value);
    }
}

void uberclock_int_write_output_select(unsigned ch, unsigned value) {
    if (ch >= 1 && ch <= 2) {
        g_output_select_write[ch - 1](value);
    }
}

void uberclock_int_write_nco_mag(int value) {
    main_nco_mag_write(pack_s12(value));
}

void uberclock_int_write_upsampler_inputs_all_x(int16_t value) {
    unsigned ch;
    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        g_ups_x_write[ch](pack_s16(value));
    }
}

void uberclock_int_write_upsampler_inputs_all_y(int16_t value) {
    unsigned ch;
    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        g_ups_y_write[ch](pack_s16(value));
    }
}

void uberclock_int_set_input_select(unsigned value) {
    main_input_select_write(value);
}

void uberclock_int_set_upsampler_input_mux(unsigned value) {
    main_upsampler_input_mux_write(value);
}

void uberclock_int_set_lowspeed_dbg_select(unsigned value) {
    main_lowspeed_dbg_select_write(value);
}

void uberclock_int_set_highspeed_dbg_select(unsigned value) {
    main_highspeed_dbg_select_write(value);
}

void uberclock_int_set_final_shift(int32_t value) {
    main_final_shift_write((uint32_t)value);
}

void uberclock_int_set_cap_enable(unsigned value) {
    main_cap_enable_write(value ? 1u : 0u);
}

void uberclock_int_set_cap_beats(uint32_t value) {
    main_cap_beats_write(value);
}

void uberclock_int_ups_fifo_write_frame(const iq5_frame_t *frame) {
    unsigned ch;
    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        g_ups_fifo_x_write[ch](pack_s16(frame->x[ch]));
        g_ups_fifo_y_write[ch](pack_s16(frame->y[ch]));
    }
    main_ups_fifo_push_write(1);
}

void uberclock_int_ups_fifo_write_replicated(int16_t x, int16_t y) {
    iq5_frame_t frame = {
        .x = {x, x, x, x, x},
        .y = {y, y, y, y, y},
    };
    uberclock_int_ups_fifo_write_frame(&frame);
}

void uberclock_int_ds_fifo_read_frame(iq5_frame_t *frame) {
    unsigned ch;
    main_ds_fifo_pop_write(1);
    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        frame->x[ch] = (int16_t)(g_ds_fifo_x_read[ch]() & 0xffffu);
        frame->y[ch] = (int16_t)(g_ds_fifo_y_read[ch]() & 0xffffu);
    }
}

void uberclock_int_fifo_clear_flags(void) {
    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);
}

int uberclock_int_ds_wait_readable(unsigned limit) {
    while (limit--) {
        if ((main_ds_fifo_flags_read() & 0x1u) != 0u) {
            return 1;
        }
    }
    return 0;
}

void uberclock_int_pulse_cap_arm(void) {
    main_cap_arm_write(0);
    uberclock_int_commit();
    main_cap_arm_write(1);
    uberclock_int_commit();
    main_cap_arm_write(0);
    uberclock_int_commit();
}

unsigned uberclock_int_read_cap_done(void) {
    return main_cap_done_read() & 1u;
}

int16_t uberclock_int_read_cap_sample(unsigned idx) {
    main_cap_idx_write(idx);
    uberclock_int_commit();
    (void)main_cap_data_read();
    return (int16_t)(main_cap_data_read() & 0xffffu);
}

unsigned uberclock_int_read_ds_flags(void) {
    return (unsigned)(main_ds_fifo_flags_read() & 0xffu);
}

unsigned uberclock_int_read_ds_overflow(void) {
    return (unsigned)(main_ds_fifo_overflow_read() & 1u);
}

unsigned uberclock_int_read_ds_underflow(void) {
    return (unsigned)(main_ds_fifo_underflow_read() & 1u);
}

void uberclock_int_clear_ds_flags(void) {
    main_ds_fifo_clear_write(1);
}

unsigned uberclock_int_read_ups_flags(void) {
    return (unsigned)(main_ups_fifo_flags_read() & 0xffu);
}

unsigned uberclock_int_read_ups_overflow(void) {
    return (unsigned)(main_ups_fifo_overflow_read() & 1u);
}

unsigned uberclock_int_read_ups_underflow(void) {
    return (unsigned)(main_ups_fifo_underflow_read() & 1u);
}

void uberclock_int_clear_ups_flags(void) {
    main_ups_fifo_clear_write(1);
}
