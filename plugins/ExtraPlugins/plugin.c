#include "main.h"

ULONG PhDisabledPluginsCount(
    VOID
    )
{
    PPH_STRING disabled;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;
    ULONG count = 0;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    remainingPart = disabled->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length)
        {
            count++;
        }
    }

    PhDereferenceObject(disabled);

    return count;
}

// Copied from Process Hacker, plugin.c

PWSTR PhGetPluginBaseName(
    _In_ PPHAPP_PLUGIN Plugin
    )
{
    if (Plugin->FileName)
    {
        PH_STRINGREF pathNamePart;
        PH_STRINGREF baseNamePart;

        if (PhSplitStringRefAtLastChar(&Plugin->FileName->sr, '\\', &pathNamePart, &baseNamePart))
            return baseNamePart.Buffer;
        else
            return Plugin->FileName->Buffer;
    }
    else
    {
        // Fake disabled plugin.
        return Plugin->Name.Buffer;
    }
}

BOOLEAN PhIsPluginLoadedByBaseName(
    _In_ PPH_STRINGREF BaseName
    )
{
    PPH_AVL_LINKS root;
    PPH_AVL_LINKS links;

    root = PluginInstance->Links.Parent;

    while (root)
    {
        if (!root->Parent)
            break;

        root = root->Parent;
    }

    for (
        links = PhMinimumElementAvlTree((PPH_AVL_TREE)root);
        links;
        links = PhSuccessorElementAvlTree(links)
        )
    {
        PPHAPP_PLUGIN plugin = CONTAINING_RECORD(links, PHAPP_PLUGIN, Links);
        PH_STRINGREF pluginBaseName;

        PhInitializeStringRefLongHint(&pluginBaseName, PhGetPluginBaseName(plugin));

        if (PhEqualStringRef(&pluginBaseName, BaseName, TRUE))
            return TRUE;
    }

    return FALSE;
}

BOOLEAN PhpLocateDisabledPlugin(
    _In_ PPH_STRING List,
    _In_ PPH_STRINGREF BaseName,
    _Out_opt_ PULONG FoundIndex
    )
{
    PH_STRINGREF namePart;
    PH_STRINGREF remainingPart;

    remainingPart = List->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &namePart, &remainingPart);

        if (PhEqualStringRef(&namePart, BaseName, TRUE))
        {
            if (FoundIndex)
                *FoundIndex = (ULONG)(namePart.Buffer - List->Buffer);

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhIsPluginDisabled(
    _In_ PPH_STRINGREF BaseName
    )
{
    BOOLEAN found;
    PPH_STRING disabled;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    found = PhpLocateDisabledPlugin(disabled, BaseName, NULL);
    PhDereferenceObject(disabled);

    return found;
}

VOID PhSetPluginDisabled(
    _In_ PPH_STRINGREF BaseName,
    _In_ BOOLEAN Disable
    )
{
    BOOLEAN found;
    PPH_STRING disabled;
    ULONG foundIndex;
    PPH_STRING newDisabled;

    disabled = PhGetStringSetting(L"DisabledPlugins");

    found = PhpLocateDisabledPlugin(disabled, BaseName, &foundIndex);

    if (Disable && !found)
    {
        // We need to add the plugin to the disabled list.

        if (disabled->Length != 0)
        {
            // We have other disabled plugins. Append a pipe character followed by the plugin name.
            newDisabled = PhCreateStringEx(NULL, disabled->Length + sizeof(WCHAR) + BaseName->Length);
            memcpy(newDisabled->Buffer, disabled->Buffer, disabled->Length);
            newDisabled->Buffer[disabled->Length / 2] = '|';
            memcpy(&newDisabled->Buffer[disabled->Length / 2 + 1], BaseName->Buffer, BaseName->Length);
            PhSetStringSetting2(L"DisabledPlugins", &newDisabled->sr);
            PhDereferenceObject(newDisabled);
        }
        else
        {
            // This is the first disabled plugin.
            PhSetStringSetting2(L"DisabledPlugins", BaseName);
        }
    }
    else if (!Disable && found)
    {
        ULONG removeCount;

        // We need to remove the plugin from the disabled list.

        removeCount = (ULONG)BaseName->Length / 2;

        if (foundIndex + (ULONG)BaseName->Length / 2 < (ULONG)disabled->Length / 2)
        {
            // Remove the following pipe character as well.
            removeCount++;
        }
        else if (foundIndex != 0)
        {
            // Remove the preceding pipe character as well.
            foundIndex--;
            removeCount++;
        }

        newDisabled = PhCreateStringEx(NULL, disabled->Length - removeCount * sizeof(WCHAR));
        memcpy(newDisabled->Buffer, disabled->Buffer, foundIndex * sizeof(WCHAR));
        memcpy(&newDisabled->Buffer[foundIndex], &disabled->Buffer[foundIndex + removeCount],
            disabled->Length - removeCount * sizeof(WCHAR) - foundIndex * sizeof(WCHAR));
        PhSetStringSetting2(L"DisabledPlugins", &newDisabled->sr);
        PhDereferenceObject(newDisabled);
    }

    PhDereferenceObject(disabled);
}

PPHAPP_PLUGIN PhCreateDisabledPlugin(
    _In_ PPH_STRINGREF BaseName
    )
{
    PPHAPP_PLUGIN plugin;

    plugin = PhAllocate(sizeof(PHAPP_PLUGIN));
    memset(plugin, 0, sizeof(PHAPP_PLUGIN));

    plugin->Name.Length = BaseName->Length;
    plugin->Name.Buffer = PhAllocate(BaseName->Length + sizeof(WCHAR));
    memcpy(plugin->Name.Buffer, BaseName->Buffer, BaseName->Length);
    plugin->Name.Buffer[BaseName->Length / 2] = 0;

    return plugin;
}

PPH_HASHTABLE PluginHashtable = NULL;
PPH_LIST PluginList = NULL;
PH_QUEUED_LOCK PluginListLock = PH_QUEUED_LOCK_INIT;

PPH_STRING PluginGetVersionInfo(
    _In_ PPH_STRING FileName
    )
{
    ULONG versionSize;
    PVOID versionInfo;
    PUSHORT languageInfo;
    UINT language;
    UINT bufferSize = 0;
    PWSTR buffer;
    PPH_STRING internalName = NULL;
    PPH_STRING version = NULL;

    versionSize = GetFileVersionInfoSize(PhGetStringOrEmpty(FileName), NULL);
    versionInfo = PhAllocate(versionSize);
    memset(versionInfo, 0, versionSize);

    if (GetFileVersionInfo(PhGetStringOrEmpty(FileName), 0, versionSize, versionInfo))
    {
        if (VerQueryValue(versionInfo, L"\\", &buffer, &bufferSize))
        {
            VS_FIXEDFILEINFO* info = (VS_FIXEDFILEINFO*)buffer;

            if (info->dwSignature == 0xfeef04bd)
            {
                PH_FORMAT fileVersionFormat[7];

                PhInitFormatU(&fileVersionFormat[0], info->dwFileVersionMS >> 16);
                PhInitFormatC(&fileVersionFormat[1], '.');
                PhInitFormatU(&fileVersionFormat[2], info->dwFileVersionMS & 0xffff);
                PhInitFormatC(&fileVersionFormat[3], '.');
                PhInitFormatU(&fileVersionFormat[4], info->dwFileVersionLS >> 16);
                PhInitFormatC(&fileVersionFormat[5], '.');
                PhInitFormatU(&fileVersionFormat[6], info->dwFileVersionLS & 0xffff);

                version = PhFormat(fileVersionFormat, 7, 30);
            }
        }

        if (VerQueryValue(versionInfo, L"\\VarFileInfo\\Translation", &languageInfo, &language))
        {
            PPH_STRING internalNameString = PhFormatString(
                L"\\StringFileInfo\\%04x%04x\\InternalName",
                languageInfo[0],
                languageInfo[1]
                );

            if (VerQueryValue(versionInfo, PhGetStringOrEmpty(internalNameString), &buffer, &bufferSize))
            {
                internalName = PhCreateStringEx(buffer, bufferSize * sizeof(WCHAR));
            }

            PhDereferenceObject(internalNameString);
        }
    }

    PhFree(versionInfo);

    return version;
}


VOID EnumerateLoadedPlugins(
    _In_ PWCT_CONTEXT Context
    )
{
    PPH_AVL_LINKS root;
    PPH_AVL_LINKS links;

    root = PluginInstance->Links.Parent;

    while (root)
    {
        if (!root->Parent)
            break;

        root = root->Parent;
    }

    for (
        links = PhMinimumElementAvlTree((PPH_AVL_TREE)root); 
        links; 
        links = PhSuccessorElementAvlTree(links)
        )
    {
        HANDLE handle;
        IO_STATUS_BLOCK isb;
        FILE_BASIC_INFORMATION basic;
        PPHAPP_PLUGIN pluginInstance;
        PH_STRINGREF pluginBaseName;
        PPH_STRING pluginVersion;
        SYSTEMTIME utcTime, localTime;
        PPLUGIN_NODE entry;

        pluginInstance = CONTAINING_RECORD(links, PHAPP_PLUGIN, Links);

        PhInitializeStringRefLongHint(&pluginBaseName, PhGetPluginBaseName(pluginInstance));

        if (PhIsPluginDisabled(&pluginBaseName))
        {
            continue;
        }

        if (!RtlDoesFileExists_U(PhGetString(pluginInstance->FileName)))
        {
            continue;
        }

        entry = PhCreateAlloc(sizeof(PLUGIN_NODE));
        memset(entry, 0, sizeof(PLUGIN_NODE));
        memset(&basic, 0, sizeof(FILE_BASIC_INFORMATION));

        if (NT_SUCCESS(PhCreateFileWin32(
            &handle,
            PhGetString(pluginInstance->FileName),
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            NtQueryInformationFile(
                handle,
                &isb,
                &basic,
                sizeof(FILE_BASIC_INFORMATION),
                FileBasicInformation
                );

            NtClose(handle);
        }

        pluginVersion = PluginGetVersionInfo(pluginInstance->FileName);

        entry->State = PLUGIN_STATE_LOCAL;
        entry->PluginInstance = pluginInstance;
        entry->InternalName = PhCreateString2(&pluginInstance->Name);
        entry->Version = PhSubstring(pluginVersion, 0, pluginVersion->Length / sizeof(WCHAR) - 4);
        entry->Name = PhCreateString(pluginInstance->Information.DisplayName);
        entry->Author = PhCreateString(pluginInstance->Information.Author);
        entry->Description = PhCreateString(pluginInstance->Information.Description);
        entry->PluginOptions = pluginInstance->Information.HasOptions;
        entry->FilePath = PhCreateString2(&pluginInstance->FileName->sr);
        entry->FileName = PhGetBaseName(entry->FilePath);
    
        PhLargeIntegerToSystemTime(&utcTime, &basic.LastWriteTime);
        SystemTimeToTzSpecificLocalTime(NULL, &utcTime, &localTime);
        entry->UpdatedTime = PhFormatDateTime(&localTime);

        PhLargeIntegerToSystemTime(&utcTime, &basic.CreationTime);
        SystemTimeToTzSpecificLocalTime(NULL, &utcTime, &localTime);
        entry->AddedTime = PhFormatDateTime(&localTime);     

        PluginsAddTreeNode(Context, entry);
    }
}