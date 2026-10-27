// PpmdZip.h

#ifndef ZIP7_INC_COMPRESS_PPMD_ZIP_H
#define ZIP7_INC_COMPRESS_PPMD_ZIP_H

#include "../../../C/Alloc.h"
#include "../../../C/Ppmd8.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/CWrappers.h"

namespace NCompress {
namespace NPpmdZip {

struct CBuf
{
  Byte *Buf;
  
  CBuf(): Buf(NULL) {}
  ~CBuf() { ::MidFree(Buf); }
  bool Alloc();
};


Z7_CLASS_IMP_NOQIB_3(
  CDecoder
  , ICompressCoder
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  bool _fullFileMode;
  CByteInBufWrap _inStream;
  CBuf _outStream;
  CPpmd8 _ppmd;
public:
  CDecoder(bool fullFileMode = true);
  ~CDecoder();
};


struct CEncProps
{
  UInt32 MemSizeMB;
  UInt32 ReduceSize;
  int Order;
  int Restor;
  
  CEncProps()
  {
    MemSizeMB = (UInt32)(Int32)-1;
    ReduceSize = (UInt32)(Int32)-1;
    Order = -1;
    Restor = -1;
  }
  void Normalize(int level);
};


Z7_CLASS_IMP_NOQIB_2(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
)
  CByteOutBufWrap _outStream;
  CBuf _inStream;
  CPpmd8 _ppmd;
  CEncProps _props;
public:
  CEncoder();
  ~CEncoder();
};

}}

#endif
