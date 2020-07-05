//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: Nov 2019
//!
//******************************************************************************

#include "private.h"
#include "task.h"

//
// Local Constants
//
#define CLR_NON_TIMER_BITS  0xE0000000

static const uint32_t bitmasks_32[32] =
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
// DataLog Input Macros
//
#define set_bit(mask)  SET_INP_BIT(fsm_trans_input, (mask))
#define clr_bit(mask)  CLR_INP_BIT(fsm_trans_input, (mask))
#define clr_all_bits() fsm_trans_input &= CLR_NON_TIMER_BITS

//
// Global Input Variables
//
uint32_t fsm_trans_input;

//
// Bitmask defines for input variable
//
#define INITIALIZED        bitmasks_32[0]
#define DATA_SENDING       bitmasks_32[1]
#define SEND_RECOVER       bitmasks_32[2]
#define RUN_CONDITION      bitmasks_32[3]

//!
//! Write the Datalog bit to either 0 or 1 based on the condition.
//!
//! @param[in] mask - Bit mask to set or clear
//! @param[in] condition - If true, set the bit, otherwise clear it
//!
static void write_bit(uint32_t mask, bool condition)
{
   if (condition)
   {
      set_bit(mask);
   }
   else
   {
      clr_bit(mask);
   }
}

//!
//! Check the condition to run the process
//!
static void check_run_condition(void)
{
   /* Internet connectivity (assume always on) */
   write_bit(RUN_CONDITION, true);
}

//!
//! Check on-going message sending
//!
static void check_send_condition(void)
{
   write_bit(DATA_SENDING, data_sending);
}

//!
//! Check the condition to reinitialize
//!
static void check_init_condition(void)
{
   write_bit(INITIALIZED, initialized);
}

//!
//! Function to set and clear FSM input bits
//!
void fsm_process_inputs(void)
{
   clr_all_bits();
   check_init_condition();
   check_send_condition();
   if (FSM_SEND_STATE == fsm_state)
   {
      write_bit(SEND_RECOVER, checkSessionError());
   }
   check_run_condition();
}
