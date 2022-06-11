/*
 * $Id: json_util.c,v 1.4 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#include <phbase.h>
#include <phnative.h> // dmex

#include "config.h"
#undef realloc

#include "strerror_override.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>
#endif /* defined(WIN32) */

#if !defined(HAVE_OPEN) && defined(WIN32)
#define open _open
#endif

#include "snprintf_compat.h"

#include "debug.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_tokener.h"
#include "json_util.h"
#include "printbuf.h"

static int _json_object_to_fd(int fd, struct json_object *obj, int flags, const char *filename);

static char _last_err[256] = "";

const char *json_util_get_last_err()
{
    if (_last_err[0] == '\0')
        return NULL;
    return _last_err;
}

void _json_c_set_last_err(const char *err_fmt, ...)
{
    va_list ap;
    va_start(ap, err_fmt);
    // Ignore (attempted) overruns from snprintf
    (void)vsnprintf(_last_err, sizeof(_last_err), err_fmt, ap);
    va_end(ap);
}

struct json_object *json_object_from_fd(int fd)
{
    return json_object_from_fd_ex(fd, -1);
}
struct json_object *json_object_from_fd_ex(int fd, int in_depth)
{
    struct printbuf *pb;
    struct json_object *obj;
    char buf[JSON_FILE_BUF_SIZE];
    int ret;
    int depth = JSON_TOKENER_DEFAULT_DEPTH;
    json_tokener *tok;

    if (!(pb = printbuf_new()))
    {
        _json_c_set_last_err("json_object_from_fd_ex: printbuf_new failed\n");
        return NULL;
    }

    if (in_depth != -1)
        depth = in_depth;
    tok = json_tokener_new_ex(depth);
    if (!tok)
    {
        _json_c_set_last_err(
            "json_object_from_fd_ex: unable to allocate json_tokener(depth=%d): %s\n", depth,
            strerror(errno));
        printbuf_free(pb);
        return NULL;
    }

    while ((ret = _read(fd, buf, JSON_FILE_BUF_SIZE)) > 0)
    {
        printbuf_memappend(pb, buf, ret);
    }
    if (ret < 0)
    {
        _json_c_set_last_err("json_object_from_fd_ex: error reading fd %d: %s\n", fd,
                             strerror(errno));
        json_tokener_free(tok);
        printbuf_free(pb);
        return NULL;
    }

    obj = json_tokener_parse_ex(tok, pb->buf, printbuf_length(pb));
    if (obj == NULL)
        _json_c_set_last_err("json_tokener_parse_ex failed: %s\n",
                             json_tokener_error_desc(json_tokener_get_error(tok)));

    json_tokener_free(tok);
    printbuf_free(pb);
    return obj;
}

struct json_object *json_object_from_file(wchar_t *filename)
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    struct json_object* obj = NULL;

    status = PhCreateFileWin32(
        &fileHandle,
        filename,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        PSTR data;
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;
        BYTE buffer[PAGE_SIZE];

        allocatedLength = sizeof(buffer);
        data = (PSTR)PhAllocate(allocatedLength);
        dataLength = 0;

        memset(data, 0, allocatedLength);

        while (NT_SUCCESS(NtReadFile(fileHandle, NULL, NULL, NULL, &isb, buffer, PAGE_SIZE, NULL, NULL)))
        {
            returnLength = (ULONG)isb.Information;

            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                data = (PSTR)PhReAllocate(data, allocatedLength);
            }

            memcpy(data + dataLength, buffer, returnLength);

            dataLength += returnLength;
        }

        if (allocatedLength < dataLength + 1)
        {
            allocatedLength++;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        data[dataLength] = ANSI_NULL;

        obj = json_tokener_parse(data);

        PhFree(data);
    }

    return obj;
}

/* extended "format and write to file" function */

int json_object_to_file_ext(wchar_t *filename, struct json_object *obj, int flags)
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    PSTR json_str;

    if (!(json_str = (PSTR)json_object_to_json_string_ext(obj, flags)))
        return -1;

    status = PhCreateFileWin32(
        &fileHandle,
        filename,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return -1;

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        json_str,
        (ULONG)strlen(json_str),
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(fileHandle);
        return -1;
    }

    NtClose(fileHandle);
    return 0;
}

int json_object_to_fd(int fd, struct json_object *obj, int flags)
{
    if (!obj)
    {
        _json_c_set_last_err("json_object_to_fd: object is null\n");
        return -1;
    }

    return _json_object_to_fd(fd, obj, flags, NULL);
}
static int _json_object_to_fd(int fd, struct json_object *obj, int flags, const char *filename)
{
    int ret;
    const char *json_str;
    unsigned int wpos, wsize;

    filename = filename ? filename : "(fd)";

    if (!(json_str = json_object_to_json_string_ext(obj, flags)))
    {
        return -1;
    }

    /* CAW: probably unnecessary, but the most 64bit safe */
    wsize = (unsigned int)(strlen(json_str) & UINT_MAX);
    wpos = 0;
    while (wpos < wsize)
    {
        if ((ret = _write(fd, json_str + wpos, wsize - wpos)) < 0)
        {
            _json_c_set_last_err("json_object_to_fd: error writing file %s: %s\n",
                                 filename, strerror(errno));
            return -1;
        }

        /* because of the above check for ret < 0, we can safely cast and add */
        wpos += (unsigned int)ret;
    }

    return 0;
}

// backwards compatible "format and write to file" function

int json_object_to_file(wchar_t *filename, struct json_object *obj)
{
    return json_object_to_file_ext(filename, obj, JSON_C_TO_STRING_PLAIN);
}

// Deprecated json_parse_double function.  See json_tokener_parse_double instead.
int json_parse_double(const char *buf, double *retval)
{
    char *end;
    *retval = strtod(buf, &end);
    return end == buf ? 1 : 0;
}

int json_parse_int64(const char *buf, int64_t *retval)
{
    char *end = NULL;
    int64_t val;

    errno = 0;
    val = strtoll(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

int json_parse_uint64(const char *buf, uint64_t *retval)
{
    char *end = NULL;
    uint64_t val;

    errno = 0;
    while (*buf == ' ')
        buf++;
    if (*buf == '-')
        return 1; /* error: uint cannot be negative */

    val = strtoull(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

#ifndef HAVE_REALLOC
void *rpl_realloc(void *p, size_t n)
{
    if (n == 0)
        n = 1;
    if (p == 0)
        return malloc(n);
    return realloc(p, n);
}
#endif

#define NELEM(a) (sizeof(a) / sizeof(a[0]))
/* clang-format off */
static const char *json_type_name[] = {
    /* If you change this, be sure to update the enum json_type definition too */
    "null",
    "boolean",
    "double",
    "int",
    "object",
    "array",
    "string",
};
/* clang-format on */

const char *json_type_to_name(enum json_type o_type)
{
    int o_type_int = (int)o_type;
    if (o_type_int < 0 || o_type_int >= (int)NELEM(json_type_name))
    {
        _json_c_set_last_err("json_type_to_name: type %d is out of range [0,%u]\n", o_type,
                             (unsigned)NELEM(json_type_name));
        return NULL;
    }
    return json_type_name[o_type];
}
