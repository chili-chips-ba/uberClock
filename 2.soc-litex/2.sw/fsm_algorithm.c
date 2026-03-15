#include "fsm_algorithm.h"
#include "uberclock.h"

#include <generated/csr.h>
#include <generated/soc.h>

enum fsm_states {IDLE, S1, S2};
char curr_state;
uint32_t fsm_counter, max_mag, current_phase_inc, max_mag_phase_inc, shooting_phase_inc ;
static volatile uint32_t ce_ticks = 0; 
int8_t sgn = 1;
int16_t mag;
int32_t phase_val;
void fsm_init(void) {
 curr_state = IDLE; 
 ce_ticks= 0;
 max_mag = 0;
 max_mag_phase_inc = 0; 
 shooting_phase_inc = 2582065;
}
static void cmd_phase_print(){  printf("Phase %ld\n", (long)phase_val); }
static void cmd_magnitude  (){  printf("Magnitude %d\n", mag); }
void tran(void) {
    switch (curr_state) {
        case IDLE: {
            if (ce_ticks == 39999 ) {
                curr_state = S1;
            }  else if (ce_ticks ==1) {
                main_phase_inc_nco_write(shooting_phase_inc);
                main_phase_inc_down_1_write(shooting_phase_inc + 500);  
                printf("Input NCO phase increment set to %u\n", shooting_phase_inc);
            }
        }
        break;
        case S1: {
                     cmd_magnitude();
                     if (mag < 30) {
                       curr_state = IDLE;
                       ce_ticks = 0;
                       shooting_phase_inc = shooting_phase_inc + 2;
                       
                     }else 

                     if ( (uint32_t)mag + 10  > max_mag  ) {
                         puts("mag greater");
                       max_mag = mag; 
                       max_mag_phase_inc = shooting_phase_inc;
                       shooting_phase_inc = shooting_phase_inc + sgn * 1;
                       curr_state = IDLE;
                       ce_ticks = 0;
                    } else {
                       //  sgn = -sgn;
                       // shooting_phase_inc = shooting_phase_inc - sgn * 2;
                        main_phase_inc_nco_write(shooting_phase_inc - 1);
                       ce_ticks = 0;
                       curr_state = S2;
                    }
                 }
            break;
        
        case S2: {
            puts("S2");
            cmd_magnitude();
            // curr_state = IDLE;
                 }
            break;
    }
}
