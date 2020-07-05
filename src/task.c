/***************************************************************************************************
 *  @file app_task.c
 *    This is the task function file of my webtool program
 *
 *  @author:     Ying Xiong
 *  @created:    May, 2020
 ***************************************************************************************************/

#include <string.h>
#include "private.h"
#include "cloud.h"
#include "parse.h"
#include "utils.h"
#include "task.h"

//
// Local Defines
//
#ifdef WEBALIVE
#define TASK_SEND_DELAY  60
#elif WEBPOLL
#define TASK_SEND_DELAY  10
#elif WEBPING
#define TASK_SEND_DELAY  1
#endif
#define TASK_SEND_LIMIT  3
#define TASK_SEND_TIMER  5

//
// Local Variables
//
static bool task_completed = false;
static int send_errors = 0;
static int send_len = 0;
#ifndef WEBGET
static uint32_t timer_start;
static uint32_t timer_count;
#endif
static char device_name[DEVICE_NAME_LEN];
static char device_addr[DEVICE_ADDR_LEN];
static char target_file[TARGET_FILE_LEN];
static char server_name[SERVER_NAME_LEN];
static int server_port;

static char cloudSendBuf[CLOUD_SEND_BUF_LEN];
static char cloudRecvBuf[CLOUD_RECV_BUF_LEN];

static CLOUD_DIAGS_T sendDiags;
static CLOUD_SESSION_T sendSession =
{
   "",
   CLOUD_INVALID_SOCKET,
   CLOUD_SESSION_IDLE,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   &cloudSendBuf[0],
   &cloudRecvBuf[0],
   sizeof(cloudRecvBuf),
   false,
   false,
   &sendDiags
};

//
// Global Variables
//
int fsm_state = FSM_INIT_STATE;
SEND_STATUS_T send_status = SEND_NOT_READY;

bool initialized = false;
bool data_sending = false;

#ifdef DOWNLOAD
//!
//! Save the received data to a local file
//!
static void saveReceivedDataToFile(char *dataBuf)
{
   char* start;
   int length;
   FILE* fp;

   start = parse_getContentStart(dataBuf);
   length = parse_getContentLength(dataBuf);
   if ((start != NULL) && (length > 0))
   {
      remove(target_file);
      fp = fopen(target_file, "w+");
      if (fp != NULL)
      {
         fwrite(start, 1, length, fp);
         fclose(fp);
         utils_sysLog(LOG_INFO, "Saved %d bytes to local file '%s'", length, target_file);
      }
   }
}
#endif

//!
//! Assamble HTTP buffer to send
//!
static int assambleSendBuffer(char *msgBuf)
{
   char *tailPtr;

   tailPtr = msgBuf;
   tailPtr += sprintf(tailPtr, "GET /%s HTTP/1.1\r\n", target_file);
   tailPtr += sprintf(tailPtr, "Host: %s\r\n", server_name);
   tailPtr += sprintf(tailPtr, "Device-Name: \"%s\"\r\n", device_name);
   tailPtr += sprintf(tailPtr, "Device-MAC: \"%s\"\r\n", device_addr);
   tailPtr += sprintf(tailPtr, "Connection: keep-alive\r\n");
   tailPtr += sprintf(tailPtr, "Pragma: no-cache\r\n");
   tailPtr += sprintf(tailPtr, "Cache-Control: no-cache\r\n");
   tailPtr += sprintf(tailPtr, "Content-Type: application/x-www-form-urlencoded\r\n");
   tailPtr += sprintf(tailPtr, "Content-Length: 0\r\n");
   tailPtr += sprintf(tailPtr, "\r\n\r\n");
   *tailPtr = '\0';

   return strlen(msgBuf);
}

//!
//! Set send status
//!
static void setSendStatus(SEND_STATUS_T status)
{
   if (send_status != status)
   {
      utils_sysLog(LOG_DEBUG, "Send status changed %d -> %d\n", send_status, status);
      send_status = status;
   }
}

//!
//! Set task state
//!
static void setState(int state)
{
   if (fsm_state != state)
   {
      utils_sysLog(LOG_DEBUG, "Task state changed %d -> %d\n", fsm_state, state);
      fsm_state = state;
   }
}

//!
//! Entry function to INIT state
//!
void initEntry(void)
{
   setState(FSM_INIT_STATE);
   if (initialized)
   {
      initialized = false;
   }
}

//!
//! Activity function in INIT state
//!
void initActivity(void)
{
   if (!initialized)
   {
      if (0 == strlen(device_name))
      {
         strcpy(device_name, DEVICE_NAME_DEF);
      }
      sendSession.name = device_name;
      if (0 == strlen(device_addr))
      {
         strcpy(device_addr, DEVICE_ADDR_DEF);
      }
      if (0 == strlen(target_file))
      {
         strcpy(target_file, TARGET_FILE_DEF);
      }
      if (0 == strlen(server_name))
      {
         strcpy(server_name, SERVER_NAME_DEF);
      }
      server_port = CLOUD_TCP_PORT_HTTP;
      utils_sysLog(LOG_INFO, "Server : %s\n", server_name);
      utils_sysLog(LOG_INFO, "Device : %s\n", device_addr);
#ifdef DOWNLOAD
      utils_sysLog(LOG_INFO, "Target : %s\n", target_file);
#endif
#ifndef WEBGET
      timer_count = 0;
#endif
      initialized = true;
   }
}

//!
//! Entry function to IDLE state
//!
void idleEntry(void)
{
   setState(FSM_IDLE_STATE);
   if (data_sending)
   {
      data_sending = false;
   }
}

//!
//! Activity function in IDLE state
//!
void idleActivity(void)
{
#ifndef WEBGET
   if ((timer_count == 0) || utils_isTimerExpired(timer_start, TASK_SEND_DELAY))
   {
      data_sending = true;
      timer_count++;
      timer_start = utils_getCurrentTime();
   }
#else
   data_sending = true;
#endif
}

//!
//! Entry function to SEND state
//!
void sendEntry(void)
{
   setState(FSM_SEND_STATE);
   setSendStatus(SEND_NOT_READY);
}

//!
//! Activity function in SEND state
//!
void sendActivity(void)
{
   CLOUD_SESSION_T *s = &sendSession;

   /* Run Send sub-state FSM */
   switch (send_status)
   {
      case SEND_NOT_READY:
         if (cloud_initSession(&sendSession, server_name, server_port))
         {
            setSendStatus(SEND_STARTING);
         }
         else
         {
            data_sending = false;
#ifdef WEBPING
            printf("ECHO from %s failed\n", server_name);
#else
            utils_sysLog(LOG_INFO, "Failed to initialize session\n");
#endif
         }
         break;
      case SEND_STARTING:
         send_len = assambleSendBuffer(s->sendBuf);
         utils_sysLog(LOG_DEBUG, "Total %d bytes to send\n", send_len);
         s->totalBytesToSend = send_len;
         setSendStatus(SEND_STARTED);
         break;
      case SEND_STARTED:
         cloud_sessionConnectAndSend(s);
         if (0 == s->errorCode)
         {
            setSendStatus(SEND_CONTINUE);
         }
         else
         {
            data_sending = false;
#ifdef WEBPING
            printf("ECHO from %s failed\n", server_name);
#else
            utils_sysLog(LOG_INFO, "Failed to create connection\n");
#endif
         }
         break;
      case SEND_CONTINUE:
         if (cloud_sessionSendRecvAll(s, TASK_SEND_TIMER))
         {
            data_sending = false;
            if (HTTP_BAD_REQUEST > sendSession.httpStatus)
            {
               setSendStatus(SEND_COMPLETED);
            }
            else
            {
               utils_sysLog(LOG_INFO, "Received http status %d\n", sendSession.httpStatus);
            }
         }
         break;
      default:
         break;
   }
}

//!
//! Exit function from SEND state
//!
void sendExit(void)
{
   if (SEND_COMPLETED == send_status)
   {
      send_errors = 0;
#ifdef DOWNLOAD
      saveReceivedDataToFile(sendSession.recvBuf);
#endif
#ifdef WEBGET
      task_completed = true;
      printf("Program exited successfully\n");
#elif WEBALIVE
      utils_sysLog(LOG_INFO, "HTTP server %s is alive\n", server_name);
#elif WEBPING
      printf("ECHO from %s, count %u\n", server_name, timer_count);
#endif
   }
   else
   {
      send_errors++;
      utils_sysLog(LOG_DEBUG, "Send session failures %d\n", send_errors);
#ifdef WEBGET
      if ((send_errors >= TASK_SEND_LIMIT) || (SEND_CONTINUE != send_status))
      {
         task_completed = true;
         printf("Program exited unexpectedly\n");
      }
#else
      if (send_errors >= TASK_SEND_LIMIT)
      {
         send_errors = 0;
         initialized = false;
         if (SEND_CONTINUE == send_status)
         {
#ifdef WEBPING
            printf("ECHO from %s failed\n", server_name);
#else
            utils_sysLog(LOG_INFO, "Failed to receive response\n");
#endif
         }
      }
#endif /* WEBGET */
   }
   cloud_closeSession(&sendSession);
}

//!
//! Reset Cloud session status
//!
void resetSessionStatus(void)
{
	cloud_resetSessionStatus(&sendSession);
}

//!
//! Check if any sesssion error happened
//!
bool checkSessionError(void)
{
   CLOUD_SESSION_T *s = &sendSession;

   return (s->timeout || s->recvComplete || (0 != s->errorCode));
}

//!
//! Get task completed flag
//!
bool get_task_completed(void)
{
   return task_completed;
}

//!
//! Set server name (URL or IP address)
//!
void set_server_name(char *name)
{
   if (strlen(name) < SERVER_NAME_LEN)
   {
      strcpy(server_name, name);
   }
}

//!
//! Set targted file name
//!
void set_target_file(char *file)
{
   if (strlen(file) < TARGET_FILE_LEN)
   {
      strcpy(target_file, file);
   }
}

//!
//! Set client device MAC address
//!
void set_device_addr(char *addr)
{
   if (strlen(addr) < DEVICE_ADDR_LEN)
   {
      strcpy(device_addr, addr);
   }
}

//!
//! Set client device name
//!
void set_device_name(char *name)
{
   if (strlen(name) < DEVICE_NAME_LEN)
   {
      strcpy(device_name, name);
   }
}
