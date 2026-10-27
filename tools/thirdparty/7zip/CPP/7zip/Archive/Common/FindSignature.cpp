// FindSignature.cpp

#include "StdAfx.h"

#include <string.h>

#include "../../../Common/MyBuffer.h"

#include "../../Common/StreamUtils.h"

#include "FindSignature.h"

HRESULT FindSignatureInStream(ISequentialInStream *stream,
    const Byte *signature, unsigned signatureSize,
    const UInt64 *limit, UInt64 &resPos)
{
  resPos = 0;
  CByteBuffer byteBuffer2(signatureSize);
  RINOK(ReadStream_FALSE(stream, byteBuffer2, signatureSize))

  if (memcmp(byteBuffer2, signature, signatureSize) == 0)
    return S_OK;

  const size_t kBufferSize = 1 << 16;
  CByteBuffer byteBuffer(kBufferSize);
  Byte *buffer = byteBuffer;
  size_t numPrevBytes = signatureSize - 1;
  memcpy(buffer, (const Byte *)byteBuffer2 + 1, numPrevBytes);
  resPos = 1;
  for (;;)
  {
    if (limit)
      if (resPos > *limit)
        return S_FALSE;
    do
    {
      const size_t numReadBytes = kBufferSize - numPrevBytes;
      UInt32 processedSize;
      RINOK(stream->Read(buffer + numPrevBytes, (UInt32)numReadBytes, &processedSize))
      numPrevBytes += (size_t)processedSize;
      if (processedSize == 0)
        return S_FALSE;
    }
    while (numPrevBytes < signatureSize);
    const size_t numTests = numPrevBytes - signatureSize + 1;
    for (size_t pos = 0; pos < numTests; pos++)
    {
      const Byte b = signature[0];
      for (; buffer[pos] != b && pos < numTests; pos++);
      if (pos == numTests)
        break;
      if (memcmp(buffer + pos, signature, signatureSize) == 0)
      {
        resPos += pos;
        return S_OK;
      }
    }
    resPos += numTests;
    numPrevBytes -= numTests;
    memmove(buffer, buffer + numTests, numPrevBytes);
  }
}

namespace NArchive {
HRESULT ReadZeroTail(ISequentialInStream *stream, bool &areThereNonZeros, UInt64 &numZeros, UInt64 maxSize);
HRESULT ReadZeroTail(ISequentialInStream *stream, bool &areThereNonZeros, UInt64 &numZeros, UInt64 maxSize)
{
  areThereNonZeros = false;
  numZeros = 0;
  const size_t kBufSize = 1 << 11;
  Byte buf[kBufSize];
  for (;;)
  {
    UInt32 size = 0;
    RINOK(stream->Read(buf, kBufSize, &size))
    if (size == 0)
      return S_OK;
    for (UInt32 i = 0; i < size; i++)
      if (buf[i] != 0)
      {
        areThereNonZeros = true;
        numZeros += i;
        return S_OK;
      }
    numZeros += size;
    if (numZeros > maxSize)
      return S_OK;
  }
}
}
