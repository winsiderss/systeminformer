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
#define ONLINECHECKS_INTERFACE_VERSION 2

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

/**
 * A live VirusTotal report.
 *
 * \remarks The counts mean something only when HttpStatus is 200. 404 is a file VirusTotal has never
 * been given, which is not the same as a clean verdict.
 */
typedef struct _ONLINECHECKS_VIRUSTOTAL_REPORT
{
    ULONG HttpStatus;
    ULONG64 Malicious;
    ULONG64 Undetected;
    PPH_STRING ScanDate;    // Referenced; the caller dereferences it. NULL when there is none.
} ONLINECHECKS_VIRUSTOTAL_REPORT, *PONLINECHECKS_VIRUSTOTAL_REPORT;

/**
 * A live Hybrid Analysis report.
 */
typedef struct _ONLINECHECKS_HYBRIDANALYSIS_REPORT
{
    ULONG HttpStatus;
    ULONG64 ThreatScore;
    ULONG64 MultiscanResult;
    PPH_STRING Verdict;     // Referenced; the caller dereferences it. NULL when there is none.
    PPH_STRING VxFamily;    // Referenced; the caller dereferences it. NULL when there is none.
} ONLINECHECKS_HYBRIDANALYSIS_REPORT, *PONLINECHECKS_HYBRIDANALYSIS_REPORT;

/**
 * Asks VirusTotal about a file hash, over the network.
 *
 * \param Sha256 The file's SHA-256, as hexadecimal text.
 * \param Report The report. Written only on success.
 * \return Successful or errant status.
 *
 * \remarks THIS SENDS A REQUEST. Only the hash is sent, never the file. With no personal access
 * token configured the request goes to System Informer's proxy at systeminformer.io carrying an
 * installation identifier; with one it goes to VirusTotal directly. Either way it leaves the
 * machine, and a hash is enough to tell a third party that this machine holds this exact file.
 */
typedef NTSTATUS (NTAPI* PONLINECHECKS_LOOKUP_VIRUSTOTAL)(
    _In_ PPH_STRING Sha256,
    _Out_ PONLINECHECKS_VIRUSTOTAL_REPORT Report
    );

/**
 * Asks Hybrid Analysis about a file hash, over the network.
 *
 * \param Sha256 The file's SHA-256, as hexadecimal text.
 * \param Report The report. Written only on success.
 * \return Successful or errant status.
 *
 * \remarks THIS SENDS A REQUEST, on the same terms as the VirusTotal one.
 */
typedef NTSTATUS (NTAPI* PONLINECHECKS_LOOKUP_HYBRIDANALYSIS)(
    _In_ PPH_STRING Sha256,
    _Out_ PONLINECHECKS_HYBRIDANALYSIS_REPORT Report
    );

typedef struct _ONLINECHECKS_INTERFACE
{
    ULONG Version;
    PONLINECHECKS_QUERY_VIRUSTOTAL QueryCachedVirusTotal;
    PONLINECHECKS_QUERY_HYBRIDANALYSIS QueryCachedHybridAnalysis;
    PONLINECHECKS_LOOKUP_VIRUSTOTAL LookupVirusTotal;
    PONLINECHECKS_LOOKUP_HYBRIDANALYSIS LookupHybridAnalysis;
} ONLINECHECKS_INTERFACE, *PONLINECHECKS_INTERFACE;

#endif
