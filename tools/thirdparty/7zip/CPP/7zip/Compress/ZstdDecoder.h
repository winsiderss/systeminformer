// ZstdDecoder.h

#ifndef ZIP7_INC_ZSTD_DECODER_H
#define ZIP7_INC_ZSTD_DECODER_H

#include "../../../C/ZstdDec.h"

#include "../../Common/MyCom.h"
#include "../ICoder.h"

namespace NCompress {
namespace NZstd {

#ifdef Z7_NO_READ_FROM_CODER
#define Z7_NO_READ_FROM_CODER_ZSTD
#endif

#ifndef Z7_NO_READ_FROM_CODER_ZSTD
// #define Z7_NO_READ_FROM_CODER_ZSTD
#endif

class CDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
  public ICompressSetBufSize,
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetDecoderProperties2)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
  Z7_COM_QI_ENTRY(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
  Z7_COM_QI_ENTRY(ISequentialInStream)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
  Z7_IFACE_COM7_IMP(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
 #endif

  HRESULT Prepare(const UInt64 *outSize);

  UInt32 _outStepMask;
  CZstdDecHandle _dec;
public:
  UInt64 _inProcessed;
  CZstdDecState _state;

private:
  UInt32 _inBufSize;
  UInt32 _inBufSize_Allocated;
  Byte *_inBuf;
  size_t _afterDecoding_tempPos;

 #ifndef Z7_NO_READ_FROM_CODER_ZSTD
  CMyComPtr<ISequentialInStream> _inStream;
  HRESULT _hres_Read;
  HRESULT _hres_Decode;
  UInt64 _writtenSize;
  bool _readWasFinished;
  bool _wasFinished;
 #endif

public:
  bool FinishMode;
  Byte DisableHash;
  CZstdDecResInfo ResInfo;

  HRESULT GetFinishResult();

  CDecoder();
  ~CDecoder();
};

}}

#endif
