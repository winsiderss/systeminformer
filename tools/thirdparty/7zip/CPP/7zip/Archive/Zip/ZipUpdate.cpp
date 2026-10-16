// ZipUpdate.cpp

#include "StdAfx.h"

// #define DEBUG_CACHE

#ifdef DEBUG_CACHE
#include <stdio.h>
  #define PRF(x) x
#else
  #define PRF(x)
#endif

#include "../../../../C/Alloc.h"

#include "../../../Common/AutoPtr.h"
#include "../../../Common/Defs.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/TimeUtils.h"
#include "../../../Windows/Thread.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/OutMemStream.h"
#include "../../Common/ProgressUtils.h"
#ifndef Z7_ST
#include "../../Common/ProgressMt.h"
#endif
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"
// #include "../../Compress/ZstdEncoderProps.h"

#include "ZipAddCommon.h"
#include "ZipOut.h"
#include "ZipUpdate.h"

using namespace NWindows;
using namespace NSynchronization;

namespace NArchive {
namespace NZip {

static const Byte kHostOS =
  #ifdef _WIN32
  NFileHeader::NHostOS::kFAT;
  #else
  NFileHeader::NHostOS::kUnix;
  #endif

static const Byte kMadeByHostOS = kHostOS;

// 18.06: now we always write zero to high byte of ExtractVersion field.
// Previous versions of p7zip wrote (NFileHeader::NHostOS::kUnix) there, that is not correct
static const Byte kExtractHostOS = 0;

static const Byte kMethodForDirectory = NFileHeader::NCompressionMethod::kStore;


static void AddAesExtra(CItem &item, Byte aesKeyMode, UInt16 method)
{
  CWzAesExtra wzAesField;
  wzAesField.Strength = aesKeyMode;
  wzAesField.Method = method;
  item.Method = NFileHeader::NCompressionMethod::kWzAES;
  item.Crc = 0;
  CExtraSubBlock sb;
  wzAesField.SetSubBlock(sb);
  item.LocalExtra.SubBlocks.Add(sb);
  item.CentralExtra.SubBlocks.Add(sb);
}


static void Copy_From_UpdateItem_To_ItemOut(const CUpdateItem &ui, CItemOut &item)
{
  item.Name = ui.Name;
  item.Name_Utf = ui.Name_Utf;
  item.Comment = ui.Comment;
  item.SetUtf8(ui.IsUtf8);
  // item.SetFlag_AltStream(ui.IsAltStream);
  // item.ExternalAttrib = ui.Attrib;
  item.Time = ui.Time;
  item.Ntfs_MTime = ui.Ntfs_MTime;
  item.Ntfs_ATime = ui.Ntfs_ATime;
  item.Ntfs_CTime = ui.Ntfs_CTime;

  item.Write_UnixTime = ui.Write_UnixTime;
  item.Write_NtfsTime = ui.Write_NtfsTime;
}

static void SetFileHeader(
    const CCompressionMethodMode &options,
    const CUpdateItem &ui,
    bool useDescriptor,
    CItemOut &item)
{
  item.Size = ui.Size;
  const bool isDir = ui.IsDir;

  item.ClearFlags();

  if (ui.NewProps)
  {
    Copy_From_UpdateItem_To_ItemOut(ui, item);
    // item.SetFlag_AltStream(ui.IsAltStream);
    item.ExternalAttrib = ui.Attrib;
  }
  /*
  else
    isDir = item.IsDir();
  */

  item.MadeByVersion.HostOS = kMadeByHostOS;
  item.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  item.ExtractVersion.HostOS = kExtractHostOS;

  item.InternalAttrib = 0; // test it
  item.SetEncrypted(!isDir && options.Password_Defined);
  item.SetDescriptorMode(useDescriptor);

  if (isDir)
  {
    item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
    item.Method = kMethodForDirectory;
    item.PackSize = 0;
    item.Size = 0;
    item.Crc = 0;
  }

  item.LocalExtra.Clear();
  item.CentralExtra.Clear();

  if (isDir)
  {
    item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
    item.Method = kMethodForDirectory;
    item.PackSize = 0;
    item.Size = 0;
    item.Crc = 0;
  }
  else if (options.IsRealAesMode())
    AddAesExtra(item, options.AesKeyMode, (Byte)(options.MethodSequence.IsEmpty() ? 8 : options.MethodSequence[0]));
}


// we call SetItemInfoFromCompressingResult() after SetFileHeader()

static void SetItemInfoFromCompressingResult(const CCompressingResult &compressingResult,
    bool isAesMode, Byte aesKeyMode, CItem &item)
{
  item.ExtractVersion.Version = compressingResult.ExtractVersion;
  item.Method = compressingResult.Method;
  if (compressingResult.Method == NFileHeader::NCompressionMethod::kLZMA && compressingResult.LzmaEos)
    item.Flags |= NFileHeader::NFlags::kLzmaEOS;
  item.Crc = compressingResult.CRC;
  item.Size = compressingResult.UnpackSize;
  item.PackSize = compressingResult.PackSize;

  item.LocalExtra.Clear();
  item.CentralExtra.Clear();

  if (isAesMode)
    AddAesExtra(item, aesKeyMode, compressingResult.Method);
}


#ifndef Z7_ST

struct CMtSem
{
  NWindows::NSynchronization::CSemaphore Semaphore;
  NWindows::NSynchronization::CCriticalSection CS;
  CIntVector Indexes;
  int Head;

  void ReleaseItem(unsigned index)
  {
    {
      CCriticalSectionLock lock(CS);
      Indexes[index] = Head;
      Head = (int)index;
    }
    Semaphore.Release();
  }

  int GetFreeItem()
  {
    int i;
    {
      CCriticalSectionLock lock(CS);
      i = Head;
      Head = Indexes[(unsigned)i];
    }
    return i;
  }
};

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo);

struct CThreadInfo
{
  DECL_EXTERNAL_CODECS_LOC_VARS_DECL

  NWindows::CThread Thread;
  NWindows::NSynchronization::CAutoResetEvent CompressEvent;
  CMtSem *MtSem;
  unsigned ThreadIndex;

  bool ExitThread;

  CMtCompressProgress *ProgressSpec;
  CMyComPtr<ICompressProgressInfo> Progress;

  COutMemStream *OutStreamSpec;
  CMyComPtr<IOutStream> OutStream;
  CMyComPtr<ISequentialInStream> InStream;

  CAddCommon Coder;
  HRESULT Result;
  CCompressingResult CompressingResult;

  bool IsFree;
  bool InSeqMode;
  bool OutSeqMode;
  bool ExpectedDataSize_IsConfirmed;

  UInt32 UpdateIndex;
  UInt32 FileTime;
  UInt64 ExpectedDataSize;

  CThreadInfo():
      MtSem(NULL),
      ExitThread(false),
      ProgressSpec(NULL),
      OutStreamSpec(NULL),
      IsFree(true),
      InSeqMode(false),
      OutSeqMode(false),
      ExpectedDataSize_IsConfirmed(false),
      FileTime(0),
      ExpectedDataSize((UInt64)(Int64)-1)
  {}

  void SetOptions(const CCompressionMethodMode &options)
  {
    Coder.SetOptions(options);
  }
  
  HRESULT CreateEvents()
  {
    const WRes wres = CompressEvent.CreateIfNotCreated_Reset();
    return HRESULT_FROM_WIN32(wres);
  }
  
  // (group < 0) means no_group.
  HRESULT CreateThread_with_group(
#ifdef _WIN32
      int group
#endif
      )
  {
    // tested in win10: If thread is created by another thread,
    // child thread probably uses same group as parent thread.
    // So we don't need to send (group) to encoder in created thread.
    const WRes wres =
#ifdef _WIN32
      group >= 0 ?
        Thread.Create_With_Group(CoderThread, this, (unsigned)group) :
#endif
        Thread.Create(CoderThread, this);
    return HRESULT_FROM_WIN32(wres);
  }

  void WaitAndCode();

  void StopWait_Close()
  {
    ExitThread = true;
    if (OutStreamSpec)
      OutStreamSpec->StopWriting(E_ABORT);
    if (CompressEvent.IsCreated())
      CompressEvent.Set();
    Thread.Wait_Close();
  }
};

void CThreadInfo::WaitAndCode()
{
  for (;;)
  {
    CompressEvent.Lock();
    if (ExitThread)
      return;

    Result = Coder.Compress(
        EXTERNAL_CODECS_LOC_VARS
        InStream, OutStream,
        InSeqMode, OutSeqMode, FileTime, ExpectedDataSize,
        ExpectedDataSize_IsConfirmed,
        Progress, CompressingResult);
    
    if (Result == S_OK && Progress)
      Result = Progress->SetRatioInfo(&CompressingResult.UnpackSize, &CompressingResult.PackSize);
    
    MtSem->ReleaseItem(ThreadIndex);
  }
}

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo)
{
  ((CThreadInfo *)threadCoderInfo)->WaitAndCode();
  return 0;
}

class CThreads
{
public:
  CObjectVector<CThreadInfo> Threads;
  ~CThreads()
  {
    FOR_VECTOR (i, Threads)
      Threads[i].StopWait_Close();
  }
};

struct CMemBlocks2: public CMemLockBlocks
{
  bool Skip;
  bool InSeqMode;
  bool PreDescriptorMode;
  bool Finished;
  CCompressingResult CompressingResult;
  
  CMemBlocks2(): Skip(false), InSeqMode(false), PreDescriptorMode(false), Finished(false),
    CompressingResult() {}
};

class CMemRefs
{
public:
  CMemBlockManagerMt *Manager;
  CObjectVector<CMemBlocks2> Refs;
  CMemRefs(CMemBlockManagerMt *manager): Manager(manager) {}
  ~CMemRefs()
  {
    FOR_VECTOR (i, Refs)
      Refs[i].FreeOpt(Manager);
  }
};


Z7_CLASS_IMP_NOQIB_1(
  CMtProgressMixer2
  , ICompressProgressInfo
)
  UInt64 ProgressOffset;
  UInt64 InSizes[2];
  UInt64 OutSizes[2];
  CMyComPtr<IProgress> Progress;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  bool _inSizeIsMain;
public:
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  void Create(IProgress *progress, bool inSizeIsMain);
  void SetProgressOffset(UInt64 progressOffset);
  void SetProgressOffset_NoLock(UInt64 progressOffset);
  HRESULT SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize);
};

void CMtProgressMixer2::Create(IProgress *progress, bool inSizeIsMain)
{
  Progress = progress;
  Progress.QueryInterface(IID_ICompressProgressInfo, &RatioProgress);
  _inSizeIsMain = inSizeIsMain;
  ProgressOffset = InSizes[0] = InSizes[1] = OutSizes[0] = OutSizes[1] = 0;
}

void CMtProgressMixer2::SetProgressOffset_NoLock(UInt64 progressOffset)
{
  InSizes[1] = OutSizes[1] = 0;
  ProgressOffset = progressOffset;
}

void CMtProgressMixer2::SetProgressOffset(UInt64 progressOffset)
{
  CriticalSection.Enter();
  SetProgressOffset_NoLock(progressOffset);
  CriticalSection.Leave();
}

HRESULT CMtProgressMixer2::SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  if (index == 0 && RatioProgress)
  {
    RINOK(RatioProgress->SetRatioInfo(inSize, outSize))
  }
  if (inSize)
    InSizes[index] = *inSize;
  if (outSize)
    OutSizes[index] = *outSize;
  UInt64 v = ProgressOffset + (_inSizeIsMain  ?
      (InSizes[0] + InSizes[1]) :
      (OutSizes[0] + OutSizes[1]));
  return Progress->SetCompleted(&v);
}

Z7_COM7F_IMF(CMtProgressMixer2::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return SetRatioInfo(0, inSize, outSize);
}


Z7_CLASS_IMP_NOQIB_1(
  CMtProgressMixer
  , ICompressProgressInfo
)
public:
  CMtProgressMixer2 *Mixer2;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  void Create(IProgress *progress, bool inSizeIsMain);
};

void CMtProgressMixer::Create(IProgress *progress, bool inSizeIsMain)
{
  Mixer2 = new CMtProgressMixer2;
  RatioProgress = Mixer2;
  Mixer2->Create(progress, inSizeIsMain);
}

Z7_COM7F_IMF(CMtProgressMixer::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return Mixer2->SetRatioInfo(1, inSize, outSize);
}


#endif

static HRESULT UpdateItemOldData(
    COutArchive &archive,
    CInArchive *inArchive,
    const CItemEx &itemEx,
    const CUpdateItem &ui,
    CItemOut &item,
    /* bool izZip64, */
    ICompressProgressInfo *progress,
    IArchiveUpdateCallbackFile *opCallback,
    UInt64 &complexity)
{
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, (UInt32)ui.IndexInArc,
        NUpdateNotifyOp::kReplicate))
  }

  UInt64 rangeSize;

  RINOK(archive.ClearRestriction())

  if (ui.NewProps)
  {
    if (item.HasDescriptor())
    {
      // we know compressed / uncompressed sizes and crc.
      // so we remove descriptor here
      item.Flags = (UInt16)(item.Flags & ~NFileHeader::NFlags::kDescriptorUsedMask);
      // return E_NOTIMPL;
    }
    // we keep ExternalAttrib and some another properties from old archive
    // item.ExternalAttrib = ui.Attrib;
    // if we don't change Comment, we keep Comment from OldProperties
    Copy_From_UpdateItem_To_ItemOut(ui, item);
    // item.SetFlag_AltStream(ui.IsAltStream);

    item.CentralExtra.RemoveUnknownSubBlocks();
    item.LocalExtra.RemoveUnknownSubBlocks();

    archive.WriteLocalHeader(item);
    rangeSize = item.GetPackSizeWithDescriptor();
  }
  else
  {
    item.LocalHeaderPos = archive.GetCurPos();
    rangeSize = itemEx.GetLocalFullSize();
  }

  CMyComPtr<ISequentialInStream> packStream;

  RINOK(inArchive->GetItemStream(itemEx, ui.NewProps, packStream))
  if (!packStream)
    return E_NOTIMPL;

  complexity += rangeSize;

  CMyComPtr<ISequentialOutStream> outStream;
  archive.CreateStreamForCopying(outStream);
  HRESULT res = NCompress::CopyStream_ExactSize(packStream, outStream, rangeSize, progress);
  archive.MoveCurPos(rangeSize);
  return res;
}


static HRESULT WriteDirHeader(COutArchive &archive, const CCompressionMethodMode *options,
    const CUpdateItem &ui, CItemOut &item)
{
  SetFileHeader(*options, ui, false, item);
  RINOK(archive.ClearRestriction())
  archive.WriteLocalHeader(item);
  return S_OK;
}


static void UpdatePropsFromStream(
    const CUpdateOptions &options,
    CUpdateItem &item, ISequentialInStream *fileInStream,
    IArchiveUpdateCallback *updateCallback, UInt64 &totalComplexity)
{
  CMyComPtr<IStreamGetProps> getProps;
  fileInStream->QueryInterface(IID_IStreamGetProps, (void **)&getProps);
  UInt64 size = (UInt64)(Int64)-1;
  bool size_WasSet = false;
  
  if (getProps)
  {
    FILETIME cTime, aTime, mTime;
    UInt32 attrib;
    if (getProps->GetProps(&size, &cTime, &aTime, &mTime, &attrib) == S_OK)
    {
      if (options.Write_MTime)
        if (!FILETIME_IsZero(mTime))
        {
          item.Ntfs_MTime = mTime;
          NTime::UtcFileTime_To_LocalDosTime(mTime, item.Time);
        }
        
      if (options.Write_CTime) if (!FILETIME_IsZero(cTime)) item.Ntfs_CTime = cTime;
      if (options.Write_ATime) if (!FILETIME_IsZero(aTime)) item.Ntfs_ATime = aTime;
        
      item.Attrib = attrib;
      size_WasSet = true;
    }
  }

  if (!size_WasSet)
  {
    CMyComPtr<IStreamGetSize> streamGetSize;
    fileInStream->QueryInterface(IID_IStreamGetSize, (void **)&streamGetSize);
    if (streamGetSize)
    {
      if (streamGetSize->GetSize(&size) == S_OK)
        size_WasSet = true;
    }
  }

  if (size_WasSet && size != (UInt64)(Int64)-1)
  {
    item.Size_WasSetFromStream = true;
    if (size != item.Size)
    {
      const Int64 newComplexity = (Int64)totalComplexity + ((Int64)size - (Int64)item.Size);
      if (newComplexity > 0)
      {
        totalComplexity = (UInt64)newComplexity;
        updateCallback->SetTotal(totalComplexity);
      }
      item.Size = size;
    }
  }
}


/*
static HRESULT ReportProps(
    IArchiveUpdateCallbackArcProp *reportArcProp,
    UInt32 index,
    const CItemOut &item,
    bool isAesMode)
{
  PROPVARIANT prop;
  prop.vt = VT_EMPTY;
  prop.wReserved1 = 0;
  
  NCOM::PropVarEm_Set_UInt64(&prop, item.Size);
  RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidSize, &prop));
  
  NCOM::PropVarEm_Set_UInt64(&prop, item.PackSize);
  RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidPackSize, &prop));

  if (!isAesMode)
  {
    NCOM::PropVarEm_Set_UInt32(&prop, item.Crc);
    RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidCRC, &prop));
  }

  RINOK(reportArcProp->ReportFinished(NEventIndexType::kOutArcIndex, index, NUpdate::NOperationResult::kOK));

  // if (opCallback) RINOK(opCallback->ReportOperation(NEventIndexType::kOutArcIndex, index, NUpdateNotifyOp::kOpFinished))

  return S_OK;
}
*/

/*
struct CTotalStats
{
  UInt64 Size;
  UInt64 PackSize;

  void UpdateWithItem(const CItemOut &item)
  {
    Size += item.Size;
    PackSize += item.PackSize;
  }
};

static HRESULT ReportArcProps(IArchiveUpdateCallbackArcProp *reportArcProp,
    CTotalStats &st)
{
  PROPVARIANT prop;
  prop.vt = VT_EMPTY;
  prop.wReserved1 = 0;
  {
    NWindows::NCOM::PropVarEm_Set_UInt64(&prop, st.Size);
    RINOK(reportArcProp->ReportProp(
      NEventIndexType::kArcProp, 0, kpidSize, &prop));
  }
  {
    NWindows::NCOM::PropVarEm_Set_UInt64(&prop, st.PackSize);
    RINOK(reportArcProp->ReportProp(
      NEventIndexType::kArcProp, 0, kpidPackSize, &prop));
  }
  return S_OK;
}
*/


static HRESULT Update2St(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive,
    CInArchive *inArchive,
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode *options, bool outSeqMode,
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback,
    UInt64 &totalComplexity,
    IArchiveUpdateCallbackFile *opCallback
    // , IArchiveUpdateCallbackArcProp *reportArcProp
    )
{
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  CAddCommon compressor;
  compressor.SetOptions(*options);
  
  CObjectVector<CItemOut> items;
  UInt64 unpackSizeTotal = 0, packSizeTotal = 0;

  FOR_VECTOR (itemIndex, updateItems)
  {
    lps->InSize = unpackSizeTotal;
    lps->OutSize = packSizeTotal;
    RINOK(lps->SetCur())
    CUpdateItem &ui = updateItems[itemIndex];
    CItemEx itemEx;
    CItemOut item;

    if (!ui.NewProps || !ui.NewData)
    {
      // Note: for (ui.NewProps && !ui.NewData) it copies Props from old archive,
      // But we will rewrite all important properties later. But we can keep some properties like Comment
      itemEx = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
        return E_NOTIMPL;
      (CItem &)item = itemEx;
    }

    if (ui.NewData)
    {
      // bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
      bool isDir = ui.IsDir;
      if (isDir)
      {
        RINOK(WriteDirHeader(archive, options, ui, item))
      }
      else
      {
       CMyComPtr<ISequentialInStream> fileInStream;
       {
        HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          lps->ProgressOffset += ui.Size;
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
          continue;
        }
        RINOK(res)
        if (!fileInStream)
          return E_INVALIDARG;

        bool inSeqMode = false;
        if (!inSeqMode)
        {
          CMyComPtr<IInStream> inStream2;
          fileInStream->QueryInterface(IID_IInStream, (void **)&inStream2);
          inSeqMode = (inStream2 == NULL);
        }
        // seqMode = true; // to test seqMode

        UpdatePropsFromStream(updateOptions, ui, fileInStream, updateCallback, totalComplexity);

        CCompressingResult compressingResult;
        
        RINOK(compressor.Set_Pre_CompressionResult(
            inSeqMode, outSeqMode,
            ui.Size,
            compressingResult))

        SetFileHeader(*options, ui, compressingResult.DescriptorMode, item);

        // file Size can be 64-bit !!!

        SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);

        RINOK(archive.SetRestrictionFromCurrent())
        archive.WriteLocalHeader(item);

        CMyComPtr<IOutStream> outStream;
        archive.CreateStreamForCompressing(outStream);
        
        RINOK(compressor.Compress(
            EXTERNAL_CODECS_LOC_VARS
            fileInStream, outStream,
            inSeqMode, outSeqMode,
            ui.Time,
            ui.Size, ui.Size_WasSetFromStream,
            progress, compressingResult))
        
        if (item.HasDescriptor() != compressingResult.DescriptorMode)
          return E_FAIL;

        SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);

        archive.WriteLocalHeader_Replace(item);
       }
       // if (reportArcProp) RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options->IsRealAesMode()))
       RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
       unpackSizeTotal += item.Size;
       packSizeTotal += item.PackSize;
      }
    }
    else
    {
      UInt64 complexity = 0;
      lps->SendRatio = false;

      RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity))

      lps->SendRatio = true;
      lps->ProgressOffset += complexity;
    }
  
    items.Add(item);
    lps->ProgressOffset += kLocalHeaderSize;
  }

  lps->InSize = unpackSizeTotal;
  lps->OutSize = packSizeTotal;
  RINOK(lps->SetCur())

  RINOK(archive.WriteCentralDir(items, comment))

  /*
  CTotalStats stat;
  stat.Size = unpackSizeTotal;
  stat.PackSize = packSizeTotal;
  if (reportArcProp)
    RINOK(ReportArcProps(reportArcProp, stat))
  */

  lps->ProgressOffset += kCentralHeaderSize * updateItems.Size() + 1;
  return lps->SetCur();
}

#ifndef Z7_ST


static const size_t kBlockSize = 1 << 16;
// kMemPerThread must be >= kBlockSize
//
static const size_t kMemPerThread = (size_t)sizeof(size_t) << 23;
// static const size_t kMemPerThread = (size_t)sizeof(size_t) << 16; // for debug
// static const size_t kMemPerThread = (size_t)1 << 16; // for debug

/*
in:
   nt_Zip >= 1:  the starting maximum number of ZIP threads for search
out:
   nt_Zip:  calculated number of ZIP threads
   returns: calculated number of ZSTD threads
*/
/*
static UInt32 CalcThreads_for_ZipZstd(CZstdEncProps *zstdProps,
    UInt64 memLimit, UInt32 totalThreads,
    UInt32 &nt_Zip)
{
  for (; nt_Zip > 1; nt_Zip--)
  {
    UInt64 mem1 = memLimit / nt_Zip;
    if (mem1 <= kMemPerThread)
      continue;
    mem1 -= kMemPerThread;
    UInt32 n_ZSTD = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
        zstdProps, mem1, totalThreads / nt_Zip);
    // we don't allow (nbWorkers == 1) here
    if (n_ZSTD <= 1)
      n_ZSTD = 0;
    zstdProps->nbWorkers = n_ZSTD;
    mem1 = ZstdEncProps_GetMemUsage(zstdProps);
    if ((mem1 + kMemPerThread) * nt_Zip <= memLimit)
      return n_ZSTD;
  }
  return ZstdEncProps_GetNumThreads_for_MemUsageLimit(
      zstdProps, memLimit, totalThreads);
}


static UInt32 SetZstdThreads(
    const CCompressionMethodMode &options,
    COneMethodInfo *oneMethodMain,
    UInt32 numThreads,
    UInt32 numZipThreads_limit,
    UInt64 numFilesToCompress,
    UInt64 numBytesToCompress)
{
  NCompress::NZstd::CEncoderProps encoderProps;
  RINOK(encoderProps.SetFromMethodProps(*oneMethodMain));
  CZstdEncProps &zstdProps = encoderProps.EncProps;
  ZstdEncProps_NormalizeFull(&zstdProps);
  if (oneMethodMain->FindProp(NCoderPropID::kNumThreads) >= 0)
  {
    // threads for ZSTD are fixed
    if (zstdProps.nbWorkers > 1)
      numThreads /= zstdProps.nbWorkers;
    if (numThreads > numZipThreads_limit)
      numThreads = numZipThreads_limit;
    if (options._memUsage_WasSet
        && !options._numThreads_WasForced)
    {
      const UInt64 mem1 = ZstdEncProps_GetMemUsage(&zstdProps);
      const UInt64 numZipThreads = options._memUsage_Compress / (mem1 + kMemPerThread);
      if (numThreads > numZipThreads)
        numThreads = (UInt32)numZipThreads;
    }
    return numThreads;
  }
  {
    // threads for ZSTD are not fixed

    // calculate estimated required number of ZST threads per file size statistics
    UInt32 t = MY_ZSTDMT_NBWORKERS_MAX;
    {
      UInt64 averageNumberOfBlocks = 0;
      const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
      const UInt64 jobSize = zstdProps.jobSize;
      if (jobSize != 0)
        averageNumberOfBlocks = averageSize / jobSize + 0;
      if (t > averageNumberOfBlocks)
        t = (UInt32)averageNumberOfBlocks;
    }
    if (t > numThreads)
      t = numThreads;

    // calculate the nuber of zip threads
    UInt32 numZipThreads = numThreads;
    if (t > 1)
      numZipThreads = numThreads / t;
    if (numZipThreads > numZipThreads_limit)
      numZipThreads = numZipThreads_limit;
    if (numZipThreads < 1)
      numZipThreads = 1;
    {
      // recalculate the number of ZSTD threads via the number of ZIP threads
      const UInt32 t2 = numThreads / numZipThreads;
      if (t < t2)
        t = t2;
    }
    
    if (options._memUsage_WasSet
        && !options._numThreads_WasForced)
    {
      t = CalcThreads_for_ZipZstd(&zstdProps,
          options._memUsage_Compress, numThreads, numZipThreads);
      numThreads = numZipThreads;
    }
    // we don't use (nbWorkers = 1) here
    if (t <= 1)
      t = 0;
    oneMethodMain->AddProp_NumThreads(t);
    return numThreads;
  }
}
*/

#endif




static HRESULT Update2(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive,
    CInArchive *inArchive,
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &options, bool outSeqMode,
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);

  /*
  CMyComPtr<IArchiveUpdateCallbackArcProp> reportArcProp;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackArcProp, (void **)&reportArcProp);
  */

  bool unknownComplexity = false;
  UInt64 complexity = 0;
 #ifndef Z7_ST
  UInt64 numFilesToCompress = 0;
  UInt64 numBytesToCompress = 0;
 #endif
 
  unsigned i;
  
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      if (ui.Size == (UInt64)(Int64)-1)
        unknownComplexity = true;
      else
        complexity += ui.Size;
     #ifndef Z7_ST
      numBytesToCompress += ui.Size;
      numFilesToCompress++;
     #endif
      /*
      if (ui.Commented)
        complexity += ui.CommentRange.Size;
      */
    }
    else
    {
      CItemEx inputItem = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(inputItem) != S_OK)
        return E_NOTIMPL;
      complexity += inputItem.GetLocalFullSize();
      // complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
    complexity += kLocalHeaderSize;
    complexity += kCentralHeaderSize;
  }

  if (comment)
    complexity += comment->Size();
  complexity++; // end of central
  
  if (!unknownComplexity)
    updateCallback->SetTotal(complexity);

  UInt64 totalComplexity = complexity;

  CCompressionMethodMode options2 = options;

  if (options2._methods.IsEmpty())
  {
    // we need method item, if default method was used
    options2._methods.AddNew();
  }

  CAddCommon compressor;
  compressor.SetOptions(options2);
  
  complexity = 0;
  
  const Byte method = options.MethodSequence.FrontItem();

  COneMethodInfo *oneMethodMain = NULL;
  if (!options2._methods.IsEmpty())
    oneMethodMain = &options2._methods[0];

  {
    FOR_VECTOR (mi, options2._methods)
    {
      options2.SetGlobalLevelTo(options2._methods[mi]);
    }
  }

  if (oneMethodMain)
  {
    // appnote recommends to use EOS marker for LZMA.
    if (method == NFileHeader::NCompressionMethod::kLZMA)
      oneMethodMain->AddProp_EndMarker_if_NotFound(true);
  }


  #ifndef Z7_ST

  UInt32 numThreads = options._numThreads;
#ifdef _WIN32
  const UInt32 numThreadGroups = options._numThreadGroups;
#endif

  UInt32 numZipThreads_limit = numThreads;
  if (numZipThreads_limit > numFilesToCompress)
    numZipThreads_limit = (UInt32)numFilesToCompress;

  if (numZipThreads_limit > 1)
  {
    const unsigned numFiles_OPEN_MAX = NSystem::Get_File_OPEN_MAX_Reduced_for_3_tasks();
    // printf("\nzip:numFiles_OPEN_MAX =%d\n", (unsigned)numFiles_OPEN_MAX);
    if (numZipThreads_limit > numFiles_OPEN_MAX)
      numZipThreads_limit = (UInt32)numFiles_OPEN_MAX;
  }

  {
    // we reduce number of threads for 32-bit to reduce memory usege to 256 MB
    const UInt32 kNumMaxThreads =
        // _WIN32 (64-bit) supports only 64 threads in one group.
        8 << (sizeof(size_t) / 2); // 32 threads for 32-bit : 128 threads for 64-bit
    if (numThreads > kNumMaxThreads)
      numThreads = kNumMaxThreads;
  }
  /*
  if (numThreads > MAXIMUM_WAIT_OBJECTS) // is 64 in Windows
    numThreads = MAXIMUM_WAIT_OBJECTS;
  */


  /*
  // zstd supports (numThreads == 0);
  if (numThreads < 1)
    numThreads = 1;
  */

  bool mtMode = (numThreads > 1);

  if (numFilesToCompress <= 1)
    mtMode = false;

  // mtMode = true; // debug: to test mtMode

  if (!mtMode)
  {
    // if (oneMethodMain) {
    /*
    if (method == NFileHeader::NCompressionMethod::kZstdWz)
    {
      if (oneMethodMain->FindProp(NCoderPropID::kNumThreads) < 0)
      {
        // numZstdThreads was not forced in oneMethodMain
        if (numThreads >= 1
            && options._memUsage_WasSet
            && !options._numThreads_WasForced)
        {
          NCompress::NZstd::CEncoderProps encoderProps;
          RINOK(encoderProps.SetFromMethodProps(*oneMethodMain))
          CZstdEncProps &zstdProps = encoderProps.EncProps;
          ZstdEncProps_NormalizeFull(&zstdProps);
          numThreads = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
              &zstdProps, options._memUsage_Compress, numThreads);
          // we allow (nbWorkers = 1) here.
        }
        oneMethodMain->AddProp_NumThreads(numThreads);
      }
    } // kZstdWz
    */
    // } // oneMethodMain

    FOR_VECTOR (mi, options2._methods)
    {
      COneMethodInfo &onem = options2._methods[mi];

      if (onem.FindProp(NCoderPropID::kNumThreads) < 0)
      {
        // fixme: we should check the number of threads for xz method also
        // fixed for 9.31. bzip2 default is just one thread.
        onem.AddProp_NumThreads(numThreads);
      }
    }
  }
  else // mtMode
  {
    if (method == NFileHeader::NCompressionMethod::kStore && !options.Password_Defined)
      numThreads = 1;
    
   if (oneMethodMain)
   {

    if (method == NFileHeader::NCompressionMethod::kBZip2)
    {
      bool fixedNumber;
      UInt32 numBZip2Threads = oneMethodMain->Get_BZip2_NumThreads(fixedNumber);
      if (!fixedNumber)
      {
        const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
        const UInt32 blockSize = oneMethodMain->Get_BZip2_BlockSize();
        const UInt64 averageNumberOfBlocks = averageSize / blockSize + 1;
        numBZip2Threads = 64;
        if (numBZip2Threads > averageNumberOfBlocks)
          numBZip2Threads = (UInt32)averageNumberOfBlocks;
        if (numBZip2Threads > numThreads)
          numBZip2Threads = numThreads;
        oneMethodMain->AddProp_NumThreads(numBZip2Threads);
      }
      numThreads /= numBZip2Threads;
    }
    else if (method == NFileHeader::NCompressionMethod::kXz)
    {
      UInt32 numLzmaThreads = 1;
      int numXzThreads = oneMethodMain->Get_Xz_NumThreads(numLzmaThreads);
      if (numXzThreads < 0)
      {
        // numXzThreads is unknown
        const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
        const UInt64 blockSize = oneMethodMain->Get_Xz_BlockSize();
        UInt64 averageNumberOfBlocks = 1;
        if (blockSize != (UInt64)(Int64)-1)
          averageNumberOfBlocks = averageSize / blockSize + 1;
        UInt32 t = 256;
        if (t > averageNumberOfBlocks)
          t = (UInt32)averageNumberOfBlocks;
        t *= numLzmaThreads;
        if (t > numThreads)
          t = numThreads;
        oneMethodMain->AddProp_NumThreads(t);
        numXzThreads = (int)t;
      }
      numThreads /= (unsigned)numXzThreads;
    }
    /*
    else if (method == NFileHeader::NCompressionMethod::kZstdWz)
    {
      numThreads = SetZstdThreads(options,
          oneMethodMain, numThreads,
          numZipThreads_limit,
          numFilesToCompress, numBytesToCompress);
    }
    */
    else if (
           method == NFileHeader::NCompressionMethod::kDeflate
        || method == NFileHeader::NCompressionMethod::kDeflate64
        || method == NFileHeader::NCompressionMethod::kPPMd)
    {
      if (numThreads > 1
          && options._memUsage_WasSet
          && !options._numThreads_WasForced)
      {
        UInt64 methodMemUsage;
        if (method == NFileHeader::NCompressionMethod::kPPMd)
          methodMemUsage = oneMethodMain->Get_Ppmd_MemSize();
        else
          methodMemUsage = (4 << 20); // for deflate
        const UInt64 threadMemUsage = kMemPerThread + methodMemUsage;
        const UInt64 numThreads64 = options._memUsage_Compress / threadMemUsage;
        if (numThreads64 < numThreads)
          numThreads = (UInt32)numThreads64;
      }
    }
    else if (method == NFileHeader::NCompressionMethod::kLZMA)
    {
      // we suppose that default LZMA is 2 thread. So we don't change it
      const UInt32 numLZMAThreads = oneMethodMain->Get_Lzma_NumThreads();
      numThreads /= numLZMAThreads;

      if (numThreads > 1
          && options._memUsage_WasSet
          && !options._numThreads_WasForced)
      {
        const UInt64 methodMemUsage = oneMethodMain->Get_Lzma_MemUsage(true);
        const UInt64 threadMemUsage = kMemPerThread + methodMemUsage;
        const UInt64 numThreads64 = options._memUsage_Compress / threadMemUsage;
        if (numThreads64 < numThreads)
          numThreads = (UInt32)numThreads64;
      }
    }
   } // (oneMethodMain)

    if (numThreads > numZipThreads_limit)
      numThreads = numZipThreads_limit;
    if (numThreads <= 1)
    {
      mtMode = false;
      numThreads = 1;
    }
  }

  // mtMode = true; // to test mtMode for seqMode

  if (!mtMode)
  #endif
    return Update2St(
        EXTERNAL_CODECS_LOC_VARS
        archive, inArchive,
        inputItems, updateItems,
        updateOptions,
        &options2, outSeqMode,
        comment, updateCallback, totalComplexity,
        opCallback
        // , reportArcProp
        );


  #ifndef Z7_ST

  /*
  CTotalStats stat;
  stat.Size = 0;
  stat.PackSize = 0;
  */
  if (numThreads < 1)
    numThreads = 1;

  CObjectVector<CItemOut> items;

  CMtProgressMixer *mtProgressMixerSpec = new CMtProgressMixer;
  CMyComPtr<ICompressProgressInfo> progress = mtProgressMixerSpec;
  mtProgressMixerSpec->Create(updateCallback, true);

  CMtCompressProgressMixer mtCompressProgressMixer;
  mtCompressProgressMixer.Init(numThreads, mtProgressMixerSpec->RatioProgress);

  CMemBlockManagerMt memManager(kBlockSize);
  CMemRefs refs(&memManager);

  CMtSem mtSem;
  CThreads threads;
  mtSem.Head = -1;
  mtSem.Indexes.ClearAndSetSize(numThreads);
  {
    WRes wres = mtSem.Semaphore.Create(0, numThreads);
    if (wres != 0)
      return HRESULT_FROM_WIN32(wres);
  }

  CUIntVector threadIndices;  // list threads in order of updateItems

  {
    RINOK(memManager.AllocateSpaceAlways((size_t)numThreads * (kMemPerThread / kBlockSize)))
    for (i = 0; i < updateItems.Size(); i++)
      refs.Refs.Add(CMemBlocks2());

    for (i = 0; i < numThreads; i++)
    {
      threads.Threads.AddNew();
      // mtSem.Indexes[i] = -1; // actually we don't use these values
    }

    for (i = 0; i < numThreads; i++)
    {
      CThreadInfo &threadInfo = threads.Threads[i];
      threadInfo.ThreadIndex = i;
      threadInfo.SetOptions(options2);
      #ifdef Z7_EXTERNAL_CODECS
      threadInfo._externalCodecs = _externalCodecs;
      #endif
      RINOK(threadInfo.CreateEvents())
      threadInfo.OutStreamSpec = new COutMemStream(&memManager);
      RINOK(threadInfo.OutStreamSpec->CreateEvents(SYNC_WFMO(&memManager.Synchro)))
      threadInfo.OutStream = threadInfo.OutStreamSpec;
      threadInfo.ProgressSpec = new CMtCompressProgress();
      threadInfo.Progress = threadInfo.ProgressSpec;
      threadInfo.ProgressSpec->Init(&mtCompressProgressMixer, i);
      threadInfo.MtSem = &mtSem;
      const HRESULT hres =
        threadInfo.CreateThread_with_group(
#ifdef _WIN32
          (numThreadGroups > 1 && numThreads > 1) ?
            (int)(i % numThreadGroups) : -1
#endif
        );
      RINOK(hres)
    }
  }

  unsigned mtItemIndex = 0;
  unsigned itemIndex = 0;
  int lastRealStreamItemIndex = -1;

  
  while (itemIndex < updateItems.Size())
  {
    if (threadIndices.Size() < numThreads && mtItemIndex < updateItems.Size())
    {
      // we start ahead the threads for compressing
      // also we set refs.Refs[itemIndex].SeqMode that is used later
      // don't move that code block
      
      CUpdateItem &ui = updateItems[mtItemIndex++];
      if (!ui.NewData)
        continue;
      CItemEx itemEx;
      CItemOut item;
      
      if (ui.NewProps)
      {
        if (ui.IsDir)
          continue;
      }
      else
      {
        itemEx = inputItems[(unsigned)ui.IndexInArc];
        if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
          return E_NOTIMPL;
        (CItem &)item = itemEx;
        if (item.IsDir() != ui.IsDir)
          return E_NOTIMPL;
        if (ui.IsDir)
          continue;
      }
      
      CMyComPtr<ISequentialInStream> fileInStream;
      
      CMemBlocks2 &memRef2 = refs.Refs[mtItemIndex - 1];

      {
        NWindows::NSynchronization::CCriticalSectionLock lock(mtProgressMixerSpec->Mixer2->CriticalSection);
        const HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          complexity += ui.Size;
          complexity += kLocalHeaderSize;
          mtProgressMixerSpec->Mixer2->SetProgressOffset_NoLock(complexity);
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
          memRef2.Skip = true;
          continue;
        }
        RINOK(res)
        if (!fileInStream)
          return E_INVALIDARG;
        UpdatePropsFromStream(updateOptions, ui, fileInStream, updateCallback, totalComplexity);
        RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
      }

      UInt32 k;
      for (k = 0; k < numThreads; k++)
        if (threads.Threads[k].IsFree)
          break;

      if (k == numThreads)
        return E_FAIL;
      {
        {
          CThreadInfo &threadInfo = threads.Threads[k];
          threadInfo.IsFree = false;
          threadInfo.InStream = fileInStream;

          bool inSeqMode = false;

          if (!inSeqMode)
          {
            CMyComPtr<IInStream> inStream2;
            fileInStream->QueryInterface(IID_IInStream, (void **)&inStream2);
            inSeqMode = (inStream2 == NULL);
          }
          memRef2.InSeqMode = inSeqMode;

          // !!!!! we must release ref before sending event
          // BUG was here in v4.43 and v4.44. It could change ref counter in two threads in same time
          fileInStream.Release();

          threadInfo.OutStreamSpec->Init();
          threadInfo.ProgressSpec->Reinit();
          
          threadInfo.UpdateIndex = mtItemIndex - 1;
          threadInfo.InSeqMode = inSeqMode;
          threadInfo.OutSeqMode = outSeqMode;
          threadInfo.FileTime = ui.Time; // FileTime is used for ZipCrypto only in seqMode
          threadInfo.ExpectedDataSize = ui.Size;
          threadInfo.ExpectedDataSize_IsConfirmed = ui.Size_WasSetFromStream;

          threadInfo.CompressEvent.Set();
          
          threadIndices.Add(k);
        }
      }
 
      continue;
    }
    
    if (refs.Refs[itemIndex].Skip)
    {
      itemIndex++;
      continue;
    }

    const CUpdateItem &ui = updateItems[itemIndex];

    CItemEx itemEx;
    CItemOut item;
    
    if (!ui.NewProps || !ui.NewData)
    {
      itemEx = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
        return E_NOTIMPL;
      (CItem &)item = itemEx;
    }

    if (ui.NewData)
    {
      // bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
      const bool isDir = ui.IsDir;
      
      if (isDir)
      {
        RINOK(WriteDirHeader(archive, &options, ui, item))
      }
      else
      {
        CMemBlocks2 &memRef = refs.Refs[itemIndex];
        
        if (memRef.Finished)
        {
          if (lastRealStreamItemIndex < (int)itemIndex)
            lastRealStreamItemIndex = (int)itemIndex;

          SetFileHeader(options, ui, memRef.CompressingResult.DescriptorMode, item);

          // the BUG was fixed in 9.26:
          // SetItemInfoFromCompressingResult must be after SetFileHeader
          // to write correct Size.

          SetItemInfoFromCompressingResult(memRef.CompressingResult,
              options.IsRealAesMode(), options.AesKeyMode, item);
          RINOK(archive.ClearRestriction())
          archive.WriteLocalHeader(item);
          // RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          CMyComPtr<ISequentialOutStream> outStream;
          archive.CreateStreamForCopying(outStream);
          memRef.WriteToStream(memManager.GetBlockSize(), outStream);
          // v23: we fixed the bug: we need to write descriptor also
          if (item.HasDescriptor())
          {
            /* that function doesn't rewrite local header, if item.HasDescriptor().
               it just writes descriptor */
            archive.WriteLocalHeader_Replace(item);
          }
          else
            archive.MoveCurPos(item.PackSize);
          memRef.FreeOpt(&memManager);
          /*
          if (reportArcProp)
          {
            stat.UpdateWithItem(item);
            RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options.IsRealAesMode()));
          }
          */
        }
        else
        {
          // current file was not finished

          if (lastRealStreamItemIndex < (int)itemIndex)
          {
            // LocalHeader was not written for current itemIndex still

            lastRealStreamItemIndex = (int)itemIndex;

            // thread was started before for that item already, and memRef.SeqMode was set

            CCompressingResult compressingResult;
            RINOK(compressor.Set_Pre_CompressionResult(
                memRef.InSeqMode, outSeqMode,
                ui.Size,
                compressingResult))

            memRef.PreDescriptorMode = compressingResult.DescriptorMode;
            SetFileHeader(options, ui, compressingResult.DescriptorMode, item);

            SetItemInfoFromCompressingResult(compressingResult, options.IsRealAesMode(), options.AesKeyMode, item);

            // file Size can be 64-bit !!!
            RINOK(archive.SetRestrictionFromCurrent())
            archive.WriteLocalHeader(item);
          }

          {
            CThreadInfo &thread = threads.Threads[threadIndices.FrontItem()];
            if (!thread.OutStreamSpec->WasUnlockEventSent())
            {
              CMyComPtr<IOutStream> outStream;
              archive.CreateStreamForCompressing(outStream);
              thread.OutStreamSpec->SetOutStream(outStream);
              thread.OutStreamSpec->SetRealStreamMode();
            }
          }

          const WRes wres = mtSem.Semaphore.Lock();
          if (wres != 0)
            return HRESULT_FROM_WIN32(wres);

          const int ti = mtSem.GetFreeItem();
          if (ti < 0)
            return E_FAIL;

          CThreadInfo &threadInfo = threads.Threads[(unsigned)ti];
          threadInfo.InStream.Release();
          threadInfo.IsFree = true;
          RINOK(threadInfo.Result)

          unsigned t = 0;

          for (;;)
          {
            if (t == threadIndices.Size())
              return E_FAIL;
            if (threadIndices[t] == (unsigned)ti)
              break;
            t++;
          }
          threadIndices.Delete(t);
          
          if (t == 0)
          {
            // if thread for current file was finished.
            if (threadInfo.UpdateIndex != itemIndex)
              return E_FAIL;

            if (memRef.PreDescriptorMode != threadInfo.CompressingResult.DescriptorMode)
              return E_FAIL;

            RINOK(threadInfo.OutStreamSpec->WriteToRealStream())
            threadInfo.OutStreamSpec->ReleaseOutStream();
            SetFileHeader(options, ui, threadInfo.CompressingResult.DescriptorMode, item);
            SetItemInfoFromCompressingResult(threadInfo.CompressingResult,
                options.IsRealAesMode(), options.AesKeyMode, item);

            archive.WriteLocalHeader_Replace(item);

            /*
            if (reportArcProp)
            {
              stat.UpdateWithItem(item);
              RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options.IsRealAesMode()));
            }
            */
          }
          else
          {
            // it's not current file. So we must store information in array
            CMemBlocks2 &memRef2 = refs.Refs[threadInfo.UpdateIndex];
            threadInfo.OutStreamSpec->DetachData(memRef2);
            memRef2.CompressingResult = threadInfo.CompressingResult;
            // memRef2.SeqMode = threadInfo.SeqMode; // it was set before
            memRef2.Finished = true;
            continue;
          }
        }
      }
    }
    else
    {
      RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity))
    }
 
    items.Add(item);
    complexity += kLocalHeaderSize;
    mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
    itemIndex++;
  }
  
  RINOK(mtCompressProgressMixer.SetRatioInfo(0, NULL, NULL))

  RINOK(archive.WriteCentralDir(items, comment))

  /*
  if (reportArcProp)
  {
    RINOK(ReportArcProps(reportArcProp, stat));
  }
  */

  complexity += kCentralHeaderSize * updateItems.Size() + 1;
  mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
  return mtCompressProgressMixer.SetRatioInfo(0, NULL, NULL);

  #endif
}

/*
// we need CSeekOutStream, if we need Seek(0, STREAM_SEEK_CUR) for seqential stream
Z7_CLASS_IMP_COM_1(
  CSeekOutStream
  , IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  CMyComPtr<ISequentialOutStream> _seqStream;
  UInt64 _size;
public:
  void Init(ISequentialOutStream *seqStream)
  {
    _size = 0;
    _seqStream = seqStream;
  }
};

Z7_COM7F_IMF(CSeekOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize;
  const HRESULT result = _seqStream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}

Z7_COM7F_IMF(CSeekOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  if (seekOrigin != STREAM_SEEK_CUR || offset != 0)
    return E_NOTIMPL;
  if (newPosition)
    *newPosition = (UInt64)_size;
  return S_OK;
}

Z7_COM7F_IMF(CSeekOutStream::SetSize(UInt64 newSize))
{
  UNUSED_VAR(newSize)
  return E_NOTIMPL;
}
*/

static const size_t kCacheBlockSize = 1 << 20;
static const size_t kCacheSize = kCacheBlockSize << 2;
static const size_t kCacheMask = kCacheSize - 1;

Z7_CLASS_IMP_NOQIB_2(
  CCacheOutStream
  , IOutStream
  , IStreamSetRestriction
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  HRESULT _hres;
  CMyComPtr<ISequentialOutStream> _seqStream;
  CMyComPtr<IOutStream> _stream;
  CMyComPtr<IStreamSetRestriction> _setRestriction;
  Byte *_cache;
  size_t _cachedSize;
  UInt64 _cachedPos;
  UInt64 _virtPos;
  UInt64 _virtSize;
  UInt64 _phyPos;
  UInt64 _phySize;
  UInt64 _restrict_begin;
  UInt64 _restrict_end;

  HRESULT FlushFromCache(size_t size);
  HRESULT FlushNonRestrictedBlocks();
  HRESULT FlushCache();
  HRESULT SetRestriction_ForWrite(size_t writeSize) const;

  HRESULT SeekPhy(UInt64 pos)
  {
    if (pos == _phyPos)
      return S_OK;
    if (!_stream)
      return E_NOTIMPL;
    _hres = _stream->Seek((Int64)pos, STREAM_SEEK_SET, &_phyPos);
    if (_hres == S_OK && _phyPos != pos)
      _hres = E_FAIL;
    return _hres;
  }

public:
  CCacheOutStream(): _cache(NULL) {}
  ~CCacheOutStream();
  bool Allocate()
  {
    if (!_cache)
      _cache = (Byte *)::MidAlloc(kCacheSize);
    return _cache != NULL;
  }
  HRESULT Init(ISequentialOutStream *seqStream, IOutStream *stream, IStreamSetRestriction *setRestriction);
  HRESULT FinalFlush();
};

CCacheOutStream::~CCacheOutStream()
{
  ::MidFree(_cache);
}


HRESULT CCacheOutStream::Init(ISequentialOutStream *seqStream, IOutStream *stream, IStreamSetRestriction *setRestriction)
{
  _hres = S_OK;
  _cachedSize = 0;
  _cachedPos = 0;
  _virtPos = 0;
  _virtSize = 0;
  // by default we have no restriction
  _restrict_begin = 0;
  _restrict_end = 0;
  _seqStream = seqStream;
  _stream = stream;
  _setRestriction = setRestriction;
  if (_stream)
  {
    RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &_virtPos))
    RINOK(_stream->Seek(0, STREAM_SEEK_END, &_virtSize))
    RINOK(_stream->Seek((Int64)_virtPos, STREAM_SEEK_SET, &_virtPos))
  }
  _phyPos = _virtPos;
  _phySize = _virtSize;
  return S_OK;
}


/* we call SetRestriction_ForWrite() just before Write() from cache.
   (_phyPos == _cachedPos)
   (writeSize != 0)
*/
HRESULT CCacheOutStream::SetRestriction_ForWrite(size_t writeSize) const
{
  if (!_setRestriction)
    return S_OK;
  PRF(printf("\n-- CCacheOutStream::SetRestriction_ForWrite _cachedPos = 0x%x, writeSize = %d\n", (unsigned)_cachedPos, (unsigned)writeSize));
  UInt64 begin = _restrict_begin;
  UInt64 end = _restrict_end;
  const UInt64 phyPos = _phyPos;
  if (phyPos != _cachedPos) return E_FAIL;
  if (phyPos == _phySize)
  {
    // The writing will be to the end of phy stream.
    // So we will try to use non-restricted write, if possible.
    if (begin == end)
      begin = _virtPos; // _virtSize; // it's supposed that (_virtSize == _virtPos)
    if (phyPos + writeSize <= begin)
    {
      // the write is not restricted
      PRF(printf("\n+++ write is not restricted \n"));
      begin = 0;
      end = 0;
    }
    else
    {
      if (begin > phyPos)
        begin = phyPos;
      end = (UInt64)(Int64)-1;
    }
  }
  else
  {
    // (phyPos != _phySize)
    if (begin == end || begin > phyPos)
      begin = phyPos;
    end = (UInt64)(Int64)-1;
  }
  return _setRestriction->SetRestriction(begin, end);
}


/* it writes up to (size) bytes from cache.
   (size > _cachedSize) is allowed
*/
HRESULT CCacheOutStream::FlushFromCache(size_t size)
{
  PRF(printf("\n-- CCacheOutStream::FlushFromCache %u\n", (unsigned)size));
  if (_hres != S_OK)
    return _hres;
  if (size > _cachedSize)
      size = _cachedSize;
  // (size <= _cachedSize)
  if (size == 0)
    return S_OK;
  RINOK(SeekPhy(_cachedPos))
  for (;;)
  {
    // (_phyPos == _cachedPos)
    const size_t pos = (size_t)_cachedPos & kCacheMask;
    const size_t cur = MyMin(kCacheSize - pos, size);
    _hres = SetRestriction_ForWrite(cur);
    RINOK(_hres)
    PRF(printf("\n-- CCacheOutStream::WriteFromCache _phyPos = 0x%x, size = %d\n", (unsigned)_phyPos, (unsigned)cur));
    _hres = WriteStream(_seqStream, _cache + pos, cur);
    RINOK(_hres)
    _phyPos += cur;
    if (_phySize < _phyPos)
      _phySize = _phyPos;
    _cachedPos += cur;
    _cachedSize -= cur;
    size -= cur;
    if (size == 0)
      return S_OK;
  }
}


HRESULT CCacheOutStream::FlushNonRestrictedBlocks()
{
  for (;;)
  {
    const size_t size = kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1));
    if (_cachedSize < size)
      break;
    UInt64 begin = _restrict_begin;
    if (begin == _restrict_end)
      begin = _virtPos;
    // we don't flush the data to restricted area
    if (_cachedPos + size > begin)
      break;
    RINOK(FlushFromCache(size))
  }
  return S_OK;
}


HRESULT CCacheOutStream::FlushCache()
{
  return FlushFromCache(_cachedSize);
}

HRESULT CCacheOutStream::FinalFlush()
{
  _restrict_begin = 0;
  _restrict_end = 0;
  RINOK(FlushCache())
  if (_stream && _hres == S_OK)
  {
    if (_virtSize != _phySize)
    {
      // it's unexpected
      RINOK(_stream->SetSize(_virtSize))
      _phySize = _virtSize;
    }
    _hres = SeekPhy(_virtPos);
  }
  return _hres;
}


Z7_COM7F_IMF(CCacheOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  PRF(printf("\n==== CCacheOutStream::Write virtPos=0x%x, %u\n", (unsigned)_virtPos, (unsigned)size));

  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_hres != S_OK)
    return _hres;

  if (_cachedSize != 0)
  if (_virtPos < _cachedPos ||
      _virtPos > _cachedPos + _cachedSize)
  {
    RINOK(FlushCache())
  }

  if (_cachedSize == 0)
    _cachedPos = _virtPos;

  const size_t pos = (size_t)_virtPos & kCacheMask;
  {
    const size_t blockRem = kCacheBlockSize - ((size_t)_virtPos & (kCacheBlockSize - 1));
    if (size > blockRem)
      size = (UInt32)blockRem;
  }
  // _cachedPos <= _virtPos <= _cachedPos + _cachedSize
  const UInt64 cachedRem = _cachedPos + _cachedSize - _virtPos;
  if (cachedRem)
  {
    // _virtPos < _cachedPos + _cachedSize
    // we rewrite only existing data in cache. So _cachedSize will be not changed
    if (size > cachedRem)
      size = (UInt32)cachedRem;
  }
  else
  {
    // _virtPos == _cachedPos + _cachedSize
    // so we need to add new data to the end of cache
    if (_cachedSize == kCacheSize)
    {
      // cache is full. So we need to flush some part of cache.
      // we flush only one block, but we are allowed to flush any size here
      RINOK(FlushFromCache(kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1))))
    }
    // _cachedSize != kCacheSize
    // so we have some space for new data in cache
    if (_cachedSize == 0)
    {
      /* this code is optional (for optimization):
         we write data directly without cache,
         if there is no restriction and we have full block. */
      if (_restrict_begin == _restrict_end
          && size == kCacheBlockSize)
      {
        RINOK(SeekPhy(_virtPos))
        if (_setRestriction)
        {
          _hres = _setRestriction->SetRestriction(_restrict_begin, _restrict_end);
          RINOK(_hres)
        }
        PRF(printf("\n-- CCacheOutStream::WriteDirectly _phyPos = 0x%x, size = %d\n", (unsigned)_phyPos, (unsigned)size));
        _hres = WriteStream(_seqStream, data, size);
        RINOK(_hres)
        if (processedSize)
          *processedSize = size;
        _virtPos += size;
        if (_virtSize < _virtPos)
          _virtSize = _virtPos;
        _phyPos += size;
        if (_phySize < _phyPos)
          _phySize = _phyPos;
        return S_OK;
      }
    }
    else // (_cachedSize != 0)
    {
      const size_t startPos = (size_t)_cachedPos & kCacheMask;
      // we don't allow new data to overwrite old start data in cache.
      // (startPos == pos) here means that cache is empty.
      // (startPos == pos) is not possible here.
      if (startPos > pos)
        size = (UInt32)MyMin((size_t)size, (size_t)(startPos - pos));
    }
    // _virtPos == (_cachedPos + _cachedSize) still
    _cachedSize += size;
  }
  
  memcpy(_cache + pos, data, size);
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  if (_virtSize < _virtPos)
    _virtSize = _virtPos;
  return FlushNonRestrictedBlocks();
}


Z7_COM7F_IMF(CCacheOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  PRF(printf("\n==== CCacheOutStream::Seek seekOrigin=%d Seek =%u\n", seekOrigin, (unsigned)offset));
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _virtSize; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}


Z7_COM7F_IMF(CCacheOutStream::SetSize(UInt64 newSize))
{
  if (_hres != S_OK)
    return _hres;

  if (newSize <= _cachedPos || _cachedSize == 0)
  {
    _cachedSize = 0;
    _cachedPos = newSize;
  }
  else
  {
    // _cachedSize != 0
    // newSize > _cachedPos
    const UInt64 offset = newSize - _cachedPos;
    if (offset <= _cachedSize)
    {
      // newSize is inside cached block or is touching cached block.
      // so we reduce cache
      _cachedSize = (size_t)offset;
      if (_phySize <= newSize)
      {
        // _phySize will be restored later after cache flush
        _virtSize = newSize;
        return S_OK;
      }
      // (_phySize > newSize)
      // so we must reduce phyStream size to (newSize) or to (_cachedPos)
      // newPhySize = _cachedPos; // optional reduce to _cachedPos
    }
    else
    {
      // newSize > _cachedPos + _cachedSize
      /* It's possible that we need to write zeros,
         if new size is larger than old size.
         We don't optimize for possible cases here.
         So we just flush the cache. */
      _hres = FlushCache();
    }
  }

  _virtSize = newSize;

  if (_hres != S_OK)
    return _hres;

  if (newSize != _phySize)
  {
    if (!_stream)
      return E_NOTIMPL;
    // if (_phyPos > newSize)
    RINOK(SeekPhy(newSize))
    if (_setRestriction)
    {
      UInt64 begin = _restrict_begin;
      UInt64 end = _restrict_end;
      if (_cachedSize != 0)
      {
        if (begin > _cachedPos)
          begin = _cachedPos;
        end = (UInt64)(Int64)-1;
      }
      _hres = _setRestriction->SetRestriction(begin, end);
      RINOK(_hres)
    }
    _hres = _stream->SetSize(newSize);
    RINOK(_hres)
    _phySize = newSize;
  }
  return S_OK;
}


Z7_COM7F_IMF(CCacheOutStream::SetRestriction(UInt64 begin, UInt64 end))
{
  PRF(printf("\n============ CCacheOutStream::SetRestriction 0x%x, %u\n", (unsigned)begin, (unsigned)end));
  _restrict_begin = begin;
  _restrict_end = end;
  return FlushNonRestrictedBlocks();
}



HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive, bool removeSfx,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &compressionMethodMode,
    IArchiveUpdateCallback *updateCallback)
{
  /*
  // it was tested before
  if (inArchive)
  {
    if (!inArchive->CanUpdate())
      return E_NOTIMPL;
  }
  */

  CMyComPtr<IStreamSetRestriction> setRestriction;
  seqOutStream->QueryInterface(IID_IStreamSetRestriction, (void **)&setRestriction);
  if (setRestriction)
  {
    RINOK(setRestriction->SetRestriction(0, 0))
  }

  CMyComPtr<IOutStream> outStream;
  CCacheOutStream *cacheStream;
  bool outSeqMode;

  {
    CMyComPtr<IOutStream> outStreamReal;

    if (!compressionMethodMode.Force_SeqOutMode)
    {
      seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStreamReal);
      /*
      if (!outStreamReal)
        return E_NOTIMPL;
      */
    }

    if (inArchive)
    {
      if (!inArchive->IsMultiVol && inArchive->ArcInfo.Base > 0 && !removeSfx)
      {
        IInStream *baseStream = inArchive->GetBaseStream();
        RINOK(InStream_SeekToBegin(baseStream))
        RINOK(NCompress::CopyStream_ExactSize(baseStream, seqOutStream, (UInt64)inArchive->ArcInfo.Base, NULL))
      }
    }

    outSeqMode = (outStreamReal == NULL);
    if (outSeqMode)
      setRestriction.Release();
    /* CCacheOutStream works as non-restricted by default.
       So we use (setRestriction == NULL) for outSeqMode */
    // bool use_cacheStream = true;
    // if (use_cacheStream)
    {
      cacheStream = new CCacheOutStream();
      outStream = cacheStream;
      if (!cacheStream->Allocate())
        return E_OUTOFMEMORY;
      RINOK(cacheStream->Init(seqOutStream, outStreamReal, setRestriction))
      setRestriction.Release();
      if (!outSeqMode)
        setRestriction = cacheStream;
    }
    /*
    else if (!outStreamReal)
    {
      CSeekOutStream *seekOutStream = new CSeekOutStream();
      outStream = seekOutStream;
      seekOutStream->Init(seqOutStream);
    }
    else
      outStream = outStreamReal;
    */
  }

  COutArchive outArchive;
  outArchive.SetRestriction = setRestriction;

  RINOK(outArchive.Create(outStream))

  if (inArchive)
  {
    if (!inArchive->IsMultiVol && (Int64)inArchive->ArcInfo.MarkerPos2 > inArchive->ArcInfo.Base)
    {
      IInStream *baseStream = inArchive->GetBaseStream();
      RINOK(InStream_SeekSet(baseStream, (UInt64)inArchive->ArcInfo.Base))
      const UInt64 embStubSize = (UInt64)((Int64)inArchive->ArcInfo.MarkerPos2 - inArchive->ArcInfo.Base);
      RINOK(NCompress::CopyStream_ExactSize(baseStream, outStream, embStubSize, NULL))
      outArchive.MoveCurPos(embStubSize);
    }
  }

  RINOK (Update2(
      EXTERNAL_CODECS_LOC_VARS
      outArchive, inArchive,
      inputItems, updateItems,
      updateOptions,
      compressionMethodMode, outSeqMode,
      inArchive ? &inArchive->ArcInfo.Comment : NULL,
      updateCallback))

  return cacheStream->FinalFlush();
}

}}
