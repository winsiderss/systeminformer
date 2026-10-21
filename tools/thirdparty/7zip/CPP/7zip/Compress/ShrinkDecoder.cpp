// ShrinkDecoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/InBuffer.h"
#include "../Common/OutBuffer.h"

#include "BitlDecoder.h"
#include "ShrinkDecoder.h"

namespace NCompress {
namespace NShrink {

static const UInt32 kEmpty = 256; // kNumItems;
static const UInt32 kBufferSize = (1 << 18);
static const unsigned kNumMinBits = 9;

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  NBitl::CBaseDecoder<CInBuffer> inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;

  inBuffer.SetStream(inStream);
  inBuffer.Init();

  outBuffer.SetStream(outStream);
  outBuffer.Init();

  {
    for (unsigned i = 0; i < kNumItems; i++)
      _parents[i] = kEmpty;
  }

  UInt64 outPrev = 0, inPrev = 0;
  unsigned numBits = kNumMinBits;
  unsigned head = 257;
  int lastSym = -1;
  Byte lastChar = 0;
  bool moreOut = false;

  HRESULT res = S_FALSE;

  for (;;)
  {
    _inProcessed = inBuffer.GetProcessedSize();
    const UInt64 nowPos = outBuffer.GetProcessedSize();

    bool eofCheck = false;

    if (outSize && nowPos >= *outSize)
    {
      if (!_fullStreamMode || moreOut)
      {
        res = S_OK;
        break;
      }
      eofCheck = true;
      // Is specSym(=256) allowed after end of stream ?
      // Do we need to read it here ?
    }

    if (progress)
    {
      if (nowPos - outPrev >= (1 << 20) || _inProcessed - inPrev >= (1 << 20))
      {
        outPrev = nowPos;
        inPrev = _inProcessed;
        res = progress->SetRatioInfo(&_inProcessed, &nowPos);
        if (res != SZ_OK)
        {
          // break;
          return res;
        }
      }
    }

    UInt32 sym = inBuffer.ReadBits(numBits);

    if (inBuffer.ExtraBitsWereRead())
    {
      res = S_OK;
      break;
    }
    
    if (sym == 256)
    {
      sym = inBuffer.ReadBits(numBits);

      if (inBuffer.ExtraBitsWereRead())
        break;

      if (sym == 1)
      {
        if (numBits >= kNumMaxBits)
          break;
        numBits++;
        continue;
      }
      if (sym != 2)
      {
        break;
        // continue; // info-zip just ignores such code
      }
      {
        /*
        ---------- Free leaf nodes ----------
        Note : that code can mark _parents[lastSym] as free, and next
        inserted node will be Orphan in that case.
        */

        unsigned i;
        for (i = 256; i < kNumItems; i++)
          _stack[i] = 0;
        for (i = 257; i < kNumItems; i++)
        {
          unsigned par = _parents[i];
          if (par != kEmpty)
            _stack[par] = 1;
        }
        for (i = 257; i < kNumItems; i++)
          if (_stack[i] == 0)
            _parents[i] = kEmpty;
        head = 257;
        continue;
      }
    }

    if (eofCheck)
    {
      // It's can be error case.
      // That error can be detected later in (*inSize != _inProcessed) check.
      res = S_OK;
      break;
    }

    bool needPrev = false;
    if (head < kNumItems && lastSym >= 0)
    {
      while (head < kNumItems && _parents[head] != kEmpty)
        head++;
      if (head < kNumItems)
      {
        /*
        if (head == lastSym), it updates Orphan to self-linked Orphan and creates two problems:
            1) we must check _stack[i++] overflow in code that walks tree nodes.
            2) self-linked node can not be removed. So such self-linked nodes can occupy all _parents items.
        */
        needPrev = true;
        _parents[head] = (UInt16)lastSym;
        _suffixes[head] = (Byte)lastChar;
        head++;
      }
    }

    lastSym = (int)sym;
    unsigned cur = sym;
    unsigned i = 0;
    
    while (cur >= 256)
    {
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
      // don't change that code:
      // Orphan Check and self-linked Orphan check (_stack overflow check);
      if (cur == kEmpty || i >= kNumItems)
        break;
    }
    
    if (cur == kEmpty || i >= kNumItems)
      break;

    _stack[i++] = (Byte)cur;
    lastChar = (Byte)cur;

    if (needPrev)
      _suffixes[(size_t)head - 1] = (Byte)cur;

    if (outSize)
    {
      const UInt64 limit = *outSize - nowPos;
      if (i > limit)
      {
        moreOut = true;
        i = (unsigned)limit;
      }
    }

    do
      outBuffer.WriteByte(_stack[--i]);
    while (i);
  }
  
  RINOK(outBuffer.Flush())

  if (res == S_OK)
    if (_fullStreamMode)
    {
      if (moreOut)
        res = S_FALSE;
      const UInt64 nowPos = outBuffer.GetProcessedSize();
      if (outSize && *outSize != nowPos)
        res = S_FALSE;
      if (inSize && *inSize != _inProcessed)
        res = S_FALSE;
    }
  
  return res;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  // catch(const CInBufferException &e) { return e.ErrorCode; }
  // catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(const CSystemException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _fullStreamMode = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


}}
