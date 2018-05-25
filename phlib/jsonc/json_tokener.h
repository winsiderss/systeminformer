/*
 * $Id: json_tokener.h,v 1.10 2006/07/25 03:24:50 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

/**
 * @file
 * @brief Methods to parse an input string into a tree of json_object objects.
 */
#ifndef _json_tokener_h_
#define _json_tokener_h_

#include <stddef.h>
#include "json_object.h"

#ifdef __cplusplus
extern "C" {
#endif

enum json_tokener_error {
  json_tokener_success,
  json_tokener_continue,
  json_tokener_error_depth,
  json_tokener_error_parse_eof,
  json_tokener_error_parse_unexpected,
  json_tokener_error_parse_null,
  json_tokener_error_parse_boolean,
  json_tokener_error_parse_number,
  json_tokener_error_parse_array,
  json_tokener_error_parse_object_key_name,
  json_tokener_error_parse_object_key_sep,
  json_tokener_error_parse_object_value_sep,
  json_tokener_error_parse_string,
  json_tokener_error_parse_comment,
  json_tokener_error_size
};

enum json_tokener_state {
  json_tokener_state_eatws,
  json_tokener_state_start,
  json_tokener_state_finish,
  json_tokener_state_null,
  json_tokener_state_comment_start,
  json_tokener_state_comment,
  json_tokener_state_comment_eol,
  json_tokener_state_comment_end,
  json_tokener_state_string,
  json_tokener_state_string_escape,
  json_tokener_state_escape_unicode,
  json_tokener_state_boolean,
  json_tokener_state_number,
  json_tokener_state_array,
  json_tokener_state_array_add,
  json_tokener_state_array_sep,
  json_tokener_state_object_field_start,
  json_tokener_state_object_field,
  json_tokener_state_object_field_end,
  json_tokener_state_object_value,
  json_tokener_state_object_value_add,
  json_tokener_state_object_sep,
  json_tokener_state_array_after_sep,
  json_tokener_state_object_field_start_after_sep,
  json_tokener_state_inf
};

struct json_tokener_srec
{
  enum json_tokener_state state, saved_state;
  struct json_object *obj;
  struct json_object *current;
  char *obj_field_name;
};

#define JSON_TOKENER_DEFAULT_DEPTH 32

struct json_tokener
{
  char *str;
  struct printbuf *pb;
  int max_depth, depth, is_double, st_pos, char_offset;
  enum json_tokener_error err;
  unsigned int ucs_char;
  char quote_char;
  struct json_tokener_srec *stack;
  int flags;
};
/**
 * @deprecated Unused in json-c code
 */
typedef struct json_tokener json_tokener;

/**
 * Be strict when parsing JSON input.  Use caution with
 * this flag as what is considered valid may become more
 * restrictive from one release to the next, causing your
 * code to fail on previously working input.
 *
 * This flag is not set by default.
 *
 * @see json_tokener_set_flags()
 */
#define JSON_TOKENER_STRICT  0x01

/**
 * Given an error previously returned by json_tokener_get_error(),
 * return a human readable description of the error.
 *
 * @return a generic error message is returned if an invalid error value is provided.
 */
const char *json_tokener_error_desc(enum json_tokener_error jerr);

/**
 * Retrieve the error caused by the last call to json_tokener_parse_ex(),
 * or json_tokener_success if there is no error.
 *
 * When parsing a JSON string in pieces, if the tokener is in the middle
 * of parsing this will return json_tokener_continue.
 *
 * See also json_tokener_error_desc().
 */
JSON_EXPORT enum json_tokener_error json_tokener_get_error(struct json_tokener *tok);

JSON_EXPORT struct json_tokener* json_tokener_new(void);
JSON_EXPORT struct json_tokener* json_tokener_new_ex(int depth);
JSON_EXPORT void json_tokener_free(struct json_tokener *tok);
JSON_EXPORT void json_tokener_reset(struct json_tokener *tok);
JSON_EXPORT struct json_object* json_tokener_parse(const char *str);
JSON_EXPORT struct json_object* json_tokener_parse_verbose(const char *str, enum json_tokener_error *error);

/**
 * Set flags that control how parsing will be done.
 */
JSON_EXPORT void json_tokener_set_flags(struct json_tokener *tok, int flags);

/**
 * Parse a string and return a non-NULL json_object if a valid JSON value
 * is found.  The string does not need to be a JSON object or array;
 * it can also be a string, number or boolean value.
 *
 * A partial JSON string can be parsed.  If the parsing is incomplete,
 * NULL will be returned and json_tokener_get_error() will return
 * json_tokener_continue.
 * json_tokener_parse_ex() can then be called with additional bytes in str
 * to continue the parsing.
 *
 * If json_tokener_parse_ex() returns NULL and the error is anything other than
 * json_tokener_continue, a fatal error has occurred and parsing must be
 * halted.  Then, the tok object must not be reused until json_tokener_reset() is
 * called.
 *
 * When a valid JSON value is parsed, a non-NULL json_object will be
 * returned.  Also, json_tokener_get_error() will return json_tokener_success.
 * Be sure to check the type with json_object_is_type() or
 * json_object_get_type() before using the object.
 *
 * @b XXX this shouldn't use internal fields:
 * Trailing characters after the parsed value do not automatically cause an
 * error.  It is up to the caller to decide whether to treat this as an
 * error or to handle the additional characters, perhaps by parsing another
 * json value starting from that point.
 *
 * Extra characters can be detected by comparing the tok->char_offset against
 * the length of the last len parameter passed in.
 *
 * The tokener does \b not maintain an internal buffer so the caller is
 * responsible for calling json_tokener_parse_ex with an appropriate str
 * parameter starting with the extra characters.
 *
 * This interface is presently not 64-bit clean due to the int len argument
 * so the function limits the maximum string size to INT32_MAX (2GB).
 * If the function is called with len == -1 then strlen is called to check
 * the string length is less than INT32_MAX (2GB)
 *
 * Example:
 * @code
json_object *jobj = NULL;
const char *mystring = NULL;
int stringlen = 0;
enum json_tokener_error jerr;
do {
	mystring = ...  // get JSON string, e.g. read from file, etc...
	stringlen = strlen(mystring);
	jobj = json_tokener_parse_ex(tok, mystring, stringlen);
} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
if (jerr != json_tokener_success)
{
	fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
	// Handle errors, as appropriate for your application.
}
if (tok->char_offset < stringlen) // XXX shouldn't access internal fields
{
	// Handle extra characters after parsed object as desired.
	// e.g. issue an error, parse another object from that point, etc...
}
// Success, use jobj here.

@endcode
 *
 * @param tok a json_tokener previously allocated with json_tokener_new()
 * @param str an string with any valid JSON expression, or portion of.  This does not need to be null terminated.
 * @param len the length of str
 */
JSON_EXPORT struct json_object* json_tokener_parse_ex(struct json_tokener *tok,
						 const char *str, int len);

#ifdef __cplusplus
}
#endif

#endif
