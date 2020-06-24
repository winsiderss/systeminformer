/*
 * $Id: json_tokener.c,v 1.20 2006/07/25 03:24:50 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 *
 * Copyright (c) 2008-2009 Yahoo! Inc.  All rights reserved.
 * The copyrights to the contents of this file are licensed under the MIT License
 * (http://www.opensource.org/licenses/mit-license.php)
 */

#include "config.h"

#include "math_compat.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arraylist.h"
#include "debug.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_object_private.h"
#include "json_tokener.h"
#include "json_util.h"
#include "printbuf.h"
#include "strdup_compat.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
#endif

#define jt_hexdigit(x) (((x) <= '9') ? (x) - '0' : ((x)&7) + 9)

#if !HAVE_STRNCASECMP && defined(_MSC_VER)
/* MSC has the version as _strnicmp */
#define strncasecmp _strnicmp
#elif !HAVE_STRNCASECMP
#error You do not have strncasecmp on your system.
#endif /* HAVE_STRNCASECMP */

/* Use C99 NAN by default; if not available, nan("") should work too. */
#ifndef NAN
#define NAN nan("")
#endif /* !NAN */

static const char json_null_str[] = "null";
static const int json_null_str_len = sizeof(json_null_str) - 1;
static const char json_inf_str[] = "Infinity";
static const char json_inf_str_lower[] = "infinity";
static const unsigned int json_inf_str_len = sizeof(json_inf_str) - 1;
static const char json_nan_str[] = "NaN";
static const int json_nan_str_len = sizeof(json_nan_str) - 1;
static const char json_true_str[] = "true";
static const int json_true_str_len = sizeof(json_true_str) - 1;
static const char json_false_str[] = "false";
static const int json_false_str_len = sizeof(json_false_str) - 1;

/* clang-format off */
static const char *json_tokener_errors[] = {
	"success",
	"continue",
	"nesting too deep",
	"unexpected end of data",
	"unexpected character",
	"null expected",
	"boolean expected",
	"number expected",
	"array value separator ',' expected",
	"quoted object property name expected",
	"object property name separator ':' expected",
	"object value separator ',' expected",
	"invalid string sequence",
	"expected comment",
	"invalid utf-8 string",
	"buffer size overflow"
};
/* clang-format on */

/**
 * validete the utf-8 string in strict model.
 * if not utf-8 format, return err.
 */
static json_bool json_tokener_validate_utf8(const char c, unsigned int *nBytes);

const char *json_tokener_error_desc(enum json_tokener_error jerr)
{
	int jerr_int = (int)jerr;
	if (jerr_int < 0 ||
	    jerr_int >= (int)(sizeof(json_tokener_errors) / sizeof(json_tokener_errors[0])))
		return "Unknown error, "
		       "invalid json_tokener_error value passed to json_tokener_error_desc()";
	return json_tokener_errors[jerr];
}

enum json_tokener_error json_tokener_get_error(struct json_tokener *tok)
{
	return tok->err;
}

/* Stuff for decoding unicode sequences */
#define IS_HIGH_SURROGATE(uc) (((uc)&0xFC00) == 0xD800)
#define IS_LOW_SURROGATE(uc) (((uc)&0xFC00) == 0xDC00)
#define DECODE_SURROGATE_PAIR(hi, lo) ((((hi)&0x3FF) << 10) + ((lo)&0x3FF) + 0x10000)
static unsigned char utf8_replacement_char[3] = {0xEF, 0xBF, 0xBD};

struct json_tokener *json_tokener_new_ex(int depth)
{
	struct json_tokener *tok;

	tok = (struct json_tokener *)calloc(1, sizeof(struct json_tokener));
	if (!tok)
		return NULL;
	tok->stack = (struct json_tokener_srec *)calloc(depth, sizeof(struct json_tokener_srec));
	if (!tok->stack)
	{
		free(tok);
		return NULL;
	}
	tok->pb = printbuf_new();
	tok->max_depth = depth;
	json_tokener_reset(tok);
	return tok;
}

struct json_tokener *json_tokener_new(void)
{
	return json_tokener_new_ex(JSON_TOKENER_DEFAULT_DEPTH);
}

void json_tokener_free(struct json_tokener *tok)
{
	json_tokener_reset(tok);
	if (tok->pb)
		printbuf_free(tok->pb);
	free(tok->stack);
	free(tok);
}

static void json_tokener_reset_level(struct json_tokener *tok, int depth)
{
	tok->stack[depth].state = json_tokener_state_eatws;
	tok->stack[depth].saved_state = json_tokener_state_start;
	json_object_put(tok->stack[depth].current);
	tok->stack[depth].current = NULL;
	free(tok->stack[depth].obj_field_name);
	tok->stack[depth].obj_field_name = NULL;
}

void json_tokener_reset(struct json_tokener *tok)
{
	int i;
	if (!tok)
		return;

	for (i = tok->depth; i >= 0; i--)
		json_tokener_reset_level(tok, i);
	tok->depth = 0;
	tok->err = json_tokener_success;
}

struct json_object *json_tokener_parse(const char *str)
{
	enum json_tokener_error jerr_ignored;
	struct json_object *obj;
	obj = json_tokener_parse_verbose(str, &jerr_ignored);
	return obj;
}

struct json_object *json_tokener_parse_verbose(const char *str, enum json_tokener_error *error)
{
	struct json_tokener *tok;
	struct json_object *obj;

	tok = json_tokener_new();
	if (!tok)
		return NULL;
	obj = json_tokener_parse_ex(tok, str, -1);
	*error = tok->err;
	if (tok->err != json_tokener_success)
	{
		if (obj != NULL)
			json_object_put(obj);
		obj = NULL;
	}

	json_tokener_free(tok);
	return obj;
}

#define state tok->stack[tok->depth].state
#define saved_state tok->stack[tok->depth].saved_state
#define current tok->stack[tok->depth].current
#define obj_field_name tok->stack[tok->depth].obj_field_name

/* Optimization:
 * json_tokener_parse_ex() consumed a lot of CPU in its main loop,
 * iterating character-by character.  A large performance boost is
 * achieved by using tighter loops to locally handle units such as
 * comments and strings.  Loops that handle an entire token within
 * their scope also gather entire strings and pass them to
 * printbuf_memappend() in a single call, rather than calling
 * printbuf_memappend() one char at a time.
 *
 * PEEK_CHAR() and ADVANCE_CHAR() macros are used for code that is
 * common to both the main loop and the tighter loops.
 */

/* PEEK_CHAR(dest, tok) macro:
 *   Peeks at the current char and stores it in dest.
 *   Returns 1 on success, sets tok->err and returns 0 if no more chars.
 *   Implicit inputs:  str, len vars
 */
#define PEEK_CHAR(dest, tok)                                                 \
	(((tok)->char_offset == len)                                         \
	     ? (((tok)->depth == 0 && state == json_tokener_state_eatws &&   \
	         saved_state == json_tokener_state_finish)                   \
	            ? (((tok)->err = json_tokener_success), 0)               \
	            : (((tok)->err = json_tokener_continue), 0))             \
	     : (((tok->flags & JSON_TOKENER_VALIDATE_UTF8) &&                \
	         (!json_tokener_validate_utf8(*str, nBytesp)))               \
	            ? ((tok->err = json_tokener_error_parse_utf8_string), 0) \
	            : (((dest) = *str), 1)))

/* ADVANCE_CHAR() macro:
 *   Increments str & tok->char_offset.
 *   For convenience of existing conditionals, returns the old value of c (0 on eof)
 *   Implicit inputs:  c var
 */
#define ADVANCE_CHAR(str, tok) (++(str), ((tok)->char_offset)++, c)

/* End optimization macro defs */

struct json_object *json_tokener_parse_ex(struct json_tokener *tok, const char *str, int len)
{
	struct json_object *obj = NULL;
	char c = '\1';
	unsigned int nBytes = 0;
	unsigned int *nBytesp = &nBytes;

#ifdef HAVE_USELOCALE
	locale_t oldlocale = uselocale(NULL);
	locale_t newloc;
#elif defined(HAVE_SETLOCALE)
	char *oldlocale = NULL;
#endif

	tok->char_offset = 0;
	tok->err = json_tokener_success;

	/* this interface is presently not 64-bit clean due to the int len argument
	 * and the internal printbuf interface that takes 32-bit int len arguments
	 * so the function limits the maximum string size to INT32_MAX (2GB).
	 * If the function is called with len == -1 then strlen is called to check
	 * the string length is less than INT32_MAX (2GB)
	 */
	if ((len < -1) || (len == -1 && strlen(str) > INT32_MAX))
	{
		tok->err = json_tokener_error_size;
		return NULL;
	}

#ifdef HAVE_USELOCALE
	{
		locale_t duploc = duplocale(oldlocale);
		newloc = newlocale(LC_NUMERIC_MASK, "C", duploc);
		if (newloc == NULL)
		{
			freelocale(duploc);
			return NULL;
		}
		uselocale(newloc);
	}
#elif defined(HAVE_SETLOCALE)
	{
		char *tmplocale;
		tmplocale = setlocale(LC_NUMERIC, NULL);
		if (tmplocale)
			oldlocale = strdup(tmplocale);
		setlocale(LC_NUMERIC, "C");
	}
#endif

	while (PEEK_CHAR(c, tok))
	{

	redo_char:
		switch (state)
		{

		case json_tokener_state_eatws:
			/* Advance until we change state */
			while (isspace((unsigned char)c))
			{
				if ((!ADVANCE_CHAR(str, tok)) || (!PEEK_CHAR(c, tok)))
					goto out;
			}
			if (c == '/' && !(tok->flags & JSON_TOKENER_STRICT))
			{
				printbuf_reset(tok->pb);
				printbuf_memappend_fast(tok->pb, &c, 1);
				state = json_tokener_state_comment_start;
			}
			else
			{
				state = saved_state;
				goto redo_char;
			}
			break;

		case json_tokener_state_start:
			switch (c)
			{
			case '{':
				state = json_tokener_state_eatws;
				saved_state = json_tokener_state_object_field_start;
				current = json_object_new_object();
				if (current == NULL)
					goto out;
				break;
			case '[':
				state = json_tokener_state_eatws;
				saved_state = json_tokener_state_array;
				current = json_object_new_array();
				if (current == NULL)
					goto out;
				break;
			case 'I':
			case 'i':
				state = json_tokener_state_inf;
				printbuf_reset(tok->pb);
				tok->st_pos = 0;
				goto redo_char;
			case 'N':
			case 'n':
				state = json_tokener_state_null; // or NaN
				printbuf_reset(tok->pb);
				tok->st_pos = 0;
				goto redo_char;
			case '\'':
				if (tok->flags & JSON_TOKENER_STRICT)
				{
					/* in STRICT mode only double-quote are allowed */
					tok->err = json_tokener_error_parse_unexpected;
					goto out;
				}
				/* FALLTHRU */
			case '"':
				state = json_tokener_state_string;
				printbuf_reset(tok->pb);
				tok->quote_char = c;
				break;
			case 'T':
			case 't':
			case 'F':
			case 'f':
				state = json_tokener_state_boolean;
				printbuf_reset(tok->pb);
				tok->st_pos = 0;
				goto redo_char;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
				state = json_tokener_state_number;
				printbuf_reset(tok->pb);
				tok->is_double = 0;
				goto redo_char;
			default: tok->err = json_tokener_error_parse_unexpected; goto out;
			}
			break;

		case json_tokener_state_finish:
			if (tok->depth == 0)
				goto out;
			obj = json_object_get(current);
			json_tokener_reset_level(tok, tok->depth);
			tok->depth--;
			goto redo_char;

		case json_tokener_state_inf: /* aka starts with 'i' (or 'I', or "-i", or "-I") */
		{
			/* If we were guaranteed to have len set, then we could (usually) handle
			 * the entire "Infinity" check in a single strncmp (strncasecmp), but
			 * since len might be -1 (i.e. "read until \0"), we need to check it
			 * a character at a time.
			 * Trying to handle it both ways would make this code considerably more
			 * complicated with likely little performance benefit.
			 */
			int is_negative = 0;
			const char *_json_inf_str = json_inf_str;
			if (!(tok->flags & JSON_TOKENER_STRICT))
				_json_inf_str = json_inf_str_lower;

			/* Note: tok->st_pos must be 0 when state is set to json_tokener_state_inf */
			while (tok->st_pos < (int)json_inf_str_len)
			{
				char inf_char = *str;
				if (!(tok->flags & JSON_TOKENER_STRICT))
					inf_char = tolower((int)*str);
				if (inf_char != _json_inf_str[tok->st_pos])
				{
					tok->err = json_tokener_error_parse_unexpected;
					goto out;
				}
				tok->st_pos++;
				(void)ADVANCE_CHAR(str, tok);
				if (!PEEK_CHAR(c, tok))
				{
					/* out of input chars, for now at least */
					goto out;
				}
			}
			/* We checked the full length of "Infinity", so create the object.
			 * When handling -Infinity, the number parsing code will have dropped
			 * the "-" into tok->pb for us, so check it now.
			 */
			if (printbuf_length(tok->pb) > 0 && *(tok->pb->buf) == '-')
			{
				is_negative = 1;
			}
			current = json_object_new_double(is_negative ? -INFINITY : INFINITY);
			if (current == NULL)
				goto out;
			saved_state = json_tokener_state_finish;
			state = json_tokener_state_eatws;
			goto redo_char;
		}
		break;
		case json_tokener_state_null: /* aka starts with 'n' */
		{
			int size;
			int size_nan;
			printbuf_memappend_fast(tok->pb, &c, 1);
			size = json_min(tok->st_pos + 1, json_null_str_len);
			size_nan = json_min(tok->st_pos + 1, json_nan_str_len);
			if ((!(tok->flags & JSON_TOKENER_STRICT) &&
			     strncasecmp(json_null_str, tok->pb->buf, size) == 0) ||
			    (strncmp(json_null_str, tok->pb->buf, size) == 0))
			{
				if (tok->st_pos == json_null_str_len)
				{
					current = NULL;
					saved_state = json_tokener_state_finish;
					state = json_tokener_state_eatws;
					goto redo_char;
				}
			}
			else if ((!(tok->flags & JSON_TOKENER_STRICT) &&
			          strncasecmp(json_nan_str, tok->pb->buf, size_nan) == 0) ||
			         (strncmp(json_nan_str, tok->pb->buf, size_nan) == 0))
			{
				if (tok->st_pos == json_nan_str_len)
				{
					current = json_object_new_double(NAN);
					if (current == NULL)
						goto out;
					saved_state = json_tokener_state_finish;
					state = json_tokener_state_eatws;
					goto redo_char;
				}
			}
			else
			{
				tok->err = json_tokener_error_parse_null;
				goto out;
			}
			tok->st_pos++;
		}
		break;

		case json_tokener_state_comment_start:
			if (c == '*')
			{
				state = json_tokener_state_comment;
			}
			else if (c == '/')
			{
				state = json_tokener_state_comment_eol;
			}
			else
			{
				tok->err = json_tokener_error_parse_comment;
				goto out;
			}
			printbuf_memappend_fast(tok->pb, &c, 1);
			break;

		case json_tokener_state_comment:
		{
			/* Advance until we change state */
			const char *case_start = str;
			while (c != '*')
			{
				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					goto out;
				}
			}
			printbuf_memappend_fast(tok->pb, case_start, 1 + str - case_start);
			state = json_tokener_state_comment_end;
		}
		break;

		case json_tokener_state_comment_eol:
		{
			/* Advance until we change state */
			const char *case_start = str;
			while (c != '\n')
			{
				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					goto out;
				}
			}
			printbuf_memappend_fast(tok->pb, case_start, str - case_start);
			MC_DEBUG("json_tokener_comment: %s\n", tok->pb->buf);
			state = json_tokener_state_eatws;
		}
		break;

		case json_tokener_state_comment_end:
			printbuf_memappend_fast(tok->pb, &c, 1);
			if (c == '/')
			{
				MC_DEBUG("json_tokener_comment: %s\n", tok->pb->buf);
				state = json_tokener_state_eatws;
			}
			else
			{
				state = json_tokener_state_comment;
			}
			break;

		case json_tokener_state_string:
		{
			/* Advance until we change state */
			const char *case_start = str;
			while (1)
			{
				if (c == tok->quote_char)
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					current =
					    json_object_new_string_len(tok->pb->buf, tok->pb->bpos);
					if (current == NULL)
						goto out;
					saved_state = json_tokener_state_finish;
					state = json_tokener_state_eatws;
					break;
				}
				else if (c == '\\')
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					saved_state = json_tokener_state_string;
					state = json_tokener_state_string_escape;
					break;
				}
				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					goto out;
				}
			}
		}
		break;

		case json_tokener_state_string_escape:
			switch (c)
			{
			case '"':
			case '\\':
			case '/':
				printbuf_memappend_fast(tok->pb, &c, 1);
				state = saved_state;
				break;
			case 'b':
			case 'n':
			case 'r':
			case 't':
			case 'f':
				if (c == 'b')
					printbuf_memappend_fast(tok->pb, "\b", 1);
				else if (c == 'n')
					printbuf_memappend_fast(tok->pb, "\n", 1);
				else if (c == 'r')
					printbuf_memappend_fast(tok->pb, "\r", 1);
				else if (c == 't')
					printbuf_memappend_fast(tok->pb, "\t", 1);
				else if (c == 'f')
					printbuf_memappend_fast(tok->pb, "\f", 1);
				state = saved_state;
				break;
			case 'u':
				tok->ucs_char = 0;
				tok->st_pos = 0;
				state = json_tokener_state_escape_unicode;
				break;
			default: tok->err = json_tokener_error_parse_string; goto out;
			}
			break;

		case json_tokener_state_escape_unicode:
		{
			unsigned int got_hi_surrogate = 0;

			/* Handle a 4-byte sequence, or two sequences if a surrogate pair */
			while (1)
			{
				if (c && strchr(json_hex_chars, c))
				{
					tok->ucs_char += ((unsigned int)jt_hexdigit(c)
					                  << ((3 - tok->st_pos++) * 4));
					if (tok->st_pos == 4)
					{
						unsigned char unescaped_utf[4];

						if (got_hi_surrogate)
						{
							if (IS_LOW_SURROGATE(tok->ucs_char))
							{
								/* Recalculate the ucs_char, then fall thru to process normally */
								tok->ucs_char =
								    DECODE_SURROGATE_PAIR(
								        got_hi_surrogate,
								        tok->ucs_char);
							}
							else
							{
								/* Hi surrogate was not followed by a low surrogate */
								/* Replace the hi and process the rest normally */
								printbuf_memappend_fast(
								    tok->pb,
								    (char *)utf8_replacement_char,
								    3);
							}
							got_hi_surrogate = 0;
						}

						if (tok->ucs_char < 0x80)
						{
							unescaped_utf[0] = tok->ucs_char;
							printbuf_memappend_fast(
							    tok->pb, (char *)unescaped_utf, 1);
						}
						else if (tok->ucs_char < 0x800)
						{
							unescaped_utf[0] =
							    0xc0 | (tok->ucs_char >> 6);
							unescaped_utf[1] =
							    0x80 | (tok->ucs_char & 0x3f);
							printbuf_memappend_fast(
							    tok->pb, (char *)unescaped_utf, 2);
						}
						else if (IS_HIGH_SURROGATE(tok->ucs_char))
						{
							/* Got a high surrogate.  Remember it and look for
							 * the beginning of another sequence, which
							 * should be the low surrogate.
							 */
							got_hi_surrogate = tok->ucs_char;
							/* Not at end, and the next two chars should be "\u" */
							if ((len == -1 ||
							     len > (tok->char_offset + 2)) &&
							    // str[0] != '0' &&  // implied by json_hex_chars, above.
							    (str[1] == '\\') && (str[2] == 'u'))
							{
								/* Advance through the 16 bit surrogate, and move
								 * on to the next sequence. The next step is to
								 * process the following characters.
								 */
								if (!ADVANCE_CHAR(str, tok) ||
								    !ADVANCE_CHAR(str, tok))
								{
									printbuf_memappend_fast(
									    tok->pb,
									    (char *)
									        utf8_replacement_char,
									    3);
								}
								/* Advance to the first char of the next sequence and
								 * continue processing with the next sequence.
								 */
								if (!ADVANCE_CHAR(str, tok) ||
								    !PEEK_CHAR(c, tok))
								{
									printbuf_memappend_fast(
									    tok->pb,
									    (char *)
									        utf8_replacement_char,
									    3);
									goto out;
								}
								tok->ucs_char = 0;
								tok->st_pos = 0;
								/* other json_tokener_state_escape_unicode */
								continue;
							}
							else
							{
								/* Got a high surrogate without another sequence following
								 * it.  Put a replacement char in for the hi surrogate
								 * and pretend we finished.
								 */
								printbuf_memappend_fast(
								    tok->pb,
								    (char *)utf8_replacement_char,
								    3);
							}
						}
						else if (IS_LOW_SURROGATE(tok->ucs_char))
						{
							/* Got a low surrogate not preceded by a high */
							printbuf_memappend_fast(
							    tok->pb, (char *)utf8_replacement_char,
							    3);
						}
						else if (tok->ucs_char < 0x10000)
						{
							unescaped_utf[0] =
							    0xe0 | (tok->ucs_char >> 12);
							unescaped_utf[1] =
							    0x80 | ((tok->ucs_char >> 6) & 0x3f);
							unescaped_utf[2] =
							    0x80 | (tok->ucs_char & 0x3f);
							printbuf_memappend_fast(
							    tok->pb, (char *)unescaped_utf, 3);
						}
						else if (tok->ucs_char < 0x110000)
						{
							unescaped_utf[0] =
							    0xf0 | ((tok->ucs_char >> 18) & 0x07);
							unescaped_utf[1] =
							    0x80 | ((tok->ucs_char >> 12) & 0x3f);
							unescaped_utf[2] =
							    0x80 | ((tok->ucs_char >> 6) & 0x3f);
							unescaped_utf[3] =
							    0x80 | (tok->ucs_char & 0x3f);
							printbuf_memappend_fast(
							    tok->pb, (char *)unescaped_utf, 4);
						}
						else
						{
							/* Don't know what we got--insert the replacement char */
							printbuf_memappend_fast(
							    tok->pb, (char *)utf8_replacement_char,
							    3);
						}
						state = saved_state;
						break;
					}
				}
				else
				{
					tok->err = json_tokener_error_parse_string;
					goto out;
				}
				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					/* Clean up any pending chars */
					if (got_hi_surrogate)
						printbuf_memappend_fast(
						    tok->pb, (char *)utf8_replacement_char, 3);
					goto out;
				}
			}
		}
		break;

		case json_tokener_state_boolean:
		{
			int size1, size2;
			printbuf_memappend_fast(tok->pb, &c, 1);
			size1 = json_min(tok->st_pos + 1, json_true_str_len);
			size2 = json_min(tok->st_pos + 1, json_false_str_len);
			if ((!(tok->flags & JSON_TOKENER_STRICT) &&
			     strncasecmp(json_true_str, tok->pb->buf, size1) == 0) ||
			    (strncmp(json_true_str, tok->pb->buf, size1) == 0))
			{
				if (tok->st_pos == json_true_str_len)
				{
					current = json_object_new_boolean(1);
					if (current == NULL)
						goto out;
					saved_state = json_tokener_state_finish;
					state = json_tokener_state_eatws;
					goto redo_char;
				}
			}
			else if ((!(tok->flags & JSON_TOKENER_STRICT) &&
			          strncasecmp(json_false_str, tok->pb->buf, size2) == 0) ||
			         (strncmp(json_false_str, tok->pb->buf, size2) == 0))
			{
				if (tok->st_pos == json_false_str_len)
				{
					current = json_object_new_boolean(0);
					if (current == NULL)
						goto out;
					saved_state = json_tokener_state_finish;
					state = json_tokener_state_eatws;
					goto redo_char;
				}
			}
			else
			{
				tok->err = json_tokener_error_parse_boolean;
				goto out;
			}
			tok->st_pos++;
		}
		break;

		case json_tokener_state_number:
		{
			/* Advance until we change state */
			const char *case_start = str;
			int case_len = 0;
			int is_exponent = 0;
			int negativesign_next_possible_location = 1;
			while (c && strchr(json_number_chars, c))
			{
				++case_len;

				/* non-digit characters checks */
				/* note: since the main loop condition to get here was
				 * an input starting with 0-9 or '-', we are
				 * protected from input starting with '.' or
				 * e/E.
				 */
				if (c == '.')
				{
					if (tok->is_double != 0)
					{
						/* '.' can only be found once, and out of the exponent part.
						 * Thus, if the input is already flagged as double, it
						 * is invalid.
						 */
						tok->err = json_tokener_error_parse_number;
						goto out;
					}
					tok->is_double = 1;
				}
				if (c == 'e' || c == 'E')
				{
					if (is_exponent != 0)
					{
						/* only one exponent possible */
						tok->err = json_tokener_error_parse_number;
						goto out;
					}
					is_exponent = 1;
					tok->is_double = 1;
					/* the exponent part can begin with a negative sign */
					negativesign_next_possible_location = case_len + 1;
				}
				if (c == '-' && case_len != negativesign_next_possible_location)
				{
					/* If the negative sign is not where expected (ie
					 * start of input or start of exponent part), the
					 * input is invalid.
					 */
					tok->err = json_tokener_error_parse_number;
					goto out;
				}

				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					printbuf_memappend_fast(tok->pb, case_start, case_len);
					goto out;
				}
			}
			if (case_len > 0)
				printbuf_memappend_fast(tok->pb, case_start, case_len);

			// Check for -Infinity
			if (tok->pb->buf[0] == '-' && case_len <= 1 && (c == 'i' || c == 'I'))
			{
				state = json_tokener_state_inf;
				tok->st_pos = 0;
				goto redo_char;
			}
		}
			{
				int64_t num64;
				uint64_t numuint64;
				double numd;
				if (!tok->is_double && tok->pb->buf[0] == '-' &&
				    json_parse_int64(tok->pb->buf, &num64) == 0)
				{
					current = json_object_new_int64(num64);
					if (current == NULL)
						goto out;
				}
				else if (!tok->is_double && tok->pb->buf[0] != '-' &&
				         json_parse_uint64(tok->pb->buf, &numuint64) == 0)
				{
					if (numuint64 && tok->pb->buf[0] == '0' &&
					    (tok->flags & JSON_TOKENER_STRICT))
					{
						tok->err = json_tokener_error_parse_number;
						goto out;
					}
					if (numuint64 <= INT64_MAX)
					{
						num64 = (uint64_t)numuint64;
						current = json_object_new_int64(num64);
						if (current == NULL)
							goto out;
					}
					else
					{
						current = json_object_new_uint64(numuint64);
						if (current == NULL)
							goto out;
					}
				}
				else if (tok->is_double &&
				         json_parse_double(tok->pb->buf, &numd) == 0)
				{
					current = json_object_new_double_s(numd, tok->pb->buf);
					if (current == NULL)
						goto out;
				}
				else
				{
					tok->err = json_tokener_error_parse_number;
					goto out;
				}
				saved_state = json_tokener_state_finish;
				state = json_tokener_state_eatws;
				goto redo_char;
			}
			break;

		case json_tokener_state_array_after_sep:
		case json_tokener_state_array:
			if (c == ']')
			{
				if (state == json_tokener_state_array_after_sep &&
				    (tok->flags & JSON_TOKENER_STRICT))
				{
					tok->err = json_tokener_error_parse_unexpected;
					goto out;
				}
				saved_state = json_tokener_state_finish;
				state = json_tokener_state_eatws;
			}
			else
			{
				if (tok->depth >= tok->max_depth - 1)
				{
					tok->err = json_tokener_error_depth;
					goto out;
				}
				state = json_tokener_state_array_add;
				tok->depth++;
				json_tokener_reset_level(tok, tok->depth);
				goto redo_char;
			}
			break;

		case json_tokener_state_array_add:
			if (json_object_array_add(current, obj) != 0)
				goto out;
			saved_state = json_tokener_state_array_sep;
			state = json_tokener_state_eatws;
			goto redo_char;

		case json_tokener_state_array_sep:
			if (c == ']')
			{
				saved_state = json_tokener_state_finish;
				state = json_tokener_state_eatws;
			}
			else if (c == ',')
			{
				saved_state = json_tokener_state_array_after_sep;
				state = json_tokener_state_eatws;
			}
			else
			{
				tok->err = json_tokener_error_parse_array;
				goto out;
			}
			break;

		case json_tokener_state_object_field_start:
		case json_tokener_state_object_field_start_after_sep:
			if (c == '}')
			{
				if (state == json_tokener_state_object_field_start_after_sep &&
				    (tok->flags & JSON_TOKENER_STRICT))
				{
					tok->err = json_tokener_error_parse_unexpected;
					goto out;
				}
				saved_state = json_tokener_state_finish;
				state = json_tokener_state_eatws;
			}
			else if (c == '"' || c == '\'')
			{
				tok->quote_char = c;
				printbuf_reset(tok->pb);
				state = json_tokener_state_object_field;
			}
			else
			{
				tok->err = json_tokener_error_parse_object_key_name;
				goto out;
			}
			break;

		case json_tokener_state_object_field:
		{
			/* Advance until we change state */
			const char *case_start = str;
			while (1)
			{
				if (c == tok->quote_char)
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					obj_field_name = strdup(tok->pb->buf);
					saved_state = json_tokener_state_object_field_end;
					state = json_tokener_state_eatws;
					break;
				}
				else if (c == '\\')
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					saved_state = json_tokener_state_object_field;
					state = json_tokener_state_string_escape;
					break;
				}
				if (!ADVANCE_CHAR(str, tok) || !PEEK_CHAR(c, tok))
				{
					printbuf_memappend_fast(tok->pb, case_start,
					                        str - case_start);
					goto out;
				}
			}
		}
		break;

		case json_tokener_state_object_field_end:
			if (c == ':')
			{
				saved_state = json_tokener_state_object_value;
				state = json_tokener_state_eatws;
			}
			else
			{
				tok->err = json_tokener_error_parse_object_key_sep;
				goto out;
			}
			break;

		case json_tokener_state_object_value:
			if (tok->depth >= tok->max_depth - 1)
			{
				tok->err = json_tokener_error_depth;
				goto out;
			}
			state = json_tokener_state_object_value_add;
			tok->depth++;
			json_tokener_reset_level(tok, tok->depth);
			goto redo_char;

		case json_tokener_state_object_value_add:
			json_object_object_add(current, obj_field_name, obj);
			free(obj_field_name);
			obj_field_name = NULL;
			saved_state = json_tokener_state_object_sep;
			state = json_tokener_state_eatws;
			goto redo_char;

		case json_tokener_state_object_sep:
			/* { */
			if (c == '}')
			{
				saved_state = json_tokener_state_finish;
				state = json_tokener_state_eatws;
			}
			else if (c == ',')
			{
				saved_state = json_tokener_state_object_field_start_after_sep;
				state = json_tokener_state_eatws;
			}
			else
			{
				tok->err = json_tokener_error_parse_object_value_sep;
				goto out;
			}
			break;
		}
		if (!ADVANCE_CHAR(str, tok))
			goto out;
	} /* while(PEEK_CHAR) */

out:
	if ((tok->flags & JSON_TOKENER_VALIDATE_UTF8) && (nBytes != 0))
	{
		tok->err = json_tokener_error_parse_utf8_string;
	}
	if (c && (state == json_tokener_state_finish) && (tok->depth == 0) &&
	    (tok->flags & JSON_TOKENER_STRICT))
	{
		/* unexpected char after JSON data */
		tok->err = json_tokener_error_parse_unexpected;
	}
	if (!c)
	{
		/* We hit an eof char (0) */
		if (state != json_tokener_state_finish && saved_state != json_tokener_state_finish)
			tok->err = json_tokener_error_parse_eof;
	}

#ifdef HAVE_USELOCALE
	uselocale(oldlocale);
	freelocale(newloc);
#elif defined(HAVE_SETLOCALE)
	setlocale(LC_NUMERIC, oldlocale);
	free(oldlocale);
#endif

	if (tok->err == json_tokener_success)
	{
		json_object *ret = json_object_get(current);
		int ii;

		/* Partially reset, so we parse additional objects on subsequent calls. */
		for (ii = tok->depth; ii >= 0; ii--)
			json_tokener_reset_level(tok, ii);
		return ret;
	}

	MC_DEBUG("json_tokener_parse_ex: error %s at offset %d\n", json_tokener_errors[tok->err],
	         tok->char_offset);
	return NULL;
}

static json_bool json_tokener_validate_utf8(const char c, unsigned int *nBytes)
{
	unsigned char chr = c;
	if (*nBytes == 0)
	{
		if (chr >= 0x80)
		{
			if ((chr & 0xe0) == 0xc0)
				*nBytes = 1;
			else if ((chr & 0xf0) == 0xe0)
				*nBytes = 2;
			else if ((chr & 0xf8) == 0xf0)
				*nBytes = 3;
			else
				return 0;
		}
	}
	else
	{
		if ((chr & 0xC0) != 0x80)
			return 0;
		(*nBytes)--;
	}
	return 1;
}

void json_tokener_set_flags(struct json_tokener *tok, int flags)
{
	tok->flags = flags;
}

size_t json_tokener_get_parse_end(struct json_tokener *tok)
{
	assert(tok->char_offset >= 0); /* Drop this line when char_offset becomes a size_t */
	return (size_t)tok->char_offset;
}
