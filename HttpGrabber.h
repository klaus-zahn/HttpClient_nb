#pragma once

#if defined (_WIN32)
  #include <winsock.h>
#endif
#include "string.h"
#include "compat.h"
#include "HttpMsg.h"
#include "log.h"

//define a suitable receive socket buffer size in bytes
#define SOCK_BUF_SIZE 200000



class HttpGrabber
{
public:
	//state of state machine connecting and reading data
	enum STATE { STATE_NOT_CONNECTED, STATE_SEND_REQUEST, STATE_WAIT_FOR_DATA, STATE_PARSE_DATA, STATE_DISCONNECT };

	//return values
	enum Result { SUCCESS, INTERNALERR, NODATA, DATAERR, DATAREADY, TIMEOUT, BUFOVERFLOW, CONNLOST, DUPLICATEDATA, DONTKNOW };

	//constructor
	HttpGrabber(const char* _host, const char* _url, unsigned short _port = 80);
	~HttpGrabber();

	//set different loglevel (LOG_ERR, LOG_INFO, LOG_DEBUG)
	void setLogLevel(TheLogLevel logLevel) {set_loglevel(logLevel);}

	//run a complete call and return one complete image data block
	Result Run(char*& msgBuf, unsigned int& size);

	//disconnect from socket
	Result Disconnect();

private:
	//connect to socket and send request	
	Result Connect();

	//convert hostname or ip
	long GetAddrFromString(const char* hostnameOrIp, sockaddr_in* addr);

	//construct and send the request
	Result SendRequest();

	//wait for data to be ready 
	Result WaitForData();

	//read data from socket and parse result to msgBuf; the msgBuf has to be deleted outside!!
	Result Read();

	//host name or ip address
	const char* host;

	//url of the request
	const char* url;
	
	//port
	unsigned short port;
	
	//timeout to wait for data
	unsigned int keepAliveTimeout_ms;

	//current state of message parsing
	STATE state;

	//the socket
	SOCKET sock;
	fd_set read_set;
		
	//the buffer to receive data
	char *rxBuf;
	unsigned int rxBufSize;

	HttpMsg lastMsg;
	enum HttpMsg::ContentType parseMode;

	// last activity on wire
	long long lastTimeStamp;

	unsigned int msgBufWritePos;
	unsigned int consumedBytes;
};

