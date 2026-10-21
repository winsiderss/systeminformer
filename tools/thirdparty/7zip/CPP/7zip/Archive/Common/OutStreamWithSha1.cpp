// OutStreamWithSha1.cpp

#include "StdAfx.h"

#include "OutStreamWithSha1.h"

Z7_COM7F_IMF(COutStreamWithSha1::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  if (_calculate)
    Sha1_Update(Sha(), (const Byte *)data, size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

Z7_COM7F_IMF(CInStreamWithSha1::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize;
  const HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  Sha1_Update(Sha(), (const Byte *)data, realProcessedSize);
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}
