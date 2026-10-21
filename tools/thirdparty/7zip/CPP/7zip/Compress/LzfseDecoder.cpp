// LzfseDecoder.cpp

/*
This code implements LZFSE data decompressing.
The code from "LZFSE compression library" was used.

2018      : Igor Pavlov : BSD 3-clause License : the code in this file
2015-2017 : Apple Inc   : BSD 3-clause License : original "LZFSE compression library" code

The code in the "LZFSE compression library" is licensed under the "BSD 3-clause License":
----
Copyright (c) 2015-2016, Apple Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.

3.  Neither the name of the copyright holder(s) nor the names of any contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
----
*/

#include "StdAfx.h"

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#include "../../../C/CpuArch.h"

#include "LzfseDecoder.h"

namespace NCompress {
namespace NLzfse {

static const Byte kSignature_LZFSE_V1 = 0x31; // '1'
static const Byte kSignature_LZFSE_V2 = 0x32; // '2'


HRESULT CDecoder::GetUInt32(UInt32 &val)
{
  Byte b[4];
  for (unsigned i = 0; i < 4; i++)
    if (!m_InStream.ReadByte(b[i]))
      return S_FALSE;
  val = GetUi32(b);
  return S_OK;
}



HRESULT CDecoder::DecodeUncompressed(UInt32 unpackSize)
{
  PRF(printf("\nUncompressed %7u\n", unpackSize));

  const unsigned kBufSize = 1 << 8;
  Byte buf[kBufSize];
  for (;;)
  {
    if (unpackSize == 0)
      return S_OK;
    UInt32 cur = unpackSize;
    if (cur > kBufSize)
      cur = kBufSize;
    const UInt32 cur2 = (UInt32)m_InStream.ReadBytes(buf, cur);
    m_OutWindowStream.PutBytes(buf, cur2);
    if (cur != cur2)
      return S_FALSE;
  }
}



HRESULT CDecoder::DecodeLzvn(UInt32 unpackSize, UInt32 packSize)
{
  PRF(printf("\nLZVN 0x%07x 0x%07x\n", unpackSize, packSize));
  
  UInt32 D = 0;

  for (;;)
  {
    if (packSize == 0)
      return S_FALSE;
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    packSize--;

    UInt32 M;
    UInt32 L;

    if (b >= 0xE0)
    {
      /*
      large L   - 11100000 LLLLLLLL <LITERALS>
      small L   - 1110LLLL <LITERALS>
      
      large Rep - 11110000 MMMMMMMM
      small Rep - 1111MMMM
      */
        
      M = b & 0xF;
      if (M == 0)
      {
        if (packSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        M = (UInt32)b1 + 16;
      }
      L = 0;
      if ((b & 0x10) == 0)
      {
        // Literals only
        L = M;
        M = 0;
      }
    }
      
    // ERROR codes
    else if ((b & 0xF0) == 0x70) // 0111xxxx
      return S_FALSE;
    else if ((b & 0xF0) == 0xD0) // 1101xxxx
      return S_FALSE;
    
    else
    {
      if ((b & 0xE0) == 0xA0)
      {
        // medium  - 101LLMMM DDDDDDMM DDDDDDDD <LITERALS>
        if (packSize < 2)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        
        Byte b2;
        if (!m_InStream.ReadByte(b2))
          return S_FALSE;
        packSize--;
        L = (((UInt32)b >> 3) & 3);
        M = (((UInt32)b & 7) << 2) + (b1 & 3);
        D = ((UInt32)b1 >> 2) + ((UInt32)b2 << 6);
      }
      else
      {
        L = (UInt32)b >> 6;
        M = ((UInt32)b >> 3) & 7;
        if ((b & 0x7) == 6)
        {
          // REP - LLMMM110 <LITERALS>
          if (L == 0)
          {
            // spec
            if (M == 0)
              break; // EOS
            if (M <= 2)
              continue; // NOP
            return S_FALSE; // UNDEFINED
          }
        }
        else
        {
          if (packSize == 0)
            return S_FALSE;
          Byte b1;
          if (!m_InStream.ReadByte(b1))
            return S_FALSE;
          packSize--;
          
          // large - LLMMM111 DDDDDDDD DDDDDDDD <LITERALS>
          // small - LLMMMDDD DDDDDDDD <LITERALS>
          D  = ((UInt32)b & 7);
          if (D == 7)
          {
            if (packSize == 0)
              return S_FALSE;
            Byte b2;
            if (!m_InStream.ReadByte(b2))
              return S_FALSE;
            packSize--;
            D = b2;
          }
          D = (D << 8) + b1;
        }
      }

      M += 3;
    }
    {
      for (unsigned i = 0; i < L; i++)
      {
        if (packSize == 0 || unpackSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        m_OutWindowStream.PutByte(b1);
        unpackSize--;
      }
    }
    
    if (M != 0)
    {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE;
    }
  }

  if (unpackSize != 0)
    return S_FALSE;

  // LZVN encoder writes 7 additional zero bytes
  if (packSize < 7)
    return S_FALSE;
  for (unsigned i = 0; i < 7; i++)
  {
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    if (b != 0)
      return S_FALSE;
  }
  packSize -= 7;
  if (packSize)
  {
    PRF(printf("packSize after unused = %u\n", packSize));
    // if (packSize <= 0x100) { Byte buf[0x100]; m_InStream.ReadBytes(buf, packSize); }
    /* Lzvn block that is used in HFS can contain junk data
       (at least 256 bytes) after payload data. Why?
       We ignore that junk data, if it's HFS (LzvnMode) mode. */
    if (!LzvnMode)
      return S_FALSE;
  }
  return S_OK;
}



// ---------- LZFSE ----------

#define MATCHES_PER_BLOCK 10000
#define LITERALS_PER_BLOCK (4 * MATCHES_PER_BLOCK)

#define NUM_L_SYMBOLS 20
#define NUM_M_SYMBOLS 20
#define NUM_D_SYMBOLS 64
#define NUM_LIT_SYMBOLS 256

#define NUM_SYMBOLS ( \
    NUM_L_SYMBOLS + \
    NUM_M_SYMBOLS + \
    NUM_D_SYMBOLS + \
    NUM_LIT_SYMBOLS)

#define NUM_L_STATES (1 << 6)
#define NUM_M_STATES (1 << 6)
#define NUM_D_STATES (1 << 8)
#define NUM_LIT_STATES (1 << 10)


typedef UInt32 CFseState;


static UInt32 SumFreqs(const UInt16 *freqs, unsigned num)
{
  UInt32 sum = 0;
  for (unsigned i = 0; i < num; i++)
    sum += (UInt32)freqs[i];
  return sum;
}


static Z7_FORCE_INLINE unsigned CountZeroBits(UInt32 val, UInt32 mask)
{
  for (unsigned i = 0;;)
  {
    if (val & mask)
      return i;
    i++;
    mask >>= 1;
  }
}


static Z7_FORCE_INLINE void InitLitTable(const UInt16 *freqs, UInt32 *table)
{
  for (unsigned i = 0; i < NUM_LIT_SYMBOLS; i++)
  {
    unsigned f = freqs[i];
    if (f == 0)
      continue;

    //         0 <   f     <= numStates
    //         0 <=  k     <= numStatesLog
    // numStates <= (f<<k) <  numStates * 2
    //         0 <  j0     <= f
    // (f + j0) = next_power_of_2 for f
    unsigned k = CountZeroBits(f, NUM_LIT_STATES);
    unsigned j0 = (((unsigned)NUM_LIT_STATES * 2) >> k) - f;

    /*
    CEntry
    {
      Byte k;
      Byte symbol;
      UInt16 delta;
    };
    */

    UInt32 e = ((UInt32)i << 8) + k;
    k += 16;
    UInt32 d = e + ((UInt32)f << k) - ((UInt32)NUM_LIT_STATES << 16);
    UInt32 step = (UInt32)1 << k;

    unsigned j = 0;
    do
    {
      *table++ = d;
      d += step;
    }
    while (++j < j0);

    e--;
    step >>= 1;
    
    for (j = j0; j < f; j++)
    {
      *table++ = e;
      e += step;
    }
  }
}


typedef struct
{
  Byte totalBits;
  Byte extraBits;
  UInt16 delta;
  UInt32 vbase;
} CExtraEntry;


static void InitExtraDecoderTable(unsigned numStates,
    unsigned numSymbols,
    const UInt16 *freqs,
    const Byte *vbits,
    CExtraEntry *table)
{
  UInt32 vbase = 0;

  for (unsigned i = 0; i < numSymbols; i++)
  {
    unsigned f = freqs[i];
    unsigned extraBits = vbits[i];

    if (f != 0)
    {
      unsigned k = CountZeroBits(f, numStates);
      unsigned j0 = ((2 * numStates) >> k) - f;
      
      unsigned j = 0;
      do
      {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(((f + j) << k) - numStates);
        e->vbase = vbase;
      }
      while (++j < j0);
      
      f -= j0;
      k--;
      
      for (j = 0; j < f; j++)
      {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(j << k);
        e->vbase = vbase;
      }
    }

    vbase += ((UInt32)1 << extraBits);
  }
}


static const Byte k_L_extra[NUM_L_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 5, 8
};

static const Byte k_M_extra[NUM_M_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 8, 11
};

static const Byte k_D_extra[NUM_D_SYMBOLS] =
{
   0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
   4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,
   8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11, 11, 11, 11,
  12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15
};



// ---------- CBitStream ----------

typedef struct
{
  UInt32 accum;
  unsigned numBits; // [0, 31] - Number of valid bits in (accum), other bits are 0
} CBitStream;


static Z7_FORCE_INLINE int FseInStream_Init(CBitStream *s,
    int n, // [-7, 0], (-n == number_of_unused_bits) in last byte
    const Byte **pbuf)
{
  *pbuf -= 4;
  s->accum = GetUi32(*pbuf);
  if (n)
  {
    s->numBits = (unsigned)(n + 32);
    if ((s->accum >> s->numBits) != 0)
      return -1; // ERROR, encoder should have zeroed the upper bits
  }
  else
  {
    *pbuf += 1;
    s->accum >>= 8;
    s->numBits = 24;
  }
  return 0; // OK
}


// 0 <= numBits < 32
#define mask31(x, numBits) ((x) & (((UInt32)1 << (numBits)) - 1))

#define FseInStream_FLUSH \
  { const unsigned nbits = (31 - in.numBits) & (unsigned)-8; \
  if (nbits) { \
    buf -= (nbits >> 3); \
    if (buf < buf_check) return S_FALSE; \
    UInt32 v = GetUi32(buf); \
    in.accum = (in.accum << nbits) | mask31(v, nbits); \
    in.numBits += nbits; }}



static Z7_FORCE_INLINE UInt32 BitStream_Pull(CBitStream *s, unsigned numBits)
{
  s->numBits -= numBits;
  const UInt32 v = s->accum >> s->numBits;
  s->accum = mask31(s->accum, s->numBits);
  return v;
}


#define DECODE_LIT(dest, pstate) { \
  UInt32 e = lit_decoder[pstate]; \
  pstate = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); \
  dest = (Byte)(e >> 8); }


static Z7_FORCE_INLINE UInt32 FseDecodeExtra(CFseState *pstate,
    const CExtraEntry *table,
    CBitStream *s)
{
  const CExtraEntry *e = &table[*pstate];
  UInt32 v = BitStream_Pull(s, e->totalBits);
  unsigned extraBits = e->extraBits;
  *pstate = (CFseState)(e->delta + (v >> extraBits));
  return e->vbase + mask31(v, extraBits);
}


#define freqs_L (freqs)
#define freqs_M (freqs_L + NUM_L_SYMBOLS)
#define freqs_D (freqs_M + NUM_M_SYMBOLS)
#define freqs_LIT (freqs_D + NUM_D_SYMBOLS)

#define GET_BITS_64(v, offset, num, dest) dest = (UInt32)   ((v >> (offset)) & ((1 << (num)) - 1));
#define GET_BITS_64_Int32(v, offset, num, dest) dest = (Int32)((v >> (offset)) & ((1 << (num)) - 1));
#define GET_BITS_32(v, offset, num, dest) dest = (CFseState)((v >> (offset)) & ((1 << (num)) - 1));


HRESULT CDecoder::DecodeLzfse(UInt32 unpackSize, Byte version)
{
  PRF(printf("\nLZFSE-%d %7u", version - '0', unpackSize));

  UInt32 numLiterals;
  UInt32 litPayloadSize;
  Int32 literal_bits;

  UInt32 lit_state_0;
  UInt32 lit_state_1;
  UInt32 lit_state_2;
  UInt32 lit_state_3;

  UInt32 numMatches;
  UInt32 lmdPayloadSize;
  Int32 lmd_bits;

  CFseState l_state;
  CFseState m_state;
  CFseState d_state;

  UInt16 freqs[NUM_SYMBOLS];

  if (version == kSignature_LZFSE_V1)
  {
    return E_NOTIMPL;
    // we need examples to test LZFSE-V1 code
    /*
    const unsigned k_v1_SubHeaderSize = 7 * 4 + 7 * 2;
    const unsigned k_v1_HeaderSize = k_v1_SubHeaderSize + NUM_SYMBOLS * 2;
    _buffer.AllocAtLeast(k_v1_HeaderSize);
    if (m_InStream.ReadBytes(_buffer, k_v1_HeaderSize) != k_v1_HeaderSize)
      return S_FALSE;

    const Byte *buf = _buffer;
    #define GET_32(offs, dest) dest = GetUi32(buf + offs)
    #define GET_16(offs, dest) dest = GetUi16(buf + offs)

    UInt32 payload_bytes;
    GET_32(0, payload_bytes);
    GET_32(4, numLiterals);
    GET_32(8, numMatches);
    GET_32(12, litPayloadSize);
    GET_32(16, lmdPayloadSize);
    if (litPayloadSize > (1 << 20) || lmdPayloadSize > (1 << 20))
      return S_FALSE;
    GET_32(20, literal_bits);
    if (literal_bits < -7 || literal_bits > 0)
      return S_FALSE;

    GET_16(24, lit_state_0);
    GET_16(26, lit_state_1);
    GET_16(28, lit_state_2);
    GET_16(30, lit_state_3);

    GET_32(32, lmd_bits);
    if (lmd_bits < -7 || lmd_bits > 0)
      return S_FALSE;

    GET_16(36, l_state);
    GET_16(38, m_state);
    GET_16(40, d_state);

    for (unsigned i = 0; i < NUM_SYMBOLS; i++)
      freqs[i] = GetUi16(buf + k_v1_SubHeaderSize + i * 2);
    */
  }
  else
  {
    UInt32 headerSize;
    {
      const unsigned kPreHeaderSize = 4 * 2; // signature and upackSize
      const unsigned kHeaderSize = 8 * 3;
      Byte temp[kHeaderSize];
      if (m_InStream.ReadBytes(temp, kHeaderSize) != kHeaderSize)
        return S_FALSE;
      
      UInt64 v;
      
      v = GetUi64(temp);
      GET_BITS_64(v,  0, 20, numLiterals)
      GET_BITS_64(v, 20, 20, litPayloadSize)
      GET_BITS_64(v, 40, 20, numMatches)
      GET_BITS_64_Int32(v, 60, 3 + 1, literal_bits) // (NumberOfUsedBits - 1)
      literal_bits -= 7; // (-NumberOfUnusedBits)
      if (literal_bits > 0)
        return S_FALSE;
      // GET_BITS_64(v, 63, 1, unused);
      
      v = GetUi64(temp + 8);
      GET_BITS_64(v,  0, 10, lit_state_0)
      GET_BITS_64(v, 10, 10, lit_state_1)
      GET_BITS_64(v, 20, 10, lit_state_2)
      GET_BITS_64(v, 30, 10, lit_state_3)
      GET_BITS_64(v, 40, 20, lmdPayloadSize)
      GET_BITS_64_Int32(v, 60, 3 + 1, lmd_bits)
      lmd_bits -= 7;
      if (lmd_bits > 0)
        return S_FALSE;
      // GET_BITS_64(v, 63, 1, unused)
      
      UInt32 v32 = GetUi32(temp + 20);
      // (total header size in bytes; this does not
      // correspond to a field in the uncompressed header version,
      // but is required; we wouldn't know the size of the
      // compresssed header otherwise.
      GET_BITS_32(v32, 0, 10, l_state)
      GET_BITS_32(v32, 10, 10, m_state)
      GET_BITS_32(v32, 20, 10 + 2, d_state)
      // GET_BITS_64(v, 62, 2, unused)
      
      headerSize = GetUi32(temp + 16);
      if (headerSize <= kPreHeaderSize + kHeaderSize)
        return S_FALSE;
      headerSize -= kPreHeaderSize + kHeaderSize;
    }

    // no freqs case is not allowed ?
    // memset(freqs, 0, sizeof(freqs));
    // if (headerSize != 0)
    {
      static const Byte numBitsTable[32] =
      {
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14,
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14
      };
  
      static const Byte valueTable[32] =
      {
        0, 2, 1, 4, 0, 3, 1, 8, 0, 2, 1, 5, 0, 3, 1, 24,
        0, 2, 1, 6, 0, 3, 1, 8, 0, 2, 1, 7, 0, 3, 1, 24
      };

      UInt32 accum = 0;
      unsigned numBits = 0;

      for (unsigned i = 0; i < NUM_SYMBOLS; i++)
      {
        while (numBits <= 14 && headerSize != 0)
        {
          Byte b;
          if (!m_InStream.ReadByte(b))
            return S_FALSE;
          accum |= (UInt32)b << numBits;
          numBits += 8;
          headerSize--;
        }
        
        unsigned b = (unsigned)accum & 31;
        unsigned n = numBitsTable[b];
        if (numBits < n)
          return S_FALSE;
        numBits -= n;
        UInt32 f = valueTable[b];
        if (n >= 8)
          f += ((accum >> 4) & (0x3ff >> (14 - n)));
        accum >>= n;
        freqs[i] = (UInt16)f;
      }
      
      if (numBits >= 8 || headerSize != 0)
        return S_FALSE;
    }
  }

  PRF(printf(" Literals=%6u Matches=%6u", numLiterals, numMatches));

  if (numLiterals > LITERALS_PER_BLOCK
      || (numLiterals & 3) != 0
      || numMatches > MATCHES_PER_BLOCK
      || lit_state_0 >= NUM_LIT_STATES
      || lit_state_1 >= NUM_LIT_STATES
      || lit_state_2 >= NUM_LIT_STATES
      || lit_state_3 >= NUM_LIT_STATES
      || l_state >= NUM_L_STATES
      || m_state >= NUM_M_STATES
      || d_state >= NUM_D_STATES)
    return S_FALSE;

  // only full table is allowed ?
  if (   SumFreqs(freqs_L, NUM_L_SYMBOLS) != NUM_L_STATES
      || SumFreqs(freqs_M, NUM_M_SYMBOLS) != NUM_M_STATES
      || SumFreqs(freqs_D, NUM_D_SYMBOLS) != NUM_D_STATES
      || SumFreqs(freqs_LIT, NUM_LIT_SYMBOLS) != NUM_LIT_STATES)
    return S_FALSE;


  const unsigned kPad = 16;

  // ---------- Decode literals ----------

  {
    _literals.AllocAtLeast(LITERALS_PER_BLOCK + 16);
    _buffer.AllocAtLeast(kPad + litPayloadSize);
    memset(_buffer, 0, kPad);
   
    if (m_InStream.ReadBytes(_buffer + kPad, litPayloadSize) != litPayloadSize)
      return S_FALSE;

    UInt32 lit_decoder[NUM_LIT_STATES];
    InitLitTable(freqs_LIT, lit_decoder);
    
    const Byte *buf_start = _buffer + kPad;
    const Byte *buf_check = buf_start - 4;
    const Byte *buf = buf_start + litPayloadSize;
    CBitStream in;
    if (FseInStream_Init(&in, literal_bits, &buf) != 0)
      return S_FALSE;
    
    Byte *lit = _literals;
    const Byte *lit_limit = lit + numLiterals;
    for (; lit < lit_limit; lit += 4)
    {
      FseInStream_FLUSH
      DECODE_LIT (lit[0], lit_state_0)
      DECODE_LIT (lit[1], lit_state_1)
      FseInStream_FLUSH
      DECODE_LIT (lit[2], lit_state_2)
      DECODE_LIT (lit[3], lit_state_3)
    }
    
    if ((buf_start - buf) * 8 != (int)in.numBits)
      return S_FALSE;
  }

  
  // ---------- Decode LMD ----------

  _buffer.AllocAtLeast(kPad + lmdPayloadSize);
  memset(_buffer, 0, kPad);
  if (m_InStream.ReadBytes(_buffer + kPad, lmdPayloadSize) != lmdPayloadSize)
    return S_FALSE;

  CExtraEntry l_decoder[NUM_L_STATES];
  CExtraEntry m_decoder[NUM_M_STATES];
  CExtraEntry d_decoder[NUM_D_STATES];

  InitExtraDecoderTable(NUM_L_STATES, NUM_L_SYMBOLS, freqs_L, k_L_extra, l_decoder);
  InitExtraDecoderTable(NUM_M_STATES, NUM_M_SYMBOLS, freqs_M, k_M_extra, m_decoder);
  InitExtraDecoderTable(NUM_D_STATES, NUM_D_SYMBOLS, freqs_D, k_D_extra, d_decoder);

  const Byte *buf_start = _buffer + kPad;
  const Byte *buf_check = buf_start - 4;
  const Byte *buf = buf_start + lmdPayloadSize;
  CBitStream in;
  if (FseInStream_Init(&in, lmd_bits, &buf))
    return S_FALSE;
    
  const Byte *lit = _literals;
  const Byte *lit_limit = lit + numLiterals;
  
  UInt32 D = 0;

  for (;;)
  {
    if (numMatches == 0)
      break;
    numMatches--;

    FseInStream_FLUSH

    unsigned L = (unsigned)FseDecodeExtra(&l_state, l_decoder, &in);

    FseInStream_FLUSH
    
    unsigned M = (unsigned)FseDecodeExtra(&m_state, m_decoder, &in);

    FseInStream_FLUSH

    {
      UInt32 new_D = FseDecodeExtra(&d_state, d_decoder, &in);
      if (new_D)
        D = new_D;
    }

    if (L != 0)
    {
      if (L > (size_t)(lit_limit - lit))
        return S_FALSE;
      unsigned cur = L;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      m_OutWindowStream.PutBytes(lit, cur);
      unpackSize -= cur;
      lit += cur;
      if (cur != L)
        return S_FALSE;
    }

    if (M != 0)
    {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE;
    }
  }

  if (unpackSize != 0)
    return S_FALSE;

  // LZFSE encoder writes 8 additional zero bytes before LMD payload
  // We test it:
  if ((size_t)(buf - buf_start) * 8 + in.numBits != 64)
    return S_FALSE;
  if (GetUi64(buf_start) != 0)
    return S_FALSE;

  return S_OK;
}


HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  PRF(printf("\n\nLzfseDecoder %7u %7u\n", (unsigned)*outSize, (unsigned)*inSize));

  const UInt32 kLzfseDictSize = 1 << 18;
  if (!m_OutWindowStream.Create(kLzfseDictSize))
    return E_OUTOFMEMORY;
  if (!m_InStream.Create(1 << 18))
    return E_OUTOFMEMORY;

  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InStream.SetStream(inStream);
  m_InStream.Init();
  
  CCoderReleaser coderReleaser(this);

  UInt64 prevOut = 0;
  UInt64 prevIn = 0;

  if (LzvnMode)
  {
    if (!outSize || !inSize)
      return E_NOTIMPL;
    const UInt64 unpackSize = *outSize;
    const UInt64 packSize = *inSize;
    if (unpackSize > (UInt32)(Int32)-1
        || packSize > (UInt32)(Int32)-1)
      return S_FALSE;
    RINOK(DecodeLzvn((UInt32)unpackSize, (UInt32)packSize))
  }
  else
  for (;;)
  {
    const UInt64 pos = m_OutWindowStream.GetProcessedSize();
    const UInt64 packPos = m_InStream.GetProcessedSize();

    if (progress && ((pos - prevOut) >= (1 << 22) || (packPos - prevIn) >= (1 << 22)))
    {
      RINOK(progress->SetRatioInfo(&packPos, &pos))
      prevIn = packPos;
      prevOut = pos;
    }

    UInt32 v;
    RINOK(GetUInt32(v))
    if ((v & 0xFFFFFF) != 0x787662) // bvx
      return S_FALSE;
    v >>= 24;
    
    if (v == 0x24) // '$', end of stream
      break;
    
    UInt32 unpackSize;
    RINOK(GetUInt32(unpackSize))

    UInt32 cur = unpackSize;
    if (outSize)
    {
      const UInt64 rem = *outSize - pos;
      if (cur > rem)
        cur = (UInt32)rem;
    }
    unpackSize -= cur;
    
    HRESULT res;
    if (v == kSignature_LZFSE_V1 || v == kSignature_LZFSE_V2)
      res = DecodeLzfse(cur, (Byte)v);
    else if (v == 0x6E) // 'n'
    {
      UInt32 packSize;
      res = GetUInt32(packSize);
      if (res == S_OK)
        res = DecodeLzvn(cur, packSize);
    }
    else if (v == 0x2D) // '-'
      res = DecodeUncompressed(cur);
    else
      return E_NOTIMPL;
    
    if (res != S_OK)
      return res;
    
    if (unpackSize != 0)
      return S_FALSE;
  }
  
  coderReleaser.NeedFlush = false;
  HRESULT res = m_OutWindowStream.Flush();
  if (res == S_OK)
    if ((!LzvnMode && inSize && *inSize != m_InStream.GetProcessedSize())
        || (outSize && *outSize != m_OutWindowStream.GetProcessedSize()))
      res = S_FALSE;
  return res;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return E_OUTOFMEMORY; }
  // catch(...) { return S_FALSE; }
}

}}
