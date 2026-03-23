#include "uberclock_core.h"

static void cap_start_cmd(char *a) {
    (void)a;
    main_cap_arm_write(0);
    uc_commit();

    main_cap_arm_write(1);
    uc_commit();

    main_cap_arm_write(0);
    uc_commit();

    puts("Capture started.");
}

static void cap_status_cmd(char *a) {
    (void)a;
    unsigned d = main_cap_done_read();
    printf("Capture %s\n", d ? "DONE" : "IN-PROGRESS");
}

static void cap_dump_cmd(char *a) {
    (void)a;
    if (!main_cap_done_read()) {
        puts("Capture not done yet. Use 'cap_status' or wait.");
        return;
    }

    puts("#idx,value");
    for (unsigned i = 0; i < 2048; ++i) {
        main_cap_idx_write(i);
        uc_commit();
        (void)main_cap_data_read();
        int16_t v = (int16_t)main_cap_data_read();
        printf("%u,%d\n", i, v);
    }
}


static void cmd_cap_arm_pulse(char *a) {
    (void)a;

    main_cap_arm_write(0);
    uc_commit();

    main_cap_arm_write(1);
    uc_commit();

    main_cap_arm_write(0);
    uc_commit();

    puts("cap_arm pulsed");
}

static void cmd_cap_done(char *a) {
    (void)a;
    printf("cap_done = %u\n", (unsigned)(main_cap_done_read() & 1u));
}

static void cmd_cap_rd(char *args) {
    char *tok = strtok(args, " \t");
    if (!tok) { puts("Usage: cap_rd <idx>"); return; }

    unsigned idx = (unsigned)strtoul(tok, NULL, 0);
    if (idx > 2047) { puts("idx must be 0..2047"); return; }

    main_cap_idx_write(idx);
    uc_commit();

    /* dummy read to allow CDC/update latency */
    (void)main_cap_data_read();
    uint32_t v = main_cap_data_read();

    int16_t s = (int16_t)(v & 0xffff);
    printf("cap[%u] = %d (0x%04x)\n", idx, (int)s, (unsigned)(v & 0xffff));
}

static void cmd_cap_enable(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void cmd_cap_beats(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "256", NULL, 0);
    if (v == 0u) { puts("cap_beats must be >= 1"); return; }
    main_cap_beats_write(v);
    uc_commit();
    printf("cap_beats = %u\n", (unsigned)v);
}

const struct cmd_entry uberclock_capture_cmds[] = {
    {"cap_arm", cmd_cap_arm_pulse, "Pulse cap_arm"},
    {"cap_done", cmd_cap_done, "Read cap_done"},
    {"cap_rd", cmd_cap_rd, "Read cap_data at index"},
    {"cap_enable", cmd_cap_enable, "0=ramp, 1=capture design to DDR"},
    {"cap_beats", cmd_cap_beats, "Set capture length in 256-bit beats"},
    {"cap_start", cap_start_cmd, "Start LS debug"},
    {"cap_status", cap_status_cmd, "LS debug status"},
    {"cap_dump", cap_dump_cmd, "LS debug dump"},
};

const unsigned uberclock_capture_cmd_count =
    (unsigned)(sizeof(uberclock_capture_cmds) / sizeof(uberclock_capture_cmds[0]));
