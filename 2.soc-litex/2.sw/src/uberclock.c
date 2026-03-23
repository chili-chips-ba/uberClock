#include "uberclock_core.h"

volatile int ce_event = 0;
int16_t g_mag = 0;
int32_t g_phase = 0;
volatile int dsp_pump_enable = 0;
volatile uint32_t dsp_work_tokens = 0;

int16_t dsp_swq_x[DSP_SWQ_LEN];
int16_t dsp_swq_y[DSP_SWQ_LEN];
unsigned dsp_swq_r = 0;
unsigned dsp_swq_w = 0;
unsigned dsp_swq_count = 0;

static void ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    if (dsp_pump_enable) {
        if (dsp_work_tokens < 1024u) {
            dsp_work_tokens++;
        }
    }
    ce_event = 1;
}

void uberclock_register_cmds(void) {
    console_register(uberclock_cmds, uberclock_cmd_count);
    console_register(uberclock_dsp_cmds, uberclock_dsp_cmd_count);
    console_register(uberclock_capture_cmds, uberclock_capture_cmd_count);
    console_register(uberclock_udp_cmds, uberclock_udp_cmd_count);
}

void uberclock_init(void) {
    main_phase_inc_nco_write(10324440);

    main_phase_inc_down_1_write(10325473);
    main_phase_inc_down_2_write(80652);
    main_phase_inc_down_3_write(80648);
    main_phase_inc_down_4_write(80644);
    main_phase_inc_down_5_write(80640);

    main_phase_inc_down_ref_write(2581110);

    main_nco_mag_write((uint32_t)(500 & 0x0fff));

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

    main_input_select_write(1);
    main_upsampler_input_mux_write(0);

    main_gain1_write(0x40000000);
    main_gain2_write(0x40000000);
    main_gain3_write(0x40000000);
    main_gain4_write(0x40000000);
    main_gain5_write(0x40000000);

    main_output_select_ch1_write(0);
    main_output_select_ch2_write(5);

    main_final_shift_write(2);

    main_lowspeed_dbg_select_write(0);
    main_highspeed_dbg_select_write(0);

    main_upsampler_input_x_write(0);
    main_upsampler_input_y_write(0);

    main_upsampler_input_mux_write(1);
    main_cap_enable_write(1);

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
    if (dsp_pump_enable) {
        unsigned budget = 32u;
        while (budget && dsp_work_tokens) {
            (void)dsp_pump_step(1, 1);
            dsp_work_tokens--;
            budget--;
        }
    }

    if (!ce_event) {
        return;
    }

    ce_event = 0;
    evm_pending_write(1);
    evm_enable_write(1);
}
