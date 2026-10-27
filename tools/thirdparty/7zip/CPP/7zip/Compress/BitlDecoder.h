// BitlDecoder.h -- the Least Significant Bit of byte is First

#ifndef ZIP7_INC_BITL_DECODER_H
#define ZIP7_INC_BITL_DECODER_H

#include "../../../C/CpuArch.h"

#include "../IStream.h"

namespace NBitl {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8 * kNumValueBytes;
const UInt32 kMask = (1 << kNumValueBits) - 1;

#if !defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
#if 1 && defined(MY_CPU_ARM_OR_ARM64) \
    && (defined(MY_CPU_ARM64) || defined(__ARM_ARCH_6T2__) \
       || defined(__ARM_ARCH) && (__ARM_ARCH >= 7)) \
    && (defined(__GNUC__) && (__GNUC__ >= 4) \
       || defined(__clang__) && (__clang_major__ >= 4))
  #define Z7_BITL_USE_REVERSE_BITS_INSTRUCTION
#elif 1
  #define Z7_BITL_USE_REVERSE_BITS_TABLE
#endif
#endif

#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
extern Byte kReverseTable[256];
#endif

inline unsigned ReverseBits8(unsigned i)
{
#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)
  return kReverseTable[i];
#elif defined(Z7_BITL_USE_REVERSE_BITS_INSTRUCTION)
  // rbit is available in ARMv6T2 and above
  asm ("rbit "
#if defined(MY_CPU_ARM)
    "%0,%0"   // it uses default register size,
              // but we need 32-bit register here.
              // we must use it only if default register size is 32-bit.
              // it will work incorrectly for ARM64.
#else
    "%w0,%w0" // it uses 32-bit registers in ARM64.
              // compiler for (MY_CPU_ARM) can't compile it.
#endif
    : "+r" (i));
  return i >> 24;
#else
  unsigned
      x = ((i & 0x55) << 1) | ((i >> 1) & 0x55);
      x = ((x & 0x33) << 2) | ((x >> 2) & 0x33);
  return  ((x & 0x0f) << 4) |  (x >> 4);
#endif
}


/* TInByte must support "Extra Bytes" (bytes that can be read after the end of stream
   TInByte::ReadByte() returns 0xFF after the end of stream
   TInByte::NumExtraBytes contains the number "Extra Bytes"
       
   Bitl decoder can read up to 4 bytes ahead to internal buffer. */

template<class TInByte>
class CBaseDecoder
{
protected:
  unsigned _bitPos;
  UInt32 _value;
  TInByte _stream;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { _stream.SetStream(inStream); }
  void ClearStreamPtr() { _stream.ClearStreamPtr(); }
  void Init()
  {
    _stream.Init();
    _bitPos = kNumBigValueBits;
    _value = 0;
  }

  // the size of portion data in real stream that was already read from this object.
  // it doesn't include unused data in BitStream object buffer (up to 4 bytes)
  // it doesn't include unused data in TInByte buffers
  // it doesn't include virtual Extra bytes after the end of real stream data
  UInt64 GetStreamSize() const
  {
    return ExtraBitsWereRead() ?
        _stream.GetStreamSize():
        GetProcessedSize();
  }

  // the size of virtual data that was read from this object.
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }

  bool ThereAreDataInBitsBuffer() const { return this->_bitPos != kNumBigValueBits; }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; _bitPos >= 8; _bitPos -= 8)
      _value = ((UInt32)_stream.ReadByte() << (kNumBigValueBits - _bitPos)) | _value;
  }
  
  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = _value & ((1 << numBits) - 1);
    _bitPos += numBits;
    _value >>= numBits;
    return res;
  }

  bool ExtraBitsWereRead() const
  {
    return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
  }
  
  bool ExtraBitsWereRead_Fast() const
  {
    // full version is not inlined in vc6.
    // return _stream.NumExtraBytes != 0 && (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
    
    // (_stream.NumExtraBytes > 4) is fast overread detection. It's possible that
    // it doesn't return true, if small number of extra bits were read.
    return (_stream.NumExtraBytes > 4);
  }

  // it must be fixed !!! with extra bits
  // UInt32 GetNumExtraBytes() const { return _stream.NumExtraBytes; }
};

template<class TInByte>
class CDecoder: public CBaseDecoder<TInByte>
{
  UInt32 _normalValue;

public:
  void Init()
  {
    CBaseDecoder<TInByte>::Init();
    _normalValue = 0;
  }
  
  Z7_FORCE_INLINE
  void Normalize()
  {
    for (; this->_bitPos >= 8; this->_bitPos -= 8)
    {
      const unsigned b = this->_stream.ReadByte();
      _normalValue = ((UInt32)b << (kNumBigValueBits - this->_bitPos)) | _normalValue;
      this->_value = (this->_value << 8) | ReverseBits8(b);
    }
  }
  
  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits)
  {
    Normalize();
    return ((this->_value >> (8 - this->_bitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits()
  {
    Normalize();
    return this->_value << this->_bitPos;
  }
  
  Z7_FORCE_INLINE
  void MovePos(size_t numBits)
  {
    this->_bitPos += (unsigned)numBits;
    _normalValue >>= numBits;
  }
  
  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = _normalValue & ((1 << numBits) - 1);
    MovePos(numBits);
    return res;
  }

  void AlignToByte() { MovePos((32 - this->_bitPos) & 7); }
  
  Z7_FORCE_INLINE
  Byte ReadDirectByte() { return this->_stream.ReadByte(); }

  Z7_FORCE_INLINE
  size_t ReadDirectBytesPart(Byte *buf, size_t size) { return this->_stream.ReadBytesPart(buf, size); }
  
  Z7_FORCE_INLINE
  Byte ReadAlignedByte()
  {
    if (this->_bitPos == kNumBigValueBits)
      return this->_stream.ReadByte();
    Byte b = (Byte)(_normalValue & 0xFF);
    MovePos(8);
    return b;
  }

  // call it only if the object is aligned for byte.
  Z7_FORCE_INLINE
  bool ReadAlignedByte_FromBuf(Byte &b)
  {
    if (this->_stream.NumExtraBytes != 0)
      if (this->_stream.NumExtraBytes >= 4
          || kNumBigValueBits - this->_bitPos <= (this->_stream.NumExtraBytes << 3))
        return false;
    if (this->_bitPos == kNumBigValueBits)
      return this->_stream.ReadByte_FromBuf(b);
    b = (Byte)(_normalValue & 0xFF);
    MovePos(8);
    return true;
  }
};

}

#endif
