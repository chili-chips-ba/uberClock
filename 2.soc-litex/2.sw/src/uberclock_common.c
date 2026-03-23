#include "uberclock_core.h"

int is_pow2_u(unsigned x) {
    return x && ((x & (x - 1u)) == 0u);
}

unsigned parse_u(const char *s, unsigned max, const char *what) {
    unsigned v = (unsigned)strtoul(s ? s : "0", NULL, 0);
    if (v >= max) {
        printf("Error: %s must be 0..%u\n", what, max - 1u);
    }
    return v;
}

int parse_s(const char *s, int minv, int maxv, const char *what) {
    long v = strtol(s ? s : "0", NULL, 0);
    if (v < minv || v > maxv) {
        printf("Error: %s must be %d..%d\n", what, minv, maxv);
    }
    return (int)v;
}

void uc_commit(void) {
    cfg_link_commit_write(1);
}
