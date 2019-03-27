#include "compat.h"
#include "log.h"
#include "HttpMsg.h"
#include "cencode.h"
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <stdio.h>


HttpMsg::HttpMsg()
{	
	Clear();
}

HttpMsg::~HttpMsg()
{
}


void HttpMsg::Clear(void)
{
	state = STATE_PARSER_RESET;
	contentType = MODE_UNKNOWN;
	httpStatus = STATUS_UNKNOWN;	
	connectionClose = true;
	encHeaderSize = 0;
	encContentHeaderSize = 0;
	contentLength = -1;
	memset(headers, 0, sizeof(headers));
	memset(headerCopy, 0, sizeof(headerCopy));
	memset(contentHeaders, 0, sizeof(contentHeaders));
	memset(contentHeaderCopy, 0, sizeof(contentHeaderCopy));
	
	ResetParser();
}


void HttpMsg::ResetParser() {
	dataStartAddr = NULL; // point to the first byte
	dataEndAddr = NULL;   // point to the last byte	
}



int HttpMsg::ParseSocketResponse(const char* buf, unsigned int size, unsigned int& consumedBytes)
{
	bool go = true;
	const char* bufParsePos = buf;
	unsigned int remainingSize = size;
	const char* bufRetPos = NULL;
	int ret = SUCCESS;

	//to leave loop set go to false
	while (go) {
		switch (state) {
			case STATE_PARSER_RESET:
				//parser was reset -> search header
				state = STATE_SEARCH_HEADER_START;
				break;
			case STATE_SEARCH_HEADER_START: 
				//look for start of http header, remaining size is changed!
				if ((bufRetPos = FindHTTPHeader(bufParsePos, remainingSize)) != NULL) {
					bufParsePos = bufRetPos;
					state = STATE_SEARCH_HEADER_END;
				} else {
					go = false;//not enough data
					ret = PARSE_INCOMPLETE;
				}
				break;
			case STATE_SEARCH_HEADER_END:
				//look for end of http header while trying to parse content
				if (ParseHeader(bufParsePos, remainingSize) == SUCCESS) {
					//check for correct status
					HttpStatus status = CheckHttpStatus(bufParsePos, remainingSize);
					if (status == STATUS_OK) {
						contentType = CheckContentType(bufParsePos, remainingSize);
						bufParsePos += encHeaderSize;
						consumedBytes += encHeaderSize;
						remainingSize -= encHeaderSize;
						if (contentType == MODE_SINGLEFRAME_JPEG) {
							state = STATE_PARSE_CONTENT;
						} else if (contentType == MODE_MULTIPART_JPG) {
							state = STATE_PARSE_CONTENT_HEADER;
						} else {
							log_error("the content type is unkonwn and cannot be parsed");
							return PARSE_INVALIDDATA;
						}	
					} else if (status == STATUS_AUTHENTICATION_REQUIRED) {						
						return AUTHENTICATION_REQUIRED;
					} else {
						log_error("The http request returned a status different from '200' or '401'");
						return PARSE_INVALIDDATA;
					}
				} else {
					go = false;//not enough data
					ret = PARSE_INCOMPLETE;
				}			
				break;
			case STATE_PARSE_CONTENT_HEADER:
				if (ParseContentHeader(bufParsePos, remainingSize) == SUCCESS) {
					bufParsePos += encContentHeaderSize;
					consumedBytes += encContentHeaderSize;
					remainingSize -= encContentHeaderSize;
					//check for correct content type
					const char* contentTypeStr = getContentHeaderValue("Content-Type");
					if (contentTypeStr != NULL && strncmp(contentTypeStr, "image/jpeg", strlen("image/jpeg")) == 0) {
						//axis cameras have content length in header
						const char* contentLengthStr = getContentHeaderValue("Content-Length");
						if (contentLengthStr) {
							contentLength = atoi(contentLengthStr);
						} else {
							contentLength = -1;
						}
					} else {
						log_error("the content type is unkonwn and cannot be parsed");
						return PARSE_INVALIDDATA;
					}
					state = STATE_PARSE_CONTENT;
				} else {
					go = false;//not enough data
					ret = PARSE_INCOMPLETE;
				}
				break;
			case STATE_PARSE_CONTENT:				
				go = false;
				ret = ParseMsgContent(bufParsePos, remainingSize);
				if (ret == SUCCESS) {//we have a full image
					uint64_t timeStampMilliseconds = 0;
					consumedBytes += (1+dataEndAddr - (unsigned char*) bufParsePos);//the 0xd9 byte was read
					//try to read the time stamp but not more than 200 bytes
					ParseAxisJpegHeader(dataStartAddr, std::min((int) (dataEndAddr-dataStartAddr), 200), timeStampMilliseconds);
					if (timeStampMilliseconds != 0) {
						log_info("axis time stamp: %lld", timeStampMilliseconds);
					}
					state = STATE_PARSE_CONTENT_HEADER;
				}
				break;
			default:
				log_error("unknown state in http message parser");
				return PARSE_INVALIDDATA;
				break;
		}
	}

	return ret;
}



const char* HttpMsg::FindHTTPHeader(const char* buf, unsigned int& size)
{
	// look for valid Header in a binary blob
	unsigned int idx;

	for (idx = 0; idx < size - 5; idx++) { // 5: strlen ("HTTP/")
		if (0 == strncasecmp(&buf[idx], "HTTP/", 5)) {
			size -= idx;
			return &buf[idx];
		}
	}
	return NULL;
}



int HttpMsg::ParseHeader(const char* buf, unsigned int size)
{
	if (strncasecmp(buf, "HTTP/", 5))
		return PARSE_NOSTARTFOUND;

	return ParseAnyHeader(buf, size, headerCopy, MAX_BUF_SIZE, headers, MAX_HEADERS, &encHeaderSize, "http header");
}



int HttpMsg::ParseAnyHeader(const char* buf, unsigned int size, char* headerCopyPtr, unsigned int headerCopySize, 
				struct header *headersPtr, unsigned int maxHeaders, unsigned int *headerSize, const char* headerType)
{
	bool endReached = false;
	char* pos = headerCopyPtr;
	unsigned int headerIdx = 0;
	int maxHeaderSize = (headerCopySize > size + 1) ? (size + 1) : headerCopySize;
	char* headerCopyEnd = headerCopyPtr + maxHeaderSize;

	strncpy(headerCopyPtr, buf, maxHeaderSize);

	while ((headerIdx < maxHeaders - 1) && pos && (pos < headerCopyEnd) && (pos = strstr(pos, "\r\n"))) { // skip the first line
		*pos++ = '\0';
		*pos++ = '\0';
		if (!strncmp(pos, "\r\n", 2)) { // last line
			endReached = true;
			*headerSize = (pos + 2) - headerCopyPtr;
			break;
		}

		headersPtr[headerIdx].name = pos;
		char* posDelim = strstr(pos, ": ");
		if (posDelim) {
			pos = posDelim;
			*pos++ = '\0';
			*pos++ = '\0';
			headersPtr[headerIdx].value = pos;
		}
		else if ((posDelim = strstr(pos, "="))) {
			pos = posDelim;
			*pos++ = '\0';
			headersPtr[headerIdx].value = pos;
		}

		headerIdx++;
	}

	headersPtr[headerIdx].name = NULL;

	//output info
	log_info("start of %s", headerType);
	for (unsigned int i0 = 0; i0 < headerIdx; i0++) {
		log_info("name: %s , value: %s", headersPtr[i0].name, headersPtr[i0].value);
	}
	log_info("end of %s", headerType);

	if (endReached)
		return SUCCESS;
	else
		return PARSE_NOENDFOUND;
}


int HttpMsg::ParseContentHeader(const char* buf, int unsigned size) 
{
	Result ret = SUCCESS;	
	
	if (size < strlen(multipartBoundary)) {
		log_info("size (%d) is smaller than boundary string length (%d)", size, strlen(multipartBoundary));
		return PARSE_NOENDFOUND;
	}

	const char* posBoundary = strnstr(buf, multipartBoundary, size);

	contentHeaders[0].name = NULL; // clear contents

	if (posBoundary != 0) {
		// begin of multiframe content found
		const char* contentBegin = strnstr(posBoundary, "\r\n", size - (posBoundary - buf));
		if (contentBegin != 0) {
			ret = ParseAnyHeader(contentBegin, size - (contentBegin - buf), contentHeaderCopy, MAX_BUF_SIZE, contentHeaders, MAX_CONTENTHEADERS, &encContentHeaderSize, "http content header");
			if (ret == SUCCESS) {
				encContentHeaderSize += (contentBegin - buf);
			}
		}
	}
	return ret;
}


enum HttpMsg::HttpStatus HttpMsg::CheckHttpStatus(const char* buf, int unsigned size)
{
	if (strncasecmp(buf, "HTTP/", 5))
		return STATUS_ERROR;

	//set connection close
	const char* doClose = getHeaderValue("Connection");
	connectionClose = doClose == NULL ? true : (strncasecmp(doClose, "close", strlen("close")) == 0);

	//add different status to look for here
	char* pos = NULL;
	if ((pos = strnstr(buf, " 200", std::min(13, (int) encHeaderSize - 4))) != NULL) {//strlen("Http/1.1 200") + 1
		return STATUS_OK;
	}
	if ((pos = strnstr(buf, " 401", std::min(13, (int) encHeaderSize - 4))) != NULL) {
		//the server requries authentication
		log_error("the server requires authentication!")
		return STATUS_AUTHENTICATION_REQUIRED;
	}
	return STATUS_ERROR;
}


enum HttpMsg::ContentType HttpMsg::CheckContentType(const char* buf, int unsigned size)
{
	const char *pos;
	const char* _contentType = getHeaderValue("Content-Type");
	const char* contentLengthStr = getHeaderValue("Content-Length");

	if (!_contentType)
		return MODE_UNKNOWN;

	if (contentLengthStr) {
		contentLength = atoi(contentLengthStr);
	} else {
		contentLength = -1;
	}

	if (strstr(_contentType, HttpDefs::MIME_IMAGE_JPEG))
		return MODE_SINGLEFRAME_JPEG;	

	if ((pos = strstr(_contentType, HttpDefs::MIME_MULTIPART_MIXEDREPLACE))) {
		if (!(pos = strstr(pos, "boundary=")))
			return MODE_UNKNOWN;

		const char * boundary = pos + 9; // we hope boundary is the last parameter in  line
		multipartBoundary[0] = multipartBoundary[1] = '-';
		multipartBoundary[2] = 0;
		//in axis camera the -- is part of the boundary
		while (*boundary == '-') { boundary++; }
		strncpy(multipartBoundary + 2, boundary, LEN_BOUNDARY_STRING);

		log_info("New multipart stream started, boundary: %s", multipartBoundary);

		/***** part of this parser may be proprietary and only working for axis IP cameras *****/
		// step to content block
		if (!(pos = strnstr(buf, "\r\n\r\n", size)))
			return MODE_UNKNOWN;

		pos += 4;
		
		if (strnstr(pos, HttpDefs::MIME_IMAGE_JPEG, size - (pos - buf)))
			return MODE_MULTIPART_JPG;		

		return MODE_UNKNOWN;
	}
	return MODE_UNKNOWN;
}



int HttpMsg::ParseMsgContent(const char* buf, int unsigned size)
{
	if (contentType == MODE_UNKNOWN)
		return PARSE_INVALIDDATA;

	if (size < 4) {
		log_debug("received data size (%d) is less than Start Of Image sequence", size);
		return PARSE_INCOMPLETE;
	}
	
	switch (contentType) {
		case MODE_MULTIPART_JPG:
		case MODE_SINGLEFRAME_JPEG:
			if (contentLength > 0) {	
				if ((dataStartAddr = (unsigned char*)strnstr(buf, "ÿØÿà", size)) != NULL) {
					log_debug("'Start Of Image' including JFIF tag ÿØÿà found");
				} else {
					if ((dataStartAddr = (unsigned char*)strnstr(buf, "ÿØÿþ", size)) != NULL) {
						log_debug("'Start Of Image' w/o JFIF tag ÿØÿþ found (axis ip camera?)");
					} else {
						log_error("no 'Start Of Image' flag  'ÿØ' found. Try again");
						return PARSE_INVALIDDATA;
					}
				}
				unsigned int offset = dataStartAddr - (unsigned char*)buf;
				if ((offset + contentLength) <= size) {
					//check for correct end
					if (*(dataStartAddr + contentLength - 1) == 0xD9) {
						//set end address
						dataEndAddr = dataStartAddr + contentLength - 1;
						log_info("received a complete jpg image of length %d", contentLength);
						break;
					} else {
						log_error("no end of jpeg (0xD9) found!");
						return PARSE_INVALIDDATA;
					}
				}
				else {
					log_debug("received data size (%d) is less than Content-Length (%d)", size - offset, contentLength);
					return PARSE_INCOMPLETE;
				}
			} else { // not all servers send Content-Length
				log_error("no content length sent; implement a valid parser");
				return PARSER_NOTIMPLEMENTED;
			}
			break;		
		
		default:
			return PARSER_NOTIMPLEMENTED;
	}

	if (!dataStartAddr)
		return PARSE_NOSTARTFOUND;

	if (!dataEndAddr || dataStartAddr > dataEndAddr)
		return PARSE_INCOMPLETE;

	return SUCCESS;
}


int HttpMsg::ParseAxisJpegHeader(const unsigned char* buf, int unsigned size, uint64_t& timeStampMilliseconds) {
	//currently we only parse time stamp
	const unsigned char timeStampHeaderStr[6] = { 0xFF, 0xFE, 0x00, 0x0F, 0x0A, 0x01 };
	uint32_t sizeTimeStampHeader = sizeof(timeStampHeaderStr);
	Result ret = SUCCESS;
	uint32_t offset = 0;
	//find the timeStampHeader
	while ((offset + sizeTimeStampHeader) < size && memcmp(buf + offset, timeStampHeaderStr, sizeof(timeStampHeaderStr))) {
		offset++;
	}
	offset += sizeTimeStampHeader;
	if ((offset + 5) < size) {//we require at least 5 byte
		const unsigned char* timeStamp = buf + offset;

		uint64_t timeStampSec = (*timeStamp << 24) + (*(timeStamp + 1) << 16) + (*(timeStamp + 2) << 8) + *(timeStamp + 3);
		uint64_t timeStampHundred = *(timeStamp + 4);

		timeStampMilliseconds = timeStampSec * 1000 + timeStampHundred * 10;
	}
	return ret;
}


const char *HttpMsg::getAnyHeaderValue(const char *name, struct header *headersPtr, unsigned int* index)
{
    unsigned int idx = index ? *index : 0;//possibility to set (and return) idx (in case field name is used more than once)
    if (!headersPtr)
        return NULL;

    while (headersPtr[idx].name){
        if (!strcasecmp(headersPtr[idx].name, name)){
			if (index) { *index = idx; }
            return headersPtr[idx].value;
        }
        idx++;
    }
    return NULL;
}

const char *HttpMsg::getHeaderValue(const char *name, unsigned int* index)
{
    return getAnyHeaderValue(name, headers, index);
}

const char *HttpMsg::getContentHeaderValue(const char *name)
{
    return getAnyHeaderValue(name, contentHeaders);
}




