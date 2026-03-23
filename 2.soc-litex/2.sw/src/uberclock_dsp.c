#include "uberclock_core.h"

#define FFT_MAX_N 32u
#define FFT_CFG_MAX_BYTES 768u

static kiss_fft_cpx fft_in[FFT_MAX_N];
static kiss_fft_cpx fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES];
static uint32_t fft_fs_hz = 1000000u;


/* ========================================================================= */
/*                         FIFO DSP test harness                             */
/* ========================================================================= */

static inline void dsp_process(int16_t in_x, int16_t in_y,
                               int16_t *out_x, int16_t *out_y) {
    /* Passthrough. */
    *out_x = in_x;
    *out_y = in_y;
}

unsigned dsp_pump_step(unsigned max_in, unsigned max_out) {
    unsigned popped = 0;
    unsigned i;

    /* Stage A: drain input aggressively into SW queue. */
    for (i = 0; i < max_in; i++) {
        if (((main_ds_fifo_flags_read() & 0x1u) != 0u) && (dsp_swq_count < DSP_SWQ_LEN)) {
            main_ds_fifo_pop_write(1);
            int16_t in_x = (int16_t)(main_ds_fifo_x_read() & 0xffff);
            int16_t in_y = (int16_t)(main_ds_fifo_y_read() & 0xffff);

            int16_t out_x = 0, out_y = 0;
            dsp_process(in_x, in_y, &out_x, &out_y);

            dsp_swq_x[dsp_swq_w] = out_x;
            dsp_swq_y[dsp_swq_w] = out_y;
            dsp_swq_w = (dsp_swq_w + 1u) % DSP_SWQ_LEN;
            dsp_swq_count++;
            popped++;
        } else {
            break;
        }
    }

    /* Stage B: push with a smaller cap to avoid large output bursts. */
    for (i = 0; i < max_out; i++) {
        if ((dsp_swq_count != 0u) && ((main_ups_fifo_flags_read() & 0x2u) != 0u)) {
            int16_t out_x = dsp_swq_x[dsp_swq_r];
            int16_t out_y = dsp_swq_y[dsp_swq_r];
            dsp_swq_r = (dsp_swq_r + 1u) % DSP_SWQ_LEN;
            dsp_swq_count--;

            main_ups_fifo_x_write((uint32_t)((int32_t)out_x & 0xffff));
            main_ups_fifo_y_write((uint32_t)((int32_t)out_y & 0xffff));
            main_ups_fifo_push_write(1);
        } else {
            break;
        }
    }
    return popped;
}

void fifo_clear_flags(void) {
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
    main_ds_fifo_pop_write(1);
    uint32_t vx = main_ds_fifo_x_read();
    uint32_t vy = main_ds_fifo_y_read();
    int16_t sx = (int16_t)(vx & 0xffff);
    int16_t sy = (int16_t)(vy & 0xffff);
    printf("ds_fifo: x=%d y=%d\n", (int)sx, (int)sy);
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
    main_ups_fifo_x_write((uint32_t)((int32_t)x & 0xffff));
    main_ups_fifo_y_write((uint32_t)((int32_t)y & 0xffff));
    main_ups_fifo_push_write(1);
    printf("ups_fifo push: x=%d y=%d\n", x, y);
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
        fifo_clear_flags();
    } else {
        dsp_work_tokens = 0;
    }
    printf("dsp_run = %u\n", dsp_pump_enable);
}

static void cmd_fft_fs(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "0", NULL, 0);
    if (v == 0u) {
        puts("Usage: fft_fs <Hz>, Hz must be > 0");
        return;
    }
    fft_fs_hz = v;
    printf("fft_fs = %lu Hz\n", (unsigned long)fft_fs_hz);
}

static void cmd_fft_ds(char *args) {
    char *tok = strtok(args, " \t");
    unsigned n = tok ? (unsigned)strtoul(tok, NULL, 0) : 32u;
    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("Usage: fft_ds [N], N must be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }

    for (unsigned i = 0; i < n; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }
        main_ds_fifo_pop_write(1);
        int16_t sx = (int16_t)(main_ds_fifo_x_read() & 0xffffu);
        int16_t sy = (int16_t)(main_ds_fifo_y_read() & 0xffffu);
        fft_in[i].r = (kiss_fft_scalar)sx;
        fft_in[i].i = (kiss_fft_scalar)sy;
    }

    size_t cfg_need = 0;
    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    size_t cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    kiss_fft_cfg cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed (static cfg)");
        return;
    }

    kiss_fft(cfg, fft_in, fft_out);

    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;
    unsigned bins = (n / 2u);
    for (unsigned k = 0; k < bins; k++) {
        int32_t re = (int32_t)fft_out[k].r;
        int32_t im = (int32_t)fft_out[k].i;
        uint64_t pwr = (uint64_t)(re * re) + (uint64_t)(im * im);
        printf("bin[%3u] re=%7ld im=%7ld pwr=%10llu\n",
               k, (long)re, (long)im, (unsigned long long)pwr);
        if (k > 0u && pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = k;
        }
    }

    if (bins > 1u) {
        uint64_t f_hz = ((uint64_t)peak_k * (uint64_t)fft_fs_hz) / (uint64_t)n;
        printf("peak: bin=%u f=%llu Hz (Fs=%lu, N=%u, pwr=%llu)\n",
               peak_k,
               (unsigned long long)f_hz,
               (unsigned long)fft_fs_hz,
               n,
               (unsigned long long)peak_pwr);
    }
}

const struct cmd_entry uberclock_dsp_cmds[] = {
    {"ds_pop",               cmd_ds_pop,              "Pop one downsampled sample from FIFO"},
    {"ds_status",            cmd_ds_status,           "Show downsample FIFO readable/overflow"},
    {"ups_push",             cmd_ups_push,            "Push one sample into upsampler FIFO"},
    {"ups_status",           cmd_ups_status,          "Show upsampler FIFO writable/overflow"},
    {"dsp_test",             cmd_dsp_test,            "Run DSP loop over FIFO samples (optional N)"},
    {"dsp_run",              cmd_dsp_run,             "Enable/disable non-blocking DSP pump"},
    {"fft_fs",               cmd_fft_fs,              "Set DS sample rate (Hz) used by fft_ds"},
    {"fft_ds",               cmd_fft_ds,              "Run FFT over downsample FIFO IQ samples"},
};

const unsigned uberclock_dsp_cmd_count =
    (unsigned)(sizeof(uberclock_dsp_cmds) / sizeof(uberclock_dsp_cmds[0]));
