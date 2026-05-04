#include <generated/csr.h>
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_channels.h"

static const uint32_t default_phase_down[UBERCLOCK_CHANNEL_COUNT] = {
    11321544u, 80652u, 80648u, 80644u, 80640u
};

static const uint32_t default_phase_cpu[UBERCLOCK_CHANNEL_COUNT] = {
    52429u, 52429u, 52429u, 52429u, 52429u
};

static const int16_t default_magnitude_cpu[UBERCLOCK_CHANNEL_COUNT] = {
    0, 0, 0, 0, 0
};

static const int32_t default_gain[UBERCLOCK_CHANNEL_COUNT] = {
    0x40000000, 0, 0, 0, 0
};

int uberclock_channel_set_phase_down(unsigned channel_index, uint32_t phase_increment) {
    switch (channel_index) {
        case 0u: main_phase_inc_down_1_write(phase_increment); return 0;
        case 1u: main_phase_inc_down_2_write(phase_increment); return 0;
        case 2u: main_phase_inc_down_3_write(phase_increment); return 0;
        case 3u: main_phase_inc_down_4_write(phase_increment); return 0;
        case 4u: main_phase_inc_down_5_write(phase_increment); return 0;
        default: return -1;
    }
}

uint32_t uberclock_channel_get_phase_down(unsigned channel_index) {
    switch (channel_index) {
        case 0u: return main_phase_inc_down_1_read();
        case 1u: return main_phase_inc_down_2_read();
        case 2u: return main_phase_inc_down_3_read();
        case 3u: return main_phase_inc_down_4_read();
        case 4u: return main_phase_inc_down_5_read();
        default: return 0u;
    }
}

int uberclock_channel_set_phase_cpu(unsigned channel_index, uint32_t phase_increment) {
    switch (channel_index) {
        case 0u: main_phase_inc_cpu1_write(phase_increment); return 0;
        case 1u: main_phase_inc_cpu2_write(phase_increment); return 0;
        case 2u: main_phase_inc_cpu3_write(phase_increment); return 0;
        case 3u: main_phase_inc_cpu4_write(phase_increment); return 0;
        case 4u: main_phase_inc_cpu5_write(phase_increment); return 0;
        default: return -1;
    }
}

int uberclock_channel_set_magnitude_cpu(unsigned channel_index, int16_t magnitude) {
    uint32_t encoded_magnitude = (uint32_t)((int32_t)magnitude & 0x0fff);

    switch (channel_index) {
        case 0u: main_mag_cpu1_write(encoded_magnitude); return 0;
        case 1u: main_mag_cpu2_write(encoded_magnitude); return 0;
        case 2u: main_mag_cpu3_write(encoded_magnitude); return 0;
        case 3u: main_mag_cpu4_write(encoded_magnitude); return 0;
        case 4u: main_mag_cpu5_write(encoded_magnitude); return 0;
        default: return -1;
    }
}

int uberclock_channel_set_gain(unsigned channel_index, int32_t gain) {
    switch (channel_index) {
        case 0u: main_gain1_write((uint32_t)gain); return 0;
        case 1u: main_gain2_write((uint32_t)gain); return 0;
        case 2u: main_gain3_write((uint32_t)gain); return 0;
        case 3u: main_gain4_write((uint32_t)gain); return 0;
        case 4u: main_gain5_write((uint32_t)gain); return 0;
        default: return -1;
    }
}

void uberclock_channels_apply_default_init(void) {
    unsigned channel_index;

    for (channel_index = 0u; channel_index < UBERCLOCK_CHANNEL_COUNT; ++channel_index) {
        (void)uberclock_channel_set_phase_down(channel_index, default_phase_down[channel_index]);
        (void)uberclock_channel_set_phase_cpu(channel_index, default_phase_cpu[channel_index]);
        (void)uberclock_channel_set_magnitude_cpu(channel_index, default_magnitude_cpu[channel_index]);
        (void)uberclock_channel_set_gain(channel_index, default_gain[channel_index]);
    }
}
