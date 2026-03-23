#include <stdio.h>
#include <irq.h>
#include <libbase/uart.h>
#include "inc/console.h"
#include "uberclock.h"
#include "ubddr3.h"

extern void donut(void);
extern void helloc(void);
#ifdef WITH_CXX
extern void hellocpp(void);
#endif

static void cmd_donut(char *a){ (void)a; puts_help_header("Donut"); donut(); }
static void cmd_helloc(char *a){ (void)a; puts_help_header("Hello C"); helloc(); }
#ifdef WITH_CXX
static void cmd_hellocpp(char *a){ (void)a; puts_help_header("Hello C++"); hellocpp(); }
#endif
static void cmd_help_root(char *a){
	(void)a;
	puts_help_header("Top-level help");
	puts("  help        - Show this overview");
	puts("  help_uc     - UberClock control, debug, capture, DSP, and UDP commands");
	puts("  help_ddr    - DDR/UberDDR3 memory bring-up and test commands");
	puts("  ub_help     - UDP / S2MM streaming commands only");
	puts("  ddrinfo     - Quick DDR calibration / base-address status");
	puts("  donut       - ASCII donut demo");
	puts("  helloc      - Minimal C hello test"
	#ifdef WITH_CXX
	"  |  hellocpp - Minimal C++ hello test"
	#endif
	);
}

static const struct cmd_entry g_root_cmds[] = {
	{ "help",   cmd_help_root },
	{ "donut",  cmd_donut     },
	{ "helloc", cmd_helloc    },
	#ifdef WITH_CXX
	{ "hellocpp", cmd_hellocpp },
	#endif
};

int main(void) {
	#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
	#endif
	uart_init();

	console_init("\e[92;1muberClock\e[0m>");
	console_register(g_root_cmds, sizeof(g_root_cmds)/sizeof(g_root_cmds[0]));
	uberclock_register_cmds();
	ubddr3_register_cmds();

	uberclock_init();

    puts("Type 'help' for an overview, 'help_uc' for UberClock controls, 'ub_help' for UDP/S2MM, or 'help_ddr' for DDR tests.");
	console_print_prompt();

	while (1) {
		console_poll();
		uberclock_poll();
	}
	return 0;
}
