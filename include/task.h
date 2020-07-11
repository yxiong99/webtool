#ifndef _TASK_H_
#define _TASK_H_

#include <stdbool.h>

#define SERVER_NAME_DEF  "192.168.112.1"
#ifdef WEBPOLL
#define TARGET_FILE_DEF  "device.conf"
#elif WEBGET
#define TARGET_FILE_DEF  "hello.txt"
#else
#define TARGET_FILE_DEF  "alive"
#endif
#define DEVICE_ADDR_DEF  "00:00:00:00:00:00"
#define DEVICE_NAME_DEF  "anonymous"

#define SERVER_NAME_LEN  128
#define TARGET_FILE_LEN  64
#define DEVICE_NAME_LEN  32
#define DEVICE_ADDR_LEN  18

//
// Send Status
//
typedef enum
{
   SEND_NOT_READY,
   SEND_STARTING,
   SEND_STARTED,
   SEND_CONTINUE,
   SEND_COMPLETED
}
SEND_STATUS_T;

//
// Function Prototypes
//
void initEntry(void);
void initActivity(void);
void idleEntry(void);
void idleActivity(void);
void sendEntry(void);
void sendActivity(void);
void sendExit(void);

void resetSessionStatus(void);
bool checkSessionError(void);

bool get_task_completed(void);
void set_server_name(char *name);
void set_target_file(char *file);
void set_device_addr(char *addr);
void set_device_name(char *name);

#endif /* _TASK_H_ */
