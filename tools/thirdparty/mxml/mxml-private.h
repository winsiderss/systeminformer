/*
 * Private definitions for Mini-XML, a small XML file parsing library.
 *
 * https://www.msweet.org/mxml
 *
 * Copyright © 2003-2019 by Michael R Sweet.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

#ifndef _mxml_private_h_
#  define _mxml_private_h_

/*
 * Include necessary headers...
 */

#  include "config.h"
#  include "mxml.h"


/*
 * Private structures...
 */

typedef struct _mxml_attr_s		/**** An XML element attribute value. ****/
{
  char			*name;		/* Attribute name */
  char			*value;		/* Attribute value */
} _mxml_attr_t;

typedef struct _mxml_element_s		/**** An XML element value. ****/
{
  char			*name;		/* Name of element */
  int			num_attrs;	/* Number of attributes */
  _mxml_attr_t		*attrs;		/* Attributes */
} _mxml_element_t;

typedef struct _mxml_text_s		/**** An XML text value. ****/
{
  int			whitespace;	/* Leading whitespace? */
  char			*string;	/* Fragment string */
} _mxml_text_t;

typedef struct _mxml_custom_s		/**** An XML custom value. ****/
{
  void			*data;		/* Pointer to (allocated) custom data */
  mxml_custom_destroy_cb_t destroy;	/* Pointer to destructor function */
} _mxml_custom_t;

typedef union _mxml_value_u		/**** An XML node value. ****/
{
  _mxml_element_t	element;	/* Element */
  int			integer;	/* Integer number */
  char			*opaque;	/* Opaque string */
  double		real;		/* Real number */
  _mxml_text_t		text;		/* Text fragment */
  _mxml_custom_t	custom;		/* Custom data @since Mini-XML 2.1@ */
} _mxml_value_t;

struct _mxml_node_s			/**** An XML node. ****/
{
  mxml_type_t		type;		/* Node type */
  struct _mxml_node_s	*next;		/* Next node under same parent */
  struct _mxml_node_s	*prev;		/* Previous node under same parent */
  struct _mxml_node_s	*parent;	/* Parent node */
  struct _mxml_node_s	*child;		/* First child node */
  struct _mxml_node_s	*last_child;	/* Last child node */
  _mxml_value_t		value;		/* Node value */
  int			ref_count;	/* Use count */
  void			*user_data;	/* User data */
};

struct _mxml_index_s			 /**** An XML node index. ****/
{
  char			*attr;		/* Attribute used for indexing or NULL */
  int			num_nodes;	/* Number of nodes in index */
  int			alloc_nodes;	/* Allocated nodes in index */
  int			cur_node;	/* Current node */
  mxml_node_t		**nodes;	/* Node array */
};

typedef struct _mxml_global_s		/**** Global, per-thread data ****/

{
  void	(*error_cb)(const char *);
  int	num_entity_cbs;
  int	(*entity_cbs[100])(const char *name);
  int	wrap;
  mxml_custom_load_cb_t	custom_load_cb;
  mxml_custom_save_cb_t	custom_save_cb;
} _mxml_global_t;


/*
 * Functions...
 */

extern _mxml_global_t	*_mxml_global(void);
extern int		_mxml_entity_cb(const char *name);

#endif /* !_mxml_private_h_ */
