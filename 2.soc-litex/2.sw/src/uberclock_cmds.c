#include "uberclock_core.h"

static const char *input_select_name(unsigned v) {
    switch (v) {
        case 0u: return "filter_in (ADC ch0)";
        case 1u: return "nco_cos";
        case 2u: return "sum[13:2]";
        default: return "sum[13:2] (RTL default)";
    }
}

static const char *upsampler_input_mux_name(unsigned v) {
    switch (v) {
        case 0u: return "upsampled_gain_x/y";
        case 1u: return "upsampler_input_x/y";
        case 2u: return "x/y_cpu_nco << 4";
        default: return "x/y_cpu_nco << 4 (RTL default)";
    }
}

static const char *output_select_name(unsigned v) {
    switch (v & 0x0fu) {
        case 0x0u: return "upsampled_gain_y1[15:2]";
        case 0x1u: return "upsampled_gain_y2[15:2]";
        case 0x2u: return "upsampled_gain_y3[15:2]";
        case 0x3u: return "upsampled_gain_y4[15:2]";
        case 0x4u: return "upsampled_gain_y5[15:2]";
        case 0x5u: return "tx_channel_output1[15:2]";
        case 0x6u: return "tx_channel_output2[15:2]";
        case 0x7u: return "tx_channel_output3[15:2]";
        case 0x8u: return "tx_channel_output4[15:2]";
        case 0x9u: return "tx_channel_output5[15:2]";
        case 0xAu: return "nco_cos << 2";
        case 0xBu: return "filter_in << 2";
        case 0xCu: return "filter_in_1 << 2";
        default:   return "sum (RTL default)";
    }
}

static void uc_help(char *args) {
    (void)args;
    puts_help_header("UberClock commands");

    puts("  Signal generation / tuning");
    puts("    phase_nco              <val>   Input NCO phase increment (0..2^26-1)");
    puts("    nco_mag                <val>   Input NCO magnitude (-2048..2047)");
    puts("    phase_down_1..5        <val>   Downconversion mixer phase increment");
    puts("    phase_down_ref         <val>   Downconversion reference phase increment");
    puts("    phase_cpu1..5          <val>   CPU-side NCO phase increment");
    puts("    mag_cpu1..5            <val>   CPU-side NCO magnitude (-2048..2047)");
    puts("");
    puts("  Routing / output selection");
    puts("    input_select           <0..3>  Front-end mux used by all RX channels:");
    puts("                                   0=filter_in (ADC ch0), 1=nco_cos,");
    puts("                                   2=sum[13:2], 3=sum[13:2] (RTL default)");
    puts("    upsampler_input_mux    <0..2>  TX upsampler input mux used by all channels:");
    puts("                                   0=upsampled_gain_x/y, 1=upsampler_input_x/y,");
    puts("                                   2=x/y_cpu_nco << 4");
    puts("    output_select_ch1      <0..15> DAC/output channel 1 source:");
    puts("                                   0=y1 gain, 1=y2 gain, 2=y3 gain, 3=y4 gain");
    puts("                                   4=y5 gain, 5=tx1, 6=tx2, 7=tx3");
    puts("                                   8=tx4, 9=tx5, 10=nco_cos, 11=filter_in");
    puts("                                   12=filter_in_1, 13..15=sum");
    puts("    output_select_ch2      <0..15> DAC/output channel 2 source:");
    puts("                                   same mapping as output_select_ch1");
    puts("    gain1..gain5           <int32> Per-channel gain stage coefficient");
    puts("    final_shift            <0..7>  Final output scaling shift");
    puts("    upsampler_x            <val>   Manual I/sample injection (-32768..32767)");
    puts("    upsampler_y            <val>   Manual Q/sample injection (-32768..32767)");
    puts("");
    puts("  Debug / observability");
    puts("    lowspeed_dbg_select    <0..4>  Select low-speed debug/capture source");
    puts("    highspeed_dbg_select   <0..3>  Select high-speed debug output source");
    puts("    phase                          Print live phase value");
    puts("    magnitude                      Print live magnitude value");
    puts("");
    puts("  Related command groups");
    puts("    cap_*                Capture buffer control / readback");
    puts("    dsp_*                FIFO / DSP test helpers");
    puts("    ub_*                 DDR-to-UDP / S2MM streaming helpers");
    puts("");
}

static void cmd_phase_nco(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_nco");
    if (p >= (1u << 26)) return;
    main_phase_inc_nco_write(p);
    uc_commit();
    printf("Input NCO phase increment set to %u\n", p);
}

static void cmd_phase_cpu1(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu1");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu1_write(p);
    uc_commit();
    printf("CPU phase increment ch1 set to %u\n", p);
}

static void cmd_phase_cpu2(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu2");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu2_write(p);
    uc_commit();
    printf("CPU phase increment ch2 set to %u\n", p);
}

static void cmd_phase_cpu3(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu3");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu3_write(p);
    uc_commit();
    printf("CPU phase increment ch3 set to %u\n", p);
}

static void cmd_phase_cpu4(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu4");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu4_write(p);
    uc_commit();
    printf("CPU phase increment ch4 set to %u\n", p);
}

static void cmd_phase_cpu5(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu5");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu5_write(p);
    uc_commit();
    printf("CPU phase increment ch5 set to %u\n", p);
}

static void cmd_nco_mag(char *a) {
    int v = parse_s(a, -2048, 2047, "nco_mag");
    if (v < -2048 || v > 2047) return;
    main_nco_mag_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("nco_mag set to %d\n", v);
}

static void cmd_phase_down_ref(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_down_ref");
    if (p >= (1u << 26)) return;
    main_phase_inc_down_ref_write(p);
    uc_commit();
    printf("Downconversion phase ref increment set to %u\n", p);
}

static void cmd_mag_cpu1(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu1");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu1_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu1 set to %d\n", v);
}

static void cmd_mag_cpu2(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu2");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu2_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu2 set to %d\n", v);
}

static void cmd_mag_cpu3(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu3");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu3_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu3 set to %d\n", v);
}

static void cmd_mag_cpu4(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu4");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu4_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu4 set to %d\n", v);
}

static void cmd_mag_cpu5(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu5");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu5_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu5 set to %d\n", v);
}

static void cmd_lowspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 4u) { puts("lowspeed_dbg_select must be 0..4"); return; }
    main_lowspeed_dbg_select_write(v);
    uc_commit();
    printf("lowspeed_dbg_select = %u\n", v);
}

static void cmd_highspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 3u) { puts("highspeed_dbg_select must be 0..3"); return; }
    main_highspeed_dbg_select_write(v);
    uc_commit();
    printf("highspeed_dbg_select = %u\n", v);
}

static void cmd_phase_dn(char *a, int ch) {
    unsigned p = parse_u(a, 1u << 26, "phase_down");
    if (p >= (1u << 26)) return;
    switch (ch) {
        case 1: main_phase_inc_down_1_write(p); break;
        case 2: main_phase_inc_down_2_write(p); break;
        case 3: main_phase_inc_down_3_write(p); break;
        case 4: main_phase_inc_down_4_write(p); break;
        case 5: main_phase_inc_down_5_write(p); break;
        default: return;
    }
    uc_commit();
    printf("Downconversion phase ch%d increment set to %u\n", ch, p);
}

static void cmd_phase_down_1(char *a) { cmd_phase_dn(a, 1); }
static void cmd_phase_down_2(char *a) { cmd_phase_dn(a, 2); }
static void cmd_phase_down_3(char *a) { cmd_phase_dn(a, 3); }
static void cmd_phase_down_4(char *a) { cmd_phase_dn(a, 4); }
static void cmd_phase_down_5(char *a) { cmd_phase_dn(a, 5); }

static void cmd_output_sel_ch1(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch1_write(v);
    uc_commit();
    printf("output_select_ch1 = %u -> %s\n", v, output_select_name(v));
}

static void cmd_output_sel_ch2(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch2_write(v);
    uc_commit();
    printf("output_select_ch2 = %u -> %s\n", v, output_select_name(v));
}

static void cmd_input_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_input_select_write(v);
    uc_commit();
    printf("input_select = %u -> %s\n", v, input_select_name(v));
}

static void cmd_ups_in_mux(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_upsampler_input_mux_write(v);
    uc_commit();
    printf("upsampler_input_mux = %u -> %s\n", v, upsampler_input_mux_name(v));
}

static void cmd_gain(char *a, int idx) {
    int32_t g = (int32_t)strtol(a ? a : "0", NULL, 0);
    switch (idx) {
        case 1: main_gain1_write((uint32_t)g); break;
        case 2: main_gain2_write((uint32_t)g); break;
        case 3: main_gain3_write((uint32_t)g); break;
        case 4: main_gain4_write((uint32_t)g); break;
        case 5: main_gain5_write((uint32_t)g); break;
        default: return;
    }
    uc_commit();
    printf("Gain%d register set to %ld (0x%08lX)\n", idx, (long)g, (unsigned long)g);
}

static void cmd_gain1(char *a) { cmd_gain(a, 1); }
static void cmd_gain2(char *a) { cmd_gain(a, 2); }
static void cmd_gain3(char *a) { cmd_gain(a, 3); }
static void cmd_gain4(char *a) { cmd_gain(a, 4); }
static void cmd_gain5(char *a) { cmd_gain(a, 5); }

static void cmd_final_shift(char *a) {
    int32_t fs = (int32_t)strtol(a ? a : "0", NULL, 0);
    main_final_shift_write((uint32_t)fs);
    uc_commit();
    printf("final_shift set to %ld (0x%08lX)\n", (long)fs, (unsigned long)fs);
}

static void cmd_upsampler_x(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_x");
    if (v < -32768 || v > 32767) return;
    main_upsampler_input_x_write((uint32_t)((int32_t)v & 0xffff));
    uc_commit();
    printf("upsampler_input_x = %d\n", v);
}

static void cmd_upsampler_y(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_y");
    if (v < -32768 || v > 32767) return;
    main_upsampler_input_y_write((uint32_t)((int32_t)v & 0xffff));
    uc_commit();
    printf("upsampler_input_y = %d\n", v);
}

static void cmd_phase_print(char *a) { (void)a; printf("Phase %ld\n", (long)g_phase); }
static void cmd_magnitude(char *a)   { (void)a; printf("Magnitude %d\n", g_mag); }

const struct cmd_entry uberclock_cmds[] = {
    {"help_uc",              uc_help,                 "Show grouped UberClock help"},
    {"phase_nco",            cmd_phase_nco,           "Set input NCO phase increment"},
    {"nco_mag",              cmd_nco_mag,             "Set input NCO magnitude (signed 12-bit)"},
    {"phase_down_1",         cmd_phase_down_1,        "Set downconversion channel 1 phase increment"},
    {"phase_down_2",         cmd_phase_down_2,        "Set downconversion channel 2 phase increment"},
    {"phase_down_3",         cmd_phase_down_3,        "Set downconversion channel 3 phase increment"},
    {"phase_down_4",         cmd_phase_down_4,        "Set downconversion channel 4 phase increment"},
    {"phase_down_5",         cmd_phase_down_5,        "Set downconversion channel 5 phase increment"},
    {"phase_down_ref",       cmd_phase_down_ref,      "Set downconversion reference phase increment"},
    {"phase_cpu1",           cmd_phase_cpu1,          "Set CPU/NCO phase increment for channel 1"},
    {"phase_cpu2",           cmd_phase_cpu2,          "Set CPU/NCO phase increment for channel 2"},
    {"phase_cpu3",           cmd_phase_cpu3,          "Set CPU/NCO phase increment for channel 3"},
    {"phase_cpu4",           cmd_phase_cpu4,          "Set CPU/NCO phase increment for channel 4"},
    {"phase_cpu5",           cmd_phase_cpu5,          "Set CPU/NCO phase increment for channel 5"},
    {"mag_cpu1",             cmd_mag_cpu1,            "Set CPU/NCO magnitude for channel 1"},
    {"mag_cpu2",             cmd_mag_cpu2,            "Set CPU/NCO magnitude for channel 2"},
    {"mag_cpu3",             cmd_mag_cpu3,            "Set CPU/NCO magnitude for channel 3"},
    {"mag_cpu4",             cmd_mag_cpu4,            "Set CPU/NCO magnitude for channel 4"},
    {"mag_cpu5",             cmd_mag_cpu5,            "Set CPU/NCO magnitude for channel 5"},
    {"output_select_ch1",    cmd_output_sel_ch1,      "Route DAC1: y1..y5, tx1..tx5, nco_cos, filter_in, filter_in_1, sum"},
    {"output_select_ch2",    cmd_output_sel_ch2,      "Route DAC2: y1..y5, tx1..tx5, nco_cos, filter_in, filter_in_1, sum"},
    {"input_select",         cmd_input_select,        "Select RX input: filter_in, nco_cos, or sum[13:2]"},
    {"upsampler_input_mux",  cmd_ups_in_mux,          "Select TX input: gain path, manual x/y, or CPU NCO"},
    {"lowspeed_dbg_select",  cmd_lowspeed_dbg_select, "Select low-speed debug / capture tap"},
    {"highspeed_dbg_select", cmd_highspeed_dbg_select,"Select high-speed debug output tap"},
    {"upsampler_x",          cmd_upsampler_x,         "Write manual upsampler I/sample value"},
    {"upsampler_y",          cmd_upsampler_y,         "Write manual upsampler Q/sample value"},
    {"gain1",                cmd_gain1,               "Set gain coefficient for channel 1"},
    {"gain2",                cmd_gain2,               "Set gain coefficient for channel 2"},
    {"gain3",                cmd_gain3,               "Set gain coefficient for channel 3"},
    {"gain4",                cmd_gain4,               "Set gain coefficient for channel 4"},
    {"gain5",                cmd_gain5,               "Set gain coefficient for channel 5"},
    {"final_shift",          cmd_final_shift,         "Set final output scaling shift"},
    {"phase",                cmd_phase_print,         "Print current live phase value"},
    {"magnitude",            cmd_magnitude,           "Print current live magnitude value"},
};

const unsigned uberclock_cmd_count =
    (unsigned)(sizeof(uberclock_cmds) / sizeof(uberclock_cmds[0]));
