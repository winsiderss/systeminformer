/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#ifndef _PH_CIRCBUF_H
#define _PH_CIRCBUF_H

#define PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE

#undef T
#define T ULONG
#include "circbuf_h.h"

#undef T
#define T ULONG64
#include "circbuf_h.h"

#undef T
#define T PVOID
#include "circbuf_h.h"

#undef T
#define T SIZE_T
#include "circbuf_h.h"

#undef T
#define T FLOAT
#include "circbuf_h.h"

#endif
