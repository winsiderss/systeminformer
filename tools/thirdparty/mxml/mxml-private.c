//
// Private functions for Mini-XML, a small XML file parsing library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2003-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "mxml-private.h"


//
// Some crazy people think that unloading a shared object is a good or safe
// thing to do.  Unfortunately, most objects are simply *not* safe to unload
// and bad things *will* happen.
//
// The following mess of conditional code allows us to provide a destructor
// function in Mini-XML for our thread-global storage so that it can possibly
// be unloaded safely, although since there is no standard way to do so I
// can't even provide any guarantees that you can do it safely on all platforms.
//
// This code currently supports AIX, HP-UX, Linux, macOS, Solaris, and
// Windows.  It might work on the BSDs and IRIX, but I haven't tested that.
//

#if defined(__sun) || defined(_AIX)
#  pragma fini(_mxml_fini)
#  define _MXML_FINI _mxml_fini
#elif defined(__hpux)
#  pragma FINI _mxml_fini
#  define _MXML_FINI _mxml_fini
#elif defined(__GNUC__) // Linux and macOS
#  define _MXML_FINI __attribute((destructor)) _mxml_fini
#else
#  define _MXML_FINI _fini
#endif // __sun


//
// 'mxmlSetStringCallbacks()' - Set the string copy/free callback functions.
//
// This function sets the string copy/free callback functions for the current
// thread.  The `strcopy_cb` function makes a copy of the provided string while
// the `strfree_cb` function frees the copy.  Each callback accepts the
// `str_cbdata` pointer along with the pointer to the string:
//
// ```c
// char *my_strcopy_cb(void *cbdata, const char *s)
// {
//   ... make a copy of "s" ...
// }
//
// void my_strfree_cb(void *cbdata, char *s)
// {
//   ... release the memory used by "s" ...
// }
// ```
//
// The default `strcopy_cb` function calls `strdup` while the default
// `strfree_cb` function calls `free`.
//

void
mxmlSetStringCallbacks(
    mxml_strcopy_cb_t strcopy_cb,	// I - String copy callback function
    mxml_strfree_cb_t strfree_cb,	// I - String free callback function
    void              *str_cbdata)	// I - String callback data
{
  _mxml_global_t *global = _mxml_global();
					// Global data


  global->strcopy_cb = strcopy_cb;
  global->strfree_cb = strfree_cb;
  global->str_cbdata = str_cbdata;
}


//
// '_mxml_strcopy()' - Copy a string.
//

char *					// O - Copy of string
_mxml_strcopy(const char *s)		// I - String
{
  _mxml_global_t *global = _mxml_global();
					// Global data


  if (!s)
    return (NULL);

  if (global->strcopy_cb)
    return ((global->strcopy_cb)(global->str_cbdata, s));
  else
    return (strdup(s));
}


//
// '_mxml_strfree()' - Free a string.
//

void
_mxml_strfree(char *s)			// I - String
{
  _mxml_global_t *global = _mxml_global();
					// Global data


  if (!s)
    return;

  if (global->strfree_cb)
    (global->strfree_cb)(global->str_cbdata, s);
  else
    free((void *)s);
}


#ifdef HAVE_PTHREAD_H			// POSIX threading
#  include <pthread.h>

static int		_mxml_initialized = 0;
					// Have we been initialized?
static pthread_key_t	_mxml_key;	// Thread local storage key
static pthread_once_t	_mxml_key_once = PTHREAD_ONCE_INIT;
					// One-time initialization object
static void		_mxml_init(void);
static void		_mxml_destructor(void *g);


//
// '_mxml_destructor()' - Free memory used for globals...
//

static void
_mxml_destructor(void *g)		// I - Global data
{
  free(g);
}


//
// '_mxml_fini()' - Clean up when unloaded.
//

static void
_MXML_FINI(void)
{
  if (_mxml_initialized)
    pthread_key_delete(_mxml_key);
}


//
// '_mxml_global()' - Get global data.
//

_mxml_global_t *			// O - Global data
_mxml_global(void)
{
  _mxml_global_t	*global;	// Global data


  pthread_once(&_mxml_key_once, _mxml_init);

  if ((global = (_mxml_global_t *)pthread_getspecific(_mxml_key)) == NULL)
  {
    global = (_mxml_global_t *)calloc(1, sizeof(_mxml_global_t));
    pthread_setspecific(_mxml_key, global);
  }

  return (global);
}


//
// '_mxml_init()' - Initialize global data...
//

static void
_mxml_init(void)
{
  _mxml_initialized = 1;
  pthread_key_create(&_mxml_key, _mxml_destructor);
}


#elif defined(_WIN32) && defined(MXML1_EXPORTS) // WIN32 threading
#  include <windows.h>

static DWORD _mxml_tls_index;		// Index for global storage


//
// 'DllMain()' - Main entry for library.
//

BOOL WINAPI				// O - Success/failure
DllMain(HINSTANCE hinst,		// I - DLL module handle
        DWORD     reason,		// I - Reason
        LPVOID    reserved)		// I - Unused
{
  _mxml_global_t	*global;	// Global data


  (void)hinst;
  (void)reserved;

  switch (reason)
  {
    case DLL_PROCESS_ATTACH :		// Called on library initialization
        if ((_mxml_tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES)
          return (FALSE);
        break;

    case DLL_THREAD_DETACH :		// Called when a thread terminates
        if ((global = (_mxml_global_t *)TlsGetValue(_mxml_tls_index)) != NULL)
          free(global);
        break;

    case DLL_PROCESS_DETACH :		// Called when library is unloaded
        if ((global = (_mxml_global_t *)TlsGetValue(_mxml_tls_index)) != NULL)
          free(global);

        TlsFree(_mxml_tls_index);
        break;

    default:
        break;
  }

  return (TRUE);
}


//
// '_mxml_global()' - Get global data.
//

_mxml_global_t *			// O - Global data
_mxml_global(void)
{
  _mxml_global_t	*global;	// Global data


  if ((global = (_mxml_global_t *)TlsGetValue(_mxml_tls_index)) == NULL)
  {
    global = (_mxml_global_t *)calloc(1, sizeof(_mxml_global_t));

    TlsSetValue(_mxml_tls_index, (LPVOID)global);
  }

  return (global);
}


#else					// No threading
//
// '_mxml_global()' - Get global data.
//

_mxml_global_t *			// O - Global data
_mxml_global(void)
{
  static _mxml_global_t	global =	// Global data
  {
    NULL,				// strcopy_cb
    NULL,				// strfree_cb
    NULL,				// str_cbdata
  };


  return (&global);
}
#endif // HAVE_PTHREAD_H
