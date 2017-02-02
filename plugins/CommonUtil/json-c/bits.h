/*
 * $Id: bits.h,v 1.10 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#ifndef _bits_h_
#define _bits_h_

#ifndef json_min
#define json_min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef json_max
#define json_max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define hexdigit(x) (((x) <= '9') ? (x) - '0' : ((x) & 7) + 9)
#define error_ptr(error) ((void*)error)
#define error_description(error)  (json_tokener_errors[error])
#define is_error(ptr) (ptr == NULL)

#endif
