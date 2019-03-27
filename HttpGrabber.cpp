//implements a generic grabber for http-based cameras
#include <stdio.h>
#include "HttpGrabber.h"
#include "log.h"
#include "compat.h"


//http get request mask
static const char* request_GET_FORMAT =
"GET /%s HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"\r\n";


HttpGrabber::HttpGrabber(const char* _host, const char* _url, unsigned short _port):
	host(_host), url(_url), port(_port), keepAliveTimeout_ms(1000), state(STATE_NOT_CONNECTED),
	sock(INVALID_SOCKET), msgBufWritePos(0), consumedBytes(0)
{
	rxBufSize = SOCK_BUF_SIZE;
	rxBuf = new char[rxBufSize];
}


HttpGrabber::~HttpGrabber()
{
	Disconnect();
	delete[] rxBuf;
}


HttpGrabber::Result HttpGrabber::Run(char*& msgBuf, unsigned int& size)
{
	bool go = true;
	Result ret = SUCCESS;

	while (go) {
		switch (state) {
			case STATE_NOT_CONNECTED:
				if (Connect() != SUCCESS) {
					return CONNLOST;
				}
				state = STATE_SEND_REQUEST;
				break;
			case STATE_SEND_REQUEST:
				if (SendRequest() != SUCCESS) {
					return CONNLOST;
				}
				state = STATE_WAIT_FOR_DATA;
				break;
			case STATE_WAIT_FOR_DATA:
				ret = WaitForData();
				if (ret == CONNLOST || ret == DATAERR || ret == DONTKNOW || ret == TIMEOUT) {
					return ret;
				}
				if (ret == DATAREADY) {
					state = STATE_PARSE_DATA;
				}
				break;
			case STATE_PARSE_DATA:
				ret = Read();
				if (ret == SUCCESS) {//a complete image arrived
					const unsigned char* buf = lastMsg.getDataStartAddr(size);
					msgBuf = new char[size];
					memcpy(msgBuf, buf, size);
					lastMsg.ResetParser();
					//we leave the state machine and return result
					//the caller decides whether to go on or not
					go = false;
				}
				state = STATE_WAIT_FOR_DATA;

				if (ret == CONNLOST || ret == DATAERR || ret == DONTKNOW) {
					return ret;
				}				
				
				break;
			default:
				log_error("unknown state in HttpGrabber");
				ret = DONTKNOW;
				break;
		}
	}

	return ret;
}


HttpGrabber::Result HttpGrabber::Connect()
{
	sockaddr_in addr;	
	int sockbufsize = SOCK_BUF_SIZE;

	const char *errMsg = "unknown error";	

	if (sock == INVALID_SOCKET) {
		if (0 > (sock = socket(AF_INET, SOCK_STREAM, 0))) {
			errMsg = "couldn't create socket";
			goto error;
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (GetAddrFromString(host, &addr)) {
			errMsg = "couldn't resolve ip address";
			goto error;
		}
		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, sizeof(sockbufsize));
		
		msgBufWritePos = 0;
		consumedBytes = 0;

		log_info("trying to connect to host %s:%d", host, port);
		if (0 > connect(sock, (sockaddr*)&addr, sizeof(sockaddr))) {
			errMsg = "Couldn't connect to server!";
			goto error;
		}
	}
	
	return SUCCESS;

error:
	log_error("%s", errMsg);
	return CONNLOST;
}


HttpGrabber::Result HttpGrabber::SendRequest()
{	
	char httpReqMsg[MAX_BUF_SIZE];
	//construct the request
	int _len = snprintf(httpReqMsg, MAX_BUF_SIZE, request_GET_FORMAT, url, host);
	if (MAX_BUF_SIZE <= _len) {
		log_error("buffer size (%d) too small for request (%d)", MAX_BUF_SIZE, _len);
		return INTERNALERR;
	}

	if (_len != send(sock, httpReqMsg, _len, 0)) {
		Disconnect();
		log_error("send request\n%s\n resulted in an error", httpReqMsg);
		return CONNLOST;
	}
	log_debug(">> GET %s:%d/%s", host, port, httpReqMsg);
	return SUCCESS;
}


HttpGrabber::Result HttpGrabber::WaitForData()
{
	if (sock == INVALID_SOCKET)
		return CONNLOST;

	struct timeval _timeout;
	if (keepAliveTimeout_ms) {
		_timeout.tv_sec = keepAliveTimeout_ms/1000;
		_timeout.tv_usec = (keepAliveTimeout_ms%1000)/1000;
	}
	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);
	int ret = select(sock + 1, &read_set, NULL, NULL, (keepAliveTimeout_ms ? &_timeout : NULL));
	if (ret == 0) {
		return TIMEOUT;
	}
	else if (ret > 0) {
		if (FD_ISSET(sock, &read_set))
			return DATAREADY;
	}
	else if (ret < 0)
		return DATAERR;
	return DONTKNOW;
}


// method returns max 1 message per call
// therefore there is a shortened path to read out the rx buffer
HttpGrabber::Result HttpGrabber::Read()
{
	if (sock == INVALID_SOCKET)
		return CONNLOST;

	// check for abandon chunk of last read (may be the begin of a new message), shift second message (fragment)
	if (consumedBytes == msgBufWritePos) {
		msgBufWritePos = 0; // we don't miss anything
	}
	else if (consumedBytes && (msgBufWritePos > consumedBytes)) {   // remove already consumed bytes from rxBuf
		memmove(rxBuf, rxBuf + consumedBytes, msgBufWritePos - consumedBytes);
		msgBufWritePos -= consumedBytes;
	}
	consumedBytes = 0;
	int recvLen = recv(sock, rxBuf + msgBufWritePos, rxBufSize - msgBufWritePos, 0);
	if (recvLen > 0) {
		msgBufWritePos += recvLen;		
		int parseRes = lastMsg.ParseSocketResponse(rxBuf, msgBufWritePos, consumedBytes);// msgBufWritePos is also end of read buffer
		if (parseRes == HttpMsg::SUCCESS) {
			return SUCCESS;			
		}
		else if (parseRes == HttpMsg::PARSE_INCOMPLETE || parseRes == HttpMsg::PARSE_NOSTARTFOUND) {
			if (msgBufWritePos >= (rxBufSize - 1)) {
				log_error("receive buffer size (%d) is too small compared to content length (%d)", rxBufSize, lastMsg.GetContentLength());
				msgBufWritePos = 0; //reset data
				lastMsg.ResetParser();
				return BUFOVERFLOW;
			}
			log_debug("waiting for additional data");
			return NODATA;  //incomplete, wait for next arrival
		}
		else {
			msgBufWritePos = 0; //reset data
			return DATAERR;// 
		}
	}
	return CONNLOST;
}



HttpGrabber::Result HttpGrabber::Disconnect()
{
	log_debug("Disconnect");
	lastMsg.Clear();
	state = STATE_NOT_CONNECTED;	
	if (sock == INVALID_SOCKET)
		return SUCCESS;

	shutdown(sock, SHUT_RDWR);
//	closesocket(sock);
	sock = INVALID_SOCKET;
	return SUCCESS;
}



long HttpGrabber::GetAddrFromString(const char* hostnameOrIp, sockaddr_in* addr)
{
	unsigned long ip;
	hostent* he;

	/* Check parameter */
	if (hostnameOrIp == NULL || addr == NULL)
		return -1;

	/* an IP in hostnameOrIp ? */
	ip = inet_addr(hostnameOrIp);

	/* in case of error inet_addr returns INADDR_NONE */
	if (ip != INADDR_NONE) {
		addr->sin_addr.s_addr = ip;
		return 0;
	}
	else {
		/* get hostname in hostnameOrIp */
		he = gethostbyname(hostnameOrIp);
		if (he == NULL) {
			return -1;
		}
		else {
			/*copy 4 bytes of IP from 'he' to 'addr' */
			memcpy(&(addr->sin_addr), he->h_addr_list[0], 4);
		}
		return 0;
	}
}

