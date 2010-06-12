/*
 * "$Id: mxml-string.c 387 2009-04-18 17:05:52Z mike $"
 *
 * String functions for Mini-XML, a small XML-like file parsing library.
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
 *
 * Contents:
 *
 *   _mxml_snprintf()  - Format a string.
 *   _mxml_strdup()    - Duplicate a string.
 *   _mxml_strdupf()   - Format and duplicate a string.
 *   _mxml_vsnprintf() - Format a string into a fixed size buffer.
 *   _mxml_vstrdupf()  - Format and duplicate a string.
 */

/*
 * Include necessary headers...
 */

#include <phbase.h>
#include "config.h"

/*
 * '_mxml_strdupf()' - Format and duplicate a string.
 */

char *					/* O - New string pointer */
_mxml_strdupf(const char *format,	/* I - Printf-style format string */
              ...)			/* I - Additional arguments as needed */
{
  va_list	ap;			/* Pointer to additional arguments */
  char		*s;			/* Pointer to formatted string */


 /*
  * Get a pointer to the additional arguments, format the string,
  * and return it...
  */

  va_start(ap, format);
  s = _mxml_vstrdupf(format, ap);
  va_end(ap);

  return (s);
}

/*
 * '_mxml_vstrdupf()' - Format and duplicate a string.
 */

char *					/* O - New string pointer */
_mxml_vstrdupf(const char *format,	/* I - Printf-style format string */
               va_list    ap)		/* I - Pointer to additional arguments */
{
  int	bytes;				/* Number of bytes required */
  char	*buffer,			/* String buffer */
	temp[256];			/* Small buffer for first vsnprintf */


 /*
  * First format with a tiny buffer; this will tell us how many bytes are
  * needed...
  */

  bytes = _vsnprintf(temp, sizeof(temp), format, ap);

  if (bytes < sizeof(temp))
  {
   /*
    * Hey, the formatted string fits in the tiny buffer, so just dup that...
    */

    return (PhDuplicateAnsiStringZSafe(temp));
  }

 /*
  * Allocate memory for the whole thing and reformat to the new, larger
  * buffer...
  */

  if ((buffer = PhAllocateExSafe(bytes + 1, HEAP_ZERO_MEMORY)) != NULL)
    _vsnprintf(buffer, bytes + 1, format, ap);

 /*
  * Return the new string...
  */

  return (buffer);
}

/*
 * End of "$Id: mxml-string.c 387 2009-04-18 17:05:52Z mike $".
 */
