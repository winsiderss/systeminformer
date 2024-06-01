/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2025
 *
 */

#include <phbase.h>
#include <circbuf.h>

#undef T
#define T ULONG
#include "circbuf_i.h"

#undef T
#define T ULONG64
#include "circbuf_i.h"

#undef T
#define T PVOID
#include "circbuf_i.h"

#undef T
#define T SIZE_T
#include "circbuf_i.h"

#undef T
#define T FLOAT
#include "circbuf_i.h"

#undef T
