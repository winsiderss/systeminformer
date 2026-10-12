// BitlEncoder.h -- the Least Significant Bit of byte is First

#ifndef ZIP7_INC_BITL_ENCODER_H
#define ZIP7_INC_BITL_ENCODER_H

#include "../Common/OutBuffer.h"

class CBitlEncoder
{
  COutBuffer _stream;
  unsigned _bitPos;
  Byte _curByte;
public:
  bool Create(UInt32 bufSize) { return _stream.Create(bufSize); }
  void SetStream(ISequentialOutStream *outStream) { _stream.SetStream(outStream); }
  // unsigned GetBitPosition() const { return (8 - _bitPos); }
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
  void Init()
  {
    _stream.Init();
    _bitPos = 8;
    _curByte = 0;
  }
  HRESULT Flush()
  {
    FlushByte();
    return _stream.Flush();
  }
  void FlushByte()
  {
    if (_bitPos < 8)
      _stream.WriteByte(_curByte);
    _bitPos = 8;
    _curByte = 0;
  }
  Z7_FORCE_INLINE
  void WriteBits(UInt32 value, unsigned numBits)
  {
    while (numBits > 0)
    {
      if (numBits < _bitPos)
      {
        _curByte |= (Byte)((value & ((1 << numBits) - 1)) << (8 - _bitPos));
        _bitPos -= numBits;
        return;
      }
      numBits -= _bitPos;
      _stream.WriteByte((Byte)(_curByte | (value << (8 - _bitPos))));
      value >>= _bitPos;
      _bitPos = 8;
      _curByte = 0;
    }
  }
  void WriteByte(Byte b) { _stream.WriteByte(b);}
};

#endif
