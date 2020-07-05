#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#include "public.h"

//
// Private Variables
//
extern int fsm_state;
extern bool initialized;
extern bool data_sending;

/* Transition input word extern */
extern uint32_t fsm_trans_input;

/* Input word processing function prototype */
void fsm_process_inputs(void);

/* Entry action prototypes */
void init_state_entry(void);
void idle_state_entry(void);
void send_state_entry(void);

/* Activity function prototypes */
void init_state_act(void);
void idle_state_act(void);
void send_state_act(void);

/* Exit action prototypes */
void send_state_exit(void);

/* Transition action prototypes */
void idle_state_to_send_state1(void);
void send_state_to_send_state2(void);

#endif /* _PRIVATE_H_ */
