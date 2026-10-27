// ImplodeDecoder.cpp

#include "StdAfx.h"

#include "../../Common/Defs.h"

#include "ImplodeDecoder.h"

namespace NCompress {
namespace NImplode {
namespace NDecoder {

bool CHuffmanDecoder::Build(const Byte *lens, unsigned numSymbols) throw()
{
  unsigned counts[kNumHuffmanBits + 1];
  unsigned i;
  for (i = 0; i <= kNumHuffmanBits; i++)
    counts[i] = 0;
  for (i = 0; i < numSymbols; i++)
    counts[lens[i]]++;

  const UInt32 kMaxValue = (UInt32)1 << kNumHuffmanBits;
  // _limits[0] = kMaxValue;
  UInt32 startPos = kMaxValue;
  unsigned sum = 0;

  for (i = 1; i <= kNumHuffmanBits; i++)
  {
    const unsigned cnt = counts[i];
    const UInt32 range = (UInt32)cnt << (kNumHuffmanBits - i);
    if (startPos < range)
      return false;
    startPos -= range;
    _limits[i] = startPos;
    _poses[i] = sum;
    sum += cnt;
    counts[i] = sum;
  }
  // counts[0] += sum;
  if (startPos != 0)
    return false;
  for (i = 0; i < numSymbols; i++)
  {
    const unsigned len = lens[i];
    if (len != 0)
      _symbols[--counts[len]] = (Byte)i;
  }
  return true;
}


unsigned CHuffmanDecoder::Decode(CInBit *inStream) const throw()
{
  const UInt32 val = inStream->GetValue(kNumHuffmanBits);
  size_t numBits;
  for (numBits = 1; val < _limits[numBits]; numBits++);
  const unsigned sym = _symbols[_poses[numBits]
      + (unsigned)((val - _limits[numBits]) >> (kNumHuffmanBits - numBits))];
  inStream->MovePos(numBits);
  return sym;
}



static const unsigned kNumLenDirectBits = 8;

static const unsigned kNumDistDirectBitsSmall = 6;
static const unsigned kNumDistDirectBitsBig = 7;

static const unsigned kLitTableSize = (1 << 8);
static const unsigned kDistTableSize = 64;
static const unsigned kLenTableSize = 64;

static const UInt32 kHistorySize = (1 << kNumDistDirectBitsBig) * kDistTableSize; // 8 KB


CCoder::CCoder():
  _flags(0),
  _fullStreamMode(false)
{}


bool CCoder::BuildHuff(CHuffmanDecoder &decoder, unsigned numSymbols)
{
  Byte levels[kMaxHuffTableSize];
  unsigned numRecords = (unsigned)_inBitStream.ReadAlignedByte() + 1;
  unsigned index = 0;
  do
  {
    const unsigned b = (unsigned)_inBitStream.ReadAlignedByte();
    const unsigned level = (b & 0xF) + 1;
    const unsigned rep = ((unsigned)b >> 4) + 1;
    if (index + rep > numSymbols)
      return false;
    for (unsigned j = 0; j < rep; j++)
      levels[index++] = (Byte)level;
  }
  while (--numRecords);

  if (index != numSymbols)
    return false;
  return decoder.Build(levels, numSymbols);
}


HRESULT CCoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (!_inBitStream.Create(1 << 18))
    return E_OUTOFMEMORY;
  if (!_outWindowStream.Create(kHistorySize << 1)) // 16 KB
    return E_OUTOFMEMORY;
  if (!outSize)
    return E_INVALIDARG;

  _outWindowStream.SetStream(outStream);
  _outWindowStream.Init(false);
  _inBitStream.SetStream(inStream);
  _inBitStream.Init();

  const unsigned numDistDirectBits = (_flags & 2) ?
      kNumDistDirectBitsBig:
      kNumDistDirectBitsSmall;
  const bool literalsOn = ((_flags & 4) != 0);
  const UInt32 minMatchLen = (literalsOn ? 3 : 2);

  if (literalsOn)
    if (!BuildHuff(_litDecoder, kLitTableSize))
      return S_FALSE;
  if (!BuildHuff(_lenDecoder, kLenTableSize))
    return S_FALSE;
  if (!BuildHuff(_distDecoder, kDistTableSize))
    return S_FALSE;
 
  UInt64 prevProgress = 0;
  bool moreOut = false;
  UInt64 pos = 0, unPackSize = *outSize;

  while (pos < unPackSize)
  {
    if (pos - prevProgress >= (1u << 18) && progress)
    {
      prevProgress = pos;
      const UInt64 packSize = _inBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos))
    }

    if (_inBitStream.ReadBits(1) != 0)
    {
      Byte b;
      if (literalsOn)
      {
        const unsigned sym = _litDecoder.Decode(&_inBitStream);
        // if (sym >= kLitTableSize) break;
        b = (Byte)sym;
      }
      else
        b = (Byte)_inBitStream.ReadBits(8);
      _outWindowStream.PutByte(b);
      pos++;
    }
    else
    {
      const UInt32 lowDistBits = _inBitStream.ReadBits(numDistDirectBits);
      UInt32 dist = (UInt32)_distDecoder.Decode(&_inBitStream);
      // if (dist >= kDistTableSize) break;
      dist = (dist << numDistDirectBits) + lowDistBits;
      unsigned len = _lenDecoder.Decode(&_inBitStream);
      // if (len >= kLenTableSize) break;
      if (len == kLenTableSize - 1)
        len += _inBitStream.ReadBits(kNumLenDirectBits);
      len += minMatchLen;
      {
        const UInt64 limit = unPackSize - pos;
        // limit != 0
        if (len > limit)
        {
          moreOut = true;
          len = (UInt32)limit;
        }
      }
      do
      {
        // len != 0
        if (dist < pos)
        {
          _outWindowStream.CopyBlock(dist, len);
          pos += len;
          break;
        }
        _outWindowStream.PutByte(0);
        pos++;
      }
      while (--len);
    }
  }

  HRESULT res = _outWindowStream.Flush();

  if (res == S_OK)
  {
    if (_fullStreamMode)
    {
      if (moreOut)
        res = S_FALSE;
      if (inSize && *inSize != _inBitStream.GetProcessedSize())
        res = S_FALSE;
    }
    if (pos != unPackSize)
      res = S_FALSE;
  }

  return res;
}


Z7_COM7F_IMF(CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);  }
  // catch(const CInBufferException &e)  { return e.ErrorCode; }
  // catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(const CSystemException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


Z7_COM7F_IMF(CCoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size == 0)
    return E_NOTIMPL;
  _flags = data[0];
  return S_OK;
}


Z7_COM7F_IMF(CCoder::SetFinishMode(UInt32 finishMode))
{
  _fullStreamMode = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inBitStream.GetProcessedSize();
  return S_OK;
}

}}}
