// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

#ifndef TRACKQ_H
#define TRACKQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS_FIFO_HW_DEPTH           16384u

#define TRACKQ_CHANNELS 3
#define TRACKQ_CH1_DELTA_HZ 10u
#define TRACKQ_CH2_DELTA_HZ 30u
#define TRACKQ_CH3_DELTA_HZ 30u
#define TRACKQ_CH1_START_HZ 10002950u
#define TRACKQ_CH2_START_HZ 3386370u
#define TRACKQ_CH3_START_HZ 3727990u

#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 12288u

#define TRACK3_RF_FS_HZ            65000000u
#define TRACK3_DEFAULT_STEP_HZ     5u
#define TRACK3_DEFAULT_MAX_STEPS   400u
#define TRACK3_DEFAULT_N           2048u
#define TRACK3_DEFAULT_SETTLE      256u
#define TRACK3_DEFAULT_CENTER_HZ   1000u
#define TRACK3_DEFAULT_DELTA_HZ    10u
#define TRACKQ_REF_INPUT_HZ        10000000u
#define TRACKQ_NCO_TARGET_HZ       10000000u
/* Nominal mode frequencies and temperature-sensitivity coefficients for the
 * three tracked crystal modes. These come from the BVD/temperature-chamber
 * characterization work under 5.characterization/3.model (see bvd_extractor.py
 * for the BVD-parameter extraction step), but there is currently no committed
 * script or log tying these exact six constants to a specific characterization
 * run -- the transfer from that work to these #defines is manual. If the
 * crystal, board, or characterization sweep changes, re-derive these by hand
 * and update this comment with the run/dataset used. */
#define TRACKQ_TEMP_NOM_CH1_MHZ    10004000000ll
#define TRACKQ_TEMP_NOM_CH2_MHZ     6269781000ll
#define TRACKQ_TEMP_NOM_CH3_MHZ     3388594000ll
#define TRACKQ_TEMP_C1_NC_PER_MHZ    -278440ll
#define TRACKQ_TEMP_C2_NC_PER_MHZ      -4614ll
#define TRACKQ_TEMP_C3_NC_PER_MHZ    -247762ll
#define TRACK3_DEFAULT_BAND_BINS   1u
#define TRACKQ_INTERVAL_TICKS      10000u
#define TRACKQ_CORR_SHIFT          10u
#define TRACKQ_MAX_STEP_HZ         2
#define TRACKQ_ERR_ALPHA_NUM       1
#define TRACKQ_ERR_ALPHA_DEN       4
#define TRACKQ_KP_NUM              1
#define TRACKQ_KP_DEN              4
/* Master-NCO (referent-clock) correction: same IIR + clamp shape as the
 * per-channel loop above, plus an outlier gate. A single bad FFT peak pick
 * on the reference channel must not be allowed to slew the output clock. */
#define TRACKQ_REF_ERR_ALPHA_NUM   1
#define TRACKQ_REF_ERR_ALPHA_DEN   4
#define TRACKQ_REF_MAX_STEP_HZ     2
#define TRACKQ_REF_MAX_DEVIATION_HZ 130000ll /* +-2000 ppm of 65 MHz: beyond any real crystal excursion, so reject as a bad peak pick */
#define TRACKQ_MIN_CONF_PCT        5u
#define TRACKQ_WEAK_DEADBAND_PCT   10u
#define TRACKQ_WEAK_GAIN_DEN       4
#define TRACKQ_WEAK_MAX_ERR_MHZ    750
#define TRACKQ_SERVICE_STRIDE      16u
#define TRACK3_FIFO_WAIT_POLLS     1000000u
#define TRACK3_SIDE_MIN_PCT        2u
#define TRACK3_SIDE_MAX_PCT        95u
#define TRACK3_SIDE_BALANCE_PCT    40u

#define FIFO_HEALTH_INTERVAL_TICKS 1000u /* ~100 ms at the 10 kHz ce_down rate */

/* trackq_step()/health monitor, called from uberclock_poll(). */
void trackq_step(void);
void trackq_fifo_health_poll(void);

/* Console commands, referenced by uberclock.c's command table. */
void cmd_track3(char *args);
void cmd_trackq_start(char *args);
void cmd_trackq_probe(char *args);
void cmd_trackq_stop(char *args);
void cmd_fft64_peak(char *args);
void cmd_fft_ds(char *args);
void cmd_fft_ds_peak(char *args);
void cmd_fft_fs(char *a);
void cmd_fifo_health(char *a);
void cmd_fft32_ds_y(char *args);

#ifdef __cplusplus
}
#endif

#endif
