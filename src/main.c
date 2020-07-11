/***************************************************************************************************
 *  @file main.c
 *    This is the main entry of my webtool program
 *
 *  @author:     Ying Xiong
 *  @created:    May, 2020
 ***************************************************************************************************/

#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <rhapsody.h>
#include "private.h"
#include "utils.h"
#include "task.h"

extern void fsm_init(const FSM_CONFIG_T *fsm_config_ptr);
extern void fsm_process(const FSM_CONFIG_T *fsm_config_ptr);

volatile static int loop_done;

//!
//! Handle interrupt signals
//!
static void signal_handler(int signum)
{
   loop_done = 1;
}

//!
//! Finite state machine loop
//!
static void* fsm_loop(void* arg)
{
   loop_done = 0;
   signal(SIGINT,  &signal_handler);
   signal(SIGQUIT, &signal_handler);
   signal(SIGTERM, &signal_handler);

   fsm_init(&fsm_config);
   while (!loop_done)
   {
      fsm_process(&fsm_config);
      if (get_task_completed())
      {
         break;
      }
      sleep(FSM_LOOP_DELAY);
   }
   pthread_exit(NULL);
}

//!
//! Initialize default params
//!
static void init(void)
{
   set_server_name("");
   set_target_file("");
   set_device_addr("");
   set_device_name("");
}

//!
//! Display program options
//!
static void usage(char *arg)
{
#ifdef DOWNLOAD
   printf("Usage: %s [-h] [-f <>] [-i <>] [-m <>] [-s <>]\n", arg);
#else
   printf("Usage: %s [-h] [-i <>] [-m <>] [-s <>]\n", arg);
#endif
   printf("  -h  display this usage\n");
#ifdef DOWNLOAD
   printf("  -f  <target file name>\n");
#endif
   printf("  -i  <device identifier>\n");
   printf("  -m  <device MAC address>\n");
   printf("  -s  <server URL or IP address>\n");
}

//!
//! Main function
//!
int main(int argc, char* argv[])
{
   struct sched_param schedpa;
   pthread_attr_t attr;
   pthread_t thread;
   void* status;
   int c;

   init();
   for (;;)
   {
#ifdef DOWNLOAD
      c = getopt(argc, argv, "hf:i:m:s:");
#else
      c = getopt(argc, argv, "hi:m:s:");
#endif
      if (c < 0)
      {
         break;
      }
      switch (c)
      {
         case 'h':
            usage(argv[0]);
            return -1;
#ifdef DOWNLOAD
         case 'f':
            set_target_file(optarg);
            break;
#endif
         case 'i':
            set_device_name(optarg);
            break;
         case 'm':
            set_device_addr(optarg);
            break;
         case 's':
            set_server_name(optarg);
            break;
         default:
            usage(argv[0]);
            return -1;
		}
	}
   if (optind < argc)
   {
      usage(argv[0]);
      return -1;
   }
#ifdef WEBALIVE
   utils_sysLog(LOG_INFO, "----- HTTP echo alive from a web server -----\n");
#elif WEBPOLL
   utils_sysLog(LOG_INFO, "----- HTTP poll a file from a web server -----\n");
#elif WEBPING
   utils_sysLog(LOG_INFO, "----- HTTP echo ping from a web server -----\n");
#else
   utils_sysLog(LOG_INFO, "----- HTTP get a file from a web server -----\n");
#endif
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, fsm_loop, NULL);
   schedpa.sched_priority = 1;
   pthread_setschedparam(thread, SCHED_RR, &schedpa);
   pthread_attr_destroy(&attr);
   pthread_join(thread, &status);

   return 0;
}
