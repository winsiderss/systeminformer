// QuantumDecoder.h

#ifndef ZIP7_INC_COMPRESS_QUANTUM_DECODER_H
#define ZIP7_INC_COMPRESS_QUANTUM_DECODER_H

#include "../../Common/MyTypes.h"

namespace NCompress {
namespace NQuantum {

const unsigned kNumLitSelectorBits = 2;
const unsigned kNumLitSelectors = 1 << kNumLitSelectorBits;
const unsigned kNumLitSymbols = 1 << (8 - kNumLitSelectorBits);
const unsigned kNumMatchSelectors = 3;
const unsigned kNumSelectors = kNumLitSelectors + kNumMatchSelectors;
const unsigned kNumSymbolsMax = kNumLitSymbols; // 64

class CRangeDecoder;

class CModelDecoder
{
  unsigned NumItems;
  unsigned ReorderCount;
  Byte Vals[kNumSymbolsMax];
  UInt16 Freqs[kNumSymbolsMax + 1];
public:
  Byte _pad[64 - 10]; // for structure size alignment

  void Init(unsigned numItems, unsigned startVal);
  unsigned Decode(CRangeDecoder *rc);
};


class CDecoder
{
  UInt32 _winSize;
  UInt32 _winPos;
  UInt32 _winSize_allocated;
  bool _overWin;
  Byte *_win;
  unsigned _numDictBits;

  CModelDecoder m_Selector;
  CModelDecoder m_Literals[kNumLitSelectors];
  CModelDecoder m_PosSlot[kNumMatchSelectors];
  CModelDecoder m_LenSlot;

  void Init();
  HRESULT CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize);
public:
  HRESULT Code(const Byte *inData, size_t inSize, UInt32 outSize, bool keepHistory);
  HRESULT SetParams(unsigned numDictBits);

  CDecoder(): _win(NULL), _numDictBits(0) {}
  const Byte * GetDataPtr() const { return _win + _winPos; }
};

}}

#endif
