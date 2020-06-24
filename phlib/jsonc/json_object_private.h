/*
 * $Id: json_object_private.h,v 1.4 2006/01/26 02:16:28 mclark Exp $
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
 * @brief Do not use, json-c internal, may be changed or removed at any time.
 */
#ifndef _json_object_private_h_
#define _json_object_private_h_

#ifdef __cplusplus
extern "C" {
#endif

/**< how many bytes are directly stored in json_object for strings? */
#define LEN_DIRECT_STRING_DATA 32

struct json_object;
#include "json_inttypes.h"
#include "json_types.h"

typedef void(json_object_private_delete_fn)(struct json_object *o);

/* json object int type, support extension*/
typedef enum json_object_int_type
{
	json_object_int_type_int64,
	json_object_int_type_uint64
} json_object_int_type;

struct json_object
{
	enum json_type o_type;
	uint32_t _ref_count;
	json_object_private_delete_fn *_delete;
	json_object_to_json_string_fn *_to_json_string;
	struct printbuf *_pb;
	union data
	{
		json_bool c_boolean;
		double c_double;
		struct
		{
			union
			{
				int64_t c_int64;
				uint64_t c_uint64;
			} cint;
			enum json_object_int_type cint_type;
		} c_int;
		struct lh_table *c_object;
		struct array_list *c_array;
		struct
		{
			union
			{
				/* optimize: if we have small strings, we can store them
				 * directly. This saves considerable CPU cycles AND memory.
				 */
				char *ptr;
				char data[LEN_DIRECT_STRING_DATA];
			} str;
			int len;
		} c_string;
	} o;
	json_object_delete_fn *_user_delete;
	void *_userdata;
};

void _json_c_set_last_err(const char *err_fmt, ...);

extern const char *json_number_chars;
extern const char *json_hex_chars;

#ifdef __cplusplus
}
#endif

#endif
