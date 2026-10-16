// DeflateDecoder.h

#ifndef ZIP7_INC_DEFLATE_DECODER_H
#define ZIP7_INC_DEFLATE_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitlDecoder.h"
#include "DeflateConst.h"
#include "HuffmanDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

const int kLenIdFinished = -1;
const int kLenIdNeedInit = -2;

const unsigned kNumTableBits_Main = 10;
const unsigned kNumTableBits_Dist = 6;

class CCoder:
  public ICompressCoder,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
#ifndef Z7_NO_READ_FROM_CODER
  public ISequentialInStream,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_COM_QI_ENTRY(ISequentialInStream)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
public:
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
private:
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ISequentialInStream)
#endif

  CLzOutWindow m_OutWindowStream;
  NBitl::CDecoder<CInBuffer> m_InBitStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kFixedMainTableSize, kNumTableBits_Main> m_MainDecoder;
  NCompress::NHuffman::CDecoder256<kNumHuffmanBits, kFixedDistTableSize, kNumTableBits_Dist> m_DistDecoder;
  NCompress::NHuffman::CDecoder7b<kLevelTableSize> m_LevelDecoder;

  UInt32 m_StoredBlockSize;

  unsigned _numDistLevels;
  bool m_FinalBlock;
  bool m_StoredMode;

  bool _deflateNSIS;
  bool _deflate64Mode;
  bool _keepHistory;
  bool _needFinishInput;
  
  bool _needInitInStream;
  bool _needReadTable;
  Int32 _remainLen;
  UInt32 _rep0;

  bool _outSizeDefined;
  CMyComPtr<ISequentialInStream> m_InStreamRef;
  UInt64 _outSize;
  UInt64 _outStartPos;

  void SetOutStreamSizeResume(const UInt64 *outSize);
  UInt64 GetOutProcessedCur() const { return m_OutWindowStream.GetProcessedSize() - _outStartPos; }

  UInt32 ReadBits(unsigned numBits);

  bool DecodeLevels(Byte *levels, unsigned numSymbols);
  bool ReadTables();
  
  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  class CCoderReleaser
  {
    CCoder *_coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): _coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        _coder->Flush();
    }
  };
  friend class CCoderReleaser;

  HRESULT CodeSpec(UInt32 curSize, bool finishInputStream, UInt32 inputProgressLimit = 0);
public:

  CCoder(bool deflate64Mode);
  virtual ~CCoder() {}

  void SetNsisMode(bool nsisMode) { _deflateNSIS = nsisMode; }

  void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  void Set_NeedFinishInput(bool needFinishInput) { _needFinishInput = needFinishInput; }

  bool IsFinished() const { return _remainLen == kLenIdFinished; }
  bool IsFinalBlock() const { return m_FinalBlock; }

  HRESULT CodeReal(ISequentialOutStream *outStream, ICompressProgressInfo *progress);

public:
  HRESULT CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);
  HRESULT InitInStream(bool needInit);

  void AlignToByte() { m_InBitStream.AlignToByte(); }
  Byte ReadAlignedByte();
  UInt32 ReadAligned_UInt16() // aligned for Byte range
  {
    const UInt32 v = m_InBitStream.ReadAlignedByte();
    return v | ((UInt32)m_InBitStream.ReadAlignedByte() << 8);
  }
  bool InputEofError() const { return m_InBitStream.ExtraBitsWereRead(); }

  // size of used real data from input stream
  UInt64 GetStreamSize() const { return m_InBitStream.GetStreamSize(); }

  // size of virtual input stream processed
  UInt64 GetInputProcessedSize() const { return m_InBitStream.GetProcessedSize(); }
};

class CCOMCoder     : public CCoder { public: CCOMCoder(): CCoder(false) {} };
class CCOMCoder64   : public CCoder { public: CCOMCoder64(): CCoder(true) {} };

}}}

#endif
