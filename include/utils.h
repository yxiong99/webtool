#ifndef _UTILS_H_
#define _UTILS_H_

#include <netdb.h>
#include <syslog.h>

//
// Function Prototypes
//
uint32_t utils_getCurrentTime(void);
void utils_sysLog(int level, const char* fmt, ... );
bool utils_isTimerExpired(uint32_t start_time, uint32_t delta_time);

#endif /* _UTILS_H_ */
