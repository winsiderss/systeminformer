// LzxDecoder.h

#ifndef ZIP7_INC_LZX_DECODER_H
#define ZIP7_INC_LZX_DECODER_H

#include "HuffmanDecoder.h"
#include "Lzx.h"

namespace NCompress {
namespace NLzx {

const unsigned kAdditionalOutputBufSize = 32 * 2;

const unsigned kNumTableBits_Main = 11;
const unsigned kNumTableBits_Len = 8;

// if (kNumLenSymols_Big <= 256) we can  use NHuffman::CDecoder256
// if (kNumLenSymols_Big >  256) we must use NHuffman::CDecoder
// const unsigned kNumLenSymols_Big_Start = kNumLenSlots - 1 + kMatchMinLen;  // 8 - 1 + 2
const unsigned kNumLenSymols_Big_Start = 0;
// const unsigned kNumLenSymols_Big_Start = 0;
const unsigned kNumLenSymols_Big = kNumLenSymols_Big_Start + kNumLenSymbols;

#if 1
  // for smallest structure size:
  const unsigned kPosSlotOffset = 0;
#else
  // use virtual entries for mispredicted branches:
  const unsigned kPosSlotOffset = 256 / kNumLenSlots;
#endif

class CBitByteDecoder;

class CDecoder
{
public:
  UInt32 _pos;
  UInt32 _winSize;
  Byte *_win;

  bool _overDict;
  bool _isUncompressedBlock;
  bool _skipByte;
  bool _keepHistory;
  bool _keepHistoryForNext;
  bool _needAlloc;
  bool _wimMode;
  Byte _numDictBits;

  // unsigned _numAlignBits_PosSlots;
  // unsigned _numAlignBits;
  UInt32 _numAlignBits_Dist;
private:
  unsigned _numPosLenSlots;
  UInt32 _unpackBlockSize;

  UInt32 _writePos;

  UInt32 _x86_translationSize;
  UInt32 _x86_processedSize;
  Byte *_x86_buf;

  Byte *_unpackedData;
public:
  Byte  _extra[kPosSlotOffset + kNumPosSlots];
  UInt32 _reps[kPosSlotOffset + kNumPosSlots];

  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, kNumTableBits_Main> _mainDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kNumLenSymols_Big, kNumTableBits_Len> _lenDecoder;
  NHuffman::CDecoder7b<kAlignTableSize> _alignDecoder;
private:
  Byte _mainLevels[kMainTableSize];
  Byte _lenLevels[kNumLenSymols_Big];

  HRESULT Flush() throw();
  bool ReadTables(CBitByteDecoder &_bitStream) throw();

  HRESULT CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  HRESULT SetParams2(unsigned numDictBits) throw();
public:
  CDecoder() throw();
  ~CDecoder() throw();
  
  void Set_WimMode(bool wimMode) { _wimMode = wimMode; }
  void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  void Set_KeepHistoryForNext(bool keepHistoryForNext) { _keepHistoryForNext = keepHistoryForNext; }

  HRESULT Set_ExternalWindow_DictBits(Byte *win, unsigned numDictBits)
  {
    _needAlloc = false;
    _win = win;
    _winSize = (UInt32)1 << numDictBits;
    return SetParams2(numDictBits);
  }
  HRESULT Set_DictBits_and_Alloc(unsigned numDictBits) throw();

  HRESULT Code_WithExceedReadWrite(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  
  bool WasBlockFinished()     const { return _unpackBlockSize == 0; }
  const Byte *GetUnpackData() const { return _unpackedData; }
  UInt32 GetUnpackSize()      const { return _pos - _writePos; }
};

}}

#endif
