// DeflateEncoder.h

#ifndef ZIP7_INC_DEFLATE_ENCODER_H
#define ZIP7_INC_DEFLATE_ENCODER_H

#include "../../../C/LzFind.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "BitlEncoder.h"
#include "DeflateConst.h"

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

struct CCodeValue
{
  UInt16 Len;
  UInt16 Pos;
  void SetAsLiteral() { Len = (1 << 15); }
  bool IsLiteral() const { return (Len >= (1 << 15)); }
};

struct COptimal
{
  UInt32 Price;
  UInt16 PosPrev;
  UInt16 BackPrev;
};

const UInt32 kNumOptsBase = 1 << 12;
const UInt32 kNumOpts = kNumOptsBase + kMatchMaxLen;

class CCoder;

struct CTables: public CLevels
{
  bool UseSubBlocks;
  bool StoreMode;
  bool StaticMode;
  UInt32 BlockSizeRes;
  UInt32 m_Pos;
  void InitStructures();
};


struct CEncProps
{
  int Level;
  int algo;
  int fb;
  int btMode;
  UInt32 mc;
  UInt32 numPasses;

  CEncProps()
  {
    Level = -1;
    mc = 0;
    algo = fb = btMode = -1;
    numPasses = (UInt32)(Int32)-1;
  }
  void Normalize();
};

class CCoder
{
  CMatchFinder _lzInWindow;
  CBitlEncoder m_OutStream;

public:
  CCodeValue *m_Values;

  UInt16 *m_MatchDistances;
  UInt32 m_NumFastBytes;
  bool _fastMode;
  bool _btMode;

  UInt16 *m_OnePosMatchesMemory;
  UInt16 *m_DistanceMemory;

  UInt32 m_Pos;

  unsigned m_NumPasses;
  unsigned m_NumDivPasses;
  bool m_CheckStatic;
  bool m_IsMultiPass;
  UInt32 m_ValueBlockSize;

  UInt32 m_NumLenCombinations;
  UInt32 m_MatchMaxLen;
  const Byte *m_LenStart;
  const Byte *m_LenDirectBits;

  bool m_Created;
  bool m_Deflate64Mode;

  Byte m_LevelLevels[kLevelTableSize];
  unsigned m_NumLitLenLevels;
  unsigned m_NumDistLevels;
  UInt32 m_NumLevelCodes;
  UInt32 m_ValueIndex;

  bool m_SecondPass;
  UInt32 m_AdditionalOffset;

  UInt32 m_OptimumEndIndex;
  UInt32 m_OptimumCurrentIndex;
  
  Byte  m_LiteralPrices[256];
  Byte  m_LenPrices[kNumLenSymbolsMax];
  Byte  m_PosPrices[kDistTableSize64];

  CLevels m_NewLevels;
  UInt32 mainFreqs[kFixedMainTableSize];
  UInt32 distFreqs[kDistTableSize64];
  UInt32 mainCodes[kFixedMainTableSize];
  UInt32 distCodes[kDistTableSize64];
  UInt32 levelCodes[kLevelTableSize];
  Byte levelLens[kLevelTableSize];

  UInt32 BlockSizeRes;

  CTables *m_Tables;
  COptimal m_Optimum[kNumOpts];

  UInt32 m_MatchFinderCycles;

  void GetMatches();
  void MovePos(UInt32 num);
  UInt32 Backward(UInt32 &backRes, UInt32 cur);
  UInt32 GetOptimal(UInt32 &backRes);
  UInt32 GetOptimalFast(UInt32 &backRes);

  void LevelTableDummy(const Byte *levels, unsigned numLevels, UInt32 *freqs);

  void WriteBits(UInt32 value, unsigned numBits);
  void LevelTableCode(const Byte *levels, unsigned numLevels, const Byte *lens, const UInt32 *codes);

  void MakeTables(unsigned maxHuffLen);
  UInt32 GetLzBlockPrice() const;
  void TryBlock();
  UInt32 TryDynBlock(unsigned tableIndex, UInt32 numPasses);

  UInt32 TryFixedBlock(unsigned tableIndex);

  void SetPrices(const CLevels &levels);
  void WriteBlock();

  HRESULT Create();
  void Free();

  void WriteStoreBlock(UInt32 blockSize, UInt32 additionalOffset, bool finalBlock);
  void WriteTables(bool writeMode, bool finalBlock);
  
  void WriteBlockData(bool writeMode, bool finalBlock);

  UInt32 GetBlockPrice(unsigned tableIndex, unsigned numDivPasses);
  void CodeBlock(unsigned tableIndex, bool finalBlock);

  void SetProps(const CEncProps *props2);
public:
  CCoder(bool deflate64Mode = false);
  ~CCoder();

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  HRESULT BaseSetEncoderProperties2(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
};


class CCOMCoder Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
  Z7_IFACES_IMP_UNK_2(ICompressCoder, ICompressSetCoderProperties)
public:
  CCOMCoder(): CCoder(false) {}
};

class CCOMCoder64 Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
  Z7_IFACES_IMP_UNK_2(ICompressCoder, ICompressSetCoderProperties)
public:
  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif
