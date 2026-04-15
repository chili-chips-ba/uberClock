/**
 * @file uberclock_capture.h
 * @brief Low-speed capture interface.
 */

#ifndef UBERCLOCK_CAPTURE_H
#define UBERCLOCK_CAPTURE_H

#include <stdint.h>

/**
 * @brief Start a capture operation.
 */
void uberclock_capture_start(void);

/**
 * @brief Print capture status.
 */
void uberclock_capture_status(void);

/**
 * @brief Dump all captured samples.
 */
void uberclock_capture_dump(void);

/**
 * @brief Print capture completion status.
 */
void uberclock_capture_print_done(void);

/**
 * @brief Print a single captured sample.
 *
 * @param index Sample index
 */
void uberclock_capture_print_sample(unsigned index);

#endif
