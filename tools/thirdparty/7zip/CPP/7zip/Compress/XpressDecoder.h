// XpressDecoder.h

#ifndef ZIP7_INC_XPRESS_DECODER_H
#define ZIP7_INC_XPRESS_DECODER_H

#include "../../Common/MyTypes.h"

namespace NCompress {
namespace NXpress {

// (out) buffer size must be larger than (outSize) for the following value:
const unsigned kAdditionalOutputBufSize = 32;

HRESULT Decode_WithExceedWrite(const Byte *in, size_t inSize, Byte *out, size_t outSize);

}}

#endif
