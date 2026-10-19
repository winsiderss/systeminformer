/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2026
 *
 */

#include <kph.h>
#include <informer_filep.h>

#include <trace.h>

typedef struct _KPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM
{
    SLIST_ENTRY ListEntry;
    PFLT_INSTANCE Instance;
    PFILE_OBJECT FileObject;
} KPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM, *PKPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpCachedFileNameTypeName, L"KphCachedFileName");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpCachedFileNameType = NULL;
static BOOLEAN KphpFileNameCacheInitialized = FALSE;
KPH_PROTECTED_DATA_SECTION_POP();

//
// We use a simple list and spin lock here since we are very selective about
// what we cache. The list is not expected to grow very large.
//
static ALIGNED_EX_SPINLOCK KphpFileNameCacheLock = 0;
static LIST_ENTRY KphpFileNameCacheList;

static SLIST_HEADER KphpFileNameCachePopulateList;
static KSI_WORK_QUEUE_ITEM KphpFileNameCachePopulateWorkItem;

/**
 * \brief Retrieves file name information using the filter name cache.
 *
 * \param[in] Data The callback data for the operation.
 * \param[out] FltFileName Receives the file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileNameInformation(
    _In_ PFLT_CALLBACK_DATA Data,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    NTSTATUS status;
    ULONG flags;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(Data->Iopb->TargetFileObject);

    flags = (FLT_FILE_NAME_NORMALIZED |
             FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP);

    status = FltGetFileNameInformation(Data, flags, &FltFileName->NameInfo);
    if (NT_SUCCESS(status))
    {
        FltFileName->Type = KphFltFileNameTypeNameInfo;
        return status;
    }

    flags = (FLT_FILE_NAME_OPENED |
             FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP);

    status = FltGetFileNameInformation(Data, flags, &FltFileName->NameInfo);
    if (NT_SUCCESS(status))
    {
        FltFileName->Type = KphFltFileNameTypeNameInfo;
    }

    return status;
}

/**
 * \brief Retrieves the length of the file name for a file object.
 *
 * \param[in] Volume The volume of the file object.
 * \param[in] FileObject The file object.
 *
 * \return The length of the file name.
 */
_IRQL_requires_max_(APC_LEVEL)
ULONG KphpFltNameCacheFileNameLength(
    _In_ PFLT_VOLUME Volume,
    _In_ PFILE_OBJECT FileObject
    )
{
    ULONG length;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_VERIFY(FltGetVolumeName(Volume,
                               NULL,
                               &length) == STATUS_BUFFER_TOO_SMALL);
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(length,
                                     FileObject->FileName.Length,
                                     &length)));

    return length;
}

/**
 * \brief Copies the file name of a file object.
 *
 * \param[in] Volume The volume of the file object.
 * \param[in] FileObject The file object.
 * \param[in,out] FileName The file name to copy into.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltNameCacheCopyFileName(
    _In_ PFLT_VOLUME Volume,
    _In_ PFILE_OBJECT FileObject,
    _Inout_ PUNICODE_STRING FileName
    )
{
    PUNICODE_STRING fileName;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_VERIFY(NT_SUCCESS(FltGetVolumeName(Volume, FileName, NULL)));

    fileName = &FileObject->FileName;

    NT_VERIFY(NT_SUCCESS(RtlAppendUnicodeStringToString(FileName, fileName)));
}

/**
 * \brief Queues a worker to populate the file name cache outside of paging I/O.
 *
 * \param[in] Instance The filter instance.
 * \param[in] FileObject The file object to populate the cache for.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltQueueFileNameCachePopulate(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS status;
    PKPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM item;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    item = KphAllocateNPaged(sizeof(KPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM),
                             KPH_TAG_FLT_FILE_NAME_CACHE_POPULATE);
    if (!item)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate file name cache populate item.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = FltObjectReference(Instance);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "FltObjectReference failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ObReferenceObject(FileObject);

    item->Instance = Instance;
    item->FileObject = FileObject;

    if (!InterlockedPushEntrySList(&KphpFileNameCachePopulateList,
                                   &item->ListEntry))
    {
        KsiQueueWorkItem(&KphpFileNameCachePopulateWorkItem, DelayedWorkQueue);
    }

    item = NULL;

Exit:

    if (item)
    {
        KphFree(item, KPH_TAG_FLT_FILE_NAME_CACHE_POPULATE);
    }

    return status;
}

/**
 * \brief Retrieves the file name, using the stream handle context as a cache.
 *
 * \param[in] Filter The filter.
 * \param[in] Instance The filter instance.
 * \param[in] Volume The volume of the file object.
 * \param[in] FileObject The file object.
 * \param[in] PagingIo TRUE if the operation is paging I/O.
 * \param[out] FltFileName Receives the file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileNameUseContext(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_INSTANCE Instance,
    _In_ PFLT_VOLUME Volume,
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN PagingIo,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    NTSTATUS status;
    ULONG length;
    PFLT_CONTEXT oldContext;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    //
    // Check if the context already exists.
    //

    status = FltGetStreamHandleContext(Instance,
                                       FileObject,
                                       &FltFileName->Context);
    if (NT_SUCCESS(status))
    {
        FltFileName->Type = KphFltFileNameTypeContext;
        return status;
    }

    if (PagingIo)
    {
        status = KphpFltQueueFileNameCachePopulate(Instance, FileObject);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpFltQueueFileNameCachePopulate failed: %!STATUS!",
                          status);

            return status;
        }

        return STATUS_FLT_NAME_CACHE_MISS;
    }

    //
    // Try to create a new context.
    //

    length = KphpFltNameCacheFileNameLength(Volume, FileObject);

    status = FltAllocateContext(Filter,
                                FLT_STREAMHANDLE_CONTEXT,
                                length + sizeof(UNICODE_STRING),
                                NonPagedPoolNx,
                                &FltFileName->Context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "FltAllocateContext failed: %!STATUS!",
                      status);

        FltFileName->Context = NULL;
        return status;
    }

    NT_ASSERT(FltFileName->Context == FltFileName->FileName);

    FltFileName->FileName->Length = 0;
    FltFileName->FileName->MaximumLength = (USHORT)length;
    FltFileName->FileName->Buffer = Add2Ptr(FltFileName->FileName,
                                            sizeof(UNICODE_STRING));

    KphpFltNameCacheCopyFileName(Volume, FileObject, FltFileName->FileName);

    //
    // Set the new context.
    //

    oldContext = NULL;
    status = FltSetStreamHandleContext(Instance,
                                       FileObject,
                                       FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                       FltFileName->Context,
                                       &oldContext);
    if (NT_SUCCESS(status))
    {
        FltFileName->Type = KphFltFileNameTypeContext;
        return status;
    }

    //
    // We raced and lost, or failed for some other reason.
    //

    FltReleaseContext(FltFileName->Context);
    FltFileName->Context = NULL;

    if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
    {
        //
        // Return the existing context.
        //
        FltFileName->Type = KphFltFileNameTypeContext;
        FltFileName->Context = oldContext;
        return STATUS_SUCCESS;
    }

    //
    // This is unexpected, we could do more juggling, but instead choose to
    // just give up.
    //
    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  INFORMER,
                  "FltSetStreamHandleContext failed: %!STATUS!",
                  status);

    if (oldContext)
    {
        FltReleaseContext(oldContext);
    }

    return status;
}

/**
 * \brief Retrieves the file name, using the global name cache.
 *
 * \param[in] Instance The filter instance.
 * \param[in] Volume The volume of the file object.
 * \param[in] FileObject The file object.
 * \param[in] PagingIo TRUE if the operation is paging I/O.
 * \param[out] FltFileName Receives the file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileNameUseNameCache(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFLT_VOLUME Volume,
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN PagingIo,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    NTSTATUS status;
    KIRQL previousIrql;
    ULONG length;
    PKPH_FLT_FILE_NAME_CACHE_ENTRY cacheEntry;
    PKPH_FLT_FILE_NAME_CACHE_ENTRY existingCacheEntry;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    //
    // Check if the file object is already in the cache.
    //

    cacheEntry = NULL;
    previousIrql = ExAcquireSpinLockShared(&KphpFileNameCacheLock);

    for (PLIST_ENTRY entry = KphpFileNameCacheList.Flink;
         entry != &KphpFileNameCacheList;
         entry = entry->Flink)
    {
        cacheEntry = CONTAINING_RECORD(entry,
                                        KPH_FLT_FILE_NAME_CACHE_ENTRY,
                                        ListEntry);

        if (FileObject == cacheEntry->FileObject)
        {
            KphReferenceObject(cacheEntry);
            break;
        }

        cacheEntry = NULL;
    }

    ExReleaseSpinLockShared(&KphpFileNameCacheLock, previousIrql);

    if (cacheEntry)
    {
        //
        // Return a reference (taken above) to the existing cached entry.
        //
        FltFileName->NameCache = cacheEntry;
        FltFileName->Type = KphFltFileNameTypeNameCache;
        return STATUS_SUCCESS;
    }

    if (PagingIo)
    {
        status = KphpFltQueueFileNameCachePopulate(Instance, FileObject);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpFltQueueFileNameCachePopulate failed: %!STATUS!",
                          status);

            return status;
        }

        return STATUS_FLT_NAME_CACHE_MISS;
    }

    //
    // Try to create new entry and insert it into the cache.
    //

    length = KphpFltNameCacheFileNameLength(Volume, FileObject);

    status = KphCreateObject(KphpCachedFileNameType,
                             length + sizeof(KPH_FLT_FILE_NAME_CACHE_ENTRY),
                             &cacheEntry,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        return status;
    }

    cacheEntry->FileObject = FileObject;

    cacheEntry->FileName.Length = 0;
    cacheEntry->FileName.MaximumLength = (USHORT)length;
    cacheEntry->FileName.Buffer = Add2Ptr(cacheEntry,
                                          sizeof(KPH_FLT_FILE_NAME_CACHE_ENTRY));

    KphpFltNameCacheCopyFileName(Volume, FileObject, &cacheEntry->FileName);

    //
    // Insert it into the cache, and check if we've raced.
    //

    existingCacheEntry = NULL;
    previousIrql = ExAcquireSpinLockExclusive(&KphpFileNameCacheLock);

    for (PLIST_ENTRY entry = KphpFileNameCacheList.Flink;
         entry != &KphpFileNameCacheList;
         entry = entry->Flink)
    {
        existingCacheEntry = CONTAINING_RECORD(entry,
                                               KPH_FLT_FILE_NAME_CACHE_ENTRY,
                                               ListEntry);

        if (cacheEntry->FileObject == existingCacheEntry->FileObject)
        {
            KphReferenceObject(existingCacheEntry);
            break;
        }

        existingCacheEntry = NULL;
    }

    if (!existingCacheEntry)
    {
        //
        // Insert the new entry (with a reference) in the list.
        //
        KphReferenceObject(cacheEntry);
        InsertHeadList(&KphpFileNameCacheList, &cacheEntry->ListEntry);
    }

    ExReleaseSpinLockExclusive(&KphpFileNameCacheLock, previousIrql);

    if (existingCacheEntry)
    {
        //
        // Return the existing cached entry reference (taken above) to the
        // caller, and drop the one we just created.
        //
        FltFileName->NameCache = existingCacheEntry;
        KphDereferenceObject(cacheEntry);
    }
    else
    {
        //
        // New entry inserted into the cache
        //
        FltFileName->NameCache = cacheEntry;
    }

    FltFileName->Type = KphFltFileNameTypeNameCache;

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the file name, by copying it from the related objects.
 *
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] FltFileName Receives the file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileNameCopy(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    ULONG length;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(FltObjects->Volume);
    NT_ASSERT(FltObjects->FileObject);

    length = KphpFltNameCacheFileNameLength(FltObjects->Volume,
                                            FltObjects->FileObject);

    FltFileName->FileName = KphAllocateNPaged(length + sizeof(UNICODE_STRING),
                                              KPH_TAG_FLT_FILE_NAME);
    if (!FltFileName->FileName)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate file name.");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FltFileName->FileName->Length = 0;
    FltFileName->FileName->MaximumLength = (USHORT)length;
    FltFileName->FileName->Buffer = Add2Ptr(FltFileName->FileName,
                                            sizeof(UNICODE_STRING));

    KphpFltNameCacheCopyFileName(FltObjects->Volume,
                                 FltObjects->FileObject,
                                 FltFileName->FileName);

    FltFileName->Type = KphFltFileNameTypeFileName;

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the volume name, by copying it from the related objects.
 *
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] FltFileName Receives the file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetVolumeName(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    ULONG length;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    if (!FltObjects->Volume)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "Volume is null");

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    (VOID)FltGetVolumeName(FltObjects->Volume, NULL, &length);

    FltFileName->FileName = KphAllocateNPaged(length + sizeof(UNICODE_STRING),
                                              KPH_TAG_FLT_FILE_NAME);
    if (!FltFileName->FileName)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate file name.");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FltFileName->FileName->Length = 0;
    FltFileName->FileName->MaximumLength = (USHORT)length;
    FltFileName->FileName->Buffer = Add2Ptr(FltFileName->FileName,
                                            sizeof(UNICODE_STRING));

    (VOID)FltGetVolumeName(FltObjects->Volume, FltFileName->FileName, NULL);

    FltFileName->Type = KphFltFileNameTypeFileName;

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the file name for the given callback data.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] FltFileName Receives the file name, must be related using
 * KphpFltReleaseFileName.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileName(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    NTSTATUS status;
    BOOLEAN pagingIo;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(FltObjects->FileObject == Data->Iopb->TargetFileObject);

    KphpFltZeroFileName(FltFileName);

    if (!FltObjects->FileObject)
    {
        if ((Data->Iopb->MajorFunction == IRP_MJ_VOLUME_MOUNT) ||
            (Data->Iopb->MajorFunction == IRP_MJ_VOLUME_DISMOUNT))
        {
            return KphpFltGetVolumeName(FltObjects, FltFileName);
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "FileObject is null");

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    status = KphpFltGetFileNameInformation(Data, FltFileName);
    if (NT_SUCCESS(status))
    {
        return status;
    }

    if (!FltObjects->Volume)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "Volume is null");

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    pagingIo = (FlagOn(Data->Iopb->IrpFlags, IRP_PAGING_IO) ||
                FlagOn(Data->Iopb->IrpFlags, IRP_SYNCHRONOUS_PAGING_IO));

    if (FltSupportsStreamHandleContexts(FltObjects->FileObject))
    {
        status = KphpFltGetFileNameUseContext(FltObjects->Filter,
                                              FltObjects->Instance,
                                              FltObjects->Volume,
                                              FltObjects->FileObject,
                                              pagingIo,
                                              FltFileName);
    }
    else if (FsRtlIsPagingFile(FltObjects->FileObject))
    {
        status = KphpFltGetFileNameUseNameCache(FltObjects->Instance,
                                                FltObjects->Volume,
                                                FltObjects->FileObject,
                                                pagingIo,
                                                FltFileName);
    }
    else if (!pagingIo)
    {
        status = KphpFltGetFileNameCopy(FltObjects, FltFileName);
    }
    else
    {
        status = STATUS_FLT_NAME_CACHE_MISS;
    }

    return status;
}

/**
 * \brief Retrieves destination file name information using the filter name
 * cache.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] FltFileName Receives the destination file name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetDestinationFileNameInformation(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltDestFileName
    )
{
    NTSTATUS status;
    HANDLE rootDirectory;
    PWSTR fileName;
    ULONG fileNameLength;
    ULONG flags;

    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(FltObjects->FileObject);

    if (Data->Iopb->MajorFunction != IRP_MJ_SET_INFORMATION)
    {
        NT_ASSERT(Data->Iopb->MinorFunction == IRP_MJ_SET_INFORMATION);
        return STATUS_INVALID_PARAMETER;
    }

    switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
    {
        case FileRenameInformation:
        case FileRenameInformationBypassAccessCheck:
        case FileRenameInformationEx:
        case FileRenameInformationExBypassAccessCheck:
        {
            PFILE_RENAME_INFORMATION renameInfo;

            renameInfo = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

            rootDirectory = renameInfo->RootDirectory;
            fileName = renameInfo->FileName;
            fileNameLength = renameInfo->FileNameLength;

            break;
        }
        case FileLinkInformation:
        case FileLinkInformationBypassAccessCheck:
        case FileLinkInformationEx:
        case FileLinkInformationExBypassAccessCheck:
        {
            PFILE_LINK_INFORMATION linkInfo;

            linkInfo = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

            rootDirectory = linkInfo->RootDirectory;
            fileName = linkInfo->FileName;
            fileNameLength = linkInfo->FileNameLength;

            break;
        }
        default:
        {
            NT_ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
        }
    }

    flags = (FLT_FILE_NAME_NORMALIZED |
             FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP);

    status = FltGetDestinationFileNameInformation(FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  rootDirectory,
                                                  fileName,
                                                  fileNameLength,
                                                  flags,
                                                  &FltDestFileName->NameInfo);
    if (NT_SUCCESS(status))
    {
        FltDestFileName->Type = KphFltFileNameTypeNameInfo;
        return status;
    }

    flags = (FLT_FILE_NAME_OPENED |
             FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP);
    status = FltGetDestinationFileNameInformation(FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  rootDirectory,
                                                  fileName,
                                                  fileNameLength,
                                                  flags,
                                                  &FltDestFileName->NameInfo);
    if (NT_SUCCESS(status))
    {
        FltDestFileName->Type = KphFltFileNameTypeNameInfo;
    }

    return status;
}

/**
 * \brief Retrieves the destination file name for the given callback data.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] FltFileName Receives the destination file name, must be related
 * using KphpFltReleaseFileName.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetDestFileName(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltDestFileName
    )
{
    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(FltObjects->FileObject == Data->Iopb->TargetFileObject);

    KphpFltZeroFileName(FltDestFileName);

    if (!FltObjects->FileObject)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "FileObject is null");

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return KphpFltGetDestinationFileNameInformation(Data,
                                                    FltObjects,
                                                    FltDestFileName);
}

/**
 * \brief Releases the file name.
 *
 * \param[in] FltFileName The file name to release.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltReleaseFileName(
    _In_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    switch (FltFileName->Type)
    {
        case KphFltFileNameTypeNameInfo:
        {
            NT_ASSERT(FltFileName->NameInfo);
            FltReleaseFileNameInformation(FltFileName->NameInfo);
            break;
        }
        case KphFltFileNameTypeContext:
        {
            NT_ASSERT(FltFileName->Context);
            FltReleaseContext(FltFileName->Context);
            break;
        }
        case KphFltFileNameTypeFileName:
        {
            NT_ASSERT(FltFileName->FileName);
            KphFree(FltFileName->FileName, KPH_TAG_FLT_FILE_NAME);
            break;
        }
        case KphFltFileNameTypeNameCache:
        {
            NT_ASSERT(FltFileName->NameCache);
            KphDereferenceObject(FltFileName->NameCache);
            break;
        }
        case KphFltFileNameTypeNone:
        default:
        {
            //
            // This is a union, assert is only needed once.
            //
            NT_ASSERT(!FltFileName->NameInfo);
            break;
        }
    }
}

/**
 * \brief Reaps the file name cache entry for the given callback data.
 *
 * \details A global name cache is used in limited cases to cache the file name
 * for a given file object. In order to cleanly reap information from the cache
 * this must be called in the post operation callback for IRP_MJ_CLOSE.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltReapFileNameCache(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    )
{
    KIRQL previosIrql;
    PKPH_FLT_FILE_NAME_CACHE_ENTRY cacheEntry;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    NT_ASSERT(FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION));
    NT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_CLOSE);
    DBG_UNREFERENCED_PARAMETER(Data);

    cacheEntry = NULL;
    previosIrql = ExAcquireSpinLockExclusive(&KphpFileNameCacheLock);

    for (PLIST_ENTRY entry = KphpFileNameCacheList.Flink;
         entry != &KphpFileNameCacheList;
         entry = entry->Flink)
    {
        cacheEntry = CONTAINING_RECORD(entry,
                                       KPH_FLT_FILE_NAME_CACHE_ENTRY,
                                       ListEntry);

        if (cacheEntry->FileObject == FltObjects->FileObject)
        {
            RemoveEntryList(&cacheEntry->ListEntry);
            break;
        }

        cacheEntry = NULL;
    }

    ExReleaseSpinLockExclusive(&KphpFileNameCacheLock, previosIrql);

    if (cacheEntry)
    {
        KphDereferenceObject(cacheEntry);
    }
}

/**
 * \brief Allocates a cached file name object.
 *
 * \param[in] Size The size of the object to allocate.
 *
 * \return The allocated object, or NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpFltAllocateCachedFileName(
    _In_ SIZE_T Size
    )
{
    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    return KphAllocateNPaged(Size, KPH_TAG_FLT_CACHED_FILE_NAME);
}

/**
 * \brief Frees a cached file name object.
 *
 * \param[in] Object The object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFltFreeCachedFileName(
    _In_freesMem_ PVOID Object
    )
{
    KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    KphFree(Object, KPH_TAG_FLT_CACHED_FILE_NAME);
}

KPH_PAGED_FILE();

/**
 * \brief Populates the file name cache for a file object outside of paging
 * I/O.
 *
 * \param[in] Instance The filter instance.
 * \param[in] FileObject The file object to populate the cache for.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltPopulateFileNameCache(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS status;
    PFLT_FILTER filter;
    PFLT_VOLUME volume;
    KPH_FLT_FILE_NAME fltFileName;

    KPH_PAGED_CODE_PASSIVE();

    filter = NULL;
    volume = NULL;

    status = FltGetFilterFromInstance(Instance, &filter);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "FltGetFilterFromInstance failed: %!STATUS!",
                      status);

        filter = NULL;
        goto Exit;
    }

    status = FltGetVolumeFromInstance(Instance, &volume);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "FltGetVolumeFromInstance failed: %!STATUS!",
                      status);

        volume = NULL;
        goto Exit;
    }

    KphpFltZeroFileName(&fltFileName);

    if (FltSupportsStreamHandleContexts(FileObject))
    {
        status = KphpFltGetFileNameUseContext(filter,
                                              Instance,
                                              volume,
                                              FileObject,
                                              FALSE,
                                              &fltFileName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpFltGetFileNameUseContext failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }
    else if (FsRtlIsPagingFile(FileObject))
    {
        status = KphpFltGetFileNameUseNameCache(Instance,
                                                volume,
                                                FileObject,
                                                FALSE,
                                                &fltFileName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpFltGetFileNameUseNameCache failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Unexpected file object for cache populate");

        NT_ASSERT(FALSE);
    }

    KphpFltReleaseFileName(&fltFileName);

Exit:

    if (volume)
    {
        FltObjectDereference(volume);
    }

    if (filter)
    {
        FltObjectDereference(filter);
    }
}

/**
 * \brief Worker which populates the file name cache for misses during
 * paging I/O.
 *
 * \param[in] Parameter Unused parameter.
 */
_Function_class_(KSI_WORK_QUEUE_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpFltFileNameCachePopulateWorker(
    _In_opt_ PVOID Parameter
    )
{
    PSLIST_ENTRY entry;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    entry = InterlockedFlushSList(&KphpFileNameCachePopulateList);

    while (entry)
    {
        PKPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM item;

        item = CONTAINING_RECORD(entry,
                                 KPH_FLT_FILE_NAME_CACHE_POPULATE_ITEM,
                                 ListEntry);

        entry = entry->Next;

        KphpFltPopulateFileNameCache(item->Instance, item->FileObject);

        ObDereferenceObject(item->FileObject);
        FltObjectDereference(item->Instance);
        KphFree(item, KPH_TAG_FLT_FILE_NAME_CACHE_POPULATE);
    }
}

/**
 * \brief Cleans up the file name cache.
 *
 * \details A global name cache is used in limited cases to cache the file name
 * for a given file object. This must be called after the filter stops
 * filtering.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltCleanupFileNameCache(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!KphpFileNameCacheInitialized)
    {
        return;
    }

    NT_ASSERT(!QueryDepthSList(&KphpFileNameCachePopulateList));

    while (!IsListEmpty(&KphpFileNameCacheList))
    {
        PKPH_FLT_FILE_NAME_CACHE_ENTRY cacheEntry;

        cacheEntry = CONTAINING_RECORD(RemoveHeadList(&KphpFileNameCacheList),
                                       KPH_FLT_FILE_NAME_CACHE_ENTRY,
                                       ListEntry);

        KphDereferenceObject(cacheEntry);
    }
}

/**
 * \brief Initializes the file name cache.
 *
 * \details A global name cache is used in limited cases to cache the file name
 * for a given file object. This must be called before the filter is begins
 * filtering.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltInitializeFileNameCache(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    InitializeListHead(&KphpFileNameCacheList);
    InitializeSListHead(&KphpFileNameCachePopulateList);
    KsiInitializeWorkItem(&KphpFileNameCachePopulateWorkItem,
                          KphDriverObject,
                          KphpFltFileNameCachePopulateWorker,
                          NULL);

    typeInfo.Allocate = KphpFltAllocateCachedFileName;
    typeInfo.Initialize = NULL;
    typeInfo.Delete = NULL;
    typeInfo.Free = KphpFltFreeCachedFileName;
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpCachedFileNameTypeName,
                        &typeInfo,
                        &KphpCachedFileNameType);

    KphpFileNameCacheInitialized = TRUE;
}
