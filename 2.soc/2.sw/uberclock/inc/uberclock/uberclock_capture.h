#ifndef UBERCLOCK_CAPTURE_H
#define UBERCLOCK_CAPTURE_H

#include <stdint.h>

void uberclock_capture_start(void);
void uberclock_capture_status(void);
void uberclock_capture_dump(void);
void uberclock_capture_print_done(void);
void uberclock_capture_print_sample(unsigned index);

#endif

