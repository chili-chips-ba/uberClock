#include <stdio.h>
#include <irq.h>
#include <libbase/uart.h>
#include "console.h"
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
	puts("  help_uc             - UberClock command list");
	puts("  help_ddr            - DDR/UberDDR3 memory command list");
	puts("  ddrinfo/ddrwait/... - DDR helpers (see also: ddrtest, ddrpat, timertest)");
	puts("  donut | helloc"
	#ifdef WITH_CXX
	" | hellocpp"
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

    puts("Type 'help' for top-level, 'help_uc' for UberClock commands, 'help_ddr' for DDR commands.");
	console_print_prompt();

	while (1) {
		console_poll();
		uberclock_poll();
	}
	return 0;
}
