#include <stdio.h>
#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include "uberclock/uberclock.h"
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_types.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_channels.h"
#include "uberclock/uberclock_siggen.h"
#include "uberclock/uberclock_track.h"
#include "uberclock/uberclock_commands.h"

static struct uberclock_app_context application_context = {
    .runtime = {0u, 0u, 0, 0},
    .fft = {{{0}}, {{0}}, {0}, 10000u},
    .track = {
        0,
        UBERCLOCK_TRACK_DEFAULT_N,
        UBERCLOCK_TRACK_DEFAULT_SETTLE,
        UBERCLOCK_TRACK_DEFAULT_CENTER_HZ,
        UBERCLOCK_TRACK_DEFAULT_DELTA_HZ,
        0u,
        0,
        0
    },
    .siggen = {0, 0u, 0u, 0u, 0u, 0u, 0u, 3000}
};

static void service_ce_event(void) {
    uberclock_siggen_service_push();
    application_context.runtime.magnitude = (int16_t)(main_magnitude_read() & 0xffffu);
    evm_pending_write(1);
    evm_enable_write(1);
}

static void ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    if (application_context.runtime.ce_event < 0xffffffffu) {
        application_context.runtime.ce_event++;
    }
    application_context.runtime.ce_ticks++;
}

struct uberclock_app_context *uberclock_app_context(void) {
    return &application_context;
}

struct uberclock_runtime *uberclock_runtime_state(void) {
    return &application_context.runtime;
}

struct uberclock_fft_context *uberclock_fft_context(void) {
    return &application_context.fft;
}

struct uberclock_track_state *uberclock_track_state(void) {
    return &application_context.track;
}

struct uberclock_siggen_state *uberclock_siggen_state(void) {
    return &application_context.siggen;
}

void uberclock_runtime_record_ce_event(void) {
    if (application_context.runtime.ce_event < 0xffffffffu) {
        application_context.runtime.ce_event++;
    }
}

void uberclock_runtime_service_ce_event(void) {
    if (application_context.runtime.ce_event == 0u) {
        return;
    }
    application_context.runtime.ce_event--;
    service_ce_event();
}

void uberclock_runtime_service_ce_events(unsigned budget) {
    while (budget-- > 0u && application_context.runtime.ce_event != 0u) {
        application_context.runtime.ce_event--;
        service_ce_event();
    }
}

void uberclock_runtime_wait_ticks(uint32_t wait_ticks) {
    uint32_t start_tick = application_context.runtime.ce_ticks;
    while ((uint32_t)(application_context.runtime.ce_ticks - start_tick) < wait_ticks) {
        uberclock_runtime_service_ce_event();
    }
}

void uberclock_cache_sync(void) {
    flush_cpu_dcache();
    flush_l2_cache();
}

void uberclock_register_commands(void) {
    uberclock_commands_register();
}

void uberclock_init(void) {
    uberclock_set_nco_phase_increment(10324440u);
    uberclock_set_phase_down_reference(2581110u);
    uberclock_set_nco_magnitude(500);

    uberclock_channels_apply_default_init();
    uberclock_set_input_select(0u);
    uberclock_set_upsampler_input_mux(1u);
    uberclock_set_output_select_ch1(5u);
    uberclock_set_output_select_ch2(0u);
    uberclock_set_final_shift(0);
    uberclock_set_lowspeed_debug_select(5u);
    uberclock_set_highspeed_debug_select(0u);
    uberclock_set_upsampler_input_x(0);
    uberclock_set_upsampler_input_y(0);
    uberclock_set_capture_enable(1u);
    uberclock_siggen_start();
    uberclock_commit_config();

    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);

    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));

    puts("UberClock init done.");
}

void uberclock_poll(void) {
    uberclock_track_poll();
    while (application_context.runtime.ce_event != 0u) {
        application_context.runtime.ce_event--;
        service_ce_event();
    }
}
