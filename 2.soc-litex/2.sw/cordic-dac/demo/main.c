#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>

/*-----------------------------------------------------------------------*/
/* Conditionally enable CORDIC commands if the CSR exists               */
/*-----------------------------------------------------------------------*/
#ifdef CSR_MAIN_CORDIC_PHASE_ADDR
#  define WITH_CORDIC
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
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

static void help(void) {
	puts("\nLiteX minimal demo app built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("  help               - Show this command");
	puts("  reboot             - Reboot CPU");
	#ifdef CSR_LEDS_BASE
	puts("  led                - LED demo");
	#endif
	puts("  donut              - Spinning Donut demo");
	puts("  helloc             - Hello C");
	#ifdef WITH_CXX
	puts("  hellocpp           - Hello C++");
	#endif
	#ifdef WITH_CORDIC
	puts("  phase <value>      - Set CORDIC phase (0..524287), wait, read & display");
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

#ifdef WITH_CORDIC
/* “phase” command: just update the phase register */
static void phase_cmd(char *args) {
	unsigned p = strtoul(args, NULL, 0);
	if (p >= (1u << 19)) {
		printf("Error: phase must be 0–524287\n");
		return;
	}
	/* write the new phase, leave it to the hardware to update */
	main_cordic_phase_write(p);
	printf("Phase changed to %u\n", p);
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
	#ifdef WITH_CORDIC
	else if (!strcmp(token, "phase")) {
		char *arg = get_token(&line);
		phase_cmd(arg);
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
