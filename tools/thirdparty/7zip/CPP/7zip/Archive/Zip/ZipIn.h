// Archive/ZipIn.h

#ifndef ZIP7_INC_ZIP_IN_H
#define ZIP7_INC_ZIP_IN_H

#include "../../../Common/MyBuffer2.h"
#include "../../../Common/MyCom.h"

#include "../../Common/StreamUtils.h"
#include "../../IStream.h"

#include "ZipHeader.h"
#include "ZipItem.h"

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size);

namespace NArchive {
namespace NZip {
  
class CItemEx: public CItem
{
public:
  UInt32 LocalFullHeaderSize; // including Name and Extra
  // int ParentOfAltStream; // -1, if not AltStream
  
  bool DescriptorWasRead;

  CItemEx():
    // ParentOfAltStream(-1),
    DescriptorWasRead(false) {}

  UInt64 GetLocalFullSize() const
    { return LocalFullHeaderSize + GetPackSizeWithDescriptor(); }
  UInt64 GetDataPosition() const
    { return LocalHeaderPos + LocalFullHeaderSize; }

  bool IsBadDescriptor() const
  {
    return !FromCentral && FromLocal && HasDescriptor() && !DescriptorWasRead;
  }
};


struct CInArchiveInfo
{
  Int64 Base; /* Base offset of start of archive in stream.
                 Offsets in headers must be calculated from that Base.
                 Base is equal to MarkerPos for normal ZIPs.
                 Base can point to PE stub for some ZIP SFXs.
                 if CentralDir was read,
                   Base can be negative, if start of data is not available,
                 if CentralDirs was not read,
                   Base = ArcInfo.MarkerPos; */

  /* The following *Pos variables contain absolute offsets in Stream */

  UInt64 MarkerPos;  /* Pos of first signature, it can point to kSpan/kNoSpan signature
                        = MarkerPos2      in most archives
                        = MarkerPos2 - 4  if there is kSpan/kNoSpan signature */
  UInt64 MarkerPos2; // Pos of first local item signature in stream
  UInt64 FinishPos;  // Finish pos of archive data in starting volume
  UInt64 FileEndPos; // Finish pos of stream

  UInt64 FirstItemRelatOffset; /* Relative offset of first local (read from cd) (relative to Base).
                                  = 0 in most archives
                                  = size of stub for some SFXs */


  int MarkerVolIndex;

  bool CdWasRead;
  bool IsSpanMode;
  bool ThereIsTail;

  // UInt32 BaseVolIndex;

  CByteBuffer Comment;


  CInArchiveInfo():
      Base(0),
      MarkerPos(0),
      MarkerPos2(0),
      FinishPos(0),
      FileEndPos(0),
      FirstItemRelatOffset(0),
      MarkerVolIndex(-1),
      CdWasRead(false),
      IsSpanMode(false),
      ThereIsTail(false)
      // BaseVolIndex(0)
      {}
  
  void Clear()
  {
    // BaseVolIndex = 0;
    Base = 0;
    MarkerPos = 0;
    MarkerPos2 = 0;
    FinishPos = 0;
    FileEndPos = 0;
    MarkerVolIndex = -1;
    ThereIsTail = false;

    FirstItemRelatOffset = 0;

    CdWasRead = false;
    IsSpanMode = false;

    Comment.Free();
  }
};


struct CCdInfo
{
  bool IsFromEcd64;
  
  UInt16 CommentSize;
  
  // 64
  UInt16 VersionMade;
  UInt16 VersionNeedExtract;

  // old zip
  UInt32 ThisDisk;
  UInt32 CdDisk;
  UInt64 NumEntries_in_ThisDisk;
  UInt64 NumEntries;
  UInt64 Size;
  UInt64 Offset;

  CCdInfo() { memset(this, 0, sizeof(*this)); IsFromEcd64 = false; }

  void ParseEcd32(const Byte *p);   // (p) includes signature
  void ParseEcd64e(const Byte *p);  // (p) exclude signature

  bool IsEmptyArc() const
  {
    return ThisDisk == 0
        && CdDisk == 0
        && NumEntries_in_ThisDisk == 0
        && NumEntries == 0
        && Size == 0
        && Offset == 0 // test it
    ;
  }
};


struct CVols
{
  struct CSubStreamInfo
  {
    CMyComPtr<IInStream> Stream;
    UInt64 Size;

    HRESULT SeekToStart() const { return InStream_SeekToBegin(Stream); }

    CSubStreamInfo(): Size(0) {}
  };
  
  CObjectVector<CSubStreamInfo> Streams;
  
  int StreamIndex;   // -1 for StartStream
                     // -2 for ZipStream at multivol detection code
                     // >=0 volume index in multivol
  
  bool NeedSeek;

  bool DisableVolsSearch;
  bool StartIsExe;  // is .exe
  bool StartIsZ;    // is .zip or .zNN
  bool StartIsZip;  // is .zip
  bool IsUpperCase;
  bool MissingZip;

  bool ecd_wasRead;

  Int32 StartVolIndex; // -1, if unknown vol index
                       // = (NN - 1), if StartStream is .zNN
                       // = 0, if start vol is exe

  Int32 StartParsingVol; // if we need local parsing, we must use that stream
  unsigned NumVols;

  int EndVolIndex; // index of last volume (ecd volume),
                   // -1, if is not multivol

  UString BaseName; // name of archive including '.'
  UString MissingName;

  CMyComPtr<IInStream> ZipStream;

  CCdInfo ecd;

  UInt64 TotalBytesSize; // for MultiVol only

  void ClearRefs()
  {
    Streams.Clear();
    ZipStream.Release();
    TotalBytesSize = 0;
  }

  void Clear()
  {
    StreamIndex = -1;
    NeedSeek = false;

    DisableVolsSearch = false;
    StartIsExe = false;
    StartIsZ = false;
    StartIsZip = false;
    IsUpperCase = false;

    StartVolIndex = -1;
    StartParsingVol = 0;
    NumVols = 0;
    EndVolIndex = -1;

    BaseName.Empty();
    MissingName.Empty();

    MissingZip = false;
    ecd_wasRead = false;

    ClearRefs();
  }

  HRESULT ParseArcName(IArchiveOpenVolumeCallback *volCallback);

  HRESULT Read(void *data, UInt32 size, UInt32 *processedSize);
};


Z7_CLASS_IMP_COM_1(
  CVolStream
  , ISequentialInStream
)
public:
  CVols *Vols;
};


class CInArchive
{
  CMidBuffer Buffer;
  size_t _bufPos;
  size_t _bufCached;

  UInt64 _streamPos;
  UInt64 _cnt;

  // UInt32 _startLocalFromCd_Disk;
  // UInt64 _startLocalFromCd_Offset;

  size_t GetAvail() const { return _bufCached - _bufPos; }

  void InitBuf() { _bufPos = 0; _bufCached = 0; }
  void DisableBufMode() { InitBuf(); _inBufMode = false; }

  void SkipLookahed(size_t skip)
  {
    _bufPos += skip;
    _cnt += skip;
  }

  HRESULT AllocateBuffer(size_t size);

  UInt64 GetVirtStreamPos() { return _streamPos - _bufCached + _bufPos; }

  bool _inBufMode;

  bool IsArcOpen;
  bool CanStartNewVol;

  UInt32 _signature;

  CMyComPtr<IInStream> StreamRef;
  IInStream *Stream;
  IInStream *StartStream;
  IArchiveOpenCallback *Callback;

  HRESULT Seek_SavePos(UInt64 offset);
  HRESULT SeekToVol(int volIndex, UInt64 offset);

  HRESULT ReadFromCache(Byte *data, unsigned size, unsigned &processed);
  HRESULT ReadFromCache_FALSE(Byte *data, unsigned size);

  HRESULT ReadVols2(IArchiveOpenVolumeCallback *volCallback,
      unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols);
  HRESULT ReadVols();

  HRESULT FindMarker(const UInt64 *searchLimit);
  HRESULT IncreaseRealPosition(UInt64 addValue, bool &isFinished);

  HRESULT LookAhead(size_t minRequiredInBuffer);
  void SafeRead(Byte *data, unsigned size);
  void ReadBuffer(CByteBuffer &buffer, unsigned size);
  // Byte ReadByte();
  // UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();

  void ReadSignature();

  void Skip(size_t num);
  HRESULT Skip64(UInt64 num, unsigned numFiles);

  bool ReadFileName(unsigned nameSize, AString &dest);

  bool ReadExtra(const CLocalItem &item, unsigned extraSize, CExtraBlock &extra,
      UInt64 &unpackSize, UInt64 &packSize, CItem *cdItem);
  bool ReadLocalItem(CItemEx &item);
  HRESULT FindDescriptor(CItemEx &item, unsigned numFiles);
  HRESULT ReadCdItem(CItemEx &item);
  HRESULT TryEcd64(UInt64 offset, CCdInfo &cdInfo);
  HRESULT FindCd(bool checkOffsetMode);
  HRESULT TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, UInt64 cdOffset, UInt64 cdSize);
  HRESULT ReadCd(CObjectVector<CItemEx> &items, UInt32 &cdDisk, UInt64 &cdOffset, UInt64 &cdSize);
  HRESULT ReadLocals(CObjectVector<CItemEx> &localItems);

  HRESULT ReadHeaders(CObjectVector<CItemEx> &items);

  HRESULT GetVolStream(unsigned vol, UInt64 pos, CMyComPtr<ISequentialInStream> &stream);

public:
  CInArchiveInfo ArcInfo;
  
  bool IsArc;
  bool IsZip64;

  bool IsApk;
  bool IsCdUnsorted;
  
  bool HeadersError;
  bool HeadersWarning;
  bool ExtraMinorError;
  bool UnexpectedEnd;
  bool LocalsWereRead;
  bool LocalsCenterMerged;
  bool NoCentralDir;
  bool Overflow32bit; // = true, if zip without Zip64 extension support and it has some fields values truncated to 32-bits.
  bool Cd_NumEntries_Overflow_16bit; // = true, if no Zip64 and 16-bit ecd:NumEntries was overflowed.

  bool MarkerIsFound;
  bool MarkerIsSafe;

  bool IsMultiVol;
  bool UseDisk_in_SingleVol;
  UInt32 EcdVolIndex;

  CVols Vols;

  bool Force_ReadLocals_Mode;
  bool Disable_VolsRead;
  bool Disable_FindMarker;
 
  CInArchive():
      IsArcOpen(false),
      Stream(NULL),
      StartStream(NULL),
      Callback(NULL),
      Force_ReadLocals_Mode(false),
      Disable_VolsRead(false),
      Disable_FindMarker(false)
      {}

  UInt64 GetPhySize() const
  {
    if (IsMultiVol)
      return ArcInfo.FinishPos;
    else
      return (UInt64)((Int64)ArcInfo.FinishPos - ArcInfo.Base);
  }

  UInt64 GetOffset() const
  {
    if (IsMultiVol)
      return 0;
    else
      return (UInt64)ArcInfo.Base;
  }

  
  void ClearRefs();
  void Close();
  HRESULT Open(IInStream *stream, const UInt64 *searchLimit, IArchiveOpenCallback *callback, CObjectVector<CItemEx> &items);

  bool IsOpen() const { return IsArcOpen; }
  
  bool AreThereErrors() const
  {
    return HeadersError
        || UnexpectedEnd
        || !Vols.MissingName.IsEmpty();
  }

  bool IsLocalOffsetOK(const CItemEx &item) const
  {
    if (item.FromLocal)
      return true;
    return (Int64)GetOffset() + (Int64)item.LocalHeaderPos >= 0;
  }

  UInt64 GetEmbeddedStubSize() const
  {
    // it's possible that first item in CD doesn refers to first local item
    // so FirstItemRelatOffset is not first local item

    if (ArcInfo.CdWasRead)
      return ArcInfo.FirstItemRelatOffset;
    if (IsMultiVol)
      return 0;
    return (UInt64)((Int64)ArcInfo.MarkerPos2 - ArcInfo.Base);
  }


  HRESULT CheckDescriptor(const CItemEx &item);
  HRESULT Read_LocalItem_After_CdItem(CItemEx &item, bool &isAvail, bool &headersError);
  HRESULT Read_LocalItem_After_CdItem_Full(CItemEx &item);

  HRESULT GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr<ISequentialInStream> &stream);

  IInStream *GetBaseStream() { return StreamRef; }

  bool CanUpdate() const
  {
    if (AreThereErrors()
       || IsMultiVol
       || ArcInfo.Base < 0
       || (Int64)ArcInfo.MarkerPos2 < ArcInfo.Base
       || ArcInfo.ThereIsTail
       || GetEmbeddedStubSize() != 0
       || IsApk
       || IsCdUnsorted)
      return false;
   
    // 7-zip probably can update archives with embedded stubs.
    // we just disable that feature for more safety.

    return true;
  }
};
  
}}
  
#endif
