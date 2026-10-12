// Rar2Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR2_DECODER_H
#define ZIP7_INC_COMPRESS_RAR2_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "HuffmanDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NRar2 {

const unsigned kNumReps = 4;
const unsigned kDistTableSize = 48;
const unsigned kNumLen2Symbols = 8;
const unsigned kLenTableSize = 28;
const unsigned kMainTableSize = 256 + 2 + kNumReps + kNumLen2Symbols + kLenTableSize;
const unsigned kHeapTablesSizesSum = kMainTableSize + kDistTableSize + kLenTableSize;
const unsigned k_MM_TableSize = 256 + 1;
const unsigned k_MM_NumChanelsMax = 4;
const unsigned k_MM_TablesSizesSum = k_MM_TableSize * k_MM_NumChanelsMax;
const unsigned kMaxTableSize = k_MM_TablesSizesSum;

namespace NMultimedia {

struct CFilter
{
  int K1,K2,K3,K4,K5;
  int D1,D2,D3,D4;
  int LastDelta;
  UInt32 Dif[11];
  UInt32 ByteCount;
  int LastChar;

  void Init() { memset(this, 0, sizeof(*this)); }
  Byte Decode(int &channelDelta, Byte delta);
};

struct CFilter2
{
  CFilter  m_Filters[k_MM_NumChanelsMax];
  int m_ChannelDelta;
  unsigned CurrentChannel;

  void Init() { memset(this, 0, sizeof(*this)); }
  Byte Decode(Byte delta)
  {
    return m_Filters[CurrentChannel].Decode(m_ChannelDelta, delta);
  }
};

}

typedef NBitm::CDecoder<CInBuffer> CBitDecoder;

const unsigned kNumHufBits = 15;

Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  bool m_TablesOK;
  bool m_AudioMode;

  CLzOutWindow m_OutWindowStream;
  CBitDecoder m_InBitStream;

  UInt32 m_RepDistPtr;
  UInt32 m_RepDists[kNumReps];
  UInt32 m_LastLength;
  unsigned m_NumChannels;

  NHuffman::CDecoder<kNumHufBits, kMainTableSize, 9> m_MainDecoder;
  NHuffman::CDecoder256<kNumHufBits, kDistTableSize, 7> m_DistDecoder;
  NHuffman::CDecoder256<kNumHufBits, kLenTableSize, 7> m_LenDecoder;
  NHuffman::CDecoder<kNumHufBits, k_MM_TableSize, 9> m_MMDecoders[k_MM_NumChanelsMax];

  UInt64 m_PackSize;
  
  NMultimedia::CFilter2 m_MmFilter;
  Byte m_LastLevels[kMaxTableSize];

  void InitStructures();
  UInt32 ReadBits(unsigned numBits);
  bool ReadTables();
  bool ReadLastTables();

  bool DecodeMm(UInt32 pos);
  bool DecodeLz(Int32 pos);

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

public:
  CDecoder();
};

}}

#endif
