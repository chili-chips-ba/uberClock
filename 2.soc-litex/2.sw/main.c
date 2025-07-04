#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>

#ifdef CSR_MAIN_DAC1_DATA_ADDR
#  define WITH_DAC
#endif

#ifdef CSR_MAIN_PHASE_INC_ADDR
#  define WITH_CORDIC_DAC
#endif

#ifdef CSR_MAIN_INPUT_SW_REG_ADDR
#  define WITH_INPUT_MUX
#endif

#if defined(CSR_MAIN_DOWNSAMPLED_ADDR) && defined(CSR_MAIN_UPSAMPLER_IN_ADDR)
#  define WITH_DSP
#endif

#ifdef CSR_MAIN_UPSAMPLER_GAIN_ADDR
#define WITH_UPSAMPLER_GAIN
#endif

#ifdef CSR_MAIN_MODE_SEL_ADDR
#  define WITH_UBERCLOCK
#endif

#if defined(CSR_MAIN_PHASE_INC_NCO_ADDR) && defined(CSR_MAIN_PHASE_INC_DOWN_ADDR)
#  define WITH_CORDIC_DSP_DAC
#endif

#ifdef CSR_MAIN_OUTPUT_SELECT_ADDR
#  define WITH_OUTPUT_SELECT
#endif

#ifdef CSR_MAIN_INPUT_SELECT_ADDR
#  define WITH_INPUT_SELECT
#endif

#ifdef CSR_MAIN_GAIN1_ADDR
#define WITH_GAIN1
#endif

#ifdef CSR_MAIN_GAIN2_ADDR
#define WITH_GAIN2
#endif

static volatile int dsp_loop_running = 0;

static char *readstr(void) {
	char c[2];
	static char s[64];
	static int ptr = 0;
	if (readchar_nonblock()) {
		c[0] = getchar();
		c[1] = '\0';
		switch (c[0]) {
			case 0x7f: case 0x08:
				if (ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07: break;
			case '\r': case '\n':
				s[ptr] = '\0';
				fputs("\n", stdout);
				ptr = 0;
				return s;
			default:
				if (ptr < (int)(sizeof(s) - 1)) {
					fputs(c, stdout);
					s[ptr++] = c[0];
				}
				break;
		}
	}
	return NULL;
}

static char *get_token(char **str) {
	char *p = strchr(*str, ' ');
	if (!p) {
		char *t = *str;
		*str += strlen(*str);
		return t;
	}
	*p = '\0';
	char *t = *str;
	*str = p + 1;
	return t;
}

static void prompt(void) {
	printf("\e[92;1muberClock\e[0m> ");
}

static void help(void) {
	puts("\nuberClock built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("  help                  - Show this command");
	puts("  reboot                - Reboot CPU");
	#ifdef WITH_DAC
	puts("  dac1 <value>          - Set DAC1 output (0–0x3FFF)");
	puts("  dac2 <value>          - Set DAC2 output (0–0x3FFF)");
	#endif
	#ifdef WITH_CORDIC_DAC
	puts("  phase <value>         - Set CORDIC_DAC phase (0..524287)");
	#endif
	#ifdef WITH_INPUT_MUX
	puts("  input_sw_reg <value>  - Set input_mux select to 1 or 0");
	#endif
	#ifdef WITH_DSP
	puts("  dsp                   - Single step: read downsampled, invert, send to upsampler");
	puts("  dsploop               - Start continuous DSP loop (20 kHz)");
	puts("  dspstop               - Stop DSP loop");
	#endif
	#ifdef WITH_UPSAMPLER_GAIN
	puts("  gain <val>            - Set upsampler gain (-128 to 127)");
	#endif
	#ifdef CSR_MAIN_DOWNSAMPLED_ADDR
	puts("  profile_adc           - Measure ADC CSR read speed");
	#endif
	#if defined(CSR_MAIN_DAC1_DATA_ADDR) && defined(CSR_MAIN_DAC1_WRT_EN_ADDR)
	puts("  profile_dac           - Measure DAC CSR write speed");
	#endif
	#ifdef WITH_UBERCLOCK
	puts("  mode <0–4>            - Select signal-path mode (see docs)");
	#endif
	#ifdef WITH_CORDIC_DSP_DAC
	puts("  phase_nco <val>      - Set input CORDIC NCO phase increment (0–524287)");
	puts("  phase_down <val>     - Set downconversion CORDIC phase increment (0–524287)");
	#endif
	#ifdef WITH_OUTPUT_SELECT
	puts("  output_select <val>  - Choose DAC1 output source:");
	puts("                           0 = downsampledY (after filters)");
	puts("                           1 = upsampledY (after interpolation)");
	puts("                           2 = yval_downconverted (CORDIC downconv)");
	puts("                           3 = yval_upconverted (CORDIC upconv)");
	#endif
	#ifdef WITH_INPUT_SELECT
	puts("  input_select <val>   - Set main input select register (0=ADC, 1=NCO)");
	#endif
	#ifdef WITH_GAIN1
	puts("  gain1 <val>           - Set Gain1 register (Q format value)");
	#endif
	#ifdef WITH_GAIN2
	puts("  gain2 <val>           - Set Gain2 register (Q format value)");
	#endif
}

static void reboot_cmd(void) {
	ctrl_reset_write(1);
}

#ifdef WITH_DAC
static void dac1_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0x3FFF;
	main_dac1_data_write(v);
	main_dac1_wrt_en_write(1);
	main_dac1_wrt_en_write(0);
	printf("DAC1 set to 0x%04X\n", v);
}

static void dac2_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0x3FFF;
	main_dac2_data_write(v);
	main_dac2_wrt_en_write(1);
	main_dac2_wrt_en_write(0);
	printf("DAC2 set to 0x%04X\n", v);
}
#endif

#ifdef WITH_CORDIC_DAC
static void phase_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 19)) {
		printf("Error: phase must be 0–524287\n");
		return;
	}
	main_phase_inc_write(p);
	printf("CORDIC_DAC phase changed to %u\n", p);
}
#endif

#ifdef WITH_INPUT_MUX
static void input_sw_reg_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0x1;
	main_input_sw_reg_write(v);
	printf("input_sw_reg set to %u\n", v);
}
#endif

#ifdef WITH_DSP
static void dsp_cmd(void) {
	uint16_t ds = main_downsampled_read();
	uint16_t inv = ~ds;
	main_upsampler_in_write(inv);
	printf("dsp: downsampled=0x%04X, upsampler_in=0x%04X\n", ds, inv);
}

static void dsp_loop_cmd(void) {
	if (dsp_loop_running) {
		printf("DSP loop already running!\n");
		return;
	}
	printf("Starting DSP loop... Press dspstop to interrupt.\n");
	dsp_loop_running = 1;
	while (dsp_loop_running) {
		uint16_t ds = main_downsampled_read();
		uint16_t inv = ~ds;
		main_upsampler_in_write(inv);
	}
	printf("DSP loop stopped.\n");
}

static void dsp_stop_cmd(void) {
	if (!dsp_loop_running) {
		printf("DSP loop not running.\n");
		return;
	}
	dsp_loop_running = 0;
}
#endif


#ifdef WITH_UBERCLOCK
static void mode_cmd(char *args) {
	    unsigned v = strtoul(args, NULL, 0) & 0x7;
	    if (v > 4) {
		        printf("Error: mode must be 0–4\n");
		        return;
		    }
		    main_mode_sel_write(v);
		    printf("uberClock mode set to %u\n", v);
		}
#endif


#ifdef WITH_CORDIC_DSP_DAC
static void phase_nco_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 19)) {
		printf("Error: phase_nco must be 0–524287\n");
		return;
	}
	main_phase_inc_nco_write(p);
	printf("Input NCO phase increment set to %u\n", p);
}

static void phase_down_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 19)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_write(p);
	printf("Downconversion phase increment set to %u\n", p);
}
#endif

#ifdef WITH_UPSAMPLER_GAIN
static void gain_cmd(char *args) {
	int gain = atoi(args);
	if (gain < -128 || gain > 127) {
		printf("Gain must be in -128 to 127 range.\n");
		return;
	}
	main_upsampler_gain_write((uint8_t)gain);
	printf("Upsampler gain set to %d\n", gain);
}
#endif

#ifdef CSR_MAIN_DOWNSAMPLED_ADDR
static void profile_adc(void) {
	const uint64_t N = 10000;
	uint16_t dummy;
	uint32_t start = timer0_value_read();
	for (int i = 0; i < N; ++i) {
		dummy = main_downsampled_read();
	}
	(void)dummy;
	uint32_t end = timer0_value_read();
	uint32_t cycles = start - end;

	// Use microseconds as integer
	uint32_t us_per_read = (1000000UL * cycles) / CONFIG_CLOCK_FREQUENCY / N;
	uint32_t reads_per_sec = ((uint64_t)CONFIG_CLOCK_FREQUENCY * N) / cycles;

	printf("ADC read rate: %lu reads/sec, %lu us/read\n",
		   (unsigned long)reads_per_sec,
		   (unsigned long)us_per_read);
}

#endif

#if defined(CSR_MAIN_DAC1_DATA_ADDR) && defined(CSR_MAIN_DAC1_WRT_EN_ADDR)
static void profile_dac(void) {
	const uint64_t N = 10000;
	uint32_t start = timer0_value_read();
	for (int i = 0; i < N; ++i) {
		main_dac1_data_write(i & 0x3FFF);
		main_dac1_wrt_en_write(1);
		main_dac1_wrt_en_write(0);
	}
	uint32_t end = timer0_value_read();
	uint32_t cycles = start - end;

	uint32_t us_per_write = (1000000UL * cycles) / CONFIG_CLOCK_FREQUENCY / N;
	uint32_t writes_per_sec = (CONFIG_CLOCK_FREQUENCY * N) / cycles;

	printf("DAC write rate: %lu writes/sec, %lu us/write\n",
		   (unsigned long)writes_per_sec,
		   (unsigned long)us_per_write);
}
#endif

#ifdef WITH_OUTPUT_SELECT
static void output_select_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0);
	main_output_select_write(v);
	printf("Main output select register set to %u\n", v);
}
#endif

#ifdef WITH_INPUT_SELECT
static void input_select_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0);
	main_input_select_write(v);
	printf("Main input select register set to %u\n", v);
}
#endif

#ifdef WITH_GAIN1
static void gain1_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain1_write((uint32_t)gain);
	printf("Gain1 register set to %d (0x%08X)\n", gain, (uint32_t)gain);
}
#endif

#ifdef WITH_GAIN2
static void gain2_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain2_write((uint32_t)gain);
	printf("Gain2 register set to %d (0x%08X)\n", gain, (uint32_t)gain);
}
#endif

static void console_service(void) {
	char *line = readstr();
	if (!line) return;

	char *token = get_token(&line);
	if      (!strcmp(token, "help"))    help();
	else if (!strcmp(token, "reboot"))  reboot_cmd();
	#ifdef WITH_DAC
	else if (!strcmp(token, "dac1")) {
		char *arg = get_token(&line);
		dac1_cmd(arg);
	} else if (!strcmp(token, "dac2")) {
		char *arg = get_token(&line);
		dac2_cmd(arg);
	}
	#endif
	#ifdef WITH_CORDIC_DAC
	else if (!strcmp(token, "phase")) {
		char *arg = get_token(&line);
		phase_cmd(arg);
	}
	#endif
	#ifdef WITH_INPUT_MUX
	else if (!strcmp(token, "input_sw_reg")) {
		char *arg = get_token(&line);
		input_sw_reg_cmd(arg);
	}
	#endif
	#ifdef WITH_DSP
	else if (!strcmp(token, "dsp"))      dsp_cmd();
	else if (!strcmp(token, "dsploop"))  dsp_loop_cmd();
	else if (!strcmp(token, "dspstop"))  dsp_stop_cmd();
	#endif
	#ifdef CSR_MAIN_DOWNSAMPLED_ADDR
	else if (!strcmp(token, "profile_adc")) profile_adc();
	#endif

	#if defined(CSR_MAIN_DAC1_DATA_ADDR) && defined(CSR_MAIN_DAC1_WRT_EN_ADDR)
	else if (!strcmp(token, "profile_dac")) profile_dac();
	#endif

	#ifdef WITH_UBERCLOCK
	else if (!strcmp(token, "mode")) {
		char *arg = get_token(&line);
		mode_cmd(arg);
	}
	#endif

	#ifdef WITH_UPSAMPLER_GAIN
	else if (!strcmp(token, "gain")) {
		char *arg = get_token(&line);
		gain_cmd(arg);
	}
	#endif
	#ifdef WITH_CORDIC_DSP_DAC
	else if (!strcmp(token, "phase_nco")) {
		char *arg = get_token(&line);
		phase_nco_cmd(arg);
	}
	else if (!strcmp(token, "phase_down")) {
		char *arg = get_token(&line);
		phase_down_cmd(arg);
	}
	#endif
	#ifdef WITH_OUTPUT_SELECT
	else if (!strcmp(token, "output_select")) {
		char *arg = get_token(&line);
		output_select_cmd(arg);
	}
	#endif
	#ifdef WITH_INPUT_SELECT
	else if (!strcmp(token, "input_select")) {
		char *arg = get_token(&line);
		input_select_cmd(arg);
	}
	#endif
	#ifdef WITH_GAIN1
	else if (!strcmp(token, "gain1")) {
		char *arg = get_token(&line);
		gain1_cmd(arg);
	}
	#endif
	#ifdef WITH_GAIN2
	else if (!strcmp(token, "gain2")) {
		char *arg = get_token(&line);
		gain2_cmd(arg);
	}
	#endif
	else {
		printf("Unknown command: %s\n", token);
	}

	prompt();
}

int main(void) {
	#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
	#endif
	uart_init();
	help();
	prompt();
	while (1) {
		console_service();
	}
	return 0;
}
