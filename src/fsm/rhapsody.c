//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: Nov 2019
//!
//******************************************************************************

#include "rhapsody.h"

//
// Types specific to FSM files
//
#ifndef TRUE
#define TRUE   true
#endif

#ifndef FALSE
#define FALSE  false
#endif

static const uint32_t bitmasks_32bit[32] =
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};

//
// Local Constants
//
#define FSM_TIMER1_BIT  bitmasks_32bit[31]
#define FSM_TIMER2_BIT  bitmasks_32bit[30]
#define FSM_TIMER3_BIT  bitmasks_32bit[29]

#define RESERVED_INPUT_WORD_VALUE 0xFFFFFFFF

/****************************************************************************

   fsm_init

   Description:   This initializes a state machine's workspace by performing
   the following:
   1) Setting the initial state and setting the state transition flag.  Setting
      the state transition flag tells the state machine engine that there was a
      state transition so the state machine engine will run the entry action
      (if one is configured).
   2) Disabling the state machine timer and clearing the timer values and
      timeouts.
   3) Initializing the last input word and last condition mask that caused
      a transition.

   This also clears the state machine's input word.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Initialized state machine workspace and input word.

   Return:
      void

****************************************************************************/
void fsm_init(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Set the current to the initial state, set state transition flag to indicate
      a state transition just occurred so the entry action will be executed
      for the initial state, disable the state machine timers, and initialize the
      last input word storage */
   local_workspace_ptr->current_state = fsm_config_ptr->initial_state;
   local_workspace_ptr->transition = TRUE;
   local_workspace_ptr->timer1_enable = FALSE;
   local_workspace_ptr->timer1 = 0;
   local_workspace_ptr->timer1_timeout = 0;
   local_workspace_ptr->timer1_timeout_backup = 0;
   local_workspace_ptr->timer2_enable = FALSE;
   local_workspace_ptr->timer2 = 0;
   local_workspace_ptr->timer2_timeout = 0;
   local_workspace_ptr->timer2_timeout_backup = 0;
   local_workspace_ptr->timer3_enable = FALSE;
   local_workspace_ptr->timer3 = 0;
   local_workspace_ptr->timer3_timeout = 0;
   local_workspace_ptr->timer3_timeout_backup = 0;
   local_workspace_ptr->last_input_word = RESERVED_INPUT_WORD_VALUE;
   local_workspace_ptr->last_event_trans_cond_mask = RESERVED_INPUT_WORD_VALUE;

   /* Clear the input word */
   *(fsm_config_ptr->trans_input_ptr) = 0;

   /* if power up trans function exist execute it */
   if (NULL != fsm_config_ptr->power_up_trans_funct_ptr)
   {
      /* Execute the power up function */
      (*(fsm_config_ptr->power_up_trans_funct_ptr))();
   }

}

/****************************************************************************

   fsm_process

   Description:   This performs the following:
   1) If the state machine timer1 is enabled, then check to see if the timer
      has expired.  If it hasn't expired then increment timer1.  If it has
      expired then set the state machine timer1 bit.
   2) If the state machine timer2 is enabled, then check to see if the timer
      has expired.  If it hasn't expired then increment timer2.  If it has
      expired then set the state machine timer2 bit.
   3) If the state machine timer3 is enabled, then check to see if the timer
      has expired.  If it hasn't expired then increment timer3.  If it has
      expired then set the state machine timer3 bit.
   4) If this is the first call after transition, then this executes the
      entry action function (if one is configured) for the state and resets
      the state transition flag in the state machine's workspace.
   5) Executes the activity function (if one is configured) for the current
      state.  NOTE:  This always executes the activity function whether it
      is the first call after a transition or not.
   6) Executes the input word processing function.
   7) If the input word has changed, then check to see if it is time for a
      transition.
   8) If it is time for a transition and the condition mask value is different
      for this transition than the last transition or it is a transition to
      another state, then it is time for a transition.  The condition mask
      check is performed to allow transitions to the current state without
      causing race conditions.  Also, if a transition is performed to the
      same state, then that transition can't be the next transition.  This
      performs the following if it is time for a transition:
      A)  If this is a transition to a different state, then set the last input
          word and last transition condition members to invalid values.  This
          is done to gaurantee that a transition will occur after the new state
          is entered if the input word value warrants a transition.  This also
          updates the current state in the workspace if this is a transition to
          a different state.
      B)  Execute the current state's exit action function (if one is configured)
          and execute the transition action function (if one is configured).
      C)  Sets sets the state transition flag in the workspace so the entry
          action will be executed during the next call (see 2 above).

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Updated state machine workspace.

   Return:
      void

****************************************************************************/
void fsm_process(const FSM_CONFIG_T *fsm_config_ptr)
{
   const FSM_STATE_CONFIG_T *local_state_config_ptr;
   const FSM_TRANS_CONFIG_T *local_state_trans_config_ptr;
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* IF1 The state machine timer1 is enabled */
   if(FALSE != local_workspace_ptr->timer1_enable)
   {
      /* THEN1 Process timer1 */
      /* IF2 The time has not expired */
#if (defined(FSM_DEBUG_VERBOSE))
      fprintf(stdout, "[1] >>> timer1: (%d) %d/%d  %d\n", local_workspace_ptr->timer1_enable, (int)local_workspace_ptr->timer1, (int)local_workspace_ptr->timer1_timeout, (int)local_workspace_ptr->timer1_timeout_backup); fflush(stdout);
#endif
      if((local_workspace_ptr->timer1 < local_workspace_ptr->timer1_timeout) &&
         (local_workspace_ptr->timer1 < local_workspace_ptr->timer1_timeout_backup) )
      {
         /* THEN2 increment timer1 */
         local_workspace_ptr->timer1++;
#if (defined(FSM_DEBUG_VERBOSE))
         fprintf(stdout, "[1A] >>> timer1: (%d) %d/%d  %d\n", local_workspace_ptr->timer1_enable, (int)local_workspace_ptr->timer1, (int)local_workspace_ptr->timer1_timeout, (int)local_workspace_ptr->timer1_timeout_backup); fflush(stdout);
#endif
      }
      else
      {
         /* ELSE2 Set the "timer1" bit in the SM input word */
         *(fsm_config_ptr->trans_input_ptr) |= (uint32_t)FSM_TIMER1_BIT;
#if (defined(FSM_DEBUG_VERBOSE))
         fprintf(stdout, "[1B] >>> timer1: (%d) %d/%d  %d\n", local_workspace_ptr->timer1_enable, (int)local_workspace_ptr->timer1, (int)local_workspace_ptr->timer1_timeout, (int)local_workspace_ptr->timer1_timeout_backup); fflush(stdout);
#endif
      }
   }

   /* IF3 The state machine timer2 is enabled */
   if(FALSE != local_workspace_ptr->timer2_enable)
   {
      /* THEN3 Process timer2 */
      /* IF4 The time has not expired */
      if((local_workspace_ptr->timer2 < local_workspace_ptr->timer2_timeout) &&
         (local_workspace_ptr->timer2 < local_workspace_ptr->timer2_timeout_backup) )
      {
         /* THEN4 increment timer2 */
         local_workspace_ptr->timer2++;
      }
      else
      {
         /* ELSE4 Set the "timer2" bit in the SM input word */
         *(fsm_config_ptr->trans_input_ptr) |= (uint32_t)FSM_TIMER2_BIT;
      }
   }

   /* IF3 The state machine timer3 is enabled */
   if(FALSE != local_workspace_ptr->timer3_enable)
   {
      /* THEN3 Process timer3 */
      /* IF4 The time has not expired */
      if((local_workspace_ptr->timer3 < local_workspace_ptr->timer3_timeout) &&
         (local_workspace_ptr->timer3 < local_workspace_ptr->timer3_timeout_backup) )
      {
         /* THEN4 increment timer3 */
         local_workspace_ptr->timer3++;
      }
      else
      {
         /* ELSE4 Set the "timer3" bit in the SM input word */
         *(fsm_config_ptr->trans_input_ptr) |= (uint32_t)FSM_TIMER3_BIT;
      }
   }

   /* Init local state config pointer based on the "new" current state */
   local_state_config_ptr = fsm_config_ptr->state_config_ptr + local_workspace_ptr->current_state;

   /* IF5 A transition just occurred */
   if (FALSE != local_workspace_ptr->transition)
   {
      /* THEN5 IF6 There is a defined entry action function */
      if (NULL != local_state_config_ptr->entry_action_funct_ptr)
      {
         /* THEN6 Execute the entry action function */
         (*(local_state_config_ptr->entry_action_funct_ptr))();
      }
      /* Reset the state transition flag */
      local_workspace_ptr->transition = FALSE;
   }

   /* IF7 There is an activity function for the current state */
   if (NULL != local_state_config_ptr->activity_funct_ptr)
   {
      /* THEN7 Execute the current state activity function */
      (*(local_state_config_ptr->activity_funct_ptr))();
   }

   /* Execute the input word processing function */
   (*(fsm_config_ptr->process_inputs_funct_ptr))();

   /* IF9 The input word changed (an "event" occurred) */
   if(*(fsm_config_ptr->trans_input_ptr) != local_workspace_ptr->last_input_word)
   {
      /* THEN9 Update the last input word value */
      local_workspace_ptr->last_input_word = *(fsm_config_ptr->trans_input_ptr);

      /* Prepare to check for a transition.  Init local state transition config and
         state trans pointers based on the current state */
      local_state_trans_config_ptr = local_state_config_ptr->state_trans_config_ptr;

      /* Process all entries in the state transition table looking to see if it is time
         to transition out of current state */
      while (FSM_TRANS_END != local_state_trans_config_ptr->next_state)
      {
         /* Perform a transition check */
         /* IF10 It is possibly time for a transition - if the following calculation == 0 then
           it may be time to transition:  (Input XOR trans_cond_mask) AND trans_dont_care_mask */
         if(0 == ((*(fsm_config_ptr->trans_input_ptr) ^ local_state_trans_config_ptr->trans_cond_mask)
            & local_state_trans_config_ptr->trans_dont_care_mask))
         {
            /* THEN10 IF11 This "event" condition mask is different than the last "event"
               condition mask (This check prevents race conditions when a state
               transitions to itself OR this is a transition to another state (i.e. there is
               no race condition to itself) */
            if((local_workspace_ptr->last_event_trans_cond_mask != local_state_trans_config_ptr->trans_cond_mask) ||
               (local_workspace_ptr->current_state != local_state_trans_config_ptr->next_state))
            {
#if (defined(FSM_DEBUG_VERBOSE))
               FSM_Transition(local_workspace_ptr->current_state, local_state_trans_config_ptr->next_state, *(fsm_config_ptr->trans_input_ptr));
#endif
               /* THEN11 "Reset" the last transition condition mask that caused an transition */
               local_workspace_ptr->last_event_trans_cond_mask = local_state_trans_config_ptr->trans_cond_mask;

               /* IF12 This transition is to a different state */
               if(local_workspace_ptr->current_state != local_state_trans_config_ptr->next_state)
               {
                  /* THEN12 Set the last input word value and clear the last condition mask
                     value to gaurantee that the next transition check will cause
                     a transition if the check is true.  Also, set current_state to the new
                     state */
                  local_workspace_ptr->last_input_word = RESERVED_INPUT_WORD_VALUE;
                  local_workspace_ptr->last_event_trans_cond_mask = RESERVED_INPUT_WORD_VALUE;
                  local_workspace_ptr->current_state = local_state_trans_config_ptr->next_state;
               }

               /* IF13 There is a defined exit action function */
               if(NULL != local_state_config_ptr->exit_action_funct_ptr)
               {
                  /* THEN13 Execute the exit action function */
                  (*(local_state_config_ptr->exit_action_funct_ptr))();
               }

               /* IF14 There is a defined transition action function */
               if (NULL != local_state_trans_config_ptr->trans_funct_ptr)
               {
                  /* THEN14 Execute the transition action function */
                  (*(local_state_trans_config_ptr->trans_funct_ptr))();
               }

               /* Set the state transition flag so the entry
                  action will be executed next time */
               local_workspace_ptr->transition = TRUE;
               break; /* Break out of the while loop */
            }
         }
         /* Advance to next state transition table entry */
         local_state_trans_config_ptr++;
      }
   }
}

/****************************************************************************

   fsm_timer1_enable

   Description:   This enables a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Resetting the timer1 counter to 0.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Updated state machine timer1 counter, timer1 timeout, and timer1 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer1_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer1" bit in the SM input word, set timeout,
     and enable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1 = 0;
   local_workspace_ptr->timer1_timeout = timeout;
   local_workspace_ptr->timer1_timeout_backup = timeout;
   local_workspace_ptr->timer1_enable = TRUE;
#if (defined(FSM_DEBUG_VERBOSE))
   fprintf(stdout, "[fsm_timer1_enable] >>> timer1: (%d) %d/%d backup: %d\n", local_workspace_ptr->timer1_enable, (int)local_workspace_ptr->timer1, (int)local_workspace_ptr->timer1_timeout, (int)local_workspace_ptr->timer1_timeout_backup); fflush(stdout);
#endif
}

/****************************************************************************

   fsm_timer1_new_timeout

   Description:   This stores a new timeout for timer1 without restarting
   the timer by performing the following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Storing the value (in loops) in the state machine's workspace.
   3) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Note:  This may be used to enable timer 1 (i.e. it doesn't have to be called
   after timer1 is enabled).

   Input:
      uint32_t time:  New timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Updated state machine timer1 timeout, and timer1 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer1_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer1" bit in the SM input word, set new timeout,
     and enable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1_timeout = timeout;
   local_workspace_ptr->timer1_timeout_backup = timeout;
   local_workspace_ptr->timer1_enable = TRUE;
}

/****************************************************************************

   fsm_timer1_restart_with_timeout_and_old_time

   Description:   This enables a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Resetting the timer1 counter to the value given.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t restartvalue: timer value to start with
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Updated state machine timer1 counter, timer1 timeout, and timer1 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer1_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer1" bit in the SM input word, set new timeout,
     and enable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1_timeout = timeout;
   local_workspace_ptr->timer1_timeout_backup = timeout;
   local_workspace_ptr->timer1 = restartvalue;
   local_workspace_ptr->timer1_enable = TRUE;
}

/****************************************************************************

   fsm_timer1_disable

   Description:   This disables a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Setting the timer1 counter to 0.
   3) Setting the timer1 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Cleared state machine timer1 counter.
      Updated state machine timer1 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer1_disable(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer1" bit in the SM input word and disable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1 = 0;
   local_workspace_ptr->timer1_enable = FALSE;
}

/****************************************************************************

   fsm_timer1_stop (stops the timer withour reseting the time)

   Description:   This stops a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Setting the timer1 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Updated state machine timer1 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer1_stop(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer1" bit in the SM input word and disable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1_enable = FALSE;
}

/****************************************************************************

   fsm_timer1_start (starts the timer withour reseting the time)

   Description:   This starts a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer1 bit in the state machine's transition
      input word.
      Sets state machine timer1 enable member of the workspace to enable.

   Return:
      void

****************************************************************************/
void fsm_timer1_start(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer1" bit in the SM input word and enables timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER1_BIT;
   local_workspace_ptr->timer1_enable = TRUE;
}

/****************************************************************************

   fsm_timer1_get_elapsed

   Description:   This returns the current timer1 value.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  Elapsed time

****************************************************************************/
uint32_t fsm_timer1_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer1);
}

/****************************************************************************

   fsm_timer1_get_timeout

   Description:   This returns the current timer1 timeout.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  timeout time

****************************************************************************/
uint32_t fsm_timer1_get_timeout(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer1_timeout);
}

/****************************************************************************

   fsm_timer2_enable

   Description:   This enables a state machine's timer2 by performing the
   following:
   1) Clearing the state machine timer2 bit in the transition input word.
   2) Resetting the timer2 counter to 0.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer2 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Updated state machine timer2 counter, timer2 timeout, and timer2 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer2_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer2" bit in the SM input word, set timeout,
     and enable timer2 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2 = 0;
   local_workspace_ptr->timer2_timeout = timeout;
   local_workspace_ptr->timer2_timeout_backup = timeout;
   local_workspace_ptr->timer2_enable = TRUE;
}

/****************************************************************************

   fsm_timer2_new_timeout

   Description:   This stores a new timeout for timer2 without restarting
   the timer by performing the following:
   1) Clearing the state machine timer2 bit in the transition input word.
   2) Storing the value (in loops) in the state machine's workspace.
   3) Setting the timer2 enable flag to TRUE in the state machine's workspace.

   Note:  This may be used to enable timer 2 (i.e. it doesn't have to be called
   after timer2 is enabled).

   Input:
      uint32_t timeout:  New timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Updated state machine timer2 timeout, and timer2 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer2_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer2" bit in the SM input word, set new timeout,
     and enable timer2 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2_timeout = timeout;
   local_workspace_ptr->timer2_timeout_backup = timeout;
   local_workspace_ptr->timer2_enable = TRUE;
}

/****************************************************************************

   fsm_timer2_restart_with_timeout_and_old_time

   Description:   This enables a state machine's timer2 by performing the
   following:
   1) Clearing the state machine timer2 bit in the transition input word.
   2) Resetting the timer2 counter to the value given.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer2 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t restartvalue: timer value to start with
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Updated state machine timer2 counter, timer2 timeout, and timer2 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer2_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer1" bit in the SM input word, set new timeout,
     and enable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2_timeout = timeout;
   local_workspace_ptr->timer2_timeout_backup = timeout;
   local_workspace_ptr->timer2 = restartvalue;
   local_workspace_ptr->timer2_enable = TRUE;
}

/****************************************************************************

   fsm_timer2_disable

   Description:   This disables a state machine's timer2 by performing the
   following:
   1) Clearing the state machine timer2 bit in the transition input word.
   2) Setting the timer2 counter to 0.
   3) Setting the timer2 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Cleared state machine timer2 counter.
      Updated state machine timer2 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer2_disable(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer2" bit in the SM input word and disable timer2 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2 = 0;
   local_workspace_ptr->timer2_enable = FALSE;
}

/****************************************************************************

   fsm_timer2_stop (stops the timer withour reseting the time)

   Description:   This stops a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer2 bit in the transition input word.
   2) Setting the timer1 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Updated state machine timer2 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer2_stop(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer2" bit in the SM input word and disable timer2 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2_enable = FALSE;
}

/****************************************************************************

   fsm_timer2_start (starts the timer withour reseting the time)

   Description:   This starts a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer2 bit in the state machine's transition
      input word.
      Sets state machine timer2 enable member of the workspace to enable.

   Return:
      void

****************************************************************************/
void fsm_timer2_start(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer2" bit in the SM input word and enables timer2 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER2_BIT;
   local_workspace_ptr->timer2_enable = TRUE;
}

/****************************************************************************

   fsm_timer2_get_elapsed

   Description:   This returns the current timer2 value.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  Elapsed time

****************************************************************************/
uint32_t fsm_timer2_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer2);
}

/****************************************************************************

   fsm_timer2_get_timeout

   Description:   This returns the current timer2 timeout.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  timeout time

****************************************************************************/
uint32_t fsm_timer2_get_timeout(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer2_timeout);
}
/****************************************************************************

   fsm_timer3_enable

   Description:   This enables a state machine's timer3 by performing the
   following:
   1) Clearing the state machine timer3 bit in the transition input word.
   2) Resetting the timer3 counter to 0.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer3 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Updated state machine timer3 counter, timer3 timeout, and timer3 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer3_enable(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer3" bit in the SM input word, set timeout,
     and enable timer3 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3 = 0;
   local_workspace_ptr->timer3_timeout = timeout;
   local_workspace_ptr->timer3_timeout_backup = timeout;
   local_workspace_ptr->timer3_enable = TRUE;
}

/****************************************************************************

   fsm_timer3_new_timeout

   Description:   This stores a new timeout for timer3 without restarting
   the timer by performing the following:
   1) Clearing the state machine timer3 bit in the transition input word.
   2) Storing the value (in loops) in the state machine's workspace.
   3) Setting the timer3 enable flag to TRUE in the state machine's workspace.

   Note:  This may be used to enable timer 2 (i.e. it doesn't have to be called
   after timer3 is enabled).

   Input:
      uint32_t timeout:  New timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Updated state machine timer3 timeout, and timer3 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer3_new_timeout(uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer3" bit in the SM input word, set new timeout,
     and enable timer3 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3_timeout = timeout;
   local_workspace_ptr->timer3_timeout_backup = timeout;
   local_workspace_ptr->timer3_enable = TRUE;
}

/****************************************************************************

   fsm_timer3_restart_with_timeout_and_old_time

   Description:   This enables a state machine's timer3 by performing the
   following:
   1) Clearing the state machine timer3 bit in the transition input word.
   2) Resetting the timer3 counter to the value given.
   3) Storing the value (in loops) in the state machine's workspace.
   4) Setting the timer3 enable flag to TRUE in the state machine's workspace.

   Input:
      uint32_t restartvalue: timer value to start with
      uint32_t timeout:  Timeout in ticks
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Updated state machine timer3 counter, timer3 timeout, and timer3 enable
      members of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer3_restart_with_timeout_and_old_time(uint32_t restartvalue, uint32_t timeout, const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

  /* Clear the "timer1" bit in the SM input word, set new timeout,
     and enable timer1 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3_timeout = timeout;
   local_workspace_ptr->timer3_timeout_backup = timeout;
   local_workspace_ptr->timer3 = restartvalue;
   local_workspace_ptr->timer3_enable = TRUE;
}

/****************************************************************************

   fsm_timer3_disable

   Description:   This disables a state machine's timer3 by performing the
   following:
   1) Clearing the state machine timer3 bit in the transition input word.
   2) Setting the timer3 counter to 0.
   3) Setting the timer3 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Cleared state machine timer3 counter.
      Updated state machine timer3 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer3_disable(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer3" bit in the SM input word and disable timer3 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3 = 0;
   local_workspace_ptr->timer3_enable = FALSE;
}

/****************************************************************************

   fsm_timer3_stop (stops the timer withour reseting the time)

   Description:   This stops a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer3 bit in the transition input word.
   2) Setting the timer1 enable flag to FALSE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Updated state machine timer3 enable member of the workspace.

   Return:
      void

****************************************************************************/
void fsm_timer3_stop(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer3" bit in the SM input word and disable timer3 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3_enable = FALSE;
}

/****************************************************************************

   fsm_timer3_start (starts the timer withour reseting the time)

   Description:   This starts a state machine's timer1 by performing the
   following:
   1) Clearing the state machine timer1 bit in the transition input word.
   2) Setting the timer1 enable flag to TRUE in the state machine's workspace.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      Cleared state machine timer3 bit in the state machine's transition
      input word.
      Sets state machine timer3 enable member of the workspace to enable.

   Return:
      void

****************************************************************************/
void fsm_timer3_start(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   /* Clear the "timer3" bit in the SM input word and enables timer3 */
   *(fsm_config_ptr->trans_input_ptr) &= ~FSM_TIMER3_BIT;
   local_workspace_ptr->timer3_enable = TRUE;
}

/****************************************************************************

   fsm_timer3_get_elapsed

   Description:   This returns the current timer3 value.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  Elapsed time

****************************************************************************/
uint32_t fsm_timer3_get_elapsed(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer3);
}

/****************************************************************************

   fsm_timer3_get_timeout

   Description:   This returns the current timer3 timeout.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state machine configuration

   Output:

   Return:
      uint32_t:  timeout time

****************************************************************************/
uint32_t fsm_timer3_get_timeout(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr->timer3_timeout);
}

/****************************************************************************

   fsm_get_current_state

   Description:   This returns the current state.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      None.

   Return:
      uint8_t: Current state

****************************************************************************/
uint8_t fsm_get_current_state(const FSM_CONFIG_T *fsm_config_ptr)
{
   return(fsm_config_ptr->workspace_ptr)->current_state;
}

/****************************************************************************

   fsm_clear_trans_history

   Description:   This clears the last input word value and the last
   condition mask value to allow the next transition check to cause
   a transition if the check is true.  This basically overrides the
   race condition protection.

   Input:
      const FSM_CONFIG_T *fsm_work_ptr:  Pointer to a state
      machine configuration

   Output:
      None.

   Return:
      None.

****************************************************************************/
void fsm_clear_trans_history(const FSM_CONFIG_T *fsm_config_ptr)
{
   FSM_WORKSPACE_T *local_workspace_ptr;

   /* Init local workspace pointer */
   local_workspace_ptr = fsm_config_ptr->workspace_ptr;

   local_workspace_ptr->last_input_word = RESERVED_INPUT_WORD_VALUE;
   local_workspace_ptr->last_event_trans_cond_mask = RESERVED_INPUT_WORD_VALUE;
}


/** @} */
/***************************************************************************************************
 *
 *  MODIFICATIONS:
 *
 *  Refer to PVCS for revision history prior to migration to RTC; in RTC use Show History.
 *
***************************************************************************************************/
