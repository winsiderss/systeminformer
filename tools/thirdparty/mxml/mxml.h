//
// Header file for Mini-XML, a small XML file parsing library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2003-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef MXML_H
#  define MXML_H
#  include <stdio.h>
#  include <stdlib.h>
#  include <stdbool.h>
#  include <stdint.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>
#  include <limits.h>
#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Constants...
//

#  define MXML_MAJOR_VERSION	4	// Major version number
#  define MXML_MINOR_VERSION	0	// Minor version number

#  ifdef __GNUC__
#    define MXML_FORMAT(a,b)	__attribute__ ((__format__ (__printf__, a, b)))
#  else
#    define MXML_FORMAT(a,b)
#  endif // __GNUC__


//
// Data types...
//

typedef enum mxml_add_e			// @link mxmlAdd@ add values
{
  MXML_ADD_BEFORE,			// Add node before specified node
  MXML_ADD_AFTER			// Add node after specified node
} mxml_add_t;

typedef enum mxml_descend_e		// @link mxmlFindElement@, @link mxmlWalkNext@, and @link mxmlWalkPrev@ descend values
{
  MXML_DESCEND_FIRST = -1,		// Descend for first find
  MXML_DESCEND_NONE = 0,		// Don't descend when finding/walking
  MXML_DESCEND_ALL = 1			// Descend when finding/walking
} mxml_descend_t;

typedef enum mxml_sax_event_e		// SAX event type.
{
  MXML_SAX_EVENT_CDATA,			// CDATA node
  MXML_SAX_EVENT_COMMENT,		// Comment node
  MXML_SAX_EVENT_DATA,			// Data node
  MXML_SAX_EVENT_DECLARATION,		// Declaration node
  MXML_SAX_EVENT_DIRECTIVE,		// Processing instruction node
  MXML_SAX_EVENT_ELEMENT_CLOSE,		// Element closed
  MXML_SAX_EVENT_ELEMENT_OPEN		// Element opened
} mxml_sax_event_t;

typedef enum mxml_type_e		// The XML node type.
{
  MXML_TYPE_IGNORE = -1,		// Ignore/throw away node
  MXML_TYPE_CDATA,			// CDATA value ("<[CDATA[...]]>")
  MXML_TYPE_COMMENT,			// Comment ("<!--...-->")
  MXML_TYPE_DECLARATION,		// Declaration ("<!...>")
  MXML_TYPE_DIRECTIVE,			// Processing instruction ("<?...?>")
  MXML_TYPE_ELEMENT,			// XML element with attributes
  MXML_TYPE_INTEGER,			// Integer value
  MXML_TYPE_OPAQUE,			// Opaque string
  MXML_TYPE_REAL,			// Real value
  MXML_TYPE_TEXT,			// Text fragment
  MXML_TYPE_CUSTOM			// Custom data
} mxml_type_t;

typedef enum mxml_ws_e			// Whitespace periods
{
  MXML_WS_BEFORE_OPEN,			// Callback for before open tag
  MXML_WS_AFTER_OPEN,			// Callback for after open tag
  MXML_WS_BEFORE_CLOSE,			// Callback for before close tag
  MXML_WS_AFTER_CLOSE,			// Callback for after close tag
} mxml_ws_t;

typedef void (*mxml_error_cb_t)(void *cbdata, const char *message);
					// Error callback function

typedef struct _mxml_node_s mxml_node_t;// An XML node

typedef struct _mxml_index_s mxml_index_t;
					// An XML node index

typedef struct _mxml_options_s mxml_options_t;
					// XML options

typedef void (*mxml_custfree_cb_t)(void *cbdata, void *custdata);
					// Custom data destructor

typedef bool (*mxml_custload_cb_t)(void *cbdata, mxml_node_t *node, const char *s);
					// Custom data load callback function

typedef char *(*mxml_custsave_cb_t)(void *cbdata, mxml_node_t *node);
					// Custom data save callback function

typedef int (*mxml_entity_cb_t)(void *cbdata, const char *name);
					// Entity callback function

typedef size_t (*mxml_io_cb_t)(void *cbdata, void *buffer, size_t bytes);
					// Read/write callback function

typedef bool (*mxml_sax_cb_t)(void *cbdata, mxml_node_t *node, mxml_sax_event_t event);
					// SAX callback function

typedef char *(*mxml_strcopy_cb_t)(void *cbdata, const char *s);
					// String copy/allocation callback
typedef void (*mxml_strfree_cb_t)(void *cbdata, char *s);
					// String free callback

typedef mxml_type_t (*mxml_type_cb_t)(void *cbdata, mxml_node_t *node);
					// Type callback function

typedef const char *(*mxml_ws_cb_t)(void *cbdata, mxml_node_t *node, mxml_ws_t when);
					// Whitespace callback function



//
// Prototypes...
//

extern void		mxmlAdd(mxml_node_t *parent, mxml_add_t add, mxml_node_t *child, mxml_node_t *node);

extern void		mxmlDelete(mxml_node_t *node);

extern void		mxmlElementClearAttr(mxml_node_t *node, const char *name);
extern const char	*mxmlElementGetAttr(mxml_node_t *node, const char *name);
extern const char       *mxmlElementGetAttrByIndex(mxml_node_t *node, size_t idx, const char **name);
extern size_t		mxmlElementGetAttrCount(mxml_node_t *node);
extern void		mxmlElementSetAttr(mxml_node_t *node, const char *name, const char *value);
extern void		mxmlElementSetAttrf(mxml_node_t *node, const char *name, const char *format, ...) MXML_FORMAT(3,4);

extern mxml_node_t	*mxmlFindElement(mxml_node_t *node, mxml_node_t *top, const char *element, const char *attr, const char *value, mxml_descend_t descend);
extern mxml_node_t	*mxmlFindPath(mxml_node_t *node, const char *path);

extern const char	*mxmlGetCDATA(mxml_node_t *node);
extern const char	*mxmlGetComment(mxml_node_t *node);
extern const void	*mxmlGetCustom(mxml_node_t *node);
extern const char	*mxmlGetDeclaration(mxml_node_t *node);
extern const char	*mxmlGetDirective(mxml_node_t *node);
extern const char	*mxmlGetElement(mxml_node_t *node);
extern mxml_node_t	*mxmlGetFirstChild(mxml_node_t *node);
extern long		mxmlGetInteger(mxml_node_t *node);
extern mxml_node_t	*mxmlGetLastChild(mxml_node_t *node);
extern mxml_node_t	*mxmlGetNextSibling(mxml_node_t *node);
extern const char	*mxmlGetOpaque(mxml_node_t *node);
extern mxml_node_t	*mxmlGetParent(mxml_node_t *node);
extern mxml_node_t	*mxmlGetPrevSibling(mxml_node_t *node);
extern double		mxmlGetReal(mxml_node_t *node);
extern size_t		mxmlGetRefCount(mxml_node_t *node);
extern const char	*mxmlGetText(mxml_node_t *node, bool *whitespace);
extern mxml_type_t	mxmlGetType(mxml_node_t *node);
extern void		*mxmlGetUserData(mxml_node_t *node);

extern void		mxmlIndexDelete(mxml_index_t *ind);
extern mxml_node_t	*mxmlIndexEnum(mxml_index_t *ind);
extern mxml_node_t	*mxmlIndexFind(mxml_index_t *ind, const char *element, const char *value);
extern size_t		mxmlIndexGetCount(mxml_index_t *ind);
extern mxml_index_t	*mxmlIndexNew(mxml_node_t *node, const char *element, const char *attr);
extern mxml_node_t	*mxmlIndexReset(mxml_index_t *ind);

extern mxml_node_t	*mxmlLoadFd(mxml_node_t *top, mxml_options_t *options, int fd);
extern mxml_node_t	*mxmlLoadFile(mxml_node_t *top, mxml_options_t *options, FILE *fp);
extern mxml_node_t	*mxmlLoadFilename(mxml_node_t *top, mxml_options_t *options, const char *filename);
extern mxml_node_t	*mxmlLoadIO(mxml_node_t *top, mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata);
extern mxml_node_t	*mxmlLoadString(mxml_node_t *top, mxml_options_t *options, const char *s);

extern void		mxmlOptionsDelete(mxml_options_t *options);
extern mxml_options_t	*mxmlOptionsNew(void);
extern void		mxmlOptionsSetCustomCallbacks(mxml_options_t *options, mxml_custload_cb_t load_cb, mxml_custsave_cb_t save_cb, void *cbdata);
extern void		mxmlOptionsSetEntityCallback(mxml_options_t *options, mxml_entity_cb_t cb, void *cbdata);
extern void		mxmlOptionsSetErrorCallback(mxml_options_t *options, mxml_error_cb_t cb, void *cbdata);
extern void		mxmlOptionsSetSAXCallback(mxml_options_t *options, mxml_sax_cb_t cb, void *cbdata);
extern void		mxmlOptionsSetTypeCallback(mxml_options_t *options, mxml_type_cb_t cb, void *cbdata);
extern void		mxmlOptionsSetTypeValue(mxml_options_t *options, mxml_type_t type);
extern void		mxmlOptionsSetWhitespaceCallback(mxml_options_t *options, mxml_ws_cb_t cb, void *cbdata);
extern void		mxmlOptionsSetWrapMargin(mxml_options_t *options, int column);

extern mxml_node_t	*mxmlNewCDATA(mxml_node_t *parent, const char *string);
extern mxml_node_t	*mxmlNewCDATAf(mxml_node_t *parent, const char *format, ...) MXML_FORMAT(2,3);
extern mxml_node_t	*mxmlNewComment(mxml_node_t *parent, const char *comment);
extern mxml_node_t	*mxmlNewCommentf(mxml_node_t *parent, const char *format, ...) MXML_FORMAT(2,3);
extern mxml_node_t	*mxmlNewCustom(mxml_node_t *parent, void *data, mxml_custfree_cb_t free_cb, void *free_cbdata);
extern mxml_node_t	*mxmlNewDeclaration(mxml_node_t *parent, const char *declaration);
extern mxml_node_t	*mxmlNewDeclarationf(mxml_node_t *parent, const char *format, ...) MXML_FORMAT(2,3);
extern mxml_node_t	*mxmlNewDirective(mxml_node_t *parent, const char *directive);
extern mxml_node_t	*mxmlNewDirectivef(mxml_node_t *parent, const char *format, ...) MXML_FORMAT(2,3);
extern mxml_node_t	*mxmlNewElement(mxml_node_t *parent, const char *name);
extern mxml_node_t	*mxmlNewInteger(mxml_node_t *parent, long integer);
extern mxml_node_t	*mxmlNewOpaque(mxml_node_t *parent, const char *opaque);
extern mxml_node_t	*mxmlNewOpaquef(mxml_node_t *parent, const char *format, ...) MXML_FORMAT(2,3);
extern mxml_node_t	*mxmlNewReal(mxml_node_t *parent, double real);
extern mxml_node_t	*mxmlNewText(mxml_node_t *parent, bool whitespace, const char *string);
extern mxml_node_t	*mxmlNewTextf(mxml_node_t *parent, bool whitespace, const char *format, ...) MXML_FORMAT(3,4);
extern mxml_node_t	*mxmlNewXML(const char *version);

extern int		mxmlRelease(mxml_node_t *node);
extern void		mxmlRemove(mxml_node_t *node);
extern int		mxmlRetain(mxml_node_t *node);

extern char		*mxmlSaveAllocString(mxml_node_t *node, mxml_options_t *options);
extern bool		mxmlSaveFd(mxml_node_t *node, mxml_options_t *options, int fd);
extern bool		mxmlSaveFile(mxml_node_t *node, mxml_options_t *options, FILE *fp);
extern bool		mxmlSaveFilename(mxml_node_t *node, mxml_options_t *options, const char *filename);
extern bool		mxmlSaveIO(mxml_node_t *node, mxml_options_t *options, mxml_io_cb_t io_cb, void *io_cbdata);
extern size_t		mxmlSaveString(mxml_node_t *node, mxml_options_t *options, char *buffer, size_t bufsize);

extern bool		mxmlSetCDATA(mxml_node_t *node, const char *data);
extern bool		mxmlSetCDATAf(mxml_node_t *node, const char *format, ...) MXML_FORMAT(2,3);
extern bool		mxmlSetComment(mxml_node_t *node, const char *comment);
extern bool		mxmlSetCommentf(mxml_node_t *node, const char *format, ...) MXML_FORMAT(2,3);
extern bool		mxmlSetDeclaration(mxml_node_t *node, const char *declaration);
extern bool		mxmlSetDeclarationf(mxml_node_t *node, const char *format, ...) MXML_FORMAT(2,3);
extern bool		mxmlSetDirective(mxml_node_t *node, const char *directive);
extern bool		mxmlSetDirectivef(mxml_node_t *node, const char *format, ...) MXML_FORMAT(2,3);
extern bool		mxmlSetCustom(mxml_node_t *node, void *data, mxml_custfree_cb_t free_cb, void *free_cbdata);
extern bool		mxmlSetElement(mxml_node_t *node, const char *name);
extern bool		mxmlSetInteger(mxml_node_t *node, long integer);
extern bool		mxmlSetOpaque(mxml_node_t *node, const char *opaque);
extern bool		mxmlSetOpaquef(mxml_node_t *node, const char *format, ...) MXML_FORMAT(2,3);
extern bool		mxmlSetReal(mxml_node_t *node, double real);
extern void		mxmlSetStringCallbacks(mxml_strcopy_cb_t strcopy_cb, mxml_strfree_cb_t strfree_cb, void *str_cbdata);
extern bool		mxmlSetText(mxml_node_t *node, bool whitespace, const char *string);
extern bool		mxmlSetTextf(mxml_node_t *node, bool whitespace, const char *format, ...) MXML_FORMAT(3,4);
extern bool		mxmlSetUserData(mxml_node_t *node, void *data);

extern mxml_node_t	*mxmlWalkNext(mxml_node_t *node, mxml_node_t *top, mxml_descend_t descend);
extern mxml_node_t	*mxmlWalkPrev(mxml_node_t *node, mxml_node_t *top, mxml_descend_t descend);


#  ifdef __cplusplus
}
#  endif // __cplusplus
#endif // !MXML_H
