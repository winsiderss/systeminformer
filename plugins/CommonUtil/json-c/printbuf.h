/*
 * $Id: printbuf.h,v 1.4 2006/01/26 02:16:28 mclark Exp $
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

#ifndef _printbuf_h_
#define _printbuf_h_

#ifdef __cplusplus
extern "C" {
#endif

struct printbuf {
  char *buf;
  size_t bpos;
  size_t size;
};

extern struct printbuf*
printbuf_new(void);

/* As an optimization, printbuf_memappend_fast is defined as a macro
 * that handles copying data if the buffer is large enough; otherwise
 * it invokes printbuf_memappend_real() which performs the heavy
 * lifting of realloc()ing the buffer and copying data.
 * Your code should not use printbuf_memappend directly--use
 * printbuf_memappend_fast instead.
 */
extern size_t printbuf_memappend(struct printbuf *p, const char *buf, size_t size);

__inline void printbuf_memappend_fast(struct printbuf *p, const char *bufptr, size_t bufsize)
{
    if ((p->size - p->bpos) > bufsize)
    {
        memcpy(p->buf + p->bpos, (bufptr), bufsize);
        p->bpos += (int)bufsize;
        p->buf[p->bpos] = '\0';
    }
    else
    {
        printbuf_memappend(p, (bufptr), bufsize);
    }
}

#define printbuf_length(p) ((p)->bpos)

/**
 * Set len bytes of the buffer to charvalue, starting at offset offset.
 * Similar to calling memset(x, charvalue, len);
 *
 * The memory allocated for the buffer is extended as necessary.
 *
 * If offset is -1, this starts at the end of the current data in the buffer.
 */
extern int
printbuf_memset(struct printbuf *pb, size_t offset, int charvalue, size_t len);

extern int
sprintbuf(struct printbuf *p, const char *msg, ...);

extern void
printbuf_reset(struct printbuf *p);

extern void
printbuf_free(struct printbuf *p);

#ifdef __cplusplus
}
#endif

#endif
