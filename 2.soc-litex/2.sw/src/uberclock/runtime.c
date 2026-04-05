#include <stdio.h>

#include <irq.h>
#include <generated/csr.h>

#include "uberclock/uberclock_internal.h"

enum fsm_states {IDLE, S1, S2};

static volatile int ce_event = 0;
static volatile uint32_t ce_ticks = 0;
static int16_t mag;
static int32_t phase;

static char curr_state;
static uint32_t max_mag;
static uint32_t max_mag_phase_inc;
static uint32_t shooting_phase_inc;
static int8_t sgn = 1;

static void prv_ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    ce_event = 1;
    ce_ticks++;
}

static void prv_fsm_init(void) {
    curr_state = IDLE;
    ce_ticks = 0;
    max_mag = 0;
    max_mag_phase_inc = 0;
    shooting_phase_inc = 10328467;
}

static void prv_fsm_step(void) {
    switch (curr_state) {
    case IDLE:
        if (ce_ticks == 9999) {
            curr_state = S1;
        } else if (ce_ticks == 1) {
            main_phase_inc_nco_write(shooting_phase_inc);
            uberclock_int_write_phase_down(1, shooting_phase_inc + 1000);
            puts("Input NCO phase increment set");
        }
        break;
    case S1:
        if (mag < 30) {
            curr_state = IDLE;
            ce_ticks = 0;
            shooting_phase_inc += 6;
        } else if ((uint32_t)mag + 10 > max_mag) {
            puts("mag greater");
            max_mag = mag;
            max_mag_phase_inc = shooting_phase_inc;
            shooting_phase_inc = shooting_phase_inc + sgn * 6;
            curr_state = IDLE;
            ce_ticks = 0;
        } else {
            main_phase_inc_nco_write(shooting_phase_inc - 6);
            ce_ticks = 0;
            curr_state = S2;
        }
        break;
    case S2:
        puts("S2");
        break;
    default:
        break;
    }
}

void uberclock_runtime_init(void) {
    unsigned ch;

    main_phase_inc_nco_write(10324440);
    uberclock_int_write_phase_down(1, 10327476);
    uberclock_int_write_phase_down(2, 80652);
    uberclock_int_write_phase_down(3, 80648);
    uberclock_int_write_phase_down(4, 80644);
    uberclock_int_write_phase_down(5, 80640);
    main_phase_inc_down_ref_write(2581110);
    uberclock_int_write_nco_mag(500);

    for (ch = 1; ch <= UBERCLOCK_CHANNELS; ch++) {
        uberclock_int_write_phase_cpu(ch, 52429);
        uberclock_int_write_mag_cpu(ch, 0);
    }

    uberclock_int_set_input_select(0);
    uberclock_int_set_upsampler_input_mux(1);
    uberclock_int_write_gain(1, 0x40000000);
    uberclock_int_write_gain(2, 0x00000000);
    uberclock_int_write_gain(3, 0x00000000);
    uberclock_int_write_gain(4, 0x00000000);
    uberclock_int_write_gain(5, 0x00000000);
    uberclock_int_write_output_select(1, 5);
    uberclock_int_write_output_select(2, 0);
    uberclock_int_set_final_shift(0);
    uberclock_int_set_lowspeed_dbg_select(5);
    uberclock_int_set_highspeed_dbg_select(0);
    uberclock_int_write_upsampler_inputs_all_x(0);
    uberclock_int_write_upsampler_inputs_all_y(0);
    uberclock_int_set_upsampler_input_mux(1);
    uberclock_int_set_cap_enable(1);
    prv_fsm_init();
    uberclock_siggen_start();
    uberclock_int_commit();
    uberclock_int_fifo_clear_flags();

    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, prv_ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));

    printf("UberClock init done.\n");
}

void uberclock_runtime_poll(void) {
    if (!ce_event) {
        return;
    }

    uberclock_siggen_push_one();
    mag = (int16_t)(main_magnitude_read() & 0xffff);
#ifdef CSR_MAIN_PHASE_ADDR
    phase = (int32_t)(main_phase_read() & 0x1ffffff);
#else
    phase = 0;
#endif
    ce_event = 0;
    evm_pending_write(1);
    evm_enable_write(1);
    (void)max_mag_phase_inc;
    (void)sgn;
    (void)prv_fsm_step;
}

int16_t uberclock_runtime_get_magnitude(void) {
    return mag;
}

int32_t uberclock_runtime_get_phase(void) {
    return phase;
}

uint32_t uberclock_runtime_get_ce_ticks(void) {
    return ce_ticks;
}
