#ifndef _PARSE_H_
#define _PARSE_H_

//
// HTTP Response Code Defines
//
#define HTTP_CONTINUE               100
#define HTTP_SWITCHING_PROTOCOLS    101
#define HTTP_PROCESSING             102

#define HTTP_SUCCESS                200
#define HTTP_CREATED                201
#define HTTP_ACCEPTED               202
#define HTTP_NON_AUTH               203
#define HTTP_NO_CONTENT             204
#define HTTP_RESET_CONTENT          205
#define HTTP_PARTIAL_CONTENT        206
#define HTTP_MULTI_STATUS           207
#define HTTP_ALREADY_REPORTED       208
#define HTTP_IM_USED                226

#define HTTP_MULTIPLE_CHOICE        300
#define HTTP_MOVED_PERMANENTLY      301
#define HTTP_TEMPORARY_REDIRECT     307
#define HTTP_PERMANENT_REDIRECT     308

#define HTTP_BAD_REQUEST            400
#define HTTP_UNAUTHORIZED           401
#define HTTP_FORBIDDEN              403
#define HTTP_NOT_FOUND              404
#define HTTP_REQUEST_TIMEOUT        408
#define HTTP_LENGTH_REQUIRED        411

#define HTTP_INTERNAL_ERROR         500
#define HTTP_NOT_IMPLEMENTED        501
#define HTTP_BAD_GATEWAY            502
#define HTTP_SERVICE_UNAVAILABLE    503
#define HTTP_GATEWAY_TIMEOUT        504
#define HTTP_VERSION_NOT_SUPPORTED  505

//
// Function Prototypes
//
char* parse_getContentStart(char* strPtr);
int  parse_getContentLength(char* strPtr);
bool parse_contentLengthFound(char* strPtr);
bool parse_entireContentReceived(char* strPtr, size_t bytesRcvd);
bool parse_fullDataChunkFound(char* strPtr);
bool parse_fullHeaderFound(char* strPtr);
int  parse_getStatusCode(char* strPtr);
bool parse_goodStatusCode(int code);
bool parse_transferEncodingChunkedFound(char* strPtr);

#endif /* _PARSE_H_ */
