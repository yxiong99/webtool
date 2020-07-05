//******************************************************************************
//!
//! Author:  Ying Xiong
//! Created: Nov 2019
//!
//******************************************************************************

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

#define CONTENT_LENGTH_STR       "Content-Length:"
#define TRANSF_ENC_CHUNKED_STR   "Transfer-Encoding: chunked"
#define HTTP_HEADER_TERMINATION  "\r\n\r\n"
#define MIN_GOOD_HTTP_STATUS     200
#define MAX_GOOD_HTTP_STATUS     407

//!
//! Get the start of the HTTP content.
//!
//! @param[in] strPtr  Pointer to http string
//! @return  Pointer to start of the HTTP content
//!
char* parse_getContentStart(char* strPtr)
{
   char* bodyStart = strstr(strPtr, HTTP_HEADER_TERMINATION) + 4;
   return bodyStart;
}

//!
//! Get the content length from the specified http data buffer.
//!
//! @param[in] strPtr  Pointer to http string
//!
//! @return  -1 if content length not found, otherwise length in bytes
//!
int parse_getContentLength(char* strPtr)
{
   char* tempPtr;
   int content_length = -1;

   if (parse_fullHeaderFound(strPtr))
   {
      tempPtr = strstr(strPtr, CONTENT_LENGTH_STR);
      if (tempPtr)
      {
         tempPtr += strlen(CONTENT_LENGTH_STR); // Look past the "Content-Length"
         content_length = strtol(tempPtr, NULL, 10);
      }
   }

   return content_length;
}

//!
//! Determine if the header in the specified string contains "Content-Length:".
//!
//! @param[in] strPtr  Pointer to http string
//! @return  true if the desired string is found, otherwise false
//!
bool parse_contentLengthFound(char* strPtr)
{
   char* tempPtr;
   bool  retval;

   tempPtr = strstr(strPtr, CONTENT_LENGTH_STR);
   retval = (NULL != tempPtr);

   return retval;
}

//!
//! Determine if the entire specified content length has been received.
//!
//! @param[in] strPtr  Pointer to http string
//! @param[in] bytesRcvd  Total number of received bytes
//! @return  true if entire content length has been received, otherwise false
//!
bool parse_entireContentReceived(char* strPtr, size_t bytesRcvd)
{
   int content_length = 0;
   bool retval = false;

   content_length = parse_getContentLength(strPtr);
   if (content_length >= 0)
   {
      char* tempPtr = strstr(strPtr, HTTP_HEADER_TERMINATION) + 4;
      size_t exp_bytes = (tempPtr - strPtr) + content_length;

      if (exp_bytes == bytesRcvd)
      {
         retval = true;
      }
   }

   return retval;
}


//!
//! Determine if the end of a data chunk is received.
//!
//! This function assumes that chunked encoding was found in the
//! HTTP header.
//!
//! @param[in] strPtr  Pointer to http string
//! @return  true if end of chunk is received, otherwise false
//!
bool parse_fullDataChunkFound(char* strPtr)
{
   bool retval = false;
   char* content;

   content = parse_getContentStart(strPtr);
   if (content && strstr(content, "0\r\n\r\n"))
   {
      retval = true;
   }

   return retval;
}

//!
//! Determine if the end of the HTTP header is found in the specified
//! string.
//!
//! @param[in] strPtr  Pointer to http string
//! @return  true if end of header is found, otherwise false
//!
bool parse_fullHeaderFound(char* strPtr)
{
   char* tempPtr;
   bool retval;

   tempPtr = strstr(strPtr, HTTP_HEADER_TERMINATION);
   retval = (NULL != tempPtr);

   return retval;
}

//!
//! Get the HTTP response code from the specified http string.
//!
//! @param[in] strPtr  Pointer to http string
//! @return  HTTP response code
//!
int parse_getStatusCode(char* strPtr)
{
   char* tempPtr;
   int status = -1;

   tempPtr = strstr(strPtr, "HTTP/");
   if (NULL != tempPtr)
   {
      tempPtr += 8; // Look past the "HTTP/1.1"
      status = strtol(tempPtr, NULL, 10);
   }

   return status;
}

//!
//! Determine if a good HTTP status code was received.
//!
//! @param[in] code  Received status code
//! @return  true if good HTTP status code received, otherwise false
//!
bool parse_goodStatusCode(int code)
{
   bool good = false;

   if ((code >= MIN_GOOD_HTTP_STATUS) && (code <= MAX_GOOD_HTTP_STATUS))
   {
      good = true;
   }

   return good;
}

//!
//! Determine if the specified string contains "Transfer-Encoding: chunked".
//!
//! @param[in] strPtr  Pointer to http string
//! @return  true if the desired string is found, otherwise false
//!
bool parse_transferEncodingChunkedFound(char* strPtr)
{
   char* tempPtr;
   bool retval;

   tempPtr = strstr(strPtr, TRANSF_ENC_CHUNKED_STR);
   retval = (NULL != tempPtr);

   return retval;
}
