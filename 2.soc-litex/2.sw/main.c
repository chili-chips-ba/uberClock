//--------------------------------------------------------------------------------
// uberClock
//--------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------
// System timing (optional)
//--------------------------------------------------------------------------------
#define SYS_HZ   65000000UL
#define TICK_HZ  10000UL
#define RELOAD   (SYS_HZ / TICK_HZ)   // 6500

//--------------------------------------------------------------------------------
// Console helpers
//--------------------------------------------------------------------------------
static char *readstr(void) {
	char c[2];
	static char s[64];
	static int ptr = 0;
	if (readchar_nonblock()) {
		c[0] = getchar();
		c[1] = '\0';
		switch (c[0]) {
			case 0x7f: case 0x08:  // backspace
				if (ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07: // bell, ignore
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
	printf("\e[92;1muberClock\e[0m> ");
}

static void help(void) {
	puts("\nuberClock built " __DATE__ " " __TIME__ "\n");
	puts("Available commands:");
	puts("  help                   - Show this command");
	puts("  reboot                 - Reboot CPU");
	puts("  phase_nco  <val>          - Set input CORDIC NCO phase increment (0–524287)");
	puts("  phase_down <val>          - Set downconversion CORDIC phase increment (0–524287)");
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
	puts("  gain1 <val>               - Set gain1 register");
	puts("  gain2 <val>               - Set gain2 register");
	puts("  hs_snap                   - Snapshot HS buffer and print 64 samples");
	puts("  ls_snap                   - Snapshot LS buffer and print 64 samples (needs 16384 CE pulses)");
	puts("  hs_peek                   - Dump last 64 HS samples (rolling)");
	puts("  ls_peek                   - Dump last 64 LS samples (rolling)");
	puts("  hs_peek_csv [N]           - Dump last N HS samples as CSV");
	puts("  ls_peek_csv [N]           - Dump last N LS samples as CSV");
	puts("  hs_snap_csv [N]           - Snapshot HS then dump N samples as CSV");
	puts("  ls_snap_csv [N]           - Snapshot LS then dump N samples as CSV");

}

//--------------------------------------------------------------------------------
// Reboot
//--------------------------------------------------------------------------------
static void reboot_cmd(void) {
	ctrl_reset_write(1);
}

//--------------------------------------------------------------------------------
// Main data-path CSRs
//--------------------------------------------------------------------------------
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

static void output_select_ch1_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0x3;
	main_output_select_ch1_write(v);
	printf("output_select_ch1 set to %u\n", v);
}

static void output_select_ch2_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0) & 0x3;
	main_output_select_ch2_write(v);
	printf("output_select_ch2 set to %u\n", v);
}

static void input_select_cmd(char *args) {
	unsigned v = strtoul(args, NULL, 0);
	main_input_select_write(v);
	printf("Main input select register set to %u\n", v);
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

//--------------------------------------------------------------------------------
// Event/IRQ: ce_down
//--------------------------------------------------------------------------------
static volatile bool ce_event = false;

static void ce_down_isr(void) {
	// acknowledge + mask further events until handled in main loop
	evm_pending_write(1);
	evm_enable_write(0);
	ce_event = true;
}

//--------------------------------------------------------------------------------
// HS/LS debug buffer helpers (match your Python + mem.h sizes)
// HS: 0x8000_0000, size 0x4000 → 8192 samples (16-bit)
// LS: 0x8000_8000, size 0x8000 → 16384 samples (16-bit)
//--------------------------------------------------------------------------------
#define HS_WIDTH_BITS 16
#define HS_DEPTH      8192
#define LS_WIDTH_BITS 16
#define LS_DEPTH      16384

static inline volatile uint16_t* hs_ptr(void) { return (volatile uint16_t*)HS_DBG_BASE; }
static inline volatile uint16_t* ls_ptr(void) { return (volatile uint16_t*)LS_DBG_BASE; }

static inline uint32_t wrap_pow2(uint32_t v, uint32_t depth) { return v & (depth - 1); }
static inline uint32_t wrap_sub(uint32_t a, uint32_t b, uint32_t depth) { return (a - b) & (depth - 1); }

// ---- HS controls ----
static void hs_clear(void){ hs_buf_clear_write(1); }
static void hs_mode_snapshot(void){ hs_buf_mode_write(1); }
static void hs_mode_rolling(void){ hs_buf_mode_write(0); }
static void hs_arm(void){ hs_buf_arm_write(1); }
static void hs_wait_done(void){ while(!hs_buf_done_read()) {} }
static uint32_t hs_start_idx(void){
	uint32_t wr = hs_buf_wr_ptr_read(); return wrap_sub(wr, HS_DEPTH, HS_DEPTH);
}
static void hs_dump(uint32_t start, uint32_t count){
	if(count > HS_DEPTH) count = HS_DEPTH;
	volatile uint16_t* base = hs_ptr();
	for(uint32_t i=0;i<count;i++){
		uint32_t idx = wrap_pow2(start + i, HS_DEPTH);
		printf("%u: %d\n", i, (int16_t)base[idx]);
	}
}

static void hs_dump_csv(uint32_t start, uint32_t count){
	if(count > HS_DEPTH) count = HS_DEPTH;
	volatile uint16_t* base = hs_ptr();
	for(uint32_t i=0;i<count;i++){
		uint32_t idx = wrap_pow2(start + i, HS_DEPTH);
		printf("%u,%d\n", i, (int16_t)base[idx]);
	}
}
// ---- LS controls ----
static void ls_clear(void){ ls_buf_clear_write(1); }
static void ls_mode_snapshot(void){ ls_buf_mode_write(1); }
static void ls_mode_rolling(void){ ls_buf_mode_write(0); }
static void ls_arm(void){ ls_buf_arm_write(1); }
static void ls_wait_done(void){ while(!ls_buf_done_read()) {} }
static uint32_t ls_start_idx(void){
	uint32_t wr = ls_buf_wr_ptr_read(); return wrap_sub(wr, LS_DEPTH, LS_DEPTH);
}
static void ls_dump(uint32_t start, uint32_t count){
	if(count > LS_DEPTH) count = LS_DEPTH;
	volatile uint16_t* base = ls_ptr();
	for(uint32_t i=0;i<count;i++){
		uint32_t idx = wrap_pow2(start + i, LS_DEPTH);
		printf("%u: %d\n", i, (int16_t)base[idx]);
	}
}

static void ls_dump_csv(uint32_t start, uint32_t count){
	if(count > LS_DEPTH) count = LS_DEPTH;
	volatile uint16_t* base = ls_ptr();
	for(uint32_t i=0;i<count;i++){
		uint32_t idx = wrap_pow2(start + i, LS_DEPTH);
		printf("%u,%d\n", i, (int16_t)base[idx]);
	}
}


//--------------------------------------------------------------------------------
// Debug buffer CLI commands
//--------------------------------------------------------------------------------
static void cmd_hs_snap(void){
	puts("\n[HS] snapshot 64:");
	hs_clear(); hs_mode_snapshot(); hs_arm(); hs_wait_done();
	uint32_t start = hs_start_idx();
	hs_dump(start, 64);
}

static void cmd_ls_snap(void){
	puts("\n[LS] snapshot 64:");
	ls_clear(); ls_mode_snapshot(); ls_arm(); ls_wait_done(); // needs 16384 CE pulses
	uint32_t start = ls_start_idx();
	ls_dump(start, 64);
}

static void cmd_hs_peek(void){
	puts("\n[HS] rolling peek last 64:");
	hs_mode_rolling();
	for(volatile int i=0;i<100000;i++) {}  // tiny delay
	uint32_t wr = hs_buf_wr_ptr_read();
	uint32_t start = wrap_sub(wr, 64, HS_DEPTH);
	hs_dump(start, 64);
}

static void cmd_ls_peek(void){
	puts("\n[LS] rolling peek last 64:");
	ls_mode_rolling();
	for(volatile int i=0;i<100000;i++) {}
	uint32_t wr = ls_buf_wr_ptr_read();
	uint32_t start = wrap_sub(wr, 64, LS_DEPTH);
	ls_dump(start, 64);
}

static void cmd_hs_peek_csv(char *args){
	uint32_t n = strtoul(args ? args : "256", NULL, 0);
	hs_mode_rolling();
	for(volatile int i=0;i<100000;i++) {}
	uint32_t wr = hs_buf_wr_ptr_read();
	uint32_t start = wrap_sub(wr, n, HS_DEPTH);
	hs_dump_csv(start, n);
}

static void cmd_ls_peek_csv(char *args){
	uint32_t n = strtoul(args ? args : "256", NULL, 0);
	ls_mode_rolling();
	for(volatile int i=0;i<100000;i++) {}
	uint32_t wr = ls_buf_wr_ptr_read();
	uint32_t start = wrap_sub(wr, n, LS_DEPTH);
	ls_dump_csv(start, n);
}

static void cmd_hs_snap_csv(char *args){
	uint32_t n = strtoul(args ? args : "256", NULL, 0);
	hs_clear(); hs_mode_snapshot(); hs_arm(); hs_wait_done();
	hs_dump_csv(hs_start_idx(), n);
}

static void cmd_ls_snap_csv(char *args){
	uint32_t n = strtoul(args ? args : "256", NULL, 0);
	ls_clear(); ls_mode_snapshot(); ls_arm(); ls_wait_done();
	ls_dump_csv(ls_start_idx(), n);
}

//--------------------------------------------------------------------------------
// Console dispatcher
//--------------------------------------------------------------------------------
static void console_service(void) {
	char *line = readstr();
	if (!line) return;

	char *token = get_token(&line);

	if      (!strcmp(token, "help"))          help();
	else if (!strcmp(token, "reboot"))        reboot_cmd();
	else if (!strcmp(token, "phase_nco"))     { char *arg = get_token(&line); phase_nco_cmd(arg); }
	else if (!strcmp(token, "phase_down"))    { char *arg = get_token(&line); phase_down_cmd(arg); }
	else if (!strcmp(token, "output_select_ch1")) { char *arg = get_token(&line); output_select_ch1_cmd(arg); }
	else if (!strcmp(token, "output_select_ch2")) { char *arg = get_token(&line); output_select_ch2_cmd(arg); }
	else if (!strcmp(token, "input_select"))  { char *arg = get_token(&line); input_select_cmd(arg); }
	else if (!strcmp(token, "hs_snap"))       cmd_hs_snap();
	else if (!strcmp(token, "ls_snap"))       cmd_ls_snap();
	else if (!strcmp(token, "hs_peek"))       cmd_hs_peek();
	else if (!strcmp(token, "ls_peek"))       cmd_ls_peek();
	else if (!strcmp(token, "hs_peek_csv")) { char *arg=get_token(&line); cmd_hs_peek_csv(arg); }
	else if (!strcmp(token, "ls_peek_csv")) { char *arg=get_token(&line); cmd_ls_peek_csv(arg); }
	else if (!strcmp(token, "hs_snap_csv")) { char *arg=get_token(&line); cmd_hs_snap_csv(arg); }
	else if (!strcmp(token, "ls_snap_csv")) { char *arg=get_token(&line); cmd_ls_snap_csv(arg); }

	else {
		printf("Unknown command: %s\n", token);
	}

	prompt();
}

//--------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------
int main(void) {
	// Optional: sane defaults so you hear/see something
	main_phase_inc_nco_write(80660);
	main_phase_inc_down_write(80652);
	main_input_select_write(1);
	main_gain1_write (0x40000000);
	main_gain2_write (0x40000000);
	main_output_select_ch1_write(0);
	main_output_select_ch2_write(0);

	// UART + banner
	uart_init();

	// Configure CE_DOWN IRQ
	evm_pending_write(1);         // clear any stale
	evm_enable_write(1);          // unmask in EventManager
	irq_setie(0);
	irq_attach(EVM_INTERRUPT, ce_down_isr);
	irq_setmask(irq_getmask() | (1 << EVM_INTERRUPT));
	irq_setie(1);

	// Hello + prompt
	help();
	prompt();

	// Main loop: console + CE_DOWN handling
	while (1) {
		console_service();

		if (ce_event) {
			// Process CE_DOWN in thread context
			uint16_t ds_x = main_downsampled_data_x_read();
			uint32_t doubled_x = (uint32_t)ds_x * 2u;
			main_upsampler_input_x_write(doubled_x);

			uint16_t ds_y = main_downsampled_data_y_read();
			uint32_t doubled_y = (uint32_t)ds_y * 2u;
			main_upsampler_input_y_write(doubled_y);

			ce_event = false;

			// Re-arm the next CE_DOWN
			evm_pending_write(1);
			evm_enable_write(1);
		}
	}
}
