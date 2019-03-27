#ifndef COMPAT_TIME_H
#define COMPAT_H


#include <ctime>


//get time in milliseconds
long long time_ms();

/*
* Find the first occurrence of find in s, where the search is limited to the
* first slen characters of s.
* oss license: http://src.gnu-darwin.org/src/lib/libc/string/strnstr.c.html
*/
char * strnstr(const char *s, const char *find, size_t slen);

#if defined(_WIN32)
//suppress warning
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)


//remove definition of min max in windows in order to use std::min/std::max with #include <algorith> instead
#define NOMINMAX
#include <sys/timeb.h>
#include <wchar.h> // struct tm
#include <time.h> // mktime(),..
#include <winsock.h> //struct timeval

// BTW: note that "struct timeval" contains 2 long (tv_sec, tv_usec),
// on windows 64bit a long may stays 32bit (see LLP compiler option), we need to take "long long" to force the 64bit

//compare str1 and st2 disregarding case
#define strcasecmp(str1, str2) _stricmp(str1, str2)
//compare at most the first n bytes of str1 and st2 disregarding case
#define strncasecmp(str1, str2, n) _strnicmp(str1, str2, n)

int gettimeofday(struct timeval *tv, void *tz); 

#define SHUT_RDWR 2
#define START_NETWORKING  WSADATA wsa;WSAStartup(MAKEWORD(2,0),&wsa)
#define STOP_NETWORKING   WSACleanup()

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#define START_NETWORKING
#define STOP_NETWORKING
#define ioctlsocket(a,b,c) ioctl(a,b,c)
#define closesocket(a) close(a)
#define INVALID_SOCKET (-1)

#define INT64_FMT PRId64
typedef int SOCKET;
#define SOCKET_ERROR            (-1)  // On error, -1 is returned
#endif /* WIN32 */
#endif /* COMPAT_TIME_H */
