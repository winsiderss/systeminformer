/*
 * $Id: arraylist.h,v 1.4 2006/01/26 02:16:28 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#ifndef _arraylist_h_
#define _arraylist_h_

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_LIST_DEFAULT_SIZE 32

typedef void (array_list_free_fn) (void *data);

struct array_list
{
  void **array;
  int length;
  int size;
  array_list_free_fn *free_fn;
};

extern struct array_list*
array_list_new(array_list_free_fn *free_fn);

extern void
array_list_free(struct array_list *al);

extern void*
array_list_get_idx(struct array_list *al, int i);

extern int
array_list_put_idx(struct array_list *al, int i, void *data);

extern int
array_list_add(struct array_list *al, void *data);

extern int
array_list_length(struct array_list *al);

extern void
array_list_sort(struct array_list *arr, int(__cdecl* compar)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
