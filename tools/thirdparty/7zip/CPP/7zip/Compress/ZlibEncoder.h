// ZlibEncoder.h

#ifndef ZIP7_INC_ZLIB_ENCODER_H
#define ZIP7_INC_ZLIB_ENCODER_H

#include "DeflateEncoder.h"

namespace NCompress {
namespace NZlib {

Z7_CLASS_IMP_NOQIB_1(
  CInStreamWithAdler
  , ISequentialInStream
)
  CMyComPtr<ISequentialInStream> _stream;
  UInt32 _adler;
  UInt64 _size;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _adler = 1; _size = 0; } // ADLER_INIT_VAL
  UInt32 GetAdler() const { return _adler; }
  UInt64 GetSize() const { return _size; }
};

Z7_CLASS_IMP_NOQIB_1(
  CEncoder
  , ICompressCoder
)
  CInStreamWithAdler *AdlerSpec;
  CMyComPtr<ISequentialInStream> AdlerStream;
  CMyComPtr<ICompressCoder> DeflateEncoder;
public:
  NCompress::NDeflate::NEncoder::CCOMCoder *DeflateEncoderSpec;
  
  void Create();
  UInt64 GetInputProcessedSize() const { return AdlerSpec->GetSize(); }
};

}}

#endif
