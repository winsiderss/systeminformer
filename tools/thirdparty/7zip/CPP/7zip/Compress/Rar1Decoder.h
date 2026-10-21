// Rar1Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR1_DECODER_H
#define ZIP7_INC_COMPRESS_RAR1_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NRar1 {

const unsigned kNumRepDists = 4;

Z7_CLASS_IMP_COM_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  bool StMode;

  CLzOutWindow m_OutWindowStream;
  NBitm::CDecoder<CInBuffer> m_InBitStream;

  UInt64 m_UnpackSize;

  UInt32 LastDist;
  UInt32 LastLength;

  UInt32 m_RepDistPtr;
  UInt32 m_RepDists[kNumRepDists];

  int FlagsCnt;
  UInt32 FlagBuf, AvrPlc, AvrPlcB, AvrLn1, AvrLn2, AvrLn3;
  unsigned Buf60, NumHuf, LCount;
  UInt32 Nhfb, Nlzb, MaxDist3;

  UInt32 ChSet[256], ChSetA[256], ChSetB[256], ChSetC[256];
  UInt32 Place[256], PlaceA[256], PlaceB[256], PlaceC[256];
  UInt32 NToPl[256], NToPlB[256], NToPlC[256];

  UInt32 ReadBits(unsigned numBits);
  HRESULT CopyBlock(UInt32 distance, UInt32 len);
  UInt32 DecodeNum(const Byte *numTab);
  HRESULT ShortLZ();
  HRESULT LongLZ();
  HRESULT HuffDecode();
  void GetFlagsBuf();
  void CorrHuff(UInt32 *CharSet, UInt32 *NumToPlace);
  void OldUnpWriteBuf();
  
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

public:
  CDecoder();
};

}}

#endif
