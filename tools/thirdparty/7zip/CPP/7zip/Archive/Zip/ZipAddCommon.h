// ZipAddCommon.h

#ifndef ZIP7_INC_ZIP_ADD_COMMON_H
#define ZIP7_INC_ZIP_ADD_COMMON_H

#include "../../ICoder.h"
#include "../../IProgress.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/FilterCoder.h"

#include "../../Compress/CopyCoder.h"

#include "../../Crypto/ZipCrypto.h"
#include "../../Crypto/WzAes.h"

#include "ZipCompressionMode.h"

namespace NArchive {
namespace NZip {

struct CCompressingResult
{
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt32 CRC;
  UInt16 Method;
  Byte ExtractVersion;
  bool DescriptorMode;
  bool LzmaEos;

  CCompressingResult()
  {
    // for GCC:
    UnpackSize = 0;
  }
};

class CAddCommon  MY_UNCOPYABLE
{
  CCompressionMethodMode _options;
  CMyComPtr2<ICompressCoder, NCompress::CCopyCoder> _copyCoder;

  CMyComPtr<ICompressCoder> _compressEncoder;
  Byte _compressExtractVersion;
  bool _isLzmaEos;

  CMyComPtr2<ISequentialOutStream, CFilterCoder> _cryptoStream;

  NCrypto::NZip::CEncoder *_filterSpec;
  NCrypto::NWzAes::CEncoder *_filterAesSpec;

  Byte *_buf;
  
  HRESULT CalcStreamCRC(ISequentialInStream *inStream, UInt32 &resultCRC);
public:
  // CAddCommon(const CCompressionMethodMode &options);
  CAddCommon();
  void SetOptions(const CCompressionMethodMode &options);
  ~CAddCommon();

  HRESULT Set_Pre_CompressionResult(bool inSeqMode, bool outSeqMode, UInt64 unpackSize,
      CCompressingResult &opRes) const;
  
  HRESULT Compress(
      DECL_EXTERNAL_CODECS_LOC_VARS
      ISequentialInStream *inStream, IOutStream *outStream,
      bool inSeqMode, bool outSeqMode,
      UInt32 fileTime,
      UInt64 expectedDataSize, bool expectedDataSize_IsConfirmed,
      ICompressProgressInfo *progress, CCompressingResult &opRes);
};

}}

#endif
