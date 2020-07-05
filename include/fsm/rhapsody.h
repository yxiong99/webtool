#ifndef _RHAPSODY_H_
#define _RHAPSODY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

//
// Constants
//
#define FSM_TRANS_END 255

//
// Macros for setting/clearing bits in the input word
//
#define SET_INP_BIT(input_word,bit_mask) input_word |= bit_mask
#define CLR_INP_BIT(input_word,bit_mask) input_word &= ~(bit_mask)

//
// FSM workspace structure for persistent data
//
typedef struct
{
   uint32_t timer1;
   uint32_t timer1_timeout;
   uint32_t timer1_timeout_backup;
   uint32_t timer2;
   uint32_t timer2_timeout;
   uint32_t timer2_timeout_backup;
   uint32_t timer3;
   uint32_t timer3_timeout;
   uint32_t timer3_timeout_backup;
   uint32_t last_input_word; /* Last input word */
   uint32_t last_event_trans_cond_mask; /* Last condition mask that caused an event */
   uint8_t  timer1_enable; /* Indicates whether timer1 is enabled or not */
   uint8_t  timer2_enable; /* Indicates whether timer2 is enabled or not */
   uint8_t  timer3_enable; /* Indicates whether timer3 is enabled or not */
   uint8_t  current_state;
   uint8_t  transition; /* Flag to indicate that a transition just occurred */
}
FSM_WORKSPACE_T;


//
// State transition configuration structure
//
typedef struct
{
   uint8_t  next_state;
   uint32_t trans_cond_mask; /* Transition condition mask (use 0 for don't care bits) */
   uint32_t trans_dont_care_mask; /* Transition don't care mask to indicate don't care conditions with a 0 */
   void (*trans_funct_ptr)(void); /* Function to run upon state transition to next state */
}
FSM_TRANS_CONFIG_T;


//
// State configuration structure
//
typedef struct
{
   void (*entry_action_funct_ptr)(void); /* Function to run upon state entry */
   void (*activity_funct_ptr)(void); /* Function to run while in the state */
   void (*exit_action_funct_ptr)(void); /* Function to run upon state entry */
   const FSM_TRANS_CONFIG_T *state_trans_config_ptr; /* Pointer to current state transition config */
}
FSM_STATE_CONFIG_T;


//
// State machine main configuration structure
//
typedef struct
{
   uint8_t   initial_state;
   uint32_t* trans_input_ptr;
   const FSM_STATE_CONFIG_T *state_config_ptr;
   FSM_WORKSPACE_T *workspace_ptr;
   void (*process_inputs_funct_ptr)(void); /* Function to process the input word bits */
   void (*power_up_trans_funct_ptr)(void); /* Function for power up if needed */
}
FSM_CONFIG_T;

//
// FSM Main Routines
//

#ifdef __cplusplus
extern "C" {
#endif

void fsm_init(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_process(const FSM_CONFIG_T *fsm_config_ptr);

//
// Timer 1 Routines
//
void fsm_timer1_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer1_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer1_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer1_disable(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer1_stop(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer1_start(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer1_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer1_get_timeout(const FSM_CONFIG_T *fsm_config_ptr);

//
// Timer 2 Routines
//
void fsm_timer2_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer2_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer2_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer2_disable(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer2_stop(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer2_start(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer2_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer2_get_timeout(const FSM_CONFIG_T *fsm_config_ptr);

//
// Timer 3 Routines
//
void fsm_timer3_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer3_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer3_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer3_disable(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer3_stop(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_timer3_start(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer3_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr);
uint32_t fsm_timer3_get_timeout(const FSM_CONFIG_T *fsm_config_ptr);

//
// State and Transition Routines
//
uint8_t fsm_get_current_state(const FSM_CONFIG_T *fsm_config_ptr);
void fsm_clear_trans_history(const FSM_CONFIG_T *fsm_config_ptr);

#ifdef __cplusplus
}
#endif

#endif /* _RHAPSODY_H_ */
