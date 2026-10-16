// ZipUpdate.h

#ifndef ZIP7_INC_ZIP_UPDATE_H
#define ZIP7_INC_ZIP_UPDATE_H

#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipCompressionMode.h"
#include "ZipIn.h"

namespace NArchive {
namespace NZip {

/*
struct CUpdateRange
{
  UInt64 Position;
  UInt64 Size;
  
  // CUpdateRange() {}
  CUpdateRange(UInt64 position, UInt64 size): Position(position), Size(size) {}
};
*/

struct CUpdateItem
{
  bool NewData;
  bool NewProps;
  bool IsDir;
  bool Write_NtfsTime;
  bool Write_UnixTime;
  // bool Write_UnixTime_ATime;
  bool IsUtf8;
  bool Size_WasSetFromStream;
  // bool IsAltStream;
  int IndexInArc;
  unsigned IndexInClient;
  UInt32 Attrib;
  UInt32 Time;
  UInt64 Size;
  AString Name;
  CByteBuffer Name_Utf;    // for Info-Zip (kIzUnicodeName) Extra
  CByteBuffer Comment;
  // bool Commented;
  // CUpdateRange CommentRange;
  FILETIME Ntfs_MTime;
  FILETIME Ntfs_ATime;
  FILETIME Ntfs_CTime;

  void Clear()
  {
    IsDir = false;
    
    Write_NtfsTime = false;
    Write_UnixTime = false;

    IsUtf8 = false;
    Size_WasSetFromStream = false;
    // IsAltStream = false;
    Time = 0;
    Size = 0;
    Name.Empty();
    Name_Utf.Free();
    Comment.Free();

    FILETIME_Clear(Ntfs_MTime);
    FILETIME_Clear(Ntfs_ATime);
    FILETIME_Clear(Ntfs_CTime);
  }

  CUpdateItem():
    IsDir(false),
    Write_NtfsTime(false),
    Write_UnixTime(false),
    IsUtf8(false),
    Size_WasSetFromStream(false),
    // IsAltStream(false),
    Time(0),
    Size(0)
    {}
};


struct CUpdateOptions
{
  bool Write_MTime;
  bool Write_ATime;
  bool Write_CTime;
};


HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive, bool removeSfx,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &compressionMethodMode,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
