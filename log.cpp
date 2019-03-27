#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "log.h"
#include "compat.h"

TheLogLevel logLevel = LOG_INFO;  //default

int verboseLevel = 1; //default

const char *levelDesc[] = {
    "ERR",
    "INF",
    "DBG",
    "???"
};



int _log(TheLogLevel level, const char* filename, int line, const char *format, ...)
{
    va_list list;
    struct tm *current;
    time_t now;

    // prepare user message
    char msg[MAX_LOG_MSG];
    char padding[FLENAME_ALIGNMENT+1];

    va_start(list, format);
    if (vsnprintf(msg, MAX_LOG_MSG, format, list) < 0) {
        va_end(list);
        for(int ii = MAX_LOG_MSG-4; ii < MAX_LOG_MSG-1; ii++) {msg[ii] = '.';}
        msg[MAX_LOG_MSG-1] = 0;
    }
    va_end(list);

    // prepare timestamp
    time(&now);
    current = localtime(&now);

    if (level < 0 || level > 2)
        level = LOG_UNDEFINED;

    // fill padding for left align source info
    int padLen = FLENAME_ALIGNMENT - (int) strlen(msg);
    if (padLen > 0){
        padding[padLen--] = '\0';
    
        while (padLen >= 0){
            padding[padLen--] = ' ';
        }
    }
    else{ // oversized user message
        strcpy(padding," "); // 1 space
    }
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;
 
    if (logLevel >= LOG_DEBUG){
        return printf("%02d/%02d %02d:%02d:%02d.%03d %s %s %s[%s:%d]\n", 
            current->tm_mon+1, current->tm_mday, current->tm_hour, current->tm_min, current->tm_sec, milli,  // tm_mon+1 since 0 = january
            levelDesc[level], msg,
            padding,
            filename, line);
    }
    else{
        return printf("%02d/%02d %02d:%02d:%02d.%03d %s %s %s\n", 
            current->tm_mon+1, current->tm_mday, current->tm_hour, current->tm_min, current->tm_sec, milli,  // tm_mon+1 since 0 = january
            levelDesc[level], msg,
            padding);
    }
}

int _verbose(const char *format, ...)
{
    va_list list;

    // prepare user message
    char msg[4096];
    va_start(list, format);
    if (vsnprintf(msg, 4096, format, list) < 0) {
	    va_end(list);
	    return -1;
    }
    va_end(list);
    return printf("%s\n", msg);
}

void set_loglevel(unsigned int level)
{
    if (level < LOG_ERR || level > LOG_UNDEFINED)
        logLevel = LOG_UNDEFINED;
    else 
        logLevel = (TheLogLevel)level;
}

void set_verbosity(unsigned verbosity)
{
    verboseLevel = verbosity;
}
