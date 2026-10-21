// BitmEncoder.h -- the Most Significant Bit of byte is First

#ifndef ZIP7_INC_BITM_ENCODER_H
#define ZIP7_INC_BITM_ENCODER_H

#include "../IStream.h"

template<class TOutByte>
class CBitmEncoder
{
  unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
  unsigned _curByte; // low (_bitPos) bits are zeros
                     // high (8 - _bitPos) bits are filled
  TOutByte _stream;
public:
  bool Create(UInt32 bufferSize) { return _stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { _stream.SetStream(outStream);}
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
  void Init()
  {
    _stream.Init();
    _bitPos = 8;
    _curByte = 0;
  }
  HRESULT Flush()
  {
    if (_bitPos < 8)
    {
      _stream.WriteByte((Byte)_curByte);
      _bitPos = 8;
      _curByte = 0;
    }
    return _stream.Flush();
  }

  // required condition: (value >> numBits) == 0
  // numBits == 0 is allowed
  void WriteBits(UInt32 value, unsigned numBits)
  {
    do
    {
      unsigned bp = _bitPos;
      unsigned curByte = _curByte;
      if (numBits < bp)
      {
        bp -= numBits;
        _curByte = curByte | (value << bp);
        _bitPos = bp;
        return;
      }
      numBits -= bp;
      const UInt32 hi = (value >> numBits);
      value -= (hi << numBits);
      _stream.WriteByte((Byte)(curByte | hi));
      _bitPos = 8;
      _curByte = 0;
    }
    while (numBits);
  }
  void WriteByte(unsigned b)
  {
    const unsigned bp = _bitPos;
    const unsigned a = _curByte | (b >> (8 - bp));
    _curByte = b << bp;
   _stream.WriteByte((Byte)a);
  }

  void WriteBytes(const Byte *data, size_t num)
  {
    const unsigned bp = _bitPos;
#if 1 // 1 for optional speed-optimized code branch
    if (bp == 8)
    {
      _stream.WriteBytes(data, num);
      return;
    }
#endif
    unsigned c = _curByte;
    const unsigned bp_rev = 8 - bp;
    for (size_t i = 0; i < num; i++)
    {
      const unsigned b = data[i];
      _stream.WriteByte((Byte)(c | (b >> bp_rev)));
      c = b << bp;
    }
    _curByte = c;
  }
};

#endif
