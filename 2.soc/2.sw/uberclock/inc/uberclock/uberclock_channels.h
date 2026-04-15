/**
 * @file uberclock_channels.h
 * @brief Per-channel configuration API.
 */

#ifndef UBERCLOCK_CHANNELS_H
#define UBERCLOCK_CHANNELS_H

#include <stdint.h>

/**
 * @brief Set downconversion phase increment for a channel.
 */
int uberclock_channel_set_phase_down(unsigned channel_index, uint32_t phase_increment);

/**
 * @brief Get downconversion phase increment.
 */
uint32_t uberclock_channel_get_phase_down(unsigned channel_index);

/**
 * @brief Set CPU-generated phase increment.
 */
int uberclock_channel_set_phase_cpu(unsigned channel_index, uint32_t phase_increment);

/**
 * @brief Set CPU-generated magnitude.
 */
int uberclock_channel_set_magnitude_cpu(unsigned channel_index, int16_t magnitude);

/**
 * @brief Set channel gain.
 */
int uberclock_channel_set_gain(unsigned channel_index, int32_t gain);

/**
 * @brief Apply default initialization to all channels.
 */
void uberclock_channels_apply_default_init(void);

#endif
