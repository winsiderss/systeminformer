// ZipAddCommon.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"
#include "../../../../C/Alloc.h"

#include "../../../Windows/PropVariant.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../MyVersion.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/LzmaEncoder.h"
#include "../../Compress/PpmdZip.h"
#include "../../Compress/XzEncoder.h"

#include "../Common/InStreamWithCRC.h"

#include "ZipAddCommon.h"
#include "ZipHeader.h"

namespace NArchive {
namespace NZip {

using namespace NFileHeader;


static const unsigned kLzmaPropsSize = 5;
static const unsigned kLzmaHeaderSize = 4 + kLzmaPropsSize;

Z7_CLASS_IMP_NOQIB_3(
  CLzmaEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressSetCoderPropertiesOpt
)
public:
  CMyComPtr2<ICompressCoder, NCompress::NLzma::CEncoder> Encoder;
  Byte Header[kLzmaHeaderSize];
};

Z7_COM7F_IMF(CLzmaEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  Encoder.Create_if_Empty();
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> outStream;
  outStream->Init(Header + 4, kLzmaPropsSize);
  RINOK(Encoder->SetCoderProperties(propIDs, props, numProps))
  RINOK(Encoder->WriteCoderProperties(outStream))
  if (outStream->GetPos() != kLzmaPropsSize)
    return E_FAIL;
  Header[0] = MY_VER_MAJOR;
  Header[1] = MY_VER_MINOR;
  Header[2] = kLzmaPropsSize;
  Header[3] = 0;
  return S_OK;
}

Z7_COM7F_IMF(CLzmaEncoder::SetCoderPropertiesOpt(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  return Encoder->SetCoderPropertiesOpt(propIDs, props, numProps);
}

Z7_COM7F_IMF(CLzmaEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  RINOK(WriteStream(outStream, Header, kLzmaHeaderSize))
  return Encoder.Interface()->Code(inStream, outStream, inSize, outSize, progress);
}


CAddCommon::CAddCommon():
    _isLzmaEos(false),
    _buf(NULL)
    {}

void CAddCommon::SetOptions(const CCompressionMethodMode &options)
{
  _options = options;
}

CAddCommon::~CAddCommon()
{
  MidFree(_buf);
}

static const UInt32 kBufSize = ((UInt32)1 << 16);

HRESULT CAddCommon::CalcStreamCRC(ISequentialInStream *inStream, UInt32 &resultCRC)
{
  if (!_buf)
  {
    _buf = (Byte *)MidAlloc(kBufSize);
    if (!_buf)
      return E_OUTOFMEMORY;
  }

  UInt32 crc = CRC_INIT_VAL;
  for (;;)
  {
    UInt32 processed;
    RINOK(inStream->Read(_buf, kBufSize, &processed))
    if (processed == 0)
    {
      resultCRC = CRC_GET_DIGEST(crc);
      return S_OK;
    }
    crc = CrcUpdate(crc, _buf, (size_t)processed);
  }
}


HRESULT CAddCommon::Set_Pre_CompressionResult(bool inSeqMode, bool outSeqMode, UInt64 unpackSize,
    CCompressingResult &opRes) const
{
  // We use Zip64, if unPackSize size is larger than 0xF8000000 to support
  // cases when compressed size can be about 3% larger than uncompressed size

  const UInt32 kUnpackZip64Limit = 0xF8000000;
  
  opRes.UnpackSize = unpackSize;
  opRes.PackSize = (UInt64)1 << 60; // we use big value to force Zip64 mode.

  if (unpackSize < kUnpackZip64Limit)
    opRes.PackSize = (UInt32)0xFFFFFFFF - 1; // it will not use Zip64 for that size

  if (opRes.PackSize < unpackSize)
    opRes.PackSize = unpackSize;

  const Byte method = _options.MethodSequence[0];

  if (method == NCompressionMethod::kStore && !_options.Password_Defined)
    opRes.PackSize = unpackSize;

  opRes.CRC = 0;

  opRes.LzmaEos = false;

  opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
  opRes.DescriptorMode = outSeqMode;
  
  if (_options.Password_Defined)
  {
    opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;
    if (_options.IsAesMode)
      opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
    else
    {
      if (inSeqMode)
        opRes.DescriptorMode = true;
    }
  }
  
  opRes.Method = method;
  Byte ver = 0;
  
  switch (method)
  {
    case NCompressionMethod::kStore: break;
    case NCompressionMethod::kDeflate: ver = NCompressionMethod::kExtractVersion_Deflate; break;
    case NCompressionMethod::kDeflate64: ver = NCompressionMethod::kExtractVersion_Deflate64; break;
    case NCompressionMethod::kXz   : ver = NCompressionMethod::kExtractVersion_Xz; break;
    case NCompressionMethod::kPPMd : ver = NCompressionMethod::kExtractVersion_PPMd; break;
    case NCompressionMethod::kBZip2: ver = NCompressionMethod::kExtractVersion_BZip2; break;
    case NCompressionMethod::kLZMA :
    {
      ver = NCompressionMethod::kExtractVersion_LZMA;
      const COneMethodInfo *oneMethodMain = &_options._methods[0];
      opRes.LzmaEos = oneMethodMain->Get_Lzma_Eos();
      break;
    }
    default: break;
  }
  if (opRes.ExtractVersion < ver)
      opRes.ExtractVersion = ver;

  return S_OK;
}


HRESULT CAddCommon::Compress(
    DECL_EXTERNAL_CODECS_LOC_VARS
    ISequentialInStream *inStream, IOutStream *outStream,
    bool inSeqMode, bool outSeqMode,
    UInt32 fileTime,
    UInt64 expectedDataSize, bool expectedDataSize_IsConfirmed,
    ICompressProgressInfo *progress, CCompressingResult &opRes)
{
  // opRes.LzmaEos = false;

  if (!inStream)
  {
    // We can create empty stream here. But it was already implemented in caller code in 9.33+
    return E_INVALIDARG;
  }

  CMyComPtr2_Create<ISequentialInStream, CSequentialInStreamWithCRC> inCrcStream;
  
  CMyComPtr<IInStream> inStream2;
  if (!inSeqMode)
  {
    inStream->QueryInterface(IID_IInStream, (void **)&inStream2);
    if (!inStream2)
    {
      // inSeqMode = true;
      // inSeqMode must be correct before
      return E_FAIL;
    }
  }

  inCrcStream->SetStream(inStream);
  inCrcStream->SetFullSize(expectedDataSize_IsConfirmed ? expectedDataSize : (UInt64)(Int64)-1);
  // inCrcStream->Init();

  unsigned numTestMethods = _options.MethodSequence.Size();
  // numTestMethods != 0

  bool descriptorMode = outSeqMode;
  
  // ZipCrypto without descriptor requires additional reading pass for
  // inStream to calculate CRC for password check field.
  // The descriptor allows to use ZipCrypto check field without CRC (InfoZip's modification).

  if (!outSeqMode)
    if (inSeqMode && _options.Password_Defined && !_options.IsAesMode)
      descriptorMode = true;
  opRes.DescriptorMode = descriptorMode;

  if (numTestMethods > 1)
    if (inSeqMode || outSeqMode || !inStream2)
      numTestMethods = 1;

  UInt32 crc = 0;
  bool crc_IsCalculated = false;
  
  CFilterCoder::C_OutStream_Releaser outStreamReleaser;
  // opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
  
  for (unsigned i = 0; i < numTestMethods; i++)
  {
    inCrcStream->Init();

    if (i != 0)
    {
      // if (inStream2)
      {
        RINOK(InStream_SeekToBegin(inStream2))
      }
      RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL))
      RINOK(outStream->SetSize(0))
    }

    opRes.LzmaEos = false;
    opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;

    const Byte method = _options.MethodSequence[i];
    if (method == NCompressionMethod::kStore && descriptorMode)
    {
      // we still can create descriptor_mode archives with "Store" method, but they are not good for 100%
      return E_NOTIMPL;
    }
    
    bool needCode = true;
    
    if (_options.Password_Defined)
    {
      opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;

      if (!_cryptoStream.IsDefined())
        _cryptoStream.SetFromCls(new CFilterCoder(true));
      
      if (_options.IsAesMode)
      {
        opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
        if (!_cryptoStream->Filter)
        {
          _cryptoStream->Filter = _filterAesSpec = new NCrypto::NWzAes::CEncoder;
          _filterAesSpec->SetKeyMode(_options.AesKeyMode);
          RINOK(_filterAesSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Len()))
        }
        RINOK(_filterAesSpec->WriteHeader(outStream))
      }
      else
      {
        if (!_cryptoStream->Filter)
        {
          _cryptoStream->Filter = _filterSpec = new NCrypto::NZip::CEncoder;
          _filterSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Len());
        }
        
        UInt32 check;
        
        if (descriptorMode)
        {
          // it's Info-ZIP modification for stream_mode descriptor_mode (bit 3 of the general purpose bit flag is set)
          check = (fileTime & 0xFFFF);
        }
        else
        {
          if (!crc_IsCalculated)
          {
            RINOK(CalcStreamCRC(inStream, crc))
            crc_IsCalculated = true;
            RINOK(InStream_SeekToBegin(inStream2))
            inCrcStream->Init();
          }
          check = (crc >> 16);
        }
        
        RINOK(_filterSpec->WriteHeader_Check16(outStream, (UInt16)check))
      }
      
      if (method == NCompressionMethod::kStore)
      {
        needCode = false;
        RINOK(_cryptoStream->Code(inCrcStream, outStream, NULL, NULL, progress))
      }
      else
      {
        RINOK(_cryptoStream->SetOutStream(outStream))
        RINOK(_cryptoStream->InitEncoder())
        outStreamReleaser.FilterCoder = _cryptoStream.ClsPtr();
      }
    }

    if (needCode)
    {
      switch (method)
      {
      case NCompressionMethod::kStore:
      {
        _copyCoder.Create_if_Empty();
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.Password_Defined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        RINOK(_copyCoder.Interface()->Code(inCrcStream, outStreamNew, NULL, NULL, progress))
        break;
      }
      
      default:
      {
        if (!_compressEncoder)
        {
          CLzmaEncoder *_lzmaEncoder = NULL;
          if (method == NCompressionMethod::kLZMA)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_LZMA;
            _lzmaEncoder = new CLzmaEncoder();
            _compressEncoder = _lzmaEncoder;
          }
          else if (method == NCompressionMethod::kXz)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_Xz;
            NCompress::NXz::CEncoder *encoder = new NCompress::NXz::CEncoder();
            _compressEncoder = encoder;
          }
          else if (method == NCompressionMethod::kPPMd)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_PPMd;
            NCompress::NPpmdZip::CEncoder *encoder = new NCompress::NPpmdZip::CEncoder();
            _compressEncoder = encoder;
          }
          else
          {
          CMethodId methodId;
          switch (method)
          {
            case NCompressionMethod::kBZip2:
              methodId = kMethodId_BZip2;
              _compressExtractVersion = NCompressionMethod::kExtractVersion_BZip2;
              break;
            default:
              _compressExtractVersion = ((method == NCompressionMethod::kDeflate64) ?
                  NCompressionMethod::kExtractVersion_Deflate64 :
                  NCompressionMethod::kExtractVersion_Deflate);
              methodId = kMethodId_ZipBase + method;
              break;
          }
          RINOK(CreateCoder_Id(
              EXTERNAL_CODECS_LOC_VARS
              methodId, true, _compressEncoder))
          if (!_compressEncoder)
            return E_NOTIMPL;

          if (method == NCompressionMethod::kDeflate ||
              method == NCompressionMethod::kDeflate64)
          {
          }
          else if (method == NCompressionMethod::kBZip2)
          {
          }
          }
          {
            CMyComPtr<ICompressSetCoderProperties> setCoderProps;
            _compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProps);
            if (setCoderProps)
            {
              if (!_options._methods.IsEmpty())
              {
                COneMethodInfo *oneMethodMain = &_options._methods[0];

                RINOK(oneMethodMain->SetCoderProps(setCoderProps,
                    _options.DataSizeReduce_Defined ? &_options.DataSizeReduce : NULL))
              }
            }
          }
          if (method == NCompressionMethod::kLZMA)
            _isLzmaEos = _lzmaEncoder->Encoder->IsWriteEndMark();
        }

        if (method == NCompressionMethod::kLZMA)
          opRes.LzmaEos = _isLzmaEos;

        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.Password_Defined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        if (_compressExtractVersion > opRes.ExtractVersion)
          opRes.ExtractVersion = _compressExtractVersion;

        {
          CMyComPtr<ICompressSetCoderPropertiesOpt> optProps;
          _compressEncoder->QueryInterface(IID_ICompressSetCoderPropertiesOpt, (void **)&optProps);
          if (optProps)
          {
            const PROPID propID = NCoderPropID::kExpectedDataSize;
            NWindows::NCOM::CPropVariant prop = (UInt64)expectedDataSize;
            RINOK(optProps->SetCoderPropertiesOpt(&propID, &prop, 1))
          }
        }
        
        try {
        RINOK(_compressEncoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress))
        } catch (...) { return E_FAIL; }
        break;
      }
      } // switch end

      if (_options.Password_Defined)
      {
        RINOK(_cryptoStream->OutStreamFinish())
      }
    }

    if (_options.Password_Defined)
    {
      if (_options.IsAesMode)
      {
        RINOK(_filterAesSpec->WriteFooter(outStream))
      }
    }
    
    RINOK(outStream->Seek(0, STREAM_SEEK_CUR, &opRes.PackSize))

    {
      opRes.CRC = inCrcStream->GetCRC();
      opRes.UnpackSize = inCrcStream->GetSize();
      opRes.Method = method;
    }

    if (!inCrcStream->WasFinished())
      return E_FAIL;

    if (_options.Password_Defined)
    {
      if (opRes.PackSize < opRes.UnpackSize +
          (_options.IsAesMode ? _filterAesSpec->GetAddPackSize() : NCrypto::NZip::kHeaderSize))
        break;
    }
    else if (opRes.PackSize < opRes.UnpackSize)
      break;
  }

  return S_OK;
}

}}
