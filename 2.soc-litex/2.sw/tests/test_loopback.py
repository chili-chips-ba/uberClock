#!/usr/bin/env python3
"""Compile the actual capture/estimator functions with a mock CSR FIFO.

No board or Python packages required. The production C functions are extracted
verbatim; KISS FFT is compiled in the same FIXED_POINT=16 mode as firmware.
"""
from pathlib import Path
import importlib.util
import math
import re
import subprocess
import tempfile

SW = Path(__file__).resolve().parents[1]
SOURCE = (SW / "uberclock.c").read_text()


def function(name):
    match = re.search(r"^static [^;{}]*\b" + name + r"\([^;{}]*\)\s*\{.*?^\}",
                      SOURCE, re.M | re.S)
    if not match:
        raise RuntimeError(f"Cannot find production function {name}")
    return match[0]


PREAMBLE = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kiss_fft.h"
#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 12288u
#define TRACK3_FIFO_WAIT_POLLS 1000000u
typedef struct { int16_t x[6], y[6]; } iq6_frame_t;
static kiss_fft_cpx fft_in[FFT_MAX_N], fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES] __attribute__((aligned(8)));
static uint32_t fft_fs_hz = 10000;
static int16_t track_samples[3][FFT_MAX_N], track_samples_ref[FFT_MAX_N];
static unsigned ds_capture_drained, ds_capture_old_overflow;
static unsigned ds_capture_overflow, ds_capture_underflow;
static unsigned trackq_dbg_k_peak;
static int64_t trackq_dbg_y1, trackq_dbg_y2, trackq_dbg_y3, trackq_dbg_den;

static unsigned old_count, ready, source_id, pops, ov, uf, clear_delay;
static int perpetual, stalled, stuck_clear, inject_ov, inject_uf, mock_fft;
static int16_t latched;
static void reset_fifo(void) {
    old_count = 16384; ready = source_id = pops = uf = clear_delay = 0;
    ov = 1;
    perpetual = stalled = stuck_clear = inject_ov = inject_uf = 0;
}
static unsigned main_ds_fifo_flags_read(void) {
    if (clear_delay && !--clear_delay && !stuck_clear) ov = uf = 0;
    return perpetual || old_count || ready;
}
static unsigned main_ds_fifo_overflow_read(void) { return ov; }
static unsigned main_ds_fifo_underflow_read(void) { return uf; }
static void main_ds_fifo_clear_write(unsigned value) {
    (void)value; clear_delay = 8;
}
static void main_ds_fifo_pop_write(unsigned value) {
    (void)value; ++pops;
    if (perpetual) return;
    if (old_count) { --old_count; latched = -123; return; }
    if (!ready) { uf = 1; return; }
    ready = 0; latched = (int16_t)source_id++;
    if (inject_ov && source_id == 270) ov = 1;
    if (inject_uf && source_id == 270) uf = 1;
}
static unsigned main_ds_fifo_x1_read(void) { return (uint16_t)latched; }
static unsigned main_ds_fifo_y1_read(void) { return (uint16_t)latched; }
static void ds_fifo_read_frame(iq6_frame_t *frame) {
    main_ds_fifo_pop_write(1);
    for (unsigned i = 0; i < 6; ++i) frame->x[i] = frame->y[i] = latched;
}
static void track3_service_background(void) {
    if (!stalled && !old_count) ready = 1;
}
static void track3_service_background_budget(unsigned budget) { (void)budget; }
static void test_fft(kiss_fft_cfg cfg, const kiss_fft_cpx *in, kiss_fft_cpx *out) {
    if (!mock_fft) { kiss_fft(cfg, in, out); return; }
    memset(out, 0, sizeof(fft_out));
    if (mock_fft == 1) { out[3].r = 2; out[4].r = 10; out[5].r = 10; }
    if (mock_fft == 2) { out[6].r = 2; out[7].r = 10; out[8].r = 2; }
}
'''

TESTS = r'''
int main(void) {
    reset_fifo();
    assert(capture_ds_track_multi(2048, 256));
    assert(ds_capture_drained == 16384 && ds_capture_old_overflow == 1);
    assert(!ds_capture_overflow && !ds_capture_underflow);
    for (unsigned i = 0; i < 2048; ++i) assert(track_samples[0][i] == (int16_t)(256 + i));

    reset_fifo(); perpetual = 1;
    assert(!capture_ds_track_multi(64, 256));
    assert(pops == 65536);
    reset_fifo(); stuck_clear = 1;
    assert(!capture_ds_track_multi(64, 256));
    reset_fifo(); stalled = 1;
    assert(!capture_ds_track_multi(64, 256));
    reset_fifo(); inject_ov = 1;
    assert(!capture_ds_track_multi(64, 256));
    assert(ds_capture_overflow);
    reset_fifo(); inject_uf = 1;
    assert(!capture_ds_track_multi(64, 256));
    assert(ds_capture_underflow);
    reset_fifo();
    assert(capture_ds_fft_channel(2, 64, 256));
    for (unsigned i = 0; i < 64; ++i) assert(fft_in[i].r == (int16_t)(256 + i));
    assert(!capture_ds_fft_channel(6, 64, 256));

    const double beat = 1032.0 * 65000000.0 / (1u << 26);
    int64_t estimate = 0;
    for (unsigned i = 0; i < 2048; ++i)
        track_samples[0][i] = (int16_t)lrint(12000*cos(2*3.141592653589793*beat*i/10000));
    assert(trackq_fft_peak_vertex_estimate_mhz(track_samples[0], 2048, 0, 0, &estimate));
    assert(llabs(estimate - 1000822) < 15); /* rectangular power-fit bias is expected */
    memset(track_samples, 0, sizeof(track_samples));
    assert(!trackq_fft_peak_vertex_estimate_mhz(track_samples[0], 2048, 0, 0, &estimate));
    assert(estimate == 0 && trackq_dbg_k_peak == 0 && trackq_dbg_den == 0);
    mock_fft = 1;
    assert(trackq_fft_peak_vertex_estimate_mhz(track_samples[0], 64, 0, 0, &estimate));
    assert(estimate == 703125); /* equal adjacent peaks: valid half-bin */
    mock_fft = 2;
    assert(trackq_fft_peak_vertex_estimate_mhz(track_samples[0], 64, 1000000, 20, &estimate));
    assert(estimate == 1093750); /* coarse-bin upper search edge included */
    assert(uc_phase_inc_from_hz(10000000, 65000000) == 10324441);
    assert(uc_phase_inc_from_hz(0, 65000000) == 0);
    assert(uc_phase_inc_from_hz(1, 0) == 0);
    assert(uc_phase_inc_to_mhz(1032, 65000000) == 999570);
    puts("PASS: fresh FIFO, drain limit, delayed clear, clear failure, timeout,");
    puts("      overflow/underflow rejection, channel capture, fixed-point FFT,");
    puts("      stale diagnostics, half-bin tie, coarse search, DDS rounding");
    return 0;
}
'''


def main():
    names = ["is_pow2_u", "uc_phase_inc_from_hz", "uc_phase_inc_to_mhz",
             "ds_fifo_flush_all", "track3_wait_ds_fifo", "ds_capture_end",
             "ds_capture_begin", "capture_ds_fft_channel", "capture_ds_track_multi",
             "fft_bin_power_at"]
    code = PREAMBLE + "\n".join(function(name) for name in names)
    code += "\n#define kiss_fft test_fft\n" + function("trackq_fft_peak_vertex_estimate_mhz")
    code += "\n#undef kiss_fft\n" + TESTS
    with tempfile.TemporaryDirectory(prefix="uberclock-test-") as directory:
        path = Path(directory)
        (path / "test.c").write_text(code)
        subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-error=sign-compare", "-O1",
                        "-fsanitize=undefined", "-fno-sanitize-recover=undefined", "-DFIXED_POINT=16", "-I", str(SW / "kissfft"),
                        str(path / "test.c"), str(SW / "kiss_fft.c"), "-lm",
                        "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
    analyzer_path = SW.parent / "3.build" / "analyze_loopback_capture.py"
    spec = importlib.util.spec_from_file_location("capture_analysis", analyzer_path)
    analyzer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(analyzer)
    expected = 1032 * 65000000 / 2**26
    for n in (64, 256, 1024, 2048):
        samples = [round(12000 * math.cos(2*math.pi*expected*i/10000 + .7) + 200)
                   for i in range(n)]
        _, frequency, rms = analyzer.fit_capture(samples, 10000)
        assert abs(frequency - expected) < .01, (n, frequency)
        assert rms < .4, (n, rms)
    print("PASS: host sine fit on quantized synthetic records at all four FFT lengths")


if __name__ == "__main__":
    main()
