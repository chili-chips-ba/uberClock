#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>
#include <generated/mem.h>

#ifndef MEM_AD_DA_BASE
#  define MEM_AD_DA_BASE 0x30000100
#endif

/*-----------------------------------------------------------------------*/
/* UART input                                                            */
/*-----------------------------------------------------------------------*/
static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if (readchar_nonblock()) {
		c[0] = getchar();
		c[1] = 0;
		switch (c[0]) {
			case 0x7f:
			case 0x08:
				if (ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
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

static char *get_token(char **str)
{
	char *p = strchr(*str, ' ');
	char *tok;
	if (!p) {
		tok = *str;
		*str += strlen(*str);
	} else {
		*p = '\0';
		tok = *str;
		*str = p + 1;
	}
	return tok;
}

static void prompt(void)
{
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/
static void help(void)
{
	puts("\nLiteX minimal demo app built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("help               - Show this command");
	puts("reboot             - Reboot CPU");
	#ifdef CSR_LEDS_BASE
	puts("led                - LED demo");
	#endif
	#ifdef MEM_AD_DA_BASE
	puts("ad_da              - ADC→DAC loopback demo");
	#endif
	puts("donut              - Spinning Donut demo");
	puts("helloc             - Hello C");
	#ifdef WITH_CXX
	puts("hellocpp           - Hello C++");
	#endif
}

/*-----------------------------------------------------------------------*/
/* Commands                                                              */
/*-----------------------------------------------------------------------*/
static void reboot_cmd(void)
{
	ctrl_reset_write(1);
}

#ifdef CSR_LEDS_BASE
static void led_cmd(void)
{
	int i;
	printf("LED demo...\n");

	printf("Counter mode...\n");
	for (i = 0; i < 32; i++) {
		leds_out_write(i);
		busy_wait(100);
	}

	printf("Shift mode...\n");
	for (i = 0; i < 4; i++) {
		leds_out_write(1 << i);
		busy_wait(200);
	}
	for (i = 0; i < 4; i++) {
		leds_out_write(1 << (3 - i));
		busy_wait(200);
	}

	printf("Dance mode...\n");
	for (i = 0; i < 4; i++) {
		leds_out_write(0x55);
		busy_wait(200);
		leds_out_write(0xAA);
		busy_wait(200);
	}
}
#endif

#ifdef MEM_AD_DA_BASE
static void ad_da_cmd(void)
{
	volatile uint32_t *ad_da = (uint32_t *)MEM_AD_DA_BASE;
	const int ADC_REG = 0;    /* low 12 bits = ADC sample */
	const int DAC_REG = 1;    /* low 14 bits → DAC */
	uint32_t raw, dac_val;

	printf("\n--- Starting ADC→DAC loopback ---\n");
	while (1) {
		/* read the 12-bit ADC sample */
		raw = ad_da[ADC_REG] & 0x0FFF;
		/* scale to 14-bit for DAC */
		dac_val = (raw << 2) & 0x3FFF;
		/* write to DAC register */
		ad_da[DAC_REG] = dac_val;
		/* report */
		printf("ADC = %4u   →   DAC = %4u\n", raw, dac_val);
		/* slow down */
		busy_wait(500000);
	}
}
#endif

extern void donut(void);
static void donut_cmd(void)
{
	printf("Donut demo...\n");
	donut();
}

extern void helloc(void);
static void helloc_cmd(void)
{
	printf("Hello C demo...\n");
	helloc();
}

#ifdef WITH_CXX
extern void hellocpp(void);
static void hellocpp_cmd(void)
{
	printf("Hello C++ demo...\n");
	hellocpp();
}
#endif

/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/
static void console_service(void)
{
	char *str, *tok;
	str = readstr();
	if (!str) return;
	tok = get_token(&str);

	if (strcmp(tok, "help") == 0) {
		help();
	} else if (strcmp(tok, "reboot") == 0) {
		reboot_cmd();
		#ifdef CSR_LEDS_BASE
	} else if (strcmp(tok, "led") == 0) {
		led_cmd();
		#endif
		#ifdef MEM_AD_DA_BASE
	} else if (strcmp(tok, "ad_da") == 0) {
		ad_da_cmd();
		#endif
	} else if (strcmp(tok, "donut") == 0) {
		donut_cmd();
	} else if (strcmp(tok, "helloc") == 0) {
		helloc_cmd();
		#ifdef WITH_CXX
	} else if (strcmp(tok, "hellocpp") == 0) {
		hellocpp_cmd();
		#endif
	}
	prompt();
}

int main(void)
{
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
