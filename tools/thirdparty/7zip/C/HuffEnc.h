/* HuffEnc.h -- Huffman encoding
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_HUFF_ENC_H
#define ZIP7_INC_HUFF_ENC_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define Z7_HUFFMAN_LEN_MAX 16
#define Z7_HUFFMAN_FREQS_SUM_MAX ((1 << 22) - 1)
/*
Conditions:
  2 <= num <= 1024 = 2 ^ NUM_BITS
  Sum(freqs) <= Z7_HUFFMAN_FREQS_SUM_MAX = 4M - 1 = 2 ^ (32 - NUM_BITS) - 1
  1 <= maxLen <= 16 = Z7_HUFFMAN_LEN_MAX
*/
void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, unsigned num, unsigned maxLen);

EXTERN_C_END

#endif
