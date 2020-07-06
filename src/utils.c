//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: May 2020
//!
//******************************************************************************

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include "utils.h"

#define MSG_BUFF_SIZE  1023

//
// Local Variables
//
static int syslog_level = LOG_INFO;

//!
//! Get the cuurent system time in seconds
//!
uint32_t utils_getCurrentTime(void)
{
   struct timeval curr_time;

   gettimeofday(&curr_time, NULL);
   return (uint32_t)(curr_time.tv_sec);
}

//!
//! Check the log level and if okay, send it to syslog
//!
void utils_sysLog(int level, const char* fmt, ... )
{
    int rc;
    va_list ap;
    char msg[MSG_BUFF_SIZE + 1];

    if (level <= syslog_level)
    {
        va_start(ap, fmt);
        rc = vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        if (rc >= 0)
        {
            syslog(level, "%s\n", msg);
        }
    }
}

//!
//! Check if the poll timer has expired
//!
bool utils_isTimerExpired(uint32_t start_time, uint32_t delta_time)
{
   bool expired = false;

   if ((utils_getCurrentTime() - start_time) >= delta_time)
   {
      expired = true;
   }
   return expired;
}
