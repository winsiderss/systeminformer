/*
 * Copyright (c) 2016 Alexandru Ardelean.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#include "config.h"

#include "strerror_override.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_pointer.h"
#include "strdup_compat.h"
#include "vasprintf_compat.h"

/* Avoid ctype.h and locale overhead */
#define is_plain_digit(c) ((c) >= '0' && (c) <= '9')

/**
 * JavaScript Object Notation (JSON) Pointer
 *   RFC 6901 - https://tools.ietf.org/html/rfc6901
 */

static void string_replace_all_occurrences_with_char(char *s, const char *occur, char repl_char)
{
    int slen = (int)strlen(s);
    int skip = (int)strlen(occur) - 1; /* length of the occurrence, minus the char we're replacing */
    char *p = s;
    while ((p = strstr(p, occur)))
    {
        *p = repl_char;
        p++;
        slen -= skip;
        memmove(p, (p + skip), slen - (p - s) + 1); /* includes null char too */
    }
}

static int is_valid_index(struct json_object *jo, const char *path, int32_t *idx)
{
    int i, len = (int)strlen(path);
    /* this code-path optimizes a bit, for when we reference the 0-9 index range
     * in a JSON array and because leading zeros not allowed
     */
    if (len == 1)
    {
        if (is_plain_digit(path[0]))
        {
            *idx = (path[0] - '0');
            goto check_oob;
        }
        errno = EINVAL;
        return 0;
    }
    /* leading zeros not allowed per RFC */
    if (path[0] == '0')
    {
        errno = EINVAL;
        return 0;
    }
    /* RFC states base-10 decimals */
    for (i = 0; i < len; i++)
    {
        if (!is_plain_digit(path[i]))
        {
            errno = EINVAL;
            return 0;
        }
    }

    *idx = strtol(path, NULL, 10);
    if (*idx < 0)
    {
        errno = EINVAL;
        return 0;
    }
check_oob:
    len = (int)json_object_array_length(jo);
    if (*idx >= len)
    {
        errno = ENOENT;
        return 0;
    }

    return 1;
}

static int json_pointer_get_single_path(struct json_object *obj, char *path,
                                        struct json_object **value)
{
    if (json_object_is_type(obj, json_type_array))
    {
        int32_t idx;
        if (!is_valid_index(obj, path, &idx))
            return -1;
        obj = json_object_array_get_idx(obj, idx);
        if (obj)
        {
            if (value)
                *value = obj;
            return 0;
        }
        /* Entry not found */
        errno = ENOENT;
        return -1;
    }

    /* RFC states that we first must eval all ~1 then all ~0 */
    string_replace_all_occurrences_with_char(path, "~1", '/');
    string_replace_all_occurrences_with_char(path, "~0", '~');

    if (!json_object_object_get_ex(obj, path, value))
    {
        errno = ENOENT;
        return -1;
    }

    return 0;
}

static int json_pointer_set_single_path(struct json_object *parent, const char *path,
                                        struct json_object *value)
{
    if (json_object_is_type(parent, json_type_array))
    {
        int32_t idx;
        /* RFC (Chapter 4) states that '-' may be used to add new elements to an array */
        if (path[0] == '-' && path[1] == '\0')
            return json_object_array_add(parent, value);
        if (!is_valid_index(parent, path, &idx))
            return -1;
        return json_object_array_put_idx(parent, idx, value);
    }

    /* path replacements should have been done in json_pointer_get_single_path(),
     * and we should still be good here
     */
    if (json_object_is_type(parent, json_type_object))
        return json_object_object_add(parent, path, value);

    /* Getting here means that we tried to "dereference" a primitive JSON type
     * (like string, int, bool).i.e. add a sub-object to it
     */
    errno = ENOENT;
    return -1;
}

static int json_pointer_get_recursive(struct json_object *obj, char *path,
                                      struct json_object **value)
{
    char *endp;
    int rc;

    /* All paths (on each recursion level must have a leading '/' */
    if (path[0] != '/')
    {
        errno = EINVAL;
        return -1;
    }
    path++;

    endp = strchr(path, '/');
    if (endp)
        *endp = '\0';

    /* If we err-ed here, return here */
    if ((rc = json_pointer_get_single_path(obj, path, &obj)))
        return rc;

    if (endp)
    {
        /* Put the slash back, so that the sanity check passes on next recursion level */
        *endp = '/';
        return json_pointer_get_recursive(obj, endp, value);
    }

    /* We should be at the end of the recursion here */
    if (value)
        *value = obj;

    return 0;
}

int json_pointer_get(struct json_object *obj, const char *path, struct json_object **res)
{
    char *path_copy = NULL;
    int rc;

    if (!obj || !path)
    {
        errno = EINVAL;
        return -1;
    }

    if (path[0] == '\0')
    {
        if (res)
            *res = obj;
        return 0;
    }

    /* pass a working copy to the recursive call */
    if (!(path_copy = _strdup(path)))
    {
        errno = ENOMEM;
        return -1;
    }
    rc = json_pointer_get_recursive(obj, path_copy, res);
    free(path_copy);

    return rc;
}

int json_pointer_getf(struct json_object *obj, struct json_object **res, const char *path_fmt, ...)
{
    char *path_copy = NULL;
    int rc = 0;
    va_list args;

    if (!obj || !path_fmt)
    {
        errno = EINVAL;
        return -1;
    }

    va_start(args, path_fmt);
    rc = vasprintf(&path_copy, path_fmt, args);
    va_end(args);

    if (rc < 0)
        return rc;

    if (path_copy[0] == '\0')
    {
        if (res)
            *res = obj;
        goto out;
    }

    rc = json_pointer_get_recursive(obj, path_copy, res);
out:
    free(path_copy);

    return rc;
}

int json_pointer_set(struct json_object **obj, const char *path, struct json_object *value)
{
    const char *endp;
    char *path_copy = NULL;
    struct json_object *set = NULL;
    int rc;

    if (!obj || !path)
    {
        errno = EINVAL;
        return -1;
    }

    if (path[0] == '\0')
    {
        json_object_put(*obj);
        *obj = value;
        return 0;
    }

    if (path[0] != '/')
    {
        errno = EINVAL;
        return -1;
    }

    /* If there's only 1 level to set, stop here */
    if ((endp = strrchr(path, '/')) == path)
    {
        path++;
        return json_pointer_set_single_path(*obj, path, value);
    }

    /* pass a working copy to the recursive call */
    if (!(path_copy = _strdup(path)))
    {
        errno = ENOMEM;
        return -1;
    }
    path_copy[endp - path] = '\0';
    rc = json_pointer_get_recursive(*obj, path_copy, &set);
    free(path_copy);

    if (rc)
        return rc;

    endp++;
    return json_pointer_set_single_path(set, endp, value);
}

int json_pointer_setf(struct json_object **obj, struct json_object *value, const char *path_fmt,
                      ...)
{
    char *endp;
    char *path_copy = NULL;
    struct json_object *set = NULL;
    va_list args;
    int rc = 0;

    if (!obj || !path_fmt)
    {
        errno = EINVAL;
        return -1;
    }

    /* pass a working copy to the recursive call */
    va_start(args, path_fmt);
    rc = vasprintf(&path_copy, path_fmt, args);
    va_end(args);

    if (rc < 0)
        return rc;

    if (path_copy[0] == '\0')
    {
        json_object_put(*obj);
        *obj = value;
        goto out;
    }

    if (path_copy[0] != '/')
    {
        errno = EINVAL;
        rc = -1;
        goto out;
    }

    /* If there's only 1 level to set, stop here */
    if ((endp = strrchr(path_copy, '/')) == path_copy)
    {
        set = *obj;
        goto set_single_path;
    }

    *endp = '\0';
    rc = json_pointer_get_recursive(*obj, path_copy, &set);

    if (rc)
        goto out;

set_single_path:
    endp++;
    rc = json_pointer_set_single_path(set, endp, value);
out:
    free(path_copy);
    return rc;
}
