#include <stdio.h>
#include <stdlib.h>
#include "uberclock/uberclock_parse.h"

unsigned uberclock_parse_unsigned(const char *text, unsigned max_value_exclusive, const char *what) {
    unsigned value = (unsigned)strtoul(text ? text : "0", NULL, 0);
    if (value >= max_value_exclusive) {
        printf("Error: %s must be 0..%u\n", what, max_value_exclusive - 1u);
    }
    return value;
}

int uberclock_parse_signed(const char *text, int min_value, int max_value, const char *what) {
    long value = strtol(text ? text : "0", NULL, 0);
    if (value < min_value || value > max_value) {
        printf("Error: %s must be %d..%d\n", what, min_value, max_value);
    }
    return (int)value;
}

int uberclock_parse_ipv4(const char *text, uint32_t *out_ip) {
    unsigned octet0;
    unsigned octet1;
    unsigned octet2;
    unsigned octet3;

    if (!text || !out_ip) {
        return -1;
    }
    if (sscanf(text, "%u.%u.%u.%u", &octet0, &octet1, &octet2, &octet3) != 4) {
        return -1;
    }
    if (octet0 > 255u || octet1 > 255u || octet2 > 255u || octet3 > 255u) {
        return -1;
    }

    *out_ip = ((uint32_t)octet0 << 24) |
    ((uint32_t)octet1 << 16) |
    ((uint32_t)octet2 << 8) |
    (uint32_t)octet3;
    return 0;
}
