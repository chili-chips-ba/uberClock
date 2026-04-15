#include <stdio.h>
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_capture.h"

void uberclock_capture_start(void) {
    uberclock_capture_arm_pulse();
    puts("Capture started.");
}

void uberclock_capture_status(void) {
    printf("Capture %s\n", uberclock_capture_done() ? "DONE" : "IN-PROGRESS");
}

void uberclock_capture_dump(void) {
    unsigned index;

    if (!uberclock_capture_done()) {
        puts("Capture not done yet. Use 'cap_status' or wait.");
        return;
    }

    puts("#idx,value");
    for (index = 0u; index < UBERCLOCK_CAPTURE_SAMPLE_COUNT; ++index) {
        printf("%u,%d\n", index, uberclock_capture_read_sample(index));
    }
}

void uberclock_capture_print_done(void) {
    printf("cap_done = %u\n", uberclock_capture_done());
}

void uberclock_capture_print_sample(unsigned index) {
    int16_t value = uberclock_capture_read_sample(index);
    printf("cap[%u] = %d (0x%04x)\n", index, (int)value, (unsigned)((uint16_t)value));
}

