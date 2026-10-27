// Lzx.h

#ifndef ZIP7_INC_COMPRESS_LZX_H
#define ZIP7_INC_COMPRESS_LZX_H

#include "../../Common/MyTypes.h"

namespace NCompress {
namespace NLzx {

const unsigned kBlockType_NumBits = 3;
const unsigned kBlockType_Verbatim = 1;
const unsigned kBlockType_Aligned = 2;
const unsigned kBlockType_Uncompressed = 3;

const unsigned kNumHuffmanBits = 16;
const unsigned kNumReps = 3;

const unsigned kNumLenSlots = 8;
const unsigned kMatchMinLen = 2;
const unsigned kNumLenSymbols = 249;
const unsigned kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const unsigned kNumAlignLevelBits = 3;
const unsigned kNumAlignBits = 3;
const unsigned kAlignTableSize = 1 << kNumAlignBits;

const unsigned kNumPosSlots = 50;
const unsigned kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

const unsigned kMainTableSize = 256 + kNumPosLenSlots;
const unsigned kLevelTableSize = 20;
const unsigned kMaxTableSize = kMainTableSize;

const unsigned kNumLevelBits = 4;

const unsigned kLevelSym_Zero1 = 17;
const unsigned kLevelSym_Zero2 = 18;
const unsigned kLevelSym_Same = 19;

const unsigned kLevelSym_Zero1_Start = 4;
const unsigned kLevelSym_Zero1_NumBits = 4;

const unsigned kLevelSym_Zero2_Start = kLevelSym_Zero1_Start + (1 << kLevelSym_Zero1_NumBits);
const unsigned kLevelSym_Zero2_NumBits = 5;

const unsigned kLevelSym_Same_NumBits = 1;
const unsigned kLevelSym_Same_Start = 4;
 
const unsigned kNumDictBits_Min = 15;
const unsigned kNumDictBits_Max = 21;
const UInt32 kDictSize_Max = (UInt32)1 << kNumDictBits_Max;

const unsigned kNumLinearPosSlotBits = 17;
// const unsigned kNumPowerPosSlots = 38;
// const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 1) * 2; // non-including two first linear slots.
const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 2) * 2; // including two first linear slots.

}}

#endif
