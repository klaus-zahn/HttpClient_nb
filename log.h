#ifndef LOG_H
#define  LOG_H

#if defined (_WIN32)
// avoid full path expansion
#define __FILENAME__ (strchr(__FILE__ ,'\\')?(strrchr(__FILE__ ,'\\')+1):__FILE__)
#else
// avoid full path expansion
#define __FILENAME__ (strchr(__FILE__ ,'/')?(strrchr(__FILE__ ,'/')+1):__FILE__)
#endif

// for better readybility we align the filename left-aligned after user message
#define FLENAME_ALIGNMENT 63

// limited length for user defined part = 2*FLENAME_ALIGNMENT
#define MAX_LOG_MSG 255

typedef enum logLevelEnum {LOG_ERR, LOG_INFO, LOG_DEBUG, LOG_UNDEFINED} TheLogLevel;


/************************************ user fucntions*************************************************************/
extern TheLogLevel logLevel;
extern int verboseLevel;

/** 
* @brief	Print debug message, with header and line feed
*/
#define log_debug(...) \
    if (logLevel >= LOG_DEBUG){ _log(LOG_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__); }

/** 
* @brief	Print info message, with header and line feed
*/
#define log_info(...) \
    if (logLevel >= LOG_INFO){ _log(LOG_INFO,  __FILENAME__, __LINE__, __VA_ARGS__); }

/** 
* @brief	Print error message, with header and line feed
*/
#define log_error(...) \
    if (logLevel >= LOG_ERR){ _log(LOG_ERR,  __FILENAME__, __LINE__, __VA_ARGS__); }

/** 
* @brief	stdout without header, with line feed
*/
#define log_verbose(...) \
    if (verboseLevel > 0){ _verbose( __VA_ARGS__); }
/***********************************************************************************************************/

/** 
* @brief	Print log output
*/ 
int _log(TheLogLevel level, const char* filename, int line, const char *format, ...);

/** 
* @brief	Print verbose output
*/ 
int _verbose(const char *format, ...);

/** 
* @brief	set log level
*/ 
void set_loglevel(unsigned level);

/** 
* @brief	set verbosity level
*/ 
void set_verbosity(unsigned verbosity);

#endif
