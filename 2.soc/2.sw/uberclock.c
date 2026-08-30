// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

// uberclock.c
// Console command table (mostly thin wrappers around CSR reads/writes) and
// top-level init/poll glue. The sig3 test-tone generator and the trackq
// tracking algorithm live in sig3.c / trackq.c.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include "uberclock.h"
#include "sig3.h"
#include "trackq.h"
#include "console.h"
#include "ubddr3.h"
#include "libliteeth/udp.h"   // LiteEth UDP stack header

static void write_upsampler_inputs_all_x(int16_t v) {
    uint32_t w = (uint32_t)((int32_t)v & 0xffff);
    main_upsampler_input_x1_write(w);
    main_upsampler_input_x2_write(w);
    main_upsampler_input_x3_write(w);
    main_upsampler_input_x4_write(w);
    main_upsampler_input_x5_write(w);
}

static void write_upsampler_inputs_all_y(int16_t v) {
    uint32_t w = (uint32_t)((int32_t)v & 0xffff);
    main_upsampler_input_y1_write(w);
    main_upsampler_input_y2_write(w);
    main_upsampler_input_y3_write(w);
    main_upsampler_input_y4_write(w);
    main_upsampler_input_y5_write(w);
}

void ups_fifo_write_frame(const iq5_frame_t *frame) {
    main_ups_fifo_x1_write((uint32_t)((int32_t)frame->x[0] & 0xffff));
    main_ups_fifo_y1_write((uint32_t)((int32_t)frame->y[0] & 0xffff));
    main_ups_fifo_x2_write((uint32_t)((int32_t)frame->x[1] & 0xffff));
    main_ups_fifo_y2_write((uint32_t)((int32_t)frame->y[1] & 0xffff));
    main_ups_fifo_x3_write((uint32_t)((int32_t)frame->x[2] & 0xffff));
    main_ups_fifo_y3_write((uint32_t)((int32_t)frame->y[2] & 0xffff));
    main_ups_fifo_x4_write((uint32_t)((int32_t)frame->x[3] & 0xffff));
    main_ups_fifo_y4_write((uint32_t)((int32_t)frame->y[3] & 0xffff));
    main_ups_fifo_x5_write((uint32_t)((int32_t)frame->x[4] & 0xffff));
    main_ups_fifo_y5_write((uint32_t)((int32_t)frame->y[4] & 0xffff));
    main_ups_fifo_push_write(1);
}

static void ups_fifo_write_replicated(int16_t x, int16_t y) {
    iq5_frame_t frame = {
        .x = {x, x, x, x, x},
        .y = {y, y, y, y, y},
    };
    ups_fifo_write_frame(&frame);
}

void ds_fifo_read_frame(iq6_frame_t *frame) {
    main_ds_fifo_pop_write(1);
    frame->x[0] = (int16_t)(main_ds_fifo_x1_read() & 0xffffu);
    frame->y[0] = (int16_t)(main_ds_fifo_y1_read() & 0xffffu);
    frame->x[1] = (int16_t)(main_ds_fifo_x2_read() & 0xffffu);
    frame->y[1] = (int16_t)(main_ds_fifo_y2_read() & 0xffffu);
    frame->x[2] = (int16_t)(main_ds_fifo_x3_read() & 0xffffu);
    frame->y[2] = (int16_t)(main_ds_fifo_y3_read() & 0xffffu);
    frame->x[3] = (int16_t)(main_ds_fifo_x4_read() & 0xffffu);
    frame->y[3] = (int16_t)(main_ds_fifo_y4_read() & 0xffffu);
    frame->x[4] = (int16_t)(main_ds_fifo_x5_read() & 0xffffu);
    frame->y[4] = (int16_t)(main_ds_fifo_y5_read() & 0xffffu);
    frame->x[5] = (int16_t)(main_ds_fifo_xref_read() & 0xffffu);
    frame->y[5] = (int16_t)(main_ds_fifo_yref_read() & 0xffffu);
}


static inline void ub_cache_sync(void) { flush_cpu_dcache(); flush_l2_cache(); }



/* ========================================================================= */
/*                             UberClock                                     */
/* ========================================================================= */

volatile uint32_t ce_event = 0;
volatile uint32_t ce_ticks = 0;
static int16_t  mag;
static int32_t  phase;
static volatile int dsp_pump_enable = 0;
static volatile uint32_t dsp_work_tokens = 0;

#define DSP_SWQ_LEN 256u
static int16_t dsp_swq_x[DSP_SWQ_LEN];
static int16_t dsp_swq_y[DSP_SWQ_LEN];
static unsigned dsp_swq_r = 0;
static unsigned dsp_swq_w = 0;
static unsigned dsp_swq_count = 0;

/* Fixed-cadence DSP pumping is driven from ce_down ISR. */
static unsigned dsp_pump_step(unsigned max_in, unsigned max_out);

void service_one_ce_event(void) {
    sig3_push_one();
    mag = (int16_t)(main_magnitude_read() & 0xffff);
    evm_pending_write(1);
    evm_enable_write(1);
}


void uc_commit(void) {
    cfg_link_commit_write(1);
}

/* ---- ISR ---- */
static void ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    if (ce_event < 0xffffffffu) ce_event++;
    ce_ticks++;
}

/* ---- Help ---- */
static void uc_help(char *args) {
    (void)args;
    puts_help_header("UberClock commands");

    puts("  phase_nco        <val>      (0..16777215)");
    puts("  nco_mag          <val>      (signed 12-bit: -2048..2047)");

    puts("  phase_down_1 <val> ... phase_down_5 <val>  (0..524287)");
    puts("  phase_down_ref   <val>      (0..524287)");

    puts("  phase_cpu1       <val>      (0..16777215)");
    puts("  phase_cpu2       <val>      (0..16777215)");
    puts("  phase_cpu3       <val>      (0..16777215)");
    puts("  phase_cpu4       <val>      (0..16777215)");
    puts("  phase_cpu5       <val>      (0..16777215)");

    puts("  mag_cpu1         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu2         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu3         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu4         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu5         <val>      (signed 12-bit: -2048..2047)");

    puts("  input_select         <0..3> (0=ADC,1=NCO,2=SUM,3=reserved)");
    puts("  upsampler_input_mux  <0..2> (0=Gain,1=CPU,2=CPU NCO)");

    puts("  output_select_ch1    <0..15> (14=downsampled_y_ref, 15=sum)");
    puts("  output_select_ch2    <0..15> (14=downsampled_y_ref, 15=sum)");
    puts("  gain1|gain2|gain3|gain4|gain5 <int32>");
    puts("  final_shift          <0..7>");

    puts("  lowspeed_dbg_select  <0..7> (6=downsampled_y_ref)");
    puts("  highspeed_dbg_select <0..3> (1=filter_in_1)");

    puts("  upsampler_x          <val>  (signed 16-bit, replicated to ch1..ch5)");
    puts("  upsampler_y          <val>  (signed 16-bit, replicated to ch1..ch5)");
    puts("  sig3_start                  (start 5 independent 3-tone generators)");
    puts("  sig3_stop                   (stop 5 independent 3-tone generators)");
    puts("  sig3_amp <val> | <ch> <val> (set per-tone amplitude for all or one channel)");
    puts("  sig3_freqs <ch> <f1> <f2> <f3> (set 3-tone frequencies for one channel)");
    puts("  sig3_enable_ch     <ch>     (enable one sig3 channel)");
    puts("  sig3_disable_ch    <ch>     (disable one sig3 channel)");
    puts("  ds_pop                      (pop one 6-channel downsampled frame, ch1..ch5 + ref)");
    puts("  ds_status                   (read downsample FIFO flags/overflow)");
    puts("  ups_push <x> <y>             (enqueue one 5-channel frame, replicated)");
    puts("  ups_status                  (read upsampler FIFO flags/overflow)");
    puts("  dsp_run <0|1>               (enable/disable non-blocking DSP pump)");
    puts("  fft_ds [N]                  (FFT over DS FIFO IQ samples, N=8..2048, prints bins)");
    puts("  fft_ds_peak [N]             (FFT over DS FIFO IQ samples, N=8..2048, peak only)");
    puts("  fft_fs <Hz>                 (set DS sample rate used for fft_ds Hz print)");
    puts("  track3 <ch> <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
    puts("  trackq_start <f1> <f2> <f3> [N] [center_hz] [delta_ch1_hz] [delta_ch2_hz] [delta_ch3_hz]");
    puts("  trackq_probe [N] [center_hz] [delta_hz]");
    puts("  trackq_stop                 (stop 3-point quadratic tracking)");

    puts("  cap_arm              (pulse arm capture)");
    puts("  cap_done             (read cap_done)");
    puts("  cap_rd <idx>          (read cap_data at idx)");
    puts("  cap_enable   <0|1>    (0=ramp->DDR, 1=capture design->DDR)");
    puts("  cap_beats    <N>      (# of 256-bit beats captured by the gateware)");
    puts("");
}

/* ---- Phase/NCO/downconversion ---- */
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
    /* store as 12-bit signed in low bits */
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
    if (v > 7) { puts("lowspeed_dbg_select must be 0..7"); return; }
    main_lowspeed_dbg_select_write(v);
    uc_commit();
    printf("lowspeed_dbg_select = %u\n", v);
}

static void cmd_highspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 3) { puts("highspeed_dbg_select must be 0..3"); return; }
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
static void cmd_phase_down_1(char *a){ cmd_phase_dn(a, 1); }
static void cmd_phase_down_2(char *a){ cmd_phase_dn(a, 2); }
static void cmd_phase_down_3(char *a){ cmd_phase_dn(a, 3); }
static void cmd_phase_down_4(char *a){ cmd_phase_dn(a, 4); }
static void cmd_phase_down_5(char *a){ cmd_phase_dn(a, 5); }

/* ---- Muxes / gains ---- */
static void cmd_output_sel_ch1(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch1_write(v);
    uc_commit();
    printf("output_select_ch1 set to %u\n", v);
}

static void cmd_output_sel_ch2(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch2_write(v);
    uc_commit();
    printf("output_select_ch2 set to %u\n", v);
}

static void cmd_input_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_input_select_write(v);
    uc_commit();
    printf("Main input select register set to %u\n", v);
}

static void cmd_ups_in_mux(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_upsampler_input_mux_write(v);
    uc_commit();
    printf("Upsampler input mux register set to %u\n", v);
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
    printf("Gain%d register set to %ld (0x%08lX)\n",
           idx, (long)g, (unsigned long)g);
}
static void cmd_gain1(char *a){ cmd_gain(a, 1); }
static void cmd_gain2(char *a){ cmd_gain(a, 2); }
static void cmd_gain3(char *a){ cmd_gain(a, 3); }
static void cmd_gain4(char *a){ cmd_gain(a, 4); }
static void cmd_gain5(char *a){ cmd_gain(a, 5); }

static void cmd_final_shift(char *a) {
    int32_t fs = (int32_t)strtol(a ? a : "0", NULL, 0);
    main_final_shift_write((uint32_t)fs);
    uc_commit();
    printf("final_shift set to %ld (0x%08lX)\n",
           (long)fs, (unsigned long)fs);
}

static void cmd_cap_enable(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void cmd_upsampler_x(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_x");
    if (v < -32768 || v > 32767) return;
    write_upsampler_inputs_all_x((int16_t)v);
    uc_commit();
    printf("upsampler_input_x[1..5] = %d\n", v);
}

static void cmd_upsampler_y(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_y");
    if (v < -32768 || v > 32767) return;
    write_upsampler_inputs_all_y((int16_t)v);
    uc_commit();
    printf("upsampler_input_y[1..5] = %d\n", v);
}

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
/* ========================================================================= */
/*                         FIFO DSP test harness                             */
/* ========================================================================= */

static inline void dsp_process(int16_t in_x, int16_t in_y,
                               int16_t *out_x, int16_t *out_y) {
    /* Intentional pass-through: dsp_test/dsp_run exercise the ds_fifo/ups_fifo
     * round trip itself, not any algorithm. Not a stub for missing DSP work. */
    *out_x = in_x;
    *out_y = in_y;
}

static unsigned dsp_pump_step(unsigned max_in, unsigned max_out) {
    (void)max_out; /* no loopback push anymore */

    unsigned popped = 0;
    unsigned i;

    for (i = 0; i < max_in; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) != 0u) {
            iq6_frame_t frame;
            ds_fifo_read_frame(&frame);
            int16_t in_x = frame.x[0];
            int16_t in_y = frame.y[0];

            /* optional processing hook, but no UPS push */
            int16_t out_x = 0, out_y = 0;
            dsp_process(in_x, in_y, &out_x, &out_y);
            (void)out_x;
            (void)out_y;

            /* store for later analysis if wanted */
            dsp_swq_x[dsp_swq_w] = in_x;
            dsp_swq_y[dsp_swq_w] = in_y;
            dsp_swq_w = (dsp_swq_w + 1u) % DSP_SWQ_LEN;
            if (dsp_swq_count < DSP_SWQ_LEN) {
                dsp_swq_count++;
            } else {
                /* overwrite oldest if buffer full */
                dsp_swq_r = (dsp_swq_r + 1u) % DSP_SWQ_LEN;
            }

            popped++;
        } else {
            break;
        }
    }

    return popped;
}

static void fifo_clear_flags(void) {
    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);
}

static void cmd_dsp_test(char *args) {
    char *tok = strtok(args, " \t");
    unsigned limit = tok ? (unsigned)strtoul(tok, NULL, 0) : 0;

    /* Temporarily disable background pump so dsp_test owns the flow. */
    int prev_run = dsp_pump_enable;
    dsp_pump_enable = 0;

    unsigned processed = 0;
    unsigned stall = 0;
    const unsigned STALL_MAX = 1000000u;

    while (!limit || processed < limit) {
        unsigned step = dsp_pump_step(64, 64);
        if (step == 0u) {
            if (++stall >= STALL_MAX) break;
            continue;
        }
        stall = 0;
        processed += step;
    }

    dsp_pump_enable = prev_run;

    printf("dsp_test processed %u samples (stall=%u)\n", processed, stall);
}

static void cmd_ds_pop(char *a) {
    (void)a;
    iq6_frame_t frame;
    ds_fifo_read_frame(&frame);
    printf("ds_fifo:"
           " ch1=(%d,%d)"
           " ch2=(%d,%d)"
           " ch3=(%d,%d)"
           " ch4=(%d,%d)"
           " ch5=(%d,%d)"
           " ref=(%d,%d)\n",
           (int)frame.x[0], (int)frame.y[0],
           (int)frame.x[1], (int)frame.y[1],
           (int)frame.x[2], (int)frame.y[2],
           (int)frame.x[3], (int)frame.y[3],
           (int)frame.x[4], (int)frame.y[4],
           (int)frame.x[5], (int)frame.y[5]);
}


static void cmd_ds_status(char *a) {
    (void)a;
    unsigned flags = (unsigned)(main_ds_fifo_flags_read() & 0xffu);
    unsigned overflow = (unsigned)(main_ds_fifo_overflow_read() & 1u);
    unsigned underflow = (unsigned)(main_ds_fifo_underflow_read() & 1u);
    printf("ds_fifo: readable=%u overflow=%u underflow=%u\n", flags & 1u, overflow, underflow);
    main_ds_fifo_clear_write(1);
}

static void cmd_ups_push(char *args) {
    char *tokx = strtok(args, " \t");
    char *toky = strtok(NULL, " \t");
    if (!tokx || !toky) { puts("Usage: ups_push <x> <y>"); return; }
    int x = parse_s(tokx, -32768, 32767, "ups_x");
    int y = parse_s(toky, -32768, 32767, "ups_y");
    if (x < -32768 || x > 32767 || y < -32768 || y > 32767) return;
    ups_fifo_write_replicated((int16_t)x, (int16_t)y);
    printf("ups_fifo push: replicated x=%d y=%d to ch1..ch5\n", x, y);
}

static void cmd_ups_status(char *a) {
    (void)a;
    unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
    unsigned overflow = (unsigned)(main_ups_fifo_overflow_read() & 1u);
    unsigned underflow = (unsigned)(main_ups_fifo_underflow_read() & 1u);
    printf("ups_fifo: writable=%u overflow=%u underflow=%u\n",
           (flags >> 1) & 1u, overflow, underflow);
    main_ups_fifo_clear_write(1);
}

static void cmd_dsp_run(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    dsp_pump_enable = v ? 1 : 0;
    if (dsp_pump_enable) {
        dsp_swq_r = 0;
        dsp_swq_w = 0;
        dsp_swq_count = 0;
        dsp_work_tokens = 0;
        main_ds_fifo_clear_write(1);
    } else {
        dsp_work_tokens = 0;
    }
    printf("dsp_run = %u\n", dsp_pump_enable);
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

static void cmd_phase_print(char *a) {
    (void)a;
    printf("Phase %ld\n", (long)phase);
}

static void cmd_magnitude(char *a) {
    (void)a;
    printf("Magnitude %d\n", mag);
}

/* ========================================================================= */
/*                     UberDDR3 + S2MM (capture-to-DDR) CLI                   */
/* ========================================================================= */

static inline uint8_t ub_size_to_code(const char *s) {
    if (!s) return 0;             // 00 = bus width
    if (!strcmp(s,"bus")) return 0;
    if (!strcmp(s,"32"))  return 1;
    if (!strcmp(s,"16"))  return 2;
    if (!strcmp(s,"8"))   return 3;
    return 0;
}

static void ub_help(char *args) {
    (void)args;
    puts_help_header("UberDDR3/S2MM commands");
    puts("  ub_info");
    puts("      Print DDR calibration state and CSR base.");
    puts("  ub_mode");
    puts("      Print current capture mode (cap_enable).");
    puts("  ub_setmode <0|1>");
    puts("      Set cap_enable (0=ramp, 1=capture design) and commit.");
    puts("  ub_ramp <addr_hex> [beats] [size]");
    puts("      FORCE ramp mode (cap_enable=0), then start S2MM into DDR.");
    puts("  ub_cap  <addr_hex> [beats] [size]");
    puts("      FORCE capture mode (cap_enable=1), then start S2MM into DDR.");
    puts("  ub_start <addr_hex> [beats] [size]");
    puts("      Start S2MM using CURRENT cap_enable mode.");
    puts("  ub_wait");
    puts("      Poll until DMA not busy; flush caches; print error if any.");
    puts("  ub_hexdump <addr_hex> <bytes>");
    puts("      Dump memory to verify write.");
    puts("  ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
    puts("      Send DDR memory region via UDP to PC.");
    puts("      Example: ub_send 0xA0000000 8192 192.168.0.2 5000");
    puts("");
}

static void cmd_ub_info(char *a) {
    (void)a;
    int cal = 0;
    cal = ubddr3_calib_done_read();
#ifdef CSR_UBDDR3_BASE
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d",
           (unsigned long)CSR_UBDDR3_BASE, cal);
#else
    printf("UBDDR3 CSR base: <not exported>  calib_done: %d", cal);
#endif
    printf("\n");
}

static void cmd_ub_mode(char *a) {
    (void)a;
    unsigned v = main_cap_enable_read() & 1u;
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void cmd_ub_setmode(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

/* Core DMA start helper used by ub_ramp/ub_cap/ub_start */
static void ub_dma_start(uint64_t addr, uint32_t beats, uint8_t size_code) {
    (void)beats;
    (void)size_code;

    ubddr3_dma_inc_write(1);
    ubddr3_dma_size_write(size_code);
    ubddr3_dma_addr0_write((uint32_t)(addr & 0xffffffffu));
    ubddr3_dma_addr1_write((uint32_t)(addr >> 32));

    ubddr3_dma_req_write(1);

}

/* ub_start: run DMA using current cap_enable mode */
static void cmd_ub_start(char *args) {
    char *p = args;
    char *tok_addr  = strtok(p, " \t");
    char *tok_beats = strtok(NULL, " \t");
    char *tok_size  = strtok(NULL, " \t");
    uint64_t addr;
    uint32_t beats;
    uint8_t  sz;

    if (!tok_addr) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    addr = strtoull(tok_addr, NULL, 0);
    beats = (uint32_t)(tok_beats ? strtoul(tok_beats, NULL, 0) : 256u);
    sz = ub_size_to_code(tok_size);

    unsigned mode = main_cap_enable_read() & 1u;

    printf("S2MM start: mode=%s addr=0x%08lx_%08lx beats=%u size=%s\n",
           mode ? "CAPTURE" : "RAMP",
           (unsigned long)(addr >> 32), (unsigned long)(addr & 0xffffffffu),
           (unsigned)beats,
           (sz==0)?"bus":(sz==1)?"32":(sz==2)?"16":"8");

    main_cap_beats_write(beats);
    uc_commit();
    ub_dma_start(addr, beats, sz);
}

/* ub_ramp: force ramp mode then start */
static void cmd_ub_ramp2(char *args) {
    main_cap_enable_write(0);
    uc_commit();
    cmd_ub_start(args);
}

/* ub_cap: force capture mode then start */
static void cmd_ub_cap(char *args) {
    main_cap_enable_write(1);
    uc_commit();
    cmd_ub_start(args);
}

static void cmd_ub_wait(char *a) {
    (void)a;
    printf("Waiting for DMA ... "); fflush(stdout);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    puts("done.");
    if (ubddr3_dma_err_read())
        puts("DMA error flag is set!");
}

static void cmd_ub_hexdump(char *a) {
    char *tok_addr = strtok(a, " \t");
    char *tok_len  = strtok(NULL, " \t");
    if (!tok_addr || !tok_len) {
        puts("Usage: ub_hexdump <addr_hex> <bytes>");
        return;
    }
    uint64_t addr = strtoull(tok_addr, NULL, 0);
    uint32_t len  = (uint32_t)strtoul(tok_len, NULL, 0);

    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;
    for (uint32_t i = 0; i < len; i++) {
        if ((i & 0x0f) == 0)
            printf("\n%08lx: ", (unsigned long)((addr + i) & 0xffffffffu));
        printf("%02x ", p[i]);
    }
    puts("");
}

static void cmd_cap_beats(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "256", NULL, 0);
    if (v == 0) { puts("cap_beats must be >= 1"); return; }
    main_cap_beats_write(v);
    uc_commit();
    printf("cap_beats = %u\n", (unsigned)v);
}

/* ========================================================================= */
/*                           UDP DDR streamer (FAST)                          */
/* ========================================================================= */

#define UBD3_MAGIC 0x55424433u /* "UBD3" */

struct __attribute__((packed)) ubd3_hdr {
    uint32_t magic;
    uint32_t seq;
    uint32_t offset;
    uint32_t total;
};

/* Your board IP (change if you want) */
#define UBD3_BOARD_IP IPTOINT(192,168,0,123)

/* Keep payload below MTU. 1400 is safe; 1472 may work with MTU1500. */
#define UBD3_PAYLOAD_MAX 1400u

/* Call udp_service() once every N packets (power-of-two recommended) */
#define UBD3_SERVICE_EVERY 1u

/* Small pacing delay to reduce host-side RX overruns on long bursts. */
#define UBD3_PACE_WAIT_MS 1u

/* 0 disables progress completely */
#define UBD3_PROGRESS_EVERY 0u

static inline void u32le_store(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 0);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int parse_ipv4(const char *s, uint32_t *out_ip) {
    unsigned a,b,c,d;
    if (!s) return -1;
    if (sscanf(s, "%u.%u.%u.%u", &a,&b,&c,&d) != 4) return -1;
    if (a>255 || b>255 || c>255 || d>255) return -1;
    *out_ip = IPTOINT(a,b,c,d);
    return 0;
}

/* Fast sender: minimal prints, fewer udp_service() calls, direct header store. */
static void cmd_ub_send(char *args) {
    char *tok_addr = strtok(args, " \t");
    char *tok_len  = strtok(NULL, " \t");
    char *tok_ip   = strtok(NULL, " \t");
    char *tok_port = strtok(NULL, " \t");

    if (!tok_addr || !tok_len || !tok_ip || !tok_port) {
        puts("Usage: ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
        return;
    }

    uint64_t addr  = strtoull(tok_addr, NULL, 0);
    uint32_t total = (uint32_t)strtoul(tok_len, NULL, 0);

    uint32_t dst_ip = 0;
    if (parse_ipv4(tok_ip, &dst_ip) != 0) {
        puts("Error: bad dst_ip format (use a.b.c.d)");
        return;
    }

    uint16_t dst_port = (uint16_t)strtoul(tok_port, NULL, 0);
    uint16_t src_port = dst_port;

    if (total == 0) {
        puts("Error: bytes must be > 0");
        return;
    }

    static const unsigned char board_mac[6] = {0x02,0x00,0x00,0x00,0x00,0xAB};

    eth_init();
    udp_set_mac(board_mac);
    udp_set_ip(UBD3_BOARD_IP);
    udp_start(board_mac, UBD3_BOARD_IP);

    /* ARP resolve (minimal) */
    int ok = 0;
    for (unsigned i = 0; i < 200000; i++) {
        udp_service();
        if (udp_arp_resolve(dst_ip) != 0) { ok = 1; break; }
    }
    if (!ok) {
        puts("No ARP reply.");
        return;
    }

    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;

    const uint32_t hdr_sz   = 16u;
    const uint32_t max_data = (UBD3_PAYLOAD_MAX > hdr_sz) ? (UBD3_PAYLOAD_MAX - hdr_sz) : 0u;
    if (max_data < 64u) {
        puts("Error: UBD3_PAYLOAD_MAX too small");
        return;
    }

    uint32_t sent = 0;
    uint32_t seq  = 0;

    /* If SERVICE_EVERY is not power-of-two, fallback to modulo logic */
    const uint32_t service_mask = (UBD3_SERVICE_EVERY && ((UBD3_SERVICE_EVERY & (UBD3_SERVICE_EVERY - 1u)) == 0u))
    ? (UBD3_SERVICE_EVERY - 1u)
    : 0u;

    while (sent < total) {
        /* keep the network stack alive (not every packet) */
        if (UBD3_SERVICE_EVERY) {
            if (service_mask) {
                if ((seq & service_mask) == 0u) udp_service();
            } else {
                if ((seq % UBD3_SERVICE_EVERY) == 0u) udp_service();
            }
        }

        uint32_t chunk = total - sent;
        if (chunk > max_data) chunk = max_data;

        uint8_t *tx = (uint8_t*)udp_get_tx_buffer();
        if (!tx) return;

        /* Write header directly (little-endian) */
        u32le_store(tx + 0,  UBD3_MAGIC);
        u32le_store(tx + 4,  seq);
        u32le_store(tx + 8,  sent);
        u32le_store(tx + 12, total);

        memcpy(tx + hdr_sz, (const void*)(p + sent), chunk);
        (void)udp_send(src_port, dst_port, (unsigned)(hdr_sz + chunk));
        if (UBD3_PACE_WAIT_MS)
            busy_wait(UBD3_PACE_WAIT_MS);

        sent += chunk;
        seq++;

    if (UBD3_PROGRESS_EVERY && ((seq % UBD3_PROGRESS_EVERY) == 0u)) {
        printf("sent %lu / %lu\n", (unsigned long)sent, (unsigned long)total);
    }
    }
}

/* ========================================================================= */
/*                           Command registration                             */
/* ========================================================================= */

static const struct cmd_entry uc_tbl[] = {
    /* UberClock commands */
    {"help_uc",              uc_help,                 "UberClock help"},
{"fft64_peak", cmd_fft64_peak, "64-point FFT over DS FIFO IQ samples, print peak only"},

    {"phase_nco",            cmd_phase_nco,           "Set input CORDIC NCO phase increment"},
    {"nco_mag",              cmd_nco_mag,             "Set NCO magnitude (signed 12-bit)"},

    {"phase_down_1",         cmd_phase_down_1,        "Set downconversion ch1 phase inc"},
    {"phase_down_2",         cmd_phase_down_2,        "Set downconversion ch2 phase inc"},
    {"phase_down_3",         cmd_phase_down_3,        "Set downconversion ch3 phase inc"},
    {"phase_down_4",         cmd_phase_down_4,        "Set downconversion ch4 phase inc"},
    {"phase_down_5",         cmd_phase_down_5,        "Set downconversion ch5 phase inc"},
    {"phase_down_ref",       cmd_phase_down_ref,      "Set downconversion ref phase inc"},

    {"phase_cpu1",           cmd_phase_cpu1,          "Set CPU NCO phase inc ch1"},
    {"phase_cpu2",           cmd_phase_cpu2,          "Set CPU NCO phase inc ch2"},
    {"phase_cpu3",           cmd_phase_cpu3,          "Set CPU NCO phase inc ch3"},
    {"phase_cpu4",           cmd_phase_cpu4,          "Set CPU NCO phase inc ch4"},
    {"phase_cpu5",           cmd_phase_cpu5,          "Set CPU NCO phase inc ch5"},

    {"mag_cpu1",             cmd_mag_cpu1,            "Set CPU NCO magnitude ch1"},
    {"mag_cpu2",             cmd_mag_cpu2,            "Set CPU NCO magnitude ch2"},
    {"mag_cpu3",             cmd_mag_cpu3,            "Set CPU NCO magnitude ch3"},
    {"mag_cpu4",             cmd_mag_cpu4,            "Set CPU NCO magnitude ch4"},
    {"mag_cpu5",             cmd_mag_cpu5,            "Set CPU NCO magnitude ch5"},

    {"output_select_ch1",    cmd_output_sel_ch1,      "Select DAC1 source (0..15, 14=ref_y, 15=sum)"},
    {"output_select_ch2",    cmd_output_sel_ch2,      "Select DAC2 source (0..15, 14=ref_y, 15=sum)"},
    {"input_select",         cmd_input_select,        "Set input select register"},
    {"upsampler_input_mux",  cmd_ups_in_mux,          "Set upsampler input mux (0..2)"},

    {"lowspeed_dbg_select",  cmd_lowspeed_dbg_select, "Select low-speed debug source (0..7, 6=ref_y)"},
    {"highspeed_dbg_select", cmd_highspeed_dbg_select,"Select high-speed debug source (0..3, 1=filter_in_1)"},

    {"upsampler_x",          cmd_upsampler_x,         "Write upsampler_input_x1..x5 (signed 16-bit)"},
    {"upsampler_y",          cmd_upsampler_y,         "Write upsampler_input_y1..y5 (signed 16-bit)"},
    {"ds_pop",               cmd_ds_pop,              "Pop one 6-channel downsampled frame from FIFO"},
    {"ds_status",            cmd_ds_status,           "Show downsample FIFO readable/overflow"},
    {"ups_push",             cmd_ups_push,            "Push one replicated 5-channel frame into upsampler FIFO"},
    {"ups_status",           cmd_ups_status,          "Show upsampler FIFO writable/overflow"},
    {"fifo_health",          cmd_fifo_health,         "Show cumulative ds_fifo/ups_fifo overflow/underflow counts"},
    {"dsp_test",             cmd_dsp_test,            "Run DSP loop over FIFO samples (optional N)"},
    {"dsp_run",              cmd_dsp_run,             "Enable/disable non-blocking DSP pump"},
    {"fft_fs",               cmd_fft_fs,              "Set DS sample rate (Hz) used by fft_ds"},
    {"fft_ds",               cmd_fft_ds,              "Run FFT over downsample FIFO IQ samples and print bins"},
    {"fft_ds_peak",          cmd_fft_ds_peak,         "Run FFT over downsample FIFO IQ samples and print peak only"},
    {"track3",               cmd_track3,              "Sweep phase_down_<ch> until the 3-tone pattern is found"},
    {"trackq_start",         cmd_trackq_start,        "Start 3-point quadratic tracking: trackq_start <f1> <f2> <f3> [N] [center] [delta1] [delta2] [delta3]"},
    {"trackq_probe",         cmd_trackq_probe,        "Capture one 3-point tracking snapshot"},
    {"trackq_stop",          cmd_trackq_stop,         "Stop 3-point quadratic tracking"},

    {"gain1",                cmd_gain1,               "Set gain1"},
    {"gain2",                cmd_gain2,               "Set gain2"},
    {"gain3",                cmd_gain3,               "Set gain3"},
    {"gain4",                cmd_gain4,               "Set gain4"},
    {"gain5",                cmd_gain5,               "Set gain5"},
    {"final_shift",          cmd_final_shift,         "Set final shift"},

    {"cap_arm",              cmd_cap_arm_pulse,       "Pulse cap_arm"},
    {"cap_done",             cmd_cap_done,            "Read cap_done"},
    {"cap_rd",               cmd_cap_rd,              "Read cap_data at index"},

    {"cap_enable",           cmd_cap_enable,          "0=ramp, 1=capture design to DDR"},
    {"cap_beats",            cmd_cap_beats,           "Set capture length in 256-bit beats"},

    {"phase",                cmd_phase_print,         "Print current CORDIC phase (if wired)"},
    {"magnitude",            cmd_magnitude,           "Print current CORDIC magnitude (if wired)"},
    {"cap_start",            cap_start_cmd,           "Start LS debug"},
    {"cap_status",           cap_status_cmd,          "LS debug status"},
    {"cap_dump",             cap_dump_cmd,             "Ls dump cmd"},
    /* UberDDR3 / S2MM commands */
    {"ub_help",              ub_help,                 "UberDDR3/S2MM help"},
    {"ub_info",              cmd_ub_info,             "Show UBDDR3 info/state"},
    {"ub_mode",              cmd_ub_mode,             "Show current cap_enable mode"},
    {"ub_setmode",           cmd_ub_setmode,          "Set cap_enable (0=ramp,1=capture)"},
    {"ub_start",             cmd_ub_start,            "Start S2MM using current mode"},
    {"ub_ramp",              cmd_ub_ramp2,            "Force ramp mode then start S2MM"},
    {"ub_cap",               cmd_ub_cap,              "Force capture mode then start S2MM"},
    {"ub_wait",              cmd_ub_wait,             "Wait until DMA done"},
    {"ub_hexdump",           cmd_ub_hexdump,          "Hexdump DDR memory"},
    {"ub_send",              cmd_ub_send,             "Send DDR memory region via UDP"},
    {"fft32_ds_y", cmd_fft32_ds_y, "Real FFT of 32 Y samples from DS FIFO"},
    {"sig3_start", cmd_sig3_start, "Start 5 independent 3-tone software generators"},
    {"sig3_stop",  cmd_sig3_stop,  "Stop 5 independent 3-tone software generators"},
    {"sig3_amp",   cmd_sig3_amp,   "Set 3-tone per-tone amplitude: sig3_amp <val> | <ch> <val>"},
    {"sig3_freqs", cmd_sig3_freqs, "Set channel 3-tone frequencies: sig3_freqs <ch> <f1> <f2> <f3>"},
    {"sig3_enable_ch",  cmd_sig3_enable_ch,  "Enable one sig3 channel: sig3_enable_ch <ch>"},
    {"sig3_disable_ch", cmd_sig3_disable_ch, "Disable one sig3 channel: sig3_disable_ch <ch>"},
};

void uberclock_register_cmds(void) {
    console_register(uc_tbl, (unsigned)(sizeof(uc_tbl) / sizeof(uc_tbl[0])));
}

/* ========================================================================= */
/*                            Init / poll functions                           */
/* ========================================================================= */

void uberclock_init(void) {
    main_phase_inc_nco_write(10324440);

    main_phase_inc_down_1_write(uc_phase_inc_from_hz(TRACKQ_CH1_START_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_2_write(uc_phase_inc_from_hz(TRACKQ_CH2_START_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_3_write(uc_phase_inc_from_hz(TRACKQ_CH3_START_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_4_write(80644);
    main_phase_inc_down_5_write(80640);

    main_phase_inc_down_ref_write(uc_phase_inc_from_hz(9999000u, TRACK3_RF_FS_HZ));

    main_nco_mag_write((uint32_t)(300 & 0x0fff));

    main_phase_inc_cpu1_write(52429);
    main_phase_inc_cpu2_write(52429);
    main_phase_inc_cpu3_write(52429);
    main_phase_inc_cpu4_write(52429);
    main_phase_inc_cpu5_write(52429);

    main_mag_cpu1_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu2_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu3_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu4_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu5_write((uint32_t)(0 & 0x0fff));

    main_input_select_write(0);
    main_upsampler_input_mux_write(1);

    main_gain1_write(0x40000000);
    main_gain2_write(0x40000000);
    main_gain3_write(0x40000000);
    main_gain4_write(0x00000000);
    main_gain5_write(0x00000000);

    main_output_select_ch1_write(14);
    main_output_select_ch2_write(0);

    main_final_shift_write(2);

    main_lowspeed_dbg_select_write(5);
    main_highspeed_dbg_select_write(0);

    write_upsampler_inputs_all_x(0);
    write_upsampler_inputs_all_y(0);

    main_upsampler_input_mux_write(1);
    main_cap_enable_write(1);
    cmd_dsp_run("1");
    sig3_start();
    uc_commit();

    dsp_swq_r = 0;
    dsp_swq_w = 0;
    dsp_swq_count = 0;
    fifo_clear_flags();

    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));

    printf("UberClock init done.\n");
}

void uberclock_poll(void) {
    trackq_step();
    trackq_fifo_health_poll();

    while (ce_event) {
        ce_event--;
        service_one_ce_event();
    }
}
