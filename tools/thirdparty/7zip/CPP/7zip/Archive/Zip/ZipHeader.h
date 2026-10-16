// ZipHeader.h

#ifndef ZIP7_INC_ARCHIVE_ZIP_HEADER_H
#define ZIP7_INC_ARCHIVE_ZIP_HEADER_H

#include "../../../Common/MyTypes.h"

namespace NArchive {
namespace NZip {

const unsigned kMarkerSize = 4;

namespace NSignature
{
  const UInt32 kLocalFileHeader   = 0x04034B50;
  const UInt32 kDataDescriptor    = 0x08074B50;
  const UInt32 kCentralFileHeader = 0x02014B50;
  const UInt32 kEcd               = 0x06054B50;
  const UInt32 kEcd64             = 0x06064B50;
  const UInt32 kEcd64Locator      = 0x07064B50;
  const UInt32 kSpan              = 0x08074B50;
  const UInt32 kNoSpan            = 0x30304B50; // PK00, replaces kSpan, if there is only 1 segment
}

const unsigned kLocalHeaderSize = 4 + 26; // including signature
const unsigned kDataDescriptorSize32 = 4 + 4 + 4 * 2;  // including signature
const unsigned kDataDescriptorSize64 = 4 + 4 + 8 * 2;  // including signature
const unsigned kCentralHeaderSize = 4 + 42; // including signature

const unsigned kEcdSize = 22; // including signature
const unsigned kEcd64_MainSize = 44;
const unsigned kEcd64_FullSize = 12 + kEcd64_MainSize;
const unsigned kEcd64Locator_Size = 20;

namespace NFileHeader
{
  namespace NCompressionMethod
  {
    enum EType
    {
      kStore = 0,
      kShrink = 1,
      kReduce1 = 2,
      kReduce2 = 3,
      kReduce3 = 4,
      kReduce4 = 5,
      kImplode = 6,
      kTokenize = 7,
      kDeflate = 8,
      kDeflate64 = 9,
      kPKImploding = 10,
      
      kBZip2 = 12,
      
      kLZMA = 14,
      
      kTerse = 18,
      kLz77 = 19,
      kZstdPk = 20,
      
      kZstdWz = 93,
      kMP3 = 94,
      kXz = 95,
      kJpeg = 96,
      kWavPack = 97,
      kPPMd = 98,
      kWzAES = 99
    };

    const Byte kMadeByProgramVersion = 63;
    
    const Byte kExtractVersion_Default = 10;
    const Byte kExtractVersion_Dir = 20;
    const Byte kExtractVersion_ZipCrypto = 20;
    const Byte kExtractVersion_Deflate = 20;
    const Byte kExtractVersion_Deflate64 = 21;
    const Byte kExtractVersion_Zip64 = 45;
    const Byte kExtractVersion_BZip2 = 46;
    const Byte kExtractVersion_Aes = 51;
    const Byte kExtractVersion_LZMA = 63;
    const Byte kExtractVersion_PPMd = 63;
    const Byte kExtractVersion_Xz = 20; // test it
  }

  namespace NExtraID
  {
    enum
    {
      kZip64 = 0x01,
      kNTFS = 0x0A,
      kUnix0 = 0x0D,                // Info-ZIP : (UNIX) PK
      kStrongEncrypt = 0x17,
      kIzNtSecurityDescriptor = 0x4453,
      kUnixTime = 0x5455,           // "UT" (time) Info-ZIP
      kUnix1 = 0x5855,              // Info-ZIP
      kIzUnicodeComment = 0x6375,
      kIzUnicodeName = 0x7075,
      kUnix2 = 0x7855,              // Info-ZIP
      kUnixN = 0x7875,              // Info-ZIP
      kWzAES = 0x9901,
      kApkAlign = 0xD935
    };
  }

  namespace NNtfsExtra
  {
    const UInt16 kTagTime = 1;
    enum
    {
      kMTime = 0,
      kATime,
      kCTime
    };
  }

  namespace NUnixTime
  {
    enum
    {
      kMTime = 0,
      kATime,
      kCTime
    };
  }

  namespace NUnixExtra
  {
    enum
    {
      kATime = 0,
      kMTime
    };
  }

  namespace NFlags
  {
    const unsigned kEncrypted = 1 << 0;
    const unsigned kLzmaEOS = 1 << 1;
    const unsigned kDescriptorUsedMask = 1 << 3;
    const unsigned kStrongEncrypted = 1 << 6;
    const unsigned kUtf8 = 1 << 11;
    const unsigned kAltStream = 1 << 14;

    const unsigned kImplodeDictionarySizeMask = 1 << 1;
    const unsigned kImplodeLiteralsOnMask     = 1 << 2;
    
    /*
    const unsigned kDeflateTypeBitStart = 1;
    const unsigned kNumDeflateTypeBits = 2;
    const unsigned kNumDeflateTypes = (1 << kNumDeflateTypeBits);
    const unsigned kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
    */
  }
  
  namespace NHostOS
  {
    enum EEnum
    {
      kFAT      =  0,
      kAMIGA    =  1,
      kVMS      =  2,  // VAX/VMS
      kUnix     =  3,
      kVM_CMS   =  4,
      kAtari    =  5,  // what if it's a minix filesystem? [cjh]
      kHPFS     =  6,  // filesystem used by OS/2 (and NT 3.x)
      kMac      =  7,
      kZ_System =  8,
      kCPM      =  9,
      kTOPS20   = 10,  // pkzip 2.50 NTFS
      kNTFS     = 11,  // filesystem used by Windows NT
      kQDOS     = 12,  // SMS/QDOS
      kAcorn    = 13,  // Archimedes Acorn RISC OS
      kVFAT     = 14,  // filesystem used by Windows 95, NT
      kMVS      = 15,
      kBeOS     = 16,  // hybrid POSIX/database filesystem
      kTandem   = 17,
      kOS400    = 18,
      kOSX      = 19
    };
  }


  namespace NAmigaAttrib
  {
    const UInt32 kIFMT     = 06000;    // Amiga file type mask
    const UInt32 kIFDIR    = 04000;    // Amiga directory
    const UInt32 kIFREG    = 02000;    // Amiga regular file
    const UInt32 kIHIDDEN  = 00200;    // to be supported in AmigaDOS 3.x
    const UInt32 kISCRIPT  = 00100;    // executable script (text command file)
    const UInt32 kIPURE    = 00040;    // allow loading into resident memory
    const UInt32 kIARCHIVE = 00020;    // not modified since bit was last set
    const UInt32 kIREAD    = 00010;    // can be opened for reading
    const UInt32 kIWRITE   = 00004;    // can be opened for writing
    const UInt32 kIEXECUTE = 00002;    // executable image, a loadable runfile
    const UInt32 kIDELETE  = 00001;    // can be deleted
  }
}

}}

#endif
