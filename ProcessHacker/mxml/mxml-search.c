/*
 * "$Id: mxml-search.c 297 2007-09-09 07:16:52Z mike $"
 *
 * Search/navigation functions for Mini-XML, a small XML-like file
 * parsing library.
 *
 * Copyright 2003-2007 by Michael Sweet.
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
 *   mxmlFindElement() - Find the named element.
 *   mxmlWalkNext()    - Walk to the next logical node in the tree.
 *   mxmlWalkPrev()    - Walk to the previous logical node in the tree.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"


/*
 * 'mxmlFindElement()' - Find the named element.
 *
 * The search is constrained by the name, attribute name, and value; any
 * NULL names or values are treated as wildcards, so different kinds of
 * searches can be implemented by looking for all elements of a given name
 * or all elements with a specific attribute. The descend argument determines
 * whether the search descends into child nodes; normally you will use
 * MXML_DESCEND_FIRST for the initial search and MXML_NO_DESCEND to find
 * additional direct descendents of the node. The top node argument
 * constrains the search to a particular node's children.
 */

mxml_node_t *				/* O - Element node or NULL */
mxmlFindElement(mxml_node_t *node,	/* I - Current node */
                mxml_node_t *top,	/* I - Top node */
                const char  *name,	/* I - Element name or NULL for any */
		const char  *attr,	/* I - Attribute name, or NULL for none */
		const char  *value,	/* I - Attribute value, or NULL for any */
		int         descend)	/* I - Descend into tree - MXML_DESCEND, MXML_NO_DESCEND, or MXML_DESCEND_FIRST */
{
  const char	*temp;			/* Current attribute value */


 /*
  * Range check input...
  */

  if (!node || !top || (!attr && value))
    return (NULL);

 /*
  * Start with the next node...
  */

  node = mxmlWalkNext(node, top, descend);

 /*
  * Loop until we find a matching element...
  */

  while (node != NULL)
  {
   /*
    * See if this node matches...
    */

    if (node->type == MXML_ELEMENT &&
        node->value.element.name &&
	(!name || !strcmp(node->value.element.name, name)))
    {
     /*
      * See if we need to check for an attribute...
      */

      if (!attr)
        return (node);			/* No attribute search, return it... */

     /*
      * Check for the attribute...
      */

      if ((temp = mxmlElementGetAttr(node, attr)) != NULL)
      {
       /*
        * OK, we have the attribute, does it match?
	*/

	if (!value || !strcmp(value, temp))
	  return (node);		/* Yes, return it... */
      }
    }

   /*
    * No match, move on to the next node...
    */

    if (descend == MXML_DESCEND)
      node = mxmlWalkNext(node, top, MXML_DESCEND);
    else
      node = node->next;
  }

  return (NULL);
}


/*
 * 'mxmlWalkNext()' - Walk to the next logical node in the tree.
 *
 * The descend argument controls whether the first child is considered
 * to be the next node. The top node argument constrains the walk to
 * the node's children.
 */

mxml_node_t *				/* O - Next node or NULL */
mxmlWalkNext(mxml_node_t *node,		/* I - Current node */
             mxml_node_t *top,		/* I - Top node */
             int         descend)	/* I - Descend into tree - MXML_DESCEND, MXML_NO_DESCEND, or MXML_DESCEND_FIRST */
{
  if (!node)
    return (NULL);
  else if (node->child && descend)
    return (node->child);
  else if (node == top)
    return (NULL);
  else if (node->next)
    return (node->next);
  else if (node->parent && node->parent != top)
  {
    node = node->parent;

    while (!node->next)
      if (node->parent == top || !node->parent)
        return (NULL);
      else
        node = node->parent;

    return (node->next);
  }
  else
    return (NULL);
}


/*
 * 'mxmlWalkPrev()' - Walk to the previous logical node in the tree.
 *
 * The descend argument controls whether the previous node's last child
 * is considered to be the previous node. The top node argument constrains
 * the walk to the node's children.
 */

mxml_node_t *				/* O - Previous node or NULL */
mxmlWalkPrev(mxml_node_t *node,		/* I - Current node */
             mxml_node_t *top,		/* I - Top node */
             int         descend)	/* I - Descend into tree - MXML_DESCEND, MXML_NO_DESCEND, or MXML_DESCEND_FIRST */
{
  if (!node || node == top)
    return (NULL);
  else if (node->prev)
  {
    if (node->prev->last_child && descend)
    {
     /*
      * Find the last child under the previous node...
      */

      node = node->prev->last_child;

      while (node->last_child)
        node = node->last_child;

      return (node);
    }
    else
      return (node->prev);
  }
  else if (node->parent != top)
    return (node->parent);
  else
    return (NULL);
}


/*
 * End of "$Id: mxml-search.c 297 2007-09-09 07:16:52Z mike $".
 */
