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

#ifndef _json_object_private_h_
#define _json_object_private_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (json_object_private_delete_fn)(struct json_object *o);

struct json_object
{
  enum json_type o_type;
  json_object_private_delete_fn *_delete;
  json_object_to_json_string_fn *_to_json_string;
  int _ref_count;
  struct printbuf *_pb;
  union data {
    json_bool c_boolean;
    double c_double;
    int64_t c_int64;
    struct lh_table *c_object;
    struct array_list *c_array;
    struct {
        char *str;
        size_t len;
    } c_string;
  } o;
  json_object_delete_fn *_user_delete;
  void *_userdata;
};

#ifdef __cplusplus
}
#endif

#endif
