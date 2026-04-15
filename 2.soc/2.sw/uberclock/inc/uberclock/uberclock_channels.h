#ifndef UBERCLOCK_CHANNELS_H
#define UBERCLOCK_CHANNELS_H

#include <stdint.h>

int uberclock_channel_set_phase_down(unsigned channel_index, uint32_t phase_increment);
uint32_t uberclock_channel_get_phase_down(unsigned channel_index);
int uberclock_channel_set_phase_cpu(unsigned channel_index, uint32_t phase_increment);
int uberclock_channel_set_magnitude_cpu(unsigned channel_index, int16_t magnitude);
int uberclock_channel_set_gain(unsigned channel_index, int32_t gain);
void uberclock_channels_apply_default_init(void);

#endif

