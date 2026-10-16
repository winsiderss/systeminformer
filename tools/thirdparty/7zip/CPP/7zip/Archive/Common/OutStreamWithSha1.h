// OutStreamWithSha1.h

#ifndef ZIP7_INC_OUT_STREAM_WITH_SHA1_H
#define ZIP7_INC_OUT_STREAM_WITH_SHA1_H

#include "../../../../C/Sha1.h"

#include "../../../Common/MyBuffer2.h"
#include "../../../Common/MyCom.h"

#include "../../IStream.h"

Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithSha1
  , ISequentialOutStream
)
  bool _calculate;
  CMyComPtr<ISequentialOutStream> _stream;
  CAlignedBuffer1 _sha;
  UInt64 _size;

  CSha1 *Sha() { return (CSha1 *)(void *)(Byte *)_sha; }
public:
  COutStreamWithSha1(): _sha(sizeof(CSha1)) {}
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(bool calculate = true)
  {
    _calculate = calculate;
    _size = 0;
    Sha1_Init(Sha());
  }
  void InitSha1() { Sha1_Init(Sha()); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { Sha1_Final(Sha(), digest); }
};


Z7_CLASS_IMP_NOQIB_1(
  CInStreamWithSha1
  , ISequentialInStream
)
  CMyComPtr<ISequentialInStream> _stream;
  CAlignedBuffer1 _sha;
  UInt64 _size;

  CSha1 *Sha() { return (CSha1 *)(void *)(Byte *)_sha; }
public:
  CInStreamWithSha1(): _sha(sizeof(CSha1)) {}
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    Sha1_Init(Sha());
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { Sha1_Final(Sha(), digest); }
};

#endif
