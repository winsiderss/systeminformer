/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#ifndef _ONLINECHECKSINTF_H
#define _ONLINECHECKSINTF_H

#define ONLINECHECKS_PLUGIN_NAME L"OnlineChecks"
#define ONLINECHECKS_INTERFACE_VERSION 1

/**
 * Why a cached lookup returned what it did.
 *
 * \remarks Not a boolean, because "no verdict for this file" and "there is nowhere to look" are
 * different answers and only one of them means anything about the file. OnlineChecks opens its
 * database only when scanning is enabled, so a caller that treats an empty result as "not known to
 * any scanner" is wrong every time scanning is switched off.
 */
typedef enum _ONLINECHECKS_LOOKUP_RESULT
{
    OnlineChecksLookupFound,        // A cached verdict for this hash.
    OnlineChecksLookupNotFound,     // The database is open and holds nothing for this hash.
    OnlineChecksLookupUnavailable,  // Scanning is disabled, so there is no database to look in.
} ONLINECHECKS_LOOKUP_RESULT, *PONLINECHECKS_LOOKUP_RESULT;

/**
 * A cached VirusTotal verdict.
 *
 * \remarks HttpStatus is the status of the response that produced the row: 200 for a verdict, 404
 * for a file VirusTotal has never seen, and anything else for a failure that was cached to avoid
 * asking again. Malicious and Undetected are only meaningful when it is 200.
 */
typedef struct _ONLINECHECKS_VIRUSTOTAL_RESULT
{
    ULONG HttpStatus;
    LARGE_INTEGER Expiry;   // When the row stops being used; it is not deleted at that point.
    ULONG64 Malicious;      // Engines that flagged the file.
    ULONG64 Undetected;     // Engines that did not.
} ONLINECHECKS_VIRUSTOTAL_RESULT, *PONLINECHECKS_VIRUSTOTAL_RESULT;

/**
 * A cached Hybrid Analysis verdict.
 */
typedef struct _ONLINECHECKS_HYBRIDANALYSIS_RESULT
{
    ULONG HttpStatus;
    LARGE_INTEGER Expiry;
    ULONG64 MultiscanResult;    // Percentage of engines that flagged the file.
    PPH_STRING VxFamily;        // Referenced; the caller dereferences it. NULL when there is none.
} ONLINECHECKS_HYBRIDANALYSIS_RESULT, *PONLINECHECKS_HYBRIDANALYSIS_RESULT;

/**
 * Reads a cached VirusTotal verdict for a file hash.
 *
 * \param Sha256 The file's SHA-256, as hexadecimal text.
 * \param Result The cached verdict. Written only when the return is OnlineChecksLookupFound.
 * \return Whether a verdict was found, none was cached, or there was nowhere to look.
 *
 * \remarks Reads the local database only. Never makes a network request, and never queues a scan.
 */
typedef ONLINECHECKS_LOOKUP_RESULT (NTAPI* PONLINECHECKS_QUERY_VIRUSTOTAL)(
    _In_ PPH_STRING Sha256,
    _Out_ PONLINECHECKS_VIRUSTOTAL_RESULT Result
    );

/**
 * Reads a cached Hybrid Analysis verdict for a file hash.
 *
 * \param Sha256 The file's SHA-256, as hexadecimal text.
 * \param Result The cached verdict. Written only when the return is OnlineChecksLookupFound.
 * \return Whether a verdict was found, none was cached, or there was nowhere to look.
 *
 * \remarks Reads the local database only. Never makes a network request, and never queues a scan.
 */
typedef ONLINECHECKS_LOOKUP_RESULT (NTAPI* PONLINECHECKS_QUERY_HYBRIDANALYSIS)(
    _In_ PPH_STRING Sha256,
    _Out_ PONLINECHECKS_HYBRIDANALYSIS_RESULT Result
    );

typedef struct _ONLINECHECKS_INTERFACE
{
    ULONG Version;
    PONLINECHECKS_QUERY_VIRUSTOTAL QueryCachedVirusTotal;
    PONLINECHECKS_QUERY_HYBRIDANALYSIS QueryCachedHybridAnalysis;
} ONLINECHECKS_INTERFACE, *PONLINECHECKS_INTERFACE;

#endif
