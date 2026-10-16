// Archive/ZipIn.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../Common/DynamicBuffer.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StringToInt.h"

#include "../../../Windows/PropVariant.h"

#include "../IArchive.h"

#include "ZipIn.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(offs, v) v = Get16(p + (offs))
#define G32(offs, v) v = Get32(p + (offs))
#define G64(offs, v) v = Get64(p + (offs))

namespace NArchive {
namespace NZip {

/* we try to use same size of Buffer (1 << 17) for all tasks.
   it allow to avoid reallocations and cache clearing. */

static const size_t kSeqBufferSize = (size_t)1 << 17;

/*
Open()
{
  _inBufMode = false;
  ReadVols()
    FindCd();
      TryEcd64()
  SeekToVol()
  FindMarker()
    _inBufMode = true;
  ReadHeaders()
    _inBufMode = false;
    ReadCd()
      FindCd()
        TryEcd64()
      TryReadCd()
      {
        SeekToVol();
        _inBufMode = true;
      }
    _inBufMode = true;
    ReadLocals()
    ReadCdItem()
    ....
}
FindCd() writes to Buffer without touching (_inBufMode)
*/

/*
  if (not defined ZIP_SELF_CHECK) : it reads CD and if error in first pass CD reading, it reads LOCALS-CD-MODE
  if (    defined ZIP_SELF_CHECK) : it always reads CD and LOCALS-CD-MODE
  use ZIP_SELF_CHECK to check LOCALS-CD-MODE for any zip archive
*/

// #define ZIP_SELF_CHECK


struct CEcd
{
  UInt16 ThisDisk;
  UInt16 CdDisk;
  UInt16 NumEntries_in_ThisDisk;
  UInt16 NumEntries;
  UInt32 Size;
  UInt32 Offset;
  UInt16 CommentSize;
  
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

  void Parse(const Byte *p); // (p) doesn't include signature
};

void CEcd::Parse(const Byte *p)
{
  // (p) doesn't include signature
  G16(0, ThisDisk);
  G16(2, CdDisk);
  G16(4, NumEntries_in_ThisDisk);
  G16(6, NumEntries);
  G32(8, Size);
  G32(12, Offset);
  G16(16, CommentSize);
}


void CCdInfo::ParseEcd32(const Byte *p)
{
  IsFromEcd64 = false;
  // (p) includes signature
  p += 4;
  G16(0, ThisDisk);
  G16(2, CdDisk);
  G16(4, NumEntries_in_ThisDisk);
  G16(6, NumEntries);
  G32(8, Size);
  G32(12, Offset);
  G16(16, CommentSize);
}

void CCdInfo::ParseEcd64e(const Byte *p)
{
  IsFromEcd64 = true;
  // (p) exclude signature
  G16(0, VersionMade);
  G16(2, VersionNeedExtract);
  G32(4, ThisDisk);
  G32(8, CdDisk);

  G64(12, NumEntries_in_ThisDisk);
  G64(20, NumEntries);
  G64(28, Size);
  G64(36, Offset);
}


struct CLocator
{
  UInt32 Ecd64Disk;
  UInt32 NumDisks;
  UInt64 Ecd64Offset;
  
  CLocator(): Ecd64Disk(0), NumDisks(0), Ecd64Offset(0) {}

  void Parse(const Byte *p)
  {
    G32(0, Ecd64Disk);
    G64(4, Ecd64Offset);
    G32(12, NumDisks);
  }

  bool IsEmptyArc() const
  {
    return Ecd64Disk == 0 && NumDisks == 0 && Ecd64Offset == 0;
  }
};

  


void CInArchive::ClearRefs()
{
  StreamRef.Release();
  Stream = NULL;
  StartStream = NULL;
  Callback = NULL;

  Vols.Clear();
}

void CInArchive::Close()
{
  _cnt = 0;
  DisableBufMode();

  IsArcOpen = false;

  IsArc = false;
  IsZip64 = false;
  
  IsApk = false;
  IsCdUnsorted = false;

  HeadersError = false;
  HeadersWarning = false;
  ExtraMinorError = false;
  
  UnexpectedEnd = false;
  LocalsWereRead = false;
  LocalsCenterMerged = false;
  NoCentralDir = false;
  Overflow32bit = false;
  Cd_NumEntries_Overflow_16bit = false;
  
  MarkerIsFound = false;
  MarkerIsSafe = false;

  IsMultiVol = false;
  UseDisk_in_SingleVol = false;
  EcdVolIndex = 0;
 
  ArcInfo.Clear();

  ClearRefs();
}



HRESULT CInArchive::Seek_SavePos(UInt64 offset)
{
  // InitBuf();
  // if (!Stream) return S_FALSE;
  return Stream->Seek((Int64)offset, STREAM_SEEK_SET, &_streamPos);
}


/* SeekToVol() will keep the cached mode, if new volIndex is
   same Vols.StreamIndex volume, and offset doesn't go out of cached region */

HRESULT CInArchive::SeekToVol(int volIndex, UInt64 offset)
{
  if (volIndex != Vols.StreamIndex)
  {
    if (IsMultiVol && volIndex >= 0)
    {
      if ((unsigned)volIndex >= Vols.Streams.Size())
        return S_FALSE;
      if (!Vols.Streams[(unsigned)volIndex].Stream)
        return S_FALSE;
      Stream = Vols.Streams[(unsigned)volIndex].Stream;
    }
    else if (volIndex == -2)
    {
      if (!Vols.ZipStream)
        return S_FALSE;
      Stream = Vols.ZipStream;
    }
    else
      Stream = StartStream;
    Vols.StreamIndex = volIndex;
  }
  else
  {
    if (offset <= _streamPos)
    {
      const UInt64 back = _streamPos - offset;
      if (back <= _bufCached)
      {
        _bufPos = _bufCached - (size_t)back;
        return S_OK;
      }
    }
  }
  InitBuf();
  return Seek_SavePos(offset);
}


HRESULT CInArchive::AllocateBuffer(size_t size)
{
  if (size <= Buffer.Size())
    return S_OK;
  /* in cached mode virtual_pos is not equal to phy_pos (_streamPos)
     so we change _streamPos and do Seek() to virtual_pos before cache clearing */
  if (_bufPos != _bufCached)
  {
    RINOK(Seek_SavePos(GetVirtStreamPos()))
  }
  InitBuf();
  Buffer.AllocAtLeast(size);
  if (!Buffer.IsAllocated())
    return E_OUTOFMEMORY;
  return S_OK;
}

// ---------- ReadFromCache ----------
// reads from cache and from Stream
// move to next volume can be allowed if (CanStartNewVol) and only before first byte reading

HRESULT CInArchive::ReadFromCache(Byte *data, unsigned size, unsigned &processed)
{
  HRESULT result = S_OK;
  processed = 0;

  for (;;)
  {
    if (size == 0)
      return S_OK;
    
    const size_t avail = GetAvail();

    if (avail != 0)
    {
      unsigned cur = size;
      if (cur > avail)
        cur = (unsigned)avail;
      memcpy(data, (const Byte *)Buffer + _bufPos, cur);

      data += cur;
      size -= cur;
      processed += cur;

      _bufPos += cur;
      _cnt += cur;

      CanStartNewVol = false;
      
      continue;
    }

    InitBuf();

    if (_inBufMode)
    {
      UInt32 cur = 0;
      result = Stream->Read(Buffer, (UInt32)Buffer.Size(), &cur);
      _bufPos = 0;
      _bufCached = cur;
      _streamPos += cur;
      if (cur != 0)
        CanStartNewVol = false;
      if (result != S_OK)
        break;
      if (cur != 0)
        continue;
    }
    else
    {
      size_t cur = size;
      result = ReadStream(Stream, data, &cur);
      data += cur;
      size -= (unsigned)cur;
      processed += (unsigned)cur;
      _streamPos += cur;
      _cnt += cur;
      if (cur != 0)
      {
        CanStartNewVol = false;
        break;
      }
      if (result != S_OK)
        break;
    }

    if (   !IsMultiVol
        || !CanStartNewVol
        || Vols.StreamIndex < 0
        || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
      break;

    const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
    if (!s.Stream)
      break;
    result = s.SeekToStart();
    if (result != S_OK)
      break;
    Vols.StreamIndex++;
    _streamPos = 0;
    // Vols.NeedSeek = false;

    Stream = s.Stream;
  }

  return result;
}


HRESULT CInArchive::ReadFromCache_FALSE(Byte *data, unsigned size)
{
  unsigned processed;
  HRESULT res = ReadFromCache(data, size, processed);
  if (res == S_OK && size != processed)
    return S_FALSE;
  return res;
}


static bool CheckDosTime(UInt32 dosTime)
{
  if (dosTime == 0)
    return true;
  unsigned month = (dosTime >> 21) & 0xF;
  unsigned day = (dosTime >> 16) & 0x1F;
  unsigned hour = (dosTime >> 11) & 0x1F;
  unsigned min = (dosTime >> 5) & 0x3F;
  unsigned sec = (dosTime & 0x1F) * 2;
  if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  return true;
}

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size)
{
  if (size < 8)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'P')
    return k_IsArc_Res_NO;

  UInt32 sig = Get32(p);

  if (sig == NSignature::kNoSpan || sig == NSignature::kSpan)
  {
    p += 4;
    size -= 4;
  }

  sig = Get32(p);

  if (sig == NSignature::kEcd64)
  {
    if (size < kEcd64_FullSize)
      return k_IsArc_Res_NEED_MORE;

    const UInt64 recordSize = Get64(p + 4);
    if (   recordSize < kEcd64_MainSize
        || recordSize > kEcd64_MainSize + (1 << 20))
      return k_IsArc_Res_NO;
    CCdInfo cdInfo;
    cdInfo.ParseEcd64e(p + 12);
    if (!cdInfo.IsEmptyArc())
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
  }

  if (sig == NSignature::kEcd)
  {
    if (size < kEcdSize)
      return k_IsArc_Res_NEED_MORE;
    CEcd ecd;
    ecd.Parse(p + 4);
    // if (ecd.cdSize != 0)
    if (!ecd.IsEmptyArc())
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
  }
 
  if (sig != NSignature::kLocalFileHeader)
    return k_IsArc_Res_NO;

  if (size < kLocalHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  
  p += 4;

  {
    const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return k_IsArc_Res_NEED_MORE;
  }

  /*
  if (p[0] >= 128) // ExtractVersion.Version;
    return k_IsArc_Res_NO;
  */

  // ExtractVersion.Version = p[0];
  // ExtractVersion.HostOS = p[1];
  // Flags = Get16(p + 2);
  // Method = Get16(p + 4);
  /*
  // 9.33: some zip archives contain incorrect value in timestamp. So we don't check it now
  UInt32 dosTime = Get32(p + 6);
  if (!CheckDosTime(dosTime))
    return k_IsArc_Res_NO;
  */
  // Crc = Get32(p + 10);
  // PackSize = Get32(p + 14);
  // Size = Get32(p + 18);
  const unsigned nameSize = Get16(p + 22);
  unsigned extraSize = Get16(p + 24);
  const UInt32 extraOffset = kLocalHeaderSize + (UInt32)nameSize;
  
  /*
  // 21.02: fixed. we don't use the following check
  if (extraOffset + extraSize > (1 << 16))
    return k_IsArc_Res_NO;
  */

  p -= 4;

  {
    size_t rem = size - kLocalHeaderSize;
    if (rem > nameSize)
      rem = nameSize;
    const Byte *p2 = p + kLocalHeaderSize;
    for (size_t i = 0; i < rem; i++)
      if (p2[i] == 0)
      {
        // we support some "bad" zip archives that contain zeros after name
        for (size_t k = i + 1; k < rem; k++)
          if (p2[k] != 0)
            return k_IsArc_Res_NO;
        break;
        /*
        if (i != nameSize - 1)
          return k_IsArc_Res_NO;
        */
      }
  }

  if (size < extraOffset)
    return k_IsArc_Res_NEED_MORE;

  if (extraSize > 0)
  {
    p += extraOffset;
    size -= extraOffset;
    while (extraSize != 0)
    {
      if (extraSize < 4)
      {
        // 7-Zip before 9.31 created incorrect WzAES Extra in folder's local headers.
        // so we return k_IsArc_Res_YES to support such archives.
        // return k_IsArc_Res_NO; // do we need to support such extra ?
        return k_IsArc_Res_YES;
      }
      if (size < 4)
        return k_IsArc_Res_NEED_MORE;
      unsigned dataSize = Get16(p + 2);
      size -= 4;
      extraSize -= 4;
      p += 4;
      if (dataSize > extraSize)
      {
        // It can be error on header.
        // We want to support such rare case bad archives.
        // We use additional checks to reduce false-positive probability.
        if (nameSize == 0
            || nameSize > (1 << 9)
            || extraSize > (1 << 9))
          return k_IsArc_Res_NO;
        return k_IsArc_Res_YES;
      }
      if (dataSize > size)
        return k_IsArc_Res_NEED_MORE;
      size -= dataSize;
      extraSize -= dataSize;
      p += dataSize;
    }
  }
  
  return k_IsArc_Res_YES;
}

static UInt32 IsArc_Zip_2(const Byte *p, size_t size, bool isFinal)
{
  UInt32 res = IsArc_Zip(p, size);
  if (res == k_IsArc_Res_NEED_MORE && isFinal)
    return k_IsArc_Res_NO;
  return res;
}


  
/* FindPK_4() is allowed to access data up to and including &limit[3].
   limit[4] access is not allowed.
  return:
    (return_ptr <  limit) : "PK" was found at (return_ptr)
    (return_ptr >= limit) : limit was reached or crossed. So no "PK" found before limit
*/
Z7_NO_INLINE
static const Byte *FindPK_4(const Byte *p, const Byte *limit)
{
  for (;;)
  {
    for (;;)
    {
      if (p >= limit)
        return limit;
      Byte b = p[1];
      if (b == 0x4B) { if (p[0] == 0x50) { return p;     } p += 1; break; }
      if (b == 0x50) { if (p[2] == 0x4B) { return p + 1; } p += 2; break; }
      b = p[3];
      p += 4;
      if (b == 0x4B) { if (p[-2]== 0x50) { return p - 2; } p -= 1; break; }
      if (b == 0x50) { if (p[0] == 0x4B) { return p - 1; }         break; }
    }
  }
  /*
  for (;;)
  {
    for (;;)
    {
      if (p >= limit)
        return limit;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
    }
    if (*p == 0x4B)
      return p - 1;
  }
  */
}


/*
---------- FindMarker ----------
returns:
  S_OK:
    ArcInfo.MarkerVolIndex : volume of marker
    ArcInfo.MarkerPos   : Pos of first signature
    ArcInfo.MarkerPos2  : Pos of main signature (local item signature in most cases)
    _streamPos          : stream pos
    _cnt                : The number of virtal Bytes after start of search to offset after signature
    _signature          : main signature
 
  S_FALSE: can't find marker, or there is some non-zip data after marker

  Error code: stream reading error.
*/

HRESULT CInArchive::FindMarker(const UInt64 *searchLimit)
{
  ArcInfo.MarkerPos = GetVirtStreamPos();
  ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
  ArcInfo.MarkerVolIndex = Vols.StreamIndex;

  _cnt = 0;

  CanStartNewVol = false;

  if (searchLimit && *searchLimit == 0)
  {
    Byte startBuf[kMarkerSize];
    RINOK(ReadFromCache_FALSE(startBuf, kMarkerSize))

    UInt32 marker = Get32(startBuf);
    _signature = marker;

    if (   marker == NSignature::kNoSpan
        || marker == NSignature::kSpan)
    {
      RINOK(ReadFromCache_FALSE(startBuf, kMarkerSize))
      _signature = Get32(startBuf);
    }
      
    if (   _signature != NSignature::kEcd
        && _signature != NSignature::kEcd64
        && _signature != NSignature::kLocalFileHeader)
      return S_FALSE;

    ArcInfo.MarkerPos2 = GetVirtStreamPos() - 4;
    ArcInfo.IsSpanMode = (marker == NSignature::kSpan);

    // we use weak test in case of (*searchLimit == 0)
    // since error will be detected later in Open function
    return S_OK;
  }

  // zip specification: (_zip_header_size < (1 << 16))
  // so we need such size to check header
  const size_t kCheckSize = (size_t)1 << 16;
  const size_t kBufSize   = (size_t)1 << 17; // (kBufSize must be > kCheckSize)

  RINOK(AllocateBuffer(kBufSize))

  _inBufMode = true;

  UInt64 progressPrev = 0;

  for (;;)
  {
    RINOK(LookAhead(kBufSize))
    
    const size_t avail = GetAvail();
    
    size_t limitPos;
    // (avail > kBufSize) is possible, if (Buffer.Size() > kBufSize)
    const bool isFinished = (avail < kBufSize);
    if (isFinished)
    {
      const unsigned kMinAllowed = 4;
      if (avail <= kMinAllowed)
      {
        if (   !IsMultiVol
            || Vols.StreamIndex < 0
            || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
          break;

        SkipLookahed(avail);

        const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
        if (!s.Stream)
          break;
        
        RINOK(s.SeekToStart())
        
        InitBuf();
        Vols.StreamIndex++;
        _streamPos = 0;
        Stream = s.Stream;
        continue;
      }
      limitPos = avail - kMinAllowed;
    }
    else
      limitPos = (avail - kCheckSize);

    // we don't check at (limitPos) for good fast aligned operations

    if (searchLimit)
    {
      if (_cnt > *searchLimit)
        break;
      UInt64 rem = *searchLimit - _cnt;
      if (limitPos > rem)
        limitPos = (size_t)rem + 1;
    }

    if (limitPos == 0)
      break;

    const Byte * const pStart = Buffer + _bufPos;
    const Byte * p = pStart;
    const Byte * const limit = pStart + limitPos;
   
    for (;; p++)
    {
      p = FindPK_4(p, limit);
      if (p >= limit)
        break;
      size_t rem = (size_t)(pStart + avail - p);
      /* 22.02 : we limit check size with kCheckSize to be consistent for
         any different combination of _bufPos in Buffer and size of Buffer. */
      if (rem > kCheckSize)
        rem = kCheckSize;
      const UInt32 res = IsArc_Zip_2(p, rem, isFinished);
      if (res != k_IsArc_Res_NO)
      {
        if (rem < kMarkerSize)
          return S_FALSE;
        _signature = Get32(p);
        SkipLookahed((size_t)(p - pStart));
        ArcInfo.MarkerVolIndex = Vols.StreamIndex;
        ArcInfo.MarkerPos = GetVirtStreamPos();
        ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
        SkipLookahed(4);
        if (   _signature == NSignature::kNoSpan
            || _signature == NSignature::kSpan)
        {
          if (rem < kMarkerSize * 2)
            return S_FALSE;
          ArcInfo.IsSpanMode = (_signature == NSignature::kSpan);
          _signature = Get32(p + 4);
          ArcInfo.MarkerPos2 += 4;
          SkipLookahed(4);
        }
        return S_OK;
      }
    }

    if (!IsMultiVol && isFinished)
      break;

    SkipLookahed((size_t)(p - pStart));

    if (Callback && (_cnt - progressPrev) >= ((UInt32)1 << 23))
    {
      progressPrev = _cnt;
      // const UInt64 numFiles64 = 0;
      RINOK(Callback->SetCompleted(NULL, &_cnt))
    }
  }
  
  return S_FALSE;
}


/*
---------- IncreaseRealPosition ----------
moves virtual offset in virtual stream.
changing to new volumes is allowed
*/

HRESULT CInArchive::IncreaseRealPosition(UInt64 offset, bool &isFinished)
{
  isFinished = false;

  for (;;)
  {
    const size_t avail = GetAvail();
    
    if (offset <= avail)
    {
      _bufPos += (size_t)offset;
      _cnt += offset;
      return S_OK;
    }
    
    _cnt += avail;
    offset -= avail;
    
    _bufCached = 0;
    _bufPos = 0;
    
    if (!_inBufMode)
      break;
  
    CanStartNewVol = true;
    LookAhead(1);

    if (GetAvail() == 0)
      return S_OK;
  }

  // cache is empty

  if (!IsMultiVol)
  {
    _cnt += offset;
    return Stream->Seek((Int64)offset, STREAM_SEEK_CUR, &_streamPos);
  }

  for (;;)
  {
    if (offset == 0)
      return S_OK;
    
    if (Vols.StreamIndex < 0)
      return S_FALSE;
    if ((unsigned)Vols.StreamIndex >= Vols.Streams.Size())
    {
      isFinished = true;
      return S_OK;
    }
    {
      const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex];
      if (!s.Stream)
      {
        isFinished = true;
        return S_OK;
      }
      if (_streamPos > s.Size)
        return S_FALSE;
      const UInt64 rem = s.Size - _streamPos;
      if ((UInt64)offset <= rem)
      {
        _cnt += offset;
        return Stream->Seek((Int64)offset, STREAM_SEEK_CUR, &_streamPos);
      }
      RINOK(Seek_SavePos(s.Size))
      offset -= rem;
      _cnt += rem;
    }
    
    Stream = NULL;
    _streamPos = 0;
    Vols.StreamIndex++;
    if ((unsigned)Vols.StreamIndex >= Vols.Streams.Size())
    {
      isFinished = true;
      return S_OK;
    }
    const CVols::CSubStreamInfo &s2 = Vols.Streams[(unsigned)Vols.StreamIndex];
    if (!s2.Stream)
    {
      isFinished = true;
      return S_OK;
    }
    Stream = s2.Stream;
    RINOK(Seek_SavePos(0))
  }
}



/*
---------- LookAhead ----------
Reads data to buffer, if required.

It can read from volumes as long as Buffer.Size().
But it moves to new volume, only if it's required to provide minRequired bytes in buffer.

in:
  (minRequired <= Buffer.Size())

return:
  S_OK : if (GetAvail() < minRequired) after function return, it's end of stream(s) data, or no new volume stream.
  Error codes: IInStream::Read() error or IInStream::Seek() error for multivol
*/

HRESULT CInArchive::LookAhead(size_t minRequired)
{
  for (;;)
  {
    const size_t avail = GetAvail();

    if (minRequired <= avail)
      return S_OK;
    
    if (_bufPos != 0)
    {
      if (avail != 0)
        memmove(Buffer, Buffer + _bufPos, avail);
      _bufPos = 0;
      _bufCached = avail;
    }

    const size_t pos = _bufCached;
    UInt32 processed = 0;
    HRESULT res = Stream->Read(Buffer + pos, (UInt32)(Buffer.Size() - pos), &processed);
    _streamPos += processed;
    _bufCached += processed;

    if (res != S_OK)
      return res;

    if (processed != 0)
      continue;

    if (   !IsMultiVol
        || !CanStartNewVol
        || Vols.StreamIndex < 0
        || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
      return S_OK;

    const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
    if (!s.Stream)
      return S_OK;
    
    RINOK(s.SeekToStart())

    Vols.StreamIndex++;
    _streamPos = 0;
    Stream = s.Stream;
    // Vols.NeedSeek = false;
  }
}


class CUnexpectEnd {};


/*
---------- SafeRead ----------

reads data of exact size from stream(s)

in:
  _inBufMode
  if (CanStartNewVol) it can go to next volume before first byte reading, if there is end of volume data.

in, out:
  _streamPos  :  position in Stream
  Stream
  Vols  :  if (IsMultiVol)
  _cnt

out:
  (CanStartNewVol == false), if some data was read

return:
  S_OK : success reading of requested data

exceptions:
  CSystemException() - stream reading error
  CUnexpectEnd()  :  could not read data of requested size
*/

void CInArchive::SafeRead(Byte *data, unsigned size)
{
  unsigned processed;
  HRESULT result = ReadFromCache(data, size, processed);
  if (result != S_OK)
    throw CSystemException(result);
  if (size != processed)
    throw CUnexpectEnd();
}

void CInArchive::ReadBuffer(CByteBuffer &buffer, unsigned size)
{
  buffer.Alloc(size);
  if (size != 0)
    SafeRead(buffer, size);
}

// Byte CInArchive::ReadByte  () { Byte b;      SafeRead(&b, 1); return b; }
// UInt16 CInArchive::ReadUInt16() { Byte buf[2]; SafeRead(buf, 2); return Get16(buf); }
UInt32 CInArchive::ReadUInt32() { Byte buf[4]; SafeRead(buf, 4); return Get32(buf); }
UInt64 CInArchive::ReadUInt64() { Byte buf[8]; SafeRead(buf, 8); return Get64(buf); }

void CInArchive::ReadSignature()
{
  CanStartNewVol = true;
  _signature = ReadUInt32();
  // CanStartNewVol = false; // it's already changed in SafeRead
}


// we Skip() inside headers only, so no need for stream change in multivol.

void CInArchive::Skip(size_t num)
{
  while (num != 0)
  {
    const unsigned kBufSize = (size_t)1 << 10;
    Byte buf[kBufSize];
    unsigned step = kBufSize;
    if (step > num)
      step = (unsigned)num;
    SafeRead(buf, step);
    num -= step;
  }
}

/*
HRESULT CInArchive::Callback_Completed(unsigned numFiles)
{
  const UInt64 numFiles64 = numFiles;
  return Callback->SetCompleted(&numFiles64, &_cnt);
}
*/

HRESULT CInArchive::Skip64(UInt64 num, unsigned numFiles)
{
  if (num == 0)
    return S_OK;

  for (;;)
  {
    size_t step = (size_t)1 << 24;
    if (step > num)
      step = (size_t)num;
    Skip(step);
    num -= step;
    if (num == 0)
      return S_OK;
    if (Callback)
    {
      const UInt64 numFiles64 = numFiles;
      RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
    }
  }
}


bool CInArchive::ReadFileName(unsigned size, AString &s)
{
  if (size == 0)
  {
    s.Empty();
    return true;
  }
  char *p = s.GetBuf(size);
  SafeRead((Byte *)p, size);
  unsigned i = size;
  do
  {
    if (p[i - 1] != 0)
      break;
  }
  while (--i);
  s.ReleaseBuf_CalcLen(size);
  return s.Len() == i;
}


#define ZIP64_IS_32_MAX(n) ((n) == 0xFFFFFFFF)
#define ZIP64_IS_16_MAX(n) ((n) == 0xFFFF)


bool CInArchive::ReadExtra(const CLocalItem &item, unsigned extraSize, CExtraBlock &extra,
    UInt64 &unpackSize, UInt64 &packSize,
    CItem *cdItem)
{
  extra.Clear();
  
  while (extraSize >= 4)
  {
    CExtraSubBlock subBlock;
    const UInt32 pair = ReadUInt32();
    subBlock.ID = (pair & 0xFFFF);
    unsigned size = (unsigned)(pair >> 16);
    // const unsigned origSize = size;
    
    extraSize -= 4;
    
    if (size > extraSize)
    {
      // it's error in extra
      HeadersWarning = true;
      extra.Error = true;
      Skip(extraSize);
      return false;
    }
 
    extraSize -= size;
    
    if (subBlock.ID == NFileHeader::NExtraID::kZip64)
    {
      extra.IsZip64 = true;
      bool isOK = true;

      if (!cdItem
          && size == 16
          && !ZIP64_IS_32_MAX(unpackSize)
          && !ZIP64_IS_32_MAX(packSize))
      {
        /* Win10 Explorer's "Send to Zip" for big (3500 MiB) files
           creates Zip64 Extra in local file header.
           But if both uncompressed and compressed sizes are smaller than 4 GiB,
           Win10 doesn't store 0xFFFFFFFF in 32-bit fields as expected by zip specification.
           21.04: we ignore these minor errors in Win10 zip archives. */
        if (ReadUInt64() != unpackSize)
          isOK = false;
        if (ReadUInt64() != packSize)
          isOK = false;
        size = 0;
      }
      else
      {
        if (ZIP64_IS_32_MAX(unpackSize))
          { if (size < 8) isOK = false; else { size -= 8; unpackSize = ReadUInt64(); }}
      
        if (isOK && ZIP64_IS_32_MAX(packSize))
          { if (size < 8) isOK = false; else { size -= 8; packSize = ReadUInt64(); }}
      
        if (cdItem)
        {
          if (isOK)
          {
            if (ZIP64_IS_32_MAX(cdItem->LocalHeaderPos))
              { if (size < 8) isOK = false; else { size -= 8; cdItem->LocalHeaderPos = ReadUInt64(); }}
            /*
            else if (size == 8)
            {
              size -= 8;
              const UInt64 v = ReadUInt64();
              // soong_zip, an AOSP tool (written in the Go) writes incorrect value.
              // we can ignore that minor error here
              if (v != cdItem->LocalHeaderPos)
                isOK = false; // ignore error
              // isOK = false; // force error
            }
            */
          }
         
          if (isOK && ZIP64_IS_16_MAX(cdItem->Disk))
            { if (size < 4) isOK = false; else { size -= 4; cdItem->Disk = ReadUInt32(); }}
        }
      }
    
      // we can ignore errors, when some zip archiver still write all fields to zip64 extra in local header
      // if (&& (cdItem || !isOK || origSize != 8 * 3 + 4 || size != 8 * 1 + 4))
      if (!isOK || size != 0)
      {
        HeadersWarning = true;
        extra.Error = true;
        extra.IsZip64_Error = true;
      }
      Skip(size);
    }
    else
    {
      ReadBuffer(subBlock.Data, size);
      extra.SubBlocks.Add(subBlock);
      if (subBlock.ID == NFileHeader::NExtraID::kIzUnicodeName)
      {
        if (!subBlock.CheckIzUnicode(item.Name))
          extra.Error = true;
      }
    }
  }

  if (extraSize != 0)
  {
    ExtraMinorError = true;
    extra.MinorError = true;
    // 7-Zip before 9.31 created incorrect WzAES Extra in folder's local headers.
    // so we don't return false, but just set warning flag
    // return false;
    Skip(extraSize);
  }

  return true;
}


bool CInArchive::ReadLocalItem(CItemEx &item)
{
  item.Disk = 0;
  if (IsMultiVol && Vols.StreamIndex >= 0)
    item.Disk = (UInt32)Vols.StreamIndex;
  const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
  Byte p[kPureHeaderSize];
  SafeRead(p, kPureHeaderSize);
  {
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return false;
  }

  item.ExtractVersion.Version = p[0];
  item.ExtractVersion.HostOS = p[1];
  G16(2, item.Flags);
  G16(4, item.Method);
  G32(6, item.Time);
  G32(10, item.Crc);
  G32(14, item.PackSize);
  G32(18, item.Size);
  const unsigned nameSize = Get16(p + 22);
  const unsigned extraSize = Get16(p + 24);
  bool isOkName = ReadFileName(nameSize, item.Name);
  item.LocalFullHeaderSize = kLocalHeaderSize + (UInt32)nameSize + extraSize;
  item.DescriptorWasRead = false;

  /*
  if (item.IsDir())
    item.Size = 0; // check It
  */

  if (extraSize > 0)
  {
    if (!ReadExtra(item, extraSize, item.LocalExtra, item.Size, item.PackSize, NULL))
    {
      /* Most of archives are OK for Extra. But there are some rare cases
         that have error. And if error in first item, it can't open archive.
         So we ignore that error */
      // return false;
    }
  }
  
  if (!CheckDosTime(item.Time))
  {
    HeadersWarning = true;
    // return false;
  }
  
  if (item.Name.Len() != nameSize)
  {
    // we support some "bad" zip archives that contain zeros after name
    if (!isOkName)
      return false;
    HeadersWarning = true;
  }
  
  // return item.LocalFullHeaderSize <= ((UInt32)1 << 16);
  return true;
}


static bool FlagsAreSame(const CItem &i1, const CItem &i2_cd)
{
  if (i1.Method != i2_cd.Method)
    return false;
  
  UInt32 mask = i1.Flags ^ i2_cd.Flags;
  if (mask == 0)
    return true;
  switch (i1.Method)
  {
    case NFileHeader::NCompressionMethod::kDeflate:
      mask &= 0x7FF9;
      break;
    default:
      if (i1.Method <= NFileHeader::NCompressionMethod::kImplode)
        mask &= 0x7FFF;
  }

  // we can ignore utf8 flag, if name is ascii, or if only cdItem has utf8 flag
  if (mask & NFileHeader::NFlags::kUtf8)
    if ((i1.Name.IsAscii() && i2_cd.Name.IsAscii())
        || (i2_cd.Flags & NFileHeader::NFlags::kUtf8))
      mask &= ~NFileHeader::NFlags::kUtf8;

  // some bad archive in rare case can use descriptor without descriptor flag in Central Dir
  // if (i1.HasDescriptor())
  mask &= ~NFileHeader::NFlags::kDescriptorUsedMask;
  
  return (mask == 0);
}


// #ifdef _WIN32
static bool AreEqualPaths_IgnoreSlashes(const char *s1, const char *s2)
{
  for (;;)
  {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 == c2)
    {
      if (c1 == 0)
        return true;
    }
    else
    {
      if (c1 == '\\') c1 = '/';
      if (c2 == '\\') c2 = '/';
      if (c1 != c2)
        return false;
    }
  }
}
// #endif


static bool AreItemsEqual(const CItemEx &localItem, const CItemEx &cdItem)
{
  if (!FlagsAreSame(localItem, cdItem))
    return false;
  if (!localItem.HasDescriptor())
  {
    if (cdItem.PackSize != localItem.PackSize
        || cdItem.Size != localItem.Size
        || (cdItem.Crc != localItem.Crc && cdItem.Crc != 0)) // some program writes 0 to crc field in central directory
      return false;
  }
  /* pkzip 2.50 creates incorrect archives. It uses
       - WIN encoding for name in local header
       - OEM encoding for name in central header
     We don't support these strange items. */

  /* if (cdItem.Name.Len() != localItem.Name.Len())
    return false;
  */
  if (cdItem.Name != localItem.Name)
  {
    // #ifdef _WIN32
    // some xap files use backslash in central dir items.
    // we can ignore such errors in windows, where all slashes are converted to backslashes
    unsigned hostOs = cdItem.GetHostOS();
    
    if (hostOs == NFileHeader::NHostOS::kFAT ||
        hostOs == NFileHeader::NHostOS::kNTFS)
    {
      if (!AreEqualPaths_IgnoreSlashes(cdItem.Name, localItem.Name))
      {
        // pkzip 2.50 uses DOS encoding in central dir and WIN encoding in local header.
        // so we ignore that error
        if (hostOs != NFileHeader::NHostOS::kFAT
            || cdItem.MadeByVersion.Version < 25
            || cdItem.MadeByVersion.Version > 40)
          return false;
      }
    }
    /*
    else
    #endif
      return false;
    */
  }
  return true;
}


HRESULT CInArchive::Read_LocalItem_After_CdItem(CItemEx &item, bool &isAvail, bool &headersError)
{
  isAvail = true;
  headersError = false;
  if (item.FromLocal)
    return S_OK;
  try
  {
    UInt64 offset = item.LocalHeaderPos;

    if (IsMultiVol)
    {
      if (item.Disk >= Vols.Streams.Size())
      {
        isAvail = false;
        return S_FALSE;
      }
      Stream = Vols.Streams[item.Disk].Stream;
      Vols.StreamIndex = (int)item.Disk;
      if (!Stream)
      {
        isAvail = false;
        return S_FALSE;
      }
    }
    else
    {
      if (UseDisk_in_SingleVol && item.Disk != EcdVolIndex)
      {
        isAvail = false;
        return S_FALSE;
      }
      Stream = StreamRef;

      offset = (UInt64)((Int64)offset + ArcInfo.Base);
      if (ArcInfo.Base < 0 && (Int64)offset < 0)
      {
        isAvail = false;
        return S_FALSE;
      }
    }

    _inBufMode = false;
    RINOK(Seek_SavePos(offset))
    InitBuf();
    /*
    // we can use buf mode with small buffer to reduce
    // the number of Read() calls in ReadLocalItem()
    _inBufMode = true;
    Buffer.Alloc(1 << 10);
    if (!Buffer.IsAllocated())
      return E_OUTOFMEMORY;
    */

    CItemEx localItem;
    if (ReadUInt32() != NSignature::kLocalFileHeader)
      return S_FALSE;
    ReadLocalItem(localItem);
    if (!AreItemsEqual(localItem, item))
      return S_FALSE;
    item.LocalFullHeaderSize = localItem.LocalFullHeaderSize;
    item.LocalExtra = localItem.LocalExtra;
    if (item.Crc != localItem.Crc && !localItem.HasDescriptor())
    {
      item.Crc = localItem.Crc;
      headersError = true;
    }
    if ((item.Flags ^ localItem.Flags) & NFileHeader::NFlags::kDescriptorUsedMask)
    {
      item.Flags = (UInt16)(item.Flags ^ NFileHeader::NFlags::kDescriptorUsedMask);
      headersError = true;
    }
    item.FromLocal = true;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}


/*
---------- FindDescriptor ----------

in:
  _streamPos : position in Stream
  Stream :
  Vols : if (IsMultiVol)

action:
  searches descriptor in input stream(s).
  sets
    item.DescriptorWasRead = true;
    item.Size
    item.PackSize
    item.Crc
  if descriptor was found

out:
  S_OK:
      if ( item.DescriptorWasRead) : if descriptor was found
      if (!item.DescriptorWasRead) : if descriptor was not found : unexpected end of stream(s)

  S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

  another error code: Callback error.

exceptions :
  CSystemException() : stream reading error
*/

HRESULT CInArchive::FindDescriptor(CItemEx &item, unsigned numFiles)
{
  // const size_t kBufSize = (size_t)1 << 5; // don't increase it too much. It reads data look ahead.

  // Buffer.Alloc(kBufSize);
  // Byte *buf = Buffer;
  
  UInt64 packedSize = 0;
  
  UInt64 progressPrev = _cnt;

  for (;;)
  {
    /* appnote specification claims that we must use 64-bit descriptor, if there is zip64 extra.
       But some old third-party xps archives used 64-bit descriptor without zip64 extra. */
    // unsigned descriptorSize = kDataDescriptorSize64 + kNextSignatureSize;
    
    // const unsigned kNextSignatureSize = 0;  // we can disable check for next signatuire
    const unsigned kNextSignatureSize = 4;  // we check also for signature for next File headear

    const unsigned descriptorSize4 = item.GetDescriptorSize() + kNextSignatureSize;

    if (descriptorSize4 > Buffer.Size()) return E_FAIL;

    // size_t processedSize;
    CanStartNewVol = true;
    RINOK(LookAhead(descriptorSize4))
    const size_t avail = GetAvail();
    
    if (avail < descriptorSize4)
    {
      // we write to packSize all these available bytes.
      // later it's simpler to work with such value than with 0
      // if (item.PackSize == 0)
        item.PackSize = packedSize + avail;
      if (item.Method == 0)
        item.Size = item.PackSize;
      SkipLookahed(avail);
      return S_OK;
    }

    const Byte * const pStart = Buffer + _bufPos;
    const Byte * p = pStart;
    const Byte * const limit = pStart + (avail - descriptorSize4);
    
    for (; p <= limit; p++)
    {
      // descriptor signature field is Info-ZIP's extension to pkware Zip specification.
      // New ZIP specification also allows descriptorSignature.
      
      p = FindPK_4(p, limit + 1);
      if (p > limit)
        break;

      /*
      if (*p != 0x50)
        continue;
      */

      if (Get32(p) != NSignature::kDataDescriptor)
        continue;

      // we check next signatuire after descriptor
      // maybe we need check only 2 bytes "PK" instead of 4 bytes, if some another type of header is possible after descriptor
      const UInt32 sig = Get32(p + descriptorSize4 - kNextSignatureSize);
      if (   sig != NSignature::kLocalFileHeader
          && sig != NSignature::kCentralFileHeader)
        continue;

      const UInt64 packSizeCur = packedSize + (size_t)(p - pStart);
      if (descriptorSize4 == kDataDescriptorSize64 + kNextSignatureSize) // if (item.LocalExtra.IsZip64)
      {
        const UInt64 descriptorPackSize = Get64(p + 8);
        if (descriptorPackSize != packSizeCur)
          continue;
        item.Size = Get64(p + 16);
      }
      else
      {
        const UInt32 descriptorPackSize = Get32(p + 8);
        if (descriptorPackSize != (UInt32)packSizeCur)
          continue;
        item.Size = Get32(p + 12);
        // that item.Size can be truncated to 32-bit value here
      }
      // We write calculated 64-bit packSize, even if descriptor64 was not used
      item.PackSize = packSizeCur;
      
      item.DescriptorWasRead = true;
      item.Crc = Get32(p + 4);

      const size_t skip = (size_t)(p - pStart) + descriptorSize4 - kNextSignatureSize;

      SkipLookahed(skip);

      return S_OK;
    }
    
    const size_t skip = (size_t)(p - pStart);
    SkipLookahed(skip);

    packedSize += skip;

    if (Callback)
    if (_cnt - progressPrev >= ((UInt32)1 << 22))
    {
      progressPrev = _cnt;
      const UInt64 numFiles64 = numFiles;
      RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
    }
  }
}


HRESULT CInArchive::CheckDescriptor(const CItemEx &item)
{
  if (!item.HasDescriptor())
    return S_OK;
  
  // pkzip's version without descriptor signature is not supported
  
  bool isFinished = false;
  RINOK(IncreaseRealPosition(item.PackSize, isFinished))
  if (isFinished)
    return S_FALSE;

  /*
  if (!IsMultiVol)
  {
    RINOK(Seek_SavePos(ArcInfo.Base + item.GetDataPosition() + item.PackSize));
  }
  */
  
  Byte buf[kDataDescriptorSize64];
  try
  {
    CanStartNewVol = true;
    SafeRead(buf, item.GetDescriptorSize());
  }
  catch (const CSystemException &e) { return e.ErrorCode; }
  // catch (const CUnexpectEnd &)
  catch(...)
  {
    return S_FALSE;
  }
  // RINOK(ReadStream_FALSE(Stream, buf, item.GetDescriptorSize()));

  if (Get32(buf) != NSignature::kDataDescriptor)
    return S_FALSE;
  UInt32 crc = Get32(buf + 4);
  UInt64 packSize, unpackSize;
  
  if (item.LocalExtra.IsZip64)
  {
    packSize = Get64(buf + 8);
    unpackSize = Get64(buf + 16);
  }
  else
  {
    packSize = Get32(buf + 8);
    unpackSize = Get32(buf + 12);
  }
  
  if (crc != item.Crc || item.PackSize != packSize || item.Size != unpackSize)
    return S_FALSE;
  return S_OK;
}


HRESULT CInArchive::Read_LocalItem_After_CdItem_Full(CItemEx &item)
{
  if (item.FromLocal)
    return S_OK;
  try
  {
    bool isAvail = true;
    bool headersError = false;
    RINOK(Read_LocalItem_After_CdItem(item, isAvail, headersError))
    if (headersError)
      return S_FALSE;
    if (item.HasDescriptor())
      return CheckDescriptor(item);
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}
  

HRESULT CInArchive::ReadCdItem(CItemEx &item)
{
  item.FromCentral = true;
  Byte p[kCentralHeaderSize - 4];
  SafeRead(p, kCentralHeaderSize - 4);

  item.MadeByVersion.Version = p[0];
  item.MadeByVersion.HostOS = p[1];
  item.ExtractVersion.Version = p[2];
  item.ExtractVersion.HostOS = p[3];
  G16(4, item.Flags);
  G16(6, item.Method);
  G32(8, item.Time);
  G32(12, item.Crc);
  G32(16, item.PackSize);
  G32(20, item.Size);
  const unsigned nameSize = Get16(p + 24);
  const unsigned extraSize = Get16(p + 26);
  const unsigned commentSize = Get16(p + 28);
  G16(30, item.Disk);
  G16(32, item.InternalAttrib);
  G32(34, item.ExternalAttrib);
  G32(38, item.LocalHeaderPos);
  ReadFileName(nameSize, item.Name);
  
  if (extraSize > 0)
    ReadExtra(item, extraSize, item.CentralExtra, item.Size, item.PackSize, &item);

  // May be these strings must be deleted
  /*
  if (item.IsDir())
    item.Size = 0;
  */
  
  ReadBuffer(item.Comment, commentSize);
  return S_OK;
}


/*
TryEcd64()
  (_inBufMode == false) is expected here
  so TryEcd64() can't change the Buffer.
  if (Ecd64 is not covered by cached region),
    TryEcd64() can change cached region ranges (_bufCached, _bufPos) and _streamPos.
*/

HRESULT CInArchive::TryEcd64(UInt64 offset, CCdInfo &cdInfo)
{
  if (offset >= ((UInt64)1 << 63))
    return S_FALSE;
  Byte buf[kEcd64_FullSize];

  RINOK(SeekToVol(Vols.StreamIndex, offset))
  RINOK(ReadFromCache_FALSE(buf, kEcd64_FullSize))

  if (Get32(buf) != NSignature::kEcd64)
    return S_FALSE;
  UInt64 mainSize = Get64(buf + 4);
  if (mainSize < kEcd64_MainSize || mainSize > ((UInt64)1 << 40))
    return S_FALSE;
  cdInfo.ParseEcd64e(buf + 12);
  return S_OK;
}


/* FindCd() doesn't use previous cached region,
   but it uses Buffer. So it sets new cached region */

HRESULT CInArchive::FindCd(bool checkOffsetMode)
{
  // There are no useful data in cache in most cases here.
  // So here we don't use cache data from previous operations.
  InitBuf();
  UInt64 endPos;
  RINOK(InStream_GetSize_SeekToEnd(Stream, endPos))
  _streamPos = endPos;
  const size_t kBufSizeMax = (size_t)1 << 17; // must be larger than
      // (1 << 16) + kEcdSize + kEcd64Locator_Size + kEcd64_FullSize
  const size_t bufSize = (endPos < kBufSizeMax) ? (size_t)endPos : kBufSizeMax;
  if (bufSize < kEcdSize)
    return S_FALSE;
  RINOK(AllocateBuffer(kBufSizeMax))
  {
    RINOK(Seek_SavePos(endPos - bufSize))
    size_t processed = bufSize;
    const HRESULT res = ReadStream(Stream, Buffer, &processed);
    _streamPos += processed;
    _bufCached = processed;
    _bufPos = 0;
    _cnt += processed;
    if (res != S_OK)
      return res;
    if (processed != bufSize)
      return S_FALSE;
  }

  CCdInfo &cdInfo = Vols.ecd;
  
  for (size_t i = bufSize - kEcdSize + 1;;)
  {
    const Byte *buf = Buffer;
    {
      const Byte *p = buf + i;
      do
        if (p == buf)
          return S_FALSE;
      while (*(--p) != 0x50);

      i = (size_t)(p - buf);
      if (Get32(p) != NSignature::kEcd)
        continue;
      cdInfo.ParseEcd32(p);
    }
    
    if (i >= kEcd64Locator_Size)
    {
      const size_t locatorIndex = i - kEcd64Locator_Size;
      if (Get32(buf + locatorIndex) == NSignature::kEcd64Locator)
      {
        CLocator locator;
        locator.Parse(buf + locatorIndex + 4);
        UInt32 numDisks = locator.NumDisks;
        // we ignore the error, where some zip creators use (NumDisks == 0)
        if (numDisks == 0)
          numDisks = 1;
        if ((cdInfo.ThisDisk == numDisks - 1 || ZIP64_IS_16_MAX(cdInfo.ThisDisk))
            && locator.Ecd64Disk < numDisks)
        {
          if (locator.Ecd64Disk != cdInfo.ThisDisk && !ZIP64_IS_16_MAX(cdInfo.ThisDisk))
            return E_NOTIMPL;
          
          // Most of the zip64 use fixed size Zip64 ECD
          // we try relative backward reading.
          const UInt64 absEcd64 = endPos - bufSize + i - (kEcd64Locator_Size + kEcd64_FullSize);
          
          if (locatorIndex >= kEcd64_FullSize)
          if (checkOffsetMode || absEcd64 == locator.Ecd64Offset)
          {
            const Byte *ecd64 = buf + locatorIndex - kEcd64_FullSize;
            if (Get32(ecd64) == NSignature::kEcd64 &&
                Get64(ecd64 + 4) == kEcd64_MainSize)
            {
              cdInfo.ParseEcd64e(ecd64 + 12);
              ArcInfo.Base = (Int64)(absEcd64 - locator.Ecd64Offset);
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
          
          // some zip64 use variable size Zip64 ECD.
          // we try to use absolute offset from locator.
          if (absEcd64 != locator.Ecd64Offset)
          {
            if (TryEcd64(locator.Ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = 0;
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
          
          // for variable Zip64 ECD with for archives with offset != 0.

          if (checkOffsetMode
              && ArcInfo.MarkerPos != 0
              && ArcInfo.MarkerPos + locator.Ecd64Offset != absEcd64)
          {
            if (TryEcd64(ArcInfo.MarkerPos + locator.Ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
        }
      }
    }
    
    // bool isVolMode = (Vols.EndVolIndex != -1);
    // UInt32 searchDisk = (isVolMode ? Vols.EndVolIndex : 0);
    
    if (/* searchDisk == thisDisk && */ cdInfo.CdDisk <= cdInfo.ThisDisk)
    {
      // if (isVolMode)
      {
        if (cdInfo.CdDisk != cdInfo.ThisDisk)
          return S_OK;
      }
      
      UInt64 absEcdPos = endPos - bufSize + i;
      UInt64 cdEnd = cdInfo.Size + cdInfo.Offset;
      ArcInfo.Base = 0;
      // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
      if (absEcdPos != cdEnd)
      {
        /*
        if (cdInfo.Offset <= 16 && cdInfo.Size != 0)
        {
          // here we support some rare ZIP files with Central directory at the start
          ArcInfo.Base = 0;
        }
        else
        */
        ArcInfo.Base = (Int64)(absEcdPos - cdEnd);
      }
      return S_OK;
    }
  }
}


HRESULT CInArchive::TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, UInt64 cdOffset, UInt64 cdSize)
{
  items.Clear();
  IsCdUnsorted = false;
  
  if ((Int64)cdOffset < 0)
    return S_FALSE;

  // _startLocalFromCd_Disk = (UInt32)(Int32)-1;
  // _startLocalFromCd_Offset = (UInt64)(Int64)-1;

  RINOK(SeekToVol(IsMultiVol ? (int)cdInfo.CdDisk : -1, cdOffset))

  _inBufMode = true;
  _cnt = 0;

  if (Callback)
  {
    RINOK(Callback->SetTotal(&cdInfo.NumEntries, IsMultiVol ? &Vols.TotalBytesSize : NULL))
  }
  UInt64 numFileExpected = cdInfo.NumEntries;
  const UInt64 *totalFilesPtr = &numFileExpected;
  bool isCorrect_NumEntries = (cdInfo.IsFromEcd64 || numFileExpected >= ((UInt32)1 << 16));

  while (_cnt < cdSize)
  {
    CanStartNewVol = true;
    if (ReadUInt32() != NSignature::kCentralFileHeader)
      return S_FALSE;
    CanStartNewVol = false;
    {
      CItemEx cdItem;
      RINOK(ReadCdItem(cdItem))
      
      /*
      if (cdItem.Disk < _startLocalFromCd_Disk ||
          cdItem.Disk == _startLocalFromCd_Disk &&
          cdItem.LocalHeaderPos < _startLocalFromCd_Offset)
      {
        _startLocalFromCd_Disk = cdItem.Disk;
        _startLocalFromCd_Offset = cdItem.LocalHeaderPos;
      }
      */

      if (items.Size() > 0 && !IsCdUnsorted)
      {
        const CItemEx &prev = items.Back();
        if (cdItem.Disk < prev.Disk
            || (cdItem.Disk == prev.Disk &&
            cdItem.LocalHeaderPos < prev.LocalHeaderPos))
          IsCdUnsorted = true;
      }

      items.Add(cdItem);
    }
    if (Callback && (items.Size() & 0xFFF) == 0)
    {
      const UInt64 numFiles = items.Size();

      if (numFiles > numFileExpected && totalFilesPtr)
      {
        if (isCorrect_NumEntries)
          totalFilesPtr = NULL;
        else
          while (numFiles > numFileExpected)
            numFileExpected += (UInt32)1 << 16;
        RINOK(Callback->SetTotal(totalFilesPtr, NULL))
      }

      RINOK(Callback->SetCompleted(&numFiles, &_cnt))
    }
  }

  CanStartNewVol = true;

  return (_cnt == cdSize) ? S_OK : S_FALSE;
}


/*
static int CompareCdItems(void *const *elem1, void *const *elem2, void *)
{
  const CItemEx *i1 = *(const CItemEx **)elem1;
  const CItemEx *i2 = *(const CItemEx **)elem2;

  if (i1->Disk < i2->Disk) return -1;
  if (i1->Disk > i2->Disk) return 1;
  if (i1->LocalHeaderPos < i2->LocalHeaderPos) return -1;
  if (i1->LocalHeaderPos > i2->LocalHeaderPos) return 1;
  if (i1 < i2) return -1;
  if (i1 > i2) return 1;
  return 0;
}
*/

HRESULT CInArchive::ReadCd(CObjectVector<CItemEx> &items, UInt32 &cdDisk, UInt64 &cdOffset, UInt64 &cdSize)
{
  bool checkOffsetMode = true;
  
  if (IsMultiVol)
  {
    if (Vols.EndVolIndex == -1)
      return S_FALSE;
    Stream = Vols.Streams[(unsigned)Vols.EndVolIndex].Stream;
    if (!Vols.StartIsZip)
      checkOffsetMode = false;
  }
  else
    Stream = StartStream;

  if (!Vols.ecd_wasRead)
  {
    RINOK(FindCd(checkOffsetMode))
  }

  CCdInfo &cdInfo = Vols.ecd;
  
  HRESULT res = S_FALSE;
  
  cdSize = cdInfo.Size;
  cdOffset = cdInfo.Offset;
  cdDisk = cdInfo.CdDisk;

  if (!IsMultiVol)
  {
    if (cdInfo.ThisDisk != cdInfo.CdDisk)
      return S_FALSE;
  }

  const UInt64 base = (IsMultiVol ? 0 : (UInt64)ArcInfo.Base);
  res = TryReadCd(items, cdInfo, base + cdOffset, cdSize);
  
  if (res == S_FALSE && !IsMultiVol && base != ArcInfo.MarkerPos)
  {
    // do we need that additional attempt to read cd?
    res = TryReadCd(items, cdInfo, ArcInfo.MarkerPos + cdOffset, cdSize);
    if (res == S_OK)
      ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
  }
  
  // Some rare case files are unsorted
  // items.Sort(CompareCdItems, NULL);
  return res;
}


static int FindItem(const CObjectVector<CItemEx> &items, const CItemEx &item)
{
  unsigned left = 0, right = items.Size();
  for (;;)
  {
    if (left >= right)
      return -1;
    const unsigned index = (unsigned)(((size_t)left + (size_t)right) / 2);
    const CItemEx &item2 = items[index];
    if (item.Disk < item2.Disk)
      right = index;
    else if (item.Disk > item2.Disk)
      left = index + 1;
    else if (item.LocalHeaderPos == item2.LocalHeaderPos)
      return (int)index;
    else if (item.LocalHeaderPos < item2.LocalHeaderPos)
      right = index;
    else
      left = index + 1;
  }
}

static bool IsStrangeItem(const CItem &item)
{
  return item.Name.Len() > (1 << 14) || item.Method > (1 << 8);
}



/*
  ---------- ReadLocals ----------

in:
  (_signature == NSignature::kLocalFileHeader)
  VirtStreamPos : after _signature : position in Stream
  Stream :
  Vols : if (IsMultiVol)
  (_inBufMode == false)

action:
  it parses local items.

  if ( IsMultiVol) it writes absolute offsets to CItemEx::LocalHeaderPos
  if (!IsMultiVol) it writes relative (from ArcInfo.Base) offsets to CItemEx::LocalHeaderPos
               later we can correct CItemEx::LocalHeaderPos values, if
               some new value for ArcInfo.Base will be detected
out:
  S_OK:
    (_signature != NSignature::kLocalFileHeade)
    _streamPos : after _signature

  S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

  another error code: stream reading error or Callback error.

  CUnexpectEnd() exception : it's not fatal exception here.
      It means that reading was interrupted by unexpected end of input stream,
      but some CItemEx items were parsed OK.
      We can stop further archive parsing.
      But we can use all filled CItemEx items.
*/

HRESULT CInArchive::ReadLocals(CObjectVector<CItemEx> &items)
{
  items.Clear();

  UInt64 progressPrev = _cnt;
  
  if (Callback)
  {
    RINOK(Callback->SetTotal(NULL, IsMultiVol ? &Vols.TotalBytesSize : NULL))
  }

  while (_signature == NSignature::kLocalFileHeader)
  {
    CItemEx item;

    item.LocalHeaderPos = GetVirtStreamPos() - 4;
    if (!IsMultiVol)
      item.LocalHeaderPos = (UInt64)((Int64)item.LocalHeaderPos - ArcInfo.Base);
    
    try
    {
      ReadLocalItem(item);
      item.FromLocal = true;
      bool isFinished = false;

      if (item.HasDescriptor())
      {
        RINOK(FindDescriptor(item, items.Size()))
        isFinished = !item.DescriptorWasRead;
      }
      else
      {
        if (item.PackSize >= ((UInt64)1 << 62))
          throw CUnexpectEnd();
        RINOK(IncreaseRealPosition(item.PackSize, isFinished))
      }
   
      items.Add(item);
      
      if (isFinished)
        throw CUnexpectEnd();

      ReadSignature();
    }
    catch (CUnexpectEnd &)
    {
      if (items.IsEmpty() || (items.Size() == 1 && IsStrangeItem(items[0])))
        return S_FALSE;
      throw;
    }


    if (Callback)
    if ((items.Size() & 0xFF) == 0
        || _cnt - progressPrev >= ((UInt32)1 << 22))
    {
      progressPrev = _cnt;
      const UInt64 numFiles = items.Size();
      RINOK(Callback->SetCompleted(&numFiles, &_cnt))
    }
  }

  if (items.Size() == 1 && _signature != NSignature::kCentralFileHeader)
    if (IsStrangeItem(items[0]))
      return S_FALSE;
  
  return S_OK;
}



HRESULT CVols::ParseArcName(IArchiveOpenVolumeCallback *volCallback)
{
  UString name;
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(volCallback->GetProperty(kpidName, &prop))
    if (prop.vt != VT_BSTR)
      return S_OK;
    name = prop.bstrVal;
  }

  const int dotPos = name.ReverseFind_Dot();
  if (dotPos < 0)
    return S_OK;
  const UString ext = name.Ptr((unsigned)(dotPos + 1));
  name.DeleteFrom((unsigned)(dotPos + 1));

  StartVolIndex = (Int32)(-1);

  if (ext.IsEmpty())
    return S_OK;
  {
    wchar_t c = ext[0];
    IsUpperCase = (c >= 'A' && c <= 'Z');
    if (ext.IsEqualTo_Ascii_NoCase("zip"))
    {
      BaseName = name;
      StartIsZ = true;
      StartIsZip = true;
      return S_OK;
    }
    else if (ext.IsEqualTo_Ascii_NoCase("exe"))
    {
      /* possible cases:
         - exe with zip inside
         - sfx: a.exe, a.z02, a.z03,... , a.zip
                a.exe is start volume.
         - zip renamed to exe
      */

      StartIsExe = true;
      BaseName = name;
      StartVolIndex = 0;
      /* sfx-zip can use both arc.exe and arc.zip
         We can open arc.zip, if it was requesed to open arc.exe.
         But it's possible that arc.exe and arc.zip are not parts of same archive.
         So we can disable such operation */

      // 18.04: we still want to open zip renamed to exe.
      /*
      {
        UString volName = name;
        volName += IsUpperCase ? "Z01" : "z01";
        {
          CMyComPtr<IInStream> stream;
          HRESULT res2 = volCallback->GetStream(volName, &stream);
          if (res2 == S_OK)
            DisableVolsSearch = true;
        }
      }
      */
      DisableVolsSearch = true;
      return S_OK;
    }
    else if (ext[0] == 'z' || ext[0] == 'Z')
    {
      if (ext.Len() < 3)
        return S_OK;
      const wchar_t *end = NULL;
      UInt32 volNum = ConvertStringToUInt32(ext.Ptr(1), &end);
      if (*end != 0 || volNum < 1 || volNum > ((UInt32)1 << 30))
        return S_OK;
      StartVolIndex = (Int32)(volNum - 1);
      BaseName = name;
      StartIsZ = true;
    }
    else
      return S_OK;
  }

  UString volName = BaseName;
  volName += (IsUpperCase ? "ZIP" : "zip");
  
  HRESULT res = volCallback->GetStream(volName, &ZipStream);
  
  if (res == S_FALSE || !ZipStream)
  {
    if (MissingName.IsEmpty())
    {
      MissingZip = true;
      MissingName = volName;
    }
    return S_OK;
  }

  return res;
}


HRESULT CInArchive::ReadVols2(IArchiveOpenVolumeCallback *volCallback,
    unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols)
{
  if (Vols.DisableVolsSearch)
    return S_OK;

  numMissingVols = 0;

  for (unsigned i = start;; i++)
  {
    if (lastDisk >= 0 && i >= (unsigned)lastDisk)
      break;
    
    if (i < Vols.Streams.Size())
      if (Vols.Streams[i].Stream)
        continue;

    CMyComPtr<IInStream> stream;

    if ((int)i == zipDisk)
    {
      stream = Vols.ZipStream;
    }
    else if ((int)i == Vols.StartVolIndex)
    {
      stream = StartStream;
    }
    else
    {
      UString volName = Vols.BaseName;
      {
        volName.Add_Char(Vols.IsUpperCase ? 'Z' : 'z');
        const unsigned v = i + 1;
        if (v < 10)
          volName.Add_Char('0');
        volName.Add_UInt32(v);
      }
        
      HRESULT res = volCallback->GetStream(volName, &stream);
      if (res != S_OK && res != S_FALSE)
        return res;
      if (res == S_FALSE || !stream)
      {
        if (i == 0)
        {
          UString volName_exe = Vols.BaseName;
          volName_exe += (Vols.IsUpperCase ? "EXE" : "exe");
          
          HRESULT res2 = volCallback->GetStream(volName_exe, &stream);
          if (res2 != S_OK && res2 != S_FALSE)
            return res2;
          res = res2;
        }
      }
      if (res == S_FALSE || !stream)
      {
        if (i == 1 && Vols.StartIsExe)
          return S_OK;
        if (Vols.MissingName.IsEmpty())
          Vols.MissingName = volName;
        numMissingVols++;
        if (numMissingVols > numMissingVolsMax)
          return S_OK;
        if (lastDisk == -1 && numMissingVols != 0)
          return S_OK;
        continue;
      }
    }

    UInt64 pos, size;
    RINOK(InStream_GetPos_GetSize(stream, pos, size))

    while (i >= Vols.Streams.Size())
      Vols.Streams.AddNew();
    
    CVols::CSubStreamInfo &ss = Vols.Streams[i];
    Vols.NumVols++;
    Vols.TotalBytesSize += size;

    ss.Stream = stream;
    ss.Size = size;

    if ((int)i == zipDisk)
    {
      Vols.EndVolIndex = (int)(Vols.Streams.Size() - 1);
      break;
    }
  }

  return S_OK;
}


HRESULT CInArchive::ReadVols()
{
  CMyComPtr<IArchiveOpenVolumeCallback> volCallback;

  Callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volCallback);
  if (!volCallback)
    return S_OK;

  RINOK(Vols.ParseArcName(volCallback))

  // const int startZIndex = Vols.StartVolIndex;

  if (!Vols.StartIsZ)
  {
    if (!Vols.StartIsExe)
      return S_OK;
  }

  int zipDisk = -1;
  int cdDisk = -1;

  if (Vols.StartIsZip)
    Vols.ZipStream = StartStream;

  if (Vols.ZipStream)
  {
    Stream = Vols.ZipStream;
    
    if (Vols.StartIsZip)
      Vols.StreamIndex = -1;
    else
    {
      Vols.StreamIndex = -2;
      InitBuf();
    }

    HRESULT res = FindCd(true);

    CCdInfo &ecd = Vols.ecd;
    if (res == S_OK)
    {
      zipDisk = (int)ecd.ThisDisk;
      Vols.ecd_wasRead = true;

      // if is not multivol or bad multivol, we return to main single stream code
      if (ecd.ThisDisk == 0
          || ecd.ThisDisk >= ((UInt32)1 << 30)
          || ecd.ThisDisk < ecd.CdDisk)
        return S_OK;
      
      cdDisk = (int)ecd.CdDisk;
      if (Vols.StartVolIndex < 0)
        Vols.StartVolIndex = (Int32)ecd.ThisDisk;
      else if ((UInt32)Vols.StartVolIndex >= ecd.ThisDisk)
        return S_OK;

      // Vols.StartVolIndex = ecd.ThisDisk;
      // Vols.EndVolIndex = ecd.ThisDisk;
      unsigned numMissingVols;
      if (cdDisk != zipDisk)
      {
        // get volumes required for cd.
        RINOK(ReadVols2(volCallback, (unsigned)cdDisk, zipDisk, zipDisk, 0, numMissingVols))
        if (numMissingVols != 0)
        {
          // cdOK = false;
        }
      }
    }
    else if (res != S_FALSE)
      return res;
  }

  if (Vols.StartVolIndex < 0)
  {
    // is not mutivol;
    return S_OK;
  }

  /*
  if (!Vols.Streams.IsEmpty())
    IsMultiVol = true;
  */
  
  unsigned numMissingVols;

  if (cdDisk != 0)
  {
    // get volumes that were no requested still
    const unsigned kNumMissingVolsMax = 1 << 12;
    RINOK(ReadVols2(volCallback, 0, cdDisk < 0 ? -1 : cdDisk, zipDisk, kNumMissingVolsMax, numMissingVols))
  }

  // if (Vols.StartVolIndex >= 0)
  {
    if (Vols.Streams.IsEmpty())
      if (Vols.StartVolIndex > (1 << 20))
        return S_OK;
    if ((unsigned)Vols.StartVolIndex >= Vols.Streams.Size()
        || !Vols.Streams[(unsigned)Vols.StartVolIndex].Stream)
    {
      // we get volumes starting from StartVolIndex, if they we not requested before know the volume index (if FindCd() was ok)
      RINOK(ReadVols2(volCallback, (unsigned)Vols.StartVolIndex, zipDisk, zipDisk, 0, numMissingVols))
    }
  }

  if (Vols.ZipStream)
  {
    // if there is no another volumes and volumeIndex is too big, we don't use multivol mode
    if (Vols.Streams.IsEmpty())
      if (zipDisk > (1 << 10))
        return S_OK;
    if (zipDisk >= 0)
    {
      // we create item in Streams for ZipStream, if we know the volume index (if FindCd() was ok)
      RINOK(ReadVols2(volCallback, (unsigned)zipDisk, zipDisk + 1, zipDisk, 0, numMissingVols))
    }
  }

  if (!Vols.Streams.IsEmpty())
  {
    IsMultiVol = true;
    /*
    if (cdDisk)
      IsMultiVol = true;
    */
    const int startZIndex = Vols.StartVolIndex;
    if (startZIndex >= 0)
    {
      // if all volumes before start volume are OK, we can start parsing from 0
      // if there are missing volumes before startZIndex, we start parsing in current startZIndex
      if ((unsigned)startZIndex < Vols.Streams.Size())
      {
        for (unsigned i = 0; i <= (unsigned)startZIndex; i++)
          if (!Vols.Streams[i].Stream)
          {
            Vols.StartParsingVol = startZIndex;
            break;
          }
      }
    }
  }

  return S_OK;
}



HRESULT CVols::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;

  for (;;)
  {
    if (StreamIndex < 0)
      return S_OK;
    if ((unsigned)StreamIndex >= Streams.Size())
      return S_OK;
    const CVols::CSubStreamInfo &s = Streams[(unsigned)StreamIndex];
    if (!s.Stream)
      return S_FALSE;
    if (NeedSeek)
    {
      RINOK(s.SeekToStart())
      NeedSeek = false;
    }
    UInt32 realProcessedSize = 0;
    HRESULT res = s.Stream->Read(data, size, &realProcessedSize);
    if (processedSize)
      *processedSize = realProcessedSize;
    if (res != S_OK)
      return res;
    if (realProcessedSize != 0)
      return res;
    StreamIndex++;
    NeedSeek = true;
  }
}

Z7_COM7F_IMF(CVolStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  return Vols->Read(data, size, processedSize);
}




#define COPY_ECD_ITEM_16(n) if (!isZip64 || !ZIP64_IS_16_MAX(ecd. n))     cdInfo. n = ecd. n;
#define COPY_ECD_ITEM_32(n) if (!isZip64 || !ZIP64_IS_32_MAX(ecd. n)) cdInfo. n = ecd. n;


HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items)
{
  // buffer that can be used for cd reading
  RINOK(AllocateBuffer(kSeqBufferSize))

  // here we can read small records. So we switch off _inBufMode.
  _inBufMode = false;

  HRESULT res = S_OK;

  bool localsWereRead = false;

  /* we try to open archive with the following modes:
     1) CD-MODE        : fast mode : we read backward ECD and CD, compare CD items with first Local item.
     2) LOCALS-CD-MODE : slow mode, if CD-MODE fails : we sequentially read all Locals and then CD.
     Then we read sequentially ECD64, Locator, ECD again at the end.

     - in LOCALS-CD-MODE we use use the following
         variables (with real cd properties) to set Base archive offset
         and check real cd properties with values from ECD/ECD64.
  */

  UInt64 cdSize = 0;
  UInt64 cdRelatOffset = 0;
  UInt32 cdDisk = 0;

  UInt64 cdAbsOffset = 0;   // absolute cd offset, for LOCALS-CD-MODE only.

if (Force_ReadLocals_Mode)
{
  IsArc = true;
  res = S_FALSE; // we will use LOCALS-CD-MODE mode
}
else
{
  if (!MarkerIsFound || !MarkerIsSafe)
  {
    IsArc = true;
    res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
    if (res == S_OK)
      ReadSignature();
    else if (res != S_FALSE)
      return res;
  }
  else  // (MarkerIsFound && MarkerIsSafe)
  {
 
  // _signature must be kLocalFileHeader or kEcd or kEcd64

  SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2 + 4);

  CanStartNewVol = false;

  if (_signature == NSignature::kEcd64)
  {
    // UInt64 ecd64Offset = GetVirtStreamPos() - 4;
    IsZip64 = true;

    {
      const UInt64 recordSize = ReadUInt64();
      if (recordSize < kEcd64_MainSize)
        return S_FALSE;
      if (recordSize >= ((UInt64)1 << 62))
        return S_FALSE;
      
      {
        const unsigned kBufSize = kEcd64_MainSize;
        Byte buf[kBufSize];
        SafeRead(buf, kBufSize);
        CCdInfo cdInfo;
        cdInfo.ParseEcd64e(buf);
        if (!cdInfo.IsEmptyArc())
          return S_FALSE;
      }
      
      RINOK(Skip64(recordSize - kEcd64_MainSize, 0))
    }

    ReadSignature();
    if (_signature != NSignature::kEcd64Locator)
      return S_FALSE;

    {
      const unsigned kBufSize = 16;
      Byte buf[kBufSize];
      SafeRead(buf, kBufSize);
      CLocator locator;
      locator.Parse(buf);
      if (!locator.IsEmptyArc())
        return S_FALSE;
    }

    ReadSignature();
    if (_signature != NSignature::kEcd)
      return S_FALSE;
  }
  
  if (_signature == NSignature::kEcd)
  {
    // It must be empty archive or backware archive
    // we don't support backware archive still
    
    const unsigned kBufSize = kEcdSize - 4;
    Byte buf[kBufSize];
    SafeRead(buf, kBufSize);
    CEcd ecd;
    ecd.Parse(buf);
    // if (ecd.cdSize != 0)
    // Do we need also to support the case where empty zip archive with PK00 uses cdOffset = 4 ??
    if (!ecd.IsEmptyArc())
      return S_FALSE;

    ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
    IsArc = true; // check it: we need more tests?

    RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2))
    ReadSignature();
  }
  else
  {
    CItemEx firstItem;
    try
    {
      try
      {
        if (!ReadLocalItem(firstItem))
          return S_FALSE;
      }
      catch(CUnexpectEnd &)
      {
        return S_FALSE;
      }

      IsArc = true;
      res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
      if (res == S_OK)
        ReadSignature();
    }
    catch(CUnexpectEnd &) { res = S_FALSE; }
    
    if (res != S_FALSE && res != S_OK)
      return res;

    if (res == S_OK && items.Size() == 0)
      res = S_FALSE;

    if (res == S_OK)
    {
      // we can't read local items here to keep _inBufMode state
      if ((Int64)ArcInfo.MarkerPos2 < ArcInfo.Base)
        res = S_FALSE;
      else
      {
        firstItem.LocalHeaderPos = (UInt64)((Int64)ArcInfo.MarkerPos2 - ArcInfo.Base);
        int index = -1;

        UInt32 min_Disk = (UInt32)(Int32)-1;
        UInt64 min_LocalHeaderPos = (UInt64)(Int64)-1;

        if (!IsCdUnsorted)
          index = FindItem(items, firstItem);
        else
        {
          FOR_VECTOR (i, items)
          {
            const CItemEx &cdItem = items[i];
            if (cdItem.Disk == firstItem.Disk
                && (cdItem.LocalHeaderPos == firstItem.LocalHeaderPos))
              index = (int)i;
            
            if (i == 0
                || cdItem.Disk < min_Disk
                || (cdItem.Disk == min_Disk && cdItem.LocalHeaderPos < min_LocalHeaderPos))
            {
              min_Disk = cdItem.Disk;
              min_LocalHeaderPos = cdItem.LocalHeaderPos;
            }
          }
        }

        if (index == -1)
          res = S_FALSE;
        else if (!AreItemsEqual(firstItem, items[(unsigned)index]))
          res = S_FALSE;
        else
        {
          ArcInfo.CdWasRead = true;
          if (IsCdUnsorted)
            ArcInfo.FirstItemRelatOffset = min_LocalHeaderPos;
          else
            ArcInfo.FirstItemRelatOffset = items[0].LocalHeaderPos;

          // ArcInfo.FirstItemRelatOffset = _startLocalFromCd_Offset;
        }
      }
    }
  }
  } // (MarkerIsFound && MarkerIsSafe)

} // (!onlyLocalsMode)


  CObjectVector<CItemEx> cdItems;

  bool needSetBase = false; // we set needSetBase only for LOCALS_CD_MODE
  unsigned numCdItems = items.Size();
  
  #ifdef ZIP_SELF_CHECK
  res = S_FALSE; // if uncommented, it uses additional LOCALS-CD-MODE mode to check the code
  #endif

  if (res != S_OK)
  {
    // ---------- LOCALS-CD-MODE ----------
    // CD doesn't match firstItem,
    // so we clear items and read Locals and CD.

    items.Clear();
    localsWereRead = true;
    
    HeadersError = false;
    HeadersWarning = false;
    ExtraMinorError = false;

    /* we can use any mode: with buffer and without buffer
         without buffer : skips packed data : fast for big files : slow for small files
         with    buffer : reads packed data : slow for big files : fast for small files
       Buffer mode is more effective. */
    // _inBufMode = false;
    _inBufMode = true;
    // we could change the buffer size here, if we want smaller Buffer.
    // RINOK(ReAllocateBuffer(kSeqBufferSize));
    // InitBuf()
    
    ArcInfo.Base = 0;

   if (!Disable_FindMarker)
   {
    if (!MarkerIsFound)
    {
      if (!IsMultiVol)
        return S_FALSE;
      if (Vols.StartParsingVol != 0)
        return S_FALSE;
      // if (StartParsingVol == 0) and we didn't find marker, we use default zero marker.
      // so we suppose that there is no sfx stub
      RINOK(SeekToVol(0, ArcInfo.MarkerPos2))
    }
    else
    {
      if (ArcInfo.MarkerPos != 0)
      {
        /*
        If multi-vol or there is (No)Span-marker at start of stream, we set (Base) as 0.
        In another caes:
          (No)Span-marker is supposed as false positive. So we set (Base) as main marker (MarkerPos2).
          The (Base) can be corrected later after ECD reading.
          But sfx volume with stub and (No)Span-marker in (!IsMultiVol) mode will have incorrect (Base) here.
        */
        ArcInfo.Base = (Int64)ArcInfo.MarkerPos2;
      }
      RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2))
    }
   }
    _cnt = 0;

    ReadSignature();
    
    LocalsWereRead = true;

    RINOK(ReadLocals(items))

    if (_signature != NSignature::kCentralFileHeader)
    {
      // GetVirtStreamPos() - 4
      if (items.IsEmpty())
        return S_FALSE;

      bool isError = true;

      const UInt32 apkSize = _signature;
      const unsigned kApkFooterSize = 16 + 8;
      if (apkSize >= kApkFooterSize && apkSize <= (1 << 20))
      {
        if (ReadUInt32() == 0)
        {
          CByteBuffer apk;
          apk.Alloc(apkSize);
          SafeRead(apk, apkSize);
          ReadSignature();
          const Byte *footer = apk + apkSize - kApkFooterSize;
          if (_signature == NSignature::kCentralFileHeader)
          if (GetUi64(footer) == apkSize)
          if (memcmp(footer + 8, "APK Sig Block 42", 16) == 0)
          {
            isError = false;
            IsApk = true;
          }
        }
      }
      
      if (isError)
      {
        NoCentralDir = true;
        HeadersError = true;
        return S_OK;
      }
    }
    
    _inBufMode = true;

    cdAbsOffset = GetVirtStreamPos() - 4;
    cdDisk = (UInt32)Vols.StreamIndex;

    #ifdef ZIP_SELF_CHECK
    if (!IsMultiVol && _cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
      return E_FAIL;
    #endif

    const UInt64 processedCnt_start = _cnt;

    for (;;)
    {
      CItemEx cdItem;
      
      RINOK(ReadCdItem(cdItem))
      
      cdItems.Add(cdItem);
      if (Callback && (cdItems.Size() & 0xFFF) == 0)
      {
        const UInt64 numFiles = items.Size();
        const UInt64 numBytes = _cnt;
        RINOK(Callback->SetCompleted(&numFiles, &numBytes))
      }
      ReadSignature();
      if (_signature != NSignature::kCentralFileHeader)
        break;
    }
    
    cdSize = _cnt - processedCnt_start;

    #ifdef ZIP_SELF_CHECK
    if (!IsMultiVol)
    {
      if (_cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
        return E_FAIL;
      if (cdSize != (GetVirtStreamPos() - 4) - cdAbsOffset)
        return E_FAIL;
    }
    #endif

    needSetBase = true;
    numCdItems = cdItems.Size();
    cdRelatOffset = (UInt64)((Int64)cdAbsOffset - ArcInfo.Base);

    if (!cdItems.IsEmpty())
    {
      ArcInfo.CdWasRead = true;
      ArcInfo.FirstItemRelatOffset = cdItems[0].LocalHeaderPos;
    }
  }

  
  
  CCdInfo cdInfo;
  CLocator locator;
  bool isZip64 = false;
  const UInt64 ecd64AbsOffset = GetVirtStreamPos() - 4;
  int ecd64Disk = -1;
  
  if (_signature == NSignature::kEcd64)
  {
    ecd64Disk = Vols.StreamIndex;

    IsZip64 = isZip64 = true;

    {
      const UInt64 recordSize = ReadUInt64();
      if (recordSize < kEcd64_MainSize
          || recordSize >= ((UInt64)1 << 62))
      {
        HeadersError = true;
        return S_OK;
      }

      {
        const unsigned kBufSize = kEcd64_MainSize;
        Byte buf[kBufSize];
        SafeRead(buf, kBufSize);
        cdInfo.ParseEcd64e(buf);
      }
      
      RINOK(Skip64(recordSize - kEcd64_MainSize, items.Size()))
    }


    ReadSignature();

    if (_signature != NSignature::kEcd64Locator)
    {
      HeadersError = true;
      return S_OK;
    }
  
    {
      const unsigned kBufSize = 16;
      Byte buf[kBufSize];
      SafeRead(buf, kBufSize);
      locator.Parse(buf);
      // we ignore the error, where some zip creators use (NumDisks == 0)
      // if (locator.NumDisks == 0) HeadersWarning = true;
    }

    ReadSignature();
  }
  
  
  if (_signature != NSignature::kEcd)
  {
    HeadersError = true;
    return S_OK;
  }

  
  CanStartNewVol = false;

  // ---------- ECD ----------

  CEcd ecd;
  {
    const unsigned kBufSize = kEcdSize - 4;
    Byte buf[kBufSize];
    SafeRead(buf, kBufSize);
    ecd.Parse(buf);
  }

  COPY_ECD_ITEM_16(ThisDisk)
  COPY_ECD_ITEM_16(CdDisk)
  COPY_ECD_ITEM_16(NumEntries_in_ThisDisk)
  COPY_ECD_ITEM_16(NumEntries)
  COPY_ECD_ITEM_32(Size)
  COPY_ECD_ITEM_32(Offset)

  bool cdOK = true;

  if ((UInt32)cdInfo.Size != (UInt32)cdSize)
  {
    // return S_FALSE;
    cdOK = false;
  }

  if (isZip64)
  {
    if (cdInfo.NumEntries != numCdItems
        || cdInfo.Size != cdSize)
    {
      cdOK = false;
    }
  }


  if (IsMultiVol)
  {
    if (cdDisk != cdInfo.CdDisk)
      HeadersError = true;
  }
  else if (needSetBase && cdOK)
  {
    const UInt64 oldBase = (UInt64)ArcInfo.Base;
    // localsWereRead == true
    // ArcInfo.Base == ArcInfo.MarkerPos2
    // cdRelatOffset == (cdAbsOffset - ArcInfo.Base)

    if (isZip64)
    {
      if (ecd64Disk == Vols.StartVolIndex)
      {
        const Int64 newBase = (Int64)ecd64AbsOffset - (Int64)locator.Ecd64Offset;
        if (newBase <= (Int64)ecd64AbsOffset)
        {
          if (!localsWereRead || newBase <= (Int64)ArcInfo.MarkerPos2)
          {
            ArcInfo.Base = newBase;
            cdRelatOffset = (UInt64)((Int64)cdAbsOffset - newBase);
          }
          else
            cdOK = false;
        }
      }
    }
    else if (numCdItems != 0) // we can't use ecd.Offset in empty archive?
    {
      if ((int)cdDisk == Vols.StartVolIndex)
      {
        const Int64 newBase = (Int64)cdAbsOffset - (Int64)cdInfo.Offset;
        if (newBase <= (Int64)cdAbsOffset)
        {
          if (!localsWereRead || newBase <= (Int64)ArcInfo.MarkerPos2)
          {
            // cd can be more accurate, when it points before Locals
            // so we change Base and cdRelatOffset
            ArcInfo.Base = newBase;
            cdRelatOffset = cdInfo.Offset;
          }
          else
          {
            // const UInt64 delta = ((UInt64)cdRelatOffset - cdInfo.Offset);
            const UInt64 delta = ((UInt64)(newBase - ArcInfo.Base));
            if ((UInt32)delta == 0)
            {
              // we set Overflow32bit mode, only if there is (x<<32) offset
              // between real_CD_offset_from_MarkerPos and CD_Offset_in_ECD.
              // Base and cdRelatOffset unchanged
              Overflow32bit = true;
            }
            else
              cdOK = false;
          }
        }
        else
          cdOK = false;
      }
    }
    // cdRelatOffset = cdAbsOffset - ArcInfo.Base;

    if (localsWereRead)
    {
      const UInt64 delta = (UInt64)((Int64)oldBase - ArcInfo.Base);
      if (delta != 0)
      {
        FOR_VECTOR (i, items)
          items[i].LocalHeaderPos += delta;
      }
    }
  }

  if (!cdOK)
    HeadersError = true;

  EcdVolIndex = cdInfo.ThisDisk;

  if (!IsMultiVol)
  {
    if (EcdVolIndex == 0 && Vols.MissingZip && Vols.StartIsExe)
    {
      Vols.MissingName.Empty();
      Vols.MissingZip = false;
    }

    if (localsWereRead)
    {
      if (EcdVolIndex != 0)
      {
        FOR_VECTOR (i, items)
          items[i].Disk = EcdVolIndex;
      }
    }

    UseDisk_in_SingleVol = true;
  }

  if (isZip64)
  {
    if ((cdInfo.ThisDisk == 0 && ecd64AbsOffset != (UInt64)(ArcInfo.Base + (Int64)locator.Ecd64Offset))
        // || cdInfo.NumEntries_in_ThisDisk != numCdItems
        || cdInfo.NumEntries != numCdItems
        || cdInfo.Size != cdSize
        || (cdInfo.Offset != cdRelatOffset && !items.IsEmpty()))
    {
      HeadersError = true;
      return S_OK;
    }
  }

  if (cdOK && !cdItems.IsEmpty())
  {
    // ---------- merge Central Directory Items ----------
  
    CRecordVector<unsigned> items2;

    int nextLocalIndex = 0;

    LocalsCenterMerged = true;

    FOR_VECTOR (i, cdItems)
    {
      if (Callback)
      if ((i & 0x3FFF) == 0)
      {
        const UInt64 numFiles64 = items.Size() + items2.Size();
        RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
      }

      const CItemEx &cdItem = cdItems[i];
      
      int index = -1;
      
      if (nextLocalIndex != -1)
      {
        if ((unsigned)nextLocalIndex < items.Size())
        {
          CItemEx &item = items[(unsigned)nextLocalIndex];
          if (item.Disk == cdItem.Disk &&
              (item.LocalHeaderPos == cdItem.LocalHeaderPos
              || (Overflow32bit && (UInt32)item.LocalHeaderPos == cdItem.LocalHeaderPos)))
            index = nextLocalIndex++;
          else
            nextLocalIndex = -1;
        }
      }

      if (index == -1)
        index = FindItem(items, cdItem);

      // index = -1;

      if (index == -1)
      {
        items2.Add(i);
        HeadersError = true;
        continue;
      }

      CItemEx &item = items[(unsigned)index];
      if (item.Name != cdItem.Name
          // || item.Name.Len() != cdItem.Name.Len()
          || item.PackSize != cdItem.PackSize
          || item.Size != cdItem.Size
          // item.ExtractVersion != cdItem.ExtractVersion
          || !FlagsAreSame(item, cdItem)
          || item.Crc != cdItem.Crc)
      {
        HeadersError = true;
        continue;
      }

      // item.Name = cdItem.Name;
      item.MadeByVersion = cdItem.MadeByVersion;
      item.CentralExtra = cdItem.CentralExtra;
      item.InternalAttrib = cdItem.InternalAttrib;
      item.ExternalAttrib = cdItem.ExternalAttrib;
      item.Comment = cdItem.Comment;
      item.FromCentral = cdItem.FromCentral;
      // 22.02: we force utf8 flag, if central header has utf8 flag
      if (cdItem.Flags & NFileHeader::NFlags::kUtf8)
        item.Flags |= NFileHeader::NFlags::kUtf8;
    }

    FOR_VECTOR (k, items2)
      items.Add(cdItems[items2[k]]);
  }

  if (ecd.NumEntries < ecd.NumEntries_in_ThisDisk)
    HeadersError = true;

  if (ecd.ThisDisk == 0)
  {
    // if (isZip64)
    {
      if (ecd.NumEntries != ecd.NumEntries_in_ThisDisk)
        HeadersError = true;
    }
  }

  if (isZip64)
  {
    if (cdInfo.NumEntries != items.Size()
        || (ecd.NumEntries != items.Size() && ecd.NumEntries != 0xFFFF))
      HeadersError = true;
  }
  else
  {
    // old 7-zip could store 32-bit number of CD items to 16-bit field.
    // if (ecd.NumEntries != items.Size())
    if (ecd.NumEntries > items.Size())
      HeadersError = true;

    if (cdInfo.NumEntries != numCdItems)
    {
      if ((UInt16)cdInfo.NumEntries != (UInt16)numCdItems)
        HeadersError = true;
      else
        Cd_NumEntries_Overflow_16bit = true;
    }
  }

  ReadBuffer(ArcInfo.Comment, ecd.CommentSize);

  _inBufMode = false;

  // DisableBufMode();
  // Buffer.Free();
  /* we can't clear buf varibles. we need them to calculate PhySize of archive */

  if ((UInt16)cdInfo.NumEntries != (UInt16)numCdItems
      || (UInt32)cdInfo.Size != (UInt32)cdSize
      || ((UInt32)cdInfo.Offset != (UInt32)cdRelatOffset && !items.IsEmpty()))
  {
    // return S_FALSE;
    HeadersError = true;
  }

  #ifdef ZIP_SELF_CHECK
  if (localsWereRead)
  {
    const UInt64 endPos = ArcInfo.MarkerPos2 + _cnt;
    if (endPos != (IsMultiVol ? Vols.TotalBytesSize : ArcInfo.FileEndPos))
    {
      // there are some data after the end of archive or error in code;
      return E_FAIL;
    }
  }
  #endif
       
  // printf("\nOpen OK");
  return S_OK;
}



HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchLimit,
    IArchiveOpenCallback *callback, CObjectVector<CItemEx> &items)
{
  items.Clear();
  
  Close();

  UInt64 startPos;
  RINOK(InStream_GetPos(stream, startPos))
  RINOK(InStream_GetSize_SeekToEnd(stream, ArcInfo.FileEndPos))
  _streamPos = ArcInfo.FileEndPos;

  StartStream = stream;
  Stream = stream;
  Callback = callback;

  DisableBufMode();
  
  bool volWasRequested = false;

  if (!Disable_VolsRead)
  if (callback
      && (startPos == 0 || !searchLimit || *searchLimit != 0))
  {
    // we try to read volumes only if it's first call (offset == 0) or scan is allowed.
    volWasRequested = true;
    RINOK(ReadVols())
  }

  if (Disable_FindMarker)
  {
    RINOK(SeekToVol(-1, startPos))
    StreamRef = stream;
    Stream = stream;
    MarkerIsFound = true;
    MarkerIsSafe = true;
    ArcInfo.MarkerPos = startPos;
    ArcInfo.MarkerPos2 = startPos;
  }
  else
  if (IsMultiVol && Vols.StartParsingVol == 0 && (unsigned)Vols.StartParsingVol < Vols.Streams.Size())
  {
    // only StartParsingVol = 0 is safe search.
    RINOK(SeekToVol(0, 0))
    // if (Stream)
    {
      // UInt64 limit = 1 << 22; // for sfx
      UInt64 limit = 0; // without sfx
    
      HRESULT res = FindMarker(&limit);
      
      if (res == S_OK)
      {
        MarkerIsFound = true;
        MarkerIsSafe = true;
      }
      else if (res != S_FALSE)
        return res;
    }
  }
  else
  {
    // printf("\nOpen offset = %u\n", (unsigned)startPos);
    if (IsMultiVol
        && (unsigned)Vols.StartParsingVol < Vols.Streams.Size()
        && Vols.Streams[(unsigned)Vols.StartParsingVol].Stream)
    {
      RINOK(SeekToVol(Vols.StartParsingVol, Vols.StreamIndex == Vols.StartVolIndex ? startPos : 0))
    }
    else
    {
      RINOK(SeekToVol(-1, startPos))
    }
    
    // UInt64 limit = 1 << 22;
    // HRESULT res = FindMarker(&limit);

    HRESULT res = FindMarker(searchLimit);
    
    // const UInt64 curPos = GetVirtStreamPos();
    const UInt64 curPos = ArcInfo.MarkerPos2 + 4;

    if (res == S_OK)
      MarkerIsFound = true;
    else if (!IsMultiVol)
    {
      /*
      // if (startPos != 0), probably CD could be already tested with another call with (startPos == 0).
      // so we don't want to try to open CD again in that case.
      if (startPos != 0)
        return res;
      // we can try to open CD, if there is no Marker and (startPos == 0).
      // is it OK to open such files as ZIP, or big number of false positive, when CD can be find in end of file ?
      */
      return res;
    }
    
    if (ArcInfo.IsSpanMode && !volWasRequested)
    {
      RINOK(ReadVols())
      if (IsMultiVol && MarkerIsFound && ArcInfo.MarkerVolIndex < 0)
        ArcInfo.MarkerVolIndex = Vols.StartVolIndex;
    }

    MarkerIsSafe = !IsMultiVol
        || (ArcInfo.MarkerVolIndex == 0 && ArcInfo.MarkerPos == 0)
        ;
    

    if (IsMultiVol)
    {
      if ((unsigned)Vols.StartVolIndex < Vols.Streams.Size())
      {
        Stream = Vols.Streams[(unsigned)Vols.StartVolIndex].Stream;
        if (Stream)
        {
          RINOK(Seek_SavePos(curPos))
        }
        else
          IsMultiVol = false;
      }
      else
        IsMultiVol = false;
    }

    if (!IsMultiVol)
    {
      if (Vols.StreamIndex != -1)
      {
        Stream = StartStream;
        Vols.StreamIndex = -1;
        InitBuf();
        RINOK(Seek_SavePos(curPos))
      }

      ArcInfo.MarkerVolIndex = -1;
      StreamRef = stream;
      Stream = stream;
    }
  }


  if (!IsMultiVol)
    Vols.ClearRefs();

  {
    HRESULT res;
    try
    {
      res = ReadHeaders(items);
    }
    catch (const CSystemException &e) { res = e.ErrorCode; }
    catch (const CUnexpectEnd &)
    {
      if (items.IsEmpty())
        return S_FALSE;
      UnexpectedEnd = true;
      res = S_OK;
    }
    catch (...)
    {
      DisableBufMode();
      throw;
    }
    
    if (IsMultiVol)
    {
      ArcInfo.FinishPos = ArcInfo.FileEndPos;
      if ((unsigned)Vols.StreamIndex < Vols.Streams.Size())
        if (GetVirtStreamPos() < Vols.Streams[(unsigned)Vols.StreamIndex].Size)
          ArcInfo.ThereIsTail = true;
    }
    else
    {
      ArcInfo.FinishPos = GetVirtStreamPos();
      ArcInfo.ThereIsTail = (ArcInfo.FileEndPos > ArcInfo.FinishPos);
    }

    DisableBufMode();

    IsArcOpen = true;
    if (!IsMultiVol)
      Vols.Streams.Clear();
    return res;
  }
}


HRESULT CInArchive::GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr<ISequentialInStream> &stream)
{
  stream.Release();

  UInt64 pos = item.LocalHeaderPos;
  if (seekPackData)
    pos += item.LocalFullHeaderSize;

  if (!IsMultiVol)
  {
    if (UseDisk_in_SingleVol && item.Disk != EcdVolIndex)
      return S_OK;
    pos = (UInt64)((Int64)pos + ArcInfo.Base);
    RINOK(InStream_SeekSet(StreamRef, pos))
    stream = StreamRef;
    return S_OK;
  }

  if (item.Disk >= Vols.Streams.Size())
    return S_OK;
    
  IInStream *str2 = Vols.Streams[item.Disk].Stream;
  if (!str2)
    return S_OK;
  RINOK(InStream_SeekSet(str2, pos))
    
  Vols.NeedSeek = false;
  Vols.StreamIndex = (int)item.Disk;
    
  CVolStream *volsStreamSpec = new CVolStream;
  volsStreamSpec->Vols = &Vols;
  stream = volsStreamSpec;
  
  return S_OK;
}

}}
