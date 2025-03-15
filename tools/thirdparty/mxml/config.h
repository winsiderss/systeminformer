//
// Visual Studio configuration file for Mini-XML, a small XML file parsing
// library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2003-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef MXML_CONFIG_H
#  define MXML_CONFIG_H

//
// Beginning with VC2005, Microsoft breaks ISO C and POSIX conformance
// by deprecating a number of functions in the name of security, even
// when many of the affected functions are otherwise completely secure.
// The _CRT_SECURE_NO_DEPRECATE definition ensures that we won't get
// warnings from their use...
//
// Then Microsoft decided that they should ignore this in VC2008 and use
// yet another define (_CRT_SECURE_NO_WARNINGS) instead...
//

#  define _CRT_SECURE_NO_DEPRECATE
//#  define _CRT_SECURE_NO_WARNINGS
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <stdarg.h>
#  include <ctype.h>
#  include <io.h>


//
// Microsoft also renames the POSIX functions to _name, and introduces
// a broken compatibility layer using the original names.  As a result,
// random crashes can occur when, for example, strdup() allocates memory
// from a different heap than used by malloc() and free().
//
// To avoid moronic problems like this, we #define the POSIX function
// names to the corresponding non-standard Microsoft names.
//

#  define close		_close
#  define open		_open
#  define read	        _read
#  define snprintf 	_snprintf
#  define strdup	_strdup
#  define vsnprintf 	_vsnprintf
#  define write		_write


//
// Version number
//

#  define MXML_VERSION "Mini-XML v4.0.4"


//
// Inline function support
//

#  define inline _inline


//
// Long long support
//

#  define HAVE_LONG_LONG_INT 1


//
// Have <pthread.h>?
//

//#  undef HAVE_PTHREAD_H


#endif // !MXML_CONFIG_H
