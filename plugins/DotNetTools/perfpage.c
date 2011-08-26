/*
 * Process Hacker .NET Tools -
 *   .NET Performance property page
 *
 * Copyright (C) 2011 wj32
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

#define CINTERFACE
#define COBJMACROS
#include "dn.h"
#include "resource.h"
#include <windowsx.h>
#include <corpub.h>

typedef struct _PERFPAGE_CONTEXT
{
    HWND WindowHandle;
    PPH_PROCESS_ITEM ProcessItem;
    BOOLEAN Enabled;
    PPH_STRING InstanceName;
} PERFPAGE_CONTEXT, *PPERFPAGE_CONTEXT;

INT_PTR CALLBACK DotNetPerfPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static GUID CLSID_CorpubPublish_I = { 0x047a9a40, 0x657e, 0x11d3, { 0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef } };
static GUID IID_ICorPublish_I = { 0x9613a0e7, 0x5a68, 0x11d3, { 0x8f, 0x84, 0x00, 0xa0, 0xc9, 0xb4, 0xd5, 0x0c } };

static PH_INITONCE DotNetObjectTypeInfoInitOnce = PH_INITONCE_INIT;
static PPERF_OBJECT_TYPE_INFO DotNetObjectTypeInfo = NULL;
static ULONG DotNetObjectTypeInfoCount = 0;
static PVOID PerfInfoTextData = NULL;

VOID AddPerfPageToPropContext(
    __in PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    )
{
    PhAddProcessPropPage(
        PropContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCDOTNETPERF), DotNetPerfPageDlgProc, NULL)
        );
}

HRESULT CreateCorpubPublish(
    __in HANDLE ProcessId,
    __out ICorPublish **Publish
    )
{
    HRESULT result;
    PPH_IS_DOT_NET_CONTEXT isDotNetContext;
    ULONG flags;
    BOOLEAN clrV4;

    clrV4 = FALSE;

    if (NT_SUCCESS(PhCreateIsDotNetContext(&isDotNetContext, NULL, 0)))
    {
        if (PhGetProcessIsDotNetFromContext(isDotNetContext, ProcessId, &flags))
        {
            if (flags & PH_IS_DOT_NET_VERSION_4)
                clrV4 = TRUE;
        }

        PhFreeIsDotNetContext(isDotNetContext);
    }

    // Using CoCreateInstance always seems to create a v2-compatible class, but not a v4-compatible one.
    // For v4 we have to manually load the correct version of mscordbi.dll.

    if (clrV4)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static HMODULE mscordbiDllBase;

        if (PhBeginInitOnce(&initOnce))
        {
            PH_STRINGREF systemRootString;
            PH_STRINGREF mscordbiPathString;
            PPH_STRING mscordbiFileName;

            LoadLibrary(L"mscoree.dll");

            PhGetSystemRoot(&systemRootString);
#ifdef _M_X64
            PhInitializeStringRef(&mscordbiPathString, L"\\Microsoft.NET\\Framework64\\v4.0.30319\\mscordbi.dll");
#else
            PhInitializeStringRef(&mscordbiPathString, L"\\Microsoft.NET\\Framework\\v4.0.30319\\mscordbi.dll");
#endif

            mscordbiFileName = PhConcatStringRef2(&systemRootString, &mscordbiPathString);
            mscordbiDllBase = LoadLibrary(mscordbiFileName->Buffer);
            PhDereferenceObject(mscordbiFileName);

            PhEndInitOnce(&initOnce);
        }

        if (mscordbiDllBase)
        {
            HRESULT (__stdcall *dllGetClassObject)(REFCLSID, REFIID, LPVOID *);

            dllGetClassObject = (PVOID)GetProcAddress(mscordbiDllBase, "DllGetClassObjectInternal");

            if (dllGetClassObject)
            {
                IClassFactory *factory;

                if (SUCCEEDED(dllGetClassObject(&CLSID_CorpubPublish_I, &IID_IClassFactory, &factory)))
                {
                    result = IClassFactory_CreateInstance(factory, NULL, &IID_ICorPublish_I, Publish);
                    IClassFactory_Release(factory);

                    return result;
                }
            }
        }
    }

    return CoCreateInstance(&CLSID_CorpubPublish_I, NULL, CLSCTX_INPROC_SERVER, &IID_ICorPublish_I, Publish);
}

HRESULT GetCorPublishProcess(
    __in HANDLE ProcessId,
    __out ICorPublishProcess **PublishProcess
    )
{
    HRESULT result;
    ICorPublish *publish;

    if (SUCCEEDED(CreateCorpubPublish(ProcessId, &publish)))
    {
        result = ICorPublish_GetProcess(publish, (ULONG)ProcessId, PublishProcess);
        ICorPublish_Release(publish);
    }

    return result;
}

VOID InitializeDotNetObjectTypeInfo(
    VOID
    )
{
    if (PhBeginInitOnce(&DotNetObjectTypeInfoInitOnce))
    {
        PH_STRINGREF nameList;

        // Use a pre-defined list because enumerating all object types is extremely slow.
        PhInitializeStringRef(
            &nameList,
            L".NET CLR Data;"
            L".NET CLR Exceptions;"
            L".NET CLR Interop;"
            L".NET CLR Jit;"
            L".NET CLR Loading;"
            L".NET CLR LocksAndThreads;"
            L".NET CLR Memory;"
            L".NET CLR Remoting;"
            L".NET CLR Security"
            );
        GetPerfObjectTypeInfo2(&nameList, &DotNetObjectTypeInfo, &DotNetObjectTypeInfoCount, &PerfInfoTextData);

        PhEndInitOnce(&DotNetObjectTypeInfoInitOnce);
    }
}

VOID AddProcessAppDomains(
    __in HWND hwndDlg,
    __in PPERFPAGE_CONTEXT Context
    )
{
    HWND appDomainsLv;
    ICorPublishProcess *publishProcess;
    ICorPublishAppDomainEnum *publishAppDomainEnum;
    ICorPublishAppDomain *publishAppDomain;
    ULONG returnCount;
    ULONG appDomainNameCount;
    WCHAR appDomainName[256];

    appDomainsLv = GetDlgItem(hwndDlg, IDC_APPDOMAINS);

    SendMessage(appDomainsLv, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(appDomainsLv);

    if (SUCCEEDED(GetCorPublishProcess(Context->ProcessItem->ProcessId, &publishProcess)))
    {
        if (SUCCEEDED(ICorPublishProcess_EnumAppDomains(publishProcess, &publishAppDomainEnum)))
        {
            while (TRUE)
            {
                if (!SUCCEEDED(ICorPublishAppDomainEnum_Next(publishAppDomainEnum, 1, &publishAppDomain, &returnCount)))
                    break;
                if (returnCount == 0)
                    break;

                if (SUCCEEDED(ICorPublishAppDomain_GetName(publishAppDomain, 256, &appDomainNameCount, appDomainName)))
                {
                    PhAddListViewItem(appDomainsLv, MAXINT, appDomainName, NULL);
                }

                ICorPublishAppDomain_Release(publishAppDomain);
            }

            ICorPublishAppDomainEnum_Release(publishAppDomainEnum);
        }

        ICorPublishProcess_Release(publishProcess);
    }

    SendMessage(appDomainsLv, WM_SETREDRAW, TRUE, 0);
}

PPERF_OBJECT_TYPE_INFO GetSelectedObjectTypeInfo(
    __in HWND hwndDlg,
    __in PPERFPAGE_CONTEXT Context
    )
{
    PPH_STRING selectedText;
    ULONG i;

    selectedText = PhGetWindowText(GetDlgItem(hwndDlg, IDC_CATEGORIES));

    for (i = 0; i < DotNetObjectTypeInfoCount; i++)
    {
        if (PhEqualStringRef(&DotNetObjectTypeInfo[i].Name, &selectedText->sr, FALSE))
        {
            return &DotNetObjectTypeInfo[i];
        }
    }

    return NULL;
}

VOID UpdateCounterData(
    __in HWND hwndDlg,
    __in PPERFPAGE_CONTEXT Context,
    __in BOOLEAN RefreshCategory
    )
{
    HWND countersLv;
    PPERF_OBJECT_TYPE_INFO typeInfo;
    WCHAR indexString[PH_INT32_STR_LEN_1];
    PVOID textData;
    PVOID data;
    PPERF_DATA_BLOCK block;
    ULONG i;
    PPERF_OBJECT_TYPE objectType;
    ULONG j;
    PPERF_COUNTER_DEFINITION counter;
    PPERF_INSTANCE_DEFINITION instance;

    countersLv = GetDlgItem(hwndDlg, IDC_COUNTERS);

    if (RefreshCategory)
        ListView_DeleteAllItems(countersLv);

    typeInfo = GetSelectedObjectTypeInfo(hwndDlg, Context);

    if (!typeInfo)
        return;

    if (PerfInfoTextData)
    {
        textData = PerfInfoTextData;
    }
    else
    {
        if (!QueryPerfInfoVariableSize(HKEY_PERFORMANCE_DATA, L"Counter 009", &textData, NULL))
            return;
    }

    PhPrintUInt32(indexString, typeInfo->NameIndex);

    if (!QueryPerfInfoVariableSize(HKEY_PERFORMANCE_DATA, indexString, &data, NULL))
    {
        PhFree(textData);
        return;
    }

    block = data;
    objectType = (PPERF_OBJECT_TYPE)((PCHAR)block + block->HeaderLength);

    for (i = 0; i < block->NumObjectTypes; i++)
    {
        if (objectType->ObjectNameTitleIndex == typeInfo->NameIndex && objectType->NumInstances != PERF_NO_INSTANCES)
        {
            PPERF_COUNTER_BLOCK counterBlock;
            BOOLEAN instanceFound = FALSE;

            // Find the instance that corresponds with the process.

            instance = (PPERF_INSTANCE_DEFINITION)((PCHAR)objectType + objectType->DefinitionLength);

            for (j = 0; j < (ULONG)objectType->NumInstances; j++)
            {
                PH_STRINGREF instanceName;

                if (instance->NameLength != 0)
                {
                    instanceName.Buffer = (PWSTR)((PCHAR)instance + instance->NameOffset);
                    instanceName.Length = (USHORT)(instance->NameLength - sizeof(WCHAR));

                    counterBlock = (PPERF_COUNTER_BLOCK)((PCHAR)instance + instance->ByteLength);

                    if (PhEqualStringRef(&instanceName, &Context->InstanceName->sr, TRUE))
                    {
                        instanceFound = TRUE;
                        break;
                    }
                }

                instance = (PPERF_INSTANCE_DEFINITION)((PCHAR)counterBlock + counterBlock->ByteLength);
            }

            // Get the counter values.

            counter = (PPERF_COUNTER_DEFINITION)((PCHAR)objectType + objectType->HeaderLength);

            for (j = 0; j < objectType->NumCounters; j++)
            {
                INT lvItemIndex = -1;

                if (
                    counter->CounterType != PERF_COUNTER_RAWCOUNT &&
                    counter->CounterType != PERF_COUNTER_LARGE_RAWCOUNT &&
                    counter->CounterType != PERF_RAW_FRACTION
                    )
                {
                    goto EndOfLoop;
                }

                if (RefreshCategory)
                {
                    PWSTR counterName;

                    counterName = FindPerfTextInTextData(textData, counter->CounterNameTitleIndex);

                    if (counterName)
                        lvItemIndex = PhAddListViewItem(countersLv, MAXINT, counterName, (PVOID)counter->CounterNameTitleIndex);
                }
                else
                {
                    lvItemIndex = PhFindListViewItemByParam(countersLv, -1, (PVOID)counter->CounterNameTitleIndex);
                }

                if (lvItemIndex != -1 && instanceFound)
                {
                    switch (counter->CounterType)
                    {
                    case PERF_COUNTER_RAWCOUNT:
                        {
                            PULONG value = (PULONG)((PCHAR)counterBlock + counter->CounterOffset);

                            PhSetListViewSubItem(countersLv, lvItemIndex, 1, PhaFormatUInt64(*value, TRUE)->Buffer);
                        }
                        break;
                    case PERF_COUNTER_LARGE_RAWCOUNT:
                        {
                            PULONG64 value = (PULONG64)((PCHAR)counterBlock + counter->CounterOffset);

                            PhSetListViewSubItem(countersLv, lvItemIndex, 1, PhaFormatUInt64(*value, TRUE)->Buffer);
                        }
                        break;
                    case PERF_RAW_FRACTION:
                        {
                            PULONG value = (PULONG)((PCHAR)counterBlock + counter->CounterOffset);
                            PPERF_COUNTER_DEFINITION denomCounter = (PPERF_COUNTER_DEFINITION)((PCHAR)counter + counter->ByteLength);
                            PULONG denomValue = (PULONG)((PCHAR)counterBlock + denomCounter->CounterOffset);
                            PH_FORMAT format;
                            WCHAR formatBuffer[10];

                            if (*denomValue != 0)
                            {
                                PhInitFormatF(&format, (FLOAT)*value * 100 / (FLOAT)*denomValue, 2);

                                if (PhFormatToBuffer(&format, 1, formatBuffer, sizeof(formatBuffer), NULL))
                                    PhSetListViewSubItem(countersLv, lvItemIndex, 1, formatBuffer);
                            }
                            else
                            {
                                PhSetListViewSubItem(countersLv, lvItemIndex, 1, L"0.00");
                            }
                        }
                        break;
                    }
                }

EndOfLoop:
                counter = (PPERF_COUNTER_DEFINITION)((PCHAR)counter + counter->ByteLength);
            }
        }

        objectType = (PPERF_OBJECT_TYPE)((PCHAR)objectType + objectType->TotalByteLength);
    }

    PhFree(data);

    if (textData != PerfInfoTextData)
        PhFree(textData);
}

INT_PTR CALLBACK DotNetPerfPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPERFPAGE_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG indexOfLastDot;
            HWND appDomainsLv;
            HWND countersLv;
            HWND categoriesHandle;
            ULONG i;

            context = PhAllocate(sizeof(PERFPAGE_CONTEXT));
            memset(context, 0, sizeof(PERFPAGE_CONTEXT));
            propPageContext->Context = context;
            context->WindowHandle = hwndDlg;
            context->ProcessItem = processItem;
            context->Enabled = TRUE;

            // The .NET counters remove the file name extension in the instance names, even if the
            // extension is something other than ".exe".

            indexOfLastDot = PhFindLastCharInString(context->ProcessItem->ProcessName, 0, '.');

            if (indexOfLastDot != -1)
            {
                context->InstanceName = PhSubstring(context->ProcessItem->ProcessName, 0, indexOfLastDot);
            }
            else
            {
                context->InstanceName = context->ProcessItem->ProcessName;
                PhReferenceObject(context->InstanceName);
            }

            appDomainsLv = GetDlgItem(hwndDlg, IDC_APPDOMAINS);
            PhSetListViewStyle(appDomainsLv, FALSE, TRUE);
            PhSetControlTheme(appDomainsLv, L"explorer");
            PhAddListViewColumn(appDomainsLv, 0, 0, 0, LVCFMT_LEFT, 300, L"Application domain");

            countersLv = GetDlgItem(hwndDlg, IDC_COUNTERS);
            PhSetListViewStyle(countersLv, FALSE, TRUE);
            PhSetControlTheme(countersLv, L"explorer");
            PhAddListViewColumn(countersLv, 0, 0, 0, LVCFMT_LEFT, 250, L"Counter");
            PhAddListViewColumn(countersLv, 1, 1, 1, LVCFMT_RIGHT, 140, L"Value");

            InitializeDotNetObjectTypeInfo();
            AddProcessAppDomains(hwndDlg, context);

            categoriesHandle = GetDlgItem(hwndDlg, IDC_CATEGORIES);

            for (i = 0; i < DotNetObjectTypeInfoCount; i++)
            {
                PPERF_OBJECT_TYPE_INFO info = &DotNetObjectTypeInfo[i];

                ComboBox_AddString(categoriesHandle, PhaCreateStringEx(info->Name.Buffer, info->Name.Length)->Buffer);
            }

            ComboBox_SelectString(categoriesHandle, -1, L".NET CLR Memory"); // select a default item
            UpdateCounterData(hwndDlg, context, TRUE);
            SetTimer(hwndDlg, 1, 1000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            if (context->InstanceName)
                PhDereferenceObject(context->InstanceName);

            PhFree(context);

            PhPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_APPDOMAINS), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CATEGORIES), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COUNTERS), dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_CATEGORIES:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    UpdateCounterData(hwndDlg, context, TRUE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            }

            PhHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_APPDOMAINS));
            PhHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_COUNTERS));
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 1 && context->Enabled)
            {
                UpdateCounterData(hwndDlg, context, FALSE);
            }
        }
        break;
    }

    return FALSE;
}
