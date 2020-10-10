/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Enable RDRANR Hardware RNG Hash Seed */
#undef ENABLE_RDRAND

/* Define if .gnu.warning accepts long strings. */
#undef HAS_GNU_WARNING_LONG

/* Define to 1 if you have the declaration of `INFINITY', and to 0 if you
   don't. */
#if (defined(_MSC_VER) && _MSC_VER >= 1800) || defined(__MINGW32__)
#define HAVE_DECL_INFINITY 1
#endif

/* Define to 1 if you have the declaration of `isinf', and to 0 if you don't.
   */
#if (defined(_MSC_VER) && _MSC_VER >= 1800) || defined(__MINGW32__)
#define HAVE_DECL_ISINF 1
#endif

/* Define to 1 if you have the declaration of `isnan', and to 0 if you don't.
   */
#if (defined(_MSC_VER) && _MSC_VER >= 1800) || defined(__MINGW32__)
#define HAVE_DECL_ISNAN 1
#endif

/* Define to 1 if you have the declaration of `nan', and to 0 if you don't. */
#if (defined(_MSC_VER) && _MSC_VER >= 1800) || defined(__MINGW32__)
#define HAVE_DECL_NAN 1
#endif

/* Define to 1 if you have the declaration of `_finite', and to 0 if you
   don't. */
#define HAVE_DECL__FINITE 1

/* Define to 1 if you have the declaration of `_isnan', and to 0 if you don't.
   */
#define HAVE_DECL__ISNAN 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#define HAVE_DOPRNT 1

/* Define to 1 if you have the <endian.h> header file. */
#undef HAVE_ENDIAN_H

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `open' function. */
#define HAVE_OPEN 1

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC 1

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the `snprintf' function. */
#if defined(__MINGW32__)
#define HAVE_SNPRINTF 1
#else
#define HAVE_SNPRINTF 1
#endif

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strdup' function. */
//#define HAVE_STRDUP 0

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#if defined(__MINGW32__)
#define HAVE_STRNCASECMP 1
#else
#undef HAVE_STRNCASECMP
#endif

#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1

/* Define to 1 if you have the <syslog.h> header file. */
#undef HAVE_SYSLOG_H

/* Define to 1 if you have the <sys/cdefs.h> header file. */
#define HAVE_SYS_CDEFS_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#if defined(__MINGW32__)
#define HAVE_SYS_PARAM_H 1
#else
#undef HAVE_SYS_PARAM_H
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#if defined(__MINGW32__)
#define HAVE_UNISTD_H 1
#else
#undef HAVE_UNISTD_H
#endif

/* Define to 1 if you have the `vasprintf' function. */
#if defined(__MINGW32__)
#define HAVE_VASPRINTF 1
#else
#undef HAVE_VASPRINTF
#endif

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `vsyslog' function. */
#undef HAVE_VSYSLOG

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "json-c"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "json-c@googlegroups.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "JSON C Library"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "JSON C Library 0.14"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "json-c"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://github.com/json-c/json-c"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.14"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "0.14"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
