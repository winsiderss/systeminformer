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

#include "..\include\phbase.h"
#include "..\include\phnative.h"

#include "config.h"
#undef realloc

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>
#endif /* defined(_WIN32) */

#if !defined(HAVE_OPEN) && defined(_WIN32)
# define open _open
#endif

#if !defined(HAVE_SNPRINTF) && defined(_MSC_VER)
  /* MSC has the version as _snprintf */
# define snprintf _snprintf
#elif !defined(HAVE_SNPRINTF)
# error You do not have snprintf on your system.
#endif /* HAVE_SNPRINTF */

#include "bits.h"
#include "debug.h"
#include "printbuf.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_tokener.h"
#include "json_util.h"

static int sscanf_is_broken = 0;
static int sscanf_is_broken_testdone = 0;
static void sscanf_is_broken_test(void);

struct json_object* json_object_from_file(wchar_t *filename)
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    struct json_object *obj = NULL;

    status = PhCreateFileWin32(
        &fileHandle,
        filename,
        FILE_GENERIC_WRITE,
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

        data[dataLength] = 0;

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
   
    if (!(json_str = json_object_to_json_string_ext(obj, flags)))
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

// backwards compatible "format and write to file" function

int json_object_to_file(wchar_t *FileName, struct json_object *obj)
{
    return json_object_to_file_ext(FileName, obj, JSON_C_TO_STRING_PRETTY);
}

int json_parse_double(const char *buf, double *retval)
{
  return (sscanf_s(buf, "%lf", retval)==1 ? 0 : 1);
}

/*
 * Not all implementations of sscanf actually work properly.
 * Check whether the one we're currently using does, and if
 * it's broken, enable the workaround code.
 */
static void sscanf_is_broken_test()
{
    int64_t num64;
    int ret_errno, is_int64_min, ret_errno2, is_int64_max;

    sscanf_s(" -01234567890123456789012345", "%" SCNd64, &num64);
    ret_errno = errno;
    is_int64_min = (num64 == INT64_MIN);

    sscanf_s(" 01234567890123456789012345", "%" SCNd64, &num64);
    ret_errno2 = errno;
    is_int64_max = (num64 == INT64_MAX);

    if (ret_errno != ERANGE || !is_int64_min ||
        ret_errno2 != ERANGE || !is_int64_max)
    {
        MC_DEBUG("sscanf_is_broken_test failed, enabling workaround code\n");
        sscanf_is_broken = 1;
    }
}

int json_parse_int64(const char *buf, int64_t *retval)
{
    int64_t num64;
    const char *buf_sig_digits;
    int orig_has_neg;
    int saved_errno;

    if (!sscanf_is_broken_testdone)
    {
        sscanf_is_broken_test();
        sscanf_is_broken_testdone = 1;
    }

    // Skip leading spaces
    while (isspace((int)*buf) && *buf)
        buf++;

    errno = 0; // sscanf won't always set errno, so initialize

    if (sscanf_s(buf, "%" SCNd64, &num64) != 1)
    {
        MC_DEBUG("Failed to parse, sscanf != 1\n");
        return 1;
    }

    saved_errno = errno;
    buf_sig_digits = buf;
    orig_has_neg = 0;
    if (*buf_sig_digits == '-')
    {
        buf_sig_digits++;
        orig_has_neg = 1;
    }

    // Not all sscanf implementations actually work
    if (sscanf_is_broken && saved_errno != ERANGE)
    {
        char buf_cmp[100];
        char *buf_cmp_start = buf_cmp;
        int recheck_has_neg = 0;
        int buf_cmp_len;

        // Skip leading zeros, but keep at least one digit
        while (buf_sig_digits[0] == '0' && buf_sig_digits[1] != '\0')
            buf_sig_digits++;
        if (num64 == 0) // assume all sscanf impl's will parse -0 to 0
            orig_has_neg = 0; // "-0" is the same as just plain "0"

        _snprintf_s(buf_cmp_start, sizeof(buf_cmp), _TRUNCATE, "%" PRId64, num64);
        if (*buf_cmp_start == '-')
        {
            recheck_has_neg = 1;
            buf_cmp_start++;
        }
        // No need to skip leading spaces or zeros here.

        buf_cmp_len = (int)strlen(buf_cmp_start);
        /**
         * If the sign is different, or
         * some of the digits are different, or
         * there is another digit present in the original string
         * then we have NOT successfully parsed the value.
         */
        if (orig_has_neg != recheck_has_neg ||
            strncmp(buf_sig_digits, buf_cmp_start, strlen(buf_cmp_start)) != 0 ||
            ((int)strlen(buf_sig_digits) != buf_cmp_len &&
             isdigit((int)buf_sig_digits[buf_cmp_len])
            )
           )
        {
            saved_errno = ERANGE;
        }
    }

    // Not all sscanf impl's set the value properly when out of range.
    // Always do this, even for properly functioning implementations,
    // since it shouldn't slow things down much.
    if (saved_errno == ERANGE)
    {
        if (orig_has_neg)
            num64 = INT64_MIN;
        else
            num64 = INT64_MAX;
    }
    *retval = num64;
    return 0;
}

#ifndef HAVE_REALLOC
void* rpl_realloc(void* p, size_t n)
{
    if (n == 0)
        n = 1;
    if (p == 0)
        return malloc(n);
    return realloc(p, n);
}
#endif

#define NELEM(a)        (sizeof(a) / sizeof(a[0]))
static const char* json_type_name[] = {
  /* If you change this, be sure to update the enum json_type definition too */
  "null",
  "boolean",
  "double",
  "int",
  "object",
  "array",
  "string",
};

const char *json_type_to_name(enum json_type o_type)
{
    int o_type_int = (int)o_type;
    if (o_type_int < 0 || o_type_int >= (int)NELEM(json_type_name))
    {
        MC_ERROR("json_type_to_name: type %d is out of range [0,%d]\n", o_type, NELEM(json_type_name));
        return NULL;
    }
    return json_type_name[o_type];
}

