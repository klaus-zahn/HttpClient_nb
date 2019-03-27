#ifndef HTTPMSG_H
#define HTTPMSG_H

#include "HttpDefs.h"
#include <inttypes.h>

#define MAX_HEADERS 16
#define MAX_CONTENTHEADERS 64
#define LEN_BOUNDARY_STRING 18 // maximal length of boundary string plus end-0
#define LEN_FRAMEINDEX_STRING 16 // maximal length of frame-index string plus end-0

//define a suitable string buffer size in bytes
#define MAX_BUF_SIZE 1000



class HttpMsg
{
public:
    //state of state machine analyzing the http response
	enum STATE { STATE_SEARCH_HEADER_START, STATE_SEARCH_HEADER_END, STATE_PARSE_CONTENT_HEADER, STATE_PARSE_CONTENT, STATE_PARSER_RESET};
	
	typedef int Result;
    enum RESULT{SUCCESS, AUTHENTICATION_REQUIRED, PARSE_INCOMPLETE, PARSE_SHORTREAD, PARSE_INVALIDLEN, PARSE_NOSTARTFOUND, PARSE_NOENDFOUND, PARSE_INVALIDDATA, PARSER_NOTIMPLEMENTED, NOTFOUND};

    // the answer of our request determines the mode (only a limited set of modes are currently implemented)
    enum ContentType {MODE_UNKNOWN, MODE_SINGLEFRAME_JPEG, MODE_MULTIPART_JPG};

	//different http status types
    enum HttpStatus {STATUS_UNKNOWN = -1, STATUS_OK, STATUS_AUTHENTICATION_REQUIRED, STATUS_ERROR};

	//constructor
    HttpMsg(void);
    ~HttpMsg(void);

	//parses the message in the receive buffer
	int ParseSocketResponse(const char* buf, int unsigned size, unsigned int& consumedBytes);

	//return start address of data and size
	const unsigned char* getDataStartAddr(unsigned int& dataSize) {
		dataSize = (dataEndAddr-dataStartAddr);
		return dataStartAddr; 
	}

	//get the content length of current message
	const int GetContentLength() {return contentLength;}

	//return connection status of http msg
	const bool GetConnectionClose() {return connectionClose;}
    
    // reset existing object (usefull for static objects)
    void Clear(void);
    
    //resets the parser
	void ResetParser();

private:
	//current state of message parsing
	STATE state;

	// length of textual header incl. "\r\n\r\n"
	unsigned int encHeaderSize; 

	// length of content header incl. boundary string and "\r\n\r\n"
	unsigned int encContentHeaderSize;

	//the content type of the return message
	enum ContentType contentType;

	//status of return message
	enum HttpStatus httpStatus;

	//whether or not to close connection to server
	bool connectionClose;

	//content length of return msg (-1 means no 'Content-Length' field)
	int contentLength;

	// point to the first byte
	unsigned char *dataStartAddr; 
	// point to the last byte
	unsigned char *dataEndAddr;   

	// helpers
	struct header {
		char *name;          // HTTP header name
		char *value;         // HTTP header value
	};
	struct header headers[MAX_HEADERS];
	char headerCopy[MAX_BUF_SIZE];
	struct header contentHeaders[MAX_CONTENTHEADERS];
	char contentHeaderCopy[MAX_BUF_SIZE];;

	//look for http-header in buf
	const char* FindHTTPHeader(const char* buf, unsigned int& size);

	//parse the http-header
	Result ParseHeader(const char* buf, unsigned int size);

	// stream parser
    Result ParseContentHeader(const char* buf, int unsigned size);
	
	//parse the header fields
	Result ParseAnyHeader(const char* buf, unsigned int size, char* headerCopyPtr, unsigned int headerCopySize, struct header *headersPtr, unsigned int maxHeaders, unsigned int *headerSize, const char* headerType);

	//check the status of the return message
	enum HttpStatus CheckHttpStatus(const char* buf, int unsigned size);

	//check the content type of the return message
	enum ContentType CheckContentType(const char* buf, int unsigned size);

	//parse the message content
	int ParseMsgContent(const char* buf, int unsigned size);

    //different getters for header field values
    const char *getAnyHeaderValue(const char *name, struct header *headersPtr, unsigned int* index = 0);
    const char *getHeaderValue(const char *name, unsigned int* index = 0);
    const char *getContentHeaderValue(const char *name);

	//parse axis jpeg header looking for times stamp
	int ParseAxisJpegHeader(const unsigned char* buf, int unsigned size, uint64_t& timeStampMilliseconds);

    char multipartBoundary[LEN_BOUNDARY_STRING+2]; // boundary of multipart messages
};

#endif /* HTTPMSG_H */
