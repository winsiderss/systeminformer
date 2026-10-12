// LzhDecoder.h

#ifndef ZIP7_INC_COMPRESS_LZH_DECODER_H
#define ZIP7_INC_COMPRESS_LZH_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "HuffmanDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NLzh {
namespace NDecoder {

const unsigned kMatchMinLen = 3;
const unsigned kMatchMaxLen = 256;
const unsigned NC = 256 + kMatchMaxLen - kMatchMinLen + 1;
const unsigned NUM_CODE_BITS = 16;
const unsigned NUM_DIC_BITS_MAX = 25;
const unsigned NT = NUM_CODE_BITS + 3;
const unsigned NP = NUM_DIC_BITS_MAX + 1;
const unsigned NPT = NP; // Max(NT, NP)

class CCoder
{
  CLzOutWindow _outWindow;
  NBitm::CDecoder<CInBuffer> _inBitStream;

  int _symbolT;
  int _symbolC;
  UInt32 DictSize;
  // bool FinishMode;

  NHuffman::CDecoder256<NUM_CODE_BITS, NPT, 7> _decoderT;
  NHuffman::CDecoder<NUM_CODE_BITS, NC, 10> _decoderC;

  class CCoderReleaser
  {
    CCoder *_coder;
  public:
    CCoderReleaser(CCoder *coder): _coder(coder) {}
    void Disable() { _coder = NULL; }
    ~CCoderReleaser() { if (_coder) _coder->_outWindow.Flush(); }
  };
  friend class CCoderReleaser;

  bool ReadTP(unsigned num, unsigned numBits, int spec);
  bool ReadC();

  HRESULT CodeReal(UInt32 outSize, ICompressProgressInfo *progress);
public:
  CCoder(): DictSize(1 << 16)
      // , FinishMode(true)
      {}
  void SetDictSize(UInt32 dictSize) { DictSize = dictSize; }
  UInt64 GetInputProcessedSize() const { return _inBitStream.GetProcessedSize(); }
  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      UInt32 outSize, ICompressProgressInfo *progress);
};

}}}

#endif
