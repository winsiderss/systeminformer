#ifndef CIRCBUF_H
#define CIRCBUF_H

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
