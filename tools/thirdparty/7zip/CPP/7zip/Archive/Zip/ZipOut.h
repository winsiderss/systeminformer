// ZipOut.h

#ifndef ZIP7_INC_ZIP_OUT_H
#define ZIP7_INC_ZIP_OUT_H

#include "../../../Common/MyCom.h"

#include "../../Common/OutBuffer.h"

#include "ZipItem.h"

namespace NArchive {
namespace NZip {

class CItemOut: public CItem
{
public:
  FILETIME Ntfs_MTime;
  FILETIME Ntfs_ATime;
  FILETIME Ntfs_CTime;
  bool Write_NtfsTime;
  bool Write_UnixTime;

  // It's possible that NtfsTime is not defined, but there is NtfsTime in Extra.
  
  CByteBuffer Name_Utf; // for Info-Zip (kIzUnicodeName) Extra

  size_t Get_UtfName_ExtraSize() const
  {
    const size_t size = Name_Utf.Size();
    if (size == 0)
      return 0;
    return 4 + 5 + size;
  }

  CItemOut():
      Write_NtfsTime(false),
      Write_UnixTime(false)
      {}
};


// COutArchive can throw CSystemException and COutBufferException

class COutArchive
{
  COutBuffer m_OutBuffer;
  CMyComPtr<IOutStream> m_Stream;

  UInt64 m_Base; // Base of archive (offset in output Stream)
  UInt64 m_CurPos; // Curent position in archive (relative from m_Base)
  UInt64 m_LocalHeaderPos; // LocalHeaderPos (relative from m_Base) for last WriteLocalHeader() call

  UInt32 m_LocalFileHeaderSize;
  UInt32 m_ExtraSize;
  bool m_IsZip64;

  void WriteBytes(const void *data, size_t size);
  void Write8(Byte b);
  void Write16(UInt16 val);
  void Write32(UInt32 val);
  void Write64(UInt64 val);
  void WriteNtfsTime(const FILETIME &ft)
  {
    Write32(ft.dwLowDateTime);
    Write32(ft.dwHighDateTime);
  }

  void WriteTimeExtra(const CItemOut &item, bool writeNtfs);
  void WriteUtfName(const CItemOut &item);
  void WriteExtra(const CExtraBlock &extra);
  void WriteCommonItemInfo(const CLocalItem &item, bool isZip64);
  void WriteCentralHeader(const CItemOut &item);

  void SeekToCurPos();
public:
  CMyComPtr<IStreamSetRestriction> SetRestriction;

  HRESULT ClearRestriction();
  HRESULT SetRestrictionFromCurrent();
  HRESULT Create(IOutStream *outStream);
  
  UInt64 GetCurPos() const { return m_CurPos; }

  void MoveCurPos(UInt64 distanceToMove)
  {
    m_CurPos += distanceToMove;
  }

  void WriteLocalHeader(CItemOut &item, bool needCheck = false);
  void WriteLocalHeader_Replace(CItemOut &item);

  void WriteDescriptor(const CItemOut &item);

  HRESULT WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer *comment);

  void CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream);
  void CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream);
};

}}

#endif
