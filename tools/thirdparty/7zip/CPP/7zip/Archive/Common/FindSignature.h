// FindSignature.h

#ifndef ZIP7_INC_FIND_SIGNATURE_H
#define ZIP7_INC_FIND_SIGNATURE_H

#include "../../IStream.h"

HRESULT FindSignatureInStream(ISequentialInStream *stream,
    const Byte *signature, unsigned signatureSize,
    const UInt64 *limit, UInt64 &resPos);

#endif
