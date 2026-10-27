// Compress/BZip2Const.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_CONST_H
#define ZIP7_INC_COMPRESS_BZIP2_CONST_H

namespace NCompress {
namespace NBZip2 {

const Byte kArSig0 = 'B';
const Byte kArSig1 = 'Z';
const Byte kArSig2 = 'h';
const Byte kArSig3 = '0';

const Byte kFinSig0 = 0x17;
const Byte kFinSig1 = 0x72;
const Byte kFinSig2 = 0x45;
const Byte kFinSig3 = 0x38;
const Byte kFinSig4 = 0x50;
const Byte kFinSig5 = 0x90;

const Byte kBlockSig0 = 0x31;
const Byte kBlockSig1 = 0x41;
const Byte kBlockSig2 = 0x59;
const Byte kBlockSig3 = 0x26;
const Byte kBlockSig4 = 0x53;
const Byte kBlockSig5 = 0x59;

const unsigned kNumOrigBits = 24;

const unsigned kNumTablesBits = 3;
const unsigned kNumTablesMin = 2;
const unsigned kNumTablesMax = 6;

const unsigned kNumLevelsBits = 5;

const unsigned kMaxHuffmanLen = 20; // Check it

const unsigned kMaxAlphaSize = 258;

const unsigned kGroupSize = 50;

const unsigned kBlockSizeMultMin = 1;
const unsigned kBlockSizeMultMax = 9;

const UInt32 kBlockSizeStep = 100000;
const UInt32 kBlockSizeMax = kBlockSizeMultMax * kBlockSizeStep;

const unsigned kNumSelectorsBits = 15;
const unsigned kNumSelectorsMax = 2 + kBlockSizeMax / kGroupSize;

const unsigned kRleModeRepSize = 4;

/*
The number of selectors stored in bzip2 block:
(numSelectors <= 18001) - must work with any decoder.
(numSelectors == 18002) - works with bzip2 1.0.6 decoder and all derived decoders.
(numSelectors  > 18002)
   lbzip2 2.5: encoder can write up to (18001 + 7) selectors.

   7-Zip before 19.03: decoder doesn't support it.
   7-Zip        19.03: decoder allows 8 additional selector records for lbzip2 compatibility.
   
   bzip2 1.0.6: decoder can overflow selector[18002] arrays. But there are another
               arrays after selector arrays. So the compiled code works.
   bzip2 1.0.7: decoder doesn't support it.
   bzip2 1.0.8: decoder allows additional selector records for lbzip2 compatibility.
*/

}}

#endif
