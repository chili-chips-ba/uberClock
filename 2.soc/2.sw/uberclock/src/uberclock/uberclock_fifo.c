// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <generated/csr.h>
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_runtime.h"

static void pop_downsample_fifo_frame(void) {
    main_ds_fifo_pop_write(1);
}

static void read_downsample_fifo_frame(struct uberclock_iq_frame *frame) {
    frame->x[0] = (int16_t)(main_ds_fifo_x1_read() & 0xffffu);
    frame->y[0] = (int16_t)(main_ds_fifo_y1_read() & 0xffffu);
    frame->x[1] = (int16_t)(main_ds_fifo_x2_read() & 0xffffu);
    frame->y[1] = (int16_t)(main_ds_fifo_y2_read() & 0xffffu);
    frame->x[2] = (int16_t)(main_ds_fifo_x3_read() & 0xffffu);
    frame->y[2] = (int16_t)(main_ds_fifo_y3_read() & 0xffffu);
    frame->x[3] = (int16_t)(main_ds_fifo_x4_read() & 0xffffu);
    frame->y[3] = (int16_t)(main_ds_fifo_y4_read() & 0xffffu);
    frame->x[4] = (int16_t)(main_ds_fifo_x5_read() & 0xffffu);
    frame->y[4] = (int16_t)(main_ds_fifo_y5_read() & 0xffffu);
}

unsigned uberclock_ds_fifo_flush(unsigned max_samples) {
    unsigned flushed = 0u;
    struct uberclock_iq_frame frame;

    while (flushed < max_samples && (main_ds_fifo_flags_read() & 0x1u) != 0u) {
        pop_downsample_fifo_frame();
        read_downsample_fifo_frame(&frame);
        ++flushed;
        uberclock_runtime_service_ce_events(4u);
    }

    return flushed;
}

int uberclock_wait_for_ds_sample(const char *phase_name, unsigned sample_index, unsigned total_samples) {
    unsigned stall = 0u;

    while ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        uberclock_runtime_service_ce_event();
        ++stall;
        if (stall >= UBERCLOCK_TRACK_FIFO_WAIT_POLLS) {
            printf("track3 %s timeout at sample %u/%u after %u polls\n",
                   phase_name,
                   sample_index,
                   total_samples,
                   stall);
            return 0;
        }
    }

    return 1;
}

int uberclock_ds_fifo_pop_simple(int16_t *sample_x, int16_t *sample_y) {
    struct uberclock_iq_frame frame;

    if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        return 0;
    }

    pop_downsample_fifo_frame();
    read_downsample_fifo_frame(&frame);
    *sample_x = frame.x[0];
    *sample_y = frame.y[0];
    return 1;
}

int uberclock_ds_fifo_pop_capture(int16_t *sample_x, int16_t *sample_y) {
    struct uberclock_iq_frame frame;

    if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        return 0;
    }

    pop_downsample_fifo_frame();
    read_downsample_fifo_frame(&frame);
    *sample_x = frame.x[0];
    *sample_y = frame.y[0];
    return 1;
}

int uberclock_ds_fifo_pop_frame(struct uberclock_iq_frame *frame) {
    if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        return 0;
    }

    pop_downsample_fifo_frame();
    read_downsample_fifo_frame(frame);
    return 1;
}

unsigned uberclock_dsp_pump_step(unsigned max_in, unsigned max_out) {
    unsigned popped = 0u;
    unsigned index;
    int16_t input_x;
    int16_t input_y;

    (void)max_out;

    for (index = 0u; index < max_in; ++index) {
        if (!uberclock_ds_fifo_pop_capture(&input_x, &input_y)) {
            break;
        }
        ++popped;
    }

    return popped;
}

int uberclock_ups_fifo_push(int16_t sample_x, int16_t sample_y) {
    struct uberclock_iq_frame frame = {
        .x = {sample_x, sample_x, sample_x, sample_x, sample_x},
        .y = {sample_y, sample_y, sample_y, sample_y, sample_y}
    };

    return uberclock_ups_fifo_push_frame(&frame);
}

int uberclock_ups_fifo_push_frame(const struct uberclock_iq_frame *frame) {
    unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);

    if (((flags >> 1) & 1u) == 0u) {
        return 0;
    }

    main_ups_fifo_x1_write((uint32_t)((int32_t)frame->x[0] & 0xffff));
    main_ups_fifo_y1_write((uint32_t)((int32_t)frame->y[0] & 0xffff));
    main_ups_fifo_x2_write((uint32_t)((int32_t)frame->x[1] & 0xffff));
    main_ups_fifo_y2_write((uint32_t)((int32_t)frame->y[1] & 0xffff));
    main_ups_fifo_x3_write((uint32_t)((int32_t)frame->x[2] & 0xffff));
    main_ups_fifo_y3_write((uint32_t)((int32_t)frame->y[2] & 0xffff));
    main_ups_fifo_x4_write((uint32_t)((int32_t)frame->x[3] & 0xffff));
    main_ups_fifo_y4_write((uint32_t)((int32_t)frame->y[3] & 0xffff));
    main_ups_fifo_x5_write((uint32_t)((int32_t)frame->x[4] & 0xffff));
    main_ups_fifo_y5_write((uint32_t)((int32_t)frame->y[4] & 0xffff));
    main_ups_fifo_push_write(1);
    return 1;
}

unsigned uberclock_ds_fifo_flags(void) {
    return (unsigned)(main_ds_fifo_flags_read() & 0xffu);
}

unsigned uberclock_ds_fifo_overflow(void) {
    return (unsigned)(main_ds_fifo_overflow_read() & 1u);
}

unsigned uberclock_ds_fifo_underflow(void) {
    return (unsigned)(main_ds_fifo_underflow_read() & 1u);
}

void uberclock_ds_fifo_clear_status(void) {
    main_ds_fifo_clear_write(1);
}

unsigned uberclock_ups_fifo_flags(void) {
    return (unsigned)(main_ups_fifo_flags_read() & 0xffu);
}

unsigned uberclock_ups_fifo_overflow(void) {
    return (unsigned)(main_ups_fifo_overflow_read() & 1u);
}

unsigned uberclock_ups_fifo_underflow(void) {
    return (unsigned)(main_ups_fifo_underflow_read() & 1u);
}

void uberclock_ups_fifo_clear_status(void) {
    main_ups_fifo_clear_write(1);
}
