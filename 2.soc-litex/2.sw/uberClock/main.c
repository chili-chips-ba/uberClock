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

/*-----------------------------------------------------------------------*/
/* UART input helper                                                    */
/*-----------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------*/
/* Prompt & Help                                                        */
/*-----------------------------------------------------------------------*/
static void prompt(void) {
	printf("\e[92;1muberClock\e[0m> ");
}

static void help(void) {
	puts("\nuberClock built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("  help                  - Show this command");
	puts("  reboot                - Reboot CPU");
	#ifdef CSR_LEDS_BASE
	puts("  led                   - LED demo");
	#endif
	puts("  donut                 - Spinning Donut demo");
	puts("  helloc                - Hello C");
	#ifdef WITH_CXX
	puts("  hellocpp              - Hello C++");
	#endif
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
}

/*-----------------------------------------------------------------------*/
/* Commands                                                              */
/*-----------------------------------------------------------------------*/
static void reboot_cmd(void) {
	ctrl_reset_write(1);
}

#ifdef CSR_LEDS_BASE
static void led_cmd(void) {
	printf("LED demo...\n");
	for (int i = 0; i < 32; i++) {
		leds_out_write(i);
		busy_wait(100);
	}
	for (int i = 0; i < 4; i++) {
		leds_out_write(1 << i);
		busy_wait(200);
	}
	for (int i = 0; i < 4; i++) {
		leds_out_write(1 << (3 - i));
		busy_wait(200);
	}
	for (int i = 0; i < 4; i++) {
		leds_out_write(0x55);
		busy_wait(200);
		leds_out_write(0xAA);
		busy_wait(200);
	}
}
#endif

extern void donut(void);
static void donut_cmd(void) {
	printf("Donut demo...\n");
	donut();
}

extern void helloc(void);
static void helloc_cmd(void) {
	printf("Hello C demo...\n");
	helloc();
}

#ifdef WITH_CXX
extern void hellocpp(void);
static void hellocpp_cmd(void) {
	printf("Hello C++ demo...\n");
	hellocpp();
}
#endif

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
/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/
static void console_service(void) {
	char *line = readstr();
	if (!line) return;

	char *token = get_token(&line);
	if      (!strcmp(token, "help"))    help();
	else if (!strcmp(token, "reboot"))  reboot_cmd();
	#ifdef CSR_LEDS_BASE
	else if (!strcmp(token, "led"))     led_cmd();
	#endif
	else if (!strcmp(token, "donut"))   donut_cmd();
	else if (!strcmp(token, "helloc"))  helloc_cmd();
	#ifdef WITH_CXX
	else if (!strcmp(token, "hellocpp")) hellocpp_cmd();
	#endif
	#ifdef WITH_DAC
	else if (!strcmp(token, "dac1")) {
		char *arg = get_token(&line);
		dac1_cmd(arg);
	}
	else if (!strcmp(token, "dac2")) {
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
