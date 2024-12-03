//
// Options functions for Mini-XML, a small XML file parsing library.
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
// 'mxmlOptionsDelete()' - Free load/save options.
//

void
mxmlOptionsDelete(
    mxml_options_t *options)		// I - Options
{
  free(options);
}


//
// 'mxmlOptionsNew()' - Allocate load/save options.
//
// This function creates a new set of load/save options to use with the
// @link mxmlLoadFd@, @link mxmlLoadFile@, @link mxmlLoadFilename@,
// @link mxmlLoadIO@, @link mxmlLoadString@, @link mxmlSaveAllocString@,
// @link mxmlSaveFd@, @link mxmlSaveFile@, @link mxmlSaveFilename@,
// @link mxmlSaveIO@, and @link mxmlSaveString@ functions.  Options can be
// reused for multiple calls to these functions and should be freed using the
// @link mxmlOptionsDelete@ function.
//
// The default load/save options load values using the constant type
// `MXML_TYPE_TEXT` and save XML data with a wrap margin of 72 columns.
// The various `mxmlOptionsSet` functions are used to change the defaults,
// for example:
//
// ```c
// mxml_options_t *options = mxmlOptionsNew();
//
// /* Load values as opaque strings */
// mxmlOptionsSetTypeValue(options, MXML_TYPE_OPAQUE);
// ```
//
// Note: The most common programming error when using the Mini-XML library is
// to load an XML file using the `MXML_TYPE_TEXT` node type, which returns
// inline text as a series of whitespace-delimited words, instead of using the
// `MXML_TYPE_OPAQUE` node type which returns the inline text as a single string
// (including whitespace).
//

mxml_options_t *			// O - Options
mxmlOptionsNew(void)
{
  mxml_options_t *options;		// Options


  if ((options = (mxml_options_t *)calloc(1, sizeof(mxml_options_t))) != NULL)
  {
    // Set default values...
    options->type_value = MXML_TYPE_TEXT;
    options->wrap       = 72;

    //if ((options->loc = localeconv()) != NULL)
    //{
    //  if (!options->loc->decimal_point || !strcmp(options->loc->decimal_point, "."))
	options->loc = NULL;
    //  else
	//options->loc_declen = strlen(options->loc->decimal_point);
    //}
  }

  return (options);
}


//
// 'mxmlOptionsSetCustomCallbacks()' - Set the custom data callbacks.
//
// This function sets the callbacks that are used for loading and saving custom
// data types. The load callback `load_cb` accepts the callback data pointer
// `cbdata`, a node pointer, and a data string and returns `true` on success and
// `false` on error, for example:
//
// ```c
// typedef struct
// {
//   unsigned year,    /* Year */
//            month,   /* Month */
//            day,     /* Day */
//            hour,    /* Hour */
//            minute,  /* Minute */
//            second;  /* Second */
//   time_t   unix;    /* UNIX time */
// } iso_date_time_t;
//
// void
// my_custom_free_cb(void *cbdata, void *data)
// {
//   free(data);
// }
//
// bool
// my_custom_load_cb(void *cbdata, mxml_node_t *node, const char *data)
// {
//   iso_date_time_t *dt;
//   struct tm tmdata;
//
//   /* Allocate custom data structure ... */
//   dt = calloc(1, sizeof(iso_date_time_t));
//
//   /* Parse the data string... */
//   if (sscanf(data, "%u-%u-%uT%u:%u:%uZ", &(dt->year), &(dt->month),
//              &(dt->day), &(dt->hour), &(dt->minute), &(dt->second)) != 6)
//   {
//     /* Unable to parse date and time numbers... */
//     free(dt);
//     return (false);
//   }
//
//   /* Range check values... */
//   if (dt->month < 1 || dt->month > 12 || dt->day < 1 || dt->day > 31 ||
//       dt->hour < 0 || dt->hour > 23 || dt->minute < 0 || dt->minute > 59 ||
//       dt->second < 0 || dt->second > 60)
//   {
//     /* Date information is out of range... */
//     free(dt);
//     return (false);
//   }
//
//   /* Convert ISO time to UNIX time in seconds... */
//   tmdata.tm_year = dt->year - 1900;
//   tmdata.tm_mon  = dt->month - 1;
//   tmdata.tm_day  = dt->day;
//   tmdata.tm_hour = dt->hour;
//   tmdata.tm_min  = dt->minute;
//   tmdata.tm_sec  = dt->second;
//
//   dt->unix = gmtime(&tmdata);
//
//   /* Set custom data and free function... */
//   mxmlSetCustom(node, data, my_custom_free, /*cbdata*/NULL);
//
//   /* Return with no errors... */
//   return (true);
// }
// ```
//
// The save callback `save_cb` accepts the callback data pointer `cbdata` and a
// node pointer and returns a malloc'd string on success and `NULL` on error,
// for example:
//
// ```c
// char *
// my_custom_save_cb(void *cbdata, mxml_node_t *node)
// {
//   char data[255];
//   iso_date_time_t *dt;
//
//   /* Get the custom data structure */
//   dt = (iso_date_time_t *)mxmlGetCustom(node);
//
//   /* Generate string version of the date/time... */
//   snprintf(data, sizeof(data), "%04u-%02u-%02uT%02u:%02u:%02uZ",
//            dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);
//
//   /* Duplicate the string and return... */
//   return (strdup(data));
// }
// ```
//

void
mxmlOptionsSetCustomCallbacks(
    mxml_options_t     *options,	// I - Options
    mxml_custload_cb_t load_cb,		// I - Custom load callback function
    mxml_custsave_cb_t save_cb,		// I - Custom save callback function
    void               *cbdata)		// I - Custom callback data
{
  if (options)
  {
    options->custload_cb = load_cb;
    options->custsave_cb = save_cb;
    options->cust_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetEntityCallback()' - Set the entity lookup callback to use when loading XML data.
//
// This function sets the callback that is used to lookup named XML character
// entities when loading XML data.  The callback function `cb` accepts the
// callback data pointer `cbdata` and the entity name.  The function returns a
// Unicode character value or `-1` if the entity is not known.  For example, the
// following entity callback supports the "euro" entity:
//
// ```c
// int my_entity_cb(void *cbdata, const char *name)
// {
//   if (!strcmp(name, "euro"))
//     return (0x20ac);
//   else
//     return (-1);
// }
// ```
//
// Mini-XML automatically supports the "amp", "gt", "lt", and "quot" character
// entities which are required by the base XML specification.
//

void
mxmlOptionsSetEntityCallback(
    mxml_options_t   *options,		// I - Options
    mxml_entity_cb_t cb,		// I - Entity callback function
    void             *cbdata)		// I - Entity callback data
{
  if (options)
  {
    options->entity_cb     = cb;
    options->entity_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetErrorCallback()' - Set the error message callback.
//
// This function sets a function to use when reporting errors.  The callback
// `cb` accepts the data pointer `cbdata` and a string pointer containing the
// error message:
//
// ```c
// void my_error_cb(void *cbdata, const char *message)
// {
//   fprintf(stderr, "myprogram: %s\n", message);
// }
// ```
//
// The default error callback writes the error message to the `stderr` file.
//

void
mxmlOptionsSetErrorCallback(
    mxml_options_t  *options,		// I - Options
    mxml_error_cb_t cb,			// I - Error callback function
    void            *cbdata)		// I - Error callback data
{
  if (options)
  {
    options->error_cb     = cb;
    options->error_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetSAXCallback()' - Set the SAX callback to use when reading XML data.
//
// This function sets a SAX callback to use when reading XML data.  The SAX
// callback function `cb` and associated callback data `cbdata` are used to
// enable the Simple API for XML streaming mode.  The callback is called as the
// XML node tree is parsed and receives the `cbdata` pointer, the `mxml_node_t`
// pointer, and an event code.  The function returns `true` to continue
// processing or `false` to stop:
//
// ```c
// bool
// sax_cb(void *cbdata, mxml_node_t *node,
//        mxml_sax_event_t event)
// {
//   ... do something ...
//
//   /* Continue processing... */
//   return (true);
// }
// ```
//
// The event will be one of the following:
//
// - `MXML_SAX_EVENT_CDATA`: CDATA was just read.
// - `MXML_SAX_EVENT_COMMENT`: A comment was just read.
// - `MXML_SAX_EVENT_DATA`: Data (integer, opaque, real, or text) was just read.
// - `MXML_SAX_EVENT_DECLARATION`: A declaration was just read.
// - `MXML_SAX_EVENT_DIRECTIVE`: A processing directive/instruction was just read.
// - `MXML_SAX_EVENT_ELEMENT_CLOSE` - A close element was just read \(`</element>`)
// - `MXML_SAX_EVENT_ELEMENT_OPEN` - An open element was just read \(`<element>`)
//
// Elements are *released* after the close element is processed.  All other nodes
// are released after they are processed.  The SAX callback can *retain* the node
// using the [mxmlRetain](@@) function.
//

void
mxmlOptionsSetSAXCallback(
    mxml_options_t *options,		// I - Options
    mxml_sax_cb_t  cb,			// I - SAX callback function
    void           *cbdata)		// I - SAX callback data
{
  if (options)
  {
    options->sax_cb     = cb;
    options->sax_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetTypeCallback()' - Set the type callback for child/value nodes.
//
// The load callback function `cb` is called to obtain the node type child/value
// nodes and receives the `cbdata` pointer and the `mxml_node_t` pointer, for
// example:
//
// ```c
// mxml_type_t
// my_type_cb(void *cbdata, mxml_node_t *node)
// {
//   const char *type;
//
//  /*
//   * You can lookup attributes and/or use the element name,
//   * hierarchy, etc...
//   */
//
//   type = mxmlElementGetAttr(node, "type");
//   if (type == NULL)
//     type = mxmlGetElement(node);
//   if (type == NULL)
//     type = "text";
//
//   if (!strcmp(type, "integer"))
//     return (MXML_TYPE_INTEGER);
//   else if (!strcmp(type, "opaque"))
//     return (MXML_TYPE_OPAQUE);
//   else if (!strcmp(type, "real"))
//     return (MXML_TYPE_REAL);
//   else
//     return (MXML_TYPE_TEXT);
// }
// ```
//

void
mxmlOptionsSetTypeCallback(
    mxml_options_t *options,		// I - Options
    mxml_type_cb_t cb,			// I - Type callback function
    void           *cbdata)		// I - Type callback data
{
  if (options)
  {
    options->type_cb     = cb;
    options->type_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetTypeValue()' - Set the type to use for all child/value nodes.
//
// This functions sets a constant node type to use for all child/value nodes.
//

void
mxmlOptionsSetTypeValue(
    mxml_options_t *options,		// I - Options
    mxml_type_t    type)		// I - Value node type
{
  if (options)
  {
    options->type_cb    = NULL;
    options->type_value = type;
  }
}


//
// 'mxmlOptionsSetWhitespaceCallback()' - Set the whitespace callback.
//
// This function sets the whitespace callback that is used when saving XML data.
// The callback function `cb` specifies a function that returns a whitespace
// string or `NULL` before and after each element.  The function receives the
// callback data pointer `cbdata`, the `mxml_node_t` pointer, and a "when"
// value indicating where the whitespace is being added, for example:
//
// ```c
// const char *my_whitespace_cb(void *cbdata, mxml_node_t *node, mxml_ws_t when)
// {
//   if (when == MXML_WS_BEFORE_OPEN || when == MXML_WS_AFTER_CLOSE)
//     return ("\n");
//   else
//     return (NULL);
// }
// ```
//

void
mxmlOptionsSetWhitespaceCallback(
    mxml_options_t *options,		// I - Options
    mxml_ws_cb_t   cb,			// I - Whitespace callback function
    void           *cbdata)		// I - Whitespace callback data
{
  if (options)
  {
    options->ws_cb     = cb;
    options->ws_cbdata = cbdata;
  }
}


//
// 'mxmlOptionsSetWrapMargin()' - Set the wrap margin when saving XML data.
//
// This function sets the wrap margin used when saving XML data.  Wrapping is
// disabled when `column` is `0`.
//

void
mxmlOptionsSetWrapMargin(
    mxml_options_t *options,		// I - Options
    int            column)		// I - Wrap column
{
  if (options)
    options->wrap = column;
}


//
// '_mxml_entity_string()' - Get the entity that corresponds to the character, if any.
//

const char *				// O - Entity or `NULL` for none
_mxml_entity_string(int ch)		// I - Character
{
  switch (ch)
  {
    case '&' :
        return ("&amp;");

    case '<' :
        return ("&lt;");

    case '>' :
        return ("&gt;");

    case '\"' :
        return ("&quot;");

    default :
        return (NULL);
  }
}


//
// '_mxml_entity_value()' - Get the character corresponding to a named entity.
//
// The entity name can also be a numeric constant. `-1` is returned if the
// name is not known.
//

int					// O - Unicode character
_mxml_entity_value(
    mxml_options_t *options,		// I - Options
    const char     *name)		// I - Entity name
{
  int		ch = -1;		// Unicode character


  if (!name)
  {
    // No name...
    return (-1);
  }
  else if (*name == '#')
  {
    // Numeric entity...
    if (name[1] == 'x')
      ch = (int)strtol(name + 2, NULL, 16);
    else
      ch = (int)strtol(name + 1, NULL, 10);
  }
  else if (!strcmp(name, "amp"))
  {
    // Ampersand
    ch = '&';
  }
  else if (!strcmp(name, "gt"))
  {
    // Greater than
    ch = '>';
  }
  else if (!strcmp(name, "lt"))
  {
    // Less than
    ch = '<';
  }
  else if (!strcmp(name, "quot"))
  {
    // Double quote
    ch = '\"';
  }
  else if (options && options->entity_cb)
  {
    // Use callback
    ch = (options->entity_cb)(options->entity_cbdata, name);
  }

  return (ch);
}


//
// '_mxml_error()' - Display an error message.
//

void
_mxml_error(mxml_options_t *options,	// I - Load/save options
            const char     *format,	// I - Printf-style format string
            ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		s[1024];		// Message string


  // Range check input...
  if (!format)
    return;

  // Format the error message string...
  va_start(ap, format);
  vsnprintf(s, sizeof(s), format, ap);
  va_end(ap);

  // And then display the error message...
  if (options->error_cb)
    (options->error_cb)(options->error_cbdata, s);
  else
    fprintf(stderr, "%s\n", s);
}
