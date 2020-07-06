//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: May 2020
//!
//******************************************************************************

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "cloud.h"
#include "parse.h"
#include "utils.h"

//
// Local Defines
//
#define SEND_TIMEOUT_MS   (1000)
#define RECV_TIMEOUT_MS   (1000)
#define CONN_TIMEOUT_MS   (5000)

//
// Local Variables
//
static CLOUD_SOCKADDR_T serverIpAddr;

//
// Local Function Prototypes
//
static bool cloud_isSocketReady(CLOUD_SESSION_T *s);
static void cloud_setSessionStatus(CLOUD_SESSION_T *s, CLOUD_SESSION_STATUS_T status);
static void cloud_setSocketError(CLOUD_SESSION_T *s, int errCode);
static void cloud_startSessionAttempt(CLOUD_SESSION_T *s);
static bool cloud_packetIsSuccessful(CLOUD_SESSION_T *s);

//!
//! Handle a socket error.
//!
static void cloud_handleSocketError(CLOUD_SESSION_T *s, int errCode)
{
   cloud_setSocketError(s, errCode);
   close(s->handle);
   s->handle = CLOUD_INVALID_SOCKET;
}

//!
//! Cloud connect.
//!
static void cloud_startConnect(CLOUD_SESSION_T *s)
{
   ssize_t retVal;

   retVal = connect(s->handle, (struct sockaddr *)&serverIpAddr, sizeof(CLOUD_SOCKADDR_T));
   if (retVal <= 0)
   {
      if ((EINPROGRESS == errno) || (EWOULDBLOCK == errno))
      {
         cloud_setSessionStatus(s, CLOUD_SESSION_CONNECT_PENDING);
      }
      else if (retVal == 0)
      {
         cloud_setSessionStatus(s, CLOUD_SESSION_CONNECT_SUCCESS);
      }
      else
      {
         utils_sysLog(LOG_ERR, "%s>> connect errno: %s\n", s->name, strerror(errno));
         cloud_handleSocketError(s, errno);
      }
   }
}

//!
//! Determine if the socket is ready.
//!
static bool cloud_isSocketReady(CLOUD_SESSION_T *s)
{
   ssize_t retVal;
   socklen_t optLen;
   int optVal;

   optVal = -1;
   optLen = sizeof (optVal);
   retVal = getsockopt(s->handle, SOL_SOCKET, SO_ERROR, &optVal, &optLen);
   return ((retVal == 0) && (optVal == 0));
}

//!
//! Cloud finish the session connect.
//!
static void cloud_finishConnect(CLOUD_SESSION_T *s)
{
   ssize_t retVal;
   fd_set writefds;
   struct timeval tv;

   FD_ZERO(&writefds);
   FD_SET(s->handle, &writefds);
   tv.tv_sec = CONN_TIMEOUT_MS / 1000;
   tv.tv_usec = 0;
   retVal = select(s->handle+1, NULL, &writefds, NULL, &tv);
   if (retVal >= 0)
   {
      if (FD_ISSET(s->handle, &writefds))
      {
         if (cloud_isSocketReady(s))
         {
            cloud_setSessionStatus(s, CLOUD_SESSION_CONNECT_SUCCESS);
         }
         else
         {
            cloud_setSessionStatus(s, CLOUD_SESSION_CONNECT_PENDING);
         }
      }
      else
      {
         utils_sysLog(LOG_ERR, "%s>> connect FD_ISSET errno: %s\n", s->name, strerror(errno));
         cloud_handleSocketError(s, errno);
      }
   }
   else
   {
      utils_sysLog(LOG_ERR, "%s>> connect select errno: %s\n", s->name, strerror(errno));
      cloud_handleSocketError(s, errno);
   }
}

//!
//! Wrapper function to perform a connect on a Cloud socket for non-blocking
//! operations. After connect() is called, a short blocking call to select() is
//! done to wait for the socket to be ready for write operations.
//!
//! @param[in] s socket handle number
//!
void cloud_sessionConnect(CLOUD_SESSION_T *s)
{
   ssize_t retVal;

   if ((CLOUD_SESSION_IDLE != s->status) &&
       (CLOUD_SESSION_CREATE_SUCCESS != s->status) &&
       (CLOUD_SESSION_CONNECT_PENDING != s->status))
   {
      /*
       * Do not perform a send operation if the socket is not IDLE, FAILED,
       * CREATED or connect in progress.
       */
      return;
   }
   /* Initiate the session connection */
   if (CLOUD_SESSION_CONNECT_PENDING != s->status)
   {
      cloud_startConnect(s);
   }
   if ((CLOUD_SESSION_CONNECT_PENDING == s->status) ||
       (CLOUD_SESSION_CONNECT_SUCCESS == s->status))
   {
      cloud_finishConnect(s);
   }
}

//!
//! Wrapper function to send data on a socket for non-blocking socket operations.
//! Check whether the socket is ready to send data using a select() call then
//! send the data through the socket.
//!
//! This function returns CALS_FAIL if the socket is not ready for send or if an
//! error occurs during the send operation.  If no error occured, the number of
//! bytes sent is returned.
//!
//! NOTE:  It is possible that not all bytes have been sent out through the
//!        socket! The caller is responsible to make successive calls to this
//!        function to send the remaining data out.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
void cloud_sessionSend(CLOUD_SESSION_T *s)
{
   ssize_t retVal;
   struct  timeval tv;
   fd_set  writefds;
   size_t  start_index;
   int32_t remain_bytes_to_send;

   if ((CLOUD_SESSION_IDLE != s->status) &&
       (CLOUD_SESSION_FAILED != s->status) &&
       (CLOUD_SESSION_CONNECT_SUCCESS != s->status) &&
       (CLOUD_SESSION_SEND_PENDING != s->status))
   {
      /* 
       * Do not perform a send operation if the socket is not IDLE, FAILED,
       * connect success, or send in progress.
       */
      return;
   }
   if (CLOUD_SESSION_SEND_PENDING != s->status)
   {
      cloud_setSessionStatus(s, CLOUD_SESSION_SEND_PENDING);
      s->totalBytesSent = 0;
   }
   FD_ZERO(&writefds);
   FD_SET(s->handle, &writefds);
   tv.tv_sec = SEND_TIMEOUT_MS / 1000;
   tv.tv_usec = 0;
   retVal = select(s->handle+1, NULL, &writefds, NULL, &tv);
   if (retVal >= 0)
   {
      if (FD_ISSET(s->handle, &writefds))
      {
         start_index = s->totalBytesSent;
         remain_bytes_to_send = s->totalBytesToSend - s->totalBytesSent;
         if (remain_bytes_to_send < 0)
         {
            return;
         }
         retVal = send(s->handle, &(s->sendBuf[start_index]), remain_bytes_to_send, 0);
         if (CLOUD_SOCKET_ERROR == retVal)
         {
            if ((EINPROGRESS != errno) && (EWOULDBLOCK != errno))
            {
               utils_sysLog(LOG_ERR, "%s>> send failed errno: %s\n", s->name, strerror(errno));
               cloud_handleSocketError(s, errno);
            }
         }
         else
         {
            s->totalBytesSent += retVal;
            if (s->totalBytesSent == s->totalBytesToSend)
            {
               cloud_setSessionStatus(s, CLOUD_SESSION_SEND_SUCCESS);
               s->totalBytesSent = 0;
            }
         }
      }
      else
      {
         utils_sysLog(LOG_ERR, "%s>> send FD_ISSET errno: %s\n", s->name, strerror(errno));
      }
   }
   else
   {
      utils_sysLog(LOG_ERR, "%s>> send select errno: %s\n", s->name, strerror(errno));
   }
}

//!
//! Parse the receive buffer and do one of the following:
//!
//! A) If header contains content-length:
//!    Compare the size of the body to the number of bytes specified by
//!    content-length.
//!    If the size matches, true is returned.  Otherwise false is returned.
//! B) If header contains transfer-encoding: chunked.
//!    Parse the body of the message and search for a chunk of size 0,
//!    indicating the end of the packet. If a chunk size of 0 is found,
//!    true is returned. Otherwise, false is returned.
//! C) Buffer does not contain a full header yet (cannot find \r\n\r\n
//!    termination)
//!    Return false.
//! D) Buffer contains none of these in the header:
//!    Set the session as failed, we can not accept it..
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
static bool cloud_recvComplete(CLOUD_SESSION_T *s)
{
   bool complete = false;

   if (parse_fullHeaderFound(s->recvBuf))
   {
      s->httpStatus = parse_getStatusCode(s->recvBuf);
      s->diags->lastHttpStatus = s->httpStatus;
      if (parse_transferEncodingChunkedFound(s->recvBuf))
      {
         complete = parse_fullDataChunkFound(s->recvBuf);
      }
      else if (parse_contentLengthFound(s->recvBuf))
      {
         complete = parse_entireContentReceived(s->recvBuf, s->totalBytesRcvd);
      }
      else
      {
         complete = true;
      }
   }

   return complete;
}

//!
//! Wrapper function to receive data on a socket for non-blocking socket
//! operations.
//!
//! Check whether the socket has any pending data to read using a select()
//! call with a short timeout. If the socket is ready, read the data from
//! the socket.
//!
//! This function returns CALS_FAIL if he socket is not ready for receive
//! operations or if an error occured during the receive call. Otherwise,
//! the number of bytes received is returned.
//!
//! Note1: A 0 return indicates that the socket has been closed remotely.
//! Note2: It is possible that receive returns a number of bytes that does
//!        not contain the complete packet.
//!        The caller to this function is responsible to make the successive
//!        calls to read the remainder of the data.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
static void cloud_sessionRecv(CLOUD_SESSION_T *s)
{
   ssize_t retVal;
   struct timeval tv;
   fd_set readfds;

   if ((CLOUD_SESSION_IDLE != s->status) &&
       (CLOUD_SESSION_FAILED != s->status) &&
       (CLOUD_SESSION_SEND_SUCCESS != s->status) &&
       (CLOUD_SESSION_RECV_PENDING != s->status))
   {
      /*
       * Do not perform a recv operation if the socket is not IDLE, FAILED,
       * send success or recv in progress.
       */
      return;
   }
   if (CLOUD_SESSION_RECV_PENDING != s->status)
   {
      cloud_setSessionStatus(s, CLOUD_SESSION_RECV_PENDING);
      s->totalBytesRcvd = 0;
      memset(s->recvBuf, 0, s->recvBufLen);
   }
   FD_ZERO(&readfds);
   FD_SET(s->handle, &readfds);
   tv.tv_sec = RECV_TIMEOUT_MS / 1000;
   tv.tv_usec = 0;
   retVal = select(s->handle+1, &readfds, NULL, NULL, &tv);
   if (retVal >= 0)
   {
      if (FD_ISSET(s->handle, &readfds))
      {
         retVal = recv(s->handle, &(s->recvBuf[s->totalBytesRcvd]), s->recvBufLen, 0);
         if (CLOUD_SOCKET_ERROR == retVal)
         {
            if ((EINPROGRESS != errno) && (EWOULDBLOCK != errno))
            {
               utils_sysLog(LOG_ERR, "%s>> receive errno: %d\n", s->name, strerror(errno));
               cloud_handleSocketError(s, errno);
            }
         }
         else if (0 == retVal)
         {
            s->recvComplete = true;
            utils_sysLog(LOG_INFO, "%s>> server closed socket\n", s->name);
         }
         else
         {
            s->totalBytesRcvd += retVal;
            if (cloud_recvComplete(s))
            {
               s->recvComplete = true;
               if (cloud_packetIsSuccessful(s))
               {
                  cloud_setSessionStatus(s, CLOUD_SESSION_RECV_SUCCESS);
               }
            }
         }
      }
      else
      {
         utils_sysLog(LOG_ERR, "%s>> recv FD_ISSET error: %s\n", s->name, strerror(errno));
      }
   }
   else
   {
      utils_sysLog(LOG_ERR, "%s>> recv select errno %s\n", s->name, strerror(errno));
   }
}

//!
//! Extract the HTTP status from the receive buffer and check whether it is
//! a successful response (2XX status)
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
static bool cloud_packetIsSuccessful(CLOUD_SESSION_T *s)
{
   bool retval = parse_goodStatusCode(s->httpStatus);
   return retval;
}

//!
//! Start a new session attempt.
//!
//! This gets the session ready for a new operation, but maintains the
//! error counts.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
static void cloud_startSessionAttempt(CLOUD_SESSION_T *s)
{
   cloud_setSessionStatus(s, CLOUD_SESSION_IDLE);
   s->totalBytesRcvd = 0;
   s->totalBytesSent = 0;
}

//!
//! Set the Cloud session status.
//!
//! @param[out] s  Pointer to session structure
//! @param[in] status  Current session status
//!
static void cloud_setSessionStatus(CLOUD_SESSION_T *s, CLOUD_SESSION_STATUS_T status)
{
   if (s->status != status)
   {
      utils_sysLog(LOG_DEBUG, "%s>> session status changed %d -> %d\n", s->name, s->status, status);
      s->status = status;
   }
}

//!
//! Set an error code on a given Cloud socket and increment the error count.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//! @param[in] err error code
//!
static void cloud_setSocketError(CLOUD_SESSION_T *s, int errCode)
{
   utils_sysLog(LOG_DEBUG, "%s>> socket error code %d\n", s->name, errCode);
   s->errorCode = errCode;
   s->diags->sockFailures++;
   s->errorTime = utils_getCurrentTime();
   s->totalBytesRcvd = 0;
   s->totalBytesSent = 0;
   cloud_setSessionStatus(s, CLOUD_SESSION_FAILED);
}

//!
//! Reset the full status of a given session.
//!
//! Typically called in between operations (states) to reset the session
//! to idle, clear error counts and number of bytes sent/received.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//!
void cloud_resetSessionStatus(CLOUD_SESSION_T *s)
{
   cloud_setSessionStatus(s, CLOUD_SESSION_IDLE);
   s->errorCode = 0;
   s->totalBytesRcvd = 0;
   s->totalBytesSent = 0;
   s->httpStatus = 0;
   s->recvComplete = false;
   s->timeout = false;
}

//!
//! Resolve DNS and open a socket to a server.
//!
//! This routine will only initialize the session if not already active.
//!
//! @param[in] s  Pointer to session structure
//! @param[in] serverName  Pointer to server name string
//! @param[in] serverPort  Server port number
//!
//! @return  true if session successfully created, otherwise false
//!
bool cloud_initSession(CLOUD_SESSION_T *s, char *serverName, uint16_t serverPort)
{
   const int recvSockTimeOut = 5000;
   struct in_addr host_addr;
   struct in_addr* p_addr;
   struct hostent* p_host;
   bool success = false;
   /*
    * Start the transaction timer here, as sometimes the socket is already active,
    * and we want to restart the timer with every new transaction.
    */
   s->transStart = utils_getCurrentTime();
   s->diags->attempts++;
   /* Don't do anything if the session is already active. */
   if (CLOUD_INVALID_SOCKET != s->handle)
   {
      utils_sysLog(LOG_DEBUG, "%s>> session already active\n", s->name);
      return true;
   }
   s->handle = CLOUD_INVALID_SOCKET;
   if (0 == strlen(serverName))
   {
      utils_sysLog(LOG_ERR, "%s>> empty server name string\n", s->name);
      success = false;
   }
   else
   {
      host_addr.s_addr = inet_addr(serverName);
      if (host_addr.s_addr == INADDR_NONE)
      {
         utils_sysLog(LOG_DEBUG, "%s>> url: %s\n", s->name, serverName);
         memset(&serverIpAddr, 0, sizeof(CLOUD_SOCKADDR_T));
         p_host = gethostbyname(serverName);
         if (NULL != p_host)
         {
            char *addr = inet_ntoa(*((struct in_addr *)(p_host->h_addr)));
            if (NULL != addr)
            {
               serverIpAddr.sin_family = AF_INET;
               serverIpAddr.sin_addr = *((struct in_addr *)(p_host->h_addr));
               serverIpAddr.sin_port = htons(serverPort);
            }
         }
      }
      else
      {
         serverIpAddr.sin_family = AF_INET;
         serverIpAddr.sin_addr = host_addr;
         serverIpAddr.sin_port = htons(serverPort);
      }
      if (0 != serverIpAddr.sin_addr.s_addr)
      {
         p_addr = (struct in_addr *)&serverIpAddr.sin_addr;
         utils_sysLog(LOG_DEBUG, "%s>> ip: %s\n", s->name, inet_ntoa(*p_addr));
         utils_sysLog(LOG_DEBUG, "%s>> port: %d\n", s->name, ntohs(serverIpAddr.sin_port));
         s->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
         setsockopt(s->handle, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvSockTimeOut, sizeof(int));
         if (CLOUD_INVALID_SOCKET != s->handle)
         {
            cloud_setSessionStatus(s, CLOUD_SESSION_CREATE_SUCCESS);
            success = true;
         }
         else
         {
            utils_sysLog(LOG_ERR, "%s>> init session errno: %s\n", s->name, strerror(errno));
            cloud_handleSocketError(s, errno);
            success = false;
         }
      }
      else
      {
         utils_sysLog(LOG_ERR, "%s>> URL resolve failed\n", s->name);
         success = false;
      }
   }

   return success;
}

//!
//! Close a Cloud Session.
//!
void cloud_closeSession(CLOUD_SESSION_T *s)
{
   if (CLOUD_INVALID_SOCKET != s->handle)
   {
      close(s->handle);
      s->handle = CLOUD_INVALID_SOCKET;
   }
   cloud_resetSessionStatus(s);
}

//!
//! If the session is not currently connected, attempt to connect,
//! then try to send data if connected. If already connected, attempt
//! to send data.
//!
void cloud_sessionConnectAndSend(CLOUD_SESSION_T *s)
{
   /* Capture the transaction start time, used for a session timeout */
   s->transStart = utils_getCurrentTime();

   if (CLOUD_SESSION_CREATE_SUCCESS == s->status)
   {
      cloud_sessionConnect(s);
   }
   if ((CLOUD_SESSION_CONNECT_SUCCESS == s->status) ||
       (CLOUD_SESSION_IDLE == s->status))
   {
      cloud_startSessionAttempt(s);
      cloud_sessionSend(s);
   }
}

//!
//! This function processes a send operation and wait for a receive from the
//! server.
//!
//! Note1: Prior to calling this function, a call to cloud_sessionSend()
//!        must have been done to set the socket status to either send in
//!        progress or send complete.
//! Note2: If the socket has been configured as non-blocking, this function
//!        needs to be called continuously until true is returned.
//!
//! @param[in] *s pointer to a Cloud session structure object.
//! @param[in] timeoutSec  The number of timeout seconds, (0 = no timeout)
//!
bool cloud_sessionSendRecvAll(CLOUD_SESSION_T *s, uint32_t timeoutSec)
{
   bool complete = false;

   switch(s->status)
   {
      case CLOUD_SESSION_IDLE:
      {
         // Should not be here if we are idle
         break;
      }
      case CLOUD_SESSION_CONNECT_PENDING:
      {
         cloud_sessionConnect(s);
         break;
      }
      case CLOUD_SESSION_CONNECT_SUCCESS:
      case CLOUD_SESSION_SEND_PENDING:
      {
         cloud_sessionSend(s);
         break;
      }
      case CLOUD_SESSION_SEND_SUCCESS:
      case CLOUD_SESSION_RECV_PENDING:
      {
         cloud_sessionRecv(s);
         break;
      }
      case CLOUD_SESSION_FAILED:
      {
         /* Do nothing, the function that indicated the failure */
         break;
      }
   }
   /*
    * A second check is used here to catch success results after the above
    * routines are called. Because it will take some time for the rhapsody
    * state machine to come back around, we speed it up by checking for
    * success status right after the above function calls.
    */
   if (CLOUD_SESSION_RECV_SUCCESS == s->status)
   {
      complete = true;
   }
   else if (CLOUD_SESSION_RECV_PENDING == s->status && s->recvComplete)
   {
      utils_sysLog(LOG_DEBUG, "%s>> receive not successful\n", s->name);
      complete = true;
   }
   else if (timeoutSec > 0)
   {
      if (utils_isTimerExpired(s->transStart, timeoutSec))
      {
         s->timeout = true;
         utils_sysLog(LOG_INFO, "%s>> current session timed out\n", s->name);
      }
   }

   return complete;
}
