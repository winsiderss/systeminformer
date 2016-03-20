#ifndef _APPSUP_H
#define _APPSUP_H


extern PWSTR Version;
extern PWSTR DownloadURL;
extern HWND _hwndProgress;


#define STATUS_MSG(Format, ...) \
{ \
    PPH_STRING msgString = PhFormatString(Format, __VA_ARGS__); \
    if (msgString) \
    { \
        SetDlgItemText(_hwndProgress, IDC_MAINHEADER1, msgString->Buffer); \
        DEBUG_MSG(L"%s\n", msgString->Buffer); \
        PhDereferenceObject(msgString); \
    } \
}

typedef struct _STOPWATCH
{
    LARGE_INTEGER StartCounter;
    LARGE_INTEGER EndCounter;
    LARGE_INTEGER Frequency;
} STOPWATCH, *PSTOPWATCH;

PPH_STRING GetSystemTemp(VOID);

PPH_STRING BrowseForFolder(
    _In_opt_ HWND DialogHandle,
    _In_opt_ PCWSTR Title
    );

VOID InitializeFont(
    _In_ HWND ControlHandle,
    _In_ LONG Height,
    _In_ LONG Weight
    );

VOID StopwatchInitialize(
    __out PSTOPWATCH Stopwatch
    );

VOID StopwatchStart(
    _Inout_ PSTOPWATCH Stopwatch
    );

ULONG StopwatchGetMilliseconds(
    _In_ PSTOPWATCH Stopwatch
    );


BOOLEAN CreateLink(
    _In_ PWSTR DestFilePath,
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir,
    _In_ PWSTR FileComment
    );

PWSTR XmlParseToken(
    _In_ PWSTR XmlString,
    _In_ PWSTR XmlTokenName
    );

HBITMAP LoadPngImageFromResources(
    _In_ PCWSTR Name
    );

_Maybenull_
PPH_STRING GetProcessHackerInstallPath(
    VOID
    );

BOOLEAN CreateDirectoryPath(
    _In_ PPH_STRING DirectoryPath
    );

#endif _APPSUP_H