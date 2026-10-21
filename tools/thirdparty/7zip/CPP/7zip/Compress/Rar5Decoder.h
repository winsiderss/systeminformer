// Rar5Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR5_DECODER_H
#define ZIP7_INC_COMPRESS_RAR5_DECODER_H

#include "../../Common/MyBuffer2.h"
#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "HuffmanDecoder.h"

namespace NCompress {
namespace NRar5 {

class CBitDecoder;

struct CFilter
{
  Byte Type;
  Byte Channels;
  UInt32 Size;
  UInt64 Start;
};


const unsigned kNumReps = 4;
const unsigned kLenTableSize = 11 * 4;
const unsigned kMainTableSize = 256 + 1 + 1 + kNumReps + kLenTableSize;
const unsigned kExtraDistSymbols_v7 = 16;
const unsigned kDistTableSize_v6 = 64;
const unsigned kDistTableSize_MAX = 64 + kExtraDistSymbols_v7;
const unsigned kNumAlignBits = 4;
const unsigned kAlignTableSize = 1 << kNumAlignBits;

const unsigned kNumHufBits = 15;

const unsigned k_NumHufTableBits_Main = 10;
const unsigned k_NumHufTableBits_Dist = 7;
const unsigned k_NumHufTableBits_Len = 7;
const unsigned k_NumHufTableBits_Align = 6;

const unsigned DICT_SIZE_BITS_MAX = 40;

Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _useAlignBits;
  bool _isLastBlock;
  bool _unpackSize_Defined;
  // bool _packSize_Defined;
  
  bool _unsupportedFilter;
  Byte _lzError;
  bool _writeError;

  bool _isSolid;
  // bool _solidAllowed;
  bool _is_v7;
  bool _tableWasFilled;
  bool _wasInit;

  Byte _exitType;

  // Byte _dictSizeLog;
  size_t _dictSize;
  Byte *_window;
  size_t _winPos;
  size_t _winSize;
  size_t _dictSize_forCheck;
  size_t _limit;
  const Byte *_buf_Res;
  UInt64 _lzSize;
  size_t _reps[kNumReps];
  unsigned _bitPos_Res;
  UInt32 _lastLen;

  // unsigned _numCorrectDistSymbols;
  unsigned _numUnusedFilters;
  unsigned _numFilters;

  UInt64 _lzWritten;
  UInt64 _lzFileStart;
  UInt64 _unpackSize;
  // UInt64 _packSize;
  UInt64 _lzEnd;
  UInt64 _writtenFileSize;
  UInt64 _filterEnd;
  UInt64 _progress_Pack;
  UInt64 _progress_Unpack;
  CAlignedBuffer _filterSrc;
  CAlignedBuffer _filterDst;

  CFilter *_filters;
  size_t _winSize_Allocated;
  ISequentialInStream *_inStream;
  ISequentialOutStream *_outStream;
  ICompressProgressInfo *_progress;
  Byte *_inputBuf;

  NHuffman::CDecoder<kNumHufBits, kMainTableSize,  k_NumHufTableBits_Main>  m_MainDecoder;
  NHuffman::CDecoder256<kNumHufBits, kDistTableSize_MAX,  k_NumHufTableBits_Dist>  m_DistDecoder;
  NHuffman::CDecoder256<kNumHufBits, kAlignTableSize,     k_NumHufTableBits_Align> m_AlignDecoder;
  NHuffman::CDecoder256<kNumHufBits, kLenTableSize,       k_NumHufTableBits_Len>   m_LenDecoder;
  Byte m_LenPlusTable[DICT_SIZE_BITS_MAX];

  void InitFilters()
  {
    _numUnusedFilters = 0;
    _numFilters = 0;
  }
  void DeleteUnusedFilters();
  HRESULT WriteData(const Byte *data, size_t size);
  HRESULT ExecuteFilter(const CFilter &f);
  HRESULT WriteBuf();
  HRESULT AddFilter(CBitDecoder &_bitStream);
  HRESULT ReadTables(CBitDecoder &_bitStream);
  HRESULT DecodeLZ2(const CBitDecoder &_bitStream) throw();
  HRESULT DecodeLZ();
  HRESULT CodeReal();
public:
  CDecoder();
  ~CDecoder();
};

}}

#endif
