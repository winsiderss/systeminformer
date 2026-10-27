// ImplodeDecoder.h

#ifndef ZIP7_INC_COMPRESS_IMPLODE_DECODER_H
#define ZIP7_INC_COMPRESS_IMPLODE_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitlDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NImplode {
namespace NDecoder {

typedef NBitl::CDecoder<CInBuffer> CInBit;

const unsigned kNumHuffmanBits = 16;
const unsigned kMaxHuffTableSize = 1 << 8;

class CHuffmanDecoder
{
  UInt32 _limits[kNumHuffmanBits + 1];
  UInt32 _poses[kNumHuffmanBits + 1];
  Byte _symbols[kMaxHuffTableSize];
public:
  bool Build(const Byte *lens, unsigned numSymbols) throw();
  unsigned Decode(CInBit *inStream) const throw();
};


Z7_CLASS_IMP_NOQIB_4(
  CCoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  Byte _flags;
  bool _fullStreamMode;

  CLzOutWindow _outWindowStream;
  CInBit _inBitStream;
  
  CHuffmanDecoder _litDecoder;
  CHuffmanDecoder _lenDecoder;
  CHuffmanDecoder _distDecoder;

  bool BuildHuff(CHuffmanDecoder &table, unsigned numSymbols);
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
public:
  CCoder();
};

}}}

#endif
