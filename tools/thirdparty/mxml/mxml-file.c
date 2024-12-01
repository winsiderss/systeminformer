//
// File loading code for Mini-XML, a small XML file parsing library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2003-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef _WIN32
#  include <unistd.h>
#endif // !_WIN32
#include "mxml-private.h"


//
// Local types...
//

typedef enum _mxml_encoding_e		// Character encoding
{
  _MXML_ENCODING_UTF8,			// UTF-8
  _MXML_ENCODING_UTF16BE,		// UTF-16 Big-Endian
  _MXML_ENCODING_UTF16LE		// UTF-16 Little-Endian
} _mxml_encoding_t;

typedef struct _mxml_stringbuf_s	// String buffer
{
  char		*buffer,		// Buffer
		*bufptr;		// Pointer into buffer
  size_t	bufsize;		// Size of buffer
  bool		bufalloc;		// Allocate buffer?
} _mxml_stringbuf_t;


//
// Macro to test for a bad XML character...
//

#define mxml_bad_char(ch) ((ch) < ' ' && (ch) != '\n' && (ch) != '\r' && (ch) != '\t')


//
// Local functions...
//

static bool		mxml_add_char(mxml_options_t *options, int ch, char **ptr, char **buffer, size_t *bufsize);
static int		mxml_get_entity(mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata, _mxml_encoding_t *encoding, mxml_node_t *parent, int *line);
static int		mxml_getc(mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata, _mxml_encoding_t *encoding);
static inline int	mxml_isspace(int ch)
			{
			  return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
			}
static mxml_node_t	*mxml_load_data(mxml_node_t *top, mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata);
static int		mxml_parse_element(mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata, mxml_node_t *node, _mxml_encoding_t *encoding, int *line);
static size_t		mxml_read_cb_fd(int *fd, void *buffer, size_t bytes);
static size_t		mxml_read_cb_file(FILE *fp, void *buffer, size_t bytes);
static size_t		mxml_read_cb_string(_mxml_stringbuf_t *sb, void *buffer, size_t bytes);
static double		mxml_strtod(mxml_options_t *options, const char *buffer, char **bufptr);
static size_t		mxml_io_cb_fd(int *fd, void *buffer, size_t bytes);
static size_t		mxml_io_cb_file(FILE *fp, void *buffer, size_t bytes);
static size_t		mxml_io_cb_string(_mxml_stringbuf_t *sb, void *buffer, size_t bytes);
static int		mxml_write_node(mxml_node_t *node, mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata, int col);
static int		mxml_write_string(const char *s, mxml_io_cb_t io_cb, void *io_cbdata, bool use_entities, int col);
static int		mxml_write_ws(mxml_node_t *node, mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata, mxml_ws_t ws, int col);


//
// 'mxmlLoadFd()' - Load a file descriptor into an XML node tree.
//
// This function loads the file descriptor `fd` into an XML node tree.  The
// nodes in the specified file are added to the specified node `top` - if `NULL`
// the XML file MUST be well-formed with a single parent processing instruction
// node like `<?xml version="1.0"?>` at the start of the file.
//
// Load options are provides via the `options` argument.  If `NULL`, all values
// will be loaded into `MXML_TYPE_TEXT` nodes.  Use the @link mxmlOptionsNew@
// function to create options when loading XML data.
//

mxml_node_t *				// O - First node or `NULL` if the file could not be read.
mxmlLoadFd(
    mxml_node_t    *top,		// I - Top node
    mxml_options_t *options,		// I - Options
    int            fd)			// I - File descriptor to read from
{
  // Range check input...
  if (fd < 0)
    return (NULL);

  // Read the XML data...
  return (mxml_load_data(top, options, (mxml_io_cb_t)mxml_read_cb_fd, &fd));
}


//
// 'mxmlLoadFile()' - Load a file into an XML node tree.
//
// This function loads the `FILE` pointer `fp` into an XML node tree.  The
// nodes in the specified file are added to the specified node `top` - if `NULL`
// the XML file MUST be well-formed with a single parent processing instruction
// node like `<?xml version="1.0"?>` at the start of the file.
//
// Load options are provides via the `options` argument.  If `NULL`, all values
// will be loaded into `MXML_TYPE_TEXT` nodes.  Use the @link mxmlOptionsNew@
// function to create options when loading XML data.
//

mxml_node_t *				// O - First node or `NULL` if the file could not be read.
mxmlLoadFile(
    mxml_node_t    *top,		// I - Top node
    mxml_options_t *options,		// I - Options
    FILE           *fp)			// I - File to read from
{
  // Range check input...
  if (!fp)
    return (NULL);

  // Read the XML data...
  return (mxml_load_data(top, options, (mxml_io_cb_t)mxml_read_cb_file, fp));
}


//
// 'mxmlLoadFilename()' - Load a file into an XML node tree.
//
// This function loads the named file `filename` into an XML node tree.  The
// nodes in the specified file are added to the specified node `top` - if `NULL`
// the XML file MUST be well-formed with a single parent processing instruction
// node like `<?xml version="1.0"?>` at the start of the file.
//
// Load options are provides via the `options` argument.  If `NULL`, all values
// will be loaded into `MXML_TYPE_TEXT` nodes.  Use the @link mxmlOptionsNew@
// function to create options when loading XML data.
//

mxml_node_t *				// O - First node or `NULL` if the file could not be read.
mxmlLoadFilename(
    mxml_node_t    *top,		// I - Top node
    mxml_options_t *options,		// I - Options
    const char     *filename)		// I - File to read from
{
  FILE		*fp;			// File pointer
  mxml_node_t	*ret;			// Node


  // Range check input...
  if (!filename)
    return (NULL);

  // Open the file...
  if ((fp = fopen(filename, "r")) == NULL)
    return (NULL);

  // Read the XML data...
  ret = mxml_load_data(top, options, (mxml_io_cb_t)mxml_read_cb_file, fp);

  // Close the file and return...
  fclose(fp);

  return (ret);
}


//
// 'mxmlLoadIO()' - Load an XML node tree using a read callback.
//
// This function loads data into an XML node tree using a read callback.  The
// nodes in the specified file are added to the specified node `top` - if `NULL`
// the XML file MUST be well-formed with a single parent processing instruction
// node like `<?xml version="1.0"?>` at the start of the file.
//
// Load options are provides via the `options` argument.  If `NULL`, all values
// will be loaded into `MXML_TYPE_TEXT` nodes.  Use the @link mxmlOptionsNew@
// function to create options when loading XML data.
//
// The read callback function `io_cb` is called to read a number of bytes from
// the source.  The callback data pointer `io_cbdata` is passed to the read
// callback with a pointer to a buffer and the maximum number of bytes to read,
// for example:
//
// ```c
// size_t my_io_cb(void *cbdata, void *buffer, size_t bytes)
// {
//   ... copy up to "bytes" bytes into buffer ...
//   ... return the number of bytes "read" or 0 on error ...
// }
// ```
//

mxml_node_t *				// O - First node or `NULL` if the file could not be read.
mxmlLoadIO(
    mxml_node_t    *top,		// I - Top node
    mxml_options_t *options,		// I - Options
    mxml_io_cb_t   io_cb,		// I - Read callback function
    void           *io_cbdata)		// I - Read callback data
{
  // Range check input...
  if (!io_cb)
    return (NULL);

  // Read the XML data...
  return (mxml_load_data(top, options, io_cb, io_cbdata));
}


//
// 'mxmlLoadString()' - Load a string into an XML node tree.
//
// This function loads the string into an XML node tree.  The nodes in the
// specified file are added to the specified node `top` - if `NULL` the XML file
// MUST be well-formed with a single parent processing instruction node like
// `<?xml version="1.0"?>` at the start of the file.
//
// Load options are provides via the `options` argument.  If `NULL`, all values
// will be loaded into `MXML_TYPE_TEXT` nodes.  Use the @link mxmlOptionsNew@
// function to create options when loading XML data.
//

mxml_node_t *				// O - First node or `NULL` if the string has errors.
mxmlLoadString(
    mxml_node_t    *top,		// I - Top node
    mxml_options_t *options,		// I - Options
    const char     *s)			// I - String to load
{
  _mxml_stringbuf_t	sb;		// String buffer


  // Range check input...
  if (!s)
    return (NULL);

  // Setup string buffer...
  sb.buffer   = (char *)s;
  sb.bufptr   = (char *)s;
  sb.bufsize  = strlen(s);
  sb.bufalloc = false;

  // Read the XML data...
  return (mxml_load_data(top, options, (mxml_io_cb_t)mxml_read_cb_string, &sb));
}


//
// 'mxmlSaveAllocString()' - Save an XML tree to an allocated string.
//
// This function saves the XML tree `node` to an allocated string.  The string
// should be freed using `free` (or the string free callback set using
// @link mxmlSetStringCallbacks@) when you are done with it.
//
// `NULL` is returned if the node would produce an empty string or if the string
// cannot be allocated.
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

char *					// O - Allocated string or `NULL`
mxmlSaveAllocString(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options)		// I - Options
{
  _mxml_stringbuf_t	sb;		// String buffer


  // Setup a string buffer
  if ((sb.buffer = malloc(1024)) == NULL)
    return (NULL);

  sb.bufptr   = sb.buffer;
  sb.bufsize  = 1024;
  sb.bufalloc = true;

  // Write the top node...
  if (mxml_write_node(node, options, (mxml_io_cb_t)mxml_io_cb_string, &sb, 0) < 0)
  {
    free(sb.buffer);
    return (NULL);
  }

  // Nul-terminate the string...
  *(sb.bufptr) = '\0';

  // Return the allocated string...
  return (sb.buffer);
}


//
// 'mxmlSaveFd()' - Save an XML tree to a file descriptor.
//
// This function saves the XML tree `node` to a file descriptor.
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

bool					// O - `true` on success, `false` on error.
mxmlSaveFd(mxml_node_t    *node,	// I - Node to write
           mxml_options_t *options,	// I - Options
           int            fd)		// I - File descriptor to write to
{
  int		col;			// Final column


  // Write the node...
  if ((col = mxml_write_node(node, options, (mxml_io_cb_t)mxml_io_cb_fd, &fd, 0)) < 0)
    return (false);

  // Make sure the file ends with a newline...
  if (col > 0)
  {
    if (write(fd, "\n", 1) < 0)
      return (false);
  }

  return (true);
}


//
// 'mxmlSaveFile()' - Save an XML tree to a file.
//
// This function saves the XML tree `node` to a stdio `FILE`.
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

bool					// O - `true` on success, `false` on error.
mxmlSaveFile(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options,		// I - Options
    FILE           *fp)			// I - File to write to
{
  int		col;			// Final column


  // Write the node...
  if ((col = mxml_write_node(node, options, (mxml_io_cb_t)mxml_io_cb_file, fp, 0)) < 0)
    return (false);

  // Make sure the file ends with a newline...
  if (col > 0)
  {
    if (putc('\n', fp) < 0)
      return (false);
  }

  return (true);
}


//
// 'mxmlSaveFilename()' - Save an XML tree to a file.
//
// This function saves the XML tree `node` to a named file.
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

bool					// O - `true` on success, `false` on error.
mxmlSaveFilename(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options,		// I - Options
    const char     *filename)		// I - File to write to
{
  bool		ret = true;		// Return value
  FILE		*fp;			// File pointer
  int		col;			// Final column


  // Open the file...
  if ((fp = fopen(filename, "w")) == NULL)
    return (false);

  // Write the node...
  if ((col = mxml_write_node(node, options, (mxml_io_cb_t)mxml_io_cb_file, fp, 0)) < 0)
  {
    ret = false;
  }
  else if (col > 0)
  {
    // Make sure the file ends with a newline...
    if (putc('\n', fp) < 0)
      ret = false;
  }

  fclose(fp);

  return (ret);
}


//
// 'mxmlSaveIO()' - Save an XML tree using a callback.
//
// This function saves the XML tree `node` using a write callback function
// `io_cb`.  The write callback is called with the callback data pointer
// `io_cbdata`, a buffer pointer, and the number of bytes to write, for
// example:
//
// ```c
// size_t my_io_cb(void *cbdata, const void *buffer, size_t bytes)
// {
//   ... write/copy bytes from buffer to the output ...
//   ... return the number of bytes written/copied or 0 on error ...
// }
// ```
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

bool					// O - `true` on success, `false` on error.
mxmlSaveIO(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options,		// I - Options
    mxml_io_cb_t   io_cb,		// I - Write callback function
    void           *io_cbdata)		// I - Write callback data
{
  int		col;			// Final column


  // Range check input...
  if (!node || !io_cb)
    return (false);

  // Write the node...
  if ((col = mxml_write_node(node, options, io_cb, io_cbdata, 0)) < 0)
    return (false);

  if (col > 0)
  {
    // Make sure the file ends with a newline...
    if ((io_cb)(io_cbdata, "\n", 1) != 1)
      return (false);
  }

  return (true);
}


//
// 'mxmlSaveString()' - Save an XML node tree to a string.
//
// This function saves the XML tree `node` to a fixed-size string buffer.
//
// Save options are provides via the `options` argument.  If `NULL`, the XML
// output will be wrapped at column 72 with no additional whitespace.  Use the
// @link mxmlOptionsNew@ function to create options for saving XML data.
//

size_t					// O - Size of string
mxmlSaveString(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options,		// I - Options
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  _mxml_stringbuf_t	sb;		// String buffer


  // Setup the string buffer...
  sb.buffer   = buffer;
  sb.bufptr   = buffer;
  sb.bufsize  = bufsize;
  sb.bufalloc = false;

  // Write the node...
  if (mxml_write_node(node, options, (mxml_io_cb_t)mxml_io_cb_string, &sb, 0) < 0)
    return (false);

  // Nul-terminate the string...
  if (sb.bufptr < (sb.buffer + sb.bufsize))
    *(sb.bufptr) = '\0';

  // Return the number of characters...
  return ((size_t)(sb.bufptr - sb.buffer));
}


//
// 'mxml_add_char()' - Add a character to a buffer, expanding as needed.
//

static bool				// O  - `true` on success, `false` on error
mxml_add_char(mxml_options_t *options,	// I - Options
              int            ch,	// I  - Character to add
              char           **bufptr,	// IO - Current position in buffer
	      char           **buffer,	// IO - Current buffer
	      size_t         *bufsize)	// IO - Current buffer size
{
  char	*newbuffer;			// New buffer value


  if (*bufptr >= (*buffer + *bufsize - 4))
  {
    // Increase the size of the buffer...
    if (*bufsize < 1024)
      (*bufsize) *= 2;
    else
      (*bufsize) += 1024;

    if ((newbuffer = realloc(*buffer, *bufsize)) == NULL)
    {
      _mxml_error(options, "Unable to expand string buffer to %lu bytes.", (unsigned long)*bufsize);

      return (false);
    }

    *bufptr = newbuffer + (*bufptr - *buffer);
    *buffer = newbuffer;
  }

  if (ch < 0x80)
  {
    // Single byte ASCII...
    *(*bufptr)++ = ch;
  }
  else if (ch < 0x800)
  {
    // Two-byte UTF-8...
    *(*bufptr)++ = 0xc0 | (ch >> 6);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }
  else if (ch < 0x10000)
  {
    // Three-byte UTF-8...
    *(*bufptr)++ = 0xe0 | (ch >> 12);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }
  else
  {
    // Four-byte UTF-8...
    *(*bufptr)++ = 0xf0 | (ch >> 18);
    *(*bufptr)++ = 0x80 | ((ch >> 12) & 0x3f);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }

  return (true);
}


//
// 'mxml_get_entity()' - Get the character corresponding to an entity...
//

static int				// O  - Character value or `EOF` on error
mxml_get_entity(
    mxml_options_t   *options,		// I  - Options
    mxml_io_cb_t     io_cb,		// I  - Read callback function
    void             *io_cbdata,	// I  - Read callback data
    _mxml_encoding_t *encoding,		// IO - Character encoding
    mxml_node_t      *parent,		// I  - Parent node
    int              *line)		// IO - Current line number
{
  int	ch;				// Current character
  char	entity[64],			// Entity string
	*entptr;			// Pointer into entity


  // Read a HTML character entity of the form "&NAME;", "&#NUMBER;", or "&#xHEX"...
  entptr = entity;

  while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
  {
    if (ch > 126 || (!isalnum(ch) && ch != '#'))
    {
      break;
    }
    else if (entptr < (entity + sizeof(entity) - 1))
    {
      *entptr++ = ch;
    }
    else
    {
      _mxml_error(options, "Entity name too long under parent <%s> on line %d.", mxmlGetElement(parent), *line);
      break;
    }
  }

  *entptr = '\0';

  if (ch != ';')
  {
    _mxml_error(options, "Character entity '%s' not terminated under parent <%s> on line %d.", entity, mxmlGetElement(parent), *line);

    if (ch == '\n')
      (*line)++;

    return (EOF);
  }

  if ((ch = _mxml_entity_value(options, entity)) < 0)
  {
    _mxml_error(options, "Entity '&%s;' not supported under parent <%s> on line %d.", entity, mxmlGetElement(parent), *line);
    return (EOF);
  }

  if (mxml_bad_char(ch))
  {
    _mxml_error(options, "Bad control character 0x%02x under parent <%s> on line %d not allowed by XML standard.", ch, mxmlGetElement(parent), *line);
    return (EOF);
  }

  return (ch);
}


//
// 'mxml_getc()' - Read a character from a file descriptor.
//

static int				// O  - Character or `EOF`
mxml_getc(mxml_options_t   *options,	// I  - Options
          mxml_io_cb_t     io_cb,	// I  - Read callback function
          void             *io_cbdata,	// I  - Read callback data
          _mxml_encoding_t *encoding)	// IO - Encoding
{
  int		ch;			// Current character
  unsigned char	buffer[4];		// Read buffer


  // Grab the next character...
  read_first_byte:

  if ((io_cb)(io_cbdata, buffer, 1) != 1)
    return (EOF);

  ch = buffer[0];

  switch (*encoding)
  {
    case _MXML_ENCODING_UTF8 :
        // Got a UTF-8 character; convert UTF-8 to Unicode and return...
	if (!(ch & 0x80))
	{
	  // ASCII
	  break;
        }
	else if (ch == 0xfe)
	{
	  // UTF-16 big-endian BOM?
	  if ((io_cb)(io_cbdata, buffer + 1, 1) != 1)
	    return (EOF);

	  if (buffer[1] != 0xff)
	    return (EOF);

          // Yes, switch to UTF-16 BE and try reading again...
	  *encoding = _MXML_ENCODING_UTF16BE;

	  goto read_first_byte;
	}
	else if (ch == 0xff)
	{
	  // UTF-16 little-endian BOM?
	  if ((io_cb)(io_cbdata, buffer + 1, 1) != 1)
	    return (EOF);

	  if (buffer[1] != 0xfe)
	    return (EOF);

          // Yes, switch to UTF-16 LE and try reading again...
	  *encoding = _MXML_ENCODING_UTF16LE;

	  goto read_first_byte;
	}
	else if ((ch & 0xe0) == 0xc0)
	{
	  // Two-byte value...
	  if ((io_cb)(io_cbdata, buffer + 1, 1) != 1)
	    return (EOF);

	  if ((buffer[1] & 0xc0) != 0x80)
	    return (EOF);

	  ch = ((ch & 0x1f) << 6) | (buffer[1] & 0x3f);

	  if (ch < 0x80)
	  {
	    _mxml_error(options, "Invalid UTF-8 sequence for character 0x%04x.", ch);
	    return (EOF);
	  }
	}
	else if ((ch & 0xf0) == 0xe0)
	{
	  // Three-byte value...
	  if ((io_cb)(io_cbdata, buffer + 1, 2) != 2)
	    return (EOF);

	  if ((buffer[1] & 0xc0) != 0x80 || (buffer[2] & 0xc0) != 0x80)
	    return (EOF);

	  ch = ((ch & 0x0f) << 12) | ((buffer[1] & 0x3f) << 6) | (buffer[2] & 0x3f);

	  if (ch < 0x800)
	  {
	    _mxml_error(options, "Invalid UTF-8 sequence for character 0x%04x.", ch);
	    return (EOF);
	  }

          // Ignore (strip) Byte Order Mark (BOM)...
	  if (ch == 0xfeff)
	    goto read_first_byte;
	}
	else if ((ch & 0xf8) == 0xf0)
	{
	  // Four-byte value...
	  if ((io_cb)(io_cbdata, buffer + 1, 3) != 3)
	    return (EOF);

	  if ((buffer[1] & 0xc0) != 0x80 || (buffer[2] & 0xc0) != 0x80 || (buffer[3] & 0xc0) != 0x80)
	    return (EOF);

	  ch = ((ch & 0x07) << 18) | ((buffer[1] & 0x3f) << 12) | ((buffer[2] & 0x3f) << 6) | (buffer[3] & 0x3f);

	  if (ch < 0x10000)
	  {
	    _mxml_error(options, "Invalid UTF-8 sequence for character 0x%04x.", ch);
	    return (EOF);
	  }
	}
	else
	{
	  return (EOF);
	}
	break;

    case _MXML_ENCODING_UTF16BE :
        // Read UTF-16 big-endian char...
	if ((io_cb)(io_cbdata, buffer + 1, 1) != 1)
	  return (EOF);

	ch = (ch << 8) | buffer[1];

        if (ch >= 0xd800 && ch <= 0xdbff)
	{
	  // Multi-word UTF-16 char...
          int lch;			// Lower bits

	  if ((io_cb)(io_cbdata, buffer + 2, 2) != 2)
	    return (EOF);

	  lch = (buffer[2] << 8) | buffer[3];

          if (lch < 0xdc00 || lch >= 0xdfff)
	    return (EOF);

          ch = (((ch & 0x3ff) << 10) | (lch & 0x3ff)) + 0x10000;
	}
	break;

    case _MXML_ENCODING_UTF16LE :
        // Read UTF-16 little-endian char...
	if ((io_cb)(io_cbdata, buffer + 1, 1) != 1)
	  return (EOF);

	ch |= buffer[1] << 8;

        if (ch >= 0xd800 && ch <= 0xdbff)
	{
	  // Multi-word UTF-16 char...
          int lch;			// Lower bits

	  if ((io_cb)(io_cbdata, buffer + 2, 2) != 2)
	    return (EOF);

	  lch = (buffer[3] << 8) | buffer[2];

          if (lch < 0xdc00 || lch >= 0xdfff)
	    return (EOF);

          ch = (((ch & 0x3ff) << 10) | (lch & 0x3ff)) + 0x10000;
	}
	break;
  }

  if (mxml_bad_char(ch))
  {
    _mxml_error(options, "Bad control character 0x%02x not allowed by XML standard.", ch);
    return (EOF);
  }

  return (ch);
}


//
// 'mxml_load_data()' - Load data into an XML node tree.
//

static mxml_node_t *			// O - First node or `NULL` if the XML could not be read.
mxml_load_data(
    mxml_node_t     *top,		// I - Top node
    mxml_options_t  *options,		// I - Options
    mxml_io_cb_t    io_cb,		// I - Read callback function
    void            *io_cbdata)		// I - Read callback data
{
  mxml_node_t	*node = NULL,		// Current node
		*first = NULL,		// First node added
		*parent = NULL;		// Current parent node
  int		line = 1,		// Current line number
		ch;			// Character from file
  bool		whitespace = false;	// Whitespace seen?
  char		*buffer,		// String buffer
		*bufptr;		// Pointer into buffer
  size_t	bufsize;		// Size of buffer
  mxml_type_t	type;			// Current node type
  _mxml_encoding_t encoding = _MXML_ENCODING_UTF8;
					// Character encoding
  static const char * const types[] =	// Type strings...
		{
		  "MXML_TYPE_CDATA",	// CDATA
		  "MXML_TYPE_COMMENT",	// Comment
		  "MXML_TYPE_DECLARATION",// Declaration
		  "MXML_TYPE_DIRECTIVE",// Processing instruction/directive
		  "MXML_TYPE_ELEMENT",	// XML element with attributes
		  "MXML_TYPE_INTEGER",	// Integer value
		  "MXML_TYPE_OPAQUE",	// Opaque string
		  "MXML_TYPE_REAL",	// Real value
		  "MXML_TYPE_TEXT",	// Text fragment
		  "MXML_TYPE_CUSTOM"	// Custom data
		};


  // Read elements and other nodes from the file...
  if ((buffer = malloc(64)) == NULL)
  {
    _mxml_error(options, "Unable to allocate string buffer.");
    return (NULL);
  }

  bufsize    = 64;
  bufptr     = buffer;
  parent     = top;
  first      = NULL;

  if (options && options->type_cb && parent)
    type = (options->type_cb)(options->type_cbdata, parent);
  else if (options && !options->type_cb)
    type = options->type_value;
  else
    type = MXML_TYPE_IGNORE;

  if ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) == EOF)
  {
    free(buffer);
    return (NULL);
  }
  else if (ch != '<' && !top)
  {
    free(buffer);
    _mxml_error(options, "XML does not start with '<' (saw '%c').", ch);
    return (NULL);
  }

  do
  {
    if ((ch == '<' || (mxml_isspace(ch) && type != MXML_TYPE_OPAQUE && type != MXML_TYPE_CUSTOM)) && bufptr > buffer)
    {
      // Add a new value node...
      *bufptr = '\0';

      switch (type)
      {
	case MXML_TYPE_INTEGER :
            node = mxmlNewInteger(parent, strtol(buffer, &bufptr, 0));
	    break;

	case MXML_TYPE_OPAQUE :
            node = mxmlNewOpaque(parent, buffer);
	    break;

	case MXML_TYPE_REAL :
            node = mxmlNewReal(parent, mxml_strtod(options, buffer, &bufptr));
	    break;

	case MXML_TYPE_TEXT :
            node = mxmlNewText(parent, whitespace, buffer);
	    break;

	case MXML_TYPE_CUSTOM :
	    if (options && options->custload_cb)
	    {
	      // Use the callback to fill in the custom data...
              node = mxmlNewCustom(parent, /*data*/NULL, /*free_cb*/NULL, /*free_cbdata*/NULL);

	      if (!(options->custload_cb)(options->cust_cbdata, node, buffer))
	      {
	        _mxml_error(options, "Bad custom value '%s' in parent <%s> on line %d.", buffer, parent ? parent->value.element.name : "null", line);
		mxmlDelete(node);
		node = NULL;
	      }
	      break;
	    }

        default : // Ignore...
	    node = NULL;
	    break;
      }

      if (*bufptr)
      {
        // Bad integer/real number value...
        _mxml_error(options, "Bad %s value '%s' in parent <%s> on line %d.", type == MXML_TYPE_INTEGER ? "integer" : "real", buffer, parent ? parent->value.element.name : "null", line);
	break;
      }

      MXML_DEBUG("mxml_load_data: node=%p(%s), parent=%p\n", node, buffer, parent);

      bufptr     = buffer;
      whitespace = mxml_isspace(ch) && type == MXML_TYPE_TEXT;

      if (!node && type != MXML_TYPE_IGNORE)
      {
        // Print error and return...
	_mxml_error(options, "Unable to add value node of type %s to parent <%s> on line %d.", types[type], parent ? parent->value.element.name : "null", line);
	goto error;
      }

      if (options && options->sax_cb)
      {
        if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_DATA))
          goto error;

        if (!mxmlRelease(node))
          node = NULL;
      }

      if (!first && node)
        first = node;
    }
    else if (mxml_isspace(ch) && type == MXML_TYPE_TEXT)
    {
      whitespace = true;
    }

    if (ch == '\n')
      line ++;

    // Add lone whitespace node if we have an element and existing whitespace...
    if (ch == '<' && whitespace && type == MXML_TYPE_TEXT)
    {
      if (parent)
      {
	node = mxmlNewText(parent, whitespace, "");

	if (options && options->sax_cb)
	{
	  if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_DATA))
	    goto error;

	  if (!mxmlRelease(node))
	    node = NULL;
	}

	if (!first && node)
	  first = node;
      }

      whitespace = false;
    }

    if (ch == '<')
    {
      // Start of open/close tag...
      bufptr = buffer;

      while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF)
      {
        if (mxml_isspace(ch) || ch == '>' || (ch == '/' && bufptr > buffer))
        {
	  break;
	}
	else if (ch == '<')
	{
	  _mxml_error(options, "Bare < in element.");
	  goto error;
	}
	else if (ch == '&')
	{
	  if ((ch = mxml_get_entity(options, io_cb, io_cbdata, &encoding, parent, &line)) == EOF)
	    goto error;

	  if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	    goto error;
	}
	else if (ch < '0' && ch != '!' && ch != '-' && ch != '.' && ch != '/')
	{
	  goto error;
	}
	else if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	{
	  goto error;
	}
	else if (((bufptr - buffer) == 1 && buffer[0] == '?') || ((bufptr - buffer) == 3 && !strncmp(buffer, "!--", 3)) || ((bufptr - buffer) == 8 && !strncmp(buffer, "![CDATA[", 8)))
	{
	  break;
	}

	if (ch == '\n')
	  line ++;
      }

      *bufptr = '\0';

      if (!strcmp(buffer, "!--"))
      {
        // Gather rest of comment...
	while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF)
	{
	  if (ch == '>' && bufptr > (buffer + 4) && bufptr[-3] != '-' && bufptr[-2] == '-' && bufptr[-1] == '-')
	    break;
	  else if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	    goto error;

	  if (ch == '\n')
	    line ++;
	}

        // Error out if we didn't get the whole comment...
        if (ch != '>')
	{
	  // Print error and return...
	  _mxml_error(options, "Early EOF in comment node on line %d.", line);
	  goto error;
	}

        // Otherwise add this as an element under the current parent...
	bufptr[-2] = '\0';

        if (!parent && first)
	{
	  // There can only be one root element!
	  _mxml_error(options, "<%s--> cannot be a second root node after <%s> on line %d.", buffer, first->value.element.name, line);
          goto error;
	}

	if ((node = mxmlNewComment(parent, buffer + 3)) == NULL)
	{
	  // Just print error for now...
	  _mxml_error(options, "Unable to add comment node to parent <%s> on line %d.", parent ? parent->value.element.name : "null", line);
	  break;
	}

	MXML_DEBUG("mxml_load_data: node=%p(<%s-->), parent=%p\n", node, buffer, parent);

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_COMMENT))
	    goto error;

          if (!mxmlRelease(node))
            node = NULL;
        }

	if (node && !first)
	  first = node;
      }
      else if (!strcmp(buffer, "![CDATA["))
      {
        // Gather CDATA section...
	while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF)
	{
	  if (ch == '>' && !strncmp(bufptr - 2, "]]", 2))
	  {
	    // Drop terminator from CDATA string...
	    bufptr[-2] = '\0';
	    break;
	  }
	  else if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	  {
	    goto error;
	  }

	  if (ch == '\n')
	    line ++;
	}

        // Error out if we didn't get the whole comment...
        if (ch != '>')
	{
	  // Print error and return...
	  _mxml_error(options, "Early EOF in CDATA node on line %d.", line);
	  goto error;
	}

        // Otherwise add this as an element under the current parent...
	bufptr[-2] = '\0';

        if (!parent && first)
	{
	  // There can only be one root element!
	  _mxml_error(options, "<%s]]> cannot be a second root node after <%s> on line %d.", buffer, first->value.element.name, line);
          goto error;
	}

	if ((node = mxmlNewCDATA(parent, buffer + 8)) == NULL)
	{
	  // Print error and return...
	  _mxml_error(options, "Unable to add CDATA node to parent <%s> on line %d.", parent ? parent->value.element.name : "null", line);
	  goto error;
	}

	MXML_DEBUG("mxml_load_data: node=%p(<%s]]>), parent=%p\n", node, buffer, parent);

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_CDATA))
	    goto error;

          if (!mxmlRelease(node))
            node = NULL;
        }

	if (node && !first)
	  first = node;
      }
      else if (buffer[0] == '?')
      {
        // Gather rest of processing instruction...
	while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF)
	{
	  if (ch == '>' && bufptr > buffer && bufptr[-1] == '?')
	    break;
	  else if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	    goto error;

	  if (ch == '\n')
	    line ++;
	}

        // Error out if we didn't get the whole processing instruction...
        if (ch != '>')
	{
	  // Print error and return...
	  _mxml_error(options, "Early EOF in processing instruction node on line %d.", line);
	  goto error;
	}

        // Otherwise add this as an element under the current parent...
	bufptr[-1] = '\0';

        if (!parent && first)
	{
	  // There can only be one root element!
	  _mxml_error(options, "<%s?> cannot be a second root node after <%s> on line %d.", buffer, first->value.element.name, line);
          goto error;
	}

	if ((node = mxmlNewDirective(parent, buffer + 1)) == NULL)
	{
	  // Print error and return...
	  _mxml_error(options, "Unable to add processing instruction node to parent <%s> on line %d.", parent ? parent->value.element.name : "null", line);
	  goto error;
	}

	MXML_DEBUG("mxml_load_data: node=%p(<%s?>), parent=%p\n", node, buffer, parent);

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_DIRECTIVE))
	    goto error;

          if (strncmp(node->value.directive, "xml ", 4) && !mxmlRelease(node))
            node = NULL;
        }

        if (node)
	{
	  if (!first)
            first = node;

	  if (!parent)
	  {
	    parent = node;

	    if (options && options->type_cb)
	      type = (options->type_cb)(options->type_cbdata, parent);
	    else if (options)
	      type = options->type_value;
	    else
	      type = MXML_TYPE_TEXT;
	  }
	}
      }
      else if (buffer[0] == '!')
      {
        // Gather rest of declaration...
	do
	{
	  if (ch == '>')
	  {
	    break;
	  }
	  else
	  {
            if (ch == '&')
            {
	      if ((ch = mxml_get_entity(options, io_cb, io_cbdata, &encoding, parent, &line)) == EOF)
		goto error;
            }

	    if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	      goto error;
	  }

	  if (ch == '\n')
	    line ++;
	}
        while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF);

        // Error out if we didn't get the whole declaration...
        if (ch != '>')
	{
	  // Print error and return...
	  _mxml_error(options, "Early EOF in declaration node on line %d.", line);
	  goto error;
	}

        // Otherwise add this as an element under the current parent...
	*bufptr = '\0';

        if (!parent && first)
	{
	  // There can only be one root element!
	  _mxml_error(options, "<%s> cannot be a second root node after <%s> on line %d.", buffer, first->value.element.name, line);
          goto error;
	}

	if ((node = mxmlNewDeclaration(parent, buffer + 1)) == NULL)
	{
	  // Print error and return...
	  _mxml_error(options, "Unable to add declaration node to parent <%s> on line %d.", parent ? parent->value.element.name : "null", line);
	  goto error;
	}

	MXML_DEBUG("mxml_load_data: node=%p(<%s>), parent=%p\n", node, buffer, parent);

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_DECLARATION))
	    goto error;

          if (!mxmlRelease(node))
            node = NULL;
        }

        if (node)
	{
	  if (!first)
            first = node;

	  if (!parent)
	  {
	    parent = node;

	    if (options && options->type_cb)
	      type = (options->type_cb)(options->type_cbdata, parent);
	    else if (options)
	      type = options->type_value;
	    else
	      type = MXML_TYPE_TEXT;
	  }
	}
      }
      else if (buffer[0] == '/')
      {
        // Handle close tag...
	MXML_DEBUG("mxml_load_data: <%s>, parent=%p\n", buffer, parent);

        if (!parent || strcmp(buffer + 1, parent->value.element.name))
	{
	  // Close tag doesn't match tree; print an error for now...
	  _mxml_error(options, "Mismatched close tag <%s> under parent <%s> on line %d.", buffer, parent ? parent->value.element.name : "(null)", line);
          goto error;
	}

        // Keep reading until we see >...
        while (ch != '>' && ch != EOF)
	  ch = mxml_getc(options, io_cb, io_cbdata, &encoding);

        node   = parent;
        parent = parent->parent;

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_ELEMENT_CLOSE))
	    goto error;

          if (!mxmlRelease(node))
          {
            if (first == node)
	      first = NULL;

	    node = NULL;
	  }
        }

        // Ascend into the parent and set the value type as needed...
	if (options && options->type_cb && parent)
	  type = (options->type_cb)(options->type_cbdata, parent);
	else if (options && !options->type_cb)
	  type = options->type_value;
      }
      else
      {
        // Handle open tag...
        if (!parent && first)
	{
	  // There can only be one root element!
	  _mxml_error(options, "<%s> cannot be a second root node after <%s> on line %d.", buffer, first->value.element.name, line);
          goto error;
	}

        if ((node = mxmlNewElement(parent, buffer)) == NULL)
	{
	  // Just print error for now...
	  _mxml_error(options, "Unable to add element node to parent <%s> on line %d.", parent ? parent->value.element.name : "null", line);
	  goto error;
	}

        if (mxml_isspace(ch))
        {
	  MXML_DEBUG("mxml_load_data: node=%p(<%s...>), parent=%p\n", node, buffer, parent);

	  if ((ch = mxml_parse_element(options, io_cb, io_cbdata, node, &encoding, &line)) == EOF)
	    goto error;
        }
        else if (ch == '/')
	{
	  MXML_DEBUG("mxml_load_data: node=%p(<%s/>), parent=%p\n", node, buffer, parent);

	  if ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != '>')
	  {
	    _mxml_error(options, "Expected > but got '%c' instead for element <%s/> on line %d.", ch, buffer, line);
            mxmlDelete(node);
            node = NULL;
            goto error;
	  }

	  ch = '/';
	}

        if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_ELEMENT_OPEN))
	    goto error;
	}

        if (!first)
	  first = node;

	if (ch == EOF)
	  break;

        if (ch != '/')
	{
	  // Descend into this node, setting the value type as needed...
	  parent = node;

	  if (options && options->type_cb && parent)
	    type = (options->type_cb)(options->type_cbdata, parent);
	  else if (options && !options->type_cb)
	    type = options->type_value;
	  else
	    type = MXML_TYPE_TEXT;
	}
        else if (options && options->sax_cb)
        {
          if (!(options->sax_cb)(options->sax_cbdata, node, MXML_SAX_EVENT_ELEMENT_CLOSE))
	    goto error;

          if (!mxmlRelease(node))
          {
            if (first == node)
	      first = NULL;

	    node = NULL;
	  }
        }
      }

      bufptr  = buffer;
    }
    else if (ch == '&')
    {
      // Add character entity to current buffer...
      if ((ch = mxml_get_entity(options, io_cb, io_cbdata, &encoding, parent, &line)) == EOF)
	goto error;

      if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	goto error;
    }
    else if (type == MXML_TYPE_OPAQUE || type == MXML_TYPE_CUSTOM || !mxml_isspace(ch))
    {
      // Add character to current buffer...
      if (!mxml_add_char(options, ch, &bufptr, &buffer, &bufsize))
	goto error;
    }
  }
  while ((ch = mxml_getc(options, io_cb, io_cbdata, &encoding)) != EOF);

  // Free the string buffer - we don't need it anymore...
  free(buffer);

  // Find the top element and return it...
  if (parent)
  {
    node = parent;

    while (parent != top && parent->parent)
      parent = parent->parent;

    if (node != parent)
    {
      _mxml_error(options, "Missing close tag </%s> under parent <%s> on line %d.", mxmlGetElement(node), node->parent ? node->parent->value.element.name : "(null)", line);

      mxmlDelete(first);

      return (NULL);
    }
  }

  if (parent)
    return (parent);
  else
    return (first);

  // Common error return...
  error:

  mxmlDelete(first);

  free(buffer);

  return (NULL);
}


//
// 'mxml_parse_element()' - Parse an element for any attributes...
//

static int				// O  - Terminating character
mxml_parse_element(
    mxml_options_t   *options,		// I - Options
    mxml_io_cb_t     io_cb,		// I - Read callback function
    void             *io_cbdata,	// I - Read callback data
    mxml_node_t      *node,		// I  - Element node
    _mxml_encoding_t *encoding,		// IO - Encoding
    int              *line)		// IO - Current line number
{
  int		ch,			// Current character in file
		quote;			// Quoting character
  char		*name,			// Attribute name
		*value,			// Attribute value
		*ptr;			// Pointer into name/value
  size_t	namesize,		// Size of name string
		valsize;		// Size of value string


  // Initialize the name and value buffers...
  if ((name = malloc(64)) == NULL)
  {
    _mxml_error(options, "Unable to allocate memory for name.");
    return (EOF);
  }

  namesize = 64;

  if ((value = malloc(64)) == NULL)
  {
    free(name);
    _mxml_error(options, "Unable to allocate memory for value.");
    return (EOF);
  }

  valsize = 64;

  // Loop until we hit a >, /, ?, or EOF...
  while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
  {
    MXML_DEBUG("mxml_parse_element: ch='%c'\n", ch);

    // Skip leading whitespace...
    if (mxml_isspace(ch))
    {
      if (ch == '\n')
        (*line)++;

      continue;
    }

    // Stop at /, ?, or >...
    if (ch == '/' || ch == '?')
    {
      // Grab the > character and print an error if it isn't there...
      quote = mxml_getc(options, io_cb, io_cbdata, encoding);

      if (quote != '>')
      {
        _mxml_error(options, "Expected '>' after '%c' for element %s, but got '%c' on line %d.", ch, mxmlGetElement(node), quote, *line);
        goto error;
      }

      break;
    }
    else if (ch == '<')
    {
      _mxml_error(options, "Bare < in element %s on line %d.", mxmlGetElement(node), *line);
      goto error;
    }
    else if (ch == '>')
    {
      break;
    }

    // Read the attribute name...
    ptr = name;
    if (!mxml_add_char(options, ch, &ptr, &name, &namesize))
      goto error;

    if (ch == '\"' || ch == '\'')
    {
      // Name is in quotes, so get a quoted string...
      quote = ch;

      while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
      {
        if (ch == '&')
        {
	  if ((ch = mxml_get_entity(options, io_cb, io_cbdata, encoding, node, line)) == EOF)
	    goto error;
	}
	else if (ch == '\n')
	{
	  (*line)++;
	}

	if (!mxml_add_char(options, ch, &ptr, &name, &namesize))
	  goto error;

	if (ch == quote)
          break;
      }
    }
    else
    {
      // Grab an normal, non-quoted name...
      while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
      {
	if (mxml_isspace(ch) || ch == '=' || ch == '/' || ch == '>' || ch == '?')
	{
	  if (ch == '\n')
	    (*line)++;
          break;
        }
	else
	{
          if (ch == '&')
          {
	    if ((ch = mxml_get_entity(options, io_cb, io_cbdata, encoding, node, line)) == EOF)
	      goto error;
          }

	  if (!mxml_add_char(options, ch, &ptr, &name, &namesize))
	    goto error;
	}
      }
    }

    *ptr = '\0';

    if (mxmlElementGetAttr(node, name))
    {
      _mxml_error(options, "Duplicate attribute '%s' in element %s on line %d.", name, mxmlGetElement(node), *line);
      goto error;
    }

    while (ch != EOF && mxml_isspace(ch))
    {
      ch = mxml_getc(options, io_cb, io_cbdata, encoding);

      if (ch == '\n')
        (*line)++;
    }

    if (ch == '=')
    {
      // Read the attribute value...
      while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF && mxml_isspace(ch))
      {
        if (ch == '\n')
          (*line)++;
      }

      if (ch == EOF)
      {
        _mxml_error(options, "Missing value for attribute '%s' in element %s on line %d.", name, mxmlGetElement(node), *line);
        goto error;
      }

      if (ch == '\'' || ch == '\"')
      {
        // Read quoted value...
        quote = ch;
	ptr   = value;

        while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
        {
	  if (ch == quote)
	  {
	    break;
	  }
	  else
	  {
	    if (ch == '&')
	    {
	      if ((ch = mxml_get_entity(options, io_cb, io_cbdata, encoding, node, line)) == EOF)
	        goto error;
	    }
	    else if (ch == '\n')
	    {
	      (*line)++;
	    }

	    if (!mxml_add_char(options, ch, &ptr, &value, &valsize))
	      goto error;
	  }
	}

        *ptr = '\0';
      }
      else
      {
        // Read unquoted value...
	ptr      = value;
	if (!mxml_add_char(options, ch, &ptr, &value, &valsize))
	  goto error;

	while ((ch = mxml_getc(options, io_cb, io_cbdata, encoding)) != EOF)
	{
	  if (mxml_isspace(ch) || ch == '=' || ch == '/' || ch == '>')
	  {
	    if (ch == '\n')
	      (*line)++;

            break;
          }
	  else
	  {
	    if (ch == '&')
	    {
	      if ((ch = mxml_get_entity(options, io_cb, io_cbdata, encoding, node, line)) == EOF)
	        goto error;
	    }

	    if (!mxml_add_char(options, ch, &ptr, &value, &valsize))
	      goto error;
	  }
	}

        *ptr = '\0';
      }

      // Set the attribute with the given string value...
      mxmlElementSetAttr(node, name, value);
      MXML_DEBUG("mxml_parse_element: %s=\"%s\"\n", name, value);
    }
    else
    {
      _mxml_error(options, "Missing value for attribute '%s' in element %s on line %d.", name, mxmlGetElement(node), *line);
      goto error;
    }

    // Check the end character...
    if (ch == '/' || ch == '?')
    {
      // Grab the > character and print an error if it isn't there...
      quote = mxml_getc(options, io_cb, io_cbdata, encoding);

      if (quote != '>')
      {
        _mxml_error(options, "Expected '>' after '%c' for element %s, but got '%c' on line %d.", ch, mxmlGetElement(node), quote, *line);
        ch = EOF;
      }

      break;
    }
    else if (ch == '>')
      break;
  }

  // Free the name and value buffers and return...
  free(name);
  free(value);

  return (ch);

  // Common error return point...
  error:

  free(name);
  free(value);

  return (EOF);
}


//
// 'mxml_read_cb_fd()' - Read bytes from a file descriptor.
//

static size_t				// O - Bytes read
mxml_read_cb_fd(int    *fd,		// I - File descriptor
                void   *buffer,		// I - Buffer
                size_t bytes)		// I - Bytes to read
{
#if _WIN32
  int		rbytes;			// Bytes read


  rbytes = read(*fd, buffer, (unsigned int)bytes);

#else
  ssize_t	rbytes;			// Bytes read


  while ((rbytes = read(*fd, buffer, bytes)) < 0)
  {
    if (errno != EINTR && errno != EAGAIN)
      break;
  }
#endif // _WIN32

  if (rbytes < 0)
    return (0);
  else
    return ((size_t)rbytes);
}


//
// 'mxml_read_cb_file()' - Read bytes from a file pointer.
//

static size_t				// O - Bytes read
mxml_read_cb_file(FILE   *fp,		// I - File pointer
                  void   *buffer,	// I - Buffer
                  size_t bytes)		// I - Bytes to read
{
  if (feof(fp))
    return (0);
  else
    return (fread(buffer, 1, bytes, fp));
}


//
// 'mxml_read_cb_string()' - Read bytes from a string.
//

static size_t				// O - Bytes read
mxml_read_cb_string(
    _mxml_stringbuf_t *sb,		// I - String buffer
    void              *buffer,		// I - Buffer
    size_t            bytes)		// I - Bytes to read
{
  size_t	remaining;		// Remaining bytes in buffer


  if ((remaining = sb->bufsize - (size_t)(sb->bufptr - sb->buffer)) < bytes)
    bytes = remaining;

  if (bytes > 0)
  {
    // Copy bytes from string...
    memcpy(buffer, sb->bufptr, bytes);
    sb->bufptr += bytes;
  }

  return (bytes);
}


//
// 'mxml_strtod()' - Convert a string to a double without respect to the locale.
//

static double				// O - Real number
mxml_strtod(mxml_options_t *options,	// I - Options
            const char     *buffer,	// I - String
            char           **bufend)	// O - End of number in string
{
  const char	*bufptr;		// Pointer into buffer
  char		temp[64],		// Temporary buffer
		*tempptr;		// Pointer into temporary buffer


  // See if the locale has a special decimal point string...
  if (!options || !options->loc)
    return (strtod(buffer, bufend));

  // Copy leading sign, numbers, period, and then numbers...
  tempptr                = temp;
  temp[sizeof(temp) - 1] = '\0';
  bufptr                 = buffer;

  if (*bufptr == '-' || *bufptr == '+')
    *tempptr++ = *bufptr++;

  while (*bufptr && isdigit(*bufptr & 255))
  {
    if (tempptr < (temp + sizeof(temp) - 1))
    {
      *tempptr++ = *bufptr++;
    }
    else
    {
      *bufend = (char *)bufptr;
      return (0.0);
    }
  }

  if (*bufptr == '.')
  {
    // Convert decimal point to locale equivalent...
    size_t	declen = strlen(options->loc->decimal_point);
					// Length of decimal point
    bufptr ++;

    if (declen <= (sizeof(temp) - (size_t)(tempptr - temp)))
    {
      memcpy(tempptr, options->loc->decimal_point, declen);
      tempptr += declen;
    }
    else
    {
      *bufend = (char *)bufptr;
      return (0.0);
    }
  }

  // Copy any remaining characters...
  while (*bufptr && isdigit(*bufptr & 255))
  {
    if (tempptr < (temp + sizeof(temp) - 1))
      *tempptr++ = *bufptr++;
    else
      break;
  }

  *bufend = (char *)bufptr;

  if (*bufptr)
    return (0.0);

  // Nul-terminate the temporary string and convert the string...
  *tempptr = '\0';

  return (strtod(temp, NULL));
}


//
// 'mxml_io_cb_fd()' - Write bytes to a file descriptor.
//

static size_t				// O - Bytes written
mxml_io_cb_fd(int    *fd,		// I - File descriptor
                 void   *buffer,	// I - Buffer
                 size_t bytes)		// I - Bytes to write
{
#if _WIN32
  int		wbytes;			// Bytes written


  wbytes = write(*fd, buffer, (unsigned int)bytes);

#else
  ssize_t	wbytes;			// Bytes written


  while ((wbytes = write(*fd, buffer, bytes)) < 0)
  {
    if (errno != EINTR && errno != EAGAIN)
      break;
  }
#endif // _WIN32

  if (wbytes < 0)
    return (0);
  else
    return ((size_t)wbytes);
}


//
// 'mxml_io_cb_file()' - Write bytes to a file pointer.
//

static size_t				// O - Bytes written
mxml_io_cb_file(FILE   *fp,		// I - File pointer
                   void   *buffer,	// I - Buffer
                   size_t bytes)	// I - Bytes to write
{
  return (fwrite(buffer, 1, bytes, fp));
}


//
// 'mxml_io_cb_string()' - Write bytes to a string buffer.
//

static size_t				// O - Bytes written
mxml_io_cb_string(
    _mxml_stringbuf_t *sb,		// I - String buffer
    void              *buffer,		// I - Buffer
    size_t            bytes)		// I - Bytes to write
{
  size_t	remaining;		// Remaining bytes


  // Expand buffer as needed...
  if ((sb->bufptr + bytes) >= (sb->buffer + sb->bufsize - 1) && sb->bufalloc)
  {
    // Reallocate buffer
    char	*temp;			// New buffer pointer
    size_t	newsize;		// New bufsize

    newsize = (size_t)(sb->bufptr - sb->buffer) + bytes + 257;
    if ((temp = realloc(sb->buffer, newsize)) == NULL)
      return (0);

    sb->bufptr  = temp + (sb->bufptr - sb->buffer);
    sb->buffer  = temp;
    sb->bufsize = newsize;
  }

  // Copy what we can...
  if (sb->bufptr >= (sb->buffer + sb->bufsize - 1))
    return (0);			// No more room
  else if ((remaining = (sb->bufsize - (size_t)(sb->bufptr - sb->buffer) - 1)) < bytes)
    bytes = remaining;

  memcpy(sb->bufptr, buffer, bytes);
  sb->bufptr += bytes;

  return (bytes);
}


//
// 'mxml_write_node()' - Save an XML node to a file.
//

static int				// O - Column or -1 on error
mxml_write_node(
    mxml_node_t    *node,		// I - Node to write
    mxml_options_t *options,		// I - Options
    mxml_io_cb_t   io_cb,		// I - Write callback function
    void           *io_cbdata,		// I - Write callback data
    int            col)			// I - Current column
{
  mxml_node_t	*current,		// Current node
		*next;			// Next node
  size_t	i,			// Looping var
		width;			// Width of attr + value
  _mxml_attr_t	*attr;			// Current attribute
  char		s[255],			// Temporary string
		*data;			// Custom data string
  const char	*text;			// Text string
  bool		whitespace;		// Whitespace before text string?


  // Loop through this node and all of its children...
  for (current = node; current && col >= 0; current = next)
  {
    // Print the node value...
    MXML_DEBUG("mxml_write_node: current=%p(%d)\n", current, current->type);

    switch (mxmlGetType(current))
    {
      case MXML_TYPE_CDATA :
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_OPEN, col);
	  col = mxml_write_string("<![CDATA[", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(mxmlGetCDATA(current), io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string("]]>", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_OPEN, col);
          break;

      case MXML_TYPE_COMMENT :
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_OPEN, col);
	  col = mxml_write_string("<!--", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(mxmlGetComment(current), io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string("-->", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_OPEN, col);
          break;

      case MXML_TYPE_DECLARATION :
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_OPEN, col);
	  col = mxml_write_string("<!", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(mxmlGetDeclaration(current), io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(">", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_OPEN, col);
          break;

      case MXML_TYPE_DIRECTIVE :
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_OPEN, col);
	  col = mxml_write_string("<?", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(mxmlGetDirective(current), io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string("?>", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_OPEN, col);
          break;

      case MXML_TYPE_ELEMENT :
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_OPEN, col);
	  col = mxml_write_string("<", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_string(mxmlGetElement(current), io_cb, io_cbdata, /*use_entities*/true, col);

	  for (i = current->value.element.num_attrs, attr = current->value.element.attrs; i > 0 && col >= 0; i --, attr ++)
	  {
	    width = strlen(attr->name);

	    if (attr->value)
	      width += strlen(attr->value) + 3;

	    if (options && options->wrap > 0 && (col + (int)width) > options->wrap)
	      col = mxml_write_string("\n", io_cb, io_cbdata, /*use_entities*/false, col);
	    else
	      col = mxml_write_string(" ", io_cb, io_cbdata, /*use_entities*/false, col);

	    col = mxml_write_string(attr->name, io_cb, io_cbdata, /*use_entities*/true, col);

	    if (attr->value)
	    {
	      col = mxml_write_string("=\"", io_cb, io_cbdata, /*use_entities*/false, col);
	      col = mxml_write_string(attr->value, io_cb, io_cbdata, /*use_entities*/true, col);
	      col = mxml_write_string("\"", io_cb, io_cbdata, /*use_entities*/false, col);
	    }
	  }

	  col = mxml_write_string(current->child ? ">" : "/>", io_cb, io_cbdata, /*use_entities*/false, col);
	  col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_OPEN, col);
	  break;

      case MXML_TYPE_INTEGER :
	  if (current->prev)
	  {
	    // Add whitespace separator...
	    if (options && options->wrap > 0 && col > options->wrap)
	      col = mxml_write_string("\n", io_cb, io_cbdata, /*use_entities*/false, col);
	    else
	      col = mxml_write_string(" ", io_cb, io_cbdata, /*use_entities*/false, col);
	  }

          // Write integer...
	  snprintf(s, sizeof(s), "%ld", current->value.integer);
	  col = mxml_write_string(s, io_cb, io_cbdata, /*use_entities*/true, col);
	  break;

      case MXML_TYPE_OPAQUE :
	  col = mxml_write_string(mxmlGetOpaque(current), io_cb, io_cbdata, /*use_entities*/true, col);
	  break;

      case MXML_TYPE_REAL :
	  if (current->prev)
	  {
	    // Add whitespace separator...
	    if (options && options->wrap > 0 && col > options->wrap)
	      col = mxml_write_string("\n", io_cb, io_cbdata, /*use_entities*/false, col);
	    else
	      col = mxml_write_string(" ", io_cb, io_cbdata, /*use_entities*/false, col);
	  }

          // Write real number...
	  snprintf(s, sizeof(s), "%g", current->value.real);

          if (options && options->loc)
          {
            char	*sptr;		// Pointer into string

	    if ((sptr = strstr(s, options->loc->decimal_point)) != NULL)
	    {
	      // Convert locale decimal point to "."
	      if (options->loc_declen > 1)
	        memmove(sptr + 1, sptr + options->loc_declen, strlen(sptr + options->loc_declen) + 1);

	      *sptr = '.';
	    }
          }

	  col = mxml_write_string(s, io_cb, io_cbdata, /*use_entities*/true, col);
	  break;

      case MXML_TYPE_TEXT :
          text = mxmlGetText(current, &whitespace);

	  if (whitespace && col > 0)
	  {
	    // Add whitespace separator...
	    if (options && options->wrap > 0 && col > options->wrap)
	      col = mxml_write_string("\n", io_cb, io_cbdata, /*use_entities*/false, col);
	    else
	      col = mxml_write_string(" ", io_cb, io_cbdata, /*use_entities*/false, col);
	  }

	  col = mxml_write_string(text, io_cb, io_cbdata, /*use_entities*/true, col);
	  break;

      case MXML_TYPE_CUSTOM :
	  if (!options || !options->custsave_cb)
	    return (-1);

	  if ((data = (options->custsave_cb)(options->cust_cbdata, current)) == NULL)
	    return (-1);

	  col = mxml_write_string(data, io_cb, io_cbdata, /*use_entities*/true, col);

	  free(data);
	  break;

      default : // Should never happen
	  return (-1);
    }

    // Figure out the next node...
    if ((next = mxmlGetFirstChild(current)) == NULL)
    {
      if (current == node)
      {
        // Don't traverse to sibling node if we are at the "root" node...
        next = NULL;
      }
      else
      {
        // Try the next sibling, and continue traversing upwards as needed...
	while ((next = mxmlGetNextSibling(current)) == NULL)
	{
	  if (current == node || !mxmlGetParent(current))
	    break;

	  // Declarations and directives have no end tags...
	  current = mxmlGetParent(current);

	  if (mxmlGetType(current) == MXML_TYPE_ELEMENT)
	  {
	    col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_BEFORE_CLOSE, col);
	    col = mxml_write_string("</", io_cb, io_cbdata, /*use_entities*/false, col);
	    col = mxml_write_string(mxmlGetElement(current), io_cb, io_cbdata, /*use_entities*/true, col);
	    col = mxml_write_string(">", io_cb, io_cbdata, /*use_entities*/false, col);
	    col = mxml_write_ws(current, options, io_cb, io_cbdata, MXML_WS_AFTER_CLOSE, col);
	  }

	  if (current == node)
	    break;
	}
      }
    }
  }

  return (col);
}


//
// 'mxml_write_string()' - Write a string, escaping & and < as needed.
//

static int				// O - New column or `-1` on error
mxml_write_string(
    const char      *s,			// I - String to write
    mxml_io_cb_t    io_cb,		// I - Write callback function
    void            *io_cbdata,		// I - Write callback data
    bool            use_entities,	// I - Escape special characters?
    int             col)		// I - Current column
{
  const char	*frag,			// Start of current string fragment
		*ptr,			// Pointer into string
		*ent;			// Entity, if any
  size_t	fraglen;		// Length of fragment


  MXML_DEBUG("mxml_write_string(io_cb=%p, io_cbdata=%p, s=\"%s\", use_entities=%s, col=%d)\n", io_cb, io_cbdata, s, use_entities ? "true" : "false", col);

  if (col < 0)
    return (-1);

  for (frag = ptr = s; *ptr; ptr ++)
  {
    if (use_entities && (ent = _mxml_entity_string(*ptr)) != NULL)
    {
      size_t entlen = strlen(ent);	// Length of entity

      if (ptr > frag)
      {
        // Write current fragment
        fraglen = (size_t)(ptr - frag);

	if ((io_cb)(io_cbdata, (char *)frag, fraglen) != fraglen)
	  return (-1);
      }

      frag = ptr + 1;

      // Write entity
      if ((io_cb)(io_cbdata, (char *)ent, entlen) != entlen)
        return (-1);

      col ++;
    }
    else if (*ptr == '\r' || *ptr == '\n')
    {
      // CR or LF resets column
      col = 0;
    }
    else if (*ptr == '\t')
    {
      // Tab indents column
      col = col - (col % MXML_TAB) + MXML_TAB;
    }
    else
    {
      // All other characters are 1 column wide
      col ++;
    }
  }

  if (ptr > frag)
  {
    // Write final fragment
    fraglen = (size_t)(ptr - frag);

    if ((io_cb)(io_cbdata, (char *)frag, fraglen) != fraglen)
      return (-1);
  }

  return (col);
}


//
// 'mxml_write_ws()' - Do whitespace callback...
//

static int				// O - New column or `-1` on error
mxml_write_ws(
    mxml_node_t    *node,		// I - Current node
    mxml_options_t *options,		// I - Options
    mxml_io_cb_t   io_cb,		// I - Write callback function
    void           *io_cbdata,		// I - Write callback data
    mxml_ws_t      ws,			// I - Whitespace value
    int            col)			// I - Current column
{
  const char	*s;			// Whitespace string


  if (options && options->ws_cb && (s = (options->ws_cb)(options->ws_cbdata, node, ws)) != NULL)
    col = mxml_write_string(s, io_cb, io_cbdata, /*use_entities*/false, col);

  return (col);
}
