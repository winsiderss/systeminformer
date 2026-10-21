// BZip2Encoder.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_ENCODER_H
#define ZIP7_INC_COMPRESS_BZIP2_ENCODER_H

#include "../../Common/MyCom.h"

#ifndef Z7_ST
#include "../../Windows/Synchronization.h"
#include "../../Windows/Thread.h"
#endif

#include "../ICoder.h"

#include "../Common/InBuffer.h"
#include "../Common/OutBuffer.h"

#include "BitmEncoder.h"
#include "BZip2Const.h"
#include "BZip2Crc.h"

namespace NCompress {
namespace NBZip2 {

const unsigned kNumPassesMax = 10;

struct CMsbfEncoderTemp
{
  unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
  unsigned _curByte; // low (_bitPos) bits are zeros
                     // high (8 - _bitPos) bits are filled
  Byte *_buf;
  Byte *_buf_base;
  void SetStream(Byte *buf) { _buf_base = _buf = buf;  }
  Byte *GetStream() const { return _buf_base; }

  void Init()
  {
    _bitPos = 8;
    _curByte = 0;
    _buf = _buf_base;
  }

  // required condition: (value >> numBits) == 0
  // numBits == 0 is allowed
  void WriteBits(UInt32 value, unsigned numBits)
  {
    do
    {
      unsigned bp = _bitPos;
      unsigned curByte = _curByte;
      if (numBits < bp)
      {
        bp -= numBits;
        _curByte = curByte | (value << bp);
        _bitPos = bp;
        return;
      }
      numBits -= bp;
      const UInt32 hi = value >> numBits;
      value -= (hi << numBits);
      Byte *buf = _buf;
      _bitPos = 8;
      _curByte = 0;
      *buf++ = (Byte)(curByte | hi);
      _buf = buf;
    }
    while (numBits);
  }

  void WriteBit(unsigned value)
  {
    const unsigned bp = _bitPos - 1;
    const unsigned curByte = _curByte | (value << bp);
    _curByte = curByte;
    _bitPos = bp;
    if (bp == 0)
    {
      *_buf++ = (Byte)curByte;
      _curByte = 0;
      _bitPos = 8;
    }
  }

  void WriteByte(unsigned b)
  {
    const unsigned bp = _bitPos;
    const unsigned a = _curByte | (b >> (8 - bp));
    _curByte = b << bp;
    Byte *buf = _buf;
    *buf++ = (Byte)a;
    _buf = buf;
  }

  UInt32 GetBytePos() const { return (UInt32)(size_t)(_buf - _buf_base); }
  UInt32 GetPos() const { return GetBytePos() * 8 + 8 - _bitPos; }
  unsigned GetCurByte() const { return _curByte; }
  unsigned GetNonFlushedByteBits() const { return _curByte >> _bitPos; }
  void SetPos(UInt32 bitPos)
  {
    _buf = _buf_base + (bitPos >> 3);
    _bitPos = 8 - ((unsigned)bitPos & 7);
  }
  void SetCurState(unsigned bitPos, unsigned curByte)
  {
    _bitPos = 8 - bitPos;
    _curByte = curByte;
  }
};


class CEncoder;

class CThreadInfo
{
private:
  CMsbfEncoderTemp m_OutStreamCurrent;
public:
  CEncoder *Encoder;
  Byte *m_Block;
private:
  Byte *m_MtfArray;
  Byte *m_TempArray;
  UInt32 *m_BlockSorterIndex;

public:
  bool m_OptimizeNumTables;
  UInt32 m_NumCrcs;
  UInt32 m_BlockIndex;
  UInt64 m_UnpackSize;

  Byte *m_Block_Base;

  Byte Lens[kNumTablesMax][kMaxAlphaSize];
  UInt32 Freqs[kNumTablesMax][kMaxAlphaSize];
  UInt32 Codes[kNumTablesMax][kMaxAlphaSize];

  Byte m_Selectors[kNumSelectorsMax];

  UInt32 m_CRCs[1 << kNumPassesMax];

  void WriteBits2(UInt32 value, unsigned numBits);
  void WriteByte2(unsigned b) { WriteBits2(b, 8); }
  void WriteBit2(unsigned v)  { m_OutStreamCurrent.WriteBit(v); }

  void EncodeBlock(const Byte *block, UInt32 blockSize);
  UInt32 EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize);
  void EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses);
public:
#ifndef Z7_ST
  NWindows::CThread Thread;

  NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;

  // it's not member of this thread. We just need one event per thread
  NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;

public:
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  HRESULT Create();
  void FinishStream(bool needLeave);
  THREAD_FUNC_RET_TYPE ThreadFunc();
#endif

  CThreadInfo(): m_BlockSorterIndex(NULL), m_Block_Base(NULL) {}
  ~CThreadInfo() { Free(); }
  bool Alloc();
  void Free();

  HRESULT EncodeBlock3(UInt32 blockSize);
};


struct CEncProps
{
  UInt32 BlockSizeMult;
  UInt32 NumPasses;
  UInt32 NumThreadGroups;
  UInt64 Affinity;
  
  CEncProps()
  {
    BlockSizeMult = (UInt32)(Int32)-1;
    NumPasses = (UInt32)(Int32)-1;
    NumThreadGroups = 0;
    Affinity = 0;
  }
  void Normalize(int level);
  bool DoOptimizeNumTables() const { return NumPasses > 1; }
};

class CEncoder Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
 #ifndef Z7_ST
  public ICompressSetCoderMt,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetCoderProperties)
 #ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
 #ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
 #endif

 #ifndef Z7_ST
  UInt32 m_NumThreadsPrev;
 #endif
public:
  CInBuffer m_InStream;
 #ifndef Z7_ST
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
 #endif
  CBitmEncoder<COutBuffer> m_OutStream;
  CEncProps _props;
  CBZip2CombinedCrc CombinedCrc;

 #ifndef Z7_ST
  CThreadInfo *ThreadsInfo;
  NWindows::NSynchronization::CManualResetEvent CanProcessEvent;
  NWindows::NSynchronization::CCriticalSection CS;
  UInt32 NumThreads;
  bool MtMode;
  UInt32 NextBlockIndex;

  bool CloseThreads;
  bool StreamWasFinished;
  NWindows::NSynchronization::CManualResetEvent CanStartWaitingEvent;
  CThreadNextGroup ThreadNextGroup;

  HRESULT Result;
  ICompressProgressInfo *Progress;
 #else
  CThreadInfo ThreadsInfo;
 #endif

  UInt64 NumBlocks;

  UInt64 GetInProcessedSize() const { return m_InStream.GetProcessedSize(); }

  UInt32 ReadRleBlock(Byte *buf);
  void WriteBytes(const Byte *data, UInt32 sizeInBits, unsigned lastByteBits);
  void WriteByte(Byte b);

 #ifndef Z7_ST
  HRESULT Create();
  void Free();
 #endif

public:
  CEncoder();
 #ifndef Z7_ST
  ~CEncoder();
 #endif

  HRESULT Flush() { return m_OutStream.Flush(); }
  
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
};

}}

#endif
