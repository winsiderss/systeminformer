//
// Private definitions for Mini-XML, a small XML file parsing library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2003-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef MXML_PRIVATE_H
#  define MXML_PRIVATE_H
#  include "config.h"
#  include "mxml.h"
#  include <locale.h>


//
// Private macros...
//

#  ifdef ENABLE_MXML_DEBUG
#    define MXML_DEBUG(...)	fprintf(stderr, __VA_ARGS__)
#  else
#    define MXML_DEBUG(...)
#  endif // DEBUG
#  ifndef MXML_ALLOC_SIZE
#    define MXML_ALLOC_SIZE	16	// Allocation increment
#  endif // !MXML_ALLOC_SIZE
#  define MXML_TAB		8	// Tabs every N columns


//
// Private structures...
//

typedef struct _mxml_attr_s		// An XML element attribute value.
{
  char			*name;		// Attribute name
  char			*value;		// Attribute value
} _mxml_attr_t;

typedef struct _mxml_element_s		// An XML element value.
{
  char			*name;		// Name of element
  size_t		num_attrs;	// Number of attributes
  _mxml_attr_t		*attrs;		// Attributes
} _mxml_element_t;

typedef struct _mxml_text_s		// An XML text value.
{
  bool			whitespace;	// Leading whitespace?
  char			*string;	// Fragment string
} _mxml_text_t;

typedef struct _mxml_custom_s		// An XML custom value.
{
  void			*data;		// Pointer to (allocated) custom data
  mxml_custfree_cb_t	free_cb;	// Free callback function
  void			*free_cbdata;	// Free callback data
} _mxml_custom_t;

typedef union _mxml_value_u		// An XML node value.
{
  char			*cdata;		// CDATA string
  char			*comment;	// Common string
  char			*declaration;	// Declaration string
  char			*directive;	// Processing instruction string
  _mxml_element_t	element;	// Element
  long			integer;	// Integer number
  char			*opaque;	// Opaque string
  double		real;		// Real number
  _mxml_text_t		text;		// Text fragment
  _mxml_custom_t	custom;		// Custom data
} _mxml_value_t;

struct _mxml_node_s			// An XML node.
{
  mxml_type_t		type;		// Node type
  struct _mxml_node_s	*next;		// Next node under same parent
  struct _mxml_node_s	*prev;		// Previous node under same parent
  struct _mxml_node_s	*parent;	// Parent node
  struct _mxml_node_s	*child;		// First child node
  struct _mxml_node_s	*last_child;	// Last child node
  _mxml_value_t		value;		// Node value
  size_t		ref_count;	// Use count
  void			*user_data;	// User data
};

typedef struct _mxml_global_s		// Global, per-thread data
{
  mxml_strcopy_cb_t	strcopy_cb;	// String copy callback function
  mxml_strfree_cb_t	strfree_cb;	// String free callback function
  void			*str_cbdata;	// String callback data
} _mxml_global_t;

struct _mxml_index_s			// An XML node index.
{
  char			*attr;		// Attribute used for indexing or NULL
  size_t		num_nodes;	// Number of nodes in index
  size_t		alloc_nodes;	// Allocated nodes in index
  size_t		cur_node;	// Current node
  mxml_node_t		**nodes;	// Node array
};

struct _mxml_options_s			// XML options
{
  struct lconv		*loc;		// Locale data
  size_t		loc_declen;	// Length of decimal point string
  mxml_custload_cb_t	custload_cb;	// Custom load callback function
  mxml_custsave_cb_t	custsave_cb;	// Custom save callback function
  void			*cust_cbdata;	// Custom callback data
  mxml_entity_cb_t	entity_cb;	// Entity callback function
  void			*entity_cbdata;	// Entity callback data
  mxml_error_cb_t	error_cb;	// Error callback function
  void			*error_cbdata;	// Error callback data
  mxml_sax_cb_t		sax_cb;		// SAX callback function
  void			*sax_cbdata;	// SAX callback data
  mxml_type_cb_t	type_cb;	// Type callback function
  void			*type_cbdata;	// Type callback data
  mxml_type_t		type_value;	// Fixed type value (if no type callback)
  int			wrap;		// Wrap margin
  mxml_ws_cb_t		ws_cb;		// Whitespace callback function
  void			*ws_cbdata;	// Whitespace callback data
};


//
// Private functions...
//

extern _mxml_global_t	*_mxml_global(void);
extern const char	*_mxml_entity_string(int ch);
extern int		_mxml_entity_value(mxml_options_t *options, const char *name);
extern void		_mxml_error(mxml_options_t *options, const char *format, ...) MXML_FORMAT(2,3);
extern char		*_mxml_strcopy(const char *s);
extern void		_mxml_strfree(char *s);

#endif // !MXML_PRIVATE_H
