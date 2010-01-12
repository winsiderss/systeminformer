/*
 * "$Id: config.h.in 387 2009-04-18 17:05:52Z mike $"
 *
 * Configuration file for Mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003-2009 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Disable deprecation
#pragma warning(disable : 4996)


/*
 * Version number...
 */

#define MXML_VERSION	""


/*
 * Inline function support...
 */

#define inline


/*
 * Long long support...
 */

#define HAVE_LONG_LONG


/*
 * Do we have the snprintf() and vsnprintf() functions?
 */

#define HAVE_SNPRINTF
#define HAVE_VSNPRINTF


/*
 * Do we have the strXXX() functions?
 */

#define HAVE_STRDUP


/*
 * Do we have threading support?
 */

#undef HAVE_PTHREAD_H

char *_mxml_vstrdupf(const char *format, va_list ap);
char *_mxml_strdupf(const char *format, ...);
