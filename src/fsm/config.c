//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: Nov 2019
//! 
//******************************************************************************

#include "private.h"

/* Allocate workspace */
FSM_WORKSPACE_T fsm_workspace;

/* State machine configuration */

/* init_state state transition configuration */
const FSM_TRANS_CONFIG_T init_state_trans_config[] =
{
   { FSM_IDLE_STATE, 0x00000009, 0x00000009, 0 },
   { FSM_TRANS_END, 0, 0, 0 }
};

/* idle_state state transition configuration */
const FSM_TRANS_CONFIG_T idle_state_trans_config[] =
{
   { FSM_SEND_STATE, 0x0000000B, 0x0000000B, &idle_state_to_send_state1 },
   { FSM_INIT_STATE, 0x00000001, 0x00000009, 0 },
   { FSM_INIT_STATE, 0x00000000, 0x00000001, 0 },
   { FSM_TRANS_END, 0, 0, 0 }
};

/* send_state state transition configuration */
const FSM_TRANS_CONFIG_T send_state_trans_config[] =
{
   { FSM_INIT_STATE, 0x00000001, 0x00000009, 0 },
   { FSM_INIT_STATE, 0x00000000, 0x00000001, 0 },
   { FSM_IDLE_STATE, 0x00000000, 0x00000002, 0 },
   { FSM_IDLE_STATE, 0x00000004, 0x00000004, 0 },
   { FSM_TRANS_END, 0, 0, 0 }
};

const FSM_STATE_CONFIG_T fsm_state_config[] =
{
   { /* init_state state configuration */
      &init_state_entry,
      &init_state_act,
      0,
      &init_state_trans_config[0]
   },
   { /* idle_state state configuration */
      &idle_state_entry,
      &idle_state_act,
      0,
      &idle_state_trans_config[0]
   },
   { /* send_state state configuration */
      &send_state_entry,
      &send_state_act,
      &send_state_exit,
      &send_state_trans_config[0]
   },
};

const FSM_CONFIG_T fsm_config =
{
   FSM_INIT_STATE,
   &fsm_trans_input,
   &fsm_state_config[0],
   &fsm_workspace,
   &fsm_process_inputs,
   0
};
