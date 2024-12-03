//
// Node set functions for Mini-XML, a small XML file parsing library.
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
// 'mxmlSetCDATA()' - Set the data for a CDATA node.
//
// This function sets the value string for a CDATA node.  The node is not
// changed if it (or its first child) is not a CDATA node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetCDATA(mxml_node_t *node,		// I - Node to set
             const char  *data)		// I - New data string
{
  char	*s;				// New element name


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_CDATA)
    node = node->child;

  if (!node || node->type != MXML_TYPE_CDATA)
    return (false);
  else if (!data)
    return (false);

  if (data == node->value.cdata)
  {
    // Don't change the value...
    return (true);
  }

  // Allocate the new value, free any old element value, and set the new value...
  if ((s = _mxml_strcopy(data)) == NULL)
    return (false);

  _mxml_strfree(node->value.cdata);
  node->value.cdata = s;

  return (true);
}


//
// 'mxmlSetCDATAf()' - Set the data for a CDATA to a formatted string.
//
// This function sets the formatted string value of a CDATA node.  The node is
// not changed if it (or its first child) is not a CDATA node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetCDATAf(mxml_node_t *node,	// I - Node
	      const char *format,	// I - `printf`-style format string
	      ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_CDATA)
    node = node->child;

  if (!node || node->type != MXML_TYPE_CDATA)
    return (false);
  else if (!format)
    return (false);

  // Format the new string, free any old string value, and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.cdata);
  node->value.cdata = s;

  return (true);
}


//
// 'mxmlSetComment()' - Set a comment to a literal string.
//
// This function sets the string value of a comment node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetComment(mxml_node_t *node,	// I - Node
               const char  *comment)	// I - Literal string
{
  char *s;				// New string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_COMMENT)
    node = node->child;

  if (!node || node->type != MXML_TYPE_COMMENT)
    return (false);
  else if (!comment)
    return (false);

  if (comment == node->value.comment)
    return (true);

  // Free any old string value and set the new value...
  if ((s = _mxml_strcopy(comment)) == NULL)
    return (false);

  _mxml_strfree(node->value.comment);
  node->value.comment = s;

  return (true);
}


//
// 'mxmlSetCommentf()' - Set a comment to a formatted string.
//
// This function sets the formatted string value of a comment node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetCommentf(mxml_node_t *node,	// I - Node
                const char *format,	// I - `printf`-style format string
		...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_COMMENT)
    node = node->child;

  if (!node || node->type != MXML_TYPE_COMMENT)
    return (false);
  else if (!format)
    return (false);

  // Format the new string, free any old string value, and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.comment);
  node->value.comment = s;

  return (true);
}


//
// 'mxmlSetCustom()' - Set the data and destructor of a custom data node.
//
// This function sets the data pointer `data` and destructor callback
// `destroy_cb` of a custom data node.  The node is not changed if it (or its
// first child) is not a custom node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetCustom(
    mxml_node_t        *node,		// I - Node to set
    void               *data,		// I - New data pointer
    mxml_custfree_cb_t free_cb,		// I - Free callback function
    void               *free_cbdata)	// I - Free callback data
{
  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_CUSTOM)
    node = node->child;

  if (!node || node->type != MXML_TYPE_CUSTOM)
    return (false);

  if (data == node->value.custom.data)
    goto set_free_callback;

  // Free any old element value and set the new value...
  if (node->value.custom.data && node->value.custom.free_cb)
    (node->value.custom.free_cb)(node->value.custom.free_cbdata, node->value.custom.data);

  node->value.custom.data = data;

  set_free_callback:

  node->value.custom.free_cb     = free_cb;
  node->value.custom.free_cbdata = free_cbdata;

  return (true);
}


//
// 'mxmlSetDeclaration()' - Set a declaration to a literal string.
//
// This function sets the string value of a declaration node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetDeclaration(
    mxml_node_t *node,			// I - Node
    const char  *declaration)		// I - Literal string
{
  char *s;				// New string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_DECLARATION)
    node = node->child;

  if (!node || node->type != MXML_TYPE_DECLARATION)
    return (false);
  else if (!declaration)
    return (false);

  if (declaration == node->value.declaration)
    return (true);

  // Free any old string value and set the new value...
  if ((s = _mxml_strcopy(declaration)) == NULL)
    return (false);

  _mxml_strfree(node->value.declaration);
  node->value.declaration = s;

  return (true);
}


//
// 'mxmlSetDeclarationf()' - Set a declaration to a formatted string.
//
// This function sets the formatted string value of a declaration node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetDeclarationf(mxml_node_t *node,	// I - Node
                    const char *format,	// I - `printf`-style format string
		    ...)		// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_DECLARATION)
    node = node->child;

  if (!node || node->type != MXML_TYPE_DECLARATION)
    return (false);
  else if (!format)
    return (false);

  // Format the new string, free any old string value, and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.declaration);
  node->value.declaration = s;

  return (true);
}


//
// 'mxmlSetDirective()' - Set a processing instruction to a literal string.
//
// This function sets the string value of a processing instruction node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetDirective(mxml_node_t *node,	// I - Node
                 const char  *directive)// I - Literal string
{
  char *s;				// New string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_DIRECTIVE)
    node = node->child;

  if (!node || node->type != MXML_TYPE_DIRECTIVE)
    return (false);
  else if (!directive)
    return (false);

  if (directive == node->value.directive)
    return (true);

  // Free any old string value and set the new value...
  if ((s = _mxml_strcopy(directive)) == NULL)
    return (false);

  _mxml_strfree(node->value.directive);
  node->value.directive = s;

  return (true);
}


//
// 'mxmlSetDirectivef()' - Set a processing instruction to a formatted string.
//
// This function sets the formatted string value of a processing instruction
// node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetDirectivef(mxml_node_t *node,	// I - Node
                  const char *format,	// I - `printf`-style format string
		  ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_DIRECTIVE)
    node = node->child;

  if (!node || node->type != MXML_TYPE_DIRECTIVE)
    return (false);
  else if (!format)
    return (false);

  // Format the new string, free any old string value, and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.directive);
  node->value.directive = s;

  return (true);
}


//
// 'mxmlSetElement()' - Set the name of an element node.
//
// This function sets the name of an element node.  The node is not changed if
// it is not an element node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetElement(mxml_node_t *node,	// I - Node to set
               const char  *name)	// I - New name string
{
  char *s;				// New name string


  // Range check input...
  if (!node || node->type != MXML_TYPE_ELEMENT)
    return (false);
  else if (!name)
    return (false);

  if (name == node->value.element.name)
    return (true);

  // Free any old element value and set the new value...
  if ((s = _mxml_strcopy(name)) == NULL)
    return (false);

  _mxml_strfree(node->value.element.name);
  node->value.element.name = s;

  return (true);
}


//
// 'mxmlSetInteger()' - Set the value of an integer node.
//
// This function sets the value of an integer node.  The node is not changed if
// it (or its first child) is not an integer node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetInteger(mxml_node_t *node,	// I - Node to set
               long        integer)	// I - Integer value
{
  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_INTEGER)
    node = node->child;

  if (!node || node->type != MXML_TYPE_INTEGER)
    return (false);

  // Set the new value and return...
  node->value.integer = integer;

  return (true);
}


//
// 'mxmlSetOpaque()' - Set the value of an opaque node.
//
// This function sets the string value of an opaque node.  The node is not
// changed if it (or its first child) is not an opaque node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetOpaque(mxml_node_t *node,	// I - Node to set
              const char  *opaque)	// I - Opaque string
{
  char *s;				// New opaque string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_OPAQUE)
    node = node->child;

  if (!node || node->type != MXML_TYPE_OPAQUE)
    return (false);
  else if (!opaque)
    return (false);

  if (node->value.opaque == opaque)
    return (true);

  // Free any old opaque value and set the new value...
  if ((s = _mxml_strcopy(opaque)) == NULL)
    return (false);

  _mxml_strfree(node->value.opaque);
  node->value.opaque = s;

  return (true);
}


//
// 'mxmlSetOpaquef()' - Set the value of an opaque string node to a formatted string.
//
// This function sets the formatted string value of an opaque node.  The node is
// not changed if it (or its first child) is not an opaque node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetOpaquef(mxml_node_t *node,	// I - Node to set
               const char  *format,	// I - Printf-style format string
	       ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_OPAQUE)
    node = node->child;

  if (!node || node->type != MXML_TYPE_OPAQUE)
    return (false);
  else if (!format)
    return (false);

  // Format the new string, free any old string value, and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.opaque);
  node->value.opaque = s;

  return (true);
}


//
// 'mxmlSetReal()' - Set the value of a real value node.
//
// This function sets the value of a real value node.  The node is not changed
// if it (or its first child) is not a real value node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetReal(mxml_node_t *node,		// I - Node to set
            double      real)		// I - Real number value
{
  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_REAL)
    node = node->child;

  if (!node || node->type != MXML_TYPE_REAL)
    return (false);

  // Set the new value and return...
  node->value.real = real;

  return (true);
}


//
// 'mxmlSetText()' - Set the value of a text node.
//
// This function sets the string and whitespace values of a text node.  The node
// is not changed if it (or its first child) is not a text node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetText(mxml_node_t *node,		// I - Node to set
            bool        whitespace,	// I - `true` = leading whitespace, `false` = no whitespace
	    const char  *string)	// I - String
{
  char *s;				// New string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_TEXT)
    node = node->child;

  if (!node || node->type != MXML_TYPE_TEXT)
    return (false);
  else if (!string)
    return (false);

  if (string == node->value.text.string)
  {
    node->value.text.whitespace = whitespace;
    return (true);
  }

  // Free any old string value and set the new value...
  if ((s = _mxml_strcopy(string)) == NULL)
    return (false);

  _mxml_strfree(node->value.text.string);

  node->value.text.whitespace = whitespace;
  node->value.text.string     = s;

  return (true);
}


//
// 'mxmlSetTextf()' - Set the value of a text node to a formatted string.
//
// This function sets the formatted string and whitespace values of a text node.
// The node is not changed if it (or its first child) is not a text node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetTextf(mxml_node_t *node,		// I - Node to set
             bool        whitespace,	// I - `true` = leading whitespace, `false` = no whitespace
             const char  *format,	// I - Printf-style format string
	     ...)			// I - Additional arguments as needed
{
  va_list	ap;			// Pointer to arguments
  char		buffer[16384];		// Format buffer
  char		*s;			// Temporary string


  // Range check input...
  if (node && node->type == MXML_TYPE_ELEMENT && node->child && node->child->type == MXML_TYPE_TEXT)
    node = node->child;

  if (!node || node->type != MXML_TYPE_TEXT)
    return (false);
  else if (!format)
    return (false);

  // Free any old string value and set the new value...
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  if ((s = _mxml_strcopy(buffer)) == NULL)
    return (false);

  _mxml_strfree(node->value.text.string);

  node->value.text.whitespace = whitespace;
  node->value.text.string     = s;

  return (true);
}


//
// 'mxmlSetUserData()' - Set the user data pointer for a node.
//

bool					// O - `true` on success, `false` on failure
mxmlSetUserData(mxml_node_t *node,	// I - Node to set
                void        *data)	// I - User data pointer
{
  // Range check input...
  if (!node)
    return (false);

  // Set the user data pointer and return...
  node->user_data = data;
  return (true);
}
