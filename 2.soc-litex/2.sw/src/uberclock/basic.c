#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>

#include "uberclock/uberclock_internal.h"

static void uberclock_int_help(char *args) {
    (void)args;
    puts_help_header("UberClock commands");
    puts("  phase_nco <val>, nco_mag <val>");
    puts("  phase_down_1..5 <val>, phase_down_ref <val>");
    puts("  phase_cpu1..5 <val>, mag_cpu1..5 <val>");
    puts("  input_select <0..3>, upsampler_input_mux <0..2>");
    puts("  output_select_ch1/ch2 <0..15>, gain1..5 <int32>, final_shift <0..7>");
    puts("  lowspeed_dbg_select <0..7>, highspeed_dbg_select <0..3>");
    puts("  upsampler_x <val>, upsampler_y <val>");
    puts("  ds_pop, ds_status, ups_push <x> <y>, ups_status");
    puts("  fft_fs, fft_ds, fft_ds_peak, fft32_ds_y, fft64_peak, track_mode");
    puts("  cap_arm, cap_done, cap_rd <idx>, cap_enable <0|1>, cap_beats <N>");
    puts("  magnitude, phase");
    puts("  sig3_start, sig3_stop, sig3_amp, sig3_freqs, sig3_enable_ch, sig3_disable_ch");
    puts("  prv_help, ub_info, ub_mode, ub_setmode, ub_start, ub_ramp, ub_cap, ub_wait, ub_hexdump, ub_send");
    puts("");
}

static void prv_cmd_phase_nco(char *a) {
    unsigned p = uberclock_int_parse_u(a, 1u << 26, "phase_nco");
    if (p >= (1u << 26)) return;
    main_phase_inc_nco_write(p);
    uberclock_int_commit();
    printf("Input NCO phase increment set to %u\n", p);
}

static void prv_cmd_phase_cpu(char *a, unsigned ch) {
    unsigned p = uberclock_int_parse_u(a, 1u << 26, "phase_cpu");
    if (p >= (1u << 26)) return;
    uberclock_int_write_phase_cpu(ch, p);
    uberclock_int_commit();
    printf("CPU phase increment ch%u set to %u\n", ch, p);
}

static void prv_cmd_phase_cpu1(char *a) { prv_cmd_phase_cpu(a, 1); }
static void prv_cmd_phase_cpu2(char *a) { prv_cmd_phase_cpu(a, 2); }
static void prv_cmd_phase_cpu3(char *a) { prv_cmd_phase_cpu(a, 3); }
static void prv_cmd_phase_cpu4(char *a) { prv_cmd_phase_cpu(a, 4); }
static void prv_cmd_phase_cpu5(char *a) { prv_cmd_phase_cpu(a, 5); }

static void prv_cmd_nco_mag(char *a) {
    int v = uberclock_int_parse_s(a, -2048, 2047, "nco_mag");
    if (v < -2048 || v > 2047) return;
    uberclock_int_write_nco_mag(v);
    uberclock_int_commit();
    printf("nco_mag set to %d\n", v);
}

static void prv_cmd_phase_down_ref(char *a) {
    unsigned p = uberclock_int_parse_u(a, 1u << 26, "phase_down_ref");
    if (p >= (1u << 26)) return;
    main_phase_inc_down_ref_write(p);
    uberclock_int_commit();
    printf("Downconversion phase ref increment set to %u\n", p);
}

static void prv_cmd_mag_cpu(char *a, unsigned ch) {
    int v = uberclock_int_parse_s(a, -2048, 2047, "mag_cpu");
    if (v < -2048 || v > 2047) return;
    uberclock_int_write_mag_cpu(ch, v);
    uberclock_int_commit();
    printf("mag_cpu%u set to %d\n", ch, v);
}

static void prv_cmd_mag_cpu1(char *a) { prv_cmd_mag_cpu(a, 1); }
static void prv_cmd_mag_cpu2(char *a) { prv_cmd_mag_cpu(a, 2); }
static void prv_cmd_mag_cpu3(char *a) { prv_cmd_mag_cpu(a, 3); }
static void prv_cmd_mag_cpu4(char *a) { prv_cmd_mag_cpu(a, 4); }
static void prv_cmd_mag_cpu5(char *a) { prv_cmd_mag_cpu(a, 5); }

static void prv_cmd_lowspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 7) {
        puts("lowspeed_dbg_select must be 0..7");
        return;
    }
    uberclock_int_set_lowspeed_dbg_select(v);
    uberclock_int_commit();
    printf("lowspeed_dbg_select = %u\n", v);
}

static void prv_cmd_highspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 3) {
        puts("highspeed_dbg_select must be 0..3");
        return;
    }
    uberclock_int_set_highspeed_dbg_select(v);
    uberclock_int_commit();
    printf("highspeed_dbg_select = %u\n", v);
}

static void prv_cmd_phase_dn(char *a, unsigned ch) {
    unsigned p = uberclock_int_parse_u(a, 1u << 26, "phase_down");
    if (p >= (1u << 26)) return;
    uberclock_int_write_phase_down(ch, p);
    uberclock_int_commit();
    printf("Downconversion phase ch%u increment set to %u\n", ch, p);
}

static void prv_cmd_phase_down_1(char *a) { prv_cmd_phase_dn(a, 1); }
static void prv_cmd_phase_down_2(char *a) { prv_cmd_phase_dn(a, 2); }
static void prv_cmd_phase_down_3(char *a) { prv_cmd_phase_dn(a, 3); }
static void prv_cmd_phase_down_4(char *a) { prv_cmd_phase_dn(a, 4); }
static void prv_cmd_phase_down_5(char *a) { prv_cmd_phase_dn(a, 5); }

static void prv_cmd_output_sel_ch1(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    uberclock_int_write_output_select(1, v);
    uberclock_int_commit();
    printf("output_select_ch1 set to %u\n", v);
}

static void prv_cmd_output_sel_ch2(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    uberclock_int_write_output_select(2, v);
    uberclock_int_commit();
    printf("output_select_ch2 set to %u\n", v);
}

static void prv_cmd_input_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    uberclock_int_set_input_select(v);
    uberclock_int_commit();
    printf("Main input select register set to %u\n", v);
}

static void prv_cmd_ups_in_mux(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    uberclock_int_set_upsampler_input_mux(v);
    uberclock_int_commit();
    printf("Upsampler input mux register set to %u\n", v);
}

static void prv_cmd_gain(char *a, unsigned idx) {
    int32_t g = (int32_t)strtol(a ? a : "0", NULL, 0);
    uberclock_int_write_gain(idx, g);
    uberclock_int_commit();
    printf("Gain%u register set to %ld (0x%08lX)\n", idx, (long)g, (unsigned long)g);
}

static void prv_cmd_gain1(char *a) { prv_cmd_gain(a, 1); }
static void prv_cmd_gain2(char *a) { prv_cmd_gain(a, 2); }
static void prv_cmd_gain3(char *a) { prv_cmd_gain(a, 3); }
static void prv_cmd_gain4(char *a) { prv_cmd_gain(a, 4); }
static void prv_cmd_gain5(char *a) { prv_cmd_gain(a, 5); }

static void prv_cmd_final_shift(char *a) {
    int32_t fs = (int32_t)strtol(a ? a : "0", NULL, 0);
    uberclock_int_set_final_shift(fs);
    uberclock_int_commit();
    printf("final_shift set to %ld (0x%08lX)\n", (long)fs, (unsigned long)fs);
}

static void prv_cmd_cap_enable(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    uberclock_int_set_cap_enable(v);
    uberclock_int_commit();
    printf("cap_enable = %u (%s)\n", v ? 1u : 0u, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void prv_cmd_upsampler_x(char *a) {
    int v = uberclock_int_parse_s(a, -32768, 32767, "upsampler_x");
    if (v < -32768 || v > 32767) return;
    uberclock_int_write_upsampler_inputs_all_x((int16_t)v);
    uberclock_int_commit();
    printf("upsampler_input_x[1..5] = %d\n", v);
}

static void prv_cmd_upsampler_y(char *a) {
    int v = uberclock_int_parse_s(a, -32768, 32767, "upsampler_y");
    if (v < -32768 || v > 32767) return;
    uberclock_int_write_upsampler_inputs_all_y((int16_t)v);
    uberclock_int_commit();
    printf("upsampler_input_y[1..5] = %d\n", v);
}

static void prv_cmd_cap_arm_pulse(char *a) {
    (void)a;
    uberclock_int_pulse_cap_arm();
    puts("cap_arm pulsed");
}

static void prv_cmd_cap_done(char *a) {
    (void)a;
    printf("cap_done = %u\n", uberclock_int_read_cap_done());
}

static void prv_cmd_cap_rd(char *args) {
    char *tok = strtok(args, " \t");
    unsigned idx;
    int16_t s;

    if (!tok) {
        puts("Usage: cap_rd <idx>");
        return;
    }

    idx = (unsigned)strtoul(tok, NULL, 0);
    if (idx > 2047) {
        puts("idx must be 0..2047");
        return;
    }

    s = uberclock_int_read_cap_sample(idx);
    printf("cap[%u] = %d (0x%04x)\n", idx, (int)s, (unsigned)((uint16_t)s));
}

static void prv_cmd_phase_print(char *a) {
    (void)a;
    printf("Phase %ld\n", (long)uberclock_runtime_get_phase());
}

static void prv_cmd_magnitude(char *a) {
    (void)a;
    printf("Magnitude %d\n", uberclock_runtime_get_magnitude());
}

static void prv_cmd_ds_pop(char *a) {
    iq5_frame_t frame;
    (void)a;

    uberclock_int_ds_fifo_read_frame(&frame);
    printf("ds_fifo:"
           " ch1=(%d,%d)"
           " ch2=(%d,%d)"
           " ch3=(%d,%d)"
           " ch4=(%d,%d)"
           " ch5=(%d,%d)\n",
           (int)frame.x[0], (int)frame.y[0],
           (int)frame.x[1], (int)frame.y[1],
           (int)frame.x[2], (int)frame.y[2],
           (int)frame.x[3], (int)frame.y[3],
           (int)frame.x[4], (int)frame.y[4]);
}

static void prv_cmd_ds_status(char *a) {
    unsigned flags;
    unsigned overflow;
    unsigned underflow;
    (void)a;

    flags = uberclock_int_read_ds_flags();
    overflow = uberclock_int_read_ds_overflow();
    underflow = uberclock_int_read_ds_underflow();
    printf("ds_fifo: readable=%u overflow=%u underflow=%u\n", flags & 1u, overflow, underflow);
    uberclock_int_clear_ds_flags();
}

static void prv_cmd_ups_push(char *args) {
    char *tokx = strtok(args, " \t");
    char *toky = strtok(NULL, " \t");
    int x;
    int y;

    if (!tokx || !toky) {
        puts("Usage: ups_push <x> <y>");
        return;
    }

    x = uberclock_int_parse_s(tokx, -32768, 32767, "ups_x");
    y = uberclock_int_parse_s(toky, -32768, 32767, "ups_y");
    if (x < -32768 || x > 32767 || y < -32768 || y > 32767) return;

    uberclock_int_ups_fifo_write_replicated((int16_t)x, (int16_t)y);
    printf("ups_fifo push: replicated x=%d y=%d to ch1..ch5\n", x, y);
}

static void prv_cmd_ups_status(char *a) {
    unsigned flags;
    unsigned overflow;
    unsigned underflow;
    (void)a;

    flags = uberclock_int_read_ups_flags();
    overflow = uberclock_int_read_ups_overflow();
    underflow = uberclock_int_read_ups_underflow();
    printf("ups_fifo: writable=%u overflow=%u underflow=%u\n",
           (flags >> 1) & 1u, overflow, underflow);
    uberclock_int_clear_ups_flags();
}

static const struct cmd_entry g_basic_cmds[] = {
    {"help_uc",              uberclock_int_help,                 "UberClock help"},
    {"phase_nco",            prv_cmd_phase_nco,           "Set input CORDIC NCO phase increment"},
    {"nco_mag",              prv_cmd_nco_mag,             "Set NCO magnitude (signed 12-bit)"},
    {"phase_down_1",         prv_cmd_phase_down_1,        "Set downconversion ch1 phase inc"},
    {"phase_down_2",         prv_cmd_phase_down_2,        "Set downconversion ch2 phase inc"},
    {"phase_down_3",         prv_cmd_phase_down_3,        "Set downconversion ch3 phase inc"},
    {"phase_down_4",         prv_cmd_phase_down_4,        "Set downconversion ch4 phase inc"},
    {"phase_down_5",         prv_cmd_phase_down_5,        "Set downconversion ch5 phase inc"},
    {"phase_down_ref",       prv_cmd_phase_down_ref,      "Set downconversion ref phase inc"},
    {"phase_cpu1",           prv_cmd_phase_cpu1,          "Set CPU NCO phase inc ch1"},
    {"phase_cpu2",           prv_cmd_phase_cpu2,          "Set CPU NCO phase inc ch2"},
    {"phase_cpu3",           prv_cmd_phase_cpu3,          "Set CPU NCO phase inc ch3"},
    {"phase_cpu4",           prv_cmd_phase_cpu4,          "Set CPU NCO phase inc ch4"},
    {"phase_cpu5",           prv_cmd_phase_cpu5,          "Set CPU NCO phase inc ch5"},
    {"mag_cpu1",             prv_cmd_mag_cpu1,            "Set CPU NCO magnitude ch1"},
    {"mag_cpu2",             prv_cmd_mag_cpu2,            "Set CPU NCO magnitude ch2"},
    {"mag_cpu3",             prv_cmd_mag_cpu3,            "Set CPU NCO magnitude ch3"},
    {"mag_cpu4",             prv_cmd_mag_cpu4,            "Set CPU NCO magnitude ch4"},
    {"mag_cpu5",             prv_cmd_mag_cpu5,            "Set CPU NCO magnitude ch5"},
    {"output_select_ch1",    prv_cmd_output_sel_ch1,      "Select DAC1 source (0..15)"},
    {"output_select_ch2",    prv_cmd_output_sel_ch2,      "Select DAC2 source (0..15)"},
    {"input_select",         prv_cmd_input_select,        "Set input select register"},
    {"upsampler_input_mux",  prv_cmd_ups_in_mux,          "Set upsampler input mux (0..2)"},
    {"lowspeed_dbg_select",  prv_cmd_lowspeed_dbg_select, "Select low-speed debug source (0..7)"},
    {"highspeed_dbg_select", prv_cmd_highspeed_dbg_select,"Select high-speed debug source (0..3)"},
    {"upsampler_x",          prv_cmd_upsampler_x,         "Write upsampler_input_x1..x5 (signed 16-bit)"},
    {"upsampler_y",          prv_cmd_upsampler_y,         "Write upsampler_input_y1..y5 (signed 16-bit)"},
    {"ds_pop",               prv_cmd_ds_pop,              "Pop one 5-channel downsampled frame from FIFO"},
    {"ds_status",            prv_cmd_ds_status,           "Show downsample FIFO readable/overflow"},
    {"ups_push",             prv_cmd_ups_push,            "Push one replicated 5-channel frame into upsampler FIFO"},
    {"ups_status",           prv_cmd_ups_status,          "Show upsampler FIFO writable/overflow"},
    {"gain1",                prv_cmd_gain1,               "Set gain1"},
    {"gain2",                prv_cmd_gain2,               "Set gain2"},
    {"gain3",                prv_cmd_gain3,               "Set gain3"},
    {"gain4",                prv_cmd_gain4,               "Set gain4"},
    {"gain5",                prv_cmd_gain5,               "Set gain5"},
    {"final_shift",          prv_cmd_final_shift,         "Set final shift"},
    {"cap_arm",              prv_cmd_cap_arm_pulse,       "Pulse cap_arm"},
    {"cap_done",             prv_cmd_cap_done,            "Read cap_done"},
    {"cap_rd",               prv_cmd_cap_rd,              "Read cap_data at index"},
    {"cap_enable",           prv_cmd_cap_enable,          "0=ramp, 1=capture design to DDR"},
    {"phase",                prv_cmd_phase_print,         "Print current CORDIC phase (if wired)"},
    {"magnitude",            prv_cmd_magnitude,           "Print current CORDIC magnitude (if wired)"},
};

void uberclock_basic_register_cmds(void) {
    console_register(g_basic_cmds, sizeof(g_basic_cmds) / sizeof(g_basic_cmds[0]));
}
