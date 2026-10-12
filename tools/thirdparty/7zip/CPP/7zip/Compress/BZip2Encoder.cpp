// BZip2Encoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"
#include "../../../C/BwtSort.h"
#include "../../../C/HuffEnc.h"

#include "BZip2Encoder.h"

namespace NCompress {
namespace NBZip2 {

#define HUFFMAN_LEN 16
#if HUFFMAN_LEN > Z7_HUFFMAN_LEN_MAX
  #error Stop_Compiling_Bad_HUFFMAN_LEN_BZip2Encoder
#endif
  
static const size_t kBufferSize = 1 << 17;
static const unsigned kNumHuffPasses = 4;


bool CThreadInfo::Alloc()
{
  if (!m_BlockSorterIndex)
  {
    m_BlockSorterIndex = (UInt32 *)::BigAlloc(BLOCK_SORT_BUF_SIZE(kBlockSizeMax) * sizeof(UInt32));
    if (!m_BlockSorterIndex)
      return false;
  }

  if (!m_Block_Base)
  {
    const unsigned kPadSize = 1 << 7; // we need at least 1 byte backward padding, becuase we use (m_Block - 1) pointer;
    m_Block_Base = (Byte *)::MidAlloc(kBlockSizeMax * 5
        + kBlockSizeMax / 10 + (20 << 10)
        + kPadSize);
    if (!m_Block_Base)
      return false;
    m_Block = m_Block_Base + kPadSize;
    m_MtfArray = m_Block + kBlockSizeMax;
    m_TempArray = m_MtfArray + kBlockSizeMax * 2 + 2;
  }
  return true;
}

void CThreadInfo::Free()
{
  ::BigFree(m_BlockSorterIndex);
  m_BlockSorterIndex = NULL;
  ::MidFree(m_Block_Base);
  m_Block_Base = NULL;
}

#ifndef Z7_ST

static THREAD_FUNC_DECL MFThread(void *threadCoderInfo)
{
  return ((CThreadInfo *)threadCoderInfo)->ThreadFunc();
}

HRESULT CThreadInfo::Create()
{
  WRes             wres = StreamWasFinishedEvent.Create();
  if (wres == 0) { wres = WaitingWasStartedEvent.Create();
  if (wres == 0) { wres = CanWriteEvent.Create();
  if (wres == 0)
  {
    wres =
#ifdef _WIN32
      Encoder->_props.NumThreadGroups > 1 ?
        Thread.Create_With_Group(MFThread, this, ThreadNextGroup_GetNext(&Encoder->ThreadNextGroup), 0) : // affinity
#endif
      Encoder->_props.Affinity != 0 ?
        Thread.Create_With_Affinity(MFThread, this, (CAffinityMask)Encoder->_props.Affinity) :
        Thread.Create(MFThread, this);
  }}}
  return HRESULT_FROM_WIN32(wres);
}

void CThreadInfo::FinishStream(bool needLeave)
{
  Encoder->StreamWasFinished = true;
  StreamWasFinishedEvent.Set();
  if (needLeave)
    Encoder->CS.Leave();
  Encoder->CanStartWaitingEvent.Lock();
  WaitingWasStartedEvent.Set();
}

THREAD_FUNC_RET_TYPE CThreadInfo::ThreadFunc()
{
  for (;;)
  {
    Encoder->CanProcessEvent.Lock();
    Encoder->CS.Enter();
    if (Encoder->CloseThreads)
    {
      Encoder->CS.Leave();
      return 0;
    }
    if (Encoder->StreamWasFinished)
    {
      FinishStream(true);
      continue;
    }
    HRESULT res = S_OK;
    bool needLeave = true;
    try
    {
      const UInt32 blockSize = Encoder->ReadRleBlock(m_Block);
      m_UnpackSize = Encoder->m_InStream.GetProcessedSize();
      m_BlockIndex = Encoder->NextBlockIndex;
      if (++Encoder->NextBlockIndex == Encoder->NumThreads)
        Encoder->NextBlockIndex = 0;
      if (blockSize == 0)
      {
        FinishStream(true);
        continue;
      }
      Encoder->CS.Leave();
      needLeave = false;
      res = EncodeBlock3(blockSize);
    }
    catch(const CInBufferException &e)  { res = e.ErrorCode; }
    catch(const COutBufferException &e) { res = e.ErrorCode; }
    catch(...) { res = E_FAIL; }
    if (res != S_OK)
    {
      Encoder->Result = res;
      FinishStream(needLeave);
      continue;
    }
  }
}

#endif

void CEncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level > 9) level = 9;
  
  if (NumPasses == (UInt32)(Int32)-1)
    NumPasses = (level >= 9 ? 7 : (level >= 7 ? 2 : 1));
  if (NumPasses < 1) NumPasses = 1;
  if (NumPasses > kNumPassesMax) NumPasses = kNumPassesMax;
  
  if (BlockSizeMult == (UInt32)(Int32)-1)
    BlockSizeMult = (level >= 5 ? 9 : (level >= 1 ? (unsigned)level * 2 - 1: 1));
  if (BlockSizeMult < kBlockSizeMultMin) BlockSizeMult = kBlockSizeMultMin;
  if (BlockSizeMult > kBlockSizeMultMax) BlockSizeMult = kBlockSizeMultMax;
}

CEncoder::CEncoder()
{
  _props.Normalize(-1);

  #ifndef Z7_ST
  ThreadsInfo = NULL;
  m_NumThreadsPrev = 0;
  NumThreads = 1;
  #endif
}

#ifndef Z7_ST
CEncoder::~CEncoder()
{
  Free();
}

HRESULT CEncoder::Create()
{
  {
    WRes             wres = CanProcessEvent.CreateIfNotCreated_Reset();
    if (wres == 0) { wres = CanStartWaitingEvent.CreateIfNotCreated_Reset(); }
    if (wres != 0)
      return HRESULT_FROM_WIN32(wres);
  }
  
  if (ThreadsInfo && m_NumThreadsPrev == NumThreads)
    return S_OK;
  try
  {
    Free();
    MtMode = (NumThreads > 1);
    m_NumThreadsPrev = NumThreads;
    ThreadsInfo = new CThreadInfo[NumThreads];
    if (!ThreadsInfo)
      return E_OUTOFMEMORY;
  }
  catch(...) { return E_OUTOFMEMORY; }
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CThreadInfo &ti = ThreadsInfo[t];
    ti.Encoder = this;
    if (MtMode)
    {
      HRESULT res = ti.Create();
      if (res != S_OK)
      {
        NumThreads = t;
        Free();
        return res;
      }
    }
  }
  return S_OK;
}

void CEncoder::Free()
{
  if (!ThreadsInfo)
    return;
  CloseThreads = true;
  CanProcessEvent.Set();
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CThreadInfo &ti = ThreadsInfo[t];
    if (MtMode)
      ti.Thread.Wait_Close();
    ti.Free();
  }
  delete []ThreadsInfo;
  ThreadsInfo = NULL;
}
#endif

struct CRleEncoder
{
  const Byte *_src;
  const Byte *_srcLim;
  Byte *_dest;
  const Byte *_destLim;
  Byte _prevByte;
  unsigned _numReps;

  void Encode();
};

Z7_NO_INLINE
void CRleEncoder::Encode()
{
  const Byte *src = _src;
  const Byte * const srcLim = _srcLim;
  Byte *dest = _dest;
  const Byte * const destLim = _destLim;
  Byte prev = _prevByte;
  unsigned numReps = _numReps;
  // (dest < destLim)
  // src = srcLim; // for debug
  while (dest < destLim)
  {
    if (src == srcLim)
      break;
    const Byte b = *src++;
    if (b != prev)
    {
      if (numReps >= kRleModeRepSize)
        *dest++ = (Byte)(numReps - kRleModeRepSize);
      *dest++ = b;
      numReps = 1;
      prev = b;
      /*
      { // speed optimization code:
        if (dest >= destLim || src == srcLim)
          break;
        const Byte b2 = *src++;
        *dest++ = b2;
        numReps += (prev == b2);
        prev = b2;
      }
      */
      continue;
    }
    numReps++;
    if (numReps <= kRleModeRepSize)
      *dest++ = b;
    else if (numReps == kRleModeRepSize + 255)
    {
      *dest++ = (Byte)(numReps - kRleModeRepSize);
      numReps = 0;
    }
  }
  _src = src;
  _dest = dest;
  _prevByte = prev;
  _numReps = numReps;
  // (dest <= destLim + 1)
}


// out: return value is blockSize: size of data filled in buffer[]:
// (returned_blockSize <= _props.BlockSizeMult * kBlockSizeStep)
UInt32 CEncoder::ReadRleBlock(Byte *buffer)
{
  CRleEncoder rle;
  UInt32 i = 0;
  if (m_InStream.ReadByte(rle._prevByte))
  {
    NumBlocks++;
    const UInt32 blockSize = _props.BlockSizeMult * kBlockSizeStep - 1; // -1 for RLE
    rle._destLim = buffer + blockSize;
    rle._numReps = 1;
    buffer[i++] = rle._prevByte;
    while (i < blockSize)
    {
      rle._dest = buffer + i;
      size_t rem;
      const Byte * const ptr = m_InStream.Lookahead(rem);
      if (rem == 0)
        break;
      rle._src = ptr;
      rle._srcLim = ptr + rem;
      rle.Encode();
      m_InStream.Skip((size_t)(rle._src - ptr));
      i = (UInt32)(size_t)(rle._dest - buffer);
      // (i <= blockSize + 1)
    }
    const int n = (int)rle._numReps - (int)kRleModeRepSize;
    if (n >= 0)
      buffer[i++] = (Byte)n;
  }
  return i;
}



Z7_NO_INLINE
void CThreadInfo::WriteBits2(UInt32 value, unsigned numBits)
  { m_OutStreamCurrent.WriteBits(value, numBits); }
/*
Z7_NO_INLINE
void CThreadInfo::WriteByte2(unsigned b)
  { m_OutStreamCurrent.WriteByte(b); }
*/
// void CEncoder::WriteBits(UInt32 value, unsigned numBits) { m_OutStream.WriteBits(value, numBits); }
Z7_NO_INLINE
void CEncoder::WriteByte(Byte b) { m_OutStream.WriteByte(b); }


#define WRITE_BITS_UPDATE(value, numBits) \
{ \
  numBits -= _bitPos; \
  const UInt32 hi = value >> numBits; \
  *_buf++ = (Byte)(_curByte | hi); \
  value -= hi << numBits; \
  _bitPos = 8; \
  _curByte = 0; \
}

#if HUFFMAN_LEN > 16

#define WRITE_BITS_HUFF(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  while (numBits >= _bitPos) { \
    WRITE_BITS_UPDATE(value, numBits) \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#else // HUFFMAN_LEN <= 16

// numBits2 <= 16 is supported
#define WRITE_BITS_HUFF(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  if (numBits >= _bitPos) \
  { \
    WRITE_BITS_UPDATE(value, numBits) \
    if (numBits >= _bitPos) \
    { \
      numBits -= _bitPos; \
      const UInt32 hi = value >> numBits; \
      *_buf++ = (Byte)hi; \
      value -= hi << numBits; \
    } \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#endif

#define WRITE_BITS_8(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  if (numBits >= _bitPos) \
  { \
    WRITE_BITS_UPDATE(value, numBits) \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#define WRITE_BIT_PRE \
  { _bitPos--; }

#define WRITE_BIT_POST \
{ \
  if (_bitPos == 0) \
  { \
    *_buf++ = (Byte)_curByte; \
    _curByte = 0; \
    _bitPos = 8; \
  } \
}

#define WRITE_BIT_0 \
{ \
  WRITE_BIT_PRE \
  WRITE_BIT_POST \
}

#define WRITE_BIT_1 \
{ \
  WRITE_BIT_PRE \
  _curByte |= 1u << _bitPos; \
  WRITE_BIT_POST \
}


// blockSize > 0
void CThreadInfo::EncodeBlock(const Byte *block, UInt32 blockSize)
{
  // WriteBit2(0); // Randomised = false
  {
    const UInt32 origPtr = BlockSort(m_BlockSorterIndex, block, blockSize);
    // if (m_BlockSorterIndex[origPtr] != 0) throw 1;
    m_BlockSorterIndex[origPtr] = blockSize;
    WriteBits2(origPtr, kNumOrigBits + 1); // + 1 for additional high bit flag (Randomised = false)
  }
  Byte mtfBuf[256];
  // memset(mtfBuf, 0, sizeof(mtfBuf)); // to disable MSVC warning
  unsigned numInUse;
  {
    Byte inUse[256];
    Byte inUse16[16];
    unsigned i;
    for (i = 0; i < 256; i++)
      inUse[i] = 0;
    for (i = 0; i < 16; i++)
      inUse16[i] = 0;
    {
      const Byte *       cur = block;
      block = block + (size_t)blockSize - 1;
      if (cur != block)
      {
        do
        {
          const unsigned b0 = cur[0];
          const unsigned b1 = cur[1];
          cur += 2;
          inUse[b0] = 1;
          inUse[b1] = 1;
        }
        while (cur < block);
      }
      if (cur == block)
        inUse[cur[0]] = 1;
      block -= blockSize; // block pointer is (original_block - 1)
    }
    numInUse = 0;
    for (i = 0; i < 256; i++)
      if (inUse[i])
      {
        inUse16[i >> 4] = 1;
        mtfBuf[numInUse++] = (Byte)i;
      }
    for (i = 0; i < 16; i++)
      WriteBit2(inUse16[i]);
    for (i = 0; i < 256; i++)
      if (inUse16[i >> 4])
        WriteBit2(inUse[i]);
  }
  const unsigned alphaSize = numInUse + 2;

  UInt32 symbolCounts[kMaxAlphaSize];
  {
    for (unsigned i = 0; i < kMaxAlphaSize; i++)
      symbolCounts[i] = 0;
    symbolCounts[(size_t)alphaSize - 1] = 1;
  }

  Byte *mtfs = m_MtfArray;
  {
    const UInt32 *bsIndex = m_BlockSorterIndex;
    const UInt32 *bsIndex_rle = bsIndex;
    const UInt32 * const bsIndex_end = bsIndex + blockSize;
    // block--; // backward fix
    // block pointer is (original_block - 1)
    do
    {
      const Byte v = block[*bsIndex++];
      Byte a = mtfBuf[0];
      if (v != a)
      {
        mtfBuf[0] = v;
        {
          UInt32 rleSize = (UInt32)(size_t)(bsIndex - bsIndex_rle) - 1;
          bsIndex_rle = bsIndex;
          while (rleSize)
          {
            const unsigned sym = (unsigned)(--rleSize & 1);
            *mtfs++ = (Byte)sym;
            symbolCounts[sym]++;
            rleSize >>= 1;
          }
        }
        unsigned pos1 = 2; // = real_pos + 1
        Byte b;
               b = mtfBuf[1];  mtfBuf[1] = a;  if (v != b)
             { a = mtfBuf[2];  mtfBuf[2] = b;  if (v == a) pos1 = 3;
        else { b = mtfBuf[3];  mtfBuf[3] = a;  if (v == b) pos1 = 4;
        else
        {
          Byte *m = mtfBuf + 7;
          for (;;)
          {
            a = m[-3];  m[-3] = b;           if (v == a) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 2)); break; }
            b = m[-2];  m[-2] = a;           if (v == b) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 1)); break; }
            a = m[-1];  m[-1] = b;           if (v == a) { pos1 = (unsigned)(size_t)(m - (mtfBuf    )); break; }
            b = m[ 0];  m[ 0] = a;  m += 4;  if (v == b) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 3)); break; }
          }
        }}}
        symbolCounts[pos1]++;
        if (pos1 >= 0xff)
        {
          *mtfs++ = 0xff;
          // pos1 -= 0xff;
          pos1++; // we need only low byte
        }
        *mtfs++ = (Byte)pos1;
      }
    }
    while (bsIndex < bsIndex_end);

    UInt32 rleSize = (UInt32)(size_t)(bsIndex - bsIndex_rle);
    while (rleSize)
    {
      const unsigned sym = (unsigned)(--rleSize & 1);
      *mtfs++ = (Byte)sym;
      symbolCounts[sym]++;
      rleSize >>= 1;
    }
    
    unsigned d = alphaSize - 1;
    if (alphaSize >= 256)
    {
      *mtfs++ = 0xff;
      d = alphaSize; // (-256)
    }
    *mtfs++ = (Byte)d;
  }

  const Byte * const mtf_lim = mtfs;

  UInt32 numSymbols = 0;
  {
    for (unsigned i = 0; i < kMaxAlphaSize; i++)
      numSymbols += symbolCounts[i];
  }

  unsigned bestNumTables = kNumTablesMin;
  UInt32 bestPrice = 0xFFFFFFFF;
  const UInt32 startPos = m_OutStreamCurrent.GetPos();
  const unsigned startCurByte = m_OutStreamCurrent.GetCurByte();
  for (unsigned nt = kNumTablesMin; nt <= kNumTablesMax + 1; nt++)
  {
    unsigned numTables;

    if (m_OptimizeNumTables)
    {
      m_OutStreamCurrent.SetPos(startPos);
      m_OutStreamCurrent.SetCurState(startPos & 7, startCurByte);
      numTables = (nt <= kNumTablesMax ? nt : bestNumTables);
    }
    else
    {
           if (numSymbols <  200) numTables = 2;
      else if (numSymbols <  600) numTables = 3;
      else if (numSymbols < 1200) numTables = 4;
      else if (numSymbols < 2400) numTables = 5;
      else                        numTables = 6;
    }

    WriteBits2(numTables, kNumTablesBits);
    const unsigned numSelectors = (numSymbols + kGroupSize - 1) / kGroupSize;
    WriteBits2((UInt32)numSelectors, kNumSelectorsBits);
    
    {
      UInt32 remFreq = numSymbols;
      unsigned gs = 0;
      unsigned t = numTables;
      do
      {
        UInt32 tFreq = remFreq / t;
        unsigned ge = gs;
        UInt32 aFreq = 0;
        while (aFreq < tFreq) //  && ge < alphaSize)
          aFreq += symbolCounts[ge++];
        
        if (ge > gs + 1 && t != numTables && t != 1 && (((numTables - t) & 1) == 1))
          aFreq -= symbolCounts[--ge];
        
        Byte *lens = Lens[(size_t)t - 1];
        unsigned i = 0;
        do
          lens[i] = (Byte)((i >= gs && i < ge) ? 0 : 1);
        while (++i < alphaSize);
        gs = ge;
        remFreq -= aFreq;
      }
      while (--t != 0);
    }
    
    
    for (unsigned pass = 0; pass < kNumHuffPasses; pass++)
    {
      memset(Freqs, 0, sizeof(Freqs[0]) * numTables);
      // memset(Freqs, 0, sizeof(Freqs));
      {
        mtfs = m_MtfArray;
        UInt32 g = 0;
        do
        {
          unsigned symbols[kGroupSize];
          unsigned i = 0;
          do
          {
            UInt32 symbol = *mtfs++;
            if (symbol >= 0xFF)
              symbol += *mtfs++;
            symbols[i] = symbol;
          }
          while (++i < kGroupSize && mtfs < mtf_lim);
          
          UInt32 bestPrice2 = 0xFFFFFFFF;
          unsigned t = 0;
          do
          {
            const Byte *lens = Lens[t];
            UInt32 price = 0;
            unsigned j = 0;
            do
              price += lens[symbols[j]];
            while (++j < i);
            if (price < bestPrice2)
            {
              m_Selectors[g] = (Byte)t;
              bestPrice2 = price;
            }
          }
          while (++t < numTables);
          UInt32 *freqs = Freqs[m_Selectors[g++]];
          unsigned j = 0;
          do
            freqs[symbols[j]]++;
          while (++j < i);
        }
        while (mtfs < mtf_lim);
      }
      
      unsigned t = 0;
      do
      {
        UInt32 *freqs = Freqs[t];
        unsigned i = 0;
        do
          if (freqs[i] == 0)
            freqs[i] = 1;
        while (++i < alphaSize);
        Huffman_Generate(freqs, Codes[t], Lens[t], kMaxAlphaSize, HUFFMAN_LEN);
      }
      while (++t < numTables);
    }
    
    unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
    unsigned _curByte; // low (_bitPos) bits are zeros
                       // high (8 - _bitPos) bits are filled
    Byte *_buf;
    {
      Byte mtfSel[kNumTablesMax];
      {
        unsigned t = 0;
        do
          mtfSel[t] = (Byte)t;
        while (++t < numTables);
      }

      _bitPos = m_OutStreamCurrent._bitPos;
      _curByte = m_OutStreamCurrent._curByte;
      _buf = m_OutStreamCurrent._buf;
      // stream.Init_from_Global(m_OutStreamCurrent);
      
      const Byte *selectors = m_Selectors;
      const Byte * const selectors_lim = selectors + numSelectors;
      Byte prev = 0; // mtfSel[0];
      do
      {
        const Byte sel = *selectors++;
        if (prev != sel)
        {
          Byte *mtfSel_cur = &mtfSel[1];
          for (;;)
          {
            WRITE_BIT_1
            const Byte next = *mtfSel_cur;
            *mtfSel_cur++ = prev;
            prev = next;
            if (next == sel)
              break;
          }
          // mtfSel[0] = sel;
        }
        WRITE_BIT_0
      }
      while (selectors != selectors_lim);
    }
    {
      unsigned t = 0;
      do
      {
        const Byte *lens = Lens[t];
        unsigned len = lens[0];
        WRITE_BITS_8(len, kNumLevelsBits)
        unsigned i = 0;
        do
        {
          const unsigned level = lens[i];
          while (len != level)
          {
            WRITE_BIT_1
            if (len < level)
            {
              len++;
              WRITE_BIT_0
            }
            else
            {
              len--;
              WRITE_BIT_1
            }
          }
          WRITE_BIT_0
        }
        while (++i < alphaSize);
      }
      while (++t < numTables);
    }
    {
      UInt32 groupSize = 1;
      const Byte *selectors = m_Selectors;
      const Byte *lens = NULL;
      const UInt32 *codes = NULL;
      mtfs = m_MtfArray;
      do
      {
        unsigned symbol = *mtfs++;
        if (symbol >= 0xFF)
          symbol += *mtfs++;
        if (--groupSize == 0)
        {
          groupSize = kGroupSize;
          const unsigned t = *selectors++;
          lens = Lens[t];
          codes = Codes[t];
        }
        WRITE_BITS_HUFF(codes[symbol], lens[symbol])
      }
      while (mtfs < mtf_lim);
    }
    // Restore_from_Local:
    m_OutStreamCurrent._bitPos = _bitPos;
    m_OutStreamCurrent._curByte = _curByte;
    m_OutStreamCurrent._buf = _buf;

    if (!m_OptimizeNumTables)
      break;
    const UInt32 price = m_OutStreamCurrent.GetPos() - startPos;
    if (price <= bestPrice)
    {
      if (nt == kNumTablesMax)
        break;
      bestPrice = price;
      bestNumTables = nt;
    }
  }
}


// blockSize > 0
UInt32 CThreadInfo::EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize)
{
  WriteByte2(kBlockSig0);
  WriteByte2(kBlockSig1);
  WriteByte2(kBlockSig2);
  WriteByte2(kBlockSig3);
  WriteByte2(kBlockSig4);
  WriteByte2(kBlockSig5);

  CBZip2Crc crc;
  const Byte * const lim = block + blockSize;
  unsigned b = *block++;
  crc.UpdateByte(b);
  for (;;)
  {
    const unsigned prev = b;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  if (b) do crc.UpdateByte(prev); while (--b);
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);
  }
  const UInt32 crcRes = crc.GetDigest();
  for (int i = 24; i >= 0; i -= 8)
    WriteByte2((Byte)(crcRes >> i));
  EncodeBlock(lim - blockSize, blockSize);
  return crcRes;
}


void CThreadInfo::EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses)
{
  const UInt32 numCrcs = m_NumCrcs;

  const UInt32 startBytePos = m_OutStreamCurrent.GetBytePos();
  const UInt32 startPos = m_OutStreamCurrent.GetPos();
  const unsigned startCurByte = m_OutStreamCurrent.GetCurByte();
  unsigned endCurByte = 0;
  UInt32 endPos = 0; // 0 means no no additional passes
  if (numPasses > 1 && blockSize >= (1 << 10))
  {
    UInt32 bs0 = blockSize / 2;
    for (; bs0 < blockSize &&
           (block[        bs0    ] ==
            block[(size_t)bs0 - 1] ||
            block[(size_t)bs0 - 1] ==
            block[(size_t)bs0 - 2]);
      bs0++)
    {}
    
    if (bs0 < blockSize)
    {
      EncodeBlock2(block, bs0, numPasses - 1);
      EncodeBlock2(block + bs0, blockSize - bs0, numPasses - 1);
      endPos = m_OutStreamCurrent.GetPos();
      endCurByte = m_OutStreamCurrent.GetCurByte();
      // we prepare next byte as identical byte to starting byte for main encoding attempt:
      if (endPos & 7)
        WriteBits2(0, 8 - (endPos & 7));
      m_OutStreamCurrent.SetCurState((startPos & 7), startCurByte);
    }
  }

  const UInt32 startBytePos2 = m_OutStreamCurrent.GetBytePos();
  const UInt32 startPos2 = m_OutStreamCurrent.GetPos();
  const UInt32 crcVal = EncodeBlockWithHeaders(block, blockSize);

  if (endPos)
  {
    const UInt32 size2 = m_OutStreamCurrent.GetPos() - startPos2;
    if (size2 >= endPos - startPos)
    {
      m_OutStreamCurrent.SetPos(endPos);
      m_OutStreamCurrent.SetCurState((endPos & 7), endCurByte);
      return;
    }
    const UInt32 numBytes = m_OutStreamCurrent.GetBytePos() - startBytePos2;
    Byte * const buffer = m_OutStreamCurrent.GetStream();
    memmove(buffer + startBytePos, buffer + startBytePos2, numBytes);
    m_OutStreamCurrent.SetPos(startPos + size2);
    // we don't call m_OutStreamCurrent.SetCurState() here because
    // m_OutStreamCurrent._curByte is correct already
  }
  m_CRCs[numCrcs] = crcVal;
  m_NumCrcs = numCrcs + 1;
}


HRESULT CThreadInfo::EncodeBlock3(UInt32 blockSize)
{
  CMsbfEncoderTemp &outStreamTemp = m_OutStreamCurrent;
  outStreamTemp.SetStream(m_TempArray);
  outStreamTemp.Init();
  m_NumCrcs = 0;

  EncodeBlock2(m_Block, blockSize, Encoder->_props.NumPasses);

#ifndef Z7_ST
  if (Encoder->MtMode)
    Encoder->ThreadsInfo[m_BlockIndex].CanWriteEvent.Lock();
#endif

  for (UInt32 i = 0; i < m_NumCrcs; i++)
    Encoder->CombinedCrc.Update(m_CRCs[i]);
  Encoder->WriteBytes(m_TempArray, outStreamTemp.GetPos(), outStreamTemp.GetNonFlushedByteBits());
  HRESULT res = S_OK;

#ifndef Z7_ST
  if (Encoder->MtMode)
  {
    UInt32 blockIndex = m_BlockIndex + 1;
    if (blockIndex == Encoder->NumThreads)
      blockIndex = 0;
    if (Encoder->Progress)
    {
      const UInt64 packSize = Encoder->m_OutStream.GetProcessedSize();
      res = Encoder->Progress->SetRatioInfo(&m_UnpackSize, &packSize);
    }
    Encoder->ThreadsInfo[blockIndex].CanWriteEvent.Set();
  }
#endif
  return res;
}

void CEncoder::WriteBytes(const Byte *data, UInt32 sizeInBits, unsigned lastByteBits)
{
  m_OutStream.WriteBytes(data, sizeInBits >> 3);
  sizeInBits &= 7;
  if (sizeInBits)
    m_OutStream.WriteBits(lastByteBits, sizeInBits);
}


HRESULT CEncoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  NumBlocks = 0;
#ifndef Z7_ST
  Progress = progress;
  ThreadNextGroup_Init(&ThreadNextGroup, _props.NumThreadGroups, 0); // startGroup
  RINOK(Create())
  for (UInt32 t = 0; t < NumThreads; t++)
#endif
  {
    #ifndef Z7_ST
    CThreadInfo &ti = ThreadsInfo[t];
    if (MtMode)
    {
      WRes             wres = ti.StreamWasFinishedEvent.Reset();
      if (wres == 0) { wres = ti.WaitingWasStartedEvent.Reset();
      if (wres == 0) { wres = ti.CanWriteEvent.Reset(); }}
      if (wres != 0)
        return HRESULT_FROM_WIN32(wres);
    }
    #else
    CThreadInfo &ti = ThreadsInfo;
    ti.Encoder = this;
    #endif

    ti.m_OptimizeNumTables = _props.DoOptimizeNumTables();

    if (!ti.Alloc())
      return E_OUTOFMEMORY;
  }


  if (!m_InStream.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!m_OutStream.Create(kBufferSize))
    return E_OUTOFMEMORY;


  m_InStream.SetStream(inStream);
  m_InStream.Init();

  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  CombinedCrc.Init();
  #ifndef Z7_ST
  NextBlockIndex = 0;
  StreamWasFinished = false;
  CloseThreads = false;
  CanStartWaitingEvent.Reset();
  #endif

  WriteByte(kArSig0);
  WriteByte(kArSig1);
  WriteByte(kArSig2);
  WriteByte((Byte)(kArSig3 + _props.BlockSizeMult));

  #ifndef Z7_ST

  if (MtMode)
  {
    ThreadsInfo[0].CanWriteEvent.Set();
    Result = S_OK;
    CanProcessEvent.Set();
    UInt32 t;
    for (t = 0; t < NumThreads; t++)
      ThreadsInfo[t].StreamWasFinishedEvent.Lock();
    CanProcessEvent.Reset();
    CanStartWaitingEvent.Set();
    for (t = 0; t < NumThreads; t++)
      ThreadsInfo[t].WaitingWasStartedEvent.Lock();
    CanStartWaitingEvent.Reset();
    RINOK(Result)
  }
  else
  #endif
  {
    for (;;)
    {
      CThreadInfo &ti =
      #ifndef Z7_ST
          ThreadsInfo[0];
      #else
          ThreadsInfo;
      #endif
      const UInt32 blockSize = ReadRleBlock(ti.m_Block);
      if (blockSize == 0)
        break;
      RINOK(ti.EncodeBlock3(blockSize))
      if (progress)
      {
        const UInt64 unpackSize = m_InStream.GetProcessedSize();
        const UInt64 packSize = m_OutStream.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&unpackSize, &packSize))
      }
    }
  }
  WriteByte(kFinSig0);
  WriteByte(kFinSig1);
  WriteByte(kFinSig2);
  WriteByte(kFinSig3);
  WriteByte(kFinSig4);
  WriteByte(kFinSig5);
  {
    const UInt32 v = CombinedCrc.GetDigest();
    for (int i = 24; i >= 0; i -= 8)
      WriteByte((Byte)(v >> i));
  }
  RINOK(Flush())
  if (!m_InStream.WasFinished())
    return E_FAIL;
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  int level = -1;
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];

    if (propID == NCoderPropID::kAffinity)
    {
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      props.Affinity = prop.uhVal.QuadPart;
      continue;
    }

    if (propID == NCoderPropID::kNumThreadGroups)
    {
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      props.NumThreadGroups = (UInt32)prop.ulVal;
      continue;
    }

    if (propID >= NCoderPropID::kReduceSize)
      continue;
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    const UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kNumPasses: props.NumPasses = v; break;
      case NCoderPropID::kDictionarySize: props.BlockSizeMult = v / kBlockSizeStep; break;
      case NCoderPropID::kLevel: level = (int)v; break;
      case NCoderPropID::kNumThreads:
      {
        #ifndef Z7_ST
        SetNumberOfThreads(v);
        #endif
        break;
      }
      default: return E_INVALIDARG;
    }
  }
  props.Normalize(level);
  _props = props;
  return S_OK;
}

#ifndef Z7_ST
Z7_COM7F_IMF(CEncoder::SetNumberOfThreads(UInt32 numThreads))
{
  const UInt32 kNumThreadsMax = 64;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  NumThreads = numThreads;
  return S_OK;
}
#endif

}}
