//
// Node support code for Mini-XML, a small XML file parsing library.
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
// Local functions...
//

static void		mxml_free(mxml_node_t *node);
static mxml_node_t	*mxml_new(mxml_node_t *parent, mxml_type_t type);


//
// 'mxmlAdd()' - Add a node to a tree.
//
// This function adds the specified node `node` to the parent.  If the `child`
// argument is not `NULL`, the new node is added before or after the specified
// child depending on the value of the `add` argument.  If the `child` argument
// is `NULL`, the new node is placed at the beginning of the child list
// (`MXML_ADD_BEFORE`) or at the end of the child list (`MXML_ADD_AFTER`).
//

void
mxmlAdd(mxml_node_t *parent,		// I - Parent node
        mxml_add_t  add,		// I - Where to add, `MXML_ADD_BEFORE` or `MXML_ADD_AFTER`
        mxml_node_t *child,		// I - Child node for where or `MXML_ADD_TO_PARENT`
	mxml_node_t *node)		// I - Node to add
{
  MXML_DEBUG("mxmlAdd(parent=%p, add=%d, child=%p, node=%p)\n", parent, add, child, node);

  // Range check input...
  if (!parent || !node)
    return;

  // Remove the node from any existing parent...
  if (node->parent)
    mxmlRemove(node);

  // Reset pointers...
  node->parent = parent;

  switch (add)
  {
    case MXML_ADD_BEFORE :
        if (!child || child == parent->child || child->parent != parent)
	{
	  // Insert as first node under parent...
	  node->next = parent->child;

	  if (parent->child)
	    parent->child->prev = node;
	  else
	    parent->last_child = node;

	  parent->child = node;
	}
	else
	{
	  // Insert node before this child...
	  node->next = child;
	  node->prev = child->prev;

	  if (child->prev)
	    child->prev->next = node;
	  else
	    parent->child = node;

	  child->prev = node;
	}
        break;

    case MXML_ADD_AFTER :
        if (!child || child == parent->last_child || child->parent != parent)
	{
	  // Insert as last node under parent...
	  node->parent = parent;
	  node->prev   = parent->last_child;

	  if (parent->last_child)
	    parent->last_child->next = node;
	  else
	    parent->child = node;

	  parent->last_child = node;
        }
	else
	{
	  // Insert node after this child...
	  node->prev = child;
	  node->next = child->next;

	  if (child->next)
	    child->next->prev = node;
	  else
	    parent->last_child = node;

	  child->next = node;
	}
        break;
  }
}


//
// 'mxmlDelete()' - Delete a node and all of its children.
//
// This function deletes the node `node` and all of its children.  If the
// specified node has a parent, this function first removes the node from its
// parent using the @link mxmlRemove@ function.
//

void
mxmlDelete(mxml_node_t *node)		// I - Node to delete
{
  mxml_node_t	*current,		// Current node
		*next;			// Next node


  MXML_DEBUG("mxmlDelete(node=%p)\n", node);

  // Range check input...
  if (!node)
    return;

  // Remove the node from its parent, if any...
  mxmlRemove(node);

  // Delete children...
  for (current = node->child; current; current = next)
  {
    // Get the next node...
    if ((next = current->child) != NULL)
    {
      // Free parent nodes after child nodes have been freed...
      current->child = NULL;
      continue;
    }

    if ((next = current->next) == NULL)
    {
      // Next node is the parent, which we'll free as needed...
      if ((next = current->parent) == node)
        next = NULL;
    }

    // Free child...
    mxml_free(current);
  }

  // Then free the memory used by the parent node...
  mxml_free(node);
}


//
// 'mxmlGetRefCount()' - Get the current reference (use) count for a node.
//
// The initial reference count of new nodes is 1. Use the @link mxmlRetain@
// and @link mxmlRelease@ functions to increment and decrement a node's
// reference count.
//

size_t					// O - Reference count
mxmlGetRefCount(mxml_node_t *node)	// I - Node
{
  // Range check input...
  if (!node)
    return (0);

  // Return the reference count...
  return (node->ref_count);
}


//
// 'mxmlNewCDATA()' - Create a new CDATA node.
//
// The new CDATA node is added to the end of the specified parent's child
// list.  The constant `NULL` can be used to specify that the new CDATA node
// has no parent.  The data string must be nul-terminated and is copied into the
// new node.
//

mxml_node_t *				// O - New node
mxmlNewCDATA(mxml_node_t *parent,	// I - Parent node or `NULL`
	     const char  *data)		// I - Data string
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewCDATA(parent=%p, data=\"%s\")\n", parent, data ? data : "(null)");

  // Range check input...
  if (!data)
    return (NULL);

  // Create the node and set the name value...
  if ((node = mxml_new(parent, MXML_TYPE_CDATA)) != NULL)
  {
    if ((node->value.cdata = _mxml_strcopy(data)) == NULL)
    {
      mxmlDelete(node);
      return (NULL);
    }
  }

  return (node);
}


//
// 'mxmlNewCDATAf()' - Create a new formatted CDATA node.
//
// The new CDATA node is added to the end of the specified parent's child list.
// The constant `NULL` can be used to specify that the new opaque string node
// has no parent.  The format string must be nul-terminated and is formatted
// into the new node.
//

mxml_node_t *				// O - New node
mxmlNewCDATAf(mxml_node_t *parent,	// I - Parent node or `NULL`
              const char  *format,	// I - Printf-style format string
	      ...)			// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewCDATAf(parent=%p, format=\"%s\", ...)\n", parent, format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_CDATA)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.cdata = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlNewComment()' - Create a new comment node.
//
// The new comment node is added to the end of the specified parent's child
// list.  The constant `NULL` can be used to specify that the new comment node
// has no parent.  The comment string must be nul-terminated and is copied into
// the new node.
//

mxml_node_t *				// O - New node
mxmlNewComment(mxml_node_t *parent,	// I - Parent node or `NULL`
	       const char  *comment)	// I - Comment string
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewComment(parent=%p, comment=\"%s\")\n", parent, comment ? comment : "(null)");

  // Range check input...
  if (!comment)
    return (NULL);

  // Create the node and set the name value...
  if ((node = mxml_new(parent, MXML_TYPE_COMMENT)) != NULL)
  {
    if ((node->value.comment = _mxml_strcopy(comment)) == NULL)
    {
      mxmlDelete(node);
      return (NULL);
    }
  }

  return (node);
}


//
// 'mxmlNewCommentf()' - Create a new formatted comment string node.
//
// The new comment string node is added to the end of the specified parent's
// child list.  The constant `NULL` can be used to specify that the new opaque
// string node has no parent.  The format string must be nul-terminated and is
// formatted into the new node.
//

mxml_node_t *				// O - New node
mxmlNewCommentf(mxml_node_t *parent,	// I - Parent node or `NULL`
                const char  *format,	// I - Printf-style format string
	        ...)			// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewCommentf(parent=%p, format=\"%s\", ...)\n", parent, format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_COMMENT)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.comment = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlNewCustom()' - Create a new custom data node.
//
// The new custom node is added to the end of the specified parent's child
// list.  The `free_cb` argument specifies a function to call to free the custom
// data when the node is deleted.
//

mxml_node_t *				// O - New node
mxmlNewCustom(
    mxml_node_t        *parent,		// I - Parent node or `NULL`
    void               *data,		// I - Pointer to data
    mxml_custfree_cb_t free_cb,		// I - Free callback function or `NULL` if none needed
    void               *free_cbdata)	// I - Free callback data
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewCustom(parent=%p, data=%p, free_cb=%p, free_cbdata=%p)\n", parent, data, free_cb, free_cbdata);

  // Create the node and set the value...
  if ((node = mxml_new(parent, MXML_TYPE_CUSTOM)) != NULL)
  {
    node->value.custom.data        = data;
    node->value.custom.free_cb     = free_cb;
    node->value.custom.free_cbdata = free_cbdata;
  }

  return (node);
}


//
// 'mxmlNewDeclaration()' - Create a new declaraction node.
//
// The new declaration node is added to the end of the specified parent's child
// list.  The constant `NULL` can be used to specify that the new
// declaration node has no parent.  The declaration string must be nul-
// terminated and is copied into the new node.
//

mxml_node_t *				// O - New node
mxmlNewDeclaration(
    mxml_node_t *parent,		// I - Parent node or `NULL`
    const char  *declaration)		// I - Declaration string
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewDeclaration(parent=%p, declaration=\"%s\")\n", parent, declaration ? declaration : "(null)");

  // Range check input...
  if (!declaration)
    return (NULL);

  // Create the node and set the name value...
  if ((node = mxml_new(parent, MXML_TYPE_DECLARATION)) != NULL)
  {
    if ((node->value.declaration = _mxml_strcopy(declaration)) == NULL)
    {
      mxmlDelete(node);
      return (NULL);
    }
  }

  return (node);
}


//
// 'mxmlNewDeclarationf()' - Create a new formatted declaration node.
//
// The new declaration node is added to the end of the specified parent's
// child list.  The constant `NULL` can be used to specify that
// the new opaque string node has no parent.  The format string must be
// nul-terminated and is formatted into the new node.
//

mxml_node_t *				// O - New node
mxmlNewDeclarationf(
    mxml_node_t *parent,		// I - Parent node or `NULL`
    const char  *format,		// I - Printf-style format string
    ...)				// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewDeclarationf(parent=%p, format=\"%s\", ...)\n", parent, format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_DECLARATION)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.declaration = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlNewDirective()' - Create a new processing instruction node.
//
// The new processing instruction node is added to the end of the specified
// parent's child list.  The constant `NULL` can be used to specify that the new
// processing instruction node has no parent.  The data string must be
// nul-terminated and is copied into the new node.
//

mxml_node_t *				// O - New node
mxmlNewDirective(mxml_node_t *parent,	// I - Parent node or `NULL`
	         const char  *directive)// I - Directive string
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewDirective(parent=%p, directive=\"%s\")\n", parent, directive ? directive : "(null)");

  // Range check input...
  if (!directive)
    return (NULL);

  // Create the node and set the name value...
  if ((node = mxml_new(parent, MXML_TYPE_DIRECTIVE)) != NULL)
  {
    if ((node->value.directive = _mxml_strcopy(directive)) == NULL)
    {
      mxmlDelete(node);
      return (NULL);
    }
  }

  return (node);
}


//
// 'mxmlNewDirectivef()' - Create a new formatted processing instruction node.
//
// The new processing instruction node is added to the end of the specified
// parent's child list.  The constant `NULL` can be used to specify that the new
// opaque string node has no parent.  The format string must be
// nul-terminated and is formatted into the new node.
//

mxml_node_t *				// O - New node
mxmlNewDirectivef(mxml_node_t *parent,	// I - Parent node or `NULL`
                  const char  *format,	// I - Printf-style format string
	          ...)			// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewDirectivef(parent=%p, format=\"%s\", ...)\n", parent, format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_DIRECTIVE)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.directive = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlNewElement()' - Create a new element node.
//
// The new element node is added to the end of the specified parent's child
// list. The constant `NULL` can be used to specify that the new element node
// has no parent.
//

mxml_node_t *				// O - New node
mxmlNewElement(mxml_node_t *parent,	// I - Parent node or `NULL`
               const char  *name)	// I - Name of element
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewElement(parent=%p, name=\"%s\")\n", parent, name ? name : "(null)");

  // Range check input...
  if (!name)
    return (NULL);

  // Create the node and set the element name...
  if ((node = mxml_new(parent, MXML_TYPE_ELEMENT)) != NULL)
    node->value.element.name = _mxml_strcopy(name);

  return (node);
}


//
// 'mxmlNewInteger()' - Create a new integer node.
//
// The new integer node is added to the end of the specified parent's child
// list. The constant `NULL` can be used to specify that the new integer node
// has no parent.
//

mxml_node_t *				// O - New node
mxmlNewInteger(mxml_node_t *parent,	// I - Parent node or `NULL`
               long        integer)	// I - Integer value
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewInteger(parent=%p, integer=%ld)\n", parent, integer);

  // Create the node and set the element name...
  if ((node = mxml_new(parent, MXML_TYPE_INTEGER)) != NULL)
    node->value.integer = integer;

  return (node);
}


//
// 'mxmlNewOpaque()' - Create a new opaque string.
//
// The new opaque string node is added to the end of the specified parent's
// child list.  The constant `NULL` can be used to specify that the new opaque
// string node has no parent.  The opaque string must be nul-terminated and is
// copied into the new node.
//

mxml_node_t *				// O - New node
mxmlNewOpaque(mxml_node_t *parent,	// I - Parent node or `NULL`
              const char  *opaque)	// I - Opaque string
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewOpaque(parent=%p, opaque=\"%s\")\n", parent, opaque ? opaque : "(null)");

  // Range check input...
  if (!opaque)
    return (NULL);

  // Create the node and set the element name...
  if ((node = mxml_new(parent, MXML_TYPE_OPAQUE)) != NULL)
    node->value.opaque = _mxml_strcopy(opaque);

  return (node);
}


//
// 'mxmlNewOpaquef()' - Create a new formatted opaque string node.
//
// The new opaque string node is added to the end of the specified parent's
// child list.  The constant `NULL` can be used to specify that the new opaque
// string node has no parent.  The format string must be nul-terminated and is
// formatted into the new node.
//

mxml_node_t *				// O - New node
mxmlNewOpaquef(mxml_node_t *parent,	// I - Parent node or `NULL`
               const char  *format,	// I - Printf-style format string
	       ...)			// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewOpaquef(parent=%p, format=\"%s\", ...)\n", parent, format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_OPAQUE)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.opaque = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlNewReal()' - Create a new real number node.
//
// The new real number node is added to the end of the specified parent's
// child list.  The constant `NULL` can be used to specify that the new real
// number node has no parent.
//

mxml_node_t *				// O - New node
mxmlNewReal(mxml_node_t *parent,	// I - Parent node or `NULL`
            double      real)		// I - Real number value
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewReal(parent=%p, real=%g)\n", parent, real);

  // Create the node and set the element name...
  if ((node = mxml_new(parent, MXML_TYPE_REAL)) != NULL)
    node->value.real = real;

  return (node);
}


//
// 'mxmlNewText()' - Create a new text fragment node.
//
// The new text node is added to the end of the specified parent's child
// list.  The constant `NULL` can be used to specify that the new text node has
// no parent.  The whitespace parameter is used to specify whether leading
// whitespace is present before the node.  The text string must be
// nul-terminated and is copied into the new node.
//

mxml_node_t *				// O - New node
mxmlNewText(mxml_node_t *parent,	// I - Parent node or `NULL`
            bool        whitespace,	// I - `true` = leading whitespace, `false` = no whitespace
	    const char  *string)	// I - String
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxmlNewText(parent=%p, whitespace=%s, string=\"%s\")\n", parent, whitespace ? "true" : "false", string ? string : "(null)");

  // Range check input...
  if (!string)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_TEXT)) != NULL)
  {
    node->value.text.whitespace = whitespace;
    node->value.text.string     = _mxml_strcopy(string);
  }

  return (node);
}


//
// 'mxmlNewTextf()' - Create a new formatted text fragment node.
//
// The new text node is added to the end of the specified parent's child
// list.  The constant `NULL` can be used to specify that the new text node has
// no parent.  The whitespace parameter is used to specify whether leading
// whitespace is present before the node.  The format string must be
// nul-terminated and is formatted into the new node.
//

mxml_node_t *				// O - New node
mxmlNewTextf(mxml_node_t *parent,	// I - Parent node or `NULL`
             bool        whitespace,	// I - `true` = leading whitespace, `false` = no whitespace
	     const char  *format,	// I - Printf-style format string
	     ...)			// I - Additional args as needed
{
  mxml_node_t	*node;			// New node
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer


  MXML_DEBUG("mxmlNewTextf(parent=%p, whitespace=%s, format=\"%s\", ...)\n", parent, whitespace ? "true" : "false", format ? format : "(null)");

  // Range check input...
  if (!format)
    return (NULL);

  // Create the node and set the text value...
  if ((node = mxml_new(parent, MXML_TYPE_TEXT)) != NULL)
  {
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    node->value.text.whitespace = whitespace;
    node->value.text.string     = _mxml_strcopy(buffer);
  }

  return (node);
}


//
// 'mxmlRemove()' - Remove a node from its parent.
//
// This function does not free memory used by the node - use @link mxmlDelete@
// for that.  This function does nothing if the node has no parent.
//

void
mxmlRemove(mxml_node_t *node)		// I - Node to remove
{
  MXML_DEBUG("mxmlRemove(node=%p)\n", node);

  // Range check input...
  if (!node || !node->parent)
    return;

  // Remove from parent...
  if (node->prev)
    node->prev->next = node->next;
  else
    node->parent->child = node->next;

  if (node->next)
    node->next->prev = node->prev;
  else
    node->parent->last_child = node->prev;

  node->parent = NULL;
  node->prev   = NULL;
  node->next   = NULL;
}


//
// 'mxmlNewXML()' - Create a new XML document tree.
//
// The "version" argument specifies the version number to put in the
// ?xml directive node. If `NULL`, version "1.0" is assumed.
//

mxml_node_t *				// O - New ?xml node
mxmlNewXML(const char *version)		// I - Version number to use
{
  char	directive[1024];		// Directive text


  snprintf(directive, sizeof(directive), "xml version=\"%s\" encoding=\"utf-8\"", version ? version : "1.0");

  return (mxmlNewDirective(NULL, directive));
}


//
// 'mxmlRelease()' - Release a node.
//
// When the reference count reaches zero, the node (and any children)
// is deleted via @link mxmlDelete@.
//

int					// O - New reference count
mxmlRelease(mxml_node_t *node)		// I - Node
{
  if (node)
  {
    if ((-- node->ref_count) <= 0)
    {
      mxmlDelete(node);
      return (0);
    }
    else if (node->ref_count < INT_MAX)
    {
      return ((int)node->ref_count);
    }
    else
    {
      return (INT_MAX);
    }
  }
  else
  {
    return (-1);
  }
}


//
// 'mxmlRetain()' - Retain a node.
//

int					// O - New reference count
mxmlRetain(mxml_node_t *node)		// I - Node
{
  if (node)
  {
    node->ref_count ++;

    if (node->ref_count < INT_MAX)
      return ((int)node->ref_count);
    else
      return (INT_MAX);
  }
  else
  {
    return (-1);
  }
}


//
// 'mxml_free()' - Free the memory used by a node.
//
// Note: Does not free child nodes, does not remove from parent.
//

static void
mxml_free(mxml_node_t *node)		// I - Node
{
  size_t	i;			// Looping var


  switch (node->type)
  {
    case MXML_TYPE_CDATA :
	_mxml_strfree(node->value.cdata);
        break;
    case MXML_TYPE_COMMENT :
	_mxml_strfree(node->value.comment);
        break;
    case MXML_TYPE_DECLARATION :
	_mxml_strfree(node->value.declaration);
        break;
    case MXML_TYPE_DIRECTIVE :
	_mxml_strfree(node->value.directive);
        break;
    case MXML_TYPE_ELEMENT :
	_mxml_strfree(node->value.element.name);

	if (node->value.element.num_attrs)
	{
	  for (i = 0; i < node->value.element.num_attrs; i ++)
	  {
	    _mxml_strfree(node->value.element.attrs[i].name);
	    _mxml_strfree(node->value.element.attrs[i].value);
	  }

          free(node->value.element.attrs);
	}
        break;
    case MXML_TYPE_INTEGER :
       // Nothing to do
        break;
    case MXML_TYPE_OPAQUE :
	_mxml_strfree(node->value.opaque);
        break;
    case MXML_TYPE_REAL :
       // Nothing to do
        break;
    case MXML_TYPE_TEXT :
	_mxml_strfree(node->value.text.string);
        break;
    case MXML_TYPE_CUSTOM :
        if (node->value.custom.data && node->value.custom.free_cb)
	  (node->value.custom.free_cb)(node->value.custom.free_cbdata, node->value.custom.data);
	break;
    default :
        break;
  }

  // Free this node...
  free(node);
}


//
// 'mxml_new()' - Create a new node.
//

static mxml_node_t *			// O - New node
mxml_new(mxml_node_t *parent,		// I - Parent node
         mxml_type_t type)		// I - Node type
{
  mxml_node_t	*node;			// New node


  MXML_DEBUG("mxml_new(parent=%p, type=%d)\n", parent, type);

  // Allocate memory for the node...
  if ((node = calloc(1, sizeof(mxml_node_t))) == NULL)
  {
    MXML_DEBUG("mxml_new: Returning NULL\n");
    return (NULL);
  }

  MXML_DEBUG("mxml_new: Returning %p\n", node);

  // Set the node type...
  node->type      = type;
  node->ref_count = 1;

  // Add to the parent if present...
  if (parent)
    mxmlAdd(parent, MXML_ADD_AFTER, /*child*/NULL, node);

  // Return the new node...
  return (node);
}
