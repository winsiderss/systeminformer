/*
 * Process Hacker Extra Plugins -
 *   UMDFHost Plugin
 *
 * Copyright (C) 2015 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phdk.h>

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION GetProcessTooltipTextCallbackRegistration;

static VOID GetProcessTooltipTextCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_GET_TOOLTIP_TEXT getTooltipText = Parameter;
    PPH_PROCESS_ITEM processItem;

    processItem = getTooltipText->Parameter;

    if (PhEqualStringRef2(&processItem->ProcessName->sr, L"WUDFHost.exe", TRUE))
    {
        HANDLE processHandle;
        PVOID environment;
        ULONG environmentLength;
        ULONG enumerationKey;
        PH_ENVIRONMENT_VARIABLE variable;

        PhAppendFormatStringBuilder(getTooltipText->StringBuilder, L"Drivers:\n");

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            processItem->ProcessId
            )))
        {
            ULONG flags = 0;

#ifdef _M_X64
            if (processItem->IsWow64)
                flags |= PH_GET_PROCESS_ENVIRONMENT_WOW64;
#endif

            if (NT_SUCCESS(PhGetProcessEnvironment(
                processHandle,
                flags,
                &environment,
                &environmentLength
                )))
            {
                enumerationKey = 0;

                while (PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable))
                {
                    PPH_STRING nameString;
                    PPH_STRING valueString;

                    // Don't display pairs with no name.
                    if (variable.Name.Length == 0)
                        continue;

                    // The strings are not guaranteed to be null-terminated, so we need to create some temporary strings.
                    nameString = PhCreateStringEx(variable.Name.Buffer, variable.Name.Length);
                    valueString = PhCreateStringEx(variable.Value.Buffer, variable.Value.Length);

                    if (PhEqualString2(nameString, L"ACTIVE_DEVICES", TRUE))
                    {
                        PWSTR stringToken = NULL;
                        PWSTR context = NULL;

                        stringToken = wcstok_s(valueString->Buffer, L";", &context);

                        while (stringToken)
                        {
                            HANDLE driverKeyHandle;
                            PPH_STRING driverKeyPath;
                            
                            driverKeyPath = PhConcatStrings2(L"System\\CurrentControlSet\\Enum\\", stringToken);

                            if (NT_SUCCESS(PhOpenKey(
                                &driverKeyHandle,
                                KEY_READ,
                                PH_KEY_LOCAL_MACHINE,
                                &driverKeyPath->sr,
                                0
                                )))
                            {
                                PPH_STRING deviceName;

                                if (deviceName = PhQueryRegistryString(driverKeyHandle, L"DeviceDesc"))
                                {
                                    PWSTR deviceDescToken = NULL;

                                    if (PhFindCharInString(deviceName, 0, ';') != -1)
                                    {
                                        PH_STRINGREF part;
                                        PH_STRINGREF remaining = deviceName->sr;

                                        while (remaining.Length != 0)
                                        {
                                            PhSplitStringRefAtChar(&remaining, ';', &part, &remaining);
                                            
                                            // TODO: Split the DeviceDesc string into the ini Filename and KeyName for loading the driver description...
                                            //     For now just loop until we have the last part of the string (which should contain a description).

                                            //PPH_STRING iniFilePath;
                                            //WCHAR systemDirectory[MAX_PATH];
                                            //GetSystemDirectory(systemDirectory, _countof(systemDirectory));
                                            //iniFilePath = PhConcatStrings(3, systemDirectory, L"\\", part.Buffer);
                                            //GetPrivateProfileString(
                                            //    L"Strings",
                                            //    part.Buffer,
                                            //    NULL, 
                                            //    &outString, 
                                            //    _countof(outString), 
                                            //    iniFilePath->Buffer
                                            //    );

                                            deviceDescToken = part.Buffer;
                                        }
                                    }
                                    else
                                    {
                                        deviceDescToken = deviceName->Buffer;
                                    }

                                    if (deviceDescToken)
                                    {
                                        PhAppendFormatStringBuilder(
                                            getTooltipText->StringBuilder, 
                                            L"    %s\n", 
                                            deviceDescToken
                                            );
                                    }

                                    PhDereferenceObject(deviceName);
                                }

                                NtClose(driverKeyHandle);
                            }

                            PhDereferenceObject(driverKeyPath);

                            stringToken = wcstok_s(NULL, L";", &context);
                        }
                    }

                    PhDereferenceObject(nameString);
                    PhDereferenceObject(valueString);
                }

                PhFreePage(environment);
            }

            NtClose(processHandle);
        }
    }
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"dmex.UMDFHostPlugin", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"UMDFHost Process Plugin";
            info->Author = L"dmex";
            info->Description = L"Plugin for showing UMDFHost driver instances via Tooltip.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackGetProcessTooltipText),
                GetProcessTooltipTextCallback,
                NULL,
                &GetProcessTooltipTextCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}