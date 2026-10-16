/* BwtSort.c -- BWT block sorting
: Igor Pavlov : Public domain */

#include "Precomp.h"

#include "BwtSort.h"
#include "Sort.h"

/* #define BLOCK_SORT_USE_HEAP_SORT */
// #define BLOCK_SORT_USE_HEAP_SORT

#ifdef BLOCK_SORT_USE_HEAP_SORT

#define HeapSortRefDown(p, vals, n, size, temp) \
  { size_t k = n; UInt32 val = vals[temp]; for (;;) { \
    size_t s = k << 1; \
    if (s > size) break; \
    if (s < size && vals[p[s + 1]] > vals[p[s]]) s++; \
    if (val >= vals[p[s]]) break; \
    p[k] = p[s]; k = s; \
  } p[k] = temp; }

void HeapSortRef(UInt32 *p, UInt32 *vals, size_t size)
{
  if (size <= 1)
    return;
  p--;
  {
    size_t i = size / 2;
    do
    {
      UInt32 temp = p[i];
      HeapSortRefDown(p, vals, i, size, temp);
    }
    while (--i != 0);
  }
  do
  {
    UInt32 temp = p[size];
    p[size--] = p[1];
    HeapSortRefDown(p, vals, 1, size, temp);
  }
  while (size > 1);
}

#endif // BLOCK_SORT_USE_HEAP_SORT


/* Don't change it !!! */
#define kNumHashBytes 2
#define kNumHashValues (1 << (kNumHashBytes * 8))

/* kNumRefBitsMax must be < (kNumHashBytes * 8) = 16 */
#define kNumRefBitsMax 12

#define BS_TEMP_SIZE kNumHashValues

#ifdef BLOCK_SORT_EXTERNAL_FLAGS

/* 32 Flags in UInt32 word */
#define kNumFlagsBits 5
#define kNumFlagsInWord (1 << kNumFlagsBits)
#define kFlagsMask (kNumFlagsInWord - 1)
#define kAllFlags 0xFFFFFFFF

#else

#define kNumBitsMax     20
#define kIndexMask      (((UInt32)1 << kNumBitsMax) - 1)
#define kNumExtraBits   (32 - kNumBitsMax)
#define kNumExtra0Bits  (kNumExtraBits - 2)
#define kNumExtra0Mask  ((1 << kNumExtra0Bits) - 1)

#define SetFinishedGroupSize(p, size) \
  {  *(p) |= ((((UInt32)(size) - 1) & kNumExtra0Mask) << kNumBitsMax); \
    if ((size) > (1 << kNumExtra0Bits)) { \
      *(p) |= 0x40000000; \
      *((p) + 1) |= (((UInt32)(size) - 1) >> kNumExtra0Bits) << kNumBitsMax; } } \

static void SetGroupSize(UInt32 *p, size_t size)
{
  if (--size == 0)
    return;
  *p |= 0x80000000 | (((UInt32)size & kNumExtra0Mask) << kNumBitsMax);
  if (size >= (1 << kNumExtra0Bits))
  {
    *p |= 0x40000000;
    p[1] |= (((UInt32)size >> kNumExtra0Bits) << kNumBitsMax);
  }
}

#endif

/*
SortGroup - is recursive Range-Sort function with HeapSort optimization for small blocks
  "range" is not real range. It's only for optimization.
returns: 1 - if there are groups, 0 - no more groups
*/

static
unsigned
Z7_FASTCALL
SortGroup(size_t BlockSize, size_t NumSortedBytes,
    size_t groupOffset, size_t groupSize,
    unsigned NumRefBits, UInt32 *Indices
#ifndef BLOCK_SORT_USE_HEAP_SORT
    , size_t left, size_t range
#endif
  )
{
  UInt32 *ind2 = Indices + groupOffset;
  UInt32 *Groups;
  if (groupSize <= 1)
  {
    /*
    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetFinishedGroupSize(ind2, 1)
    #endif
    */
    return 0;
  }
  Groups = Indices + BlockSize + BS_TEMP_SIZE;
  if (groupSize <= ((size_t)1 << NumRefBits)
#ifndef BLOCK_SORT_USE_HEAP_SORT
      && groupSize <= range
#endif
      )
  {
    UInt32 *temp = Indices + BlockSize;
    size_t j, group;
    UInt32 mask, cg;
    unsigned thereAreGroups;
    {
      UInt32 gPrev;
      UInt32 gRes = 0;
      {
        size_t sp = ind2[0] + NumSortedBytes;
        if (sp >= BlockSize)
            sp -= BlockSize;
        gPrev = Groups[sp];
        temp[0] = gPrev << NumRefBits;
      }
      
      for (j = 1; j < groupSize; j++)
      {
        size_t sp = ind2[j] + NumSortedBytes;
        UInt32 g;
        if (sp >= BlockSize)
            sp -= BlockSize;
        g = Groups[sp];
        temp[j] = (g << NumRefBits) | (UInt32)j;
        gRes |= (gPrev ^ g);
      }
      if (gRes == 0)
      {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
        SetGroupSize(ind2, groupSize);
#endif
        return 1;
      }
    }
    
    HeapSort(temp, groupSize);
    mask = ((UInt32)1 << NumRefBits) - 1;
    thereAreGroups = 0;
    
    group = groupOffset;
    cg = temp[0] >> NumRefBits;
    temp[0] = ind2[temp[0] & mask];

    {
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
#else
    size_t prevGroupStart = 0;
#endif
    
    for (j = 1; j < groupSize; j++)
    {
      const UInt32 val = temp[j];
      const UInt32 cgCur = val >> NumRefBits;
      
      if (cgCur != cg)
      {
        cg = cgCur;
        group = groupOffset + j;

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
          const size_t t = group - 1;
          Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
        }
#else
        SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
#endif
      }
      else
        thereAreGroups = 1;
      {
        const UInt32 ind = ind2[val & mask];
        temp[j] = ind;
        Groups[ind] = (UInt32)group;
      }
    }

#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
#endif
    }

    for (j = 0; j < groupSize; j++)
      ind2[j] = temp[j];
    return thereAreGroups;
  }

  /* Check that all strings are in one group (cannot sort) */
  {
    UInt32 group;
    size_t j;
    size_t sp = ind2[0] + NumSortedBytes;
    if (sp >= BlockSize)
        sp -= BlockSize;
    group = Groups[sp];
    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j] + NumSortedBytes;
      if (sp >= BlockSize)
          sp -= BlockSize;
      if (Groups[sp] != group)
        break;
    }
    if (j == groupSize)
    {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
#endif
      return 1;
    }
  }

#ifndef BLOCK_SORT_USE_HEAP_SORT
  {
  /* ---------- Range Sort ---------- */
  size_t i;
  size_t mid;
  for (;;)
  {
    size_t j;
    if (range <= 1)
    {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
#endif
      return 1;
    }
    mid = left + ((range + 1) >> 1);
    j = groupSize;
    i = 0;
    do
    {
      size_t sp = ind2[i] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
      if (Groups[sp] >= mid)
      {
        for (j--; j > i; j--)
        {
          sp = ind2[j] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
          if (Groups[sp] < mid)
          {
            UInt32 temp = ind2[i]; ind2[i] = ind2[j]; ind2[j] = temp;
            break;
          }
        }
        if (i >= j)
          break;
      }
    }
    while (++i < j);
    if (i == 0)
    {
      range = range - (mid - left);
      left = mid;
    }
    else if (i == groupSize)
      range = (mid - left);
    else
      break;
  }

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  {
    const size_t t = groupOffset + i - 1;
    UInt32 *Flags = Groups + BlockSize;
    Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
  }
#endif

  {
    size_t j;
    for (j = i; j < groupSize; j++)
      Groups[ind2[j]] = (UInt32)(groupOffset + i);
  }

  {
    unsigned res = SortGroup(BlockSize, NumSortedBytes, groupOffset, i, NumRefBits, Indices, left, mid - left);
    return   res | SortGroup(BlockSize, NumSortedBytes, groupOffset + i, groupSize - i, NumRefBits, Indices, mid, range - (mid - left));
  }

  }

#else // BLOCK_SORT_USE_HEAP_SORT

  /* ---------- Heap Sort ---------- */

  {
    size_t j;
    for (j = 0; j < groupSize; j++)
    {
      size_t sp = ind2[j] + NumSortedBytes;
      if (sp >= BlockSize)
          sp -= BlockSize;
      ind2[j] = (UInt32)sp;
    }

    HeapSortRef(ind2, Groups, groupSize);

    /* Write Flags */
    {
    size_t sp = ind2[0];
    UInt32 group = Groups[sp];

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
#else
    size_t prevGroupStart = 0;
#endif

    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j];
      if (Groups[sp] != group)
      {
        group = Groups[sp];
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
        const size_t t = groupOffset + j - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        }
#else
        SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
#endif
      }
    }

#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
#endif
    }
    {
    /* Write new Groups values and Check that there are groups */
    unsigned thereAreGroups = 0;
    for (j = 0; j < groupSize; j++)
    {
      size_t group = groupOffset + j;
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
      UInt32 subGroupSize = ((ind2[j] & ~0xC0000000) >> kNumBitsMax);
      if (ind2[j] & 0x40000000)
        subGroupSize += ((ind2[(size_t)j + 1] >> kNumBitsMax) << kNumExtra0Bits);
      subGroupSize++;
      for (;;)
      {
        const UInt32 original = ind2[j];
        size_t sp = original & kIndexMask;
        if (sp < NumSortedBytes)
          sp += BlockSize;
        sp -= NumSortedBytes;
        ind2[j] = (UInt32)sp | (original & ~kIndexMask);
        Groups[sp] = (UInt32)group;
        if (--subGroupSize == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
#else
      UInt32 *Flags = Groups + BlockSize;
      for (;;)
      {
        size_t sp = ind2[j];
        if (sp < NumSortedBytes)
          sp += BlockSize;
        sp -= NumSortedBytes;
        ind2[j] = (UInt32)sp;
        Groups[sp] = (UInt32)group;
        if ((Flags[(groupOffset + j) >> kNumFlagsBits] & (1 << ((groupOffset + j) & kFlagsMask))) == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
#endif
    }
    return thereAreGroups;
    }
  }
#endif // BLOCK_SORT_USE_HEAP_SORT
}


/* conditions: blockSize > 0 */
UInt32 BlockSort(UInt32 *Indices, const Byte *data, size_t blockSize)
{
  UInt32 *counters = Indices + blockSize;
  size_t i;
  UInt32 *Groups;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  UInt32 *Flags;
#endif

/* Radix-Sort for 2 bytes */
// { UInt32 yyy; for (yyy = 0; yyy < 100; yyy++) {
  for (i = 0; i < kNumHashValues; i++)
    counters[i] = 0;
  {
    const Byte *data2 = data;
    size_t a = data[(size_t)blockSize - 1];
    const Byte *data_lim = data + blockSize;
    if (blockSize >= 4)
    {
      data_lim -= 3;
      do
      {
        size_t b;
        b = data2[0]; counters[(a << 8) | b]++;
        a = data2[1]; counters[(b << 8) | a]++;
        b = data2[2]; counters[(a << 8) | b]++;
        a = data2[3]; counters[(b << 8) | a]++;
        data2 += 4;
      }
      while (data2 < data_lim);
      data_lim += 3;
    }
    while (data2 != data_lim)
    {
      size_t b = *data2++;
      counters[(a << 8) | b]++;
      a = b;
    }
  }
// }}

  Groups = counters + BS_TEMP_SIZE;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
  Flags = Groups + blockSize;
  {
    const size_t numWords = (blockSize + kFlagsMask) >> kNumFlagsBits;
    for (i = 0; i < numWords; i++)
      Flags[i] = kAllFlags;
  }
#endif

  {
    UInt32 sum = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      const UInt32 groupSize = counters[i];
      counters[i] = sum;
      sum += groupSize;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS
      if (groupSize)
      {
        const UInt32 t = sum - 1;
        Flags[t >> kNumFlagsBits] &= ~((UInt32)1 << (t & kFlagsMask));
      }
#endif
    }
  }

  for (i = 0; i < blockSize - 1; i++)
    Groups[i] = counters[((unsigned)data[i] << 8) | data[(size_t)i + 1]];
  Groups[i] = counters[((unsigned)data[i] << 8) | data[0]];
  
  {
#define SET_Indices(a, b, i)  \
    { UInt32 c; \
      a = (a << 8) | (b); \
      c = counters[a]; \
      Indices[c] = (UInt32)i++; \
      counters[a] = c + 1; \
    }

    size_t a = data[0];
    const Byte *data_ptr = data + 1;
    i = 0;
    if (blockSize >= 3)
    {
      blockSize -= 2;
      do
      {
        size_t b;
        b = data_ptr[0];  SET_Indices(a, b, i)
        a = data_ptr[1];  SET_Indices(b, a, i)
        data_ptr += 2;
      }
      while (i < blockSize);
      blockSize += 2;
    }
    if (i < blockSize - 1)
    {
      SET_Indices(a, data[(size_t)i + 1], i)
      a = (Byte)a;
    }
    SET_Indices(a, data[0], i)
  }
  
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
  {
    UInt32 prev = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      const UInt32 prevGroupSize = counters[i] - prev;
      if (prevGroupSize == 0)
        continue;
      SetGroupSize(Indices + prev, prevGroupSize);
      prev = counters[i];
    }
  }
#endif

  {
  unsigned NumRefBits;
  size_t NumSortedBytes;
  for (NumRefBits = 0; ((blockSize - 1) >> NumRefBits) != 0; NumRefBits++)
  {}
  NumRefBits = 32 - NumRefBits;
  if (NumRefBits > kNumRefBitsMax)
      NumRefBits = kNumRefBitsMax;

  for (NumSortedBytes = kNumHashBytes; ; NumSortedBytes <<= 1)
  {
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
    size_t finishedGroupSize = 0;
#endif
    size_t newLimit = 0;
    for (i = 0; i < blockSize;)
    {
      size_t groupSize;
#ifdef BLOCK_SORT_EXTERNAL_FLAGS

      if ((Flags[i >> kNumFlagsBits] & (1 << (i & kFlagsMask))) == 0)
      {
        i++;
        continue;
      }
      for (groupSize = 1;
        (Flags[(i + groupSize) >> kNumFlagsBits] & (1 << ((i + groupSize) & kFlagsMask))) != 0;
        groupSize++)
        {}
      groupSize++;

#else

      groupSize = (Indices[i] & ~0xC0000000) >> kNumBitsMax;
      {
        const BoolInt finishedGroup = ((Indices[i] & 0x80000000) == 0);
        if (Indices[i] & 0x40000000)
        {
          groupSize += ((Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits);
          Indices[(size_t)i + 1] &= kIndexMask;
        }
        Indices[i] &= kIndexMask;
        groupSize++;
        if (finishedGroup || groupSize == 1)
        {
          Indices[i - finishedGroupSize] &= kIndexMask;
          if (finishedGroupSize > 1)
            Indices[(size_t)(i - finishedGroupSize) + 1] &= kIndexMask;
          {
            const size_t newGroupSize = groupSize + finishedGroupSize;
            SetFinishedGroupSize(Indices + i - finishedGroupSize, newGroupSize)
            finishedGroupSize = newGroupSize;
          }
          i += groupSize;
          continue;
        }
        finishedGroupSize = 0;
      }

#endif
      
      if (NumSortedBytes >= blockSize)
      {
        size_t j;
        for (j = 0; j < groupSize; j++)
        {
          size_t t = i + j;
          /* Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask)); */
          Groups[Indices[t]] = (UInt32)t;
        }
      }
      else
        if (SortGroup(blockSize, NumSortedBytes, i, groupSize, NumRefBits, Indices
            #ifndef BLOCK_SORT_USE_HEAP_SORT
              , 0, blockSize
            #endif
            ))
          newLimit = i + groupSize;
      i += groupSize;
    }
    if (newLimit == 0)
      break;
  }
  }
#ifndef BLOCK_SORT_EXTERNAL_FLAGS
  for (i = 0; i < blockSize;)
  {
    size_t groupSize = (Indices[i] & ~0xC0000000) >> kNumBitsMax;
    if (Indices[i] & 0x40000000)
    {
      groupSize += (Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits;
      Indices[(size_t)i + 1] &= kIndexMask;
    }
    Indices[i] &= kIndexMask;
    groupSize++;
    i += groupSize;
  }
#endif
  return Groups[0];
}
