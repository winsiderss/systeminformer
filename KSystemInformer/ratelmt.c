/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <kph.h>

#include <trace.h>

//
// Time granularity for rate limiter timestamps (100-nano units per tick).
// 10,000,000 / 100 = 100 ms granularity.
// 2^32 * 100 ms ~= 13.62 years before rollover.
// 100 ms granularity provides a good balance of fill frequency and rollover.
//
#define KPH_RATE_LIMIT_SEC_MULT   (100)
#define KPH_RATE_LIMIT_TIME_UNIT  (10000000 / KPH_RATE_LIMIT_SEC_MULT)
#define KPH_RATE_LIMIT_ROLL_CHECK (86400 * KPH_RATE_LIMIT_SEC_MULT)

/**
 * \brief Initializes a rate limit.
 *
 * \param[in] Policy The rate limit policy to use.
 * \param[in] TimeStamp The current time stamp.
 * \param[in] RateLimit Receives the initialized rate limit.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphInitializeRateLimit(
    _In_ PCKPH_RATE_LIMIT_POLICY Policy,
    _In_ PLARGE_INTEGER TimeStamp,
    _Out_ PKPH_RATE_LIMIT RateLimit
    )
{
    KPH_RATE_BUCKET bucket;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    bucket.CurrentTokens = Policy->MaxBucketSize;
    bucket.LastRefillTime = (ULONG)(TimeStamp->QuadPart / KPH_RATE_LIMIT_TIME_UNIT);

    RtlZeroMemory(RateLimit, sizeof(KPH_RATE_LIMIT));
    WriteNoFence64(&RateLimit->Bucket.Quad, bucket.Quad);
    RtlCopyMemory(&RateLimit->Policy, Policy, sizeof(KPH_RATE_LIMIT_POLICY));
}

/**
 * \brief Consumes a token from the rate limit.
 *
 * \param[in] RateLimit The rate limit to consume a token from.
 * \param[in] TimeStamp The current time stamp.
 *
 * \return TRUE if a token was consumed, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphRateLimitConsumeToken(
    _Inout_ PKPH_RATE_LIMIT RateLimit,
    _In_ PLARGE_INTEGER TimeStamp
    )
{
    KPH_RATE_BUCKET oldBucket;
    KPH_RATE_BUCKET newBucket;
    ULONG currentTime;
    ULONG elapsedTime;
    ULONG64 tokensToAdd;
    BOOLEAN allowed;
    LONG64 expected;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    //
    // KPH_RATE_LIMIT_UNLIMITED
    //
    if ((RateLimit->Policy.TokensPerPeriod == ULONG_MAX) ||
        (RateLimit->Policy.MaxBucketSize == ULONG_MAX))
    {
        InterlockedIncrementNoFence64(&RateLimit->Allowed);
        return TRUE;
    }

    //
    // KPH_RATE_LIMIT_DENY_ALL
    //
    if ((RateLimit->Policy.TokensPerPeriod == 0) ||
        (RateLimit->Policy.PeriodInSeconds == 0) ||
        (RateLimit->Policy.MaxBucketSize == 0))
    {
        InterlockedIncrementNoFence64(&RateLimit->Dropped);
        return FALSE;
    }

    currentTime = (ULONG)(TimeStamp->QuadPart / KPH_RATE_LIMIT_TIME_UNIT);
    oldBucket.Quad = ReadNoFence64(&RateLimit->Bucket.Quad);

    for (;;)
    {
        newBucket.Quad = oldBucket.Quad;

        if (newBucket.LastRefillTime > currentTime)
        {
            if ((newBucket.LastRefillTime - currentTime) > KPH_RATE_LIMIT_ROLL_CHECK)
            {
                newBucket.CurrentTokens = RateLimit->Policy.MaxBucketSize;
                newBucket.LastRefillTime = currentTime;
            }

            goto ConsumeToken;
        }

        elapsedTime = (currentTime - newBucket.LastRefillTime);
        tokensToAdd = ((ULONG64)elapsedTime * RateLimit->Policy.TokensPerPeriod);
        tokensToAdd /= ((ULONG64)RateLimit->Policy.PeriodInSeconds * KPH_RATE_LIMIT_SEC_MULT);

        if (tokensToAdd == 0)
        {
            goto ConsumeToken;
        }

        if ((tokensToAdd >= RateLimit->Policy.MaxBucketSize) ||
            (tokensToAdd > (RateLimit->Policy.MaxBucketSize - newBucket.CurrentTokens)))
        {
            newBucket.CurrentTokens = RateLimit->Policy.MaxBucketSize;
        }
        else
        {
            newBucket.CurrentTokens = (newBucket.CurrentTokens + (ULONG)tokensToAdd);
        }

        newBucket.LastRefillTime = currentTime;

ConsumeToken:

        if (newBucket.CurrentTokens > 0)
        {
            newBucket.CurrentTokens--;
            allowed = TRUE;
        }
        else
        {
            allowed = FALSE;
        }

        expected = oldBucket.Quad;

        oldBucket.Quad = InterlockedCompareExchange64(&RateLimit->Bucket.Quad,
                                                      newBucket.Quad,
                                                      expected);
        if (oldBucket.Quad != expected)
        {
            InterlockedIncrementNoFence64(&RateLimit->CasMiss);
            continue;
        }

        if (allowed)
        {
            InterlockedIncrementNoFence64(&RateLimit->Allowed);
            return TRUE;
        }
        else
        {
            InterlockedIncrementNoFence64(&RateLimit->Dropped);
            return FALSE;
        }
    }
}
