// PpmdZip.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../Common/RegisterCodec.h"
#include "../Common/StreamUtils.h"

#include "PpmdZip.h"

namespace NCompress {
namespace NPpmdZip {

static const UInt32 kBufSize = 1 << 20;

bool CBuf::Alloc()
{
  if (!Buf)
    Buf = (Byte *)::MidAlloc(kBufSize);
  return (Buf != NULL);
}

CDecoder::CDecoder(bool fullFileMode):
  _fullFileMode(fullFileMode)
{
  Ppmd8_Construct(&_ppmd);
  _ppmd.Stream.In = &_inStream.vt;
}

CDecoder::~CDecoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  // try {

  if (!_outStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_inStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;

  _inStream.Stream = inStream;
  _inStream.Init();

  {
    Byte buf[2];
    for (int i = 0; i < 2; i++)
      buf[i] = _inStream.ReadByte();
    if (_inStream.Extra)
      return S_FALSE;
    
    const UInt32 val = GetUi16(buf);
    const unsigned order = (val & 0xF) + 1;
    const UInt32 mem = ((val >> 4) & 0xFF) + 1;
    const unsigned restor = (val >> 12);
    if (order < 2 || restor > 2)
      return S_FALSE;
    
    #ifndef PPMD8_FREEZE_SUPPORT
    if (restor == 2)
      return E_NOTIMPL;
    #endif
    
    if (!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc))
      return E_OUTOFMEMORY;
    
    if (!Ppmd8_Init_RangeDec(&_ppmd))
      return S_FALSE;
    Ppmd8_Init(&_ppmd, order, restor);
  }

  bool wasFinished = false;
  UInt64 processedSize = 0;

  for (;;)
  {
    size_t size = kBufSize;
    if (outSize)
    {
      const UInt64 rem = *outSize - processedSize;
      if (size > rem)
      {
        size = (size_t)rem;
        if (size == 0)
          break;
      }
    }

    int sym;
    Byte *buf = _outStream.Buf;
    const Byte *lim = buf + size;
    
    do
    {
      sym = Ppmd8_DecodeSymbol(&_ppmd);
      if (_inStream.Extra || sym < 0)
        break;
      *buf++ = (Byte)sym;
    }
    while (buf != lim);
    
    const size_t cur = (size_t)(buf - _outStream.Buf);
    processedSize += cur;

    RINOK(WriteStream(outStream, _outStream.Buf, cur))

    RINOK(_inStream.Res)
    if (_inStream.Extra)
      return S_FALSE;

    if (sym < 0)
    {
      if (sym != -1)
        return S_FALSE;
      wasFinished = true;
      break;
    }
    
    if (progress)
    {
      const UInt64 inProccessed = _inStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&inProccessed, &processedSize))
    }
  }
  
  RINOK(_inStream.Res)
  
  if (_fullFileMode)
  {
    if (!wasFinished)
    {
      const int res = Ppmd8_DecodeSymbol(&_ppmd);
      RINOK(_inStream.Res)
      if (_inStream.Extra || res != -1)
        return S_FALSE;
    }
    if (!Ppmd8_RangeDec_IsFinishedOK(&_ppmd))
      return S_FALSE;

    if (inSize && *inSize != _inStream.GetProcessed())
      return S_FALSE;
  }
  
  return S_OK;

  // } catch (...) { return E_FAIL; }
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _fullFileMode = (finishMode != 0);
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inStream.GetProcessed();
  return S_OK;
}



// ---------- Encoder ----------

void CEncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level == 0) level = 1;
  if (level > 9) level = 9;
  if (MemSizeMB == (UInt32)(Int32)-1)
    MemSizeMB = 1 << (level - 1);
  const unsigned kMult = 16;
  for (UInt32 m = 1; m < MemSizeMB; m <<= 1)
    if (ReduceSize <= (m << 20) / kMult)
    {
      MemSizeMB = m;
      break;
    }
  if (Order == -1) Order = 3 + level;
  if (Restor == -1)
    Restor = level < 7 ?
      PPMD8_RESTORE_METHOD_RESTART :
      PPMD8_RESTORE_METHOD_CUT_OFF;
}

CEncoder::~CEncoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  int level = -1;
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID > NCoderPropID::kReduceSize)
      continue;
    if (propID == NCoderPropID::kReduceSize)
    {
      props.ReduceSize = (UInt32)(Int32)-1;
      if (prop.vt == VT_UI8 && prop.uhVal.QuadPart < (UInt32)(Int32)-1)
        props.ReduceSize = (UInt32)prop.uhVal.QuadPart;
      continue;
    }
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    const UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kUsedMemorySize:
        if (v < (1 << 20) || v > (1 << 28))
          return E_INVALIDARG;
        props.MemSizeMB = v >> 20;
        break;
      case NCoderPropID::kOrder:
        if (v < PPMD8_MIN_ORDER || v > PPMD8_MAX_ORDER)
          return E_INVALIDARG;
        props.Order = (Byte)v;
        break;
      case NCoderPropID::kNumThreads: break;
      case NCoderPropID::kLevel: level = (int)v; break;
      case NCoderPropID::kAlgorithm:
        if (v >= PPMD8_RESTORE_METHOD_UNSUPPPORTED)
          return E_INVALIDARG;
        props.Restor = (int)v;
        break;
      default: return E_INVALIDARG;
    }
  }
  props.Normalize(level);
  _props = props;
  return S_OK;
}

CEncoder::CEncoder()
{
  _props.Normalize(-1);
  _ppmd.Stream.Out = &_outStream.vt;
  Ppmd8_Construct(&_ppmd);
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  if (!_inStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_outStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd8_Alloc(&_ppmd, _props.MemSizeMB << 20, &g_BigAlloc))
    return E_OUTOFMEMORY;

  _outStream.Stream = outStream;
  _outStream.Init();

  Ppmd8_Init_RangeEnc(&_ppmd)
  Ppmd8_Init(&_ppmd, (unsigned)_props.Order, (unsigned)_props.Restor);

  {
    const unsigned val =
           ((unsigned)_props.Order - 1)
         + (((unsigned)_props.MemSizeMB - 1) << 4)
         + ((unsigned)_props.Restor << 12);
    _outStream.WriteByte((Byte)(val & 0xFF));
    _outStream.WriteByte((Byte)(val >> 8));
  }
  RINOK(_outStream.Res)

  UInt64 processed = 0;
  for (;;)
  {
    UInt32 size;
    RINOK(inStream->Read(_inStream.Buf, kBufSize, &size))
    if (size == 0)
    {
      Ppmd8_EncodeSymbol(&_ppmd, -1);
      Ppmd8_Flush_RangeEnc(&_ppmd);
      return _outStream.Flush();
    }

    processed += size;
    const Byte *buf = _inStream.Buf;
    const Byte *lim = buf + size;
    do
    {
      Ppmd8_EncodeSymbol(&_ppmd, *buf);
      if (_outStream.Res != S_OK)
        break;
    }
    while (++buf != lim);

    RINOK(_outStream.Res)

    if (progress)
    {
      const UInt64 outProccessed = _outStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&processed, &outProccessed))
    }
  }
}

}}
