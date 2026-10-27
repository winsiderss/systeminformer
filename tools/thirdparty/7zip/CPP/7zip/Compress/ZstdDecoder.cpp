// ZstdDecoder.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#include "ZstdDecoder.h"

namespace NCompress {
namespace NZstd {

static const size_t k_Zstd_BlockSizeMax = 1 << 17;
/*
  we set _outStepMask as (k_Zstd_BlockSizeMax - 1), because:
    - cycSize in zstd decoder for isCyclicMode is aligned for (1 << 17) only.
      So some write sizes will be multiple of ((1 << 17) * n).
    - Also it can be optimal to flush data after each block decoding.
*/

CDecoder::CDecoder():
    _outStepMask(k_Zstd_BlockSizeMax - 1) // must be = (1 << x) - 1
    , _dec(NULL)
    , _inProcessed(0)
    , _inBufSize(1u << 19) // larger value will reduce the number of memcpy() calls in CZstdDec code
    , _inBuf(NULL)
    , FinishMode(false)
    , DisableHash(False)
    // , DisableHash(True) // for debug : fast decoding without hash calculation
{
  // ZstdDecInfo_Clear(&ResInfo);
}

CDecoder::~CDecoder()
{
  if (_dec)
    ZstdDec_Destroy(_dec);
  MidFree(_inBuf);
}


Z7_COM7F_IMF(CDecoder::SetInBufSize(UInt32 , UInt32 size))
  { _inBufSize = size;  return S_OK; }
Z7_COM7F_IMF(CDecoder::SetOutBufSize(UInt32 , UInt32 size))
{
  // we round it down:
  size >>= 1;
  size |= size >> (1 << 0);
  size |= size >> (1 << 1);
  size |= size >> (1 << 2);
  size |= size >> (1 << 3);
  size |= size >> (1 << 4);
  _outStepMask = size; // it's (1 << x) - 1 now
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte * /* prop */, UInt32 /* size */))
{
  // if (size != 3 && size != 5) return E_NOTIMPL;
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  FinishMode = (finishMode != 0);
  // FinishMode = false; // for debug
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
{
  size_t cur = ZstdDec_ReadUnusedFromInBuf(_dec, _afterDecoding_tempPos, data, size);
  _afterDecoding_tempPos += cur;
  size -= (UInt32)cur;
  if (size)
  {
    const size_t rem = _state.inLim - _state.inPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size)
    {
      memcpy((Byte *)data + cur, _state.inBuf + _state.inPos, size);
      _state.inPos += size;
      cur += size;
    }
  }
  *processedSize = (UInt32)cur;
  return S_OK;
}



HRESULT CDecoder::Prepare(const UInt64 *outSize)
{
  _inProcessed = 0;
  _afterDecoding_tempPos = 0;
  ZstdDecState_Clear(&_state);
  ZstdDecInfo_CLEAR(&ResInfo)
  // _state.outStep = _outStepMask + 1; // must be = (1 << x)
  _state.disableHash = DisableHash;
  if (outSize)
  {
    _state.outSize_Defined = True;
    _state.outSize = *outSize;
    // _state.outSize = 0; // for debug
  }
  if (!_dec)
  {
    _dec = ZstdDec_Create(&g_AlignedAlloc, &g_BigAlloc);
    if (!_dec)
      return E_OUTOFMEMORY;
  }
  if (!_inBuf || _inBufSize != _inBufSize_Allocated)
  {
    MidFree(_inBuf);
    _inBuf = NULL;
    _inBufSize_Allocated = 0;
    _inBuf = (Byte *)MidAlloc(_inBufSize);
    if (!_inBuf)
      return E_OUTOFMEMORY;
    _inBufSize_Allocated = _inBufSize;
  }
  _state.inBuf = _inBuf;
  ZstdDec_Init(_dec);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  RINOK(Prepare(outSize))
  
  UInt64 inPrev = 0;
  UInt64 outPrev = 0;
  UInt64 writtenSize = 0;
  bool readWasFinished = false;
  SRes sres = SZ_OK;
  HRESULT hres = S_OK;
  HRESULT hres_Read = S_OK;
  
  for (;;)
  {
    if (_state.inPos == _state.inLim && !readWasFinished)
    {
      _state.inPos = 0;
      _state.inLim = _inBufSize;
      hres_Read = ReadStream(inStream, _inBuf, &_state.inLim);
      // _state.inLim -= 5; readWasFinished = True; // for debug
      if (_state.inLim != _inBufSize || hres_Read != S_OK)
      {
        // hres_Read = 99; // for debug
        readWasFinished = True;
      }
    }
    {
      const size_t inPos_Start = _state.inPos;
      sres = ZstdDec_Decode(_dec, &_state);
      _inProcessed += _state.inPos - inPos_Start;
    }
    /*
    if (_state.status == ZSTD_STATUS_FINISHED_FRAME)
      printf("\nfinished frame pos=%8x, checksum=%08x\n", (unsigned)_state.outProcessed, (unsigned)_state.info.checksum);
    */
    const bool needStop = (sres != SZ_OK)
        || _state.status == ZSTD_STATUS_OUT_REACHED
        || (outSize && *outSize < _state.outProcessed)
        || (readWasFinished && _state.inPos == _state.inLim
            && ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(&_state));
   
    size_t size = _state.winPos - _state.wrPos; // full write size
    if (size)
    {
      if (!needStop)
      {
        // we try to flush on aligned positions, if possible
        size = _state.needWrite_Size; // minimal required write size
        const size_t alignedPos = _state.winPos & ~(size_t)_outStepMask;
        if (alignedPos > _state.wrPos)
        {
          const size_t size2 = alignedPos - _state.wrPos;  // optimized aligned size
          if (size < size2)
              size = size2;
        }
      }
      if (size)
      {
        {
          size_t curSize = size;
          if (outSize)
          {
            const UInt64 rem = *outSize - writtenSize;
            if (curSize > rem)
              curSize = (size_t)rem;
          }
          if (curSize)
          {
            // printf("Write wrPos=%8x, size=%8x\n", (unsigned)_state.wrPos, (unsigned)size);
            hres = WriteStream(outStream, _state.win + _state.wrPos, curSize);
            if (hres != S_OK)
              break;
            writtenSize += curSize; // it's real size of data that was written to stream
          }
        }
        _state.wrPos += size; // virtual written size, that will be reported to CZstdDec
        // _state.needWrite_Size = 0; // optional
      }
    }
    
    if (needStop)
      break;
    if (progress)
    if (_inProcessed - inPrev >= (1 << 27)
        || _state.outProcessed - outPrev >= (1 << 28))
    {
      inPrev = _inProcessed;
      outPrev = _state.outProcessed;
      RINOK(progress->SetRatioInfo(&inPrev, &outPrev))
    }
  }

  if (hres == S_OK)
  {
    ZstdDec_GetResInfo(_dec, &_state, sres, &ResInfo);
    sres = ResInfo.decode_SRes;
    /* now (ResInfo.decode_SRes) can contain 2 extra error codes:
         - SZ_ERROR_NO_ARCHIVE  : if no frames
         - SZ_ERROR_INPUT_EOF   : if ZSTD_STATUS_NEEDS_MORE_INPUT
    */
    _inProcessed -= ResInfo.extraSize;
    if (hres_Read != S_OK && _state.inLim == _state.inPos && readWasFinished)
    {
      /* if (there is stream reading error,
           and decoding was stopped because of end of input stream),
           then we use reading error as main error code */
      if (sres == SZ_OK ||
          sres == SZ_ERROR_INPUT_EOF ||
          sres == SZ_ERROR_NO_ARCHIVE)
        hres = hres_Read;
    }
    if (sres == SZ_ERROR_INPUT_EOF && !FinishMode)
    {
      /* SZ_ERROR_INPUT_EOF case is allowed case for (!FinishMode) mode.
         So we restore SZ_OK result for that case: */
      ResInfo.decode_SRes = sres = SZ_OK;
    }
    if (hres == S_OK)
    {
      hres = SResToHRESULT(sres);
      if (hres == S_OK && FinishMode)
      {
        if ((inSize && *inSize != _inProcessed)
            || ResInfo.is_NonFinishedFrame
            || (outSize && (*outSize != writtenSize || writtenSize != _state.outProcessed)))
          hres = S_FALSE;
      }
    }
  }
  return hres;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


#ifndef Z7_NO_READ_FROM_CODER_ZSTD

Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  _inProcessed = 0;
  _hres_Read = S_OK;
  _hres_Decode = S_OK;
  _writtenSize = 0;
  _readWasFinished = false;
  _wasFinished = false;
  return Prepare(outSize);
}


Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
  { _inStream = inStream; return S_OK; }
Z7_COM7F_IMF(CDecoder::ReleaseInStream())
  { _inStream.Release(); return S_OK; }


// if SetInStream() mode: the caller must call GetFinishResult() after full decoding
// to check that there decoding was finished correctly

HRESULT CDecoder::GetFinishResult()
{
  if (_state.winPos != _state.wrPos || !_wasFinished)
    return FinishMode ? S_FALSE : S_OK;
  // _state.winPos == _state.wrPos
  // _wasFinished == true
  if (FinishMode && _hres_Decode == S_OK && _state.outSize_Defined && _state.outSize != _writtenSize)
    _hres_Decode = S_FALSE;
  return _hres_Decode;
}
  

Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;

  for (;;)
  {
    if (_state.outSize_Defined)
    {
      // _writtenSize <= _state.outSize
      const UInt64 rem = _state.outSize - _writtenSize;
      if (size > rem)
        size = (UInt32)rem;
    }
    {
      size_t cur = _state.winPos - _state.wrPos;
      if (cur)
      {
        // _state.winPos != _state.wrPos;
        // so there is some decoded data that was not written still
        if (size == 0)
        {
          // if (FinishMode) and we are not allowed to write more, then it's data error
          if (FinishMode && _state.outSize_Defined && _state.outSize == _writtenSize)
            return S_FALSE;
          return S_OK;
        }
        if (cur > size)
          cur = (size_t)size;
        // cur != 0
        memcpy(data, _state.win + _state.wrPos, cur);
        _state.wrPos += cur;
        _writtenSize += cur;
        data = (void *)((Byte *)data + cur);
        if (processedSize)
          *processedSize += (UInt32)cur;
        size -= (UInt32)cur;
        continue;
      }
    }

    // _state.winPos == _state.wrPos
    if (_wasFinished)
    {
      if (_hres_Decode == S_OK && FinishMode
          && _state.outSize_Defined && _state.outSize != _writtenSize)
        _hres_Decode = S_FALSE;
      return _hres_Decode;
    }

    // _wasFinished == false
    if (size == 0 && _state.outSize_Defined && _state.outSize != _state.outProcessed)
    {
      /* size == 0 : so the caller don't need more data now.
         _state.outSize > _state.outProcessed : so more data will be requested
         later by caller for full processing.
         So we exit without ZstdDec_Decode() call, because we don't want
         ZstdDec_Decode() to start new block decoding
      */
      return S_OK;
    }
    // size != 0  || !_state.outSize_Defined || _state.outSize == _state.outProcessed)

    if (_state.inPos == _state.inLim && !_readWasFinished)
    {
      _state.inPos = 0;
      _state.inLim = _inBufSize;
      _hres_Read = ReadStream(_inStream, _inBuf, &_state.inLim);
      if (_state.inLim != _inBufSize || _hres_Read != S_OK)
      {
        // _hres_Read = 99; // for debug
        _readWasFinished = True;
      }
    }

    SRes sres;
    {
      const SizeT inPos_Start = _state.inPos;
      sres = ZstdDec_Decode(_dec, &_state);
      _inProcessed += _state.inPos - inPos_Start;
    }

    const bool inFinished = (_state.inPos == _state.inLim) && _readWasFinished;

    _wasFinished = (sres != SZ_OK)
        || _state.status == ZSTD_STATUS_OUT_REACHED
        || (_state.outSize_Defined && _state.outSize < _state.outProcessed)
        || (inFinished
            && ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(&_state));

    if (!_wasFinished)
      continue;
    
    // _wasFinished == true
    /* (_state.winPos != _state.wrPos) is possible here.
       So we still can have some data to flush,
       but we must all result variables .
    */
    HRESULT hres = S_OK;
    ZstdDec_GetResInfo(_dec, &_state, sres, &ResInfo);
    sres = ResInfo.decode_SRes;
    _inProcessed -= ResInfo.extraSize;
    if (_hres_Read != S_OK && inFinished)
    {
      if (sres == SZ_OK ||
          sres == SZ_ERROR_INPUT_EOF ||
          sres == SZ_ERROR_NO_ARCHIVE)
        hres = _hres_Read;
    }
    if (sres == SZ_ERROR_INPUT_EOF && !FinishMode)
      ResInfo.decode_SRes = sres = SZ_OK;
    if (hres == S_OK)
    {
      hres = SResToHRESULT(sres);
      if (hres == S_OK && FinishMode)
        if (!inFinished
            || ResInfo.is_NonFinishedFrame
            || (_state.outSize_Defined && _state.outSize != _state.outProcessed))
          hres = S_FALSE;
    }
    _hres_Decode = hres;
  }
}

#endif

}}
