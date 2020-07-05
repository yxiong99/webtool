//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: Nov 2019
//!
//******************************************************************************

#include "private.h"
#include "task.h"

/* Entry action function definitions */
/* INIT_STATE entry action function definition */
void init_state_entry(void)
{
   initEntry();
}

/* IDLE_STATE entry action function definition */
void idle_state_entry(void)
{
   idleEntry();
}

/* SEND_STATE entry action function definition */
void send_state_entry(void)
{
   sendEntry();
}

/* Activity function definitions */
/* INIT_STATE activity function definition */
void init_state_act(void)
{
   initActivity();
}

/* IDLE_STATE activity function definition */
void idle_state_act(void)
{
   idleActivity();
}

/* SEND_STATE activity function definition */
void send_state_act(void)
{
   sendActivity();
}

/* Exit action function definitions */
/* SEND_STATE exit action function definition */
void send_state_exit(void)
{
   sendExit();
}

/* Transition action function definitions */
/* IDLE_STATE state transition action function definitions */
/* IDLE_STATE to SEND_STATE transition action */
void idle_state_to_send_state1(void)
{
   resetSessionStatus();
}

/* SEND_STATE state transition action function definitions */
/* SEND_STATE to SEND_STATE transition action */
void send_state_to_send_state2(void)
{
   fsm_clear_trans_history(&fsm_config);
}
