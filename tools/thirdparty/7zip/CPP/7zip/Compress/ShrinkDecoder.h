// ShrinkDecoder.h

#ifndef ZIP7_INC_COMPRESS_SHRINK_DECODER_H
#define ZIP7_INC_COMPRESS_SHRINK_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCompress {
namespace NShrink {

const unsigned kNumMaxBits = 13;
const unsigned kNumItems = 1 << kNumMaxBits;

Z7_CLASS_IMP_NOQIB_3(
  CDecoder
  , ICompressCoder
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  bool _fullStreamMode;
  UInt64 _inProcessed;

  UInt16 _parents[kNumItems];
  Byte _suffixes[kNumItems];
  Byte _stack[kNumItems];

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
};

}}

#endif
