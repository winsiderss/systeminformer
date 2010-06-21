#ifndef _PH_DLTMGR_H
#define _PH_DLTMGR_H

typedef struct _PH_SINGLE_DELTA
{
    FLOAT Value;
    FLOAT Delta;
} PH_SINGLE_DELTA, *PPH_SINGLE_DELTA;

typedef struct _PH_UINT32_DELTA
{
    ULONG Value;
    ULONG Delta;
} PH_UINT32_DELTA, *PPH_UINT32_DELTA;

typedef struct _PH_UINT64_DELTA
{
    ULONG64 Value;
    ULONG64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;

#define PhInitializeDelta(DltMgr) \
    ((DltMgr)->Value = 0, (DltMgr)->Delta = 0)

#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)

#endif
