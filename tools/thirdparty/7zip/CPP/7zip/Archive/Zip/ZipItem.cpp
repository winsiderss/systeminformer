// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"
#include "../../../../C/7zCrc.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/MyLinux.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/PropVariantUtils.h"

#include "../Common/ItemNameUtils.h"

#include "ZipItem.h"

namespace NArchive {
namespace NZip {

using namespace NFileHeader;


/*
const char *k_SpecName_NTFS_STREAM = "@@NTFS@STREAM@";
const char *k_SpecName_MAC_RESOURCE_FORK = "@@MAC@RESOURCE-FORK@";
*/

static const CUInt32PCharPair g_ExtraTypes[] =
{
  { NExtraID::kZip64, "Zip64" },
  { NExtraID::kNTFS, "NTFS" },
  { NExtraID::kUnix0, "UNIX" },
  { NExtraID::kStrongEncrypt, "StrongCrypto" },
  { NExtraID::kUnixTime, "UT" },
  { NExtraID::kUnix1, "UX" },
  { NExtraID::kUnix2, "Ux" },
  { NExtraID::kUnixN, "ux" },
  { NExtraID::kIzUnicodeComment, "uc" },
  { NExtraID::kIzUnicodeName, "up" },
  { NExtraID::kIzNtSecurityDescriptor, "SD" },
  { NExtraID::kWzAES, "WzAES" },
  { NExtraID::kApkAlign, "ApkAlign" }
};

void CExtraSubBlock::PrintInfo(AString &s) const
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_ExtraTypes); i++)
  {
    const CUInt32PCharPair &pair = g_ExtraTypes[i];
    if (pair.Value == ID)
    {
      s += pair.Name;
      if (ID == NExtraID::kUnixTime)
      {
        if (Data.Size() >= 1)
        {
          s.Add_Colon();
          const Byte flags = Data[0];
          if (flags & 1) s.Add_Char('M');
          if (flags & 2) s.Add_Char('A');
          if (flags & 4) s.Add_Char('C');
          const UInt32 size = (UInt32)(Data.Size()) - 1;
          if (size % 4 == 0)
          {
            s.Add_Colon();
            s.Add_UInt32(size / 4);
          }
        }
      }
      /*
      if (ID == NExtraID::kApkAlign && Data.Size() >= 2)
      {
        char sz[32];
        sz[0] = ':';
        ConvertUInt32ToHex(GetUi16(Data), sz + 1);
        s += sz;
        for (unsigned j = 2; j < Data.Size(); j++)
        {
          char sz[32];
          sz[0] = '-';
          ConvertUInt32ToHex(Data[j], sz + 1);
          s += sz;
        }
      }
      */
      return;
    }
  }
  {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(ID, sz + 2);
    s += sz;
  }
}


void CExtraBlock::PrintInfo(AString &s) const
{
  if (Error)
    s.Add_OptSpaced("Extra_ERROR");

  if (MinorError)
    s.Add_OptSpaced("Minor_Extra_ERROR");

  if (IsZip64 || IsZip64_Error)
  {
    s.Add_OptSpaced("Zip64");
    if (IsZip64_Error)
      s += "_ERROR";
  }

  FOR_VECTOR (i, SubBlocks)
  {
    s.Add_Space_if_NotEmpty();
    SubBlocks[i].PrintInfo(s);
  }
}


bool CExtraSubBlock::ExtractNtfsTime(unsigned index, FILETIME &ft) const
{
  ft.dwHighDateTime = ft.dwLowDateTime = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kNTFS || size < 32)
    return false;
  const Byte *p = (const Byte *)Data;
  p += 4; // for reserved
  size -= 4;
  while (size > 4)
  {
    UInt16 tag = GetUi16(p);
    unsigned attrSize = GetUi16(p + 2);
    p += 4;
    size -= 4;
    if (attrSize > size)
      attrSize = size;
    
    if (tag == NNtfsExtra::kTagTime && attrSize >= 24)
    {
      p += 8 * index;
      ft.dwLowDateTime = GetUi32(p);
      ft.dwHighDateTime = GetUi32(p + 4);
      return true;
    }
    p += attrSize;
    size -= attrSize;
  }
  return false;
}

bool CExtraSubBlock::Extract_UnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  /* Info-Zip :
     The central-header extra field contains the modification
     time only, or no timestamp at all.
     Size of Data is used to flag its presence or absence
     If "Flags" indicates that Modtime is present in the local header
     field, it MUST be present in the central header field, too
  */

  res = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kUnixTime || size < 5)
    return false;
  const Byte *p = (const Byte *)Data;
  const Byte flags = *p++;
  size--;
  if (isCentral)
  {
    if (index != NUnixTime::kMTime ||
        (flags & (1 << NUnixTime::kMTime)) == 0 ||
        size < 4)
      return false;
    res = GetUi32(p);
    return true;
  }
  for (unsigned i = 0; i < 3; i++)
    if ((flags & (1 << i)) != 0)
    {
      if (size < 4)
        return false;
      if (index == i)
      {
        res = GetUi32(p);
        return true;
      }
      p += 4;
      size -= 4;
    }
  return false;
}


// Info-ZIP's abandoned "Unix1 timestamps & owner ID info"

bool CExtraSubBlock::Extract_Unix01_Time(unsigned index, UInt32 &res) const
{
  res = 0;
  const unsigned offset = index * 4;
  if (Data.Size() < offset + 4)
    return false;
  if (ID != NExtraID::kUnix0 &&
      ID != NExtraID::kUnix1)
    return false;
  const Byte *p = (const Byte *)Data + offset;
  res = GetUi32(p);
  return true;
}

/*
// PKWARE's Unix "extra" is similar to Info-ZIP's abandoned "Unix1 timestamps"
bool CExtraSubBlock::Extract_Unix_Time(unsigned index, UInt32 &res) const
{
  res = 0;
  const unsigned offset = index * 4;
  if (ID != NExtraID::kUnix0 || Data.Size() < offset)
    return false;
  const Byte *p = (const Byte *)Data + offset;
  res = GetUi32(p);
  return true;
}
*/

bool CExtraBlock::GetNtfsTime(unsigned index, FILETIME &ft) const
{
  FOR_VECTOR (i, SubBlocks)
  {
    const CExtraSubBlock &sb = SubBlocks[i];
    if (sb.ID == NFileHeader::NExtraID::kNTFS)
      return sb.ExtractNtfsTime(index, ft);
  }
  return false;
}

bool CExtraBlock::GetUnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  {
    FOR_VECTOR (i, SubBlocks)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kUnixTime)
        return sb.Extract_UnixTime(isCentral, index, res);
    }
  }
  
  switch (index)
  {
    case NUnixTime::kMTime: index = NUnixExtra::kMTime; break;
    case NUnixTime::kATime: index = NUnixExtra::kATime; break;
    default: return false;
  }
  
  {
    FOR_VECTOR (i, SubBlocks)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kUnix0 ||
          sb.ID == NFileHeader::NExtraID::kUnix1)
        return sb.Extract_Unix01_Time(index, res);
    }
  }
  return false;
}


bool CLocalItem::IsDir() const
{
  return NItemName::HasTailSlash(Name, GetCodePage());
}

bool CItem::IsDir() const
{
  // FIXME: we can check InfoZip UTF-8 name at first.
  if (NItemName::HasTailSlash(Name, GetCodePage()))
    return true;
  
  Byte hostOS = GetHostOS();

  if (Size == 0 && PackSize == 0 && !Name.IsEmpty() && Name.Back() == '\\')
  {
    // do we need to use CharPrevExA?
    // .NET Framework 4.5 : System.IO.Compression::CreateFromDirectory() probably writes backslashes to headers?
    // so we support that case
    switch (hostOS)
    {
      case NHostOS::kFAT:
      case NHostOS::kNTFS:
      case NHostOS::kHPFS:
      case NHostOS::kVFAT:
        return true;
      default: break;
    }
  }

  if (!FromCentral)
    return false;
  
  UInt16 highAttrib = (UInt16)((ExternalAttrib >> 16 ) & 0xFFFF);

  switch (hostOS)
  {
    case NHostOS::kAMIGA:
      switch (highAttrib & NAmigaAttrib::kIFMT)
      {
        case NAmigaAttrib::kIFDIR: return true;
        case NAmigaAttrib::kIFREG: return false;
        default: return false; // change it throw kUnknownAttributes;
      }
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
    case NHostOS::kHPFS:
    case NHostOS::kVFAT:
      return ((ExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    case NHostOS::kAtari:
    case NHostOS::kMac:
    case NHostOS::kVMS:
    case NHostOS::kVM_CMS:
    case NHostOS::kAcorn:
    case NHostOS::kMVS:
      return false; // change it throw kUnknownAttributes;
    case NHostOS::kUnix:
      return MY_LIN_S_ISDIR(highAttrib);
    default:
      return false;
  }
}

UInt32 CItem::GetWinAttrib() const
{
  UInt32 winAttrib = 0;
  switch (GetHostOS())
  {
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
      if (FromCentral)
        winAttrib = ExternalAttrib;
      break;
    case NHostOS::kUnix:
      // do we need to clear 16 low bits in this case?
      if (FromCentral)
      {
        /*
          Some programs write posix attributes in high 16 bits of ExternalAttrib
          Also some programs can write additional marker flag:
            0x8000 - p7zip
            0x4000 - Zip in MacOS
            no marker - Info-Zip

          Client code has two options to detect posix field:
            1) check 0x8000 marker. In that case we must add 0x8000 marker here.
            2) check that high 4 bits (file type bits in posix field) of attributes are not zero.
        */
        
        winAttrib = ExternalAttrib & 0xFFFF0000;
        
        // #ifndef _WIN32
        winAttrib |= 0x8000; // add posix mode marker
        // #endif
      }
      break;
    default: break;
  }
  if (IsDir()) // test it;
    winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttrib;
}

bool CItem::GetPosixAttrib(UInt32 &attrib) const
{
  // some archivers can store PosixAttrib in high 16 bits even with HostOS=FAT.
  if (FromCentral && GetHostOS() == NHostOS::kUnix)
  {
    attrib = ExternalAttrib >> 16;
    return (attrib != 0);
  }
  attrib = 0;
  if (IsDir())
    attrib = MY_LIN_S_IFDIR;
  return false;
}


bool CExtraSubBlock::CheckIzUnicode(const AString &s) const
{
  size_t size = Data.Size();
  if (size < 1 + 4)
    return false;
  const Byte *p = (const Byte *)Data;
  if (p[0] > 1)
    return false;
  if (CrcCalc(s, s.Len()) != GetUi32(p + 1))
    return false;
  size -= 5;
  p += 5;
  for (size_t i = 0; i < size; i++)
    if (p[i] == 0)
      return false;
  return Check_UTF8_Buf((const char *)(const void *)p, size, false);
}
  

void CItem::GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const
{
  bool isUtf8 = IsUtf8();
  // bool ignore_Utf8_Errors = true;
  
  if (!isUtf8)
  {
    {
      const unsigned id = isComment ?
          NFileHeader::NExtraID::kIzUnicodeComment:
          NFileHeader::NExtraID::kIzUnicodeName;
      const CObjectVector<CExtraSubBlock> &subBlocks = GetMainExtra().SubBlocks;
      
      FOR_VECTOR (i, subBlocks)
      {
        const CExtraSubBlock &sb = subBlocks[i];
        if (sb.ID == id)
        {
          if (sb.CheckIzUnicode(s))
          {
            // const unsigned kIzUnicodeHeaderSize = 5;
            if (Convert_UTF8_Buf_To_Unicode(
                (const char *)(const void *)(const Byte *)sb.Data + 5,
                sb.Data.Size() - 5, res))
              return;
          }
          break;
        }
      }
    }
    
    if (useSpecifiedCodePage)
      isUtf8 = (codePage == CP_UTF8);
    #ifdef _WIN32
    else if (GetHostOS() == NFileHeader::NHostOS::kUnix)
    {
      /* Some ZIP archives in Unix use UTF-8 encoding without Utf8 flag in header.
         We try to get name as UTF-8.
         Do we need to do it in POSIX version also? */
      isUtf8 = true;

      /* 21.02: we want to ignore UTF-8 errors to support file paths that are mixed
          of UTF-8 and non-UTF-8 characters. */
      // ignore_Utf8_Errors = false;
      // ignore_Utf8_Errors = true;
    }
    #endif
  }
  
  
  if (isUtf8)
  {
    ConvertUTF8ToUnicode(s, res);
    return;
  }
  
  MultiByteToUnicodeString2(res, s, useSpecifiedCodePage ? codePage : GetCodePage());
}

}}
