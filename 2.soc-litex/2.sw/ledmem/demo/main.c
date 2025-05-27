#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>

/*-----------------------------------------------------------------------*/
/* Fan-out helper for LEDMem                                             */
/*-----------------------------------------------------------------------*/
#ifdef LEDMEM_AXI_BASE
static inline void write_led_pattern(uint32_t pattern) {
	volatile uint32_t *ledmem = (uint32_t *)LEDMEM_AXI_BASE;
	for (int i = 0; i < 4; i++) {
		/* write bit-i of pattern to ram[i], which drives LED[i] */
		ledmem[i] = (pattern >> i) & 0x1;
	}
}

static void readmem_cmd(void) {
	volatile uint32_t *ledmem = (uint32_t *)LEDMEM_AXI_BASE;
	printf("Reading back 4-word RAM @ 0x%08X:\n", (unsigned)LEDMEM_AXI_BASE);
	for (int i = 0; i < 4; i++) {
		uint32_t val = ledmem[i];
		printf("  ram[%d] = 0x%08X\n", i, val);
	}
}

static void pokeled_cmd(char *args) {
	int idx   = strtoul(strtok(args, " "), NULL, 0);
	uint32_t v = strtoul(strtok(NULL, " "),  NULL, 0);
	if (idx < 0 || idx > 3) {
		printf("pokeled: index out of range (0–3)\n");
	} else {
		volatile uint32_t *ledmem = (uint32_t *)LEDMEM_AXI_BASE;
		ledmem[idx] = v;
		printf("pokeled: wrote 0x%08X to ledmem[%d]\n", v, idx);
	}
}

static void peekled_cmd(char *args) {
	int idx = strtoul(strtok(args, " "), NULL, 0);
	if (idx < 0 || idx > 3) {
		printf("peekled: index out of range (0–3)\n");
	} else {
		volatile uint32_t *ledmem = (uint32_t *)LEDMEM_AXI_BASE;
		uint32_t v = ledmem[idx];
		printf("peekled: ledmem[%d] = 0x%08X\n", idx, v);
	}
}
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
			case 0x07:
				break;
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
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/
static void help(void) {
	puts("\nLiteX minimal demo app built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("  help               - Show this command");
	puts("  reboot             - Reboot CPU");
	#ifdef CSR_LEDS_BASE
	puts("  led                - LED demo");
	#endif
	#ifdef LEDMEM_AXI_BASE
	puts("  ledmem             - AXI-Lite LEDMem demo");
	puts("  cycle_leds         - Cycle LEDs with UART output");
	puts("  readmem            - Read back all four RAM words");
	puts("  pokeled <i> <v>    - Write word <v> to LEDMem index <i> (0–3)");
	puts("  peekled <i>        - Read  word from LEDMem index <i>");
	#endif
	puts("  donut              - Spinning Donut demo");
	puts("  helloc             - Hello C");
	#ifdef WITH_CXX
	puts("  hellocpp           - Hello C++");
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

#ifdef LEDMEM_AXI_BASE
static void ledmem_cmd(void) {
	printf("LEDMem demo (AXI-Lite @ 0x%08X)...\n", (unsigned)LEDMEM_AXI_BASE);
	for (int i = 0; i < 16; i++) {
		write_led_pattern(i);
		busy_wait(200);
	}
	for (int i = 0; i < 4; i++) {
		write_led_pattern(1 << i);
		busy_wait(200);
	}
}

static void cycle_leds(void) {
	uint32_t patterns[5] = {0x1, 0x2, 0x5, 0xF, 0x0};
	int idx = 0;
	printf("Starting LED cycle…\n");
	while (1) {
		uint32_t pat = patterns[idx];
		printf("Pattern %d: 0x%X\n", idx, pat);
		write_led_pattern(pat);
		idx = (idx + 1) % 5;
		busy_wait(2000);
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

/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/
static void console_service(void) {
	char *line = readstr();
	if (!line) return;

	char *token = get_token(&line);

	if (!strcmp(token, "help"))        help();
	else if (!strcmp(token, "reboot"))      reboot_cmd();
	#ifdef CSR_LEDS_BASE
	else if (!strcmp(token, "led"))         led_cmd();
	#endif
	#ifdef LEDMEM_AXI_BASE
	else if (!strcmp(token, "ledmem"))      ledmem_cmd();
	else if (!strcmp(token, "cycle_leds"))  cycle_leds();
	else if (!strcmp(token, "readmem"))     readmem_cmd();
	else if (!strcmp(token, "pokeled"))     pokeled_cmd(line);
	else if (!strcmp(token, "peekled"))     peekled_cmd(line);
	#endif
	else if (!strcmp(token, "donut"))       donut_cmd();
	else if (!strcmp(token, "helloc"))      helloc_cmd();
	#ifdef WITH_CXX
	else if (!strcmp(token, "hellocpp"))    hellocpp_cmd();
	#endif

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
