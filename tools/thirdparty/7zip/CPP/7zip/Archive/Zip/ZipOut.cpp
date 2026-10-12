// ZipOut.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"

#include "../../../Windows/TimeUtils.h"
#include "../../Common/OffsetStream.h"

#include "ZipOut.h"

namespace NArchive {
namespace NZip {

HRESULT COutArchive::ClearRestriction()
{
  if (SetRestriction)
    return SetRestriction->SetRestriction(0, 0);
  return S_OK;
}

HRESULT COutArchive::SetRestrictionFromCurrent()
{
  if (SetRestriction)
    return SetRestriction->SetRestriction(m_Base + m_CurPos, (UInt64)(Int64)-1);
  return S_OK;
}

HRESULT COutArchive::Create(IOutStream *outStream)
{
  m_CurPos = 0;
  if (!m_OutBuffer.Create(1 << 16))
    return E_OUTOFMEMORY;
  m_Stream = outStream;
  m_OutBuffer.SetStream(outStream);
  m_OutBuffer.Init();

  return m_Stream->Seek(0, STREAM_SEEK_CUR, &m_Base);
}

void COutArchive::SeekToCurPos()
{
  HRESULT res = m_Stream->Seek((Int64)(m_Base + m_CurPos), STREAM_SEEK_SET, NULL);
  if (res != S_OK)
    throw CSystemException(res);
}

#define DOES_NEED_ZIP64(v) (v >= (UInt32)0xFFFFFFFF)
// #define DOES_NEED_ZIP64(v) (v >= 0)


Z7_NO_INLINE
void COutArchive::WriteBytes(const void *data, size_t size)
{
  m_OutBuffer.WriteBytes(data, size);
  m_CurPos += size;
}

Z7_NO_INLINE
void COutArchive::Write8(Byte b)
{
  m_OutBuffer.WriteByte(b);
  m_CurPos++;
}

Z7_NO_INLINE
void COutArchive::Write16(UInt16 val)
{
  Write8((Byte)val);
  Write8((Byte)(val >> 8));
}

Z7_NO_INLINE
void COutArchive::Write32(UInt32 val)
{
  for (int i = 0; i < 4; i++)
  {
    // Write8((Byte)val);
    m_OutBuffer.WriteByte((Byte)val);
    val >>= 8;
  }
  m_CurPos += 4;
}

#define WRITE_CONST_PAIR_16_16(a, b)  { Write32((a) | ((UInt32)(b) << 16)); }

Z7_NO_INLINE
void COutArchive::Write64(UInt64 val)
{
  for (int i = 0; i < 8; i++)
  {
    // Write8((Byte)val);
    m_OutBuffer.WriteByte((Byte)val);
    val >>= 8;
  }
  m_CurPos += 8;
}

Z7_NO_INLINE
void COutArchive::WriteExtra(const CExtraBlock &extra)
{
  FOR_VECTOR (i, extra.SubBlocks)
  {
    const CExtraSubBlock &subBlock = extra.SubBlocks[i];
    Write16((UInt16)subBlock.ID);
    Write16((UInt16)subBlock.Data.Size());
    WriteBytes(subBlock.Data, (UInt16)subBlock.Data.Size());
  }
}

void COutArchive::WriteCommonItemInfo(const CLocalItem &item, bool isZip64)
{
  {
    Byte ver = item.ExtractVersion.Version;
    if (isZip64 && ver < NFileHeader::NCompressionMethod::kExtractVersion_Zip64)
      ver = NFileHeader::NCompressionMethod::kExtractVersion_Zip64;
    Write8(ver);
  }
  Write8(item.ExtractVersion.HostOS);
  Write16(item.Flags);
  Write16(item.Method);
  Write32(item.Time);
}


#define WRITE_32_VAL_SPEC(_v_, _isZip64_) Write32((_isZip64_) ? 0xFFFFFFFF : (UInt32)(_v_));


void COutArchive::WriteUtfName(const CItemOut &item)
{
  if (item.Name_Utf.Size() == 0)
    return;
  Write16(NFileHeader::NExtraID::kIzUnicodeName);
  Write16((UInt16)(5 + item.Name_Utf.Size()));
  Write8(1); // (1 = version) of that extra field
  Write32(CrcCalc(item.Name.Ptr(), item.Name.Len()));
  WriteBytes(item.Name_Utf, (UInt16)item.Name_Utf.Size());
}


static const unsigned k_Ntfs_ExtraSize = 4 + 2 + 2 + (3 * 8);
static const unsigned k_UnixTime_ExtraSize = 1 + (1 * 4);

void COutArchive::WriteTimeExtra(const CItemOut &item, bool writeNtfs)
{
  if (writeNtfs)
  {
    // windows explorer ignores that extra
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kNTFS, k_Ntfs_ExtraSize)
    Write32(0); // reserved
    WRITE_CONST_PAIR_16_16(NFileHeader::NNtfsExtra::kTagTime, 8 * 3)
    WriteNtfsTime(item.Ntfs_MTime);
    WriteNtfsTime(item.Ntfs_ATime);
    WriteNtfsTime(item.Ntfs_CTime);
  }

  if (item.Write_UnixTime)
  {
    // windows explorer ignores that extra
    // by specification : should we write to local header also?
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kUnixTime, k_UnixTime_ExtraSize)
    const Byte flags = (Byte)((unsigned)1 << NFileHeader::NUnixTime::kMTime);
    Write8(flags);
    UInt32 unixTime;
    NWindows::NTime::FileTime_To_UnixTime(item.Ntfs_MTime, unixTime);
    Write32(unixTime);
  }
}


void COutArchive::WriteLocalHeader(CItemOut &item, bool needCheck)
{
  m_LocalHeaderPos = m_CurPos;
  item.LocalHeaderPos = m_CurPos;
  
  bool isZip64 =
      DOES_NEED_ZIP64(item.PackSize) ||
      DOES_NEED_ZIP64(item.Size);

  if (needCheck && m_IsZip64)
    isZip64 = true;

  // Why don't we write NTFS timestamps to local header?
  // Probably we want to reduce size of archive?
  const bool writeNtfs = false; // do not write NTFS timestamp to local header
  // const bool writeNtfs = item.Write_NtfsTime; // write NTFS time to local header
  const UInt32 localExtraSize = (UInt32)(
      (isZip64 ? (4 + 8 + 8): 0)
      + (writeNtfs ? 4 + k_Ntfs_ExtraSize : 0)
      + (item.Write_UnixTime ? 4 + k_UnixTime_ExtraSize : 0)
      + item.Get_UtfName_ExtraSize()
      + item.LocalExtra.GetSize());
  if ((UInt16)localExtraSize != localExtraSize)
    throw CSystemException(E_FAIL);
  if (needCheck && m_ExtraSize != localExtraSize)
    throw CSystemException(E_FAIL);

  m_IsZip64 = isZip64;
  m_ExtraSize = localExtraSize;

  item.LocalExtra.IsZip64 = isZip64;

  Write32(NSignature::kLocalFileHeader);
  
  WriteCommonItemInfo(item, isZip64);
  
  Write32(item.HasDescriptor() ? 0 : item.Crc);

  UInt64 packSize = item.PackSize;
  UInt64 size = item.Size;
  
  if (item.HasDescriptor())
  {
    packSize = 0;
    size = 0;
  }
  
  WRITE_32_VAL_SPEC(packSize, isZip64)
  WRITE_32_VAL_SPEC(size, isZip64)

  Write16((UInt16)item.Name.Len());

  Write16((UInt16)localExtraSize);
  
  WriteBytes((const char *)item.Name, (UInt16)item.Name.Len());

  if (isZip64)
  {
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kZip64, 8 + 8)
    Write64(size);
    Write64(packSize);
  }

  WriteTimeExtra(item, writeNtfs);

  WriteUtfName(item);

  WriteExtra(item.LocalExtra);

  const UInt32 localFileHeaderSize = (UInt32)(m_CurPos - m_LocalHeaderPos);
  if (needCheck && m_LocalFileHeaderSize != localFileHeaderSize)
    throw CSystemException(E_FAIL);
  m_LocalFileHeaderSize = localFileHeaderSize;

  m_OutBuffer.FlushWithCheck();
}


void COutArchive::WriteLocalHeader_Replace(CItemOut &item)
{
  m_CurPos = m_LocalHeaderPos + m_LocalFileHeaderSize + item.PackSize;

  if (item.HasDescriptor())
  {
    WriteDescriptor(item);
    m_OutBuffer.FlushWithCheck();
    return;
    // we don't replace local header, if we write Descriptor.
    // so local header with Descriptor flag must be written to local header before.
  }

  const UInt64 nextPos = m_CurPos;
  m_CurPos = m_LocalHeaderPos;
  SeekToCurPos();
  WriteLocalHeader(item, true);
  m_CurPos = nextPos;
  SeekToCurPos();
}


void COutArchive::WriteDescriptor(const CItemOut &item)
{
  Byte buf[kDataDescriptorSize64];
  SetUi32(buf, NSignature::kDataDescriptor)
  SetUi32(buf + 4, item.Crc)
  unsigned descriptorSize;
  if (m_IsZip64)
  {
    SetUi64(buf + 8, item.PackSize)
    SetUi64(buf + 16, item.Size)
    descriptorSize = kDataDescriptorSize64;
  }
  else
  {
    SetUi32(buf + 8, (UInt32)item.PackSize)
    SetUi32(buf + 12, (UInt32)item.Size)
    descriptorSize = kDataDescriptorSize32;
  }
  WriteBytes(buf, descriptorSize);
}



void COutArchive::WriteCentralHeader(const CItemOut &item)
{
  const bool isUnPack64 = DOES_NEED_ZIP64(item.Size);
  const bool isPack64 = DOES_NEED_ZIP64(item.PackSize);
  const bool isPosition64 = DOES_NEED_ZIP64(item.LocalHeaderPos);
  const bool isZip64 = isPack64 || isUnPack64 || isPosition64;
  
  Write32(NSignature::kCentralFileHeader);
  Write8(item.MadeByVersion.Version);
  Write8(item.MadeByVersion.HostOS);
  
  WriteCommonItemInfo(item, isZip64);
  Write32(item.Crc);

  WRITE_32_VAL_SPEC(item.PackSize, isPack64)
  WRITE_32_VAL_SPEC(item.Size, isUnPack64)

  Write16((UInt16)item.Name.Len());
  
  const UInt16 zip64ExtraSize = (UInt16)((isUnPack64 ? 8: 0) + (isPack64 ? 8: 0) + (isPosition64 ? 8: 0));
  const bool writeNtfs = item.Write_NtfsTime;
  const size_t centralExtraSize =
      (isZip64 ? 4 + zip64ExtraSize : 0)
      + (writeNtfs ? 4 + k_Ntfs_ExtraSize : 0)
      + (item.Write_UnixTime ? 4 + k_UnixTime_ExtraSize : 0)
      + item.Get_UtfName_ExtraSize()
      + item.CentralExtra.GetSize();

  const UInt16 centralExtraSize16 = (UInt16)centralExtraSize;
  if (centralExtraSize16 != centralExtraSize)
    throw CSystemException(E_FAIL);

  Write16(centralExtraSize16);

  const UInt16 commentSize = (UInt16)item.Comment.Size();
  
  Write16(commentSize);
  Write16(0); // DiskNumberStart
  Write16(item.InternalAttrib);
  Write32(item.ExternalAttrib);
  WRITE_32_VAL_SPEC(item.LocalHeaderPos, isPosition64)
  WriteBytes((const char *)item.Name, item.Name.Len());
  
  if (isZip64)
  {
    Write16(NFileHeader::NExtraID::kZip64);
    Write16(zip64ExtraSize);
    if (isUnPack64)
      Write64(item.Size);
    if (isPack64)
      Write64(item.PackSize);
    if (isPosition64)
      Write64(item.LocalHeaderPos);
  }
  
  WriteTimeExtra(item, writeNtfs);
  WriteUtfName(item);
  
  WriteExtra(item.CentralExtra);
  if (commentSize != 0)
    WriteBytes(item.Comment, commentSize);
}

HRESULT COutArchive::WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer *comment)
{
  RINOK(ClearRestriction())
  
  const UInt64 cdOffset = GetCurPos();
  FOR_VECTOR (i, items)
    WriteCentralHeader(items[i]);
  const UInt64 cd64EndOffset = GetCurPos();
  const UInt64 cdSize = cd64EndOffset - cdOffset;
  const bool cdOffset64 = DOES_NEED_ZIP64(cdOffset);
  const bool cdSize64 = DOES_NEED_ZIP64(cdSize);
  const bool need_Items_64 = items.Size() >= 0xFFFF;
  const unsigned items16 = (UInt16)(need_Items_64 ? 0xFFFF: items.Size());
  const bool isZip64 = (cdOffset64 || cdSize64 || need_Items_64);
  
  // isZip64 = true; // to test Zip64

  if (isZip64)
  {
    Write32(NSignature::kEcd64);
    Write64(kEcd64_MainSize);
    
    // to test extra block:
    // const UInt32 extraSize = 1 << 26;
    // Write64(kEcd64_MainSize + extraSize);

    WRITE_CONST_PAIR_16_16(45, // made by version
        45) // extract version
    Write32(0); // ThisDiskNumber
    Write32(0); // StartCentralDirectoryDiskNumber
    Write64((UInt64)items.Size());
    Write64((UInt64)items.Size());
    Write64((UInt64)cdSize);
    Write64((UInt64)cdOffset);

    // for (UInt32 iii = 0; iii < extraSize; iii++) Write8(1);

    Write32(NSignature::kEcd64Locator);
    Write32(0); // number of the disk with the start of the zip64 end of central directory
    Write64(cd64EndOffset);
    Write32(1); // total number of disks
  }
  
  Write32(NSignature::kEcd);
  WRITE_CONST_PAIR_16_16(0, 0)  // ThisDiskNumber, StartCentralDirectoryDiskNumber
  Write16((UInt16)items16);
  Write16((UInt16)items16);
  
  WRITE_32_VAL_SPEC(cdSize, cdSize64)
  WRITE_32_VAL_SPEC(cdOffset, cdOffset64)

  const UInt16 commentSize = (UInt16)(comment ? comment->Size() : 0);
  Write16((UInt16)commentSize);
  if (commentSize != 0)
    WriteBytes((const Byte *)*comment, commentSize);
  m_OutBuffer.FlushWithCheck();
  return S_OK;
}

void COutArchive::CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream)
{
  COffsetOutStream *streamSpec = new COffsetOutStream;
  outStream = streamSpec;
  streamSpec->Init(m_Stream, m_Base + m_CurPos);
}

void COutArchive::CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream)
{
  outStream = m_Stream;
}

}}
