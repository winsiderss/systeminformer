//
// Node get functions for Mini-XML, a small XML file parsing library.
//
// https://www.msweet.org/mxml
//
// Copyright © 2014-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "mxml-private.h"


//
// 'mxmlGetCDATA()' - Get the value for a CDATA node.
//
// This function gets the string value of a CDATA node.  `NULL` is returned if
// the node is not a CDATA element.
//

const char *				// O - CDATA value or `NULL`
mxmlGetCDATA(mxml_node_t *node)		// I - Node to get
{
  // Range check input...
  if (!node || node->type != MXML_TYPE_CDATA)
    return (NULL);

  // Return the CDATA string...
  return (node->value.cdata);
}


//
// 'mxmlGetComment()' - Get the value for a comment node.
//
// This function gets the string value of a comment node.  `NULL` is returned
// if the node is not a comment.
//

const char *				// O - Comment value or `NULL`
mxmlGetComment(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node || node->type != MXML_TYPE_COMMENT)
    return (NULL);

  // Return the comment string...
  return (node->value.comment);
}


//
// 'mxmlGetCustom()' - Get the value for a custom node.
//
// This function gets the binary value of a custom node.  `NULL` is returned if
// the node (or its first child) is not a custom value node.
//

const void *				// O - Custom value or `NULL`
mxmlGetCustom(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node)
    return (NULL);

  // Return the custom value...
  if (node->type == MXML_TYPE_CUSTOM)
    return (node->value.custom.data);
  else if (node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_CUSTOM)
    return (node->child->value.custom.data);
  else
    return (NULL);
}


//
// 'mxmlGetDeclaration()' - Get the value for a declaration node.
//
// This function gets the string value of a declaraction node.  `NULL` is
// returned if the node is not a declaration.
//

const char *				// O - Declaraction value or `NULL`
mxmlGetDeclaration(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node || node->type != MXML_TYPE_DECLARATION)
    return (NULL);

  // Return the comment string...
  return (node->value.declaration);
}


//
// 'mxmlGetDirective()' - Get the value for a processing instruction node.
//
// This function gets the string value of a processing instruction.  `NULL` is
// returned if the node is not a processing instruction.
//

const char *				// O - Comment value or `NULL`
mxmlGetDirective(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node || node->type != MXML_TYPE_DIRECTIVE)
    return (NULL);

  // Return the comment string...
  return (node->value.directive);
}


//
// 'mxmlGetElement()' - Get the name for an element node.
//
// This function gets the name of an element node.  `NULL` is returned if the
// node is not an element node.
//

const char *				// O - Element name or `NULL`
mxmlGetElement(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node || node->type != MXML_TYPE_ELEMENT)
    return (NULL);

  // Return the element name...
  return (node->value.element.name);
}


//
// 'mxmlGetFirstChild()' - Get the first child of a node.
//
// This function gets the first child of a node.  `NULL` is returned if the node
// has no children.
//

mxml_node_t *				// O - First child or `NULL`
mxmlGetFirstChild(mxml_node_t *node)	// I - Node to get
{
  // Return the first child node...
  return (node ? node->child : NULL);
}


//
// 'mxmlGetInteger()' - Get the integer value from the specified node or its
//                      first child.
//
// This function gets the value of an integer node.  `0` is returned if the node
// (or its first child) is not an integer value node.
//

long					// O - Integer value or `0`
mxmlGetInteger(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node)
    return (0);

  // Return the integer value...
  if (node->type == MXML_TYPE_INTEGER)
    return (node->value.integer);
  else if (node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_INTEGER)
    return (node->child->value.integer);
  else
    return (0);
}


//
// 'mxmlGetLastChild()' - Get the last child of a node.
//
// This function gets the last child of a node.  `NULL` is returned if the node
// has no children.
//

mxml_node_t *				// O - Last child or `NULL`
mxmlGetLastChild(mxml_node_t *node)	// I - Node to get
{
  return (node ? node->last_child : NULL);
}


//
// 'mxmlGetNextSibling()' - Get the next node for the current parent.
//
// This function gets the next node for the current parent.  `NULL` is returned
// if this is the last child for the current parent.
//

mxml_node_t *
mxmlGetNextSibling(mxml_node_t *node)	// I - Node to get
{
  return (node ? node->next : NULL);
}


//
// 'mxmlGetOpaque()' - Get an opaque string value for a node or its first child.
//
// This function gets the string value of an opaque node.  `NULL` is returned if
// the node (or its first child) is not an opaque value node.
//

const char *				// O - Opaque string or `NULL`
mxmlGetOpaque(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node)
    return (NULL);

  // Return the opaque value...
  if (node->type == MXML_TYPE_OPAQUE)
    return (node->value.opaque);
  else if (node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_OPAQUE)
    return (node->child->value.opaque);
  else
    return (NULL);
}


//
// 'mxmlGetParent()' - Get the parent node.
//
// This function gets the parent of a node.  `NULL` is returned for a root node.
//

mxml_node_t *				// O - Parent node or `NULL`
mxmlGetParent(mxml_node_t *node)	// I - Node to get
{
  return (node ? node->parent : NULL);
}


//
// 'mxmlGetPrevSibling()' - Get the previous node for the current parent.
//
// This function gets the previous node for the current parent.  `NULL` is
// returned if this is the first child for the current parent.
//

mxml_node_t *				// O - Previous node or `NULL`
mxmlGetPrevSibling(mxml_node_t *node)	// I - Node to get
{
  return (node ? node->prev : NULL);
}


//
// 'mxmlGetReal()' - Get the real value for a node or its first child.
//
// This function gets the value of a real value node.  `0.0` is returned if the
// node (or its first child) is not a real value node.
//

double					// O - Real value or 0.0
mxmlGetReal(mxml_node_t *node)		// I - Node to get
{
  // Range check input...
  if (!node)
    return (0.0);

  // Return the real value...
  if (node->type == MXML_TYPE_REAL)
    return (node->value.real);
  else if (node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_REAL)
    return (node->child->value.real);
  else
    return (0.0);
}


//
// 'mxmlGetText()' - Get the text value for a node or its first child.
//
// This function gets the string and whitespace values of a text node.  `NULL`
// and `false` are returned if the node (or its first child) is not a text node.
// The `whitespace` argument can be `NULL` if you don't want to know the
// whitespace value.
//
// Note: Text nodes consist of whitespace-delimited words. You will only get
// single words of text when reading an XML file with `MXML_TYPE_TEXT` nodes.
// If you want the entire string between elements in the XML file, you MUST read
// the XML file with `MXML_TYPE_OPAQUE` nodes and get the resulting strings
// using the @link mxmlGetOpaque@ function instead.
//

const char *				// O - Text string or `NULL`
mxmlGetText(mxml_node_t *node,		// I - Node to get
            bool        *whitespace)	// O - `true` if string is preceded by whitespace, `false` otherwise
{
  // Range check input...
  if (!node)
  {
    if (whitespace)
      *whitespace = false;

    return (NULL);
  }

  // Return the integer value...
  if (node->type == MXML_TYPE_TEXT)
  {
    if (whitespace)
      *whitespace = node->value.text.whitespace;

    return (node->value.text.string);
  }
  else if (node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_TEXT)
  {
    if (whitespace)
      *whitespace = node->child->value.text.whitespace;

    return (node->child->value.text.string);
  }
  else
  {
    if (whitespace)
      *whitespace = false;

    return (NULL);
  }
}


//
// 'mxmlGetType()' - Get the node type.
//
// This function gets the type of `node`.  `MXML_TYPE_IGNORE` is returned if
// `node` is `NULL`.
//

mxml_type_t				// O - Type of node
mxmlGetType(mxml_node_t *node)		// I - Node to get
{
  // Range check input...
  if (!node)
    return (MXML_TYPE_IGNORE);

  // Return the node type...
  return (node->type);
}


//
// 'mxmlGetUserData()' - Get the user data pointer for a node.
//
// This function gets the user data pointer associated with `node`.
//

void *					// O - User data pointer
mxmlGetUserData(mxml_node_t *node)	// I - Node to get
{
  // Range check input...
  if (!node)
    return (NULL);

  // Return the user data pointer...
  return (node->user_data);
}
