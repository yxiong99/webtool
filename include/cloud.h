#ifndef _CLOUD_H_
#define _CLOUD_H_

#include <netdb.h>

#define CLOUD_SEND_BUF_LEN   (2048)
#define CLOUD_RECV_BUF_LEN   (1048576)

#define CLOUD_TCP_PORT_HTTP  (80)
#define CLOUD_TCP_PORT_HTTPS (443)

//
// Socket Type
//
typedef int32_t CLOUD_SOCKET;

//
// Socket address, internet style.
//
typedef struct
{
   uint8_t            sin_family;
   uint8_t            sin_len;
   uint16_t           sin_port;
   struct in_addr     sin_addr;
   uint8_t            sin_zero[8];
}
CLOUD_SOCKADDR_T;

//
// Global Constants
//
#define CLOUD_INVALID_SOCKET (CLOUD_SOCKET)(~0)
#define CLOUD_SOCKET_ERROR   (-1)

//
// Cloud Session Status
//
typedef enum
{
   CLOUD_SESSION_IDLE,
   CLOUD_SESSION_CREATE_SUCCESS,
   CLOUD_SESSION_CONNECT_PENDING,
   CLOUD_SESSION_CONNECT_SUCCESS,
   CLOUD_SESSION_SEND_PENDING,
   CLOUD_SESSION_SEND_SUCCESS,
   CLOUD_SESSION_RECV_PENDING,
   CLOUD_SESSION_RECV_SUCCESS,
   CLOUD_SESSION_FAILED,
}
CLOUD_SESSION_STATUS_T;

//!
//! Cloud Diagnostics
//!
typedef struct
{
   uint32_t attempts;
   uint32_t sockFailures;
   uint32_t sendFailures;
   uint32_t lastHttpStatus;
}
CLOUD_DIAGS_T;

//
// Cloud Session Structure
//
typedef struct
{
   char *name;                        //!< Session name for debug printing
   CLOUD_SOCKET handle;               //!< Handle to this socket.
   CLOUD_SESSION_STATUS_T status;     //!< Last known status for this socket.
   int errorCode;                     //!< Error code. *valid when status is failed.
   int totalBytesToSend;              //!< Total bytes to send. This was added to allow binary transfers.
   int totalBytesSent;                //!< Total bytes sent. *valid only during send operations.
   int totalBytesRcvd;                //!< Total bytes received. *valid only during recv operations.
   int httpStatus;                    //!< HTTP return status code
   uint32_t errorTime;                //!< OS time of last error.
   uint32_t transStart;               //!< OS time at start of transaction.
   char* sendBuf;                     //!< Pointer to the send buffer used by this socket
   char* recvBuf;                     //!< Pointer to the recv buffer used by this socket
   size_t recvBufLen;                 //!< Recv buffer length
   bool recvComplete;                 //!< WebSockets data callback indicating a recv() is complete
   bool timeout;                      //!< Send/recv timeout
   CLOUD_DIAGS_T *diags;              //!< Cloud session diagnostics structure
}
CLOUD_SESSION_T;

//
// Function Prototypes
//
void cloud_init(void);
bool cloud_initSession(CLOUD_SESSION_T *s, char *serverName, uint16_t serverPort);
void cloud_closeSession(CLOUD_SESSION_T *s);
void cloud_sessionConnectAndSend(CLOUD_SESSION_T *s);
bool cloud_sessionSendRecvAll(CLOUD_SESSION_T *s, uint32_t timeoutSec);
void cloud_resetSessionStatus(CLOUD_SESSION_T *s);

#endif /* _CLOUD_H_ */
