//this file contains implementations required for platform compatibility issues

#include "compat.h"
#include <string.h>

#if defined (_WIN32)

int gettimeofday(struct timeval *tv, void *tz)
{
    struct __timeb64 timebuffer;
    if (!tv)
        return -1;
    if (_ftime64_s( &timebuffer ))
        return -1;

    tv->tv_sec = (long) timebuffer.time;
    tv->tv_usec = timebuffer.millitm*1000;
    return 0;
}

#endif //(_WIN32)


long long time_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long result = tv.tv_sec;
	return (result * 1000 + tv.tv_usec / 1000);
}



char * strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

