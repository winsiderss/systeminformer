// OutMemStream.h

#ifndef ZIP7_INC_OUT_MEM_STREAM_H
#define ZIP7_INC_OUT_MEM_STREAM_H

#include "../../Common/MyCom.h"

#include "MemBlocks.h"

Z7_CLASS_IMP_NOQIB_1(
  COutMemStream
  , IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  CMemBlockManagerMt *_memManager;
  size_t _curBlockPos;
  unsigned _curBlockIndex;
  bool _realStreamMode;

  bool _unlockEventWasSent;
  NWindows::NSynchronization::CAutoResetEvent_WFMO StopWritingEvent;
  NWindows::NSynchronization::CAutoResetEvent_WFMO WriteToRealStreamEvent;
  // NWindows::NSynchronization::CAutoResetEvent NoLockEvent;

  HRESULT StopWriteResult;
  CMemLockBlocks Blocks;

  CMyComPtr<ISequentialOutStream> OutSeqStream;
  CMyComPtr<IOutStream> OutStream;

  UInt64 GetPos() const { return (UInt64)_curBlockIndex * _memManager->GetBlockSize() + _curBlockPos; }

public:

  HRESULT CreateEvents(SYNC_PARAM_DECL(synchro))
  {
    WRes wres = StopWritingEvent.CreateIfNotCreated_Reset(SYNC_WFMO(synchro));
    if (wres == 0)
      wres = WriteToRealStreamEvent.CreateIfNotCreated_Reset(SYNC_WFMO(synchro));
    return HRESULT_FROM_WIN32(wres);
  }

  void SetOutStream(IOutStream *outStream)
  {
    OutStream = outStream;
    OutSeqStream = outStream;
  }

  void SetSeqOutStream(ISequentialOutStream *outStream)
  {
    OutStream = NULL;
    OutSeqStream = outStream;
  }

  void ReleaseOutStream()
  {
    OutStream.Release();
    OutSeqStream.Release();
  }

  COutMemStream(CMemBlockManagerMt *memManager):
      _memManager(memManager)
  {
    /*
    #ifndef _WIN32
    StopWritingEvent._sync       =
    WriteToRealStreamEvent._sync =  &memManager->Synchro;
    #endif
    */
  }

  ~COutMemStream() { Free(); }
  void Free();

  void Init();
  HRESULT WriteToRealStream();

  void DetachData(CMemLockBlocks &blocks);

  bool WasUnlockEventSent() const { return _unlockEventWasSent; }

  void SetRealStreamMode()
  {
    _unlockEventWasSent = true;
    WriteToRealStreamEvent.Set();
  }

  /*
  void SetNoLockMode()
  {
    _unlockEventWasSent = true;
    NoLockEvent.Set();
  }
  */

  void StopWriting(HRESULT res)
  {
    StopWriteResult = res;
    StopWritingEvent.Set();
  }
};

#endif
