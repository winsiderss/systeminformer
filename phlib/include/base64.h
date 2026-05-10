/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

#ifndef _PH_BASE64_H
#define _PH_BASE64_H

PHLIBAPI
BOOLEAN
PhBase64Encode(
    _In_reads_bytes_(InputLength) const UCHAR* Input,
    _In_ SIZE_T InputLength,
    _Out_writes_z_(OutputLength) PSTR Output,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ResultLength
    );

PHLIBAPI
BOOLEAN
PhBase64Decode(
    _In_reads_(InputLength) PCSTR Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(OutputLength) PUCHAR Output,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ResultLength
    );

#endif
