#include <stdio.h>
#include "console.h"
#include "cmd_list.h"
#include "uberclock.h"
#include "ubddr3.h"

/* Master command registry for the whole firmware.
 *
 * This is the single place all commands are declared - handler
 * *implementations* still live in uberclock.c/ubddr3.c next to the
 * hardware/DSP logic they touch, but the table (name, help text, and the
 * arg_spec GUI hint) lives here so it isn't buried inside uberclock.c, and
 * so cmdlist's machine-parseable dump has one place to stay in sync.
 *
 * arg_spec grammar (see console.h): "none", "int:min:max",
 * "enum:v=label,...", or NULL (GUI falls back to a plain text field - used
 * for commands with unbounded, optional, or multiple arguments, since a
 * wrong guessed numeric range would block legitimate input). */

static void cmd_cmdlist(char *args) {
    (void)args;

    printf("--CMDLIST-BEGIN--\n");
    unsigned ntbls = console_table_count();
    for (unsigned t = 0; t < ntbls; ++t) {
        const struct cmd_entry *tbl = console_table(t);
        unsigned len = console_table_len(t);
        for (unsigned i = 0; i < len; ++i) {
            const char *help = tbl[i].help ? tbl[i].help : "";
            const char *spec = tbl[i].arg_spec ? tbl[i].arg_spec : "";
            printf("%s\t%s\t%s\n", tbl[i].name, help, spec);
        }
    }
    printf("--CMDLIST-END--\n");
}

static const struct cmd_entry cmd_tbl[] = {
    {"cmdlist", cmd_cmdlist, "Dump all commands as name<TAB>help<TAB>arg_spec (machine-parseable)", "none"},

    /* ---- UberClock commands (uberclock.c) ---- */
    {"help_uc",              uc_help,                 "UberClock help", "none"},
    {"fft64_peak",           cmd_fft64_peak,          "64-point FFT over DS FIFO IQ samples, print peak only", "none"},

    {"phase_nco",            cmd_phase_nco,           "Set input CORDIC NCO phase increment", "int:0:67108863"},
    {"nco_mag",               cmd_nco_mag,             "Set NCO magnitude (signed 12-bit)", "int:-2048:2047"},

    {"phase_down_1",         cmd_phase_down_1,        "Set downconversion ch1 phase inc", "int:0:67108863"},
    {"phase_down_2",         cmd_phase_down_2,        "Set downconversion ch2 phase inc", "int:0:67108863"},
    {"phase_down_3",         cmd_phase_down_3,        "Set downconversion ch3 phase inc", "int:0:67108863"},
    {"phase_down_4",         cmd_phase_down_4,        "Set downconversion ch4 phase inc", "int:0:67108863"},
    {"phase_down_5",         cmd_phase_down_5,        "Set downconversion ch5 phase inc", "int:0:67108863"},
    {"phase_down_ref",       cmd_phase_down_ref,      "Set downconversion ref phase inc", "int:0:67108863"},

    {"phase_cpu1",           cmd_phase_cpu1,          "Set CPU NCO phase inc ch1", "int:0:67108863"},
    {"phase_cpu2",           cmd_phase_cpu2,          "Set CPU NCO phase inc ch2", "int:0:67108863"},
    {"phase_cpu3",           cmd_phase_cpu3,          "Set CPU NCO phase inc ch3", "int:0:67108863"},
    {"phase_cpu4",           cmd_phase_cpu4,          "Set CPU NCO phase inc ch4", "int:0:67108863"},
    {"phase_cpu5",           cmd_phase_cpu5,          "Set CPU NCO phase inc ch5", "int:0:67108863"},

    {"mag_cpu1",             cmd_mag_cpu1,            "Set CPU NCO magnitude ch1", "int:-2048:2047"},
    {"mag_cpu2",             cmd_mag_cpu2,            "Set CPU NCO magnitude ch2", "int:-2048:2047"},
    {"mag_cpu3",             cmd_mag_cpu3,            "Set CPU NCO magnitude ch3", "int:-2048:2047"},
    {"mag_cpu4",             cmd_mag_cpu4,            "Set CPU NCO magnitude ch4", "int:-2048:2047"},
    {"mag_cpu5",             cmd_mag_cpu5,            "Set CPU NCO magnitude ch5", "int:-2048:2047"},

    {"output_select_ch1",    cmd_output_sel_ch1,      "Select DAC1 source (0..15, 14=ref_y, 15=sum)", "int:0:15"},
    {"output_select_ch2",    cmd_output_sel_ch2,      "Select DAC2 source (0..15, 14=ref_y, 15=sum)", "int:0:15"},
    {"input_select",         cmd_input_select,        "Set input select register", "enum:0=ADC,1=NCO,2=SUM,3=reserved"},
    {"upsampler_input_mux",  cmd_ups_in_mux,          "Set upsampler input mux (0..2)", "enum:0=Gain,1=CPU,2=CPU NCO"},

    {"lowspeed_dbg_select",  cmd_lowspeed_dbg_select, "Select low-speed debug source (0..7, 6=ref_y)", "int:0:7"},
    {"highspeed_dbg_select", cmd_highspeed_dbg_select,"Select high-speed debug source (0..3, 1=filter_in_1)", "int:0:3"},

    {"upsampler_x",          cmd_upsampler_x,         "Write upsampler_input_x1..x5 (signed 16-bit)", "int:-32768:32767"},
    {"upsampler_y",          cmd_upsampler_y,         "Write upsampler_input_y1..y5 (signed 16-bit)", "int:-32768:32767"},
    {"ds_pop",               cmd_ds_pop,              "Pop one 6-channel downsampled frame from FIFO", "none"},
    {"ds_status",            cmd_ds_status,           "Show downsample FIFO readable/overflow", "none"},
    {"ups_push",             cmd_ups_push,            "Push one replicated 5-channel frame into upsampler FIFO", NULL},
    {"ups_status",           cmd_ups_status,          "Show upsampler FIFO writable/overflow", "none"},
    {"dsp_test",             cmd_dsp_test,            "Run DSP loop over FIFO samples (optional N)", NULL},
    {"dsp_run",              cmd_dsp_run,             "Enable/disable non-blocking DSP pump", "enum:0=disable,1=enable"},
    {"fft_fs",               cmd_fft_fs,              "Set DS sample rate (Hz) used by fft_ds", NULL},
    {"fft_ds",               cmd_fft_ds,              "Run FFT over downsample FIFO IQ samples and print bins", NULL},
    {"fft_ds_peak",          cmd_fft_ds_peak,         "Run FFT over downsample FIFO IQ samples and print peak only", NULL},
    {"track3",               cmd_track3,              "Sweep phase_down_<ch> until the 3-tone pattern is found", NULL},
    {"trackq_start",         cmd_trackq_start,        "Start measurement-only Y1 validation (no correction): trackq_start <f1> <f2> <f3> [N] [center] [delta1] [delta2] [delta3]", NULL},
    {"trackq_probe",         cmd_trackq_probe,        "Capture one 3-point tracking snapshot", NULL},
    {"trackq_dump",          cmd_trackq_dump,         "Stop validation and dump last raw Y1 capture", "none"},
    {"trackq_stop",          cmd_trackq_stop,         "Stop Y1 validation", "none"},

    {"gain1",                cmd_gain1,               "Set gain1", NULL},
    {"gain2",                cmd_gain2,               "Set gain2", NULL},
    {"gain3",                cmd_gain3,               "Set gain3", NULL},
    {"gain4",                cmd_gain4,               "Set gain4", NULL},
    {"gain5",                cmd_gain5,               "Set gain5", NULL},
    {"final_shift",          cmd_final_shift,         "Set final shift", "int:0:7"},

    {"cap_arm",              cmd_cap_arm_pulse,       "Pulse cap_arm", "none"},
    {"cap_done",             cmd_cap_done,            "Read cap_done", "none"},
    {"cap_rd",               cmd_cap_rd,              "Read cap_data at index", "int:0:2047"},

    {"cap_enable",           cmd_cap_enable,          "0=ramp, 1=capture design to DDR", "enum:0=ramp,1=capture"},
    {"cap_beats",            cmd_cap_beats,           "Set capture length in 256-bit beats", NULL},

    {"phase",                cmd_phase_print,         "Print current CORDIC phase (if wired)", "none"},
    {"magnitude",            cmd_magnitude,           "Print current CORDIC magnitude (if wired)", "none"},
    {"cap_start",            cap_start_cmd,           "Start LS debug", "none"},
    {"cap_status",           cap_status_cmd,          "LS debug status", "none"},
    {"cap_dump",             cap_dump_cmd,            "Ls dump cmd", "none"},

    /* ---- UberDDR3 / S2MM commands (uberclock.c) ---- */
    {"ub_help",              ub_help,                 "UberDDR3/S2MM help", "none"},
    {"ub_info",               cmd_ub_info,             "Show UBDDR3 info/state", "none"},
    {"ub_mode",               cmd_ub_mode,             "Show current cap_enable mode", "none"},
    {"ub_setmode",            cmd_ub_setmode,          "Set cap_enable (0=ramp,1=capture)", "enum:0=ramp,1=capture"},
    {"ub_start",              cmd_ub_start,            "Start S2MM using current mode", NULL},
    {"ub_ramp",               cmd_ub_ramp2,            "Force ramp mode then start S2MM", NULL},
    {"ub_cap",                cmd_ub_cap,              "Force capture mode then start S2MM", NULL},
    {"ub_wait",               cmd_ub_wait,             "Wait until DMA done", "none"},
    {"ub_hexdump",            cmd_ub_hexdump,          "Hexdump DDR memory", NULL},
    {"ub_send",               cmd_ub_send,             "Send DDR memory region via UDP", NULL},
    {"fft32_ds_y",            cmd_fft32_ds_y,          "Real FFT of 32 Y samples from DS FIFO", "none"},
    {"sig3_start",            cmd_sig3_start,          "Start 5 independent 3-tone software generators", "none"},
    {"sig3_stop",             cmd_sig3_stop,           "Stop 5 independent 3-tone software generators", "none"},
    {"sig3_amp",              cmd_sig3_amp,            "Set 3-tone per-tone amplitude: sig3_amp <val> | <ch> <val>", NULL},
    {"sig3_freqs",            cmd_sig3_freqs,          "Set channel 3-tone frequencies: sig3_freqs <ch> <f1> <f2> <f3>", NULL},
    {"sig3_enable_ch",        cmd_sig3_enable_ch,      "Enable one sig3 channel: sig3_enable_ch <ch>", NULL},
    {"sig3_disable_ch",       cmd_sig3_disable_ch,     "Disable one sig3 channel: sig3_disable_ch <ch>", NULL},

    /* ---- DDR commands (ubddr3.c) ---- */
    {"help_ddr",  cmd_help_ddr,  "DDR/UberDDR3 command list", "none"},
    {"ddrinfo",   cmd_ddrinfo,   "Print DDR base + calib CSR state", "none"},
    {"ddrwait",   cmd_ddrwait,   "Wait for UberDDR3 calibration to complete", "none"},
    {"ddrprobe",  cmd_ddrprobe,  "One 32-bit store/load at base", "none"},
    {"ddrbyte",   cmd_ddrbyte,   "Byte-lane sanity (0..31 at base)", "none"},
    {"ddrtest",   cmd_ddrtest,   "32-bit test, pattern=A5A5^index (default 4 KiB) [KiB]", NULL},
    {"ddrtestb",  cmd_ddrtestb, "Byte test, pattern=A5^index (default 4 KiB) [KiB]", NULL},
    {"ddrmap",    cmd_ddrmap,   "One 256-bit beat lane-map sanity check at base", "none"},
    {"ddrpat",    cmd_ddrpat,   "Pattern test: ddrpat [size] [pattern] [seed] (patterns: a5xor|const|addr|prbs|walk1|walk0)", NULL},
    {"timertest", cmd_timertest,"100 ms sanity (prints source, ticks, us)", "none"},
    {"timeinfo",  cmd_timeinfo, "Show timing source and CLK_HZ", "none"},
};

void cmd_list_register_cmds(void) {
    console_register(cmd_tbl, sizeof(cmd_tbl) / sizeof(cmd_tbl[0]));
}
