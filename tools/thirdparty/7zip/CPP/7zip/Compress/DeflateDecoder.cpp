// DeflateDecoder.cpp

#include "StdAfx.h"

#include "DeflateDecoder.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

CCoder::CCoder(bool deflate64Mode):
    _deflateNSIS(false),
    _deflate64Mode(deflate64Mode),
    _keepHistory(false),
    _needFinishInput(false),
    _needInitInStream(true),
    _outSizeDefined(false),
    _outStartPos(0)
    {}

UInt32 CCoder::ReadBits(unsigned numBits)
{
  return m_InBitStream.ReadBits(numBits);
}

Byte CCoder::ReadAlignedByte()
{
  return m_InBitStream.ReadAlignedByte();
}

bool CCoder::DecodeLevels(Byte *levels, unsigned numSymbols)
{
  unsigned i = 0;
  
  do
  {
    unsigned sym = m_LevelDecoder.Decode(&m_InBitStream);
    if (sym < kTableDirectLevels)
      levels[i++] = (Byte)sym;
    else
    {
      if (sym >= kLevelTableSize)
        return false;
      
      unsigned num;
      unsigned numBits;
      Byte symbol;
      
      if (sym == kTableLevelRepNumber)
      {
        if (i == 0)
          return false;
        numBits = 2;
        num = 0;
        symbol = levels[(size_t)i - 1];
      }
      else
      {
        sym -= kTableLevel0Number;
        sym <<= 2;
        numBits = 3 + (unsigned)sym;
        num = ((unsigned)sym << 1);
        symbol = 0;
      }
      
      num += i + 3 + ReadBits(numBits);
      if (num > numSymbols)
        return false;
      do
        levels[i++] = symbol;
      while (i < num);
    }
  }
  while (i < numSymbols);
  
  return true;
}

#define RIF(x) { if (!(x)) return false; }

bool CCoder::ReadTables(void)
{
  m_FinalBlock = (ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);
  if (m_InBitStream.ExtraBitsWereRead())
    return false;
  const UInt32 blockType = ReadBits(kBlockTypeFieldSize);
  if (blockType > NBlockType::kDynamicHuffman)
    return false;
  if (m_InBitStream.ExtraBitsWereRead())
    return false;

  if (blockType == NBlockType::kStored)
  {
    m_StoredMode = true;
    m_InBitStream.AlignToByte();
    m_StoredBlockSize = ReadAligned_UInt16(); // ReadBits(kStoredBlockLengthFieldSize)
    if (_deflateNSIS)
      return true;
    return (m_StoredBlockSize == (UInt16)~ReadAligned_UInt16());
  }

  m_StoredMode = false;

  CLevels levels;
  if (blockType == NBlockType::kFixedHuffman)
  {
    levels.SetFixedLevels();
    _numDistLevels = _deflate64Mode ? kDistTableSize64 : kDistTableSize32;
  }
  else
  {
    const unsigned numLitLenLevels = ReadBits(kNumLenCodesFieldSize) + kNumLitLenCodesMin;
    _numDistLevels = (unsigned)ReadBits(kNumDistCodesFieldSize) + kNumDistCodesMin;
    const unsigned numLevelCodes = ReadBits(kNumLevelCodesFieldSize) + kNumLevelCodesMin;

    if (!_deflate64Mode)
      if (_numDistLevels > kDistTableSize32)
        return false;
    
    const unsigned kLevelTableSize_aligned4 = kLevelTableSize + 1;
    Byte levelLevels[kLevelTableSize_aligned4];
    memset (levelLevels, 0, sizeof(levelLevels));
    unsigned i = 0;
    do
      levelLevels[kCodeLengthAlphabetOrder[i++]] = (Byte)ReadBits(kLevelFieldSize);
    while (i != numLevelCodes);
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    RIF(m_LevelDecoder.Build(levelLevels, false)) // full
    
    Byte tmpLevels[kFixedMainTableSize + kFixedDistTableSize];
    if (!DecodeLevels(tmpLevels, numLitLenLevels + _numDistLevels))
      return false;
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    levels.SubClear();
    memcpy(levels.litLenLevels, tmpLevels, numLitLenLevels);
    memcpy(levels.distLevels, tmpLevels + numLitLenLevels, _numDistLevels);
  }
  RIF(m_MainDecoder.Build(levels.litLenLevels))
  return m_DistDecoder.Build(levels.distLevels);
}


HRESULT CCoder::InitInStream(bool needInit)
{
  if (needInit)
  {
    // for HDD-Windows:
    // (1 << 15) - best for reading only prefetch
    // (1 << 22) - best for real reading / writing
    if (!m_InBitStream.Create(1 << 20))
      return E_OUTOFMEMORY;
    m_InBitStream.Init();
    _needInitInStream = false;
  }
  return S_OK;
}


HRESULT CCoder::CodeSpec(UInt32 curSize, bool finishInputStream, UInt32 inputProgressLimit)
{
  if (_remainLen == kLenIdFinished)
    return S_OK;
  
  if (_remainLen == kLenIdNeedInit)
  {
    if (!_keepHistory)
      if (!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64: kHistorySize32))
        return E_OUTOFMEMORY;
    RINOK(InitInStream(_needInitInStream))
    m_OutWindowStream.Init(_keepHistory);
  
    m_FinalBlock = false;
    _remainLen = 0;
    _needReadTable = true;
  }

  // _remainLen >= 0
  while (_remainLen && curSize)
  {
    _remainLen--;
    const Byte b = m_OutWindowStream.GetByte(_rep0);
    m_OutWindowStream.PutByte(b);
    curSize--;
  }

  UInt64 inputStart = 0;
  if (inputProgressLimit != 0)
    inputStart = m_InBitStream.GetProcessedSize();

  while (curSize || finishInputStream)
  {
    if (m_InBitStream.ExtraBitsWereRead())
      return S_FALSE;

    if (_needReadTable)
    {
      if (m_FinalBlock)
      {
        _remainLen = kLenIdFinished;
        break;
      }
 
      if (inputProgressLimit != 0)
        if (m_InBitStream.GetProcessedSize() - inputStart >= inputProgressLimit)
          return S_OK;
      
      if (!ReadTables())
        return S_FALSE;
      if (m_InBitStream.ExtraBitsWereRead())
        return S_FALSE;
      _needReadTable = false;
    }

    if (m_StoredMode)
    {
      if (finishInputStream && curSize == 0 && m_StoredBlockSize != 0)
        return S_FALSE;
      /* NSIS version contains some bits in bitl bits buffer.
         So we must read some first bytes via ReadAlignedByte */
      UInt32 num = m_StoredBlockSize;
      if (num > curSize)
          num = curSize;
      m_StoredBlockSize -= num;
      curSize -= num;
      for (; num && m_InBitStream.ThereAreDataInBitsBuffer(); num--)
        m_OutWindowStream.PutByte(ReadAlignedByte());
      if (num)
      {
#if 1
        // fast code
        do
        {
          size_t a;
          Byte *buf = m_OutWindowStream.GetOutBuffer(a);
          // a != 0
          if (a > num)
              a = num;
          // a != 0
          a = m_InBitStream.ReadDirectBytesPart(buf, a);
          if (a == 0)
            return S_FALSE;
          m_OutWindowStream.SkipWrittenBytes(a);
          num -= (UInt32)a;
        }
        while (num);
#else
        // slow code:
        do
          m_OutWindowStream.PutByte(m_InBitStream.ReadDirectByte());
        while (--num);
#endif
      }
      _needReadTable = (m_StoredBlockSize == 0);
      continue;
    }
    
    while (curSize)
    {
      if (m_InBitStream.ExtraBitsWereRead_Fast())
        return S_FALSE;
      unsigned sym;
#if 0
      sym = m_MainDecoder.Decode(&m_InBitStream);
#else
      Z7_HUFF_DECODE_CHECK(sym, &m_MainDecoder, kNumHuffmanBits, kNumTableBits_Main, &m_InBitStream, { return S_FALSE; })
#endif

      if (sym < 0x100)
      {
        m_OutWindowStream.PutByte((Byte)sym);
        curSize--;
        continue;
      }
      if (sym == kSymbolEndOfBlock)
      {
        _needReadTable = true;
        break;
      }
#if 0
      if (sym >= kMainTableSize)
        return S_FALSE;
#endif
      {
        sym -= kSymbolMatch;
        UInt32 len;
        {
          unsigned numBits;
          if (_deflate64Mode)
          {
            len = kLenStart64[sym];
            numBits = kLenDirectBits64[sym];
          }
          else
          {
            len = kLenStart32[sym];
            numBits = kLenDirectBits32[sym];
          }
          len += kMatchMinLen + m_InBitStream.ReadBits(numBits);
        }
       
#if 0
        sym = m_DistDecoder.Decode(&m_InBitStream);
        if (sym >= _numDistLevels)
          return S_FALSE;
#else
        Z7_HUFF_DECODE_CHECK(sym, &m_DistDecoder, kNumHuffmanBits, kNumTableBits_Dist, &m_InBitStream, { return S_FALSE; })
#endif

#if 1
        sym = kDistStart[sym] + m_InBitStream.ReadBits(kDistDirectBits[sym]);
#else
        if (sym >= 4)
        {
          // sym &= 31;
          const unsigned numDirectBits = (sym - 2) >> 1;
          sym = (2u | (sym & 1)) << numDirectBits;
          sym += m_InBitStream.ReadBits(numDirectBits);
        }
#endif
        UInt32 locLen = len;
        if (locLen > curSize)
          locLen = (UInt32)curSize;
        if (!m_OutWindowStream.CopyBlock(sym, locLen))
          return S_FALSE;
        curSize -= locLen;
        len -= locLen;
        if (len != 0)
        {
          _remainLen = (Int32)len;
          _rep0 = sym;
          break;
        }
      }
    }
    
    if (finishInputStream && curSize == 0)
    {
      if (m_MainDecoder.Decode(&m_InBitStream) != kSymbolEndOfBlock)
        return S_FALSE;
      _needReadTable = true;
    }
  }

  if (m_InBitStream.ExtraBitsWereRead())
    return S_FALSE;

  return S_OK;
}


#ifdef Z7_NO_EXCEPTIONS

#define DEFLATE_TRY_BEGIN
#define DEFLATE_TRY_END(res)

#else

#define DEFLATE_TRY_BEGIN try {
#define DEFLATE_TRY_END(res) } \
  catch(const CSystemException &e) { res = e.ErrorCode; } \
  catch(...) { res = S_FALSE; }

  // catch(const CInBufferException &e)  { res = e.ErrorCode; }
  // catch(const CLzOutWindowException &e)  { res = e.ErrorCode; }

#endif


HRESULT CCoder::CodeReal(ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  HRESULT res;
  
  DEFLATE_TRY_BEGIN
  
  m_OutWindowStream.SetStream(outStream);
  CCoderReleaser flusher(this);

  const UInt64 inStart = _needInitInStream ? 0 : m_InBitStream.GetProcessedSize();

  for (;;)
  {
    const UInt32 kInputProgressLimit = 1 << 21;
    UInt32 curSize = 1 << 20;
    bool finishInputStream = false;
    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - GetOutProcessedCur();
      if (curSize >= rem)
      {
        curSize = (UInt32)rem;
        if (_needFinishInput)
          finishInputStream = true;
        else if (curSize == 0)
          break;
      }
    }
    
    RINOK(CodeSpec(curSize, finishInputStream, progress ? kInputProgressLimit : 0))
    
    if (_remainLen == kLenIdFinished)
      break;

    if (progress)
    {
      const UInt64 inSize = m_InBitStream.GetProcessedSize() - inStart;
      const UInt64 nowPos64 = GetOutProcessedCur();
      RINOK(progress->SetRatioInfo(&inSize, &nowPos64))
    }
  }
  
  flusher.NeedFlush = false;
  res = Flush();
  if (res == S_OK && _remainLen != kLenIdNeedInit && InputEofError())
    return S_FALSE;
  
  DEFLATE_TRY_END(res)
  
  return res;
}


Z7_COM7F_IMF(CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  SetInStream(inStream);
  SetOutStreamSize(outSize);
  const HRESULT res = CodeReal(outStream, progress);
  ReleaseInStream();
  /*
  if (res == S_OK)
    if (_needFinishInput && inSize && *inSize != m_InBitStream.GetProcessedSize())
      res = S_FALSE;
  */
  return res;
}


Z7_COM7F_IMF(CCoder::SetFinishMode(UInt32 finishMode))
{
  Set_NeedFinishInput(finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = m_InBitStream.GetStreamSize();
  return S_OK;
}


Z7_COM7F_IMF(CCoder::ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
{
  AlignToByte();
  UInt32 i = 0;
  {
    for (i = 0; i < size; i++)
    {
      if (!m_InBitStream.ReadAlignedByte_FromBuf(((Byte *)data)[i]))
        break;
    }
  }
  if (processedSize)
    *processedSize = i;
  return S_OK;
}


Z7_COM7F_IMF(CCoder::SetInStream(ISequentialInStream *inStream))
{
  m_InStreamRef = inStream;
  m_InBitStream.SetStream(inStream);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::ReleaseInStream())
{
  m_InStreamRef.Release();
  m_InBitStream.ClearStreamPtr();
  return S_OK;
}


void CCoder::SetOutStreamSizeResume(const UInt64 *outSize)
{
  _outSizeDefined = (outSize != NULL);
  _outSize = 0;
  if (_outSizeDefined)
    _outSize = *outSize;
  m_OutWindowStream.Init(_keepHistory);
  _outStartPos = m_OutWindowStream.GetProcessedSize();
  _remainLen = kLenIdNeedInit;
}


Z7_COM7F_IMF(CCoder::SetOutStreamSize(const UInt64 *outSize))
{
  /*
    18.06:
    We want to support GetInputProcessedSize() before CCoder::Read()
    So we call m_InBitStream.Init() even before buffer allocations
    m_InBitStream.Init() just sets variables to default values
    But later we will call m_InBitStream.Init() again with real buffer pointers
  */
  m_InBitStream.Init();
  _needInitInStream = true;
  SetOutStreamSizeResume(outSize);
  return S_OK;
}


#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CCoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  const UInt64 outPos = GetOutProcessedCur();

  bool finishInputStream = false;
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - outPos;
    if (size >= rem)
    {
      size = (UInt32)rem;
      if (_needFinishInput)
        finishInputStream = true;
    }
  }
  if (!finishInputStream && size == 0)
    return S_OK;

  HRESULT res;
  DEFLATE_TRY_BEGIN
  m_OutWindowStream.SetMemStream((Byte *)data);
  res = CodeSpec(size, finishInputStream);
  DEFLATE_TRY_END(res)
  {
    const HRESULT res2 = Flush();
    if (res2 != S_OK)
      res = res2;
  }
  if (processedSize)
    *processedSize = (UInt32)(GetOutProcessedCur() - outPos);
  m_OutWindowStream.SetMemStream(NULL);
  return res;
}

#endif


HRESULT CCoder::CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetOutStreamSizeResume(outSize);
  return CodeReal(outStream, progress);
}

}}}
