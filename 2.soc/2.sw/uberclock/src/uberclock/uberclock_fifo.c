#include <stdio.h>
#include <generated/csr.h>
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_runtime.h"

static void pop_downsample_fifo_frame(void) {
    main_ds_fifo_pop_write(1);
}

static void read_downsample_fifo_frame(int16_t *sample_x, int16_t *sample_y, int capture_style) {
    uint32_t first_x;
    uint32_t first_y;
    uint32_t second_x;
    uint32_t second_y;

    first_x = main_ds_fifo_x_read();
    first_y = main_ds_fifo_y_read();
    second_x = main_ds_fifo_x_read();
    second_y = main_ds_fifo_y_read();

    if (capture_style) {
        *sample_x = (int16_t)(second_x & 0xffffu);
        *sample_y = (int16_t)(second_y & 0xffffu);
    } else {
        *sample_x = (int16_t)(first_x & 0xffffu);
        *sample_y = (int16_t)(first_y & 0xffffu);
    }
}

unsigned uberclock_ds_fifo_flush(unsigned max_samples) {
    unsigned flushed = 0u;
    int16_t sample_x;
    int16_t sample_y;

    while (flushed < max_samples && (main_ds_fifo_flags_read() & 0x1u) != 0u) {
        pop_downsample_fifo_frame();
        read_downsample_fifo_frame(&sample_x, &sample_y, 1);
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
    if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        return 0;
    }

    pop_downsample_fifo_frame();
    read_downsample_fifo_frame(sample_x, sample_y, 0);
    return 1;
}

int uberclock_ds_fifo_pop_capture(int16_t *sample_x, int16_t *sample_y) {
    if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        return 0;
    }

    pop_downsample_fifo_frame();
    read_downsample_fifo_frame(sample_x, sample_y, 1);
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
    unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);

    if (((flags >> 1) & 1u) == 0u) {
        return 0;
    }

    main_ups_fifo_x_write((uint32_t)((int32_t)sample_x & 0xffff));
    main_ups_fifo_y_write((uint32_t)((int32_t)sample_y & 0xffff));
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
