#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../console.h"
#include "uberclock/uberclock.h"
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_commands.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_channels.h"
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_fft.h"
#include "uberclock/uberclock_track.h"
#include "uberclock/uberclock_siggen.h"
#include "uberclock/uberclock_capture.h"
#include "uberclock/uberclock_dma.h"
#include "uberclock/uberclock_parse.h"

static void command_help(char *args);
static void command_dma_help(char *args);

static int parse_channel_index(const char *text, const char *what, unsigned *out_channel_index) {
    unsigned one_based_channel = uberclock_parse_unsigned(text, UBERCLOCK_CHANNEL_COUNT + 1u, what);
    if (one_based_channel == 0u || one_based_channel > UBERCLOCK_CHANNEL_COUNT) {
        printf("Error: %s must be 1..%u\n", what, UBERCLOCK_CHANNEL_COUNT);
        return -1;
    }
    *out_channel_index = one_based_channel - 1u;
    return 0;
}

static void command_phase_nco(char *args) {
    unsigned phase_increment = uberclock_parse_unsigned(args, 1u << 26, "phase_nco");
    if (phase_increment >= (1u << 26)) {
        return;
    }

    uberclock_set_nco_phase_increment(phase_increment);
    uberclock_commit_config();
    printf("Input NCO phase increment set to %u\n", phase_increment);
}

static void command_nco_mag(char *args) {
    int magnitude = uberclock_parse_signed(args, -2048, 2047, "nco_mag");
    if (magnitude < -2048 || magnitude > 2047) {
        return;
    }

    uberclock_set_nco_magnitude((int16_t)magnitude);
    uberclock_commit_config();
    printf("nco_mag set to %d\n", magnitude);
}

static void command_phase_down(char *args) {
    char *channel_text = strtok(args, " \t");
    char *value_text = strtok(NULL, " \t");
    unsigned channel_index;
    unsigned phase_increment;

    if (!channel_text || !value_text) {
        puts("Usage: phase_down <channel> <value>");
        return;
    }
    if (parse_channel_index(channel_text, "channel", &channel_index) != 0) {
        return;
    }

    phase_increment = uberclock_parse_unsigned(value_text, 1u << 26, "phase_down");
    if (phase_increment >= (1u << 26)) {
        return;
    }

    (void)uberclock_channel_set_phase_down(channel_index, phase_increment);
    uberclock_commit_config();
    printf("Downconversion phase ch%u increment set to %u\n", channel_index + 1u, phase_increment);
}

static void command_phase_down_ref(char *args) {
    unsigned phase_increment = uberclock_parse_unsigned(args, 1u << 26, "phase_down_ref");
    if (phase_increment >= (1u << 26)) {
        return;
    }

    uberclock_set_phase_down_reference(phase_increment);
    uberclock_commit_config();
    printf("Downconversion phase ref increment set to %u\n", phase_increment);
}

static void command_phase_cpu(char *args) {
    char *channel_text = strtok(args, " \t");
    char *value_text = strtok(NULL, " \t");
    unsigned channel_index;
    unsigned phase_increment;

    if (!channel_text || !value_text) {
        puts("Usage: phase_cpu <channel> <value>");
        return;
    }
    if (parse_channel_index(channel_text, "channel", &channel_index) != 0) {
        return;
    }

    phase_increment = uberclock_parse_unsigned(value_text, 1u << 26, "phase_cpu");
    if (phase_increment >= (1u << 26)) {
        return;
    }

    (void)uberclock_channel_set_phase_cpu(channel_index, phase_increment);
    uberclock_commit_config();
    printf("CPU phase increment ch%u set to %u\n", channel_index + 1u, phase_increment);
}

static void command_mag_cpu(char *args) {
    char *channel_text = strtok(args, " \t");
    char *value_text = strtok(NULL, " \t");
    unsigned channel_index;
    int magnitude;

    if (!channel_text || !value_text) {
        puts("Usage: mag_cpu <channel> <value>");
        return;
    }
    if (parse_channel_index(channel_text, "channel", &channel_index) != 0) {
        return;
    }

    magnitude = uberclock_parse_signed(value_text, -2048, 2047, "mag_cpu");
    if (magnitude < -2048 || magnitude > 2047) {
        return;
    }

    (void)uberclock_channel_set_magnitude_cpu(channel_index, (int16_t)magnitude);
    uberclock_commit_config();
    printf("mag_cpu ch%u set to %d\n", channel_index + 1u, magnitude);
}

static void command_gain(char *args) {
    char *channel_text = strtok(args, " \t");
    char *value_text = strtok(NULL, " \t");
    unsigned channel_index;
    int32_t gain;

    if (!channel_text || !value_text) {
        puts("Usage: gain <channel> <value>");
        return;
    }
    if (parse_channel_index(channel_text, "channel", &channel_index) != 0) {
        return;
    }

    gain = (int32_t)strtol(value_text, NULL, 0);
    (void)uberclock_channel_set_gain(channel_index, gain);
    uberclock_commit_config();
    printf("Gain%u register set to %ld (0x%08lX)\n",
           channel_index + 1u,
           (long)gain,
           (unsigned long)(uint32_t)gain);
}

static void command_lowspeed_dbg_select(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    if (value > 7u) {
        puts("lowspeed_dbg_select must be 0..7");
        return;
    }
    uberclock_set_lowspeed_debug_select(value);
    uberclock_commit_config();
    printf("lowspeed_dbg_select = %u\n", value);
}

static void command_highspeed_dbg_select(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    if (value > 3u) {
        puts("highspeed_dbg_select must be 0..3");
        return;
    }
    uberclock_set_highspeed_debug_select(value);
    uberclock_commit_config();
    printf("highspeed_dbg_select = %u\n", value);
}

static void command_output_select_ch1(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_set_output_select_ch1(value);
    uberclock_commit_config();
    printf("output_select_ch1 set to %u\n", value & 0x0fu);
}

static void command_output_select_ch2(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_set_output_select_ch2(value);
    uberclock_commit_config();
    printf("output_select_ch2 set to %u\n", value & 0x0fu);
}

static void command_input_select(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_set_input_select(value);
    uberclock_commit_config();
    printf("Main input select register set to %u\n", value);
}

static void command_upsampler_input_mux(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_set_upsampler_input_mux(value);
    uberclock_commit_config();
    printf("Upsampler input mux register set to %u\n", value);
}

static void command_final_shift(char *args) {
    int32_t value = (int32_t)strtol(args ? args : "0", NULL, 0);
    uberclock_set_final_shift(value);
    uberclock_commit_config();
    printf("final_shift set to %ld (0x%08lX)\n", (long)value, (unsigned long)(uint32_t)value);
}

static void command_cap_enable(char *args) {
    unsigned value = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_set_capture_enable(value);
    uberclock_commit_config();
    printf("cap_enable = %u (%s)\n", value ? 1u : 0u, value ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void command_cap_beats(char *args) {
    uint32_t beats = (uint32_t)strtoul(args ? args : "256", NULL, 0);
    if (beats == 0u) {
        puts("cap_beats must be >= 1");
        return;
    }
    uberclock_set_capture_beats(beats);
    uberclock_commit_config();
    printf("cap_beats = %u\n", (unsigned)beats);
}

static void command_upsampler_x(char *args) {
    int value = uberclock_parse_signed(args, -32768, 32767, "upsampler_x");
    if (value < -32768 || value > 32767) {
        return;
    }
    uberclock_set_upsampler_input_x((int16_t)value);
    uberclock_commit_config();
    printf("upsampler_input_x = %d\n", value);
}

static void command_upsampler_y(char *args) {
    int value = uberclock_parse_signed(args, -32768, 32767, "upsampler_y");
    if (value < -32768 || value > 32767) {
        return;
    }
    uberclock_set_upsampler_input_y((int16_t)value);
    uberclock_commit_config();
    printf("upsampler_input_y = %d\n", value);
}

static void command_ds_pop(char *args) {
    int16_t sample_x;
    int16_t sample_y;
    (void)args;

    if (!uberclock_ds_fifo_pop_simple(&sample_x, &sample_y)) {
        puts("ds_fifo empty");
        return;
    }
    printf("ds_fifo: x=%d y=%d\n", (int)sample_x, (int)sample_y);
}

static void command_ds_status(char *args) {
    (void)args;
    printf("ds_fifo: readable=%u overflow=%u underflow=%u\n",
           uberclock_ds_fifo_flags() & 1u,
           uberclock_ds_fifo_overflow(),
           uberclock_ds_fifo_underflow());
    uberclock_ds_fifo_clear_status();
}

static void command_ups_push(char *args) {
    char *sample_x_text = strtok(args, " \t");
    char *sample_y_text = strtok(NULL, " \t");
    int sample_x;
    int sample_y;

    if (!sample_x_text || !sample_y_text) {
        puts("Usage: ups_push <x> <y>");
        return;
    }

    sample_x = uberclock_parse_signed(sample_x_text, -32768, 32767, "ups_x");
    sample_y = uberclock_parse_signed(sample_y_text, -32768, 32767, "ups_y");
    if (sample_x < -32768 || sample_x > 32767 || sample_y < -32768 || sample_y > 32767) {
        return;
    }

    if (!uberclock_ups_fifo_push((int16_t)sample_x, (int16_t)sample_y)) {
        puts("ups_fifo full");
        return;
    }

    printf("ups_fifo push: x=%d y=%d\n", sample_x, sample_y);
}

static void command_ups_status(char *args) {
    (void)args;
    printf("ups_fifo: writable=%u overflow=%u underflow=%u\n",
           (uberclock_ups_fifo_flags() >> 1) & 1u,
           uberclock_ups_fifo_overflow(),
           uberclock_ups_fifo_underflow());
    uberclock_ups_fifo_clear_status();
}

static void command_dsp_test(char *args) {
    unsigned limit = args ? (unsigned)strtoul(args, NULL, 0) : 0u;
    unsigned processed = 0u;
    unsigned stall = 0u;
    const unsigned stall_max = 1000000u;

    while (limit == 0u || processed < limit) {
        unsigned step = uberclock_dsp_pump_step(64u, 64u);
        if (step == 0u) {
            ++stall;
            if (stall >= stall_max) {
                break;
            }
            continue;
        }
        stall = 0u;
        processed += step;
    }

    printf("dsp_test processed %u samples (stall=%u)\n", processed, stall);
}

static void command_fft_fs(char *args) {
    uint32_t sample_rate_hz = (uint32_t)strtoul(args ? args : "0", NULL, 0);
    if (sample_rate_hz == 0u) {
        puts("Usage: fft_fs <Hz>, Hz must be > 0");
        return;
    }

    uberclock_fft_set_sample_rate(sample_rate_hz);
    printf("fft_fs = %lu Hz\n", (unsigned long)sample_rate_hz);
}

static void command_fft_ds(char *args) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_count = args && *args ? (unsigned)strtoul(args, NULL, 0) : 32u;
    unsigned bin_count;
    unsigned bin_index;
    uint64_t peak_power = 0u;
    unsigned peak_bin = 0u;

    if (!uberclock_fft_is_power_of_two(sample_count) || sample_count < 8u || sample_count > UBERCLOCK_FFT_MAX_N) {
        printf("Usage: fft_ds [N], N must be power-of-2 and <= %u\n", UBERCLOCK_FFT_MAX_N);
        return;
    }
    if (!uberclock_fft_capture_ds_iq(sample_count) || !uberclock_fft_execute(sample_count)) {
        return;
    }

    bin_count = sample_count / 2u;
    for (bin_index = 0u; bin_index < bin_count; ++bin_index) {
        int32_t real_part = (int32_t)fft->fft_out[bin_index].r;
        int32_t imag_part = (int32_t)fft->fft_out[bin_index].i;
        uint64_t power = (uint64_t)(real_part * real_part) + (uint64_t)(imag_part * imag_part);

        printf("bin[%3u] re=%7ld im=%7ld pwr=%10llu\n",
               bin_index,
               (long)real_part,
               (long)imag_part,
               (unsigned long long)power);
        if (bin_index > 0u && power > peak_power) {
            peak_power = power;
            peak_bin = bin_index;
        }
    }

    if (bin_count > 1u) {
        uint64_t frequency_hz = ((uint64_t)peak_bin * (uint64_t)uberclock_fft_sample_rate()) / (uint64_t)sample_count;
        printf("peak: bin=%u f=%llu Hz (Fs=%lu, N=%u, pwr=%llu)\n",
               peak_bin,
               (unsigned long long)frequency_hz,
               (unsigned long)uberclock_fft_sample_rate(),
               sample_count,
               (unsigned long long)peak_power);
    }
}

static void command_fft64_peak(char *args) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_count = 64u;
    unsigned bin_count;
    unsigned bin_index;
    uint64_t peak_power = 0u;
    unsigned peak_bin = 0u;

    (void)args;
    if (!uberclock_fft_capture_ds_iq(sample_count) || !uberclock_fft_execute(sample_count)) {
        return;
    }

    bin_count = sample_count / 2u;
    for (bin_index = 1u; bin_index < bin_count; ++bin_index) {
        int32_t real_part = (int32_t)fft->fft_out[bin_index].r;
        int32_t imag_part = (int32_t)fft->fft_out[bin_index].i;
        uint64_t power = (uint64_t)((int64_t)real_part * real_part) + (uint64_t)((int64_t)imag_part * imag_part);

        if (power > peak_power) {
            peak_power = power;
            peak_bin = bin_index;
        }
    }

    printf("fft64 peak: bin=%u f=%llu Hz pwr=%llu (Fs=%lu, N=%u, df=%lu Hz)\n",
           peak_bin,
           (unsigned long long)(((uint64_t)peak_bin * (uint64_t)uberclock_fft_sample_rate()) / (uint64_t)sample_count),
           (unsigned long long)peak_power,
           (unsigned long)uberclock_fft_sample_rate(),
           sample_count,
           (unsigned long)(uberclock_fft_sample_rate() / sample_count));
}

static void command_fft32_ds_y(char *args) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_count = 32u;
    unsigned bin_index;
    uint64_t peak_power = 0u;
    unsigned peak_bin = 0u;

    (void)args;
    if (!uberclock_fft_capture_ds_y32() || !uberclock_fft_execute(sample_count)) {
        return;
    }

    puts("bin,freq_hz,re,im,pwr");
    for (bin_index = 0u; bin_index < (sample_count / 2u); ++bin_index) {
        int32_t real_part = (int32_t)fft->fft_out[bin_index].r;
        int32_t imag_part = (int32_t)fft->fft_out[bin_index].i;
        uint64_t power = (uint64_t)((int64_t)real_part * real_part) + (uint64_t)((int64_t)imag_part * imag_part);
        uint64_t frequency_hz = ((uint64_t)bin_index * (uint64_t)10000u) / (uint64_t)sample_count;

        printf("%2u,%5llu,%8ld,%8ld,%12llu\n",
               bin_index,
               (unsigned long long)frequency_hz,
               (long)real_part,
               (long)imag_part,
               (unsigned long long)power);
        if (bin_index > 0u && power > peak_power) {
            peak_power = power;
            peak_bin = bin_index;
        }
    }

    printf("fft32_ds_y peak: bin=%u f=%llu Hz pwr=%llu  (Fs=%lu, N=%u, df=%lu Hz)\n",
           peak_bin,
           (unsigned long long)(((uint64_t)peak_bin * (uint64_t)10000u) / (uint64_t)sample_count),
           (unsigned long long)peak_power,
           (unsigned long)10000u,
           sample_count,
           (unsigned long)(10000u / sample_count));
}

static void command_track3(char *args) {
    char *start_text = strtok(args, " \t");
    char *step_text = strtok(NULL, " \t");
    char *steps_text = strtok(NULL, " \t");
    char *n_text = strtok(NULL, " \t");
    char *center_text = strtok(NULL, " \t");
    char *delta_text = strtok(NULL, " \t");

    if (!start_text) {
        puts("Usage: track3 <start_phase_down_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
        return;
    }

    (void)uberclock_track3_run((uint32_t)strtoul(start_text, NULL, 0),
                               step_text ? (uint32_t)strtoul(step_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_STEP_HZ,
                               steps_text ? (unsigned)strtoul(steps_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_MAX_STEPS,
                               n_text ? (unsigned)strtoul(n_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_N,
                               center_text ? (uint32_t)strtoul(center_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_CENTER_HZ,
                               delta_text ? (uint32_t)strtoul(delta_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_DELTA_HZ);
}

static void command_trackq_start(char *args) {
    char *n_text = strtok(args, " \t");
    char *center_text = strtok(NULL, " \t");
    char *delta_text = strtok(NULL, " \t");
    (void)uberclock_trackq_start(n_text ? (unsigned)strtoul(n_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_N,
                                 center_text ? (uint32_t)strtoul(center_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_CENTER_HZ,
                                 delta_text ? (uint32_t)strtoul(delta_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_DELTA_HZ);
}

static void command_trackq_probe(char *args) {
    char *n_text = strtok(args, " \t");
    char *center_text = strtok(NULL, " \t");
    char *delta_text = strtok(NULL, " \t");
    (void)uberclock_trackq_probe(n_text ? (unsigned)strtoul(n_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_N,
                                 center_text ? (uint32_t)strtoul(center_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_CENTER_HZ,
                                 delta_text ? (uint32_t)strtoul(delta_text, NULL, 0) : UBERCLOCK_TRACK_DEFAULT_DELTA_HZ);
}

static void command_trackq_stop(char *args) {
    (void)args;
    uberclock_track_stop();
}

static void command_cap_arm(char *args) {
    (void)args;
    uberclock_capture_arm_pulse();
    puts("cap_arm pulsed");
}

static void command_cap_done(char *args) {
    (void)args;
    uberclock_capture_print_done();
}

static void command_cap_rd(char *args) {
    unsigned index;
    if (!args || !*args) {
        puts("Usage: cap_rd <idx>");
        return;
    }

    index = (unsigned)strtoul(args, NULL, 0);
    if (index >= UBERCLOCK_CAPTURE_SAMPLE_COUNT) {
        puts("idx must be 0..2047");
        return;
    }

    uberclock_capture_print_sample(index);
}

static void command_phase(char *args) {
    (void)args;
    printf("Phase %ld\n", (long)uberclock_runtime_state()->phase);
}

static void command_magnitude(char *args) {
    (void)args;
    printf("Magnitude %d\n", uberclock_runtime_state()->magnitude);
}

static void command_cap_start(char *args) {
    (void)args;
    uberclock_capture_start();
}

static void command_cap_status(char *args) {
    (void)args;
    uberclock_capture_status();
}

static void command_cap_dump(char *args) {
    (void)args;
    uberclock_capture_dump();
}

static void command_dma_help(char *args) {
    (void)args;
    uberclock_dma_print_help();
}

static void command_ub_info(char *args) {
    (void)args;
    uberclock_dma_print_info();
}

static void command_ub_mode(char *args) {
    (void)args;
    uberclock_dma_print_mode();
}

static void command_ub_setmode(char *args) {
    unsigned mode = (unsigned)strtoul(args ? args : "0", NULL, 0);
    uberclock_dma_set_mode(mode ? 1u : 0u);
}

static void command_ub_start(char *args) {
    char *address_text = strtok(args, " \t");
    char *beats_text = strtok(NULL, " \t");
    char *size_text = strtok(NULL, " \t");

    if (!address_text) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    uberclock_dma_start_current_mode(strtoull(address_text, NULL, 0),
                                     beats_text ? (uint32_t)strtoul(beats_text, NULL, 0) : 256u,
                                     size_text);
}

static void command_ub_ramp(char *args) {
    char *address_text = strtok(args, " \t");
    char *beats_text = strtok(NULL, " \t");
    char *size_text = strtok(NULL, " \t");

    if (!address_text) {
        puts("Usage: ub_ramp <addr_hex> [beats] [size]");
        return;
    }

    uberclock_dma_start_ramp(strtoull(address_text, NULL, 0),
                             beats_text ? (uint32_t)strtoul(beats_text, NULL, 0) : 256u,
                             size_text);
}

static void command_ub_cap(char *args) {
    char *address_text = strtok(args, " \t");
    char *beats_text = strtok(NULL, " \t");
    char *size_text = strtok(NULL, " \t");

    if (!address_text) {
        puts("Usage: ub_cap <addr_hex> [beats] [size]");
        return;
    }

    uberclock_dma_start_capture(strtoull(address_text, NULL, 0),
                                beats_text ? (uint32_t)strtoul(beats_text, NULL, 0) : 256u,
                                size_text);
}

static void command_ub_wait(char *args) {
    (void)args;
    uberclock_dma_wait();
}

static void command_ub_hexdump(char *args) {
    char *address_text = strtok(args, " \t");
    char *length_text = strtok(NULL, " \t");

    if (!address_text || !length_text) {
        puts("Usage: ub_hexdump <addr_hex> <bytes>");
        return;
    }

    uberclock_dma_hexdump(strtoull(address_text, NULL, 0), (uint32_t)strtoul(length_text, NULL, 0));
}

static void command_ub_send(char *args) {
    char *address_text = strtok(args, " \t");
    char *length_text = strtok(NULL, " \t");
    char *ip_text = strtok(NULL, " \t");
    char *port_text = strtok(NULL, " \t");

    if (!address_text || !length_text || !ip_text || !port_text) {
        puts("Usage: ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
        return;
    }

    (void)uberclock_dma_send_udp(strtoull(address_text, NULL, 0),
                                 (uint32_t)strtoul(length_text, NULL, 0),
                                 ip_text,
                                 (uint16_t)strtoul(port_text, NULL, 0));
}

static void command_sig3_start(char *args) {
    (void)args;
    uberclock_siggen_start();
}

static void command_sig3_stop(char *args) {
    (void)args;
    uberclock_siggen_stop();
}

static void command_sig3_amp(char *args) {
    int amplitude = uberclock_parse_signed(args, 1, 10000, "sig3_amp");
    if (amplitude < 1 || amplitude > 10000) {
        return;
    }
    uberclock_siggen_set_amplitude((int16_t)amplitude);
    printf("sig3 amplitude per tone = %d\n", uberclock_siggen_amplitude());
}

void uberclock_commands_print_help(void) {
    puts_help_header("UberClock commands");
    puts("  phase_nco <val>");
    puts("  nco_mag <val>");
    puts("  phase_down <channel> <value>");
    puts("  phase_down_ref <value>");
    puts("  phase_cpu <channel> <value>");
    puts("  mag_cpu <channel> <value>");
    puts("  input_select <0..3>");
    puts("  upsampler_input_mux <0..2>");
    puts("  output_select_ch1 <0..15>");
    puts("  output_select_ch2 <0..15>");
    puts("  gain <channel> <int32>");
    puts("  final_shift <0..7>");
    puts("  lowspeed_dbg_select <0..7>");
    puts("  highspeed_dbg_select <0..3>");
    puts("  upsampler_x <val>");
    puts("  upsampler_y <val>");
    puts("  ds_pop");
    puts("  ds_status");
    puts("  ups_push <x> <y>");
    puts("  ups_status");
    puts("  dsp_test [N]");
    puts("  fft_fs <Hz>");
    puts("  fft_ds [N]");
    puts("  fft64_peak");
    puts("  fft32_ds_y");
    puts("  track3 <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
    puts("  trackq_start [N] [center_hz] [delta_hz]");
    puts("  trackq_probe [N] [center_hz] [delta_hz]");
    puts("  trackq_stop");
    puts("  cap_arm | cap_done | cap_rd <idx>");
    puts("  cap_enable <0|1> | cap_beats <N>");
    puts("  cap_start | cap_status | cap_dump");
    puts("  phase | magnitude");
    puts("  ub_help | ub_info | ub_mode | ub_setmode <0|1>");
    puts("  ub_start <addr_hex> [beats] [size]");
    puts("  ub_ramp <addr_hex> [beats] [size]");
    puts("  ub_cap <addr_hex> [beats] [size]");
    puts("  ub_wait | ub_hexdump <addr_hex> <bytes>");
    puts("  ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
    puts("  sig3_start | sig3_stop | sig3_amp <value>");
}

static void command_help(char *args) {
    (void)args;
    uberclock_commands_print_help();
}

static const struct cmd_entry command_table[] = {
    {"help_uc", command_help, "UberClock help"},
    {"phase_nco", command_phase_nco, "Set input CORDIC NCO phase increment"},
    {"nco_mag", command_nco_mag, "Set NCO magnitude (signed 12-bit)"},
    {"phase_down", command_phase_down, "Set downconversion phase increment by channel"},
    {"phase_down_ref", command_phase_down_ref, "Set downconversion ref phase increment"},
    {"phase_cpu", command_phase_cpu, "Set CPU NCO phase increment by channel"},
    {"mag_cpu", command_mag_cpu, "Set CPU NCO magnitude by channel"},
    {"output_select_ch1", command_output_select_ch1, "Select DAC1 source (0..15)"},
    {"output_select_ch2", command_output_select_ch2, "Select DAC2 source (0..15)"},
    {"input_select", command_input_select, "Set input select register"},
    {"upsampler_input_mux", command_upsampler_input_mux, "Set upsampler input mux (0..2)"},
    {"lowspeed_dbg_select", command_lowspeed_dbg_select, "Select low-speed debug source"},
    {"highspeed_dbg_select", command_highspeed_dbg_select, "Select high-speed debug source"},
    {"upsampler_x", command_upsampler_x, "Write upsampler_input_x"},
    {"upsampler_y", command_upsampler_y, "Write upsampler_input_y"},
    {"ds_pop", command_ds_pop, "Pop one downsampled sample from FIFO"},
    {"ds_status", command_ds_status, "Show downsample FIFO status"},
    {"ups_push", command_ups_push, "Push one sample into upsampler FIFO"},
    {"ups_status", command_ups_status, "Show upsampler FIFO status"},
    {"dsp_test", command_dsp_test, "Run DSP loop over FIFO samples"},
    {"fft_fs", command_fft_fs, "Set DS sample rate in Hz"},
    {"fft_ds", command_fft_ds, "Run FFT over DS FIFO IQ samples"},
    {"fft64_peak", command_fft64_peak, "64-point FFT over DS FIFO IQ samples, print peak only"},
    {"fft32_ds_y", command_fft32_ds_y, "Real FFT of 32 Y samples from DS FIFO"},
    {"track3", command_track3, "Sweep phase_down_1 until 3-tone pattern is found"},
    {"trackq_start", command_trackq_start, "Start 3-point quadratic tracking"},
    {"trackq_probe", command_trackq_probe, "Capture one 3-point tracking snapshot"},
    {"trackq_stop", command_trackq_stop, "Stop 3-point quadratic tracking"},
    {"gain", command_gain, "Set gain by channel"},
    {"final_shift", command_final_shift, "Set final shift"},
    {"cap_arm", command_cap_arm, "Pulse cap_arm"},
    {"cap_done", command_cap_done, "Read cap_done"},
    {"cap_rd", command_cap_rd, "Read cap_data at index"},
    {"cap_enable", command_cap_enable, "0=ramp, 1=capture design to DDR"},
    {"cap_beats", command_cap_beats, "Set capture length in 256-bit beats"},
    {"phase", command_phase, "Print current phase"},
    {"magnitude", command_magnitude, "Print current magnitude"},
    {"cap_start", command_cap_start, "Start low-speed capture"},
    {"cap_status", command_cap_status, "Low-speed capture status"},
    {"cap_dump", command_cap_dump, "Dump low-speed capture samples"},
    {"ub_help", command_dma_help, "UberDDR3/S2MM help"},
    {"ub_info", command_ub_info, "Show UBDDR3 info/state"},
    {"ub_mode", command_ub_mode, "Show current cap_enable mode"},
    {"ub_setmode", command_ub_setmode, "Set cap_enable"},
    {"ub_start", command_ub_start, "Start S2MM using current mode"},
    {"ub_ramp", command_ub_ramp, "Force ramp mode then start S2MM"},
    {"ub_cap", command_ub_cap, "Force capture mode then start S2MM"},
    {"ub_wait", command_ub_wait, "Wait until DMA done"},
    {"ub_hexdump", command_ub_hexdump, "Hexdump DDR memory"},
    {"ub_send", command_ub_send, "Send DDR memory region via UDP"},
    {"sig3_start", command_sig3_start, "Start 3-tone software generator"},
    {"sig3_stop", command_sig3_stop, "Stop 3-tone software generator"},
    {"sig3_amp", command_sig3_amp, "Set 3-tone per-tone amplitude"}
};

void uberclock_commands_register(void) {
    console_register(command_table, (unsigned)(sizeof(command_table) / sizeof(command_table[0])));
}
