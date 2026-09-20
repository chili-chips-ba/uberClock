#ifndef UBERCLOCK_H
#define UBERCLOCK_H

#ifdef __cplusplus
extern "C" {
    #endif

    void uberclock_init(void);
    void uberclock_poll(void);

    /* Command handlers, called by the master table in cmd_list.c (not this
     * file) - the actual command registry now lives entirely in cmd_list.c
     * so it isn't buried alongside uberclock.c's DSP/hardware logic. */
    void cap_dump_cmd(char *args);
    void cap_start_cmd(char *args);
    void cap_status_cmd(char *args);
    void cmd_cap_arm_pulse(char *args);
    void cmd_cap_beats(char *args);
    void cmd_cap_done(char *args);
    void cmd_cap_enable(char *args);
    void cmd_cap_rd(char *args);
    void cmd_ds_pop(char *args);
    void cmd_dsp_run(char *args);
    void cmd_dsp_test(char *args);
    void cmd_ds_status(char *args);
    void cmd_fft32_ds_y(char *args);
    void cmd_fft64_peak(char *args);
    void cmd_fft_ds(char *args);
    void cmd_fft_ds_peak(char *args);
    void cmd_fft_fs(char *args);
    void cmd_final_shift(char *args);
    void cmd_gain1(char *args);
    void cmd_gain2(char *args);
    void cmd_gain3(char *args);
    void cmd_gain4(char *args);
    void cmd_gain5(char *args);
    void cmd_highspeed_dbg_select(char *args);
    void cmd_input_select(char *args);
    void cmd_lowspeed_dbg_select(char *args);
    void cmd_mag_cpu1(char *args);
    void cmd_mag_cpu2(char *args);
    void cmd_mag_cpu3(char *args);
    void cmd_mag_cpu4(char *args);
    void cmd_mag_cpu5(char *args);
    void cmd_magnitude(char *args);
    void cmd_nco_mag(char *args);
    void cmd_output_sel_ch1(char *args);
    void cmd_output_sel_ch2(char *args);
    void cmd_phase_cpu1(char *args);
    void cmd_phase_cpu2(char *args);
    void cmd_phase_cpu3(char *args);
    void cmd_phase_cpu4(char *args);
    void cmd_phase_cpu5(char *args);
    void cmd_phase_down_1(char *args);
    void cmd_phase_down_2(char *args);
    void cmd_phase_down_3(char *args);
    void cmd_phase_down_4(char *args);
    void cmd_phase_down_5(char *args);
    void cmd_phase_down_ref(char *args);
    void cmd_phase_nco(char *args);
    void cmd_phase_print(char *args);
    void cmd_sig3_amp(char *args);
    void cmd_sig3_disable_ch(char *args);
    void cmd_sig3_enable_ch(char *args);
    void cmd_sig3_freqs(char *args);
    void cmd_sig3_start(char *args);
    void cmd_sig3_stop(char *args);
    void cmd_track3(char *args);
    void cmd_trackq_probe(char *args);
    void cmd_trackq_start(char *args);
    void cmd_trackq_stop(char *args);
    void cmd_ub_cap(char *args);
    void cmd_ub_hexdump(char *args);
    void cmd_ub_info(char *args);
    void cmd_ub_mode(char *args);
    void cmd_ub_ramp2(char *args);
    void cmd_ub_send(char *args);
    void cmd_ub_setmode(char *args);
    void cmd_ub_start(char *args);
    void cmd_ub_wait(char *args);
    void cmd_upsampler_x(char *args);
    void cmd_upsampler_y(char *args);
    void cmd_ups_in_mux(char *args);
    void cmd_ups_push(char *args);
    void cmd_ups_status(char *args);
    void ub_help(char *args);
    void uc_help(char *args);

    #ifdef __cplusplus
}
#endif

#endif
