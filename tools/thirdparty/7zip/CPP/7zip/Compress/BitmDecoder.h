// BitmDecoder.h -- the Most Significant Bit of byte is First

#ifndef ZIP7_INC_BITM_DECODER_H
#define ZIP7_INC_BITM_DECODER_H

#include "../IStream.h"

namespace NBitm {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8 * kNumValueBytes;

const UInt32 kMask = (1 << kNumValueBits) - 1;

// _bitPos - the number of free bits (high bits in _value)
// (kNumBigValueBits - _bitPos) = (32 - _bitPos) == the number of ready to read bits (low bits of _value)

template<class TInByte>
class CDecoder
{
  unsigned _bitPos;
  UInt32 _value;
  TInByte _stream;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { _stream.SetStream(inStream);}

  void Init()
  {
    _stream.Init();
    _bitPos = kNumBigValueBits;
    _value = 0;
    Normalize();
  }
  
  UInt64 GetStreamSize() const { return _stream.GetStreamSize(); }
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }

  bool ExtraBitsWereRead() const
  {
    return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
  }

  bool ExtraBitsWereRead_Fast() const
  {
    return (_stream.NumExtraBytes > 4);
  }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; _bitPos >= 8; _bitPos -= 8)
      _value = (_value << 8) | _stream.ReadByte();
  }

  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits) const
  {
    // return (_value << _bitPos) >> (kNumBigValueBits - numBits);
    return ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits() const
  {
    return this->_value << this->_bitPos;
  }

  Z7_FORCE_INLINE
  void MovePos(unsigned numBits)
  {
    _bitPos += numBits;
    Normalize();
  }

  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }

  /*
  unsigned ReadBit()
  {
    UInt32 res = ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - 1);
    if (++_bitPos >= 8)
    {
      _value = (_value << 8) | _stream.ReadByte();
      _bitPos -= 8;
    }
    return (unsigned)res;
  }
  */

  void AlignToByte() { MovePos((kNumBigValueBits - _bitPos) & 7); }

  Z7_FORCE_INLINE
  UInt32 ReadAlignBits() { return ReadBits((kNumBigValueBits - _bitPos) & 7); }
};

}

#endif
