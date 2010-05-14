#include <ph.h>

#define FI_ARG_HELP 1
#define FI_ARG_ACTION 2
#define FI_ARG_NATIVE 3
#define FI_ARG_PATTERN 4
#define FI_ARG_CASESENSITIVE 5

PPH_STRING FiArgFileName;
BOOLEAN FiArgHelp;
PPH_STRING FiArgAction;
BOOLEAN FiArgNative;
PPH_STRING FiArgPattern;
BOOLEAN FiArgCaseSensitive;

ULONG64 FipDirFileCount;
ULONG64 FipDirDirCount;
ULONG64 FipDirTotalSize;
ULONG64 FipDirTotalAllocSize;

static BOOLEAN NTAPI FiCommandLineCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
    )
{
    if (Option)
    {
        switch (Option->Id)
        {
        case FI_ARG_HELP:
            FiArgHelp = TRUE;
            break;
        case FI_ARG_ACTION:
            PhSwapReference(&FiArgAction, Value);
            break;
        case FI_ARG_NATIVE:
            FiArgNative = TRUE;
            break;
        case FI_ARG_PATTERN:
            PhSwapReference(&FiArgPattern, Value);
            break;
        case FI_ARG_CASESENSITIVE:
            FiArgCaseSensitive = TRUE;
            break;
        }
    }
    else
    {
        PhSwapReference(&FiArgFileName, Value);
    }

    return TRUE;
}

VOID FiPrintHelp()
{
    wprintf(
        L"fiin [-a action] [-C] [-N] [-p pattern] [filename]\n"
        L"\t-a action\tThe action to be performed.\n"
        L"\t-C\tSpecifies that file names are case-sensitive.\n"
        L"\t-N\tSpecifies that file names are in native format.\n"
        L"\t-p\tA search pattern for listings.\n"
        );
}

BOOLEAN FiCreateFile(
    __out PHANDLE FileHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PPH_STRING FileName,
    __in ULONG CreateDisposition,
    __in_opt ULONG FileAttributes,
    __in_opt ULONG Options
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PPH_STRING fileName;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    if (!Options)
        Options = FILE_SYNCHRONOUS_IO_NONALERT;
    if (!FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (!FiArgNative)
    {
        fileName = PhFormatString(
            L"\\??\\%s%s",
            // HACK to determine if the file name is relative
            (PhStringIndexOfChar(FileName, 0, ':') < PhStringIndexOfChar(FileName, 0, '\\')) ? L"" :
            NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath.Buffer,
            FileName->Buffer
            );
    }
    else
    {
        fileName = PhCreateStringEx(FileName->Buffer, FileName->Length);
    }//wprintf(L"Result file name: %s\n", fileName->Buffer);

    InitializeObjectAttributes(
        &oa,
        &fileName->us,
        (!FiArgCaseSensitive ? OBJ_CASE_INSENSITIVE : 0),
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &oa,
        &isb,
        NULL,
        FileAttributes,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        CreateDisposition,
        Options,
        NULL,
        0
        );

    PhDereferenceObject(fileName);

    if (!NT_SUCCESS(status))
    {
        wprintf(L"Error creating/opening file: %s\n", PhGetNtMessage(status)->Buffer);
        return FALSE;
    }

    *FileHandle = fileHandle;

    return TRUE;
}

BOOLEAN NTAPI FipEnumDirectoryFileForDir(
    __in PFILE_DIRECTORY_INFORMATION Information,
    __in PVOID Context
    )
{
    PPH_STRING date, time, size;
    SYSTEMTIME systemTime;

    PhLargeIntegerToLocalSystemTime(&systemTime, &Information->LastWriteTime);
    date = PhFormatDate(&systemTime, NULL);
    time = PhFormatTime(&systemTime, NULL);
    size = PhFormatUInt64(Information->EndOfFile.QuadPart, TRUE);

    wprintf(
        L"%-10s %12s %c%c%c%c%c%c%c%c %11s %.*s\n",
        date->Buffer,
        time->Buffer,
        (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? '+' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_HIDDEN) ? 'h' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_SYSTEM) ? 's' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_READONLY) ? 'r' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? 'z' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ? 'e' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) ? '%' : ' ',
        (Information->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? '*' : ' ',
        size->Buffer,
        Information->FileNameLength / 2,
        Information->FileName
        );

    PhDereferenceObject(date);
    PhDereferenceObject(time);
    PhDereferenceObject(size);

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        FipDirDirCount++;
    else
        FipDirFileCount++;

    FipDirTotalSize += Information->EndOfFile.QuadPart;
    FipDirTotalAllocSize += Information->AllocationSize.QuadPart;

    return TRUE;
}

int __cdecl main(int argc, char *argv[])
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { FI_ARG_HELP, L"h", NoArgumentType },
        { FI_ARG_ACTION, L"a", MandatoryArgumentType },
        { FI_ARG_NATIVE, L"N", NoArgumentType },
        { FI_ARG_PATTERN, L"p", MandatoryArgumentType },
        { FI_ARG_CASESENSITIVE, L"C", NoArgumentType }
    };
    PH_STRINGREF commandLine;
    NTSTATUS status;

    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    if (!PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        FiCommandLineCallback,
        NULL
        ) || FiArgHelp)
    {
        FiPrintHelp();
        return 0;
    }

    if (!FiArgFileName && (
        PhStringEquals2(FiArgAction, L"dir", TRUE)
        ))
    {
        FiArgFileName = PhCreateStringEx(
            NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath.Buffer,
            NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath.Length
            );
    }

    if (!FiArgFileName || !FiArgAction)
    {
        FiPrintHelp();
        return 1;
    }
    else if (PhStringEquals2(FiArgAction, L"del", TRUE))
    {
         HANDLE fileHandle;

         if (FiCreateFile(
             &fileHandle,
             DELETE | SYNCHRONIZE,
             FiArgFileName,
             FILE_OPEN,
             0,
             FILE_SYNCHRONOUS_IO_NONALERT
             ))
         {
             FILE_DISPOSITION_INFORMATION dispositionInfo;
             IO_STATUS_BLOCK isb;

             dispositionInfo.DeleteFileW = TRUE;
             if (!NT_SUCCESS(status = NtSetInformationFile(fileHandle, &isb, &dispositionInfo,
                 sizeof(FILE_DISPOSITION_INFORMATION), FileDispositionInformation)))
             {
                 wprintf(L"Error deleting file: %s\n", PhGetNtMessage(status)->Buffer);
             }

             NtClose(fileHandle);
         }
    }
    else if (PhStringEquals2(FiArgAction, L"touch", TRUE))
    {
         HANDLE fileHandle;

         if (FiCreateFile(
             &fileHandle,
             FILE_READ_ATTRIBUTES | SYNCHRONIZE,
             FiArgFileName,
             FILE_CREATE,
             0,
             FILE_SYNCHRONOUS_IO_NONALERT
             ))
         {
             NtClose(fileHandle);
         }
    }
    else if (PhStringEquals2(FiArgAction, L"mkdir", TRUE))
    {
         HANDLE fileHandle;

         if (FiCreateFile(
             &fileHandle,
             FILE_READ_ATTRIBUTES | SYNCHRONIZE,
             FiArgFileName,
             FILE_CREATE,
             FILE_ATTRIBUTE_DIRECTORY,
             FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
             ))
         {
             NtClose(fileHandle);
         }
    }
    else if (PhStringEquals2(FiArgAction, L"dir", TRUE))
    {
        HANDLE fileHandle;
        PPH_STRING totalSize, totalAllocSize;

        if (FiCreateFile(
            &fileHandle,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FiArgFileName,
            FILE_OPEN,
            0,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            ))
        {
            FipDirFileCount = 0;
            FipDirDirCount = 0;
            FipDirTotalSize = 0;
            FipDirTotalAllocSize = 0;

            PhEnumDirectoryFile(
                fileHandle,
                FiArgPattern ? &FiArgPattern->sr : NULL,
                FipEnumDirectoryFileForDir,
                NULL
                );
            NtClose(fileHandle);

            totalSize = PhFormatUInt64(FipDirTotalSize, TRUE);
            totalAllocSize = PhFormatUInt64(FipDirTotalAllocSize, TRUE);

            wprintf(
                L"%12I64u file(s)  %11s bytes\n"
                L"%12I64u dir(s)   %11s bytes allocated\n",
                FipDirFileCount,
                totalSize->Buffer,
                FipDirDirCount,
                totalAllocSize->Buffer
                );

            PhDereferenceObject(totalSize);
            PhDereferenceObject(totalAllocSize);
        }
    }
    else if (PhStringEquals2(FiArgAction, L"streams", TRUE))
    {
        HANDLE fileHandle;
        PVOID streams;
        PFILE_STREAM_INFORMATION stream;

        if (FiCreateFile(
            &fileHandle,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FiArgFileName,
            FILE_OPEN,
            0,
            FILE_SYNCHRONOUS_IO_NONALERT
            ))
        {
            if (NT_SUCCESS(PhEnumFileStreams(fileHandle, &streams)))
            {
                stream = PH_FIRST_STREAM(streams);

                while (stream)
                {
                    PPH_STRING size, allocationSize;

                    size = PhFormatUInt64(stream->StreamSize.QuadPart, TRUE);
                    allocationSize = PhFormatUInt64(stream->StreamAllocationSize.QuadPart, TRUE);

                    wprintf(
                        L"%11s %11s %.*s\n",
                        size->Buffer,
                        allocationSize->Buffer,
                        stream->StreamNameLength / 2,
                        stream->StreamName
                        );

                    PhDereferenceObject(size);
                    PhDereferenceObject(allocationSize);

                    stream = PH_NEXT_STREAM(stream);
                }
            }

            NtClose(fileHandle);
        }
    }
}
