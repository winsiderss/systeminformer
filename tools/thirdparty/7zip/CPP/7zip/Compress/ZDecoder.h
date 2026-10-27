// ZDecoder.h

#ifndef ZIP7_INC_COMPRESS_Z_DECODER_H
#define ZIP7_INC_COMPRESS_Z_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCompress {
namespace NZ {

// Z decoder decodes Z data stream, including 3 bytes of header.
  
class CDecoder
{
  UInt16 *_parents;
  Byte *_suffixes;
  Byte *_stack;
  unsigned _numMaxBits;

public:
  CDecoder(): _parents(NULL), _suffixes(NULL), _stack(NULL), /* _prop(0), */ _numMaxBits(0) {}
  ~CDecoder();
  void Free();
  // UInt64 PackSize;

  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    ICompressProgressInfo *progress);
};

/*
  There is no end_of_payload_marker in Z stream.
  Z decoder stops decoding, if it reaches end of input stream.
   
  CheckStream function:
    (size) must be at least 3 bytes (size of Z header).
    if (size) is larger than size of real Z stream in (data), CheckStream can return false.
*/

const unsigned kRecommendedCheckSize = 64;

bool CheckStream(const Byte *data, size_t size);

}}

#endif
