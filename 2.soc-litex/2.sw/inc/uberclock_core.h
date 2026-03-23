#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include <generated/soc.h>

#include "console.h"
#include "kiss_fft.h"
#include "libliteeth/udp.h"
#include "uberclock.h"

extern volatile int ce_event;
extern int16_t g_mag;
extern int32_t g_phase;
extern volatile int dsp_pump_enable;
extern volatile uint32_t dsp_work_tokens;

#define DSP_SWQ_LEN 256u

extern int16_t dsp_swq_x[DSP_SWQ_LEN];
extern int16_t dsp_swq_y[DSP_SWQ_LEN];
extern unsigned dsp_swq_r;
extern unsigned dsp_swq_w;
extern unsigned dsp_swq_count;


unsigned parse_u(const char *s, unsigned max, const char *what);
int parse_s(const char *s, int minv, int maxv, const char *what);
int is_pow2_u(unsigned x);
void uc_commit(void);
void fifo_clear_flags(void);
unsigned dsp_pump_step(unsigned max_in, unsigned max_out);

extern const struct cmd_entry uberclock_cmds[];
extern const unsigned uberclock_cmd_count;

extern const struct cmd_entry uberclock_dsp_cmds[];
extern const unsigned uberclock_dsp_cmd_count;

extern const struct cmd_entry uberclock_capture_cmds[];
extern const unsigned uberclock_capture_cmd_count;

extern const struct cmd_entry uberclock_udp_cmds[];
extern const unsigned uberclock_udp_cmd_count;



