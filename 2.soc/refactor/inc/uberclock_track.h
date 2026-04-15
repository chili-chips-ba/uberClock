#ifndef UBERCLOCK_TRACK_H
#define UBERCLOCK_TRACK_H

#include <stdint.h>

void uberclock_track_poll(void);
void uberclock_track_stop(void);
int uberclock_track3_run(uint32_t start_hz,
                         uint32_t step_hz,
                         unsigned max_steps,
                         unsigned sample_count,
                         uint32_t center_hz,
                         uint32_t delta_hz);
int uberclock_trackq_start(unsigned sample_count, uint32_t center_hz, uint32_t delta_hz);
int uberclock_trackq_probe(unsigned sample_count, uint32_t center_hz, uint32_t delta_hz);

#endif

