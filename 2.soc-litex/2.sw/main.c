#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <generated/soc.h>

#define SYS_HZ    65000000UL
#define TICK_HZ   10000UL
#define RELOAD    (SYS_HZ / TICK_HZ)   // 6500

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
	puts("  help                      - Show this command");
	puts("  reboot                    - Reboot CPU");
	puts("  phase_nco  <val>          - Set input CORDIC NCO phase increment (0–524287)");
	puts("  phase_down_1 <val>          - Set downconversion CORDIC phase increment (0–524287) ch 1");
	puts("  phase_down_2 <val>          - Set downconversion CORDIC phase increment (0–524287) ch 2");
	puts("  phase_down_3 <val>          - Set downconversion CORDIC phase increment (0–524287) ch 3");
	puts("  phase_down_4 <val>          - Set downconversion CORDIC phase increment (0–524287) ch 4");
	puts("  phase_down_5 <val>          - Set downconversion CORDIC phase increment (0–524287) ch 5");
	puts("  phase_cpu  <val>          - Set CORDIC CPU phase increment (0–524287)");
	puts("  output_select_ch1 <val>   - Choose DAC1 (channel 1) output source:");
	puts("                                 0 = downsampled_y");
	puts("                                 1 = x_cpu_nco");
	puts("                                 2 = y_downconverted");
	puts("                                 3 = y_upconverted");
	puts("  output_select_ch2 <val>   - Choose DAC2 (channel 2) output source:");
	puts("                                 0 = upsampled_y");
	puts("                                 1 = filter_in");
	puts("                                 2 = nco_cos");
	puts("                                 3 = upsampled_input_y(from CPU)");
	puts("  input_select <val>        - Set input select register");
	puts("                                 0 = ADC");
	puts("                                 1 = NCO");
	puts("                                 2 = CPU");
	puts("  upsampler_input_mux <val> - Set upsampler input register register");
	puts("                                 0 = Gain");
	puts("                                 1 = CPU");
	puts("                                 2 = CPU NCO");
	puts("  gain1 <val>               - Set gain1 register");
	puts("  gain2 <val>               - Set gain2 register");
	puts("  gain3 <val>               - Set gain3 register");
	puts("  gain4 <val>               - Set gain4 register");
	puts("  gain5 <val>               - Set gain5 register");
	puts("  phase                     - Print current CORDIC phase");
	puts("  magnitude                 - Print current CORDIC magnitude");
}

static void reboot_cmd(void) {
	ctrl_reset_write(1);
}

static void phase_nco_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_nco must be 0–524287\n");
		return;
	}
	main_phase_inc_nco_write(p);
	printf("Input NCO phase increment set to %u\n", p);
}

static void nco_mag_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 12)) {
		printf("Error: phase_nco must be 0–524287\n");
		return;
	}
	main_nco_mag_write(p);
	printf("Input NCO mag set to %u\n", p);
}
static void phase_down_1_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_1_write(p);
	printf("Downconversion phase ch1 increment set to %u\n", p);
}

static void phase_down_2_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_2_write(p);
	printf("Downconversion phase ch2 increment set to %u\n", p);
}

static void phase_down_3_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_3_write(p);
	printf("Downconversion phase ch3 increment set to %u\n", p);
}


static void phase_down_4_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_4_write(p);
	printf("Downconversion phase ch4 increment set to %u\n", p);
}

static void phase_down_5_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_down must be 0–524287\n");
		return;
	}
	main_phase_inc_down_5_write(p);
	printf("Downconversion phase ch5 increment set to %u\n", p);
}
static void phase_cpu_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 24)) {
		printf("Error: phase_cpu must be 0–524287\n");
		return;
	}
	main_phase_inc_cpu_write(p);
	printf("CPU phase increment set to %u\n", p);
}

static void output_select_ch1_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0xf;
	main_output_select_ch1_write(v);
	printf("output_select_ch1 set to %u\n", v);
}

static void output_select_ch2_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0xf;
	main_output_select_ch2_write(v);
	printf("output_select_ch2 set to %u\n", v);
}

static void input_select_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0);
	main_input_select_write(v);
	printf("Main input select register set to %u\n", v);
}

static void upsampler_input_mux_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0);
	main_upsampler_input_mux_write(v);
	printf("Upsampler input mux register set to %u\n", v);
}

static void gain1_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain1_write((uint32_t)gain);
	printf("Gain1 register set to %ld (0x%08lX)\n", (long)gain, (unsigned long)gain);
}

static void gain2_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain2_write((uint32_t)gain);
	printf("Gain2 register set to %ld (0x%08lX)\n", (long)gain, (unsigned long)gain);
}

static void gain3_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain3_write((uint32_t)gain);
	printf("Gain3 register set to %ld (0x%08lX)\n", (long)gain, (unsigned long)gain);
}

static void gain4_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain4_write((uint32_t)gain);
	printf("Gain4 register set to %ld (0x%08lX)\n", (long)gain, (unsigned long)gain);
}

static void gain5_cmd(char *args) {
	int32_t gain = strtol(args, NULL, 0);
	main_gain5_write((uint32_t)gain);
	printf("Gain5 register set to %ld (0x%08lX)\n", (long)gain, (unsigned long)gain);
}

static void final_shift_cmd(char *args) {
	int32_t fs= strtol(args, NULL, 0);
	main_final_shift_write((uint32_t)fs);
	printf("S register set to %ld (0x%08lX)\n", (long)fs, (unsigned long)fs);
}
int16_t mag;
int32_t phase_val;

static void print_phase(void) {
	printf("Phase %ld \n", (long)phase_val);
}
static void magnitude(void) {
	printf("Magnitude %d \n", mag);
}
static void console_service(void) {
	char *line = readstr();
	if (!line) return;

	char *token = get_token(&line);
	if      (!strcmp(token, "help"))    help();
	else if (!strcmp(token, "reboot"))  reboot_cmd();
	else if (!strcmp(token, "phase_nco")) {
		char *arg = get_token(&line);
		phase_nco_cmd(arg);
	}

	else if (!strcmp(token, "nco_mag")) {
		char *arg = get_token(&line);
		nco_mag_cmd(arg);
	}
	else if (!strcmp(token, "phase_down_1")) {
		char *arg = get_token(&line);
		phase_down_1_cmd(arg);
	}
	else if (!strcmp(token, "phase_down_2")) {
		char *arg = get_token(&line);
		phase_down_2_cmd(arg);
	}
	else if (!strcmp(token, "phase_down_3")) {
		char *arg = get_token(&line);
		phase_down_3_cmd(arg);
	}
	else if (!strcmp(token, "phase_down_4")) {
		char *arg = get_token(&line);
		phase_down_4_cmd(arg);
	}
	else if (!strcmp(token, "phase_down_5")) {
		char *arg = get_token(&line);
		phase_down_5_cmd(arg);
	}
	else if (!strcmp(token, "phase_cpu")) {
		char *arg = get_token(&line);
		phase_cpu_cmd(arg);
	}
	else if (!strcmp(token, "output_select_ch1")) {
		char *arg = get_token(&line);
		output_select_ch1_cmd(arg);
	}
	else if (!strcmp(token, "output_select_ch2")) {
		char *arg = get_token(&line);
		output_select_ch2_cmd(arg);
	}
	else if (!strcmp(token, "input_select")) {
		char *arg = get_token(&line);
		input_select_cmd(arg);
	}
	else if (!strcmp(token, "upsampler_input_mux")) {
		char *arg = get_token(&line);
		upsampler_input_mux_cmd(arg);
	}
	else if (!strcmp(token, "gain1")) {
		char *arg = get_token(&line);
		gain1_cmd(arg);
	}
	else if (!strcmp(token, "gain2")) {
		char *arg = get_token(&line);
		gain2_cmd(arg);
	}
	else if (!strcmp(token, "gain3")) {
		char *arg = get_token(&line);
		gain3_cmd(arg);
	}
	else if (!strcmp(token, "gain4")) {
		char *arg = get_token(&line);
		gain4_cmd(arg);
	}
	else if (!strcmp(token, "gain5")) {
		char *arg = get_token(&line);
		gain5_cmd(arg);
	}
	else if (!strcmp(token, "phase")) {
		print_phase();
	}
	else if (!strcmp(token, "magnitude")) {
		magnitude();
	}
	else if (!strcmp(token, "final_shift")) {
		char *arg = get_token(&line);
		final_shift_cmd(arg);
	}
	else {
		printf("Unknown command: %s\n", token);
	}

	prompt();
}

static volatile bool ce_event = false;
static void ce_down_isr(void) {
	evm_pending_write(1);
	evm_enable_write(0);
	ce_event = true;
}

int main(void) {

	main_phase_inc_nco_write(2581836);	
	main_nco_mag_write(22); // NCO magnitude
	main_phase_inc_down_1_write(2581110);  //10MHz
	main_phase_inc_down_3_write(80648); 
	main_phase_inc_down_4_write(80644); 
	main_phase_inc_down_5_write(80640); 
	main_phase_inc_cpu_write(52429);
	main_input_select_write(0);
	main_upsampler_input_mux_write(0);
	main_gain1_write (0x40000000);
	main_gain2_write (0x40000000);
	main_gain3_write (0x40000000);
	main_gain4_write (0x40000000);
	main_gain5_write (0x40000000);
	main_output_select_ch1_write(10);
	main_output_select_ch2_write(11);
	main_final_shift_write(0);
	uart_init();

	evm_pending_write(1);
	evm_enable_write(1);

	irq_setie(0);
	irq_attach(EVM_INTERRUPT, ce_down_isr);
	irq_setmask( irq_getmask() | (1 << EVM_INTERRUPT) );
	irq_setie(1);

	help();
	prompt();


	evm_pending_write(1);
	evm_enable_write(1);


	irq_attach(EVM_INTERRUPT, ce_down_isr);
	uint32_t m = irq_getmask();
	irq_setmask(m | (1 << EVM_INTERRUPT));

	while (1) {
		console_service();

		if (ce_event) {
			// now that we've got the flag, process it here:
			uint16_t ds_x = main_downsampled_data_x_read();
			uint32_t doubled_x = (uint32_t)ds_x * 2;
			main_upsampler_input_x_write(doubled_x);
			uint16_t ds_y = main_downsampled_data_y_read();
			uint32_t doubled_y = (uint32_t)ds_y * 2;
			main_upsampler_input_y_write(doubled_y);

			mag = main_magnitude_read();
			phase_val = main_phase_read();
			ce_event = false;
			// re-arm the next CE_DOWN
			evm_pending_write(1);
			evm_enable_write(1);
		}
	}
}

