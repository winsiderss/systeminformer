/*#define DEBUG 1*/
/*
 * "$Id: mxmldoc.c 451 2014-01-04 21:50:06Z msweet $"
 *
 * Documentation generator using Mini-XML, a small XML-like file parsing
 * library.
 *
 * Copyright 2003-2014 by Michael R Sweet.
 *
 * These coded instructions, statements, and computer programs are the
 * property of Michael R Sweet and are protected by Federal copyright
 * law.  Distribution and use rights are outlined in the file "COPYING"
 * which should have been included with this file.  If this file is
 * missing or damaged, see the license at:
 *
 *     http://www.minixml.org/
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"
#include <time.h>
#include <sys/stat.h>
#ifndef WIN32
#  include <dirent.h>
#  include <unistd.h>
#endif /* !WIN32 */
#ifdef __APPLE__
#  include <spawn.h>
#  include <sys/wait.h>
extern char **environ;
#endif /* __APPLE__ */


/*
 * This program scans source and header files and produces public API
 * documentation for code that conforms to the CUPS Configuration
 * Management Plan (CMP) coding standards.  Please see the following web
 * page for details:
 *
 *     http://www.cups.org/cmp.html
 *
 * Using Mini-XML, this program creates and maintains an XML representation
 * of the public API code documentation which can then be converted to HTML
 * as desired.  The following is a poor-man's schema:
 *
 * <?xml version="1.0"?>
 * <mxmldoc xmlns="http://www.easysw.com"
 *  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *  xsi:schemaLocation="http://www.minixml.org/mxmldoc.xsd">
 *
 *   <namespace name="">                        [optional...]
 *     <constant name="">
 *       <description>descriptive text</description>
 *     </constant>
 *
 *     <enumeration name="">
 *       <description>descriptive text</description>
 *       <constant name="">...</constant>
 *     </enumeration>
 *
 *     <typedef name="">
 *       <description>descriptive text</description>
 *       <type>type string</type>
 *     </typedef>
 *
 *     <function name="" scope="">
 *       <description>descriptive text</description>
 *       <argument name="" direction="I|O|IO" default="">
 *         <description>descriptive text</description>
 *         <type>type string</type>
 *       </argument>
 *       <returnvalue>
 *         <description>descriptive text</description>
 *         <type>type string</type>
 *       </returnvalue>
 *       <seealso>function names separated by spaces</seealso>
 *     </function>
 *
 *     <variable name="" scope="">
 *       <description>descriptive text</description>
 *       <type>type string</type>
 *     </variable>
 *
 *     <struct name="">
 *       <description>descriptive text</description>
 *       <variable name="">...</variable>
 *       <function name="">...</function>
 *     </struct>
 *
 *     <union name="">
 *       <description>descriptive text</description>
 *       <variable name="">...</variable>
 *     </union>
 *
 *     <class name="" parent="">
 *       <description>descriptive text</description>
 *       <class name="">...</class>
 *       <enumeration name="">...</enumeration>
 *       <function name="">...</function>
 *       <struct name="">...</struct>
 *       <variable name="">...</variable>
 *     </class>
 *   </namespace>
 * </mxmldoc>
 */


/*
 * Basic states for file parser...
 */

#define STATE_NONE		0	/* No state - whitespace, etc. */
#define STATE_PREPROCESSOR	1	/* Preprocessor directive */
#define STATE_C_COMMENT		2	/* Inside a C comment */
#define STATE_CXX_COMMENT	3	/* Inside a C++ comment */
#define STATE_STRING		4	/* Inside a string constant */
#define STATE_CHARACTER		5	/* Inside a character constant */
#define STATE_IDENTIFIER	6	/* Inside a keyword/identifier */


/*
 * Output modes...
 */

#define OUTPUT_NONE		0	/* No output */
#define OUTPUT_HTML		1	/* Output HTML */
#define OUTPUT_XML		2	/* Output XML */
#define OUTPUT_MAN		3	/* Output nroff/man */
#define OUTPUT_TOKENS		4	/* Output docset Tokens.xml file */


/*
 * Local functions...
 */

static mxml_node_t	*add_variable(mxml_node_t *parent, const char *name,
			              mxml_node_t *type);
static mxml_node_t	*find_public(mxml_node_t *node, mxml_node_t *top,
			             const char *name);
static char		*get_comment_info(mxml_node_t *description);
static char		*get_text(mxml_node_t *node, char *buffer, int buflen);
static mxml_type_t	load_cb(mxml_node_t *node);
static mxml_node_t	*new_documentation(mxml_node_t **mxmldoc);
static int		remove_directory(const char *path);
static void		safe_strcpy(char *dst, const char *src);
static int		scan_file(const char *filename, FILE *fp,
			          mxml_node_t *doc);
static void		sort_node(mxml_node_t *tree, mxml_node_t *func);
static void		update_comment(mxml_node_t *parent,
			               mxml_node_t *comment);
static void		usage(const char *option);
static void		write_description(FILE *out, mxml_node_t *description,
			                  const char *element, int summary);
static void		write_element(FILE *out, mxml_node_t *doc,
			              mxml_node_t *element, int mode);
static void		write_file(FILE *out, const char *file);
static void		write_function(FILE *out, mxml_node_t *doc,
			               mxml_node_t *function, int level);
static void		write_html(const char *section, const char *title,
			           const char *footerfile,
			           const char *headerfile,
				   const char *introfile, const char *cssfile,
				   const char *framefile,
				   const char *docset, const char *docversion,
				   const char *feedname, const char *feedurl,
				   mxml_node_t *doc);
static void		write_html_head(FILE *out, const char *section,
			                const char *title, const char *cssfile);
static void		write_man(const char *man_name, const char *section,
			          const char *title, const char *headerfile,
				  const char *footerfile, const char *introfile,
				  mxml_node_t *doc);
static void		write_scu(FILE *out, mxml_node_t *doc,
			          mxml_node_t *scut);
static void		write_string(FILE *out, const char *s, int mode);
static void		write_toc(FILE *out, mxml_node_t *doc,
			          const char *introfile, const char *target,
				  int xml);
static void		write_tokens(FILE *out, mxml_node_t *doc,
			             const char *path);
static const char	*ws_cb(mxml_node_t *node, int where);


/*
 * 'main()' - Main entry for test program.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line args */
     char *argv[])			/* I - Command-line args */
{
  int		i;			/* Looping var */
  int		len;			/* Length of argument */
  FILE		*fp;			/* File to read */
  mxml_node_t	*doc;			/* XML documentation tree */
  mxml_node_t	*mxmldoc;		/* mxmldoc node */
  const char	*cssfile,		/* CSS stylesheet file */
		*docset,		/* Documentation set directory */
		*docversion,		/* Documentation set version */
		*feedname,		/* Feed name for documentation set */
		*feedurl,		/* Feed URL for documentation set */
		*footerfile,		/* Footer file */
		*framefile,		/* Framed HTML basename */
		*headerfile,		/* Header file */
		*introfile,		/* Introduction file */
		*name,			/* Name of manpage */
		*path,			/* Path to help file for tokens */
		*section,		/* Section/keywords of documentation */
		*title,			/* Title of documentation */
		*xmlfile;		/* XML file */
  int		mode,			/* Output mode */
		update;			/* Updated XML file */


 /*
  * Check arguments...
  */

  cssfile     = NULL;
  doc         = NULL;
  docset      = NULL;
  docversion  = NULL;
  feedname    = NULL;
  feedurl     = NULL;
  footerfile  = NULL;
  framefile   = NULL;
  headerfile  = NULL;
  introfile   = NULL;
  mode        = OUTPUT_HTML;
  mxmldoc     = NULL;
  name        = NULL;
  path        = NULL;
  section     = NULL;
  title       = NULL;
  update      = 0;
  xmlfile     = NULL;

  for (i = 1; i < argc; i ++)
    if (!strcmp(argv[i], "--help"))
    {
     /*
      * Show help...
      */

      usage(NULL);
    }
    else if (!strcmp(argv[i], "--version"))
    {
     /*
      * Show version...
      */

      puts(MXML_VERSION + 10);
      return (0);
    }
    else if (!strcmp(argv[i], "--css") && !cssfile)
    {
     /*
      * Set CSS stylesheet file...
      */

      i ++;
      if (i < argc)
        cssfile = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--docset") && !docset)
    {
     /*
      * Set documentation set directory...
      */

      i ++;
      if (i < argc)
        docset = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--docversion") && !docversion)
    {
     /*
      * Set documentation set directory...
      */

      i ++;
      if (i < argc)
        docversion = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--footer") && !footerfile)
    {
     /*
      * Set footer file...
      */

      i ++;
      if (i < argc)
        footerfile = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--feedname") && !feedname)
    {
     /*
      * Set documentation set feed name...
      */

      i ++;
      if (i < argc)
        feedname = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--feedurl") && !feedurl)
    {
     /*
      * Set documentation set feed name...
      */

      i ++;
      if (i < argc)
        feedurl = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--framed") && !framefile)
    {
     /*
      * Set base filename for framed HTML output...
      */

      i ++;
      if (i < argc)
        framefile = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--header") && !headerfile)
    {
     /*
      * Set header file...
      */

      i ++;
      if (i < argc)
        headerfile = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--intro") && !introfile)
    {
     /*
      * Set intro file...
      */

      i ++;
      if (i < argc)
        introfile = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--man") && !name)
    {
     /*
      * Output manpage...
      */

      i ++;
      if (i < argc)
      {
        mode = OUTPUT_MAN;
        name = argv[i];
      }
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--no-output"))
      mode = OUTPUT_NONE;
    else if (!strcmp(argv[i], "--section") && !section)
    {
     /*
      * Set section/keywords...
      */

      i ++;
      if (i < argc)
        section = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--title") && !title)
    {
     /*
      * Set title...
      */

      i ++;
      if (i < argc)
        title = argv[i];
      else
        usage(NULL);
    }
    else if (!strcmp(argv[i], "--tokens"))
    {
     /*
      * Output Tokens.xml file...
      */

      mode = OUTPUT_TOKENS;

      i ++;
      if (i < argc)
        path = argv[i];
      else
        usage(NULL);
    }
    else if (argv[i][0] == '-')
    {
     /*
      * Unknown/bad option...
      */

      usage(argv[i]);
    }
    else
    {
     /*
      * Process XML or source file...
      */

      len = (int)strlen(argv[i]);
      if (len > 4 && !strcmp(argv[i] + len - 4, ".xml"))
      {
       /*
        * Set XML file...
	*/

        if (xmlfile)
	  usage(NULL);

        xmlfile = argv[i];

        if (!doc)
	{
	  if ((fp = fopen(argv[i], "r")) != NULL)
	  {
	   /*
	    * Read the existing XML file...
	    */

	    doc = mxmlLoadFile(NULL, fp, load_cb);

	    fclose(fp);

	    if (!doc)
	    {
	      mxmldoc = NULL;

	      fprintf(stderr,
	              "mxmldoc: Unable to read the XML documentation file "
		      "\"%s\"!\n", argv[i]);
	    }
	    else if ((mxmldoc = mxmlFindElement(doc, doc, "mxmldoc", NULL,
                                        	NULL, MXML_DESCEND)) == NULL)
	    {
	      fprintf(stderr,
	              "mxmldoc: XML documentation file \"%s\" is missing "
		      "<mxmldoc> node!!\n", argv[i]);

	      mxmlDelete(doc);
	      doc = NULL;
	    }
	  }
	  else
	  {
	    doc     = NULL;
	    mxmldoc = NULL;
	  }

	  if (!doc)
	    doc = new_documentation(&mxmldoc);
        }
      }
      else
      {
       /*
        * Load source file...
	*/

        update = 1;

	if (!doc)
	  doc = new_documentation(&mxmldoc);

	if ((fp = fopen(argv[i], "r")) == NULL)
	{
	  fprintf(stderr, "mxmldoc: Unable to open source file \"%s\": %s\n",
	          argv[i], strerror(errno));
	  mxmlDelete(doc);
	  return (1);
	}
	else if (scan_file(argv[i], fp, mxmldoc))
	{
	  fclose(fp);
	  mxmlDelete(doc);
	  return (1);
	}
	else
	  fclose(fp);
      }
    }

  if (update && xmlfile)
  {
   /*
    * Save the updated XML documentation file...
    */

    if ((fp = fopen(xmlfile, "w")) != NULL)
    {
     /*
      * Write over the existing XML file...
      */

      mxmlSetWrapMargin(0);

      if (mxmlSaveFile(doc, fp, ws_cb))
      {
	fprintf(stderr,
	        "mxmldoc: Unable to write the XML documentation file \"%s\": "
		"%s!\n", xmlfile, strerror(errno));
	fclose(fp);
	mxmlDelete(doc);
	return (1);
      }

      fclose(fp);
    }
    else
    {
      fprintf(stderr,
              "mxmldoc: Unable to create the XML documentation file \"%s\": "
	      "%s!\n", xmlfile, strerror(errno));
      mxmlDelete(doc);
      return (1);
    }
  }

  switch (mode)
  {
    case OUTPUT_HTML :
       /*
        * Write HTML documentation...
        */

        write_html(section, title ? title : "Documentation", footerfile,
	           headerfile, introfile, cssfile, framefile, docset,
		   docversion, feedname, feedurl, mxmldoc);
        break;

    case OUTPUT_MAN :
       /*
        * Write manpage documentation...
        */

        write_man(name, section, title, footerfile, headerfile, introfile,
	          mxmldoc);
        break;

    case OUTPUT_TOKENS :
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	      "<Tokens version=\"1.0\">\n", stdout);

	write_tokens(stdout, mxmldoc, path);

	fputs("</Tokens>\n", stdout);
        break;
  }

 /*
  * Delete the tree and return...
  */

  mxmlDelete(doc);

  return (0);
}


/*
 * 'add_variable()' - Add a variable or argument.
 */

static mxml_node_t *			/* O - New variable/argument */
add_variable(mxml_node_t *parent,	/* I - Parent node */
             const char  *name,		/* I - "argument" or "variable" */
             mxml_node_t *type)		/* I - Type nodes */
{
  mxml_node_t	*variable,		/* New variable */
		*node,			/* Current node */
		*next;			/* Next node */
  char		buffer[16384],		/* String buffer */
		*bufptr;		/* Pointer into buffer */


#ifdef DEBUG
  fprintf(stderr, "add_variable(parent=%p, name=\"%s\", type=%p)\n",
          parent, name, type);
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!type || !type->child)
    return (NULL);

 /*
  * Create the variable/argument node...
  */

  variable = mxmlNewElement(parent, name);

 /*
  * Check for a default value...
  */

  for (node = type->child; node; node = node->next)
    if (!strcmp(node->value.text.string, "="))
      break;

  if (node)
  {
   /*
    * Default value found, copy it and add as a "default" attribute...
    */

    for (bufptr = buffer; node; bufptr += strlen(bufptr))
    {
      if (node->value.text.whitespace && bufptr > buffer)
	*bufptr++ = ' ';

      strcpy(bufptr, node->value.text.string);

      next = node->next;
      mxmlDelete(node);
      node = next;
    }

    mxmlElementSetAttr(variable, "default", buffer);
  }

 /*
  * Extract the argument/variable name...
  */

  if (type->last_child->value.text.string[0] == ')')
  {
   /*
    * Handle "type (*name)(args)"...
    */

    for (node = type->child; node; node = node->next)
      if (node->value.text.string[0] == '(')
	break;

    for (bufptr = buffer; node; bufptr += strlen(bufptr))
    {
      if (node->value.text.whitespace && bufptr > buffer)
	*bufptr++ = ' ';

      strcpy(bufptr, node->value.text.string);

      next = node->next;
      mxmlDelete(node);
      node = next;
    }
  }
  else
  {
   /*
    * Handle "type name"...
    */

    strcpy(buffer, type->last_child->value.text.string);
    mxmlDelete(type->last_child);
  }

 /*
  * Set the name...
  */

  mxmlElementSetAttr(variable, "name", buffer);

 /*
  * Add the remaining type information to the variable node...
  */

  mxmlAdd(variable, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, type);

 /*
  * Add new new variable node...
  */

  return (variable);
}


/*
 * 'find_public()' - Find a public function, type, etc.
 */

static mxml_node_t *			/* I - Found node or NULL */
find_public(mxml_node_t *node,		/* I - Current node */
            mxml_node_t *top,		/* I - Top node */
            const char  *name)		/* I - Name of element */
{
  mxml_node_t	*description,		/* Description node */
		*comment;		/* Comment node */


  for (node = mxmlFindElement(node, top, name, NULL, NULL,
                              node == top ? MXML_DESCEND_FIRST :
			                    MXML_NO_DESCEND);
       node;
       node = mxmlFindElement(node, top, name, NULL, NULL, MXML_NO_DESCEND))
  {
   /*
    * Get the description for this node...
    */

    description = mxmlFindElement(node, node, "description", NULL, NULL,
                                  MXML_DESCEND_FIRST);

   /*
    * A missing or empty description signals a private node...
    */

    if (!description)
      continue;

   /*
    * Look for @private@ in the comment text...
    */

    for (comment = description->child; comment; comment = comment->next)
      if ((comment->type == MXML_TEXT &&
           strstr(comment->value.text.string, "@private@")) ||
          (comment->type == MXML_OPAQUE &&
           strstr(comment->value.opaque, "@private@")))
        break;

    if (!comment)
    {
     /*
      * No @private@, so return this node...
      */

      return (node);
    }
  }

 /*
  * If we get here, there are no (more) public nodes...
  */

  return (NULL);
}


/*
 * 'get_comment_info()' - Get info from comment.
 */

static char *				/* O - Info from comment */
get_comment_info(
    mxml_node_t *description)		/* I - Description node */
{
  char		text[10240],		/* Description text */
		since[255],		/* @since value */
		*ptr;			/* Pointer into text */
  static char	info[1024];		/* Info string */


  if (!description)
    return ("");

  get_text(description, text, sizeof(text));

  for (ptr = strchr(text, '@'); ptr; ptr = strchr(ptr + 1, '@'))
  {
    if (!strncmp(ptr, "@deprecated@", 12))
      return ("<span class=\"info\">&nbsp;DEPRECATED&nbsp;</span>");
    else if (!strncmp(ptr, "@since ", 7))
    {
      strncpy(since, ptr + 7, sizeof(since) - 1);
      since[sizeof(since) - 1] = '\0';

      if ((ptr = strchr(since, '@')) != NULL)
        *ptr = '\0';

      snprintf(info, sizeof(info), "<span class=\"info\">&nbsp;%s&nbsp;</span>", since);
      return (info);
    }
  }

  return ("");
}


/*
 * 'get_text()' - Get the text for a node.
 */

static char *				/* O - Text in node */
get_text(mxml_node_t *node,		/* I - Node to get */
         char        *buffer,		/* I - Buffer */
	 int         buflen)		/* I - Size of buffer */
{
  char		*ptr,			/* Pointer into buffer */
		*end;			/* End of buffer */
  int		len;			/* Length of node */
  mxml_node_t	*current;		/* Current node */


  ptr = buffer;
  end = buffer + buflen - 1;

  for (current = node->child; current && ptr < end; current = current->next)
  {
    if (current->type == MXML_TEXT)
    {
      if (current->value.text.whitespace)
        *ptr++ = ' ';

      len = (int)strlen(current->value.text.string);
      if (len > (int)(end - ptr))
        len = (int)(end - ptr);

      memcpy(ptr, current->value.text.string, len);
      ptr += len;
    }
    else if (current->type == MXML_OPAQUE)
    {
      len = (int)strlen(current->value.opaque);
      if (len > (int)(end - ptr))
        len = (int)(end - ptr);

      memcpy(ptr, current->value.opaque, len);
      ptr += len;
    }
  }

  *ptr = '\0';

  return (buffer);
}


/*
 * 'load_cb()' - Set the type of child nodes.
 */

static mxml_type_t			/* O - Node type */
load_cb(mxml_node_t *node)		/* I - Node */
{
  if (!strcmp(node->value.element.name, "description"))
    return (MXML_OPAQUE);
  else
    return (MXML_TEXT);
}


/*
 * 'new_documentation()' - Create a new documentation tree.
 */

static mxml_node_t *			/* O - New documentation */
new_documentation(mxml_node_t **mxmldoc)/* O - mxmldoc node */
{
  mxml_node_t	*doc;			/* New documentation */


 /*
  * Create an empty XML documentation file...
  */

  doc = mxmlNewXML(NULL);

  *mxmldoc = mxmlNewElement(doc, "mxmldoc");

  mxmlElementSetAttr(*mxmldoc, "xmlns", "http://www.easysw.com");
  mxmlElementSetAttr(*mxmldoc, "xmlns:xsi",
                     "http://www.w3.org/2001/XMLSchema-instance");
  mxmlElementSetAttr(*mxmldoc, "xsi:schemaLocation",
                     "http://www.minixml.org/mxmldoc.xsd");

  return (doc);
}


/*
 * 'remove_directory()' - Remove a directory.
 */

static int				/* O - 1 on success, 0 on failure */
remove_directory(const char *path)	/* I - Directory to remove */
{
#ifdef WIN32
  /* TODO: Add Windows directory removal code */

#else
  DIR		*dir;			/* Directory */
  struct dirent	*dent;			/* Current directory entry */
  char		filename[1024];		/* Current filename */
  struct stat	fileinfo;		/* File information */


  if ((dir = opendir(path)) == NULL)
  {
    fprintf(stderr, "mxmldoc: Unable to open directory \"%s\": %s\n", path,
            strerror(errno));
    return (0);
  }

  while ((dent = readdir(dir)) != NULL)
  {
   /*
    * Skip "." and ".."...
    */

    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
      continue;

   /*
    * See if we have a file or directory...
    */

    snprintf(filename, sizeof(filename), "%s/%s", path, dent->d_name);

    if (stat(filename, &fileinfo))
    {
      fprintf(stderr, "mxmldoc: Unable to stat \"%s\": %s\n", filename,
	      strerror(errno));
      closedir(dir);
      return (0);
    }

    if (S_ISDIR(fileinfo.st_mode))
    {
      if (!remove_directory(filename))
      {
        closedir(dir);
	return (0);
      }
    }
    else if (unlink(filename))
    {
      fprintf(stderr, "mxmldoc: Unable to remove \"%s\": %s\n", filename,
	      strerror(errno));
      closedir(dir);
      return (0);
    }
  }

  closedir(dir);

  if (rmdir(path))
  {
    fprintf(stderr, "mxmldoc: Unable to remove directory \"%s\": %s\n", path,
            strerror(errno));
    return (0);
  }
#endif /* WIN32 */

  return (1);
}


/*
 * 'safe_strcpy()' - Copy a string allowing for overlapping strings.
 */

static void
safe_strcpy(char       *dst,		/* I - Destination string */
            const char *src)		/* I - Source string */
{
  while (*src)
    *dst++ = *src++;

  *dst = '\0';
}


/*
 * 'scan_file()' - Scan a source file.
 */

static int				/* O - 0 on success, -1 on error */
scan_file(const char  *filename,	/* I - Filename */
          FILE        *fp,		/* I - File to scan */
          mxml_node_t *tree)		/* I - Function tree */
{
  int		state,			/* Current parser state */
		braces,			/* Number of braces active */
		parens;			/* Number of active parenthesis */
  int		ch;			/* Current character */
  char		buffer[65536],		/* String buffer */
		*bufptr;		/* Pointer into buffer */
  const char	*scope;			/* Current variable/function scope */
  mxml_node_t	*comment,		/* <comment> node */
		*constant,		/* <constant> node */
		*enumeration,		/* <enumeration> node */
		*function,		/* <function> node */
		*fstructclass,		/* function struct/class node */
		*structclass,		/* <struct> or <class> node */
		*typedefnode,		/* <typedef> node */
		*variable,		/* <variable> or <argument> node */
		*returnvalue,		/* <returnvalue> node */
		*type,			/* <type> node */
		*description,		/* <description> node */
		*node,			/* Current node */
		*next;			/* Next node */
#if DEBUG > 1
  mxml_node_t	*temp;			/* Temporary node */
  int		oldstate,		/* Previous state */
		oldch;			/* Old character */
  static const char *states[] =		/* State strings */
		{
		  "STATE_NONE",
		  "STATE_PREPROCESSOR",
		  "STATE_C_COMMENT",
		  "STATE_CXX_COMMENT",
		  "STATE_STRING",
		  "STATE_CHARACTER",
		  "STATE_IDENTIFIER"
		};
#endif /* DEBUG > 1 */


#ifdef DEBUG
  fprintf(stderr, "scan_file(filename=\"%s\", fp=%p, tree=%p)\n", filename,
          fp, tree);
#endif /* DEBUG */

 /*
  * Initialize the finite state machine...
  */

  state        = STATE_NONE;
  braces       = 0;
  parens       = 0;
  bufptr       = buffer;

  comment      = mxmlNewElement(MXML_NO_PARENT, "temp");
  constant     = NULL;
  enumeration  = NULL;
  function     = NULL;
  variable     = NULL;
  returnvalue  = NULL;
  type         = NULL;
  description  = NULL;
  typedefnode  = NULL;
  structclass  = NULL;
  fstructclass = NULL;

  if (!strcmp(tree->value.element.name, "class"))
    scope = "private";
  else
    scope = NULL;

 /*
  * Read until end-of-file...
  */

  while ((ch = getc(fp)) != EOF)
  {
#if DEBUG > 1
    oldstate = state;
    oldch    = ch;
#endif /* DEBUG > 1 */

    switch (state)
    {
      case STATE_NONE :			/* No state - whitespace, etc. */
          switch (ch)
	  {
	    case '/' :			/* Possible C/C++ comment */
	        ch     = getc(fp);
		bufptr = buffer;

		if (ch == '*')
		  state = STATE_C_COMMENT;
		else if (ch == '/')
		  state = STATE_CXX_COMMENT;
		else
		{
		  ungetc(ch, fp);

		  if (type)
		  {
#ifdef DEBUG
                    fputs("Identifier: <<<< / >>>\n", stderr);
#endif /* DEBUG */
                    ch = type->last_child->value.text.string[0];
		    mxmlNewText(type, isalnum(ch) || ch == '_', "/");
		  }
		}
		break;

	    case '#' :			/* Preprocessor */
#ifdef DEBUG
	        fputs("    #preprocessor...\n", stderr);
#endif /* DEBUG */
	        state = STATE_PREPROCESSOR;
		break;

            case '\'' :			/* Character constant */
	        state = STATE_CHARACTER;
		bufptr = buffer;
		*bufptr++ = ch;
		break;

            case '\"' :			/* String constant */
	        state = STATE_STRING;
		bufptr = buffer;
		*bufptr++ = ch;
		break;

            case '{' :
#ifdef DEBUG
	        fprintf(stderr, "    open brace, function=%p, type=%p...\n",
		        function, type);
                if (type)
                  fprintf(stderr, "    type->child=\"%s\"...\n",
		          type->child->value.text.string);
#endif /* DEBUG */

	        if (function)
		{
		  if (fstructclass)
		  {
		    sort_node(fstructclass, function);
		    fstructclass = NULL;
		  }
		  else
		    sort_node(tree, function);

		  function = NULL;
		}
		else if (type && type->child &&
		         ((!strcmp(type->child->value.text.string, "typedef") &&
			   type->child->next &&
			   (!strcmp(type->child->next->value.text.string, "struct") ||
			    !strcmp(type->child->next->value.text.string, "union") ||
			    !strcmp(type->child->next->value.text.string, "class"))) ||
			  !strcmp(type->child->value.text.string, "union") ||
			  !strcmp(type->child->value.text.string, "struct") ||
			  !strcmp(type->child->value.text.string, "class")))
		{
		 /*
		  * Start of a class or structure...
		  */

		  if (!strcmp(type->child->value.text.string, "typedef"))
		  {
#ifdef DEBUG
                    fputs("    starting typedef...\n", stderr);
#endif /* DEBUG */

		    typedefnode = mxmlNewElement(MXML_NO_PARENT, "typedef");
		    mxmlDelete(type->child);
		  }
		  else
		    typedefnode = NULL;

		  structclass = mxmlNewElement(MXML_NO_PARENT,
		                               type->child->value.text.string);

#ifdef DEBUG
                  fprintf(stderr, "%c%s: <<<< %s >>>\n",
		          toupper(type->child->value.text.string[0]),
			  type->child->value.text.string + 1,
			  type->child->next ?
			      type->child->next->value.text.string : "(noname)");

                  fputs("    type =", stderr);
                  for (node = type->child; node; node = node->next)
		    fprintf(stderr, " \"%s\"", node->value.text.string);
		  putc('\n', stderr);

                  fprintf(stderr, "    scope = %s\n", scope ? scope : "(null)");
#endif /* DEBUG */

                  if (type->child->next)
		  {
		    mxmlElementSetAttr(structclass, "name",
		                       type->child->next->value.text.string);
		    sort_node(tree, structclass);
		  }

                  if (typedefnode && type->child)
		    type->child->value.text.whitespace = 0;
                  else if (structclass && type->child &&
		           type->child->next && type->child->next->next)
		  {
		    for (bufptr = buffer, node = type->child->next->next;
		         node;
			 bufptr += strlen(bufptr))
		    {
		      if (node->value.text.whitespace && bufptr > buffer)
			*bufptr++ = ' ';

		      strcpy(bufptr, node->value.text.string);

		      next = node->next;
		      mxmlDelete(node);
		      node = next;
		    }

		    mxmlElementSetAttr(structclass, "parent", buffer);

		    mxmlDelete(type);
		    type = NULL;
		  }
		  else
		  {
		    mxmlDelete(type);
		    type = NULL;
		  }

		  if (typedefnode && comment->last_child)
		  {
		   /*
		    * Copy comment for typedef as well as class/struct/union...
		    */

		    mxmlNewText(comment, 0,
		                comment->last_child->value.text.string);
		    description = mxmlNewElement(typedefnode, "description");
#ifdef DEBUG
		    fprintf(stderr,
		            "    duplicating comment %p/%p for typedef...\n",
			    comment->last_child, comment->child);
#endif /* DEBUG */
		    update_comment(typedefnode, comment->last_child);
		    mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		            comment->last_child);
		  }

		  description = mxmlNewElement(structclass, "description");
#ifdef DEBUG
		  fprintf(stderr, "    adding comment %p/%p to %s...\n",
		          comment->last_child, comment->child,
			  structclass->value.element.name);
#endif /* DEBUG */
		  update_comment(structclass, comment->last_child);
		  mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		          comment->last_child);

                  if (scan_file(filename, fp, structclass))
		  {
		    mxmlDelete(comment);
		    return (-1);
		  }

#ifdef DEBUG
                  fputs("    ended typedef...\n", stderr);
#endif /* DEBUG */
                  structclass = NULL;
                  break;
                }
		else if (type && type->child && type->child->next &&
		         (!strcmp(type->child->value.text.string, "enum") ||
			  (!strcmp(type->child->value.text.string, "typedef") &&
			   !strcmp(type->child->next->value.text.string, "enum"))))
                {
		 /*
		  * Enumeration type...
		  */

		  if (!strcmp(type->child->value.text.string, "typedef"))
		  {
#ifdef DEBUG
                    fputs("    starting typedef...\n", stderr);
#endif /* DEBUG */

		    typedefnode = mxmlNewElement(MXML_NO_PARENT, "typedef");
		    mxmlDelete(type->child);
		  }
		  else
		    typedefnode = NULL;

		  enumeration = mxmlNewElement(MXML_NO_PARENT, "enumeration");

#ifdef DEBUG
                  fprintf(stderr, "Enumeration: <<<< %s >>>\n",
			  type->child->next ?
			      type->child->next->value.text.string : "(noname)");
#endif /* DEBUG */

                  if (type->child->next)
		  {
		    mxmlElementSetAttr(enumeration, "name",
		                       type->child->next->value.text.string);
		    sort_node(tree, enumeration);
		  }

                  if (typedefnode && type->child)
		    type->child->value.text.whitespace = 0;
                  else
		  {
		    mxmlDelete(type);
		    type = NULL;
		  }

		  if (typedefnode && comment->last_child)
		  {
		   /*
		    * Copy comment for typedef as well as class/struct/union...
		    */

		    mxmlNewText(comment, 0,
		                comment->last_child->value.text.string);
		    description = mxmlNewElement(typedefnode, "description");
#ifdef DEBUG
		    fprintf(stderr,
		            "    duplicating comment %p/%p for typedef...\n",
			    comment->last_child, comment->child);
#endif /* DEBUG */
		    update_comment(typedefnode, comment->last_child);
		    mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		            comment->last_child);
		  }

		  description = mxmlNewElement(enumeration, "description");
#ifdef DEBUG
		  fprintf(stderr, "    adding comment %p/%p to enumeration...\n",
		          comment->last_child, comment->child);
#endif /* DEBUG */
		  update_comment(enumeration, comment->last_child);
		  mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		          comment->last_child);
		}
		else if (type && type->child &&
		         !strcmp(type->child->value.text.string, "extern"))
                {
                  if (scan_file(filename, fp, tree))
		  {
		    mxmlDelete(comment);
		    return (-1);
		  }
                }
		else if (type)
		{
		  mxmlDelete(type);
		  type = NULL;
		}

	        braces ++;
		function = NULL;
		variable = NULL;
		break;

            case '}' :
#ifdef DEBUG
	        fputs("    close brace...\n", stderr);
#endif /* DEBUG */

                if (structclass)
		  scope = NULL;

                if (!typedefnode)
		  enumeration = NULL;

		constant    = NULL;
		structclass = NULL;

	        if (braces > 0)
		  braces --;
		else
		{
		  mxmlDelete(comment);
		  return (0);
		}
		break;

            case '(' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< ( >>>\n", stderr);
#endif /* DEBUG */
		  mxmlNewText(type, 0, "(");
		}

	        parens ++;
		break;

            case ')' :
		if (type && parens)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< ) >>>\n", stderr);
#endif /* DEBUG */
		  mxmlNewText(type, 0, ")");
		}

                if (function && type && !parens)
		{
		 /*
		  * Check for "void" argument...
		  */

		  if (type->child && type->child->next)
		    variable = add_variable(function, "argument", type);
		  else
		    mxmlDelete(type);

		  type = NULL;
		}

	        if (parens > 0)
		  parens --;
		break;

	    case ';' :
#ifdef DEBUG
                fputs("Identifier: <<<< ; >>>\n", stderr);
		fprintf(stderr, "    enumeration=%p, function=%p, type=%p, type->child=%p, typedefnode=%p\n",
		        enumeration, function, type, type ? type->child : NULL, typedefnode);
#endif /* DEBUG */

		if (function)
		{
		  if (!strcmp(tree->value.element.name, "class"))
		  {
#ifdef DEBUG
		    fputs("    ADDING FUNCTION TO CLASS\n", stderr);
#endif /* DEBUG */
		    sort_node(tree, function);
		  }
		  else
		    mxmlDelete(function);

		  function = NULL;
		  variable = NULL;
		}

		if (type)
		{
		 /*
		  * See if we have a typedef...
		  */

		  if (type->child &&
		      !strcmp(type->child->value.text.string, "typedef"))
		  {
		   /*
		    * Yes, add it!
		    */

		    typedefnode = mxmlNewElement(MXML_NO_PARENT, "typedef");

		    for (node = type->child->next; node; node = node->next)
		      if (!strcmp(node->value.text.string, "("))
			break;

                    if (node)
		    {
		      for (node = node->next; node; node = node->next)
			if (strcmp(node->value.text.string, "*"))
			  break;
                    }

                    if (!node)
		      node = type->last_child;

#ifdef DEBUG
		    fprintf(stderr, "    ADDING TYPEDEF FOR %p(%s)...\n",
		            node, node->value.text.string);
#endif /* DEBUG */

		    mxmlElementSetAttr(typedefnode, "name",
				       node->value.text.string);
		    sort_node(tree, typedefnode);

                    if (type->child != node)
		      mxmlDelete(type->child);

		    mxmlDelete(node);

		    if (type->child)
		      type->child->value.text.whitespace = 0;

		    mxmlAdd(typedefnode, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
			    type);
		    type = NULL;
		    break;
		  }
		  else if (typedefnode && enumeration)
		  {
		   /*
		    * Add enum typedef...
		    */

                    node = type->child;

#ifdef DEBUG
		    fprintf(stderr, "    ADDING TYPEDEF FOR %p(%s)...\n",
		            node, node->value.text.string);
#endif /* DEBUG */

		    mxmlElementSetAttr(typedefnode, "name",
				       node->value.text.string);
		    sort_node(tree, typedefnode);
		    mxmlDelete(type);

		    type = mxmlNewElement(typedefnode, "type");
                    mxmlNewText(type, 0, "enum");
		    mxmlNewText(type, 1,
		                mxmlElementGetAttr(enumeration, "name"));
		    enumeration = NULL;
		    type = NULL;
		    break;
		  }

		  mxmlDelete(type);
		  type = NULL;
		}
		break;

	    case ':' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< : >>>\n", stderr);
#endif /* DEBUG */
		  mxmlNewText(type, 1, ":");
		}
		break;

	    case '*' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< * >>>\n", stderr);
#endif /* DEBUG */
                  ch = type->last_child->value.text.string[0];
		  mxmlNewText(type, isalnum(ch) || ch == '_', "*");
		}
		break;

	    case ',' :
		if (type && !enumeration)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< , >>>\n", stderr);
#endif /* DEBUG */
		  mxmlNewText(type, 0, ",");
		}
		break;

	    case '&' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< & >>>\n", stderr);
#endif /* DEBUG */
		  mxmlNewText(type, 1, "&");
		}
		break;

	    case '+' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< + >>>\n", stderr);
#endif /* DEBUG */
                  ch = type->last_child->value.text.string[0];
		  mxmlNewText(type, isalnum(ch) || ch == '_', "+");
		}
		break;

	    case '-' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< - >>>\n", stderr);
#endif /* DEBUG */
                  ch = type->last_child->value.text.string[0];
		  mxmlNewText(type, isalnum(ch) || ch == '_', "-");
		}
		break;

	    case '=' :
		if (type)
		{
#ifdef DEBUG
                  fputs("Identifier: <<<< = >>>\n", stderr);
#endif /* DEBUG */
                  ch = type->last_child->value.text.string[0];
		  mxmlNewText(type, isalnum(ch) || ch == '_', "=");
		}
		break;

            default :			/* Other */
	        if (isalnum(ch) || ch == '_' || ch == '.' || ch == ':' || ch == '~')
		{
		  state     = STATE_IDENTIFIER;
		  bufptr    = buffer;
		  *bufptr++ = ch;
		}
		break;
          }
          break;

      case STATE_PREPROCESSOR :		/* Preprocessor directive */
          if (ch == '\n')
	    state = STATE_NONE;
	  else if (ch == '\\')
	    getc(fp);
          break;

      case STATE_C_COMMENT :		/* Inside a C comment */
          switch (ch)
	  {
	    case '\n' :
	        while ((ch = getc(fp)) != EOF)
		  if (ch == '*')
		  {
		    ch = getc(fp);

		    if (ch == '/')
		    {
		      *bufptr = '\0';

        	      if (comment->child != comment->last_child)
		      {
#ifdef DEBUG
			fprintf(stderr, "    removing comment %p(%20.20s), last comment %p(%20.20s)...\n",
				comment->child,
				comment->child ? comment->child->value.text.string : "",
				comment->last_child,
				comment->last_child ? comment->last_child->value.text.string : "");
#endif /* DEBUG */
			mxmlDelete(comment->child);
#ifdef DEBUG
			fprintf(stderr, "    new comment %p, last comment %p...\n",
				comment->child, comment->last_child);
#endif /* DEBUG */
		      }

#ifdef DEBUG
                      fprintf(stderr,
		              "    processing comment, variable=%p, "
		              "constant=%p, typedefnode=%p, tree=\"%s\"\n",
		              variable, constant, typedefnode,
			      tree->value.element.name);
#endif /* DEBUG */

		      if (variable)
		      {
		        if (strstr(buffer, "@private@"))
			{
			 /*
			  * Delete private variables...
			  */

			  mxmlDelete(variable);
			}
			else
			{
			  description = mxmlNewElement(variable, "description");
#ifdef DEBUG
			  fprintf(stderr,
			          "    adding comment %p/%p to variable...\n",
			          comment->last_child, comment->child);
#endif /* DEBUG */
			  mxmlNewText(comment, 0, buffer);
			  update_comment(variable,
					 mxmlNewText(description, 0, buffer));
                        }

			variable = NULL;
		      }
		      else if (constant)
		      {
		        if (strstr(buffer, "@private@"))
			{
			 /*
			  * Delete private constants...
			  */

			  mxmlDelete(constant);
			}
			else
			{
			  description = mxmlNewElement(constant, "description");
#ifdef DEBUG
			  fprintf(stderr,
			          "    adding comment %p/%p to constant...\n",
				  comment->last_child, comment->child);
#endif /* DEBUG */
			  mxmlNewText(comment, 0, buffer);
			  update_comment(constant,
					 mxmlNewText(description, 0, buffer));
			}

			constant = NULL;
		      }
		      else if (typedefnode)
		      {
		        if (strstr(buffer, "@private@"))
			{
			 /*
			  * Delete private typedefs...
			  */

			  mxmlDelete(typedefnode);

			  if (structclass)
			  {
			    mxmlDelete(structclass);
			    structclass = NULL;
			  }

			  if (enumeration)
			  {
			    mxmlDelete(enumeration);
			    enumeration = NULL;
			  }
			}
			else
			{
			  description = mxmlNewElement(typedefnode, "description");
#ifdef DEBUG
			  fprintf(stderr,
			          "    adding comment %p/%p to typedef %s...\n",
				  comment->last_child, comment->child,
				  mxmlElementGetAttr(typedefnode, "name"));
#endif /* DEBUG */
			  mxmlNewText(comment, 0, buffer);
			  update_comment(typedefnode,
					 mxmlNewText(description, 0, buffer));

			  if (structclass)
			  {
			    description = mxmlNewElement(structclass, "description");
			    update_comment(structclass,
					   mxmlNewText(description, 0, buffer));
			  }
			  else if (enumeration)
			  {
			    description = mxmlNewElement(enumeration, "description");
			    update_comment(enumeration,
					   mxmlNewText(description, 0, buffer));
			  }
			}

			typedefnode = NULL;
		      }
		      else if (strcmp(tree->value.element.name, "mxmldoc") &&
		               !mxmlFindElement(tree, tree, "description",
			                        NULL, NULL, MXML_DESCEND_FIRST))
                      {
        		description = mxmlNewElement(tree, "description");
#ifdef DEBUG
			fprintf(stderr, "    adding comment %p/%p to parent...\n",
			        comment->last_child, comment->child);
#endif /* DEBUG */
        		mxmlNewText(comment, 0, buffer);
			update_comment(tree,
			               mxmlNewText(description, 0, buffer));
		      }
		      else
		      {
#ifdef DEBUG
		        fprintf(stderr, "    before adding comment, child=%p, last_child=%p\n",
			        comment->child, comment->last_child);
#endif /* DEBUG */
        		mxmlNewText(comment, 0, buffer);
#ifdef DEBUG
		        fprintf(stderr, "    after adding comment, child=%p, last_child=%p\n",
			        comment->child, comment->last_child);
#endif /* DEBUG */
                      }
#ifdef DEBUG
		      fprintf(stderr, "C comment: <<<< %s >>>\n", buffer);
#endif /* DEBUG */

		      state = STATE_NONE;
		      break;
		    }
		    else
		      ungetc(ch, fp);
		  }
		  else if (ch == '\n' && bufptr > buffer &&
		           bufptr < (buffer + sizeof(buffer) - 1))
		    *bufptr++ = ch;
		  else if (!isspace(ch))
		    break;

		if (ch != EOF)
		  ungetc(ch, fp);

                if (bufptr > buffer && bufptr < (buffer + sizeof(buffer) - 1))
		  *bufptr++ = '\n';
		break;

	    case '/' :
	        if (ch == '/' && bufptr > buffer && bufptr[-1] == '*')
		{
		  while (bufptr > buffer &&
		         (bufptr[-1] == '*' || isspace(bufptr[-1] & 255)))
		    bufptr --;
		  *bufptr = '\0';

        	  if (comment->child != comment->last_child)
		  {
#ifdef DEBUG
		    fprintf(stderr, "    removing comment %p(%20.20s), last comment %p(%20.20s)...\n",
			    comment->child,
			    comment->child ? comment->child->value.text.string : "",
			    comment->last_child,
			    comment->last_child ? comment->last_child->value.text.string : "");
#endif /* DEBUG */
		    mxmlDelete(comment->child);
#ifdef DEBUG
		    fprintf(stderr, "    new comment %p, last comment %p...\n",
			    comment->child, comment->last_child);
#endif /* DEBUG */
		  }

#ifdef DEBUG
                  fprintf(stderr,
		          "    processing comment, variable=%p, "
		          "constant=%p, typedefnode=%p, tree=\"%s\"\n",
		          variable, constant, typedefnode,
			  tree->value.element.name);
#endif /* DEBUG */

		  if (variable)
		  {
		    if (strstr(buffer, "@private@"))
		    {
		     /*
		      * Delete private variables...
		      */

		      mxmlDelete(variable);
		    }
		    else
		    {
		      description = mxmlNewElement(variable, "description");
#ifdef DEBUG
		      fprintf(stderr, "    adding comment %p/%p to variable...\n",
		              comment->last_child, comment->child);
#endif /* DEBUG */
		      mxmlNewText(comment, 0, buffer);
		      update_comment(variable,
				     mxmlNewText(description, 0, buffer));
                    }

		    variable = NULL;
		  }
		  else if (constant)
		  {
		    if (strstr(buffer, "@private@"))
		    {
		     /*
		      * Delete private constants...
		      */

		      mxmlDelete(constant);
		    }
		    else
		    {
		      description = mxmlNewElement(constant, "description");
#ifdef DEBUG
		      fprintf(stderr, "    adding comment %p/%p to constant...\n",
		              comment->last_child, comment->child);
#endif /* DEBUG */
		      mxmlNewText(comment, 0, buffer);
		      update_comment(constant,
				     mxmlNewText(description, 0, buffer));
		    }

		    constant = NULL;
		  }
		  else if (typedefnode)
		  {
		    if (strstr(buffer, "@private@"))
		    {
		     /*
		      * Delete private typedefs...
		      */

		      mxmlDelete(typedefnode);

		      if (structclass)
		      {
			mxmlDelete(structclass);
			structclass = NULL;
		      }

		      if (enumeration)
		      {
			mxmlDelete(enumeration);
			enumeration = NULL;
		      }
		    }
		    else
		    {
		      description = mxmlNewElement(typedefnode, "description");
#ifdef DEBUG
		      fprintf(stderr,
		              "    adding comment %p/%p to typedef %s...\n",
			      comment->last_child, comment->child,
			      mxmlElementGetAttr(typedefnode, "name"));
#endif /* DEBUG */
		      mxmlNewText(comment, 0, buffer);
		      update_comment(typedefnode,
				     mxmlNewText(description, 0, buffer));

		      if (structclass)
		      {
			description = mxmlNewElement(structclass, "description");
			update_comment(structclass,
				       mxmlNewText(description, 0, buffer));
		      }
		      else if (enumeration)
		      {
			description = mxmlNewElement(enumeration, "description");
			update_comment(enumeration,
				       mxmlNewText(description, 0, buffer));
		      }
		    }

		    typedefnode = NULL;
		  }
		  else if (strcmp(tree->value.element.name, "mxmldoc") &&
		           !mxmlFindElement(tree, tree, "description",
			                    NULL, NULL, MXML_DESCEND_FIRST))
                  {
        	    description = mxmlNewElement(tree, "description");
#ifdef DEBUG
		    fprintf(stderr, "    adding comment %p/%p to parent...\n",
		            comment->last_child, comment->child);
#endif /* DEBUG */
		    mxmlNewText(comment, 0, buffer);
		    update_comment(tree,
			           mxmlNewText(description, 0, buffer));
		  }
		  else
        	    mxmlNewText(comment, 0, buffer);

#ifdef DEBUG
		  fprintf(stderr, "C comment: <<<< %s >>>\n", buffer);
#endif /* DEBUG */

		  state = STATE_NONE;
		  break;
		}

	    default :
	        if (ch == ' ' && bufptr == buffer)
		  break;

	        if (bufptr < (buffer + sizeof(buffer) - 1))
		  *bufptr++ = ch;
		break;
          }
          break;

      case STATE_CXX_COMMENT :		/* Inside a C++ comment */
          if (ch == '\n')
	  {
	    state = STATE_NONE;
	    *bufptr = '\0';

            if (comment->child != comment->last_child)
	    {
#ifdef DEBUG
	      fprintf(stderr, "    removing comment %p(%20.20s), last comment %p(%20.20s)...\n",
		      comment->child,
		      comment->child ? comment->child->value.text.string : "",
		      comment->last_child,
		      comment->last_child ? comment->last_child->value.text.string : "");
#endif /* DEBUG */
	      mxmlDelete(comment->child);
#ifdef DEBUG
	      fprintf(stderr, "    new comment %p, last comment %p...\n",
		      comment->child, comment->last_child);
#endif /* DEBUG */
	    }

	    if (variable)
	    {
	      if (strstr(buffer, "@private@"))
	      {
	       /*
		* Delete private variables...
		*/

		mxmlDelete(variable);
	      }
	      else
	      {
		description = mxmlNewElement(variable, "description");
#ifdef DEBUG
		fprintf(stderr, "    adding comment %p/%p to variable...\n",
		        comment->last_child, comment->child);
#endif /* DEBUG */
		mxmlNewText(comment, 0, buffer);
		update_comment(variable,
			       mxmlNewText(description, 0, buffer));
              }

	      variable = NULL;
	    }
	    else if (constant)
	    {
	      if (strstr(buffer, "@private@"))
	      {
	       /*
		* Delete private constants...
		*/

		mxmlDelete(constant);
	      }
	      else
	      {
		description = mxmlNewElement(constant, "description");
#ifdef DEBUG
		fprintf(stderr, "    adding comment %p/%p to constant...\n",
		        comment->last_child, comment->child);
#endif /* DEBUG */
		mxmlNewText(comment, 0, buffer);
		update_comment(constant,
			       mxmlNewText(description, 0, buffer));
              }

	      constant = NULL;
	    }
	    else if (typedefnode)
	    {
	      if (strstr(buffer, "@private@"))
	      {
	       /*
		* Delete private typedefs...
		*/

		mxmlDelete(typedefnode);
		typedefnode = NULL;

		if (structclass)
		{
		  mxmlDelete(structclass);
		  structclass = NULL;
		}

		if (enumeration)
		{
		  mxmlDelete(enumeration);
		  enumeration = NULL;
		}
	      }
	      else
	      {
		description = mxmlNewElement(typedefnode, "description");
#ifdef DEBUG
		fprintf(stderr, "    adding comment %p/%p to typedef %s...\n",
			comment->last_child, comment->child,
			mxmlElementGetAttr(typedefnode, "name"));
#endif /* DEBUG */
		mxmlNewText(comment, 0, buffer);
		update_comment(typedefnode,
			       mxmlNewText(description, 0, buffer));

		if (structclass)
		{
		  description = mxmlNewElement(structclass, "description");
		  update_comment(structclass,
				 mxmlNewText(description, 0, buffer));
		}
		else if (enumeration)
		{
		  description = mxmlNewElement(enumeration, "description");
		  update_comment(enumeration,
				 mxmlNewText(description, 0, buffer));
		}
              }
	    }
	    else if (strcmp(tree->value.element.name, "mxmldoc") &&
		     !mxmlFindElement(tree, tree, "description",
			              NULL, NULL, MXML_DESCEND_FIRST))
            {
              description = mxmlNewElement(tree, "description");
#ifdef DEBUG
	      fprintf(stderr, "    adding comment %p/%p to parent...\n",
	              comment->last_child, comment->child);
#endif /* DEBUG */
	      mxmlNewText(comment, 0, buffer);
	      update_comment(tree,
			     mxmlNewText(description, 0, buffer));
	    }
	    else
              mxmlNewText(comment, 0, buffer);

#ifdef DEBUG
	    fprintf(stderr, "C++ comment: <<<< %s >>>\n", buffer);
#endif /* DEBUG */
	  }
	  else if (ch == ' ' && bufptr == buffer)
	    break;
	  else if (bufptr < (buffer + sizeof(buffer) - 1))
	    *bufptr++ = ch;
          break;

      case STATE_STRING :		/* Inside a string constant */
	  *bufptr++ = ch;

          if (ch == '\\')
	    *bufptr++ = getc(fp);
	  else if (ch == '\"')
	  {
	    *bufptr = '\0';

	    if (type)
	      mxmlNewText(type, type->child != NULL, buffer);

	    state = STATE_NONE;
	  }
          break;

      case STATE_CHARACTER :		/* Inside a character constant */
	  *bufptr++ = ch;

          if (ch == '\\')
	    *bufptr++ = getc(fp);
	  else if (ch == '\'')
	  {
	    *bufptr = '\0';

	    if (type)
	      mxmlNewText(type, type->child != NULL, buffer);

	    state = STATE_NONE;
	  }
          break;

      case STATE_IDENTIFIER :		/* Inside a keyword or identifier */
	  if (isalnum(ch) || ch == '_' || ch == '[' || ch == ']' ||
	      (ch == ',' && (parens > 1 || (type && !enumeration && !function))) ||
	      ch == ':' || ch == '.' || ch == '~')
	  {
	    if (bufptr < (buffer + sizeof(buffer) - 1))
	      *bufptr++ = ch;
	  }
	  else
	  {
	    ungetc(ch, fp);
	    *bufptr = '\0';
	    state   = STATE_NONE;

#ifdef DEBUG
            fprintf(stderr, "    braces=%d, type=%p, type->child=%p, buffer=\"%s\"\n",
	            braces, type, type ? type->child : NULL, buffer);
#endif /* DEBUG */

            if (!braces)
	    {
	      if (!type || !type->child)
	      {
		if (!strcmp(tree->value.element.name, "class"))
		{
		  if (!strcmp(buffer, "public") ||
	              !strcmp(buffer, "public:"))
		  {
		    scope = "public";
#ifdef DEBUG
		    fputs("    scope = public\n", stderr);
#endif /* DEBUG */
		    break;
		  }
		  else if (!strcmp(buffer, "private") ||
	                   !strcmp(buffer, "private:"))
		  {
		    scope = "private";
#ifdef DEBUG
		    fputs("    scope = private\n", stderr);
#endif /* DEBUG */
		    break;
		  }
		  else if (!strcmp(buffer, "protected") ||
	                   !strcmp(buffer, "protected:"))
		  {
		    scope = "protected";
#ifdef DEBUG
		    fputs("    scope = protected\n", stderr);
#endif /* DEBUG */
		    break;
		  }
		}
	      }

	      if (!type)
                type = mxmlNewElement(MXML_NO_PARENT, "type");

#ifdef DEBUG
              fprintf(stderr, "    function=%p (%s), type->child=%p, ch='%c', parens=%d\n",
	              function,
		      function ? mxmlElementGetAttr(function, "name") : "null",
	              type->child, ch, parens);
#endif /* DEBUG */

              if (!function && ch == '(')
	      {
	        if (type->child &&
		    !strcmp(type->child->value.text.string, "extern"))
		{
		 /*
		  * Remove external declarations...
		  */

		  mxmlDelete(type);
		  type = NULL;
		  break;
		}

	        if (type->child &&
		    !strcmp(type->child->value.text.string, "static") &&
		    !strcmp(tree->value.element.name, "mxmldoc"))
		{
		 /*
		  * Remove static functions...
		  */

		  mxmlDelete(type);
		  type = NULL;
		  break;
		}

	        function = mxmlNewElement(MXML_NO_PARENT, "function");
		if ((bufptr = strchr(buffer, ':')) != NULL && bufptr[1] == ':')
		{
		  *bufptr = '\0';
		  bufptr += 2;

		  if ((fstructclass =
		           mxmlFindElement(tree, tree, "class", "name", buffer,
		                           MXML_DESCEND_FIRST)) == NULL)
		    fstructclass =
		        mxmlFindElement(tree, tree, "struct", "name", buffer,
		                        MXML_DESCEND_FIRST);
		}
		else
		  bufptr = buffer;

		mxmlElementSetAttr(function, "name", bufptr);

		if (scope)
		  mxmlElementSetAttr(function, "scope", scope);

#ifdef DEBUG
                fprintf(stderr, "function: %s\n", buffer);
		fprintf(stderr, "    scope = %s\n", scope ? scope : "(null)");
		fprintf(stderr, "    comment = %p\n", comment);
		fprintf(stderr, "    child = (%p) %s\n",
		        comment->child,
			comment->child ?
			    comment->child->value.text.string : "(null)");
		fprintf(stderr, "    last_child = (%p) %s\n",
		        comment->last_child,
			comment->last_child ?
			    comment->last_child->value.text.string : "(null)");
#endif /* DEBUG */

                if (type->last_child &&
		    strcmp(type->last_child->value.text.string, "void"))
		{
                  returnvalue = mxmlNewElement(function, "returnvalue");

		  mxmlAdd(returnvalue, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, type);

		  description = mxmlNewElement(returnvalue, "description");
#ifdef DEBUG
		  fprintf(stderr, "    adding comment %p/%p to returnvalue...\n",
		          comment->last_child, comment->child);
#endif /* DEBUG */
		  update_comment(returnvalue, comment->last_child);
		  mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		          comment->last_child);
                }
		else
		  mxmlDelete(type);

		description = mxmlNewElement(function, "description");
#ifdef DEBUG
		  fprintf(stderr, "    adding comment %p/%p to function...\n",
		          comment->last_child, comment->child);
#endif /* DEBUG */
		update_comment(function, comment->last_child);
		mxmlAdd(description, MXML_ADD_AFTER, MXML_ADD_TO_PARENT,
		        comment->last_child);

		type = NULL;
	      }
	      else if (function && ((ch == ')' && parens == 1) || ch == ','))
	      {
	       /*
	        * Argument definition...
		*/

                if (strcmp(buffer, "void"))
		{
	          mxmlNewText(type, type->child != NULL &&
		                    type->last_child->value.text.string[0] != '(' &&
				    type->last_child->value.text.string[0] != '*',
			      buffer);

#ifdef DEBUG
                  fprintf(stderr, "Argument: <<<< %s >>>\n", buffer);
#endif /* DEBUG */

	          variable = add_variable(function, "argument", type);
		}
		else
		  mxmlDelete(type);

		type = NULL;
	      }
              else if (type->child && !function && (ch == ';' || ch == ','))
	      {
#ifdef DEBUG
	        fprintf(stderr, "    got semicolon, typedefnode=%p, structclass=%p\n",
		        typedefnode, structclass);
#endif /* DEBUG */

	        if (typedefnode || structclass)
		{
#ifdef DEBUG
                  fprintf(stderr, "Typedef/struct/class: <<<< %s >>>>\n", buffer);
#endif /* DEBUG */

		  if (typedefnode)
		  {
		    mxmlElementSetAttr(typedefnode, "name", buffer);

                    sort_node(tree, typedefnode);
		  }

		  if (structclass && !mxmlElementGetAttr(structclass, "name"))
		  {
#ifdef DEBUG
		    fprintf(stderr, "setting struct/class name to %s!\n",
		            type->last_child->value.text.string);
#endif /* DEBUG */
		    mxmlElementSetAttr(structclass, "name", buffer);

		    sort_node(tree, structclass);
		    structclass = NULL;
		  }

		  if (typedefnode)
		    mxmlAdd(typedefnode, MXML_ADD_BEFORE, MXML_ADD_TO_PARENT,
		            type);
                  else
		    mxmlDelete(type);

		  type        = NULL;
		  typedefnode = NULL;
		}
		else if (type->child &&
		         !strcmp(type->child->value.text.string, "typedef"))
		{
		 /*
		  * Simple typedef...
		  */

#ifdef DEBUG
                  fprintf(stderr, "Typedef: <<<< %s >>>\n", buffer);
#endif /* DEBUG */

		  typedefnode = mxmlNewElement(MXML_NO_PARENT, "typedef");
		  mxmlElementSetAttr(typedefnode, "name", buffer);
		  mxmlDelete(type->child);

                  sort_node(tree, typedefnode);

                  if (type->child)
		    type->child->value.text.whitespace = 0;

		  mxmlAdd(typedefnode, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, type);
		  type = NULL;
		}
		else if (!parens)
		{
		 /*
	          * Variable definition...
		  */

	          if (type->child &&
		      !strcmp(type->child->value.text.string, "static") &&
		      !strcmp(tree->value.element.name, "mxmldoc"))
		  {
		   /*
		    * Remove static functions...
		    */

		    mxmlDelete(type);
		    type = NULL;
		    break;
		  }

	          mxmlNewText(type, type->child != NULL &&
		                    type->last_child->value.text.string[0] != '(' &&
				    type->last_child->value.text.string[0] != '*',
			      buffer);

#ifdef DEBUG
                  fprintf(stderr, "Variable: <<<< %s >>>>\n", buffer);
                  fprintf(stderr, "    scope = %s\n", scope ? scope : "(null)");
#endif /* DEBUG */

	          variable = add_variable(MXML_NO_PARENT, "variable", type);
		  type     = NULL;

		  sort_node(tree, variable);

		  if (scope)
		    mxmlElementSetAttr(variable, "scope", scope);
		}
              }
	      else
              {
#ifdef DEBUG
                fprintf(stderr, "Identifier: <<<< %s >>>>\n", buffer);
#endif /* DEBUG */

	        mxmlNewText(type, type->child != NULL &&
		                  type->last_child->value.text.string[0] != '(' &&
				  type->last_child->value.text.string[0] != '*',
			    buffer);
	      }
	    }
	    else if (enumeration && !isdigit(buffer[0] & 255))
	    {
#ifdef DEBUG
	      fprintf(stderr, "Constant: <<<< %s >>>\n", buffer);
#endif /* DEBUG */

	      constant = mxmlNewElement(MXML_NO_PARENT, "constant");
	      mxmlElementSetAttr(constant, "name", buffer);
	      sort_node(enumeration, constant);
	    }
	    else if (type)
	    {
	      mxmlDelete(type);
	      type = NULL;
	    }
	  }
          break;
    }

#if DEBUG > 1
    if (state != oldstate)
    {
      fprintf(stderr, "    changed states from %s to %s on receipt of character '%c'...\n",
              states[oldstate], states[state], oldch);
      fprintf(stderr, "    variable = %p\n", variable);
      if (type)
      {
        fputs("    type =", stderr);
        for (temp = type->child; temp; temp = temp->next)
	  fprintf(stderr, " \"%s\"", temp->value.text.string);
	fputs("\n", stderr);
      }
    }
#endif /* DEBUG > 1 */
  }

  mxmlDelete(comment);

 /*
  * All done, return with no errors...
  */

  return (0);
}


/*
 * 'sort_node()' - Insert a node sorted into a tree.
 */

static void
sort_node(mxml_node_t *tree,		/* I - Tree to sort into */
          mxml_node_t *node)		/* I - Node to add */
{
  mxml_node_t	*temp;			/* Current node */
  const char	*tempname,		/* Name of current node */
		*nodename,		/* Name of node */
		*scope;			/* Scope */


#if DEBUG > 1
  fprintf(stderr, "    sort_node(tree=%p, node=%p)\n", tree, node);
#endif /* DEBUG > 1 */

 /*
  * Range check input...
  */

  if (!tree || !node || node->parent == tree)
    return;

 /*
  * Get the node name...
  */

  if ((nodename = mxmlElementGetAttr(node, "name")) == NULL)
    return;

  if (nodename[0] == '_')
    return;				/* Hide private names */

#if DEBUG > 1
  fprintf(stderr, "        nodename=%p (\"%s\")\n", nodename, nodename);
#endif /* DEBUG > 1 */

 /*
  * Delete any existing definition at this level, if one exists...
  */

  if ((temp = mxmlFindElement(tree, tree, node->value.element.name,
                              "name", nodename, MXML_DESCEND_FIRST)) != NULL)
  {
   /*
    * Copy the scope if needed...
    */

    if ((scope = mxmlElementGetAttr(temp, "scope")) != NULL &&
        mxmlElementGetAttr(node, "scope") == NULL)
    {
#ifdef DEBUG
      fprintf(stderr, "    copying scope %s for %s\n", scope, nodename);
#endif /* DEBUG */

      mxmlElementSetAttr(node, "scope", scope);
    }

    mxmlDelete(temp);
  }

 /*
  * Add the node into the tree at the proper place...
  */

  for (temp = tree->child; temp; temp = temp->next)
  {
#if DEBUG > 1
    fprintf(stderr, "        temp=%p\n", temp);
#endif /* DEBUG > 1 */

    if ((tempname = mxmlElementGetAttr(temp, "name")) == NULL)
      continue;

#if DEBUG > 1
    fprintf(stderr, "        tempname=%p (\"%s\")\n", tempname, tempname);
#endif /* DEBUG > 1 */

    if (strcmp(nodename, tempname) < 0)
      break;
  }

  if (temp)
    mxmlAdd(tree, MXML_ADD_BEFORE, temp, node);
  else
    mxmlAdd(tree, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, node);
}


/*
 * 'update_comment()' - Update a comment node.
 */

static void
update_comment(mxml_node_t *parent,	/* I - Parent node */
               mxml_node_t *comment)	/* I - Comment node */
{
  char	*ptr;				/* Pointer into comment */


#ifdef DEBUG
  fprintf(stderr, "update_comment(parent=%p, comment=%p)\n",
          parent, comment);
#endif /* DEBUG */

 /*
  * Range check the input...
  */

  if (!parent || !comment)
    return;

 /*
  * Convert "\/" to "/"...
  */

  for (ptr = strstr(comment->value.text.string, "\\/");
       ptr;
       ptr = strstr(ptr, "\\/"))
    safe_strcpy(ptr, ptr + 1);

 /*
  * Update the comment...
  */

  ptr = comment->value.text.string;

  if (*ptr == '\'')
  {
   /*
    * Convert "'name()' - description" to "description".
    */

    for (ptr ++; *ptr && *ptr != '\''; ptr ++);

    if (*ptr == '\'')
    {
      ptr ++;
      while (isspace(*ptr & 255))
        ptr ++;

      if (*ptr == '-')
        ptr ++;

      while (isspace(*ptr & 255))
        ptr ++;

      safe_strcpy(comment->value.text.string, ptr);
    }
  }
  else if (!strncmp(ptr, "I ", 2) || !strncmp(ptr, "O ", 2) ||
           !strncmp(ptr, "IO ", 3))
  {
   /*
    * 'Convert "I - description", "IO - description", or "O - description"
    * to description + direction attribute.
    */

    ptr = strchr(ptr, ' ');
    *ptr++ = '\0';

    if (!strcmp(parent->value.element.name, "argument"))
      mxmlElementSetAttr(parent, "direction", comment->value.text.string);

    while (isspace(*ptr & 255))
      ptr ++;

    if (*ptr == '-')
      ptr ++;

    while (isspace(*ptr & 255))
      ptr ++;

    safe_strcpy(comment->value.text.string, ptr);
  }

 /*
  * Eliminate leading and trailing *'s...
  */

  for (ptr = comment->value.text.string; *ptr == '*'; ptr ++);
  for (; isspace(*ptr & 255); ptr ++);
  if (ptr > comment->value.text.string)
    safe_strcpy(comment->value.text.string, ptr);

  for (ptr = comment->value.text.string + strlen(comment->value.text.string) - 1;
       ptr > comment->value.text.string && *ptr == '*';
       ptr --)
    *ptr = '\0';
  for (; ptr > comment->value.text.string && isspace(*ptr & 255); ptr --)
    *ptr = '\0';

#ifdef DEBUG
  fprintf(stderr, "    updated comment = %s\n", comment->value.text.string);
#endif /* DEBUG */
}


/*
 * 'usage()' - Show program usage...
 */

static void
usage(const char *option)		/* I - Unknown option */
{
  if (option)
    printf("mxmldoc: Bad option \"%s\"!\n\n", option);

  puts("Usage: mxmldoc [options] [filename.xml] [source files] >filename.html");
  puts("Options:");
  puts("    --css filename.css         Set CSS stylesheet file");
  puts("    --docset bundleid.docset   Generate documentation set");
  puts("    --docversion version       Set documentation version");
  puts("    --feedname name            Set documentation set feed name");
  puts("    --feedurl url              Set documentation set feed URL");
  puts("    --footer footerfile        Set footer file");
  puts("    --framed basename          Generate framed HTML to basename*.html");
  puts("    --header headerfile        Set header file");
  puts("    --intro introfile          Set introduction file");
  puts("    --man name                 Generate man page");
  puts("    --no-output                Do no generate documentation file");
  puts("    --section section          Set section name");
  puts("    --title title              Set documentation title");
  puts("    --tokens path              Generate Xcode docset Tokens.xml file");
  puts("    --version                  Show mxmldoc/Mini-XML version");

  exit(1);
}


/*
 * 'write_description()' - Write the description text.
 */

static void
write_description(
    FILE        *out,			/* I - Output file */
    mxml_node_t *description,		/* I - Description node */
    const char  *element,		/* I - HTML element, if any */
    int         summary)		/* I - Show summary */
{
  char	text[10240],			/* Text for description */
        *start,				/* Start of code/link */
	*ptr;				/* Pointer into text */
  int	col;				/* Current column */


  if (!description)
    return;

  get_text(description, text, sizeof(text));

  ptr = strstr(text, "\n\n");

  if (summary)
  {
    if (ptr)
      *ptr = '\0';

    ptr = text;
  }
  else if (!ptr || !ptr[2])
    return;
  else
    ptr += 2;

  if (element && *element)
    fprintf(out, "<%s class=\"%s\">", element,
            summary ? "description" : "discussion");
  else if (!summary)
    fputs(".PP\n", out);

  for (col = 0; *ptr; ptr ++)
  {
    if (*ptr == '@' &&
        (!strncmp(ptr + 1, "deprecated@", 11) ||
         !strncmp(ptr + 1, "since ", 6)))
    {
      ptr ++;
      while (*ptr && *ptr != '@')
        ptr ++;

      if (!*ptr)
        return;
    }
    else if (!strncmp(ptr, "@code ", 6))
    {
      for (ptr += 6; isspace(*ptr & 255); ptr ++);

      for (start = ptr, ptr ++; *ptr && *ptr != '@'; ptr ++);

      if (*ptr)
        *ptr = '\0';
      else
        ptr --;

      if (element && *element)
      {
        fputs("<code>", out);
        for (; *start; start ++)
        {
          if (*start == '<')
            fputs("&lt;", out);
          else if (*start == '>')
            fputs("&gt;", out);
          else if (*start == '&')
            fputs("&amp;", out);
          else
            putc(*start, out);
        }
        fputs("</code>", out);
      }
      else if (element)
        fputs(start, out);
      else
        fprintf(out, "\\fB%s\\fR", start);
    }
    else if (!strncmp(ptr, "@link ", 6))
    {
      for (ptr += 6; isspace(*ptr & 255); ptr ++);

      for (start = ptr, ptr ++; *ptr && *ptr != '@'; ptr ++);

      if (*ptr)
        *ptr = '\0';
      else
        ptr --;

      if (element && *element)
        fprintf(out, "<a href=\"#%s\"><code>%s</code></a>", start, start);
      else if (element)
        fputs(start, out);
      else
        fprintf(out, "\\fI%s\\fR", start);
    }
    else if (element)
    {
      if (*ptr == '&')
        fputs("&amp;", out);
      else if (*ptr == '<')
        fputs("&lt;", out);
      else if (*ptr == '>')
        fputs("&gt;", out);
      else if (*ptr == '\"')
        fputs("&quot;", out);
      else if (*ptr & 128)
      {
       /*
        * Convert UTF-8 to Unicode constant...
        */

        int	ch;			/* Unicode character */


        ch = *ptr & 255;

        if ((ch & 0xe0) == 0xc0)
        {
          ch = ((ch & 0x1f) << 6) | (ptr[1] & 0x3f);
	  ptr ++;
        }
        else if ((ch & 0xf0) == 0xe0)
        {
          ch = ((((ch * 0x0f) << 6) | (ptr[1] & 0x3f)) << 6) | (ptr[2] & 0x3f);
	  ptr += 2;
        }

        if (ch == 0xa0)
        {
         /*
          * Handle non-breaking space as-is...
	  */

          fputs("&nbsp;", out);
        }
        else
          fprintf(out, "&#x%x;", ch);
      }
      else if (*ptr == '\n' && ptr[1] == '\n' && ptr[2] && ptr[2] != '@')
      {
        fputs("<br>\n<br>\n", out);
        ptr ++;
      }
      else
        putc(*ptr, out);
    }
    else if (*ptr == '\n' && ptr[1] == '\n' && ptr[2] && ptr[2] != '@')
    {
      fputs("\n.PP\n", out);
      ptr ++;
    }
    else
    {
      if (*ptr == '\\' || (*ptr == '.' && col == 0))
        putc('\\', out);

      putc(*ptr, out);

      if (*ptr == '\n')
        col = 0;
      else
        col ++;
    }
  }

  if (element && *element)
    fprintf(out, "</%s>\n", element);
  else if (!element)
    putc('\n', out);
}


/*
 * 'write_element()' - Write an element's text nodes.
 */

static void
write_element(FILE        *out,		/* I - Output file */
              mxml_node_t *doc,		/* I - Document tree */
              mxml_node_t *element,	/* I - Element to write */
              int         mode)		/* I - Output mode */
{
  mxml_node_t	*node;			/* Current node */


  if (!element)
    return;

  for (node = element->child;
       node;
       node = mxmlWalkNext(node, element, MXML_NO_DESCEND))
    if (node->type == MXML_TEXT)
    {
      if (node->value.text.whitespace)
	putc(' ', out);

      if (mode == OUTPUT_HTML &&
          (mxmlFindElement(doc, doc, "class", "name", node->value.text.string,
                           MXML_DESCEND) ||
	   mxmlFindElement(doc, doc, "enumeration", "name",
	                   node->value.text.string, MXML_DESCEND) ||
	   mxmlFindElement(doc, doc, "struct", "name", node->value.text.string,
                           MXML_DESCEND) ||
	   mxmlFindElement(doc, doc, "typedef", "name", node->value.text.string,
                           MXML_DESCEND) ||
	   mxmlFindElement(doc, doc, "union", "name", node->value.text.string,
                           MXML_DESCEND)))
      {
        fputs("<a href=\"#", out);
        write_string(out, node->value.text.string, mode);
	fputs("\">", out);
        write_string(out, node->value.text.string, mode);
	fputs("</a>", out);
      }
      else
        write_string(out, node->value.text.string, mode);
    }

  if (!strcmp(element->value.element.name, "type") &&
      element->last_child->value.text.string[0] != '*')
    putc(' ', out);
}


/*
 * 'write_file()' - Copy a file to the output.
 */

static void
write_file(FILE       *out,		/* I - Output file */
           const char *file)		/* I - File to copy */
{
  FILE		*fp;			/* Copy file */
  char		line[8192];		/* Line from file */


  if ((fp = fopen(file, "r")) == NULL)
  {
    fprintf(stderr, "mxmldoc: Unable to open \"%s\": %s\n", file,
            strerror(errno));
    return;
  }

  while (fgets(line, sizeof(line), fp))
    fputs(line, out);

  fclose(fp);
}


/*
 * 'write_function()' - Write documentation for a function.
 */

static void
write_function(FILE        *out,	/* I - Output file */
               mxml_node_t *doc,	/* I - Document */
               mxml_node_t *function,	/* I - Function */
	       int         level)	/* I - Base heading level */
{
  mxml_node_t	*arg,			/* Current argument */
		*adesc,			/* Description of argument */
		*description,		/* Description of function */
		*type,			/* Type for argument */
		*node;			/* Node in description */
  const char	*name,			/* Name of function/type */
		*defval;		/* Default value */
  char		prefix;			/* Prefix character */
  char		*sep;			/* Newline separator */


  name        = mxmlElementGetAttr(function, "name");
  description = mxmlFindElement(function, function, "description", NULL,
				NULL, MXML_DESCEND_FIRST);

  fprintf(out, "<h%d class=\"%s\">%s<a name=\"%s\">%s</a></h%d>\n",
	  level, level == 3 ? "function" : "method",
	  get_comment_info(description), name, name, level);

  if (description)
    write_description(out, description, "p", 1);

  fputs("<p class=\"code\">\n", out);

  arg = mxmlFindElement(function, function, "returnvalue", NULL,
			NULL, MXML_DESCEND_FIRST);

  if (arg)
    write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
					    NULL, MXML_DESCEND_FIRST),
		  OUTPUT_HTML);
  else
    fputs("void ", out);

  fprintf(out, "%s ", name);
  for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
			     MXML_DESCEND_FIRST), prefix = '(';
       arg;
       arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
			     MXML_NO_DESCEND), prefix = ',')
  {
    type = mxmlFindElement(arg, arg, "type", NULL, NULL,
			   MXML_DESCEND_FIRST);

    fprintf(out, "%c<br>\n&nbsp;&nbsp;&nbsp;&nbsp;", prefix);
    if (type->child)
      write_element(out, doc, type, OUTPUT_HTML);

    fputs(mxmlElementGetAttr(arg, "name"), out);
    if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
      fprintf(out, " %s", defval);
  }

  if (prefix == '(')
    fputs("(void);</p>\n", out);
  else
  {
    fprintf(out,
            "<br>\n);</p>\n"
	    "<h%d class=\"parameters\">Parameters</h%d>\n"
	    "<dl>\n", level + 1, level + 1);

    for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
			       MXML_DESCEND_FIRST);
	 arg;
	 arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
			       MXML_NO_DESCEND))
    {
      fprintf(out, "<dt>%s</dt>\n", mxmlElementGetAttr(arg, "name"));

      adesc = mxmlFindElement(arg, arg, "description", NULL, NULL,
			      MXML_DESCEND_FIRST);

      write_description(out, adesc, "dd", 1);
      write_description(out, adesc, "dd", 0);
    }

    fputs("</dl>\n", out);
  }

  arg = mxmlFindElement(function, function, "returnvalue", NULL,
			NULL, MXML_DESCEND_FIRST);

  if (arg)
  {
    fprintf(out, "<h%d class=\"returnvalue\">Return Value</h%d>\n", level + 1,
            level + 1);

    adesc = mxmlFindElement(arg, arg, "description", NULL, NULL,
			    MXML_DESCEND_FIRST);

    write_description(out, adesc, "p", 1);
    write_description(out, adesc, "p", 0);
  }

  if (description)
  {
    for (node = description->child; node; node = node->next)
      if (node->value.text.string &&
	  (sep = strstr(node->value.text.string, "\n\n")) != NULL)
      {
	sep += 2;
	if (*sep && strncmp(sep, "@since ", 7) &&
	    strncmp(sep, "@deprecated@", 12))
	  break;
      }

    if (node)
    {
      fprintf(out, "<h%d class=\"discussion\">Discussion</h%d>\n", level + 1,
	      level + 1);
      write_description(out, description, "p", 0);
    }
  }
}


/*
 * 'write_html()' - Write HTML documentation.
 */

static void
write_html(const char  *section,	/* I - Section */
	   const char  *title,		/* I - Title */
	   const char  *footerfile,	/* I - Footer file */
	   const char  *headerfile,	/* I - Header file */
	   const char  *introfile,	/* I - Intro file */
	   const char  *cssfile,	/* I - Stylesheet file */
	   const char  *framefile,	/* I - Framed HTML basename */
	   const char  *docset,		/* I - Documentation set directory */
	   const char  *docversion,	/* I - Documentation set version */
	   const char  *feedname,	/* I - Feed name for doc set */
	   const char  *feedurl,	/* I - Feed URL for doc set */
	   mxml_node_t *doc)		/* I - XML documentation */
{
  FILE		*out;			/* Output file */
  mxml_node_t	*function,		/* Current function */
		*scut,			/* Struct/class/union/typedef */
		*arg,			/* Current argument */
		*description,		/* Description of function/var */
		*type;			/* Type for argument */
  const char	*name,			/* Name of function/type */
		*defval,		/* Default value */
		*basename;		/* Base filename for framed output */
  char		filename[1024];		/* Current output filename */


  if (framefile)
  {
   /*
    * Get the basename of the frame file...
    */

    if ((basename = strrchr(framefile, '/')) != NULL)
      basename ++;
    else
      basename = framefile;

    if (strstr(basename, ".html"))
      fputs("mxmldoc: Frame base name should not contain .html extension!\n",
            stderr);

   /*
    * Create the container HTML file for the frames...
    */

    snprintf(filename, sizeof(filename), "%s.html", framefile);

    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    fputs("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" "
          "\"http://www.w3.org/TR/html4/frameset.dtd\">\n"
	  "<html>\n"
	  "<head>\n"
	  "\t<title>", out);
    write_string(out, title, OUTPUT_HTML);
    fputs("</title>\n", out);

    if (section)
      fprintf(out, "\t<meta name=\"keywords\" content=\"%s\">\n", section);

    fputs("\t<meta http-equiv=\"Content-Type\" "
          "content=\"text/html;charset=utf-8\">\n"
	  "\t<meta name=\"creator\" content=\"" MXML_VERSION "\">\n"
          "</head>\n", out);

    fputs("<frameset cols=\"250,*\">\n", out);
    fprintf(out, "<frame src=\"%s-toc.html\">\n", basename);
    fprintf(out, "<frame name=\"body\" src=\"%s-body.html\">\n", basename);
    fputs("</frameset>\n"
          "<noframes>\n"
	  "<h1>", out);
    write_string(out, title, OUTPUT_HTML);
    fprintf(out,
            "</h1>\n"
            "<ul>\n"
	    "\t<li><a href=\"%s-toc.html\">Table of Contents</a></li>\n"
	    "\t<li><a href=\"%s-body.html\">Body</a></li>\n"
	    "</ul>\n", basename, basename);
    fputs("</noframes>\n"
          "</html>\n", out);
    fclose(out);

   /*
    * Write the table-of-contents file...
    */

    snprintf(filename, sizeof(filename), "%s-toc.html", framefile);

    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    write_html_head(out, section, title, cssfile);

    snprintf(filename, sizeof(filename), "%s-body.html", basename);

    fputs("<div class=\"contents\">\n", out);
    fprintf(out, "<h1 class=\"title\"><a href=\"%s\" target=\"body\">",
            filename);
    write_string(out, title, OUTPUT_HTML);
    fputs("</a></h1>\n", out);

    write_toc(out, doc, introfile, filename, 0);

    fputs("</div>\n"
          "</body>\n"
          "</html>\n", out);
    fclose(out);

   /*
    * Finally, open the body file...
    */

    snprintf(filename, sizeof(filename), "%s-body.html", framefile);

    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }
  }
  else if (docset)
  {
   /*
    * Create an Xcode documentation set - start by removing any existing
    * output directory...
    */

#ifdef __APPLE__
    const char	*id;			/* Identifier */


    if (!access(docset, 0) && !remove_directory(docset))
      return;

   /*
    * Then make the Apple standard bundle directory structure...
    */

    if (mkdir(docset, 0755))
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", docset,
              strerror(errno));
      return;
    }

    snprintf(filename, sizeof(filename), "%s/Contents", docset);
    if (mkdir(filename, 0755))
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    snprintf(filename, sizeof(filename), "%s/Contents/Resources", docset);
    if (mkdir(filename, 0755))
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    snprintf(filename, sizeof(filename), "%s/Contents/Resources/Documentation",
             docset);
    if (mkdir(filename, 0755))
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

   /*
    * The Info.plist file, which describes the documentation set...
    */

    if ((id = strrchr(docset, '/')) != NULL)
      id ++;
    else
      id = docset;

    snprintf(filename, sizeof(filename), "%s/Contents/Info.plist", docset);
    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
          "<plist version=\"1.0\">\n"
          "<dict>\n"
	  "\t<key>CFBundleIdentifier</key>\n"
	  "\t<string>", out);
    write_string(out, id, OUTPUT_HTML);
    fputs("</string>\n"
          "\t<key>CFBundleName</key>\n"
	  "\t<string>", out);
    write_string(out, title, OUTPUT_HTML);
    fputs("</string>\n"
          "\t<key>CFBundleVersion</key>\n"
	  "\t<string>", out);
    write_string(out, docversion ? docversion : "0.0", OUTPUT_HTML);
    fputs("</string>\n"
          "\t<key>CFBundleShortVersionString</key>\n"
	  "\t<string>", out);
    write_string(out, docversion ? docversion : "0.0", OUTPUT_HTML);
    fputs("</string>\n", out);

    if (feedname)
    {
      fputs("\t<key>DocSetFeedName</key>\n"
	    "\t<string>", out);
      write_string(out, feedname ? feedname : title, OUTPUT_HTML);
      fputs("</string>\n", out);
    }

    if (feedurl)
    {
      fputs("\t<key>DocSetFeedURL</key>\n"
	    "\t<string>", out);
      write_string(out, feedurl, OUTPUT_HTML);
      fputs("</string>\n", out);
    }

    fputs("</dict>\n"
          "</plist>\n", out);

    fclose(out);

   /*
    * Next the Nodes.xml file...
    */

    snprintf(filename, sizeof(filename), "%s/Contents/Resources/Nodes.xml",
             docset);
    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<DocSetNodes version=\"1.0\">\n"
	  "<TOC>\n"
	  "<Node id=\"0\">\n"
	  "<Name>", out);
    write_string(out, title, OUTPUT_HTML);
    fputs("</Name>\n"
          "<Path>Documentation/index.html</Path>\n"
	  "<Subnodes>\n", out);

    write_toc(out, doc, introfile, NULL, 1);

    fputs("</Subnodes>\n"
          "</Node>\n"
          "</TOC>\n"
          "</DocSetNodes>\n", out);

    fclose(out);

   /*
    * Then the Tokens.xml file...
    */

    snprintf(filename, sizeof(filename), "%s/Contents/Resources/Tokens.xml",
             docset);
    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<Tokens version=\"1.0\">\n", out);

    write_tokens(out, doc, "index.html");

    fputs("</Tokens>\n", out);

    fclose(out);

   /*
    * Finally the HTML file...
    */

    snprintf(filename, sizeof(filename),
             "%s/Contents/Resources/Documentation/index.html",
             docset);
    if ((out = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "mxmldoc: Unable to create \"%s\": %s\n", filename,
              strerror(errno));
      return;
    }

#else
    fputs("mxmldoc: Xcode documentation sets can only be created on "
          "Mac OS X.\n", stderr);
    return;
#endif /* __APPLE__ */
  }
  else
    out = stdout;

 /*
  * Standard header...
  */

  write_html_head(out, section, title, cssfile);

  fputs("<div class='body'>\n", out);

 /*
  * Header...
  */

  if (headerfile)
  {
   /*
    * Use custom header...
    */

    write_file(out, headerfile);
  }
  else
  {
   /*
    * Use standard header...
    */

    fputs("<h1 class=\"title\">", out);
    write_string(out, title, OUTPUT_HTML);
    fputs("</h1>\n", out);
  }

 /*
  * Table of contents...
  */

  if (!framefile)
    write_toc(out, doc, introfile, NULL, 0);

 /*
  * Intro...
  */

  if (introfile)
    write_file(out, introfile);

 /*
  * List of classes...
  */

  if ((scut = find_public(doc, doc, "class")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"CLASSES\">Classes</a></h2>\n", out);

    while (scut)
    {
      write_scu(out, doc, scut);

      scut = find_public(scut, doc, "class");
    }
  }

 /*
  * List of functions...
  */

  if ((function = find_public(doc, doc, "function")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"FUNCTIONS\">Functions</a></h2>\n", out);

    while (function)
    {
      write_function(out, doc, function, 3);

      function = find_public(function, doc, "function");
    }
  }

 /*
  * List of types...
  */

  if ((scut = find_public(doc, doc, "typedef")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"TYPES\">Data Types</a></h2>\n", out);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      fprintf(out, "<h3 class=\"typedef\">%s<a name=\"%s\">%s</a></h3>\n",
	      get_comment_info(description), name, name);

      if (description)
	write_description(out, description, "p", 1);

      fputs("<p class=\"code\">\n"
	    "typedef ", out);

      type = mxmlFindElement(scut, scut, "type", NULL, NULL,
                             MXML_DESCEND_FIRST);

      for (type = type->child; type; type = type->next)
        if (!strcmp(type->value.text.string, "("))
	  break;
	else
	{
	  if (type->value.text.whitespace)
	    putc(' ', out);

	  if (mxmlFindElement(doc, doc, "class", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "enumeration", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "struct", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "typedef", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "union", "name",
	                      type->value.text.string, MXML_DESCEND))
	  {
            fputs("<a href=\"#", out);
            write_string(out, type->value.text.string, OUTPUT_HTML);
	    fputs("\">", out);
            write_string(out, type->value.text.string, OUTPUT_HTML);
	    fputs("</a>", out);
	  }
	  else
            write_string(out, type->value.text.string, OUTPUT_HTML);
        }

      if (type)
      {
       /*
        * Output function type...
	*/

        if (type->prev && type->prev->value.text.string[0] != '*')
	  putc(' ', out);

        fprintf(out, "(*%s", name);

	for (type = type->next->next; type; type = type->next)
	{
	  if (type->value.text.whitespace)
	    putc(' ', out);

	  if (mxmlFindElement(doc, doc, "class", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "enumeration", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "struct", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "typedef", "name",
	                      type->value.text.string, MXML_DESCEND) ||
	      mxmlFindElement(doc, doc, "union", "name",
	                      type->value.text.string, MXML_DESCEND))
	  {
            fputs("<a href=\"#", out);
            write_string(out, type->value.text.string, OUTPUT_HTML);
	    fputs("\">", out);
            write_string(out, type->value.text.string, OUTPUT_HTML);
	    fputs("</a>", out);
	  }
	  else
            write_string(out, type->value.text.string, OUTPUT_HTML);
        }

        fputs(";\n", out);
      }
      else
      {
	type = mxmlFindElement(scut, scut, "type", NULL, NULL,
			       MXML_DESCEND_FIRST);
        if (type->last_child->value.text.string[0] != '*')
	  putc(' ', out);

	fprintf(out, "%s;\n", name);
      }

      fputs("</p>\n", out);

      scut = find_public(scut, doc, "typedef");
    }
  }

 /*
  * List of structures...
  */

  if ((scut = find_public(doc, doc, "struct")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"STRUCTURES\">Structures</a></h2>\n",
          out);

    while (scut)
    {
      write_scu(out, doc, scut);

      scut = find_public(scut, doc, "struct");
    }
  }

 /*
  * List of unions...
  */

  if ((scut = find_public(doc, doc, "union")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"UNIONS\">Unions</a></h2>\n", out);

    while (scut)
    {
      write_scu(out, doc, scut);

      scut = find_public(scut, doc, "union");
    }
  }

 /*
  * Variables...
  */

  if ((arg = find_public(doc, doc, "variable")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"VARIABLES\">Variables</a></h2>\n",
          out);

    while (arg)
    {
      name        = mxmlElementGetAttr(arg, "name");
      description = mxmlFindElement(arg, arg, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      fprintf(out, "<h3 class=\"variable\">%s<a name=\"%s\">%s</a></h3>\n",
	      get_comment_info(description), name, name);

      if (description)
	write_description(out, description, "p", 1);

      fputs("<p class=\"code\">", out);

      write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
                                              NULL, MXML_DESCEND_FIRST),
                    OUTPUT_HTML);
      fputs(mxmlElementGetAttr(arg, "name"), out);
      if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	fprintf(out, " %s", defval);
      fputs(";</p>\n", out);

      arg = find_public(arg, doc, "variable");
    }
  }

 /*
  * List of enumerations...
  */

  if ((scut = find_public(doc, doc, "enumeration")) != NULL)
  {
    fputs("<h2 class=\"title\"><a name=\"ENUMERATIONS\">Constants</a></h2>\n",
          out);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      fprintf(out, "<h3 class=\"enumeration\">%s<a name=\"%s\">%s</a></h3>\n",
              get_comment_info(description), name, name);

      if (description)
	write_description(out, description, "p", 1);

      fputs("<h4 class=\"constants\">Constants</h4>\n"
            "<dl>\n", out);

      for (arg = mxmlFindElement(scut, scut, "constant", NULL, NULL,
                        	 MXML_DESCEND_FIRST);
	   arg;
	   arg = mxmlFindElement(arg, scut, "constant", NULL, NULL,
                        	 MXML_NO_DESCEND))
      {
	description = mxmlFindElement(arg, arg, "description", NULL,
                                      NULL, MXML_DESCEND_FIRST);
	fprintf(out, "<dt>%s %s</dt>\n",
	        mxmlElementGetAttr(arg, "name"), get_comment_info(description));

	write_description(out, description, "dd", 1);
	write_description(out, description, "dd", 0);
      }

      fputs("</dl>\n", out);

      scut = find_public(scut, doc, "enumeration");
    }
  }

 /*
  * Footer...
  */

  if (footerfile)
  {
   /*
    * Use custom footer...
    */

    write_file(out, footerfile);
  }

  fputs("</div>\n"
        "</body>\n"
        "</html>\n", out);

 /*
  * Close output file as needed...
  */

  if (out != stdout)
    fclose(out);

#ifdef __APPLE__
 /*
  * When generating document sets, run the docsetutil program to index it...
  */

  if (docset)
  {
    int		argc = 0;		/* Argument count */
    const char	*args[5];		/* Argument array */
    pid_t	pid;			/* Process ID */
    int		status;			/* Exit status */


    args[argc++] = "/usr/bin/xcrun";
    args[argc++] = "docsetutil";
    args[argc++] = "index";
    args[argc++] = docset;
    args[argc  ] = NULL;

    if (posix_spawn(&pid, args[0], NULL, NULL, (char **)args, environ))
    {
      fprintf(stderr, "mxmldoc: Unable to index documentation set \"%s\": %s\n",
              docset, strerror(errno));
    }
    else
    {
      while (wait(&status) != pid);

      if (status)
      {
        if (WIFEXITED(status))
	  fprintf(stderr, "mxmldoc: docsetutil exited with status %d\n",
		  WEXITSTATUS(status));
        else
	  fprintf(stderr, "mxmldoc: docsetutil crashed with signal %d\n",
		  WTERMSIG(status));
      }
      else
      {
       /*
        * Remove unneeded temporary XML files...
	*/

	snprintf(filename, sizeof(filename), "%s/Contents/Resources/Nodes.xml",
		 docset);
        unlink(filename);

	snprintf(filename, sizeof(filename), "%s/Contents/Resources/Tokens.xml",
		 docset);
        unlink(filename);
      }
    }
  }
#endif /* __APPLE__ */
}


/*
 * 'write_html_head()' - Write the standard HTML header.
 */

static void
write_html_head(FILE       *out,	/* I - Output file */
                const char *section,	/* I - Section */
                const char *title,	/* I - Title */
		const char *cssfile)	/* I - Stylesheet */
{
  fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
        "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
        "<html>\n", out);

  if (section)
    fprintf(out, "<!-- SECTION: %s -->\n", section);

  fputs("<head>\n"
        "\t<title>", out);
  write_string(out, title, OUTPUT_HTML);
  fputs("\t</title>\n", out);

  if (section)
    fprintf(out, "\t<meta name=\"keywords\" content=\"%s\">\n", section);

  fputs("\t<meta http-equiv=\"Content-Type\" "
	"content=\"text/html;charset=utf-8\">\n"
	"\t<meta name=\"creator\" content=\"" MXML_VERSION "\">\n"
        "<style type=\"text/css\"><!--\n", out);

  if (cssfile)
  {
   /*
    * Use custom stylesheet file...
    */

    write_file(out, cssfile);
  }
  else
  {
   /*
    * Use standard stylesheet...
    */

    fputs("body, p, h1, h2, h3, h4 {\n"
	  "  font-family: \"lucida grande\", geneva, helvetica, arial, "
	  "sans-serif;\n"
	  "}\n"
	  "div.body h1 {\n"
	  "  font-size: 250%;\n"
	  "  font-weight: bold;\n"
	  "  margin: 0;\n"
	  "}\n"
	  "div.body h2 {\n"
	  "  font-size: 250%;\n"
	  "  margin-top: 1.5em;\n"
	  "}\n"
	  "div.body h3 {\n"
	  "  font-size: 150%;\n"
	  "  margin-bottom: 0.5em;\n"
	  "  margin-top: 1.5em;\n"
	  "}\n"
	  "div.body h4 {\n"
	  "  font-size: 110%;\n"
	  "  margin-bottom: 0.5em;\n"
	  "  margin-top: 1.5em;\n"
	  "}\n"
	  "div.body h5 {\n"
	  "  font-size: 100%;\n"
	  "  margin-bottom: 0.5em;\n"
	  "  margin-top: 1.5em;\n"
	  "}\n"
	  "div.contents {\n"
	  "  background: #e8e8e8;\n"
	  "  border: solid thin black;\n"
	  "  padding: 10px;\n"
	  "}\n"
	  "div.contents h1 {\n"
	  "  font-size: 110%;\n"
	  "}\n"
	  "div.contents h2 {\n"
	  "  font-size: 100%;\n"
	  "}\n"
	  "div.contents ul.contents {\n"
	  "  font-size: 80%;\n"
	  "}\n"
	  ".class {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "}\n"
	  ".constants {\n"
	  "}\n"
	  ".description {\n"
	  "  margin-top: 0.5em;\n"
	  "}\n"
	  ".discussion {\n"
	  "}\n"
	  ".enumeration {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "}\n"
	  ".function {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "  margin-bottom: 0;\n"
	  "}\n"
	  ".members {\n"
	  "}\n"
	  ".method {\n"
	  "}\n"
	  ".parameters {\n"
	  "}\n"
	  ".returnvalue {\n"
	  "}\n"
	  ".struct {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "}\n"
	  ".typedef {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "}\n"
	  ".union {\n"
	  "  border-bottom: solid 2px gray;\n"
	  "}\n"
	  ".variable {\n"
	  "}\n"
	  "code, p.code, pre, ul.code li {\n"
	  "  font-family: monaco, courier, monospace;\n"
	  "  font-size: 90%;\n"
	  "}\n"
	  "a:link, a:visited {\n"
	  "  text-decoration: none;\n"
	  "}\n"
	  "span.info {\n"
	  "  background: black;\n"
	  "  border: solid thin black;\n"
	  "  color: white;\n"
	  "  font-size: 80%;\n"
	  "  font-style: italic;\n"
	  "  font-weight: bold;\n"
	  "  white-space: nowrap;\n"
	  "}\n"
	  "h3 span.info, h4 span.info {\n"
	  "  float: right;\n"
	  "  font-size: 100%;\n"
	  "}\n"
	  "ul.code, ul.contents, ul.subcontents {\n"
	  "  list-style-type: none;\n"
	  "  margin: 0;\n"
	  "  padding-left: 0;\n"
	  "}\n"
	  "ul.code li {\n"
	  "  margin: 0;\n"
	  "}\n"
	  "ul.contents > li {\n"
	  "  margin-top: 1em;\n"
	  "}\n"
	  "ul.contents li ul.code, ul.contents li ul.subcontents {\n"
	  "  padding-left: 2em;\n"
	  "}\n"
	  "div.body dl {\n"
	  "  margin-top: 0;\n"
	  "}\n"
	  "div.body dt {\n"
	  "  font-style: italic;\n"
	  "  margin-top: 0;\n"
	  "}\n"
	  "div.body dd {\n"
	  "  margin-bottom: 0.5em;\n"
	  "}\n"
	  "h1.title {\n"
	  "}\n"
	  "h2.title {\n"
	  "  border-bottom: solid 2px black;\n"
	  "}\n"
	  "h3.title {\n"
	  "  border-bottom: solid 2px black;\n"
	  "}\n", out);
  }

  fputs("--></style>\n"
        "</head>\n"
        "<body>\n", out);
}


/*
 * 'write_man()' - Write manpage documentation.
 */

static void
write_man(const char  *man_name,	/* I - Name of manpage */
	  const char  *section,		/* I - Section */
	  const char  *title,		/* I - Title */
	  const char  *footerfile,	/* I - Footer file */
	  const char  *headerfile,	/* I - Header file */
	  const char  *introfile,	/* I - Intro file */
	  mxml_node_t *doc)		/* I - XML documentation */
{
  int		i;			/* Looping var */
  mxml_node_t	*function,		/* Current function */
		*scut,			/* Struct/class/union/typedef */
		*arg,			/* Current argument */
		*description,		/* Description of function/var */
		*type;			/* Type for argument */
  const char	*name,			/* Name of function/type */
		*cname,			/* Class name */
		*defval,		/* Default value */
		*parent;		/* Parent class */
  int		inscope;		/* Variable/method scope */
  char		prefix;			/* Prefix character */
  time_t	curtime;		/* Current time */
  struct tm	*curdate;		/* Current date */
  char		buffer[1024];		/* String buffer */
  static const char * const scopes[] =	/* Scope strings */
		{
		  "private",
		  "protected",
		  "public"
		};


 /*
  * Standard man page...
  */

  curtime = time(NULL);
  curdate = localtime(&curtime);
  strftime(buffer, sizeof(buffer), "%x", curdate);

  printf(".TH %s %s \"%s\" \"%s\" \"%s\"\n", man_name, section ? section : "3",
         title ? title : "", buffer, title ? title : "");

 /*
  * Header...
  */

  if (headerfile)
  {
   /*
    * Use custom header...
    */

    write_file(stdout, headerfile);
  }
  else
  {
   /*
    * Use standard header...
    */

    puts(".SH NAME");
    printf("%s \\- %s\n", man_name, title ? title : man_name);
  }

 /*
  * Intro...
  */

  if (introfile)
    write_file(stdout, introfile);

 /*
  * List of classes...
  */

  if (find_public(doc, doc, "class"))
  {
    puts(".SH CLASSES");

    for (scut = find_public(doc, doc, "class");
	 scut;
	 scut = find_public(scut, doc, "class"))
    {
      cname       = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", cname);

      write_description(stdout, description, NULL, 1);

      printf(".PP\n"
             ".nf\n"
             "class %s", cname);
      if ((parent = mxmlElementGetAttr(scut, "parent")) != NULL)
        printf(" %s", parent);
      puts("\n{");

      for (i = 0; i < 3; i ++)
      {
        inscope = 0;

	for (arg = mxmlFindElement(scut, scut, "variable", "scope", scopes[i],
                        	   MXML_DESCEND_FIRST);
	     arg;
	     arg = mxmlFindElement(arg, scut, "variable", "scope", scopes[i],
                        	   MXML_NO_DESCEND))
	{
          if (!inscope)
	  {
	    inscope = 1;
	    printf("  %s:\n", scopes[i]);
	  }

	  printf("    ");
	  write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                     NULL, MXML_DESCEND_FIRST),
                        OUTPUT_MAN);
	  printf("%s;\n", mxmlElementGetAttr(arg, "name"));
	}

	for (function = mxmlFindElement(scut, scut, "function", "scope",
	                                scopes[i], MXML_DESCEND_FIRST);
	     function;
	     function = mxmlFindElement(function, scut, "function", "scope",
	                                scopes[i], MXML_NO_DESCEND))
	{
          if (!inscope)
	  {
	    inscope = 1;
	    printf("  %s:\n", scopes[i]);
	  }

          name = mxmlElementGetAttr(function, "name");

          printf("    ");

	  arg = mxmlFindElement(function, function, "returnvalue", NULL,
                        	NULL, MXML_DESCEND_FIRST);

	  if (arg)
	    write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                       NULL, MXML_DESCEND_FIRST),
                          OUTPUT_MAN);
	  else if (strcmp(cname, name) && strcmp(cname, name + 1))
	    fputs("void ", stdout);

	  printf("%s", name);

	  for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
                        	     MXML_DESCEND_FIRST), prefix = '(';
	       arg;
	       arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
                        	     MXML_NO_DESCEND), prefix = ',')
	  {
	    type = mxmlFindElement(arg, arg, "type", NULL, NULL,
	                	   MXML_DESCEND_FIRST);

	    putchar(prefix);
	    if (prefix == ',')
	      putchar(' ');

	    if (type->child)
	      write_element(stdout, doc, type, OUTPUT_MAN);
	    fputs(mxmlElementGetAttr(arg, "name"), stdout);
            if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	      printf(" %s", defval);
	  }

	  if (prefix == '(')
	    puts("(void);");
	  else
	    puts(");");
	}
      }

      puts("};\n"
           ".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

 /*
  * List of enumerations...
  */

  if (find_public(doc, doc, "enumeration"))
  {
    puts(".SH ENUMERATIONS");

    for (scut = find_public(doc, doc, "enumeration");
	 scut;
	 scut = find_public(scut, doc, "enumeration"))
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", name);

      write_description(stdout, description, NULL, 1);
      write_description(stdout, description, NULL, 0);

      for (arg = mxmlFindElement(scut, scut, "constant", NULL, NULL,
                        	 MXML_DESCEND_FIRST);
	   arg;
	   arg = mxmlFindElement(arg, scut, "constant", NULL, NULL,
                        	 MXML_NO_DESCEND))
      {
	description = mxmlFindElement(arg, arg, "description", NULL,
                                      NULL, MXML_DESCEND_FIRST);
	printf(".TP 5\n%s\n.br\n", mxmlElementGetAttr(arg, "name"));
	write_description(stdout, description, NULL, 1);
      }
    }
  }

 /*
  * List of functions...
  */

  if (find_public(doc, doc, "function"))
  {
    puts(".SH FUNCTIONS");

    for (function = find_public(doc, doc, "function");
	 function;
	 function = find_public(function, doc, "function"))
    {
      name        = mxmlElementGetAttr(function, "name");
      description = mxmlFindElement(function, function, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", name);

      write_description(stdout, description, NULL, 1);

      puts(".PP\n"
           ".nf");

      arg = mxmlFindElement(function, function, "returnvalue", NULL,
                            NULL, MXML_DESCEND_FIRST);

      if (arg)
	write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                   NULL, MXML_DESCEND_FIRST),
                      OUTPUT_MAN);
      else
	fputs("void", stdout);

      printf(" %s ", name);
      for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
                        	 MXML_DESCEND_FIRST), prefix = '(';
	   arg;
	   arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
                        	 MXML_NO_DESCEND), prefix = ',')
      {
        type = mxmlFindElement(arg, arg, "type", NULL, NULL,
	                       MXML_DESCEND_FIRST);

	printf("%c\n    ", prefix);
	if (type->child)
	  write_element(stdout, doc, type, OUTPUT_MAN);
	fputs(mxmlElementGetAttr(arg, "name"), stdout);
        if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	  printf(" %s", defval);
      }

      if (prefix == '(')
	puts("(void);");
      else
	puts("\n);");

      puts(".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

 /*
  * List of structures...
  */

  if (find_public(doc, doc, "struct"))
  {
    puts(".SH STRUCTURES");

    for (scut = find_public(doc, doc, "struct");
	 scut;
	 scut = find_public(scut, doc, "struct"))
    {
      cname       = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", cname);

      write_description(stdout, description, NULL, 1);

      printf(".PP\n"
             ".nf\n"
	     "struct %s\n{\n", cname);
      for (arg = mxmlFindElement(scut, scut, "variable", NULL, NULL,
                        	 MXML_DESCEND_FIRST);
	   arg;
	   arg = mxmlFindElement(arg, scut, "variable", NULL, NULL,
                        	 MXML_NO_DESCEND))
      {
	printf("  ");
	write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                   NULL, MXML_DESCEND_FIRST),
                      OUTPUT_MAN);
	printf("%s;\n", mxmlElementGetAttr(arg, "name"));
      }

      for (function = mxmlFindElement(scut, scut, "function", NULL, NULL,
                                      MXML_DESCEND_FIRST);
	   function;
	   function = mxmlFindElement(function, scut, "function", NULL, NULL,
                                      MXML_NO_DESCEND))
      {
        name = mxmlElementGetAttr(function, "name");

        printf("  ");

	arg = mxmlFindElement(function, function, "returnvalue", NULL,
                              NULL, MXML_DESCEND_FIRST);

	if (arg)
	  write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                     NULL, MXML_DESCEND_FIRST),
                        OUTPUT_MAN);
	else if (strcmp(cname, name) && strcmp(cname, name + 1))
	  fputs("void ", stdout);

	fputs(name, stdout);

	for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
                        	   MXML_DESCEND_FIRST), prefix = '(';
	     arg;
	     arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
                        	   MXML_NO_DESCEND), prefix = ',')
	{
	  type = mxmlFindElement(arg, arg, "type", NULL, NULL,
	                	 MXML_DESCEND_FIRST);

	  putchar(prefix);
	  if (prefix == ',')
	    putchar(' ');

	  if (type->child)
	    write_element(stdout, doc, type, OUTPUT_MAN);
	  fputs(mxmlElementGetAttr(arg, "name"), stdout);
          if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	    printf(" %s", defval);
	}

	if (prefix == '(')
	  puts("(void);");
	else
	  puts(");");
      }

      puts("};\n"
           ".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

 /*
  * List of types...
  */

  if (find_public(doc, doc, "typedef"))
  {
    puts(".SH TYPES");

    for (scut = find_public(doc, doc, "typedef");
	 scut;
	 scut = find_public(scut, doc, "typedef"))
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", name);

      write_description(stdout, description, NULL, 1);

      fputs(".PP\n"
            ".nf\n"
	    "typedef ", stdout);

      type = mxmlFindElement(scut, scut, "type", NULL, NULL,
                             MXML_DESCEND_FIRST);

      for (type = type->child; type; type = type->next)
        if (!strcmp(type->value.text.string, "("))
	  break;
	else
	{
	  if (type->value.text.whitespace)
	    putchar(' ');

          write_string(stdout, type->value.text.string, OUTPUT_MAN);
        }

      if (type)
      {
       /*
        * Output function type...
	*/

        printf(" (*%s", name);

	for (type = type->next->next; type; type = type->next)
	{
	  if (type->value.text.whitespace)
	    putchar(' ');

          write_string(stdout, type->value.text.string, OUTPUT_MAN);
        }

        puts(";");
      }
      else
	printf(" %s;\n", name);

      puts(".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

 /*
  * List of unions...
  */

  if (find_public(doc, doc, "union"))
  {
    puts(".SH UNIONS");

    for (scut = find_public(doc, doc, "union");
	 scut;
	 scut = find_public(scut, doc, "union"))
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", name);

      write_description(stdout, description, NULL, 1);

      printf(".PP\n"
             ".nf\n"
	     "union %s\n{\n", name);
      for (arg = mxmlFindElement(scut, scut, "variable", NULL, NULL,
                        	 MXML_DESCEND_FIRST);
	   arg;
	   arg = mxmlFindElement(arg, scut, "variable", NULL, NULL,
                        	 MXML_NO_DESCEND))
      {
	printf("  ");
	write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                   NULL, MXML_DESCEND_FIRST),
                      OUTPUT_MAN);
	printf("%s;\n", mxmlElementGetAttr(arg, "name"));
      }

      puts("};\n"
           ".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

 /*
  * Variables...
  */

  if (find_public(doc, doc, "variable"))
  {
    puts(".SH VARIABLES");

    for (arg = find_public(doc, doc, "variable");
	 arg;
	 arg = find_public(arg, doc, "variable"))
    {
      name        = mxmlElementGetAttr(arg, "name");
      description = mxmlFindElement(arg, arg, "description", NULL,
                                    NULL, MXML_DESCEND_FIRST);
      printf(".SS %s\n", name);

      write_description(stdout, description, NULL, 1);

      puts(".PP\n"
           ".nf");

      write_element(stdout, doc, mxmlFindElement(arg, arg, "type", NULL,
                                                 NULL, MXML_DESCEND_FIRST),
                    OUTPUT_MAN);
      fputs(mxmlElementGetAttr(arg, "name"), stdout);
      if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	printf(" %s", defval);
      puts(";\n"
           ".fi");

      write_description(stdout, description, NULL, 0);
    }
  }

  if (footerfile)
  {
   /*
    * Use custom footer...
    */

    write_file(stdout, footerfile);
  }
}


/*
 * 'write_scu()' - Write a structure, class, or union.
 */

static void
write_scu(FILE        *out,	/* I - Output file */
          mxml_node_t *doc,	/* I - Document */
          mxml_node_t *scut)	/* I - Structure, class, or union */
{
  int		i;			/* Looping var */
  mxml_node_t	*function,		/* Current function */
		*arg,			/* Current argument */
		*description,		/* Description of function/var */
		*type;			/* Type for argument */
  const char	*name,			/* Name of function/type */
		*cname,			/* Class name */
		*defval,		/* Default value */
		*parent,		/* Parent class */
		*scope;			/* Scope for variable/function */
  int		inscope,		/* Variable/method scope */
		maxscope;		/* Maximum scope */
  char		prefix;			/* Prefix character */
  static const char * const scopes[] =	/* Scope strings */
		{
		  "private",
		  "protected",
		  "public"
		};


  cname       = mxmlElementGetAttr(scut, "name");
  description = mxmlFindElement(scut, scut, "description", NULL,
				NULL, MXML_DESCEND_FIRST);

  fprintf(out, "<h3 class=\"%s\">%s<a name=\"%s\">%s</a></h3>\n",
	  scut->value.element.name, get_comment_info(description), cname,
	  cname);

  if (description)
    write_description(out, description, "p", 1);

  fprintf(out, "<p class=\"code\">%s %s", scut->value.element.name, cname);
  if ((parent = mxmlElementGetAttr(scut, "parent")) != NULL)
    fprintf(out, " %s", parent);
  fputs(" {<br>\n", out);

  maxscope = !strcmp(scut->value.element.name, "class") ? 3 : 1;

  for (i = 0; i < maxscope; i ++)
  {
    inscope = maxscope == 1;

    for (arg = mxmlFindElement(scut, scut, "variable", NULL, NULL,
			       MXML_DESCEND_FIRST);
	 arg;
	 arg = mxmlFindElement(arg, scut, "variable", NULL, NULL,
			       MXML_NO_DESCEND))
    {
      if (maxscope > 1 &&
          ((scope = mxmlElementGetAttr(arg, "scope")) == NULL ||
	   strcmp(scope, scopes[i])))
	continue;

      if (!inscope)
      {
	inscope = 1;
	fprintf(out, "&nbsp;&nbsp;%s:<br>\n", scopes[i]);
      }

      fputs("&nbsp;&nbsp;&nbsp;&nbsp;", out);
      write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
					      NULL, MXML_DESCEND_FIRST),
		    OUTPUT_HTML);
      fprintf(out, "%s;<br>\n", mxmlElementGetAttr(arg, "name"));
    }

    for (function = mxmlFindElement(scut, scut, "function", NULL, NULL,
                                    MXML_DESCEND_FIRST);
	 function;
	 function = mxmlFindElement(function, scut, "function", NULL, NULL,
	                            MXML_NO_DESCEND))
    {
      if (maxscope > 1 &&
          ((scope = mxmlElementGetAttr(arg, "scope")) == NULL ||
	   strcmp(scope, scopes[i])))
	continue;

      if (!inscope)
      {
	inscope = 1;
	fprintf(out, "&nbsp;&nbsp;%s:<br>\n", scopes[i]);
      }

      name = mxmlElementGetAttr(function, "name");

      fputs("&nbsp;&nbsp;&nbsp;&nbsp;", out);

      arg = mxmlFindElement(function, function, "returnvalue", NULL,
			    NULL, MXML_DESCEND_FIRST);

      if (arg)
	write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
						NULL, MXML_DESCEND_FIRST),
		      OUTPUT_HTML);
      else if (strcmp(cname, name) && strcmp(cname, name + 1))
	fputs("void ", out);

      fprintf(out, "<a href=\"#%s.%s\">%s</a>", cname, name, name);

      for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
				 MXML_DESCEND_FIRST), prefix = '(';
	   arg;
	   arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
				 MXML_NO_DESCEND), prefix = ',')
      {
	type = mxmlFindElement(arg, arg, "type", NULL, NULL,
			       MXML_DESCEND_FIRST);

	putc(prefix, out);
	if (prefix == ',')
	  putc(' ', out);

	if (type->child)
	  write_element(out, doc, type, OUTPUT_HTML);

	fputs(mxmlElementGetAttr(arg, "name"), out);
	if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	  fprintf(out, " %s", defval);
      }

      if (prefix == '(')
	fputs("(void);<br>\n", out);
      else
	fputs(");<br>\n", out);
    }
  }

  fputs("};</p>\n"
	"<h4 class=\"members\">Members</h4>\n"
	"<dl>\n", out);

  for (arg = mxmlFindElement(scut, scut, "variable", NULL, NULL,
			     MXML_DESCEND_FIRST);
       arg;
       arg = mxmlFindElement(arg, scut, "variable", NULL, NULL,
			     MXML_NO_DESCEND))
  {
    description = mxmlFindElement(arg, arg, "description", NULL,
				  NULL, MXML_DESCEND_FIRST);

    fprintf(out, "<dt>%s %s</dt>\n",
	    mxmlElementGetAttr(arg, "name"), get_comment_info(description));

    write_description(out, description, "dd", 1);
    write_description(out, description, "dd", 0);
  }

  fputs("</dl>\n", out);

  for (function = mxmlFindElement(scut, scut, "function", NULL, NULL,
				  MXML_DESCEND_FIRST);
       function;
       function = mxmlFindElement(function, scut, "function", NULL, NULL,
				  MXML_NO_DESCEND))
  {
    write_function(out, doc, function, 4);
  }
}


/*
 * 'write_string()' - Write a string, quoting HTML special chars as needed.
 */

static void
write_string(FILE       *out,		/* I - Output file */
             const char *s,		/* I - String to write */
             int        mode)		/* I - Output mode */
{
  switch (mode)
  {
    case OUTPUT_HTML :
    case OUTPUT_XML :
        while (*s)
        {
          if (*s == '&')
            fputs("&amp;", out);
          else if (*s == '<')
            fputs("&lt;", out);
          else if (*s == '>')
            fputs("&gt;", out);
          else if (*s == '\"')
            fputs("&quot;", out);
          else if (*s & 128)
          {
           /*
            * Convert UTF-8 to Unicode constant...
            */

            int	ch;			/* Unicode character */


            ch = *s & 255;

            if ((ch & 0xe0) == 0xc0)
            {
              ch = ((ch & 0x1f) << 6) | (s[1] & 0x3f);
	      s ++;
            }
            else if ((ch & 0xf0) == 0xe0)
            {
              ch = ((((ch * 0x0f) << 6) | (s[1] & 0x3f)) << 6) | (s[2] & 0x3f);
	      s += 2;
            }

            if (ch == 0xa0)
            {
             /*
              * Handle non-breaking space as-is...
	      */

              fputs("&nbsp;", out);
            }
            else
              fprintf(out, "&#x%x;", ch);
          }
          else
            putc(*s, out);

          s ++;
        }
        break;

    case OUTPUT_MAN :
        while (*s)
        {
          if (*s == '\\' || *s == '-')
            putc('\\', out);

          putc(*s++, out);
        }
        break;
  }
}


/*
 * 'write_toc()' - Write a table-of-contents.
 */

static void
write_toc(FILE        *out,		/* I - Output file */
          mxml_node_t *doc,		/* I - Document */
          const char  *introfile,	/* I - Introduction file */
	  const char  *target,		/* I - Target name */
	  int         xml)		/* I - Write XML nodes? */
{
  FILE		*fp;			/* Intro file */
  mxml_node_t	*function,		/* Current function */
		*scut,			/* Struct/class/union/typedef */
		*arg,			/* Current argument */
		*description;		/* Description of function/var */
  const char	*name,			/* Name of function/type */
		*targetattr;		/* Target attribute, if any */
  int		xmlid = 1;		/* Current XML node ID */


 /*
  * If target is set, it is the frame file that contains the body.
  * Otherwise, we are creating a single-file...
  */

  if (target)
    targetattr = " target=\"body\"";
  else
    targetattr = "";

 /*
  * The table-of-contents is a nested unordered list.  Start by
  * reading any intro file to see if there are any headings there.
  */

  if (!xml)
    fputs("<h2 class=\"title\">Contents</h2>\n"
          "<ul class=\"contents\">\n", out);

  if (introfile && (fp = fopen(introfile, "r")) != NULL)
  {
    char	line[8192],		/* Line from file */
		*ptr,			/* Pointer in line */
		*end,			/* End of line */
		*anchor,		/* Anchor name */
		quote,			/* Quote character for value */
		level = '2',		/* Current heading level */
		newlevel;		/* New heading level */
    int		inelement;		/* In an element? */


    while (fgets(line, sizeof(line), fp))
    {
     /*
      * See if this line has a heading...
      */

      if ((ptr = strstr(line, "<h")) == NULL &&
          (ptr = strstr(line, "<H")) == NULL)
	continue;

      if (ptr[2] != '2' && ptr[2] != '3')
        continue;

      newlevel = ptr[2];

     /*
      * Make sure we have the whole heading...
      */

      while (!strstr(line, "</h") && !strstr(line, "</H"))
      {
        end = line + strlen(line);

	if (end == (line + sizeof(line) - 1) ||
	    !fgets(end, (int)(sizeof(line) - (end - line)), fp))
	  break;
      }

     /*
      * Convert newlines and tabs to spaces...
      */

      for (ptr = line; *ptr; ptr ++)
        if (isspace(*ptr & 255))
	  *ptr = ' ';

     /*
      * Find the anchor and text...
      */

      for (ptr = strchr(line, '<'); ptr; ptr = strchr(ptr + 1, '<'))
        if (!strncmp(ptr, "<A NAME=", 8) || !strncmp(ptr, "<a name=", 8))
	  break;

      if (!ptr)
        continue;

      ptr += 8;
      inelement = 1;

      if (*ptr == '\'' || *ptr == '\"')
      {
       /*
        * Quoted anchor...
	*/

        quote  = *ptr++;
	anchor = ptr;

	while (*ptr && *ptr != quote)
	  ptr ++;

        if (!*ptr)
	  continue;

        *ptr++ = '\0';
      }
      else
      {
       /*
        * Non-quoted anchor...
	*/

        anchor = ptr;

	while (*ptr && *ptr != '>' && !isspace(*ptr & 255))
	  ptr ++;

        if (!*ptr)
	  continue;

        if (*ptr == '>')
	  inelement = 0;

	*ptr++ = '\0';
      }

     /*
      * Write text until we see "</A>"...
      */

      if (xml)
      {
	if (newlevel < level)
	  fputs("</Node>\n"
		"</Subnodes></Node>\n", out);
	else if (newlevel > level && newlevel == '3')
	  fputs("<Subnodes>\n", out);
	else if (xmlid > 1)
	  fputs("</Node>\n", out);

	level = newlevel;

	fprintf(out, "<Node id=\"%d\">\n"
                     "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>", xmlid ++, anchor);

	quote = 0;

	while (*ptr)
	{
	  if (inelement)
	  {
	    if (*ptr == quote)
	      quote = 0;
	    else if (*ptr == '>')
	      inelement = 0;
	    else if (*ptr == '\'' || *ptr == '\"')
	      quote = *ptr;
	  }
	  else if (*ptr == '<')
	  {
	    if (!strncmp(ptr, "</A>", 4) || !strncmp(ptr, "</a>", 4))
	      break;

	    inelement = 1;
	  }
	  else
	    putc(*ptr, out);

	  ptr ++;
	}

	fputs("</Name>\n", out);
      }
      else
      {
	if (newlevel < level)
	  fputs("</li>\n"
		"</ul></li>\n", out);
	else if (newlevel > level)
	  fputs("<ul class=\"subcontents\">\n", out);
	else if (xmlid > 1)
	  fputs("</li>\n", out);

	level = newlevel;
	xmlid ++;

	fprintf(out, "%s<li><a href=\"%s#%s\"%s>", level > '2' ? "\t" : "",
	        target ? target : "", anchor, targetattr);

	quote = 0;

	while (*ptr)
	{
	  if (inelement)
	  {
	    if (*ptr == quote)
	      quote = 0;
	    else if (*ptr == '>')
	      inelement = 0;
	    else if (*ptr == '\'' || *ptr == '\"')
	      quote = *ptr;
	  }
	  else if (*ptr == '<')
	  {
	    if (!strncmp(ptr, "</A>", 4) || !strncmp(ptr, "</a>", 4))
	      break;

	    inelement = 1;
	  }
	  else
	    putc(*ptr, out);

	  ptr ++;
	}

	fputs("</a>", out);
      }
    }

    if (level > '1')
    {
      if (xml)
      {
	fputs("</Node>\n", out);

	if (level == '3')
	  fputs("</Subnodes></Node>\n", out);
      }
      else
      {
	fputs("</li>\n", out);

	if (level == '3')
	  fputs("</ul></li>\n", out);
      }
    }

    fclose(fp);
  }

 /*
  * Next the classes...
  */

  if ((scut = find_public(doc, doc, "class")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>CLASSES</Anchor>\n"
		   "<Name>Classes</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#CLASSES\"%s>Classes</a>"
		   "<ul class=\"code\">\n",
	      target ? target : "", targetattr);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      scut = find_public(scut, doc, "class");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Functions...
  */

  if ((function = find_public(doc, doc, "function")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>FUNCTIONS</Anchor>\n"
		   "<Name>Functions</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#FUNCTIONS\"%s>Functions</a>"
		   "<ul class=\"code\">\n", target ? target : "", targetattr);

    while (function)
    {
      name        = mxmlElementGetAttr(function, "name");
      description = mxmlFindElement(function, function, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      function = find_public(function, doc, "function");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Data types...
  */

  if ((scut = find_public(doc, doc, "typedef")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>TYPES</Anchor>\n"
		   "<Name>Data Types</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#TYPES\"%s>Data Types</a>"
		   "<ul class=\"code\">\n", target ? target : "", targetattr);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      scut = find_public(scut, doc, "typedef");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Structures...
  */

  if ((scut = find_public(doc, doc, "struct")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>STRUCTURES</Anchor>\n"
		   "<Name>Structures</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#STRUCTURES\"%s>Structures</a>"
		   "<ul class=\"code\">\n", target ? target : "", targetattr);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      scut = find_public(scut, doc, "struct");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Unions...
  */

  if ((scut = find_public(doc, doc, "union")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>UNIONS</Anchor>\n"
		   "<Name>Unions</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out,
              "<li><a href=\"%s#UNIONS\"%s>Unions</a><ul class=\"code\">\n",
	      target ? target : "", targetattr);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      scut = find_public(scut, doc, "union");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Globals variables...
  */

  if ((arg = find_public(doc, doc, "variable")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>VARIABLES</Anchor>\n"
		   "<Name>Variables</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#VARIABLES\"%s>Variables</a>"
		   "<ul class=\"code\">\n", target ? target : "", targetattr);

    while (arg)
    {
      name        = mxmlElementGetAttr(arg, "name");
      description = mxmlFindElement(arg, arg, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      arg = find_public(arg, doc, "variable");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Enumerations/constants...
  */

  if ((scut = find_public(doc, doc, "enumeration")) != NULL)
  {
    if (xml)
      fprintf(out, "<Node id=\"%d\">\n"
		   "<Path>Documentation/index.html</Path>\n"
	           "<Anchor>ENUMERATIONS</Anchor>\n"
		   "<Name>Constants</Name>\n"
		   "<Subnodes>\n", xmlid ++);
    else
      fprintf(out, "<li><a href=\"%s#ENUMERATIONS\"%s>Constants</a>"
		   "<ul class=\"code\">\n", target ? target : "", targetattr);

    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      if (xml)
      {
	fprintf(out, "<Node id=\"%d\">\n"
	             "<Path>Documentation/index.html</Path>\n"
	             "<Anchor>%s</Anchor>\n"
		     "<Name>%s</Name>\n"
		     "</Node>\n", xmlid ++, name, name);
      }
      else
      {
	fprintf(out, "\t<li><a href=\"%s#%s\"%s title=\"",
		target ? target : "", name, targetattr);
	write_description(out, description, "", 1);
	fprintf(out, "\">%s</a></li>\n", name);
      }

      scut = find_public(scut, doc, "enumeration");
    }

    if (xml)
      fputs("</Subnodes></Node>\n", out);
    else
      fputs("</ul></li>\n", out);
  }

 /*
  * Close out the HTML table-of-contents list as needed...
  */

  if (!xml)
    fputs("</ul>\n", out);
}


/*
 * 'write_tokens()' - Write <Token> nodes for all APIs.
 */

static void
write_tokens(FILE        *out,		/* I - Output file */
             mxml_node_t *doc,		/* I - Document */
	     const char  *path)		/* I - Path to help file */
{
  mxml_node_t	*function,		/* Current function */
		*scut,			/* Struct/class/union/typedef */
		*arg,			/* Current argument */
		*description,		/* Description of function/var */
		*type,			/* Type node */
		*node;			/* Current child node */
  const char	*name,			/* Name of function/type */
		*cename,		/* Current class/enum name */
		*defval;		/* Default value for argument */
  char		prefix;			/* Prefix for declarations */


 /*
  * Classes...
  */

  if ((scut = find_public(doc, doc, "class")) != NULL)
  {
    while (scut)
    {
      cename      = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/cpp/cl/%s</TokenIdentifier>\n"
		   "<Abstract>", path, cename, cename);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      if ((function = find_public(scut, scut, "function")) != NULL)
      {
	while (function)
	{
	  name        = mxmlElementGetAttr(function, "name");
	  description = mxmlFindElement(function, function, "description",
					NULL, NULL, MXML_DESCEND_FIRST);

	  fprintf(out, "<Token>\n"
		       "<Path>Documentation/%s</Path>\n"
		       "<Anchor>%s.%s</Anchor>\n"
		       "<TokenIdentifier>//apple_ref/cpp/clm/%s/%s", path,
		  cename, name, cename, name);

	  arg = mxmlFindElement(function, function, "returnvalue", NULL,
				NULL, MXML_DESCEND_FIRST);

	  if (arg && (type = mxmlFindElement(arg, arg, "type", NULL,
					     NULL, MXML_DESCEND_FIRST)) != NULL)
          {
	    for (node = type->child; node; node = node->next)
	      fputs(node->value.text.string, out);
	  }
	  else if (strcmp(cename, name) && strcmp(cename, name + 1))
	    fputs("void", out);

	  fputs("/", out);

	  for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
				     MXML_DESCEND_FIRST), prefix = '(';
	       arg;
	       arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
				     MXML_NO_DESCEND), prefix = ',')
	  {
	    type = mxmlFindElement(arg, arg, "type", NULL, NULL,
				   MXML_DESCEND_FIRST);

	    putc(prefix, out);

	    for (node = type->child; node; node = node->next)
	      fputs(node->value.text.string, out);

	    fputs(mxmlElementGetAttr(arg, "name"), out);
	  }

	  if (prefix == '(')
	    fputs("(void", out);

	  fputs(")</TokenIdentifier>\n"
	        "<Abstract>", out);
	  write_description(out, description, "", 1);
	  fputs("</Abstract>\n"
		"<Declaration>", out);

	  arg = mxmlFindElement(function, function, "returnvalue", NULL,
				NULL, MXML_DESCEND_FIRST);

	  if (arg)
	    write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
						    NULL, MXML_DESCEND_FIRST),
			  OUTPUT_XML);
	  else if (strcmp(cename, name) && strcmp(cename, name + 1))
	    fputs("void ", out);

	  fputs(name, out);

	  for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
				     MXML_DESCEND_FIRST), prefix = '(';
	       arg;
	       arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
				     MXML_NO_DESCEND), prefix = ',')
	  {
	    type = mxmlFindElement(arg, arg, "type", NULL, NULL,
				   MXML_DESCEND_FIRST);

	    putc(prefix, out);
	    if (prefix == ',')
	      putc(' ', out);

	    if (type->child)
	      write_element(out, doc, type, OUTPUT_XML);

	    fputs(mxmlElementGetAttr(arg, "name"), out);
	    if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	      fprintf(out, " %s", defval);
	  }

	  if (prefix == '(')
	    fputs("(void);", out);
	  else
	    fputs(");", out);

	  fputs("</Declaration>\n"
		"</Token>\n", out);

	  function = find_public(function, doc, "function");
	}
      }
      scut = find_public(scut, doc, "class");
    }
  }

 /*
  * Functions...
  */

  if ((function = find_public(doc, doc, "function")) != NULL)
  {
    while (function)
    {
      name        = mxmlElementGetAttr(function, "name");
      description = mxmlFindElement(function, function, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/func/%s</TokenIdentifier>\n"
		   "<Abstract>", path, name, name);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "<Declaration>", out);

      arg = mxmlFindElement(function, function, "returnvalue", NULL,
			    NULL, MXML_DESCEND_FIRST);

      if (arg)
	write_element(out, doc, mxmlFindElement(arg, arg, "type", NULL,
						NULL, MXML_DESCEND_FIRST),
		      OUTPUT_XML);
      else // if (strcmp(cname, name) && strcmp(cname, name + 1))
	fputs("void ", out);

      fputs(name, out);

      for (arg = mxmlFindElement(function, function, "argument", NULL, NULL,
				 MXML_DESCEND_FIRST), prefix = '(';
	   arg;
	   arg = mxmlFindElement(arg, function, "argument", NULL, NULL,
				 MXML_NO_DESCEND), prefix = ',')
      {
	type = mxmlFindElement(arg, arg, "type", NULL, NULL,
			       MXML_DESCEND_FIRST);

	putc(prefix, out);
	if (prefix == ',')
	  putc(' ', out);

	if (type->child)
	  write_element(out, doc, type, OUTPUT_XML);

	fputs(mxmlElementGetAttr(arg, "name"), out);
	if ((defval = mxmlElementGetAttr(arg, "default")) != NULL)
	  fprintf(out, " %s", defval);
      }

      if (prefix == '(')
	fputs("(void);", out);
      else
	fputs(");", out);

      fputs("</Declaration>\n"
            "</Token>\n", out);

      function = find_public(function, doc, "function");
    }
  }

 /*
  * Data types...
  */

  if ((scut = find_public(doc, doc, "typedef")) != NULL)
  {
    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/tdef/%s</TokenIdentifier>\n"
		   "<Abstract>", path, name, name);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      scut = find_public(scut, doc, "typedef");
    }
  }

 /*
  * Structures...
  */

  if ((scut = find_public(doc, doc, "struct")) != NULL)
  {
    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/tag/%s</TokenIdentifier>\n"
		   "<Abstract>", path, name, name);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      scut = find_public(scut, doc, "struct");
    }
  }

 /*
  * Unions...
  */

  if ((scut = find_public(doc, doc, "union")) != NULL)
  {
    while (scut)
    {
      name        = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/tag/%s</TokenIdentifier>\n"
		   "<Abstract>", path, name, name);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      scut = find_public(scut, doc, "union");
    }
  }

 /*
  * Globals variables...
  */

  if ((arg = find_public(doc, doc, "variable")) != NULL)
  {
    while (arg)
    {
      name        = mxmlElementGetAttr(arg, "name");
      description = mxmlFindElement(arg, arg, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/data/%s</TokenIdentifier>\n"
		   "<Abstract>", path, name, name);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      arg = find_public(arg, doc, "variable");
    }
  }

 /*
  * Enumerations/constants...
  */

  if ((scut = find_public(doc, doc, "enumeration")) != NULL)
  {
    while (scut)
    {
      cename      = mxmlElementGetAttr(scut, "name");
      description = mxmlFindElement(scut, scut, "description",
				    NULL, NULL, MXML_DESCEND_FIRST);

      fprintf(out, "<Token>\n"
		   "<Path>Documentation/%s</Path>\n"
		   "<Anchor>%s</Anchor>\n"
		   "<TokenIdentifier>//apple_ref/c/tag/%s</TokenIdentifier>\n"
		   "<Abstract>", path, cename, cename);
      write_description(out, description, "", 1);
      fputs("</Abstract>\n"
            "</Token>\n", out);

      for (arg = mxmlFindElement(scut, scut, "constant", NULL, NULL,
                        	 MXML_DESCEND_FIRST);
	   arg;
	   arg = mxmlFindElement(arg, scut, "constant", NULL, NULL,
                        	 MXML_NO_DESCEND))
      {
        name        = mxmlElementGetAttr(arg, "name");
	description = mxmlFindElement(arg, arg, "description", NULL,
                                      NULL, MXML_DESCEND_FIRST);
	fprintf(out, "<Token>\n"
		     "<Path>Documentation/%s</Path>\n"
		     "<Anchor>%s</Anchor>\n"
		     "<TokenIdentifier>//apple_ref/c/econst/%s</TokenIdentifier>\n"
		     "<Abstract>", path, cename, name);
	write_description(out, description, "", 1);
	fputs("</Abstract>\n"
	      "</Token>\n", out);
      }

      scut = find_public(scut, doc, "enumeration");
    }
  }
}


/*
 * 'ws_cb()' - Whitespace callback for saving.
 */

static const char *			/* O - Whitespace string or NULL for none */
ws_cb(mxml_node_t *node,		/* I - Element node */
      int         where)		/* I - Where value */
{
  const char *name;			/* Name of element */
  int	depth;				/* Depth of node */
  static const char *spaces = "                                        ";
					/* Whitespace (40 spaces) for indent */


  name = node->value.element.name;

  switch (where)
  {
    case MXML_WS_BEFORE_CLOSE :
        if (strcmp(name, "argument") &&
	    strcmp(name, "class") &&
	    strcmp(name, "constant") &&
	    strcmp(name, "enumeration") &&
	    strcmp(name, "function") &&
	    strcmp(name, "mxmldoc") &&
	    strcmp(name, "namespace") &&
	    strcmp(name, "returnvalue") &&
	    strcmp(name, "struct") &&
	    strcmp(name, "typedef") &&
	    strcmp(name, "union") &&
	    strcmp(name, "variable"))
	  return (NULL);

	for (depth = -4; node; node = node->parent, depth += 2);
	if (depth > 40)
	  return (spaces);
	else if (depth < 2)
	  return (NULL);
	else
	  return (spaces + 40 - depth);

    case MXML_WS_AFTER_CLOSE :
	return ("\n");

    case MXML_WS_BEFORE_OPEN :
	for (depth = -4; node; node = node->parent, depth += 2);
	if (depth > 40)
	  return (spaces);
	else if (depth < 2)
	  return (NULL);
	else
	  return (spaces + 40 - depth);

    default :
    case MXML_WS_AFTER_OPEN :
        if (strcmp(name, "argument") &&
	    strcmp(name, "class") &&
	    strcmp(name, "constant") &&
	    strcmp(name, "enumeration") &&
	    strcmp(name, "function") &&
	    strcmp(name, "mxmldoc") &&
	    strcmp(name, "namespace") &&
	    strcmp(name, "returnvalue") &&
	    strcmp(name, "struct") &&
	    strcmp(name, "typedef") &&
	    strcmp(name, "union") &&
	    strcmp(name, "variable") &&
	    strncmp(name, "?xml", 4))
	  return (NULL);
	else
          return ("\n");
  }
}


/*
 * End of "$Id: mxmldoc.c 451 2014-01-04 21:50:06Z msweet $".
 */
