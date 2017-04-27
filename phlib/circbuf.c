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
