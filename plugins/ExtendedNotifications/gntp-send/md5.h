#ifndef _MD5_H_
#define _MD5_H_

#include <stdint.h>

typedef struct {
    PH_HASH_CONTEXT hc;
} md5_context;
 
__forceinline void md5_starts(md5_context *ctx)
{
    PhInitializeHash(&ctx->hc, Md5HashAlgorithm);
}

__forceinline void md5_update(md5_context *ctx, const uint8_t *input, uint32_t length)
{
    PhUpdateHash(&ctx->hc, (PVOID)input, length);
}

__forceinline void md5_finish(md5_context *ctx, uint8_t digest[16])
{
    if (!PhFinalHash(&ctx->hc, digest, 16, NULL))
        PhRaiseStatus(STATUS_INTERNAL_ERROR);
}

#endif /* _MD5_H_ */
