// MemBlocks.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "MemBlocks.h"
#include "StreamUtils.h"

bool CMemBlockManager::AllocateSpace_bool(size_t numBlocks)
{
  FreeSpace();
  if (numBlocks == 0)
  {
    return true;
    // return false;
  }
  if (_blockSize < sizeof(void *))
    return false;
  const size_t totalSize = numBlocks * _blockSize;
  if (totalSize / _blockSize != numBlocks)
    return false;
  _data = ::MidAlloc(totalSize);
  if (!_data)
    return false;
  Byte *p = (Byte *)_data;
  for (size_t i = 0; i + 1 < numBlocks; i++, p += _blockSize)
    *(Byte **)(void *)p = (p + _blockSize);
  *(Byte **)(void *)p = NULL;
  _headFree = _data;
  return true;
}

void CMemBlockManager::FreeSpace()
{
  ::MidFree(_data);
  _data = NULL;
  _headFree= NULL;
}

void *CMemBlockManager::AllocateBlock()
{
  void *p = _headFree;
  if (p)
    _headFree = *(void **)p;
  return p;
}

void CMemBlockManager::FreeBlock(void *p)
{
  if (!p)
    return;
  *(void **)p = _headFree;
  _headFree = p;
}


// #include <stdio.h>

HRESULT CMemBlockManagerMt::AllocateSpace(size_t numBlocks, size_t numNoLockBlocks)
{
  if (numNoLockBlocks > numBlocks)
    return E_INVALIDARG;
  const size_t numLockBlocks = numBlocks - numNoLockBlocks;
  UInt32 maxCount = (UInt32)numLockBlocks;
  if (maxCount != numLockBlocks)
    return E_OUTOFMEMORY;
  if (!CMemBlockManager::AllocateSpace_bool(numBlocks))
    return E_OUTOFMEMORY;
  // we need (maxCount = 1), if we want to create non-use empty Semaphore
  if (maxCount == 0)
    maxCount = 1;

  // printf("\n Synchro.Create() \n");
  WRes wres;
  #ifndef _WIN32
  Semaphore.Close();
  wres = Synchro.Create();
  if (wres != 0)
    return HRESULT_FROM_WIN32(wres);
  wres = Semaphore.Create(&Synchro, (UInt32)numLockBlocks, maxCount);
  #else
  wres = Semaphore.OptCreateInit((UInt32)numLockBlocks, maxCount);
  #endif
  
  return HRESULT_FROM_WIN32(wres);
}


HRESULT CMemBlockManagerMt::AllocateSpaceAlways(size_t desiredNumberOfBlocks, size_t numNoLockBlocks)
{
  // desiredNumberOfBlocks = 0; // for debug
  if (numNoLockBlocks > desiredNumberOfBlocks)
    return E_INVALIDARG;
  for (;;)
  {
    // if (desiredNumberOfBlocks == 0) return E_OUTOFMEMORY;
    const HRESULT hres = AllocateSpace(desiredNumberOfBlocks, numNoLockBlocks);
    if (hres != E_OUTOFMEMORY)
      return hres;
    if (desiredNumberOfBlocks == numNoLockBlocks)
      return E_OUTOFMEMORY;
    desiredNumberOfBlocks = numNoLockBlocks + ((desiredNumberOfBlocks - numNoLockBlocks) >> 1);
  }
}

void CMemBlockManagerMt::FreeSpace()
{
  Semaphore.Close();
  CMemBlockManager::FreeSpace();
}

void *CMemBlockManagerMt::AllocateBlock()
{
  // Semaphore.Lock();
  NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
  return CMemBlockManager::AllocateBlock();
}

void CMemBlockManagerMt::FreeBlock(void *p, bool lockMode)
{
  if (!p)
    return;
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    CMemBlockManager::FreeBlock(p);
  }
  if (lockMode)
    Semaphore.Release();
}



void CMemBlocks::Free(CMemBlockManagerMt *manager)
{
  while (Blocks.Size() > 0)
  {
    manager->FreeBlock(Blocks.Back());
    Blocks.DeleteBack();
  }
  TotalSize = 0;
}

void CMemBlocks::FreeOpt(CMemBlockManagerMt *manager)
{
  Free(manager);
  Blocks.ClearAndFree();
}

HRESULT CMemBlocks::WriteToStream(size_t blockSize, ISequentialOutStream *outStream) const
{
  UInt64 totalSize = TotalSize;
  for (unsigned blockIndex = 0; totalSize > 0; blockIndex++)
  {
    size_t curSize = blockSize;
    if (curSize > totalSize)
      curSize = (size_t)totalSize;
    if (blockIndex >= Blocks.Size())
      return E_FAIL;
    RINOK(WriteStream(outStream, Blocks[blockIndex], curSize))
    totalSize -= curSize;
  }
  return S_OK;
}


void CMemLockBlocks::FreeBlock(unsigned index, CMemBlockManagerMt *memManager)
{
  memManager->FreeBlock(Blocks[index], LockMode);
  Blocks[index] = NULL;
}

void CMemLockBlocks::Free(CMemBlockManagerMt *memManager)
{
  while (Blocks.Size() > 0)
  {
    FreeBlock(Blocks.Size() - 1, memManager);
    Blocks.DeleteBack();
  }
  TotalSize = 0;
}

/*
HRes CMemLockBlocks::SwitchToNoLockMode(CMemBlockManagerMt *memManager)
{
  if (LockMode)
  {
    if (Blocks.Size() > 0)
    {
      RINOK(memManager->ReleaseLockedBlocks(Blocks.Size()));
    }
    LockMode = false;
  }
  return 0;
}
*/

void CMemLockBlocks::Detach(CMemLockBlocks &blocks, CMemBlockManagerMt *memManager)
{
  blocks.Free(memManager);
  blocks.LockMode = LockMode;
  UInt64 totalSize = 0;
  const size_t blockSize = memManager->GetBlockSize();
  FOR_VECTOR (i, Blocks)
  {
    if (totalSize < TotalSize)
      blocks.Blocks.Add(Blocks[i]);
    else
      FreeBlock(i, memManager);
    Blocks[i] = NULL;
    totalSize += blockSize;
  }
  blocks.TotalSize = TotalSize;
  Free(memManager);
}
