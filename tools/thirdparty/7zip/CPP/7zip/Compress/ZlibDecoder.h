// ZlibDecoder.h

#ifndef ZIP7_INC_ZLIB_DECODER_H
#define ZIP7_INC_ZLIB_DECODER_H

#include "DeflateDecoder.h"

namespace NCompress {
namespace NZlib {

const UInt32 ADLER_INIT_VAL = 1;

Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithAdler
  , ISequentialOutStream
)
  UInt32 _adler;
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _adler = ADLER_INIT_VAL; _size = 0; }
  UInt32 GetAdler() const { return _adler; }
  UInt64 GetSize() const { return _size; }
};

Z7_CLASS_IMP_NOQIB_1(
  CDecoder
  , ICompressCoder
)
  CMyComPtr2<ISequentialOutStream, COutStreamWithAdler> AdlerStream;
  CMyComPtr2<ICompressCoder, NDeflate::NDecoder::CCOMCoder> DeflateDecoder;
  Int32 _inputProcessedSize_Additional;
public:
  bool IsAdlerOptional;
  
  CDecoder(): IsAdlerOptional(false) {}
  UInt64 GetInputProcessedSize() const
  {
    return (UInt64)(
      (Int64)DeflateDecoder->GetInputProcessedSize() +
      (Int64)_inputProcessedSize_Additional);
  }
  UInt64 GetOutputProcessedSize() const { return AdlerStream->GetSize(); }
};

static bool inline IsZlib(const Byte *p)
{
  if ((p[0] & 0xF) != 8) // method
    return false;
  if (((unsigned)p[0] >> 4) > 7) // logar_window_size minus 8.
    return false;
  if ((p[1] & 0x20) != 0) // dictPresent
    return false;
  if ((((UInt32)p[0] << 8) + p[1]) % 31 != 0)
    return false;
  return true;
}

// IsZlib_3bytes checks 2 bytes of zlib header and starting byte of Deflate stream

static bool inline IsZlib_3bytes(const Byte *p)
{
  if (!IsZlib(p))
    return false;
  const unsigned val = p[2];
  const unsigned blockType = (val >> 1) & 0x3;
  if (blockType == 3) // unsupported block type for deflate
    return false;
  if (blockType == NCompress::NDeflate::NBlockType::kStored && (val >> 3) != 0)
    return false;
  return true;
}

}}

#endif
