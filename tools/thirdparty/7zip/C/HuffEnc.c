/* HuffEnc.c -- functions for Huffman encoding
Igor Pavlov : Public domain */

#include "Precomp.h"

#include <string.h>

#include "HuffEnc.h"
#include "Sort.h"
#include "CpuArch.h"

#define kMaxLen       Z7_HUFFMAN_LEN_MAX
#define NUM_BITS      10
#define MASK          ((1u << NUM_BITS) - 1)
#define FREQ_MASK     (~(UInt32)MASK)
#define NUM_COUNTERS  (104 * 2) // (80 * 2) or (128 * 2) : ((prime_number + 1) * 2) for smaller code.

#if 1 && (defined(MY_CPU_LE) || defined(MY_CPU_BE))
#if defined(MY_CPU_LE)
  #define HI_HALF_OFFSET 1
#else
  #define HI_HALF_OFFSET 0
#endif
#define LOAD_PARENT(p)                  ((unsigned)*((const UInt16 *)(p) + HI_HALF_OFFSET))
#define STORE_PARENT(p, fb, val)         *((UInt16 *)(p) + HI_HALF_OFFSET) = (UInt16)(val);
#define STORE_PARENT_DIRECT(p, fb, hi)  STORE_PARENT(p, fb, hi)
#define UPDATE_E(eHi)                   eHi++;
#else
#define LOAD_PARENT(p)                  ((unsigned)(*(p) >> NUM_BITS))
#define STORE_PARENT_DIRECT(p, fb, hi)  *(p) = ((fb) & MASK) | (hi); // set parent field
#define STORE_PARENT(p, fb, val)        STORE_PARENT_DIRECT(p, fb, ((UInt32)(val) << NUM_BITS))
#define UPDATE_E(eHi)                   eHi += 1 << NUM_BITS;
#endif

void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, unsigned numSymbols, unsigned maxLen)
{
#if NUM_COUNTERS > 2
  unsigned counters[NUM_COUNTERS];
#endif
#if 1 && NUM_COUNTERS > (kMaxLen + 4) * 2
  #define lenCounters   (counters)
  #define codes         (counters + kMaxLen + 4)
#else
  unsigned lenCounters[kMaxLen + 1];
  UInt32 codes[kMaxLen + 1];
#endif

  unsigned num;
  {
    unsigned i;
    // UInt32 sum = 0;

#if NUM_COUNTERS > 2
    
#define CTR_ITEM_FOR_FREQ(freq) \
    counters[(freq) >= NUM_COUNTERS - 1 ? NUM_COUNTERS - 1 : (unsigned)(freq)]

    for (i = 0; i < NUM_COUNTERS; i++)
      counters[i] = 0;
    memset(lens, 0, numSymbols);
    {
      const UInt32 *fp = freqs + numSymbols;
#define NUM_UNROLLS 1
#if NUM_UNROLLS > 1 // use 1 if odd (numSymbols) is possisble
      if (numSymbols & 1)
      {
        UInt32 f;
        f = *--fp;  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
      }
#endif
      do
      {
        UInt32 f;
        fp -= NUM_UNROLLS;
        f = fp[0];  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
#if NUM_UNROLLS > 1
        f = fp[1];  CTR_ITEM_FOR_FREQ(f)++;
        // sum += f;
#endif
      }
      while (fp != freqs);
    }
#if 0
    printf("\nsum=%8u numSymbols =%3u ctrs:", sum, numSymbols);
    {
      unsigned k = 0;
      for (k = 0; k < NUM_COUNTERS; k++)
        printf(" %u", counters[k]);
    }
#endif
        
    num = counters[1];
    counters[1] = 0;
    for (i = 2; i != NUM_COUNTERS; i += 2)
    {
      const unsigned c0 = (counters    )[i];
      const unsigned c1 = (counters + 1)[i];
      (counters    )[i] = num;  num += c0;
      (counters + 1)[i] = num;  num += c1;
    }
    counters[0] = num; // we want to write (freq==0) symbols to the end of (p) array
    {
      i = 0;
      do
      {
        const UInt32 f = freqs[i];
#if 0
        if (f == 0) lens[i] = 0; else
#endif
          p[CTR_ITEM_FOR_FREQ(f)++] = i | (f << NUM_BITS);
      }
      while (++i != numSymbols);
    }
    HeapSort(p + counters[NUM_COUNTERS - 2], counters[NUM_COUNTERS - 1] - counters[NUM_COUNTERS - 2]);
    
#else // NUM_COUNTERS <= 2
    
    num = 0;
    for (i = 0; i < numSymbols; i++)
    {
      const UInt32 freq = freqs[i];
      if (freq == 0)
        lens[i] = 0;
      else
        p[num++] = i | (freq << NUM_BITS);
    }
    HeapSort(p, num);

#endif
  }

  if (num <= 2)
  {
    unsigned minCode = 0;
    unsigned maxCode = 1;
    if (num)
    {
      maxCode = (unsigned)p[(size_t)num - 1] & MASK;
      if (num == 2)
      {
        minCode = (unsigned)p[0] & MASK;
        if (minCode > maxCode)
        {
          const unsigned temp = minCode;
          minCode = maxCode;
          maxCode = temp;
        }
      }
      else if (maxCode == 0)
        maxCode++;
    }
    p[minCode] = 0;
    p[maxCode] = 1;
    lens[minCode] = lens[maxCode] = 1;
    return;
  }
  {
    unsigned i;
    for (i = 0; i <= kMaxLen; i++)
      lenCounters[i] = 0;
    lenCounters[1] = 2;  // by default root node has 2 child leaves at level 1.
  }
  // if (num != 2)
  {
    // num > 2
    // the binary tree will contain (num - 1) internal nodes.
    // p[num - 2] will be root node of binary tree.
    UInt32 *b;
    UInt32 *n;
    // first node will have two leaf childs: p[0] and p[1]:
    // p[0] += p[1] & FREQ_MASK; // set frequency sum of child leafs
    // if (pi == n) exit(0);
    // if (pi != n)
    {
      UInt32 fb = (p[1] & FREQ_MASK) + p[0];
      UInt32 f = p[2] & FREQ_MASK;
      const UInt32 *pi = p + 2;
      UInt32 *e = p;
      UInt32 eHi = 0;
      n = p + num;
      b = p;
      // p[0] = fb;
      for (;;)
      {
        // (b <= e)
        UInt32 sum;
        e++;
        UPDATE_E(eHi)

        // (b < e)

        // p range : high bits
        // [0, b)  : parent :     processed nodes that have parent and childs
        // [b, e)  : FREQ   : non-processed nodes that have no parent but have childs
        // [e, pi) : FREQ   :     processed leaves for which parent node was     created
        // [pi, n) : FREQ   : non-processed leaves for which parent node was not created

        // first child
        // note : (*b < f) is same result as ((*b & FREQ_MASK) < f)
        if (fb < f)
        {
          // node freq is smaller
          sum = fb & FREQ_MASK;
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          fb = *b;
          if (b == e)
          {
            if (++pi == n)
              break;
            sum += f;
            fb &= MASK;
            fb |= sum;
            *e = fb;
            f = *pi & FREQ_MASK;
            continue;
          }
        }
        else if (++pi == n)
        {
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          break;
        }
        else
        {
          sum = f;
          f = *pi & FREQ_MASK;
        }

        // (b < e)

        // second child
        if (fb < f)
        {
          sum += fb;
          sum &= FREQ_MASK;
          STORE_PARENT_DIRECT (b, fb, eHi)
          b++;
          *e = (*e & MASK) | sum; // set frequency sum
          // (b <= e) is possible here
          fb = *b;
        }
        else if (++pi == n)
          break;
        else
        {
          sum += f;
          f = *pi & FREQ_MASK;
          *e = (*e & MASK) | sum; // set frequency sum
        }
      }
    }
    
    // printf("\nnum-e=%3u, numSymbols=%3u, num=%3u, b=%3u", n - e, numSymbols, n - p, b - p);
    {
      n -= 2;
      *n &= MASK; // root node : we clear high bits (zero bits mean level == 0)
      if (n != b)
      {
        // We go here, if we have some number of non-created nodes up to root.
        // We process them in simplified code:
        // position of parent for each pair of nodes is known.
        // n[-2], n[-1] : current pair of child nodes
        // (p1) : parent node for current pair.
        UInt32 *p1 = n;
        do
        {
          const unsigned len = LOAD_PARENT(p1) + 1;
          p1--;
          (lenCounters    )[len] -= 2;                  // we remove 2 leaves from level (len)
          (lenCounters + 1)[len] += 2 * 2;  // we add 4 leaves at level (len + 1)
          n -= 2;
          STORE_PARENT (n    , n[0], len)
          STORE_PARENT (n + 1, n[1], len)
        }
        while (n != b);
      }
    }
    
    if (b != p)
    {
      // we detect level of each node (realtive to root),
      // and update lenCounters[].
      // We process only intermediate nodes and we don't process leaves.
      do
      {
        // if (ii <  b) : parent_bits_of (p[ii]) == index of parent node : ii < (p[ii])
        // if (ii >= b) : parent_bits_of (p[ii]) == level of this (ii) node in tree
        unsigned len;
        b--;
        len = (unsigned)LOAD_PARENT(p + LOAD_PARENT(b)) + 1;
        STORE_PARENT (b, *b, len)
        if (len >= maxLen)
        {
          // We are not allowed to create node at level (maxLen) and higher,
          // because all leaves must be placed to level (maxLen) or lower.
          // We find nearest allowed leaf and place current node to level of that leaf:
          for (len = maxLen - 1; lenCounters[len] == 0; len--) {}
        }
        lenCounters[len]--;                 // we remove 1 leaf from level (len)
        (lenCounters + 1)[len] += 2;  // we add 2 leaves at level (len + 1)
      }
      while (b != p);
    }
  }
  {
    {
      unsigned len = maxLen;
      const UInt32 *p2 = p;
      do
      {
        unsigned k = lenCounters[len];
        if (k)
          do
            lens[(unsigned)*p2++ & MASK] = (Byte)len;
          while (--k);
      }
      while (--len);
    }
    codes[0] = 0; // we don't want garbage values to be written to p[] array.
    // codes[1] = 0;
    {
      UInt32 code = 0;
      unsigned len;
      for (len = 0; len < kMaxLen; len++)
        (codes + 1)[len] = code = (code + lenCounters[len]) << 1;
    }
    /* if (code + lenCounters[kMaxLen] - 1 != (1 << kMaxLen) - 1) throw 1; */
    {
      const Byte * const limit = lens + numSymbols;
      do
      {
        unsigned len;
        UInt32 c;
        len = lens[0];  c = codes[len];  p[0] = c;  codes[len] = c + 1;
        // len = lens[1];  c = codes[len];  p[1] = c;  codes[len] = c + 1;
        p += 1;
        lens += 1;
      }
      while (lens != limit);
    }
  }
}

#undef kMaxLen
#undef NUM_BITS
#undef MASK
#undef FREQ_MASK
#undef NUM_COUNTERS
#undef CTR_ITEM_FOR_FREQ
#undef LOAD_PARENT
#undef STORE_PARENT
#undef STORE_PARENT_DIRECT
#undef UPDATE_E
#undef HI_HALF_OFFSET
#undef NUM_UNROLLS
#undef lenCounters
#undef codes
