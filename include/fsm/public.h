#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "rhapsody.h"

/* Config extern */
extern const FSM_CONFIG_T fsm_config;

/* State machine #define list of states */
#define FSM_INIT_STATE 0
#define FSM_IDLE_STATE 1
#define FSM_SEND_STATE 2

/* State machine loop delay in seconds */
#define FSM_LOOP_DELAY 1

#endif /* _PUBLIC_H_ */
