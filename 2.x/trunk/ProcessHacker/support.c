#include <phgui.h>
#include <wchar.h>

INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PhShowMessage_V(hWnd, Type, Format, argptr);
}

INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    )
{
    INT result;
    WCHAR message[PH_MAX_MESSAGE_SIZE];

    result = _vsnwprintf(message, PH_MAX_MESSAGE_SIZE, Format, ArgPtr);

    if (result == -1)
        return -1;

    return MessageBox(hWnd, message, PH_APP_NAME, Type);
}

PVOID PhGetFileVersionInfo(
    __in PWSTR FileName
    )
{
    ULONG versionInfoSize;
    ULONG dummy;
    PVOID versionInfo;

    versionInfoSize = GetFileVersionInfoSize(
        FileName,
        &dummy
        );

    if (versionInfoSize)
    {
        versionInfo = PhAllocate(versionInfoSize);

        if (!GetFileVersionInfo(
            FileName,
            0,
            versionInfoSize,
            versionInfo
            ))
        {
            PhFree(versionInfo);

            return NULL;
        }
    }

    return versionInfo;
}

ULONG PhGetFileVersionInfoLangCodePage(
    __in PVOID VersionInfo
    )
{
    PVOID buffer;
    ULONG length;

    if (VerQueryValue(VersionInfo, L"\\VarFileInfo\\Translation", &buffer, &length))
    {
        // Combine the language ID and code page.
        return (*(PUSHORT)buffer << 16) + *((PUSHORT)buffer + 1);
    }
    else
    {
        return 0x40904e4;
    }
}

PPH_STRING PhGetFileVersionInfoString(
    __in PVOID VersionInfo,
    __in PWSTR SubBlock
    )
{
    PVOID buffer;
    ULONG length;

    if (VerQueryValue(VersionInfo, SubBlock, &buffer, &length))
    {
        return PhCreateStringEx((PWSTR)buffer, length * sizeof(WCHAR));
    }
    else
    {
        return NULL;
    }
}

PPH_STRING PhGetFileVersionInfoString2(
    __in PVOID VersionInfo,
    __in ULONG LangCodePage,
    __in PWSTR StringName
    )
{
    WCHAR subBlock[64];

    _snwprintf(subBlock, 64, L"\\StringFileInfo\\%08X\\%s", LangCodePage, StringName);

    return PhGetFileVersionInfoString(VersionInfo, subBlock);
}

PPH_STRING PhGetSystemDirectory()
{
    PPH_STRING systemDirectory;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x40;
    buffer = PhAllocate(bufferSize * 2);

    returnLength = GetSystemDirectory(buffer, bufferSize);

    if (returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize * 2);

        returnLength = GetSystemDirectory(buffer, bufferSize);
    }

    if (returnLength == 0)
    {
        PhFree(buffer);
        return NULL;
    }

    systemDirectory = PhCreateString(buffer);
    PhFree(buffer);

    return systemDirectory;
}
