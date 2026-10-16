// Compress/BZip2Decoder.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_DECODER_H
#define ZIP7_INC_COMPRESS_BZIP2_DECODER_H

#include "../../Common/MyCom.h"

// #define Z7_NO_READ_FROM_CODER
// #define Z7_ST

#ifndef Z7_ST
#include "../../Windows/Synchronization.h"
#include "../../Windows/Thread.h"
#endif

#include "../ICoder.h"

#include "BZip2Const.h"
#include "BZip2Crc.h"
#include "HuffmanDecoder.h"
#include "Mtf8.h"

namespace NCompress {
namespace NBZip2 {

bool IsEndSig(const Byte *p) throw();
bool IsBlockSig(const Byte *p) throw();

const unsigned kNumTableBits = 9;

typedef NHuffman::CDecoder<kMaxHuffmanLen, kMaxAlphaSize, kNumTableBits> CHuffmanDecoder;


struct CBlockProps
{
  UInt32 blockSize;
  UInt32 origPtr;
  unsigned randMode;
  
  CBlockProps(): blockSize(0), origPtr(0), randMode(0) {}
};


struct CBitDecoder
{
  unsigned _numBits;
  UInt32 _value;
  const Byte *_buf;
  const Byte *_lim;

  void InitBitDecoder()
  {
    _numBits = 0;
    _value = 0;
  }

  void AlignToByte()
  {
    unsigned bits = _numBits & 7;
    _numBits -= bits;
    _value <<= bits;
  }

  /*
  bool AreRemainByteBitsEmpty() const
  {
    unsigned bits = _numBits & 7;
    if (bits != 0)
      return (_value >> (32 - bits)) == 0;
    return true;
  }
  */

  SRes ReadByte(int &b);

  CBitDecoder():
    _buf(NULL),
    _lim(NULL)
  {
    InitBitDecoder();
  }
};


// 19.03: we allow additional 8 selectors to support files created by lbzip2.
const UInt32 kNumSelectorsMax_Decoder = kNumSelectorsMax + 8;

struct CBase: public CBitDecoder
{
  unsigned numInUse;
  UInt32 groupIndex;
  UInt32 groupSize;
  unsigned runPower;
  UInt32 runCounter;
  UInt32 blockSize;

  UInt32 *Counters;
  UInt32 blockSizeMax;

  unsigned state;
  UInt32 state2;
  unsigned state3;
  unsigned state4;
  unsigned state5;
  unsigned numTables;
  UInt32 numSelectors;

  CBlockProps Props;

private:
  CMtf8Decoder mtf;
  Byte selectors[kNumSelectorsMax_Decoder];
  CHuffmanDecoder huffs[kNumTablesMax];

  Byte lens[kMaxAlphaSize];

  Byte temp[10];

public:
  UInt32 crc;
  CBZip2CombinedCrc CombinedCrc;
  
  bool IsBz;
  bool StreamCrcError;
  bool MinorError;
  bool NeedMoreInput;

  bool DecodeAllStreams;
  
  UInt64 NumStreams;
  UInt64 NumBlocks;
  UInt64 FinishedPackSize;

  ISequentialInStream *InStream;

  #ifndef Z7_NO_READ_FROM_CODER
  CMyComPtr<ISequentialInStream> InStreamRef;
  #endif

  CBase():
      StreamCrcError(false),
      MinorError(false),
      NeedMoreInput(false),

      DecodeAllStreams(false),
      
      NumStreams(0),
      NumBlocks(0),
      FinishedPackSize(0)
      {}

  void InitNumStreams2()
  {
    StreamCrcError = false;
    MinorError = false;
    NeedMoreInput = 0;
    NumStreams = 0;
    NumBlocks = 0;
    FinishedPackSize = 0;
  }

  SRes ReadStreamSignature2();
  SRes ReadBlockSignature2();

  /* ReadBlock2() : Props->randMode:
       in:  need read randMode bit
       out: randMode status */
  SRes ReadBlock2();
};


class CSpecState
{
  UInt32 _tPos;
  unsigned _prevByte;
  int _reps;

public:
  CBZip2Crc _crc;
  UInt32 _blockSize;
  UInt32 *_tt;

  int _randToGo;
  unsigned _randIndex;

  void Init(UInt32 origPtr, unsigned randMode) throw();

  bool Finished() const { return _reps <= 0 && _blockSize == 0; }

  Byte *Decode(Byte *data, size_t size) throw();
};


  
 
class CDecoder:
  public ICompressCoder,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
  public ICompressReadUnusedFromInBuf,
#ifndef Z7_NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
#endif
#ifndef Z7_ST
  public ICompressSetCoderMt,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetFinishMode)
  Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize)
  Z7_COM_QI_ENTRY(ICompressReadUnusedFromInBuf)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_COM_QI_ENTRY(ICompressSetInStream)
  Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
  Z7_COM_QI_ENTRY(ISequentialInStream)
#endif
#ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  Z7_IFACE_COM7_IMP(ICompressReadUnusedFromInBuf)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP_NONFINAL(ISequentialInStream)
#endif
public:
#ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
#endif

private:
  Byte *_outBuf;
  size_t _outPos;
  UInt64 _outWritten;
  ISequentialOutStream *_outStream;
  HRESULT _writeRes;

protected:
  HRESULT ErrorResult; // for ISequentialInStream::Read mode only

public:

  UInt32 _calcedBlockCrc;
  bool _blockFinished;
  bool BlockCrcError;

  bool FinishMode;
  bool _outSizeDefined;
  UInt64 _outSize;
  UInt64 _outPosTotal;

  CSpecState _spec;
  UInt32 *_counters;

  #ifndef Z7_ST

  struct CBlock
  {
    bool StopScout;
    
    bool WasFinished;
    bool Crc_Defined;
    // bool NextCrc_Defined;
    
    UInt32 Crc;
    UInt32 NextCrc;
    HRESULT Res;
    UInt64 PackPos;
    
    CBlockProps Props;
  };

  CBlock _block;

  bool NeedWaitScout;
  bool MtMode;

  NWindows::CThread Thread;
  NWindows::NSynchronization::CAutoResetEvent DecoderEvent;
  NWindows::NSynchronization::CAutoResetEvent ScoutEvent;
  // HRESULT ScoutRes;
  
  Byte MtPad[1 << 7]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.


  void RunScout();

  void WaitScout()
  {
    if (NeedWaitScout)
    {
      DecoderEvent.Lock();
      NeedWaitScout = false;
    }
  }

  class CWaitScout_Releaser
  {
    CDecoder *_decoder;
  public:
    CWaitScout_Releaser(CDecoder *decoder): _decoder(decoder) {}
    ~CWaitScout_Releaser() { _decoder->WaitScout(); }
  };

  HRESULT CreateThread();

  #endif

  Byte *_inBuf;
  UInt64 _inProcessed;
  bool _inputFinished;
  HRESULT _inputRes;

  CBase Base;

  bool GetCrcError() const { return BlockCrcError || Base.StreamCrcError; }

  void InitOutSize(const UInt64 *outSize);
  
  bool CreateInputBufer();

  void InitInputBuffer()
  {
    // We use InitInputBuffer() before stream init.
    // So don't read from stream here
    _inProcessed = 0;
    Base._buf = _inBuf;
    Base._lim = _inBuf;
    Base.InitBitDecoder();
  }

  UInt64 GetInputProcessedSize() const
  {
    // for NSIS case : we need also look the number of bits in bitDecoder
    return _inProcessed + (size_t)(Base._buf - _inBuf);
  }

  UInt64 GetInStreamSize() const
  {
    return _inProcessed + (size_t)(Base._buf - _inBuf) - (Base._numBits >> 3);
  }

  UInt64 GetOutProcessedSize() const { return _outWritten + _outPos; }

  HRESULT ReadInput();

  void StartNewStream();
  
  HRESULT ReadStreamSignature();
  HRESULT StartRead();

  HRESULT ReadBlockSignature();
  HRESULT ReadBlock();

  HRESULT Flush();
  HRESULT DecodeBlock(const CBlockProps &props);
  HRESULT DecodeStreams(ICompressProgressInfo *progress);

  UInt64 GetNumStreams() const { return Base.NumStreams; }
  UInt64 GetNumBlocks() const { return Base.NumBlocks; }

  CDecoder();
  virtual ~CDecoder();
};



#ifndef Z7_NO_READ_FROM_CODER

class CNsisDecoder Z7_final: public CDecoder
{
  Z7_IFACE_COM7_IMP(ISequentialInStream)
};

#endif

}}

#endif
