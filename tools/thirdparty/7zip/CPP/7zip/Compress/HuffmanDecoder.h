// Compress/HuffmanDecoder.h

#ifndef ZIP7_INC_COMPRESS_HUFFMAN_DECODER_H
#define ZIP7_INC_COMPRESS_HUFFMAN_DECODER_H

#include "../../../C/CpuArch.h"
#include "../../Common/MyTypes.h"

namespace NCompress {
namespace NHuffman {

// const unsigned kNumTableBits_Default = 9;

#if 0 || 0 && defined(MY_CPU_64BIT)
// for debug or optimization:
// 64-BIT limit array can be faster for some compilers.
// for debug or optimization:
#define Z7_HUFF_USE_64BIT_LIMIT
#else
// sizet value variable allows to eliminate some move operation in some compilers.
// for debug or optimization:
// #define Z7_HUFF_USE_SIZET_VALUE
#endif

// v0 must normalized to (32 bits) : (v0 < ((UInt64)1 << 32))

#ifdef Z7_HUFF_USE_64BIT_LIMIT
  typedef UInt64 CLimitInt;
  typedef UInt64 CValueInt;
  // all _limits[*] are normalized and limited by ((UInt64)1 << 32).
  // we don't use (v1) in this branch
  #define Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)  32
  #define Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1) \
      ((NCompress::NHuffman::CLimitInt)v0 >= (huf)->_limits[0])
  #define Z7_HUFF_GET_VAL_FOR_LIMITS(v0, v1, kNumBitsMax, kNumTableBits)  (v0)
  #define Z7_HUFF_GET_VAL_FOR_TABLE( v0, v1, kNumBitsMax, kNumTableBits)  ((v0) >> (32 - kNumTableBits))
  #define Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1)
#else
  typedef UInt32 CLimitInt;
  typedef
    #ifdef Z7_HUFF_USE_SIZET_VALUE
      size_t
    #else
      UInt32
    #endif
      CValueInt;
  // v1 must be precalculated from v0 in this branch
  // _limits[0] and (v1) are normalized and limited by (1 << kNumTableBits).
  // _limits[non_0]      are normalized and limited by (1 << kNumBitsMax).
  #define Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)  (kNumBitsMax)
  #define Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1) \
      ((NCompress::NHuffman::CLimitInt)v1 >= (huf)->_limits[0])
  #define Z7_HUFF_GET_VAL_FOR_LIMITS(v0, v1, kNumBitsMax, kNumTableBits)  ((v0) >> (32 - kNumBitsMax))
  #define Z7_HUFF_GET_VAL_FOR_TABLE( v0, v1, kNumBitsMax, kNumTableBits)  (v1)
  #define Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1)  const UInt32 v1 = ((v0) >> (32 - kNumTableBits));
#endif


enum enum_BuildMode
{
  k_BuildMode_Partial       = 0,
  k_BuildMode_Full          = 1,
  k_BuildMode_Full_or_Empty = 2
};


template <class symType, class symType2, class symType4, unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = kNumTableBits_Default */>
struct CDecoderBase
{
  CLimitInt _limits[kNumBitsMax + 2 - kNumTableBits];
  UInt32 _poses[kNumBitsMax - kNumTableBits]; // unsigned
union
{
  // if  defined(MY_CPU_64BIT), we need 64-bit alignment for _symbols.
  // if !defined(MY_CPU_64BIT), we need 32-bit alignment for _symbols
  // but we provide alignment for _lens.
  // _symbols also will be aligned, if _lens are aligned
  #if defined(MY_CPU_64BIT)
    UInt64
  #else
    UInt32
  #endif
    _pad_align[m_NumSymbols < (1u << sizeof(symType) * 8) ? 1 : -1];
  /* if symType is Byte, we use 16-bytes padding to avoid cache
     bank conflict between _lens and _symbols: */
  Byte _lens[(1 << kNumTableBits) + (sizeof(symType) == 1 ? 16 : 0)];
} _u;
  symType _symbols[(1 << kNumTableBits) + m_NumSymbols - (kNumTableBits + 1)];

  /*
  Z7_FORCE_INLINE
  bool IsFull() const
  {
    return _limits[kNumBitsMax - kNumTableBits] ==
        (CLimitInt)1u << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax);
  }
  Z7_FORCE_INLINE
  bool IsEmpty() const
  {
    return _limits[kNumBitsMax - kNumTableBits] == 0;
  }
  Z7_FORCE_INLINE
  bool Is_Full_or_Empty() const
  {
    return 0 == (_limits[kNumBitsMax - kNumTableBits] &
        ~((CLimitInt)1 << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax)));
  }
  */

  Z7_FORCE_INLINE
  bool Build(const Byte *lens, enum_BuildMode buidMode = k_BuildMode_Partial) throw()
  {
    unsigned counts[kNumBitsMax + 1];
    size_t i;
    for (i = 0; i <= kNumBitsMax; i++)
      counts[i] = 0;
    for (i = 0; i < m_NumSymbols; i++)
      counts[lens[i]]++;
    
    UInt32 sum = 0;
    for (i = 1; i <= kNumTableBits; i++)
    {
      sum <<= 1;
      sum += counts[i];
    }

    CLimitInt startPos = (CLimitInt)sum;
    _limits[0] =
      #ifdef Z7_HUFF_USE_64BIT_LIMIT
          startPos << (Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - kNumTableBits);
      #else
          startPos;
      #endif

    for (i = kNumTableBits + 1; i <= kNumBitsMax; i++)
    {
      startPos <<= 1;
      _poses[i - (kNumTableBits + 1)] = (UInt32)(startPos - sum);
      const unsigned cnt = counts[i];
      counts[i] = sum;
      sum += cnt;
      startPos += cnt;
      _limits[i - kNumTableBits] = startPos << (Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - i);
    }

    _limits[kNumBitsMax + 1 - kNumTableBits] =
        (CLimitInt)1 << Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax);

    if (buidMode == k_BuildMode_Partial)
    {
      if (startPos > (1u << kNumBitsMax)) return false;
    }
    else
    {
      if (buidMode != k_BuildMode_Full && startPos == 0) return true;
      if (startPos != (1u << kNumBitsMax)) return false;
    }
    size_t sum2 = 0;
    for (i = 1; i <= kNumTableBits; i++)
    {
      const unsigned cnt = counts[i] << (kNumTableBits - i);
      counts[i] = (unsigned)sum2 >> (kNumTableBits - i);
      memset(_u._lens + sum2, (int)i, cnt);
      sum2 += cnt;
    }
    
#ifdef MY_CPU_64BIT
    symType4
    // UInt64 // for symType = UInt16
    // UInt32 // for symType = Byte
#else
    UInt32
#endif
    v = 0;
    for (i = 0; i < m_NumSymbols; i++,
      v +=
          1
          + (  (UInt32)1 << (sizeof(symType) * 8 * 1))
          // 0x00010001 // for symType = UInt16
          // 0x00000101 // for symType = Byte
#ifdef MY_CPU_64BIT
          + ((symType4)1 << (sizeof(symType) * 8 * 2))
          + ((symType4)1 << (sizeof(symType) * 8 * 3))
          // 0x0001000100010001 // for symType = UInt16
          // 0x0000000001010101 // for symType = Byte
#endif
      )
    {
      const unsigned len = lens[i];
      if (len == 0)
        continue;
      const size_t offset = counts[len];
      counts[len] = (unsigned)offset + 1;
      if (len >= kNumTableBits)
        _symbols[offset] = (symType)v;
      else
      {
        Byte *s2 = (Byte *)(void *)_symbols + (offset <<
            (kNumTableBits + sizeof(symType) / 2 - len));
        Byte *lim = s2 + ((size_t)1 <<
            (kNumTableBits + sizeof(symType) / 2 - len));
        if (len >= kNumTableBits - 2)
        {
          *(symType2 *)(void *)(s2                       ) = (symType2)v;
          *(symType2 *)(void *)(lim - sizeof(symType) * 2) = (symType2)v;
        }
        else
        {
#ifdef MY_CPU_64BIT
          symType4 *s = (symType4 *)(void *)s2;
          do
          {
            s[0] = v;  s[1] = v;  s += 2;
          }
          while (s != (const symType4 *)(const void *)lim);
#else
          symType2 *s = (symType2 *)(void *)s2;
          do
          {
            s[0] = (symType2)v;  s[1] = (symType2)v;  s += 2;
            s[0] = (symType2)v;  s[1] = (symType2)v;  s += 2;
          }
          while (s != (const symType2 *)(const void *)lim);
#endif
        }
      }
    }
    return true;
  }


#define Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES(_numBits_, kNumBitsMax, error_op)  if (_numBits_ > kNumBitsMax) { error_op }
#define Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO( _numBits_, kNumBitsMax, error_op)

  
#define Z7_HUFF_DECODE_BASE_TREE_BRANCH(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_limits, \
      check_op, error_op, _numBits_) \
{ \
    const NHuffman::CValueInt _val = get_val_for_limits(v0, v1, kNumBitsMax, kNumTableBits); \
    _numBits_ = kNumTableBits + 1; \
    if ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[1]) \
    do { _numBits_++; } \
    while ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[_numBits_ - kNumTableBits]); \
    check_op(_numBits_, kNumBitsMax, error_op) \
    sym = (huf)->_symbols[(/* (UInt32) */ (_val >> ((Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - (unsigned)_numBits_)))) \
        - (huf)->_poses[_numBits_ - (kNumTableBits + 1)]]; \
}

/*
    Z7_HUFF_DECODE_BASE_TREE_BRANCH(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_limits, \
      check_op, error_op, _numBits_) \

*/

#define Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      get_val_for_table, get_val_for_limits, \
      check_op, error_op, move_pos_op, after_op, bs) \
{ \
  if (Z7_HUFF_TABLE_COMPARE(huf, kNumTableBits, v0, v1)) \
  { \
    const NHuffman::CValueInt _val = get_val_for_limits(v0, v1, kNumBitsMax, kNumTableBits); \
    size_t _numBits_ = kNumTableBits + 1; \
    if ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[1]) \
    do { _numBits_++; } \
    while ((NCompress::NHuffman::CLimitInt)_val >= (huf)->_limits[_numBits_ - kNumTableBits]); \
    check_op(_numBits_, kNumBitsMax, error_op) \
    sym = (huf)->_symbols[(/* (UInt32) */ (_val >> ((Z7_HUFF_NUM_LIMIT_BITS(kNumBitsMax) - (unsigned)_numBits_)))) \
        - (huf)->_poses[_numBits_ - (kNumTableBits + 1)]]; \
    move_pos_op(bs, _numBits_); \
  } \
  else \
  { \
    const size_t _val = get_val_for_table(v0, v1, kNumBitsMax, kNumTableBits); \
    const size_t _numBits_ = (huf)->_u._lens[_val]; \
    sym = (huf)->_symbols[_val]; \
    move_pos_op(bs, _numBits_); \
  } \
  after_op \
}

#define Z7_HUFF_DECODE_10(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      check_op, error_op, move_pos_op, after_op, bs) \
    Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, v1, \
      Z7_HUFF_GET_VAL_FOR_TABLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS, \
      check_op, error_op, move_pos_op, after_op, bs) \


#define Z7_HUFF_DECODE_VAL_IN_HIGH32(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, \
      check_op, error_op, move_pos_op, after_op, bs) \
{ \
    Z7_HUFF_PRECALC_V1(kNumTableBits, v0, _v1_temp) \
    Z7_HUFF_DECODE_10(sym, huf, kNumBitsMax, kNumTableBits,  \
      v0, _v1_temp, \
      check_op, error_op, move_pos_op, after_op, bs) \
}

#if 0 || defined(Z7_HUFF_USE_64BIT_LIMIT)
// this branch uses bitStream->GetValue_InHigh32bits().
#define Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, \
      check_op, error_op, move_pos_op) \
{ \
  const UInt32 v0 = (bitStream)->GetValue_InHigh32bits(); \
  Z7_HUFF_PRECALC_V1(kNumTableBits, v0, v1); \
  Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits, \
      v0, v1,  \
      Z7_HUFF_GET_VAL_FOR_TABLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS, \
       check_op, error_op, move_pos_op, {}, bitStream) \
}
#else
/*
this branch uses bitStream->GetValue().
So we use SIMPLE versions for v0, v1 calculation:
  v0 is normalized for kNumBitsMax
  v1 is normalized for kNumTableBits
*/
#define Z7_HUFF_GET_VAL_FOR_LIMITS_SIMPLE(v0, v1, kNumBitsMax, kNumTableBits)  v0
#define Z7_HUFF_GET_VAL_FOR_TABLE_SIMPLE( v0, v1, kNumBitsMax, kNumTableBits)  v1
#define Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, move_pos_op) \
{ \
  const UInt32 v0 = (bitStream)->GetValue(kNumBitsMax); \
  const UInt32 v1 = v0 >> (kNumBitsMax - kNumTableBits); \
  Z7_HUFF_DECODE_BASE(sym, huf, kNumBitsMax, kNumTableBits, \
      v0, v1,  \
      Z7_HUFF_GET_VAL_FOR_TABLE_SIMPLE, \
      Z7_HUFF_GET_VAL_FOR_LIMITS_SIMPLE, \
      check_op, error_op, move_pos_op, {}, bitStream) \
}
#endif

#define Z7_HUFF_bitStream_MovePos(bitStream, numBits)  (bitStream)->MovePos((unsigned)(numBits))

#define Z7_HUFF_DECODE_1(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op) \
        Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, \
          Z7_HUFF_bitStream_MovePos)

// MovePosCheck

#define Z7_HUFF_DECODE_2(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op) \
        Z7_HUFF_DECODE_0(sym, huf, kNumBitsMax, kNumTableBits, bitStream, check_op, error_op, \
          Z7_HUFF_bitStream_MovePos)

// MovePosCheck

#define Z7_HUFF_DECODE_CHECK(sym, huf, kNumBitsMax, kNumTableBits, bitStream, error_op) \
        Z7_HUFF_DECODE_1(    sym, huf, kNumBitsMax, kNumTableBits, bitStream, \
        Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES, error_op)

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  bool Decode2(TBitDecoder *bitStream, unsigned &sym) const
  {
    Z7_HUFF_DECODE_CHECK(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      { return false; }
    )
    return true;
  }

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  bool Decode_SymCheck_MovePosCheck(TBitDecoder *bitStream, unsigned &sym) const
  {
    Z7_HUFF_DECODE_0(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES,
      { return false; },
      { return (bitStream)->MovePosCheck; }
    )
  }

  template <class TBitDecoder>
  Z7_FORCE_INLINE
  unsigned Decode(TBitDecoder *bitStream) const
  {
    unsigned sym;
    Z7_HUFF_DECODE_CHECK(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      { return (unsigned)(int)(Int32)0xffffffff; }
    )
    return sym;
  }

  
  template <class TBitDecoder>
  Z7_FORCE_INLINE
  unsigned DecodeFull(TBitDecoder *bitStream) const
  {
    /*
    const UInt32 val = bitStream->GetValue(kNumBitsMax);
    if (val < _limits[kNumTableBits])
    {
      const unsigned pair = _u._lens[(size_t)(val >> (kNumBitsMax - kNumTableBits))];
      bitStream->MovePos(pair & kPairLenMask);
      return pair >> kNumPairLenBits;
    }

    unsigned numBits;
    for (numBits = kNumTableBits + 1; val >= _limits[numBits]; numBits++);
    
    bitStream->MovePos(numBits);
    return _symbols[_poses[numBits] + (unsigned)
        ((val - _limits[(size_t)numBits - 1]) >> (kNumBitsMax - numBits))];
    */
    unsigned sym;
    Z7_HUFF_DECODE_2(sym, this, kNumBitsMax, kNumTableBits, bitStream,
      Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO, {}
    )
    return sym;
  }
};


template <unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = kNumTableBits_Default */>
struct CDecoder: public CDecoderBase
  <UInt16, UInt32, UInt64, kNumBitsMax, m_NumSymbols, kNumTableBits> {};

template <unsigned kNumBitsMax, unsigned m_NumSymbols, unsigned kNumTableBits /* = 7 */>
struct CDecoder256: public CDecoderBase
  <Byte, UInt16, UInt32, kNumBitsMax, m_NumSymbols, kNumTableBits> {};


template <unsigned numSymbols>
class CDecoder7b
{
public:
  Byte _lens[1 << 7];

  bool Build(const Byte *lens, bool full) throw()
  {
    const unsigned kNumBitsMax = 7;
    
    unsigned counts[kNumBitsMax + 1];
    unsigned _poses[kNumBitsMax + 1];
    unsigned _limits[kNumBitsMax + 1];
    unsigned i;
    for (i = 0; i <= kNumBitsMax; i++)
      counts[i] = 0;
    for (i = 0; i < numSymbols; i++)
      counts[lens[i]]++;
    
    _limits[0] = 0;
    const unsigned kMaxValue = 1u << kNumBitsMax;
    unsigned startPos = 0;
    unsigned sum = 0;
    
    for (i = 1; i <= kNumBitsMax; i++)
    {
      const unsigned cnt = counts[i];
      startPos += cnt << (kNumBitsMax - i);
      _limits[i] = startPos;
      counts[i] = sum;
      _poses[i] = sum;
      sum += cnt;
    }

    counts[0] = sum;
    _poses[0] = sum;

    if (full)
    {
      if (startPos != kMaxValue)
        return false;
    }
    else
    {
      if (startPos > kMaxValue)
        return false;
    }


    for (i = 0; i < numSymbols; i++)
    {
      const unsigned len = lens[i];
      if (len == 0)
        continue;
      const unsigned offset = counts[len]++;
      {
        Byte *dest = _lens + _limits[(size_t)len - 1]
            + ((offset - _poses[len]) << (kNumBitsMax - len));
        const unsigned num = (unsigned)1 << (kNumBitsMax - len);
        const unsigned val = (i << 3) + len;
        for (unsigned k = 0; k < num; k++)
          dest[k] = (Byte)val;
      }
    }

    if (!full)
    {
      const unsigned limit = _limits[kNumBitsMax];
      const unsigned num = ((unsigned)1 << kNumBitsMax) - limit;
      Byte *dest = _lens + limit;
      for (unsigned k = 0; k < num; k++)
        dest[k] = (Byte)
          // (0x1f << 3);
          ((0x1f << 3) + 0x7);
    }
    
    return true;
  }

#define Z7_HUFF_DECODER_7B_DECODE(dest, huf, get_val, move_pos, bs) \
  { \
    const unsigned pair = huf->_lens[(size_t)get_val(7)]; \
    const unsigned numBits = pair & 0x7; \
    move_pos(bs, numBits); \
    dest = pair >> 3; \
  }

  template <class TBitDecoder>
  unsigned Decode(TBitDecoder *bitStream) const
  {
    const unsigned pair = _lens[(size_t)bitStream->GetValue(7)];
    bitStream->MovePos(pair & 0x7);
    return pair >> 3;
  }
};

}}

#endif
