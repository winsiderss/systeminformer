// Rar3Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

/* This code uses Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#ifndef ZIP7_INC_COMPRESS_RAR3_DECODER_H
#define ZIP7_INC_COMPRESS_RAR3_DECODER_H

#include "../../../C/Ppmd7.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "HuffmanDecoder.h"
#include "Rar3Vm.h"

namespace NCompress {
namespace NRar3 {

const unsigned kNumHuffmanBits = 15;

const UInt32 kWindowSize = 1 << 22;
const UInt32 kWindowMask = kWindowSize - 1;

const unsigned kNumReps = 4;
const unsigned kNumLen2Symbols = 8;
const unsigned kLenTableSize = 28;
const unsigned kMainTableSize = 256 + 3 + kNumReps + kNumLen2Symbols + kLenTableSize;
const unsigned kDistTableSize = 60;

const unsigned kNumAlignBits = 4;
const unsigned kAlignTableSize = (1 << kNumAlignBits) + 1;

const unsigned kTablesSizesSum = kMainTableSize + kDistTableSize + kAlignTableSize + kLenTableSize;

class CBitDecoder
{
  UInt32 _value;
  unsigned _bitPos;
public:
  CInBuffer Stream;

  bool Create(UInt32 bufSize) { return Stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { Stream.SetStream(inStream);}

  void Init()
  {
    Stream.Init();
    _bitPos = 0;
    _value = 0;
  }
  
  bool ExtraBitsWereRead() const
  {
    return (Stream.NumExtraBytes > 4 || _bitPos < (Stream.NumExtraBytes << 3));
  }

  UInt64 GetProcessedSize() const { return Stream.GetProcessedSize() - (_bitPos >> 3); }

  void AlignToByte()
  {
    _bitPos &= ~(unsigned)7;
    _value = _value & ((1 << _bitPos) - 1);
  }
  
  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits)
  {
    if (_bitPos < numBits)
    {
      _bitPos += 8;
      _value = (_value << 8) | Stream.ReadByte();
      if (_bitPos < numBits)
      {
        _bitPos += 8;
        _value = (_value << 8) | Stream.ReadByte();
      }
    }
    return _value >> (_bitPos - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits()
  {
    return GetValue(kNumHuffmanBits) << (32 - kNumHuffmanBits);
  }

  
  Z7_FORCE_INLINE
  void MovePos(unsigned numBits)
  {
    _bitPos -= numBits;
    _value = _value & ((1 << _bitPos) - 1);
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    const UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }

  UInt32 ReadBits_upto8(unsigned numBits)
  {
    if (_bitPos < numBits)
    {
      _bitPos += 8;
      _value = (_value << 8) | Stream.ReadByte();
    }
    _bitPos -= numBits;
    const UInt32 res = _value >> _bitPos;
    _value = _value & ((1 << _bitPos) - 1);
    return res;
  }

  Byte ReadByteFromAligned()
  {
    if (_bitPos == 0)
      return Stream.ReadByte();
    const unsigned bitsPos = _bitPos - 8;
    const Byte b = (Byte)(_value >> bitsPos);
    _value = _value & ((1 << bitsPos) - 1);
    _bitPos = bitsPos;
    return b;
  }
};


struct CByteIn
{
  IByteIn IByteIn_obj;
  CBitDecoder BitDecoder;
};


struct CFilter: public NVm::CProgram
{
  CRecordVector<Byte> GlobalData;
  UInt32 BlockStart;
  UInt32 BlockSize;
  UInt32 ExecCount;
  
  CFilter(): BlockStart(0), BlockSize(0), ExecCount(0) {}
};

struct CTempFilter: public NVm::CProgramInitState
{
  UInt32 BlockStart;
  UInt32 BlockSize;
  bool NextWindow;
  
  UInt32 FilterIndex;

  CTempFilter()
  {
    // all filters must contain at least FixedGlobal block
    AllocateEmptyFixedGlobal();
  }
};


Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  // bool _errorMode;

  bool _lzMode;
  bool _unsupportedFilter;

  CByteIn m_InBitStream;
  Byte *_window;
  UInt32 _winPos;
  UInt32 _wrPtr;
  UInt64 _lzSize;
  UInt64 _unpackSize;
  UInt64 _writtenFileSize; // if it's > _unpackSize, then _unpackSize only written
  ISequentialOutStream *_outStream;

  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, 9> m_MainDecoder;
  UInt32 kDistStart[kDistTableSize];
  NHuffman::CDecoder256<kNumHuffmanBits, kDistTableSize, 7> m_DistDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kAlignTableSize, 6> m_AlignDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kLenTableSize, 7> m_LenDecoder;

  UInt32 _reps[kNumReps];
  UInt32 _lastLength;
  
  Byte m_LastLevels[kTablesSizesSum];

  Byte *_vmData;
  Byte *_vmCode;
  NVm::CVm _vm;
  CRecordVector<CFilter *> _filters;
  CRecordVector<CTempFilter *>  _tempFilters;
  unsigned _numEmptyTempFilters;
  UInt32 _lastFilter;

  UInt32 PrevAlignBits;
  UInt32 PrevAlignCount;

  bool TablesRead;
  bool TablesOK;
  bool PpmError;

  int PpmEscChar;
  CPpmd7 _ppmd;
  
  HRESULT WriteDataToStream(const Byte *data, UInt32 size);
  HRESULT WriteData(const Byte *data, UInt32 size);
  HRESULT WriteArea(UInt32 startPtr, UInt32 endPtr);
  void ExecuteFilter(unsigned tempFilterIndex, NVm::CBlockRef &outBlockRef);
  HRESULT WriteBuf();

  void InitFilters();
  bool AddVmCode(UInt32 firstByte, UInt32 codeSize);
  bool ReadVmCodeLZ();
  bool ReadVmCodePPM();
  
  UInt32 ReadBits(unsigned numBits);

  HRESULT InitPPM();
  // int DecodePpmSymbol();
  HRESULT DecodePPM(Int32 num, bool &keepDecompressing);

  HRESULT ReadTables(bool &keepDecompressing);
  HRESULT ReadEndOfBlock(bool &keepDecompressing);
  HRESULT DecodeLZ(bool &keepDecompressing);
  HRESULT CodeReal(ICompressProgressInfo *progress);
  
  bool InputEofError() const { return m_InBitStream.BitDecoder.ExtraBitsWereRead(); }
  bool InputEofError_Fast() const { return (m_InBitStream.BitDecoder.Stream.NumExtraBytes > 2); }

  void CopyBlock(UInt32 dist, UInt32 len)
  {
    _lzSize += len;
    UInt32 pos = (_winPos - dist - 1) & kWindowMask;
    Byte *window = _window;
    UInt32 winPos = _winPos;
    if (kWindowSize - winPos > len && kWindowSize - pos > len)
    {
      const Byte *src = window + pos;
      Byte *dest = window + winPos;
      _winPos += len;
      do
        *dest++ = *src++;
      while (--len != 0);
      return;
    }
    do
    {
      window[winPos] = window[pos];
      winPos = (winPos + 1) & kWindowMask;
      pos = (pos + 1) & kWindowMask;
    }
    while (--len != 0);
    _winPos = winPos;
  }
  
  void PutByte(Byte b)
  {
    const UInt32 wp = _winPos;
    _window[wp] = b;
    _winPos = (wp + 1) & kWindowMask;
    _lzSize++;
  }

public:
  CDecoder();
  ~CDecoder();
};

}}

#endif
