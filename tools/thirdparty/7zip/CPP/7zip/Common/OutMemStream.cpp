// OutMemStream.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "OutMemStream.h"

void COutMemStream::Free()
{
  Blocks.Free(_memManager);
  Blocks.LockMode = true;
}

void COutMemStream::Init()
{
  WriteToRealStreamEvent.Reset();
  _unlockEventWasSent = false;
  _realStreamMode = false;
  Free();
  _curBlockPos = 0;
  _curBlockIndex = 0;
}

void COutMemStream::DetachData(CMemLockBlocks &blocks)
{
  Blocks.Detach(blocks, _memManager);
  Free();
}


HRESULT COutMemStream::WriteToRealStream()
{
  RINOK(Blocks.WriteToStream(_memManager->GetBlockSize(), OutSeqStream))
  Blocks.Free(_memManager);
  return S_OK;
}


Z7_COM7F_IMF(COutMemStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (_realStreamMode)
    return OutSeqStream->Write(data, size, processedSize);
  if (processedSize)
    *processedSize = 0;
  while (size != 0)
  {
    if (_curBlockIndex < Blocks.Blocks.Size())
    {
      Byte *p = (Byte *)Blocks.Blocks[_curBlockIndex] + _curBlockPos;
      size_t curSize = _memManager->GetBlockSize() - _curBlockPos;
      if (size < curSize)
        curSize = size;
      memcpy(p, data, curSize);
      if (processedSize)
        *processedSize += (UInt32)curSize;
      data = (const void *)((const Byte *)data + curSize);
      size -= (UInt32)curSize;
      _curBlockPos += curSize;

      const UInt64 pos64 = GetPos();
      if (pos64 > Blocks.TotalSize)
        Blocks.TotalSize = pos64;
      if (_curBlockPos == _memManager->GetBlockSize())
      {
        _curBlockIndex++;
        _curBlockPos = 0;
      }
      continue;
    }

    const NWindows::NSynchronization::CHandle_WFMO events[3] =
      { StopWritingEvent, WriteToRealStreamEvent, /* NoLockEvent, */ _memManager->Semaphore };
    const DWORD waitResult = NWindows::NSynchronization::WaitForMultiObj_Any_Infinite(
        ((Blocks.LockMode /* && _memManager->Semaphore.IsCreated() */) ? 3 : 2), events);
    
    // printf("\n 1- outMemStream %d\n", waitResult - WAIT_OBJECT_0);

    switch (waitResult)
    {
      case (WAIT_OBJECT_0 + 0):
        return StopWriteResult;
      case (WAIT_OBJECT_0 + 1):
      {
        _realStreamMode = true;
        RINOK(WriteToRealStream())
        UInt32 processedSize2;
        const HRESULT res = OutSeqStream->Write(data, size, &processedSize2);
        if (processedSize)
          *processedSize += processedSize2;
        return res;
      }
      case (WAIT_OBJECT_0 + 2):
      {
        // it has bug: no write.
        /*
        if (!Blocks.SwitchToNoLockMode(_memManager))
          return E_FAIL;
        */
        break;
      }
      default:
      {
        if (waitResult == WAIT_FAILED)
        {
          const DWORD res = ::GetLastError();
          if (res != 0)
            return HRESULT_FROM_WIN32(res);
        }
        return E_FAIL;
      }
    }
    void *p = _memManager->AllocateBlock();
    if (!p)
      return E_FAIL;
    Blocks.Blocks.Add(p);
  }
  return S_OK;
}

Z7_COM7F_IMF(COutMemStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  if (_realStreamMode)
  {
    if (!OutStream)
      return E_FAIL;
    return OutStream->Seek(offset, seekOrigin, newPosition);
  }
  if (seekOrigin == STREAM_SEEK_CUR)
  {
    if (offset != 0)
      return E_NOTIMPL;
  }
  else if (seekOrigin == STREAM_SEEK_SET)
  {
    if (offset != 0)
      return E_NOTIMPL;
    _curBlockIndex = 0;
    _curBlockPos = 0;
  }
  else
    return E_NOTIMPL;
  if (newPosition)
    *newPosition = GetPos();
  return S_OK;
}

Z7_COM7F_IMF(COutMemStream::SetSize(UInt64 newSize))
{
  if (_realStreamMode)
  {
    if (!OutStream)
      return E_FAIL;
    return OutStream->SetSize(newSize);
  }
  Blocks.TotalSize = newSize;
  return S_OK;
}
