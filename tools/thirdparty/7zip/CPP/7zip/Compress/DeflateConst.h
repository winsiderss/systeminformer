// DeflateConst.h

#ifndef ZIP7_INC_DEFLATE_CONST_H
#define ZIP7_INC_DEFLATE_CONST_H

namespace NCompress {
namespace NDeflate {

const unsigned kNumHuffmanBits = 15;

const UInt32 kHistorySize32 = (1 << 15);
const UInt32 kHistorySize64 = (1 << 16);

const unsigned kDistTableSize32 = 30;
const unsigned kDistTableSize64 = 32;
  
const unsigned kNumLenSymbols32 = 256;
const unsigned kNumLenSymbols64 = 255; // don't change it. It must be <= 255.
const unsigned kNumLenSymbolsMax = kNumLenSymbols32;
  
const unsigned kNumLenSlots = 29;

const unsigned kFixedDistTableSize = 32;
const unsigned kFixedLenTableSize = 31;

const unsigned kSymbolEndOfBlock = 0x100;
const unsigned kSymbolMatch = kSymbolEndOfBlock + 1;

const unsigned kMainTableSize = kSymbolMatch + kNumLenSlots;
const unsigned kFixedMainTableSize = kSymbolMatch + kFixedLenTableSize;

const unsigned kLevelTableSize = 19;

const unsigned kTableDirectLevels = 16;
const unsigned kTableLevelRepNumber = kTableDirectLevels;
const unsigned kTableLevel0Number = kTableLevelRepNumber + 1;
const unsigned kTableLevel0Number2 = kTableLevel0Number + 1;

const unsigned kLevelMask = 0xF;

const Byte kLenStart32[kFixedLenTableSize] =
  {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224, 255, 0, 0};
const Byte kLenStart64[kFixedLenTableSize] =
  {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224, 0, 0, 0};

const Byte kLenDirectBits32[kFixedLenTableSize] =
  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 0, 0, 0};
const Byte kLenDirectBits64[kFixedLenTableSize] =
  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 16, 0, 0};

const UInt32 kDistStart[kDistTableSize64] =
  {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,
  1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152};
const Byte kDistDirectBits[kDistTableSize64] =
  {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14};

const Byte kLevelDirectBits[3] = {2, 3, 7};

const Byte kCodeLengthAlphabetOrder[kLevelTableSize] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

const unsigned kMatchMinLen = 3;
const unsigned kMatchMaxLen32 = kNumLenSymbols32 + kMatchMinLen - 1; // 256 + 2
const unsigned kMatchMaxLen64 = kNumLenSymbols64 + kMatchMinLen - 1; // 255 + 2
const unsigned kMatchMaxLen = kMatchMaxLen32;

const unsigned kFinalBlockFieldSize = 1;

namespace NFinalBlockField
{
  enum
  {
    kNotFinalBlock = 0,
    kFinalBlock = 1
  };
}

const unsigned kBlockTypeFieldSize = 2;

namespace NBlockType
{
  enum
  {
    kStored = 0,
    kFixedHuffman = 1,
    kDynamicHuffman = 2
  };
}

const unsigned kNumLenCodesFieldSize = 5;
const unsigned kNumDistCodesFieldSize = 5;
const unsigned kNumLevelCodesFieldSize = 4;

const unsigned kNumLitLenCodesMin = 257;
const unsigned kNumDistCodesMin = 1;
const unsigned kNumLevelCodesMin = 4;

const unsigned kLevelFieldSize = 3;

const unsigned kStoredBlockLengthFieldSize = 16;

struct CLevels
{
  Byte litLenLevels[kFixedMainTableSize];
  Byte distLevels[kFixedDistTableSize];

  void SubClear()
  {
    unsigned i;
    for (i = kNumLitLenCodesMin; i < kFixedMainTableSize; i++)
      litLenLevels[i] = 0;
    for (i = 0; i < kFixedDistTableSize; i++)
      distLevels[i] = 0;
  }

  void SetFixedLevels()
  {
    unsigned i = 0;
    
    for (; i < 144; i++) litLenLevels[i] = 8;
    for (; i < 256; i++) litLenLevels[i] = 9;
    for (; i < 280; i++) litLenLevels[i] = 7;
    for (; i < 288; i++) litLenLevels[i] = 8;
    
    for (i = 0; i < kFixedDistTableSize; i++)  // test it: InfoZip only uses kDistTableSize
      distLevels[i] = 5;
  }
};

}}

#endif
