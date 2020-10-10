#ifndef __vasprintf_compat_h
#define __vasprintf_compat_h

#pragma warning(push)
#pragma warning(disable: 4996)

/**
 * @file
 * @brief Do not use, json-c internal, may be changed or removed at any time.
 */

#include "snprintf_compat.h"

#include <stdlib.h>

#if !defined(HAVE_VASPRINTF)
/* CAW: compliant version of vasprintf */
static int vasprintf(char **buf, const char *fmt, va_list ap)
{
#ifndef _WIN32
	static char _T_emptybuffer = '\0';
#endif /* !defined(WIN32) */
	int chars;
	char *b;

	if (!buf)
	{
		return -1;
	}

#ifdef _WIN32
	chars = _vscprintf(fmt, ap) + 1;
#else  /* !defined(_WIN32) */
	/* CAW: RAWR! We have to hope to god here that vsnprintf doesn't overwrite
	 * our buffer like on some 64bit sun systems.... but hey, its time to move on
	 */
	chars = vsnprintf(&_T_emptybuffer, 0, fmt, ap) + 1;
	if (chars < 0)
	{
		chars *= -1;
	} /* CAW: old glibc versions have this problem */
#endif /* defined(_WIN32) */

	b = (char *)malloc(sizeof(char) * chars);
	if (!b)
	{
		return -1;
	}

	if ((chars = vsprintf(b, fmt, ap)) < 0)
	{
		free(b);
	}
	else
	{
		*buf = b;
	}

	return chars;
}
#endif /* !HAVE_VASPRINTF */

#pragma warning(pop)

#endif /* __vasprintf_compat_h */
