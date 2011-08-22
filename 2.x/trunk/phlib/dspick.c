/*
 * Process Hacker - 
 *   DS object picker wrapper
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>
#include <dspick.h>
#define CINTERFACE
#define COBJMACROS
#include <objsel.h>

#define IDataObject_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IDataObject_Release(This) ((This)->lpVtbl->Release(This))
#define IDataObject_GetData(This, pformatetcIn, pmedium) ((This)->lpVtbl->GetData(This, pformatetcIn, pmedium))

#define IDsObjectPicker_QueryInterface(This, riid, ppvObject) ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IDsObjectPicker_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IDsObjectPicker_Release(This) ((This)->lpVtbl->Release(This))
#define IDsObjectPicker_Initialize(This, pInitInfo) ((This)->lpVtbl->Initialize(This, pInitInfo))
#define IDsObjectPicker_InvokeDialog(This, hwndParent, ppdoSelections) ((This)->lpVtbl->InvokeDialog(This, hwndParent, ppdoSelections))

IDsObjectPicker *PhpCreateDsObjectPicker(
    VOID
    )
{
    static CLSID CLSID_DsObjectPicker_I = { 0x17d6ccd8, 0x3b7b, 0x11d2, { 0xb9, 0xe0, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 } };
    static IID IID_IDsObjectPicker_I = { 0x0c87e64e, 0x3b7a, 0x11d2, { 0xb9, 0xe0, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 } };

    IDsObjectPicker *picker;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_DsObjectPicker_I,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IDsObjectPicker_I,
        &picker
        )))
    {
        return picker;
    }
    else
    {
        return NULL;
    }
}

VOID PhFreeDsObjectPickerDialog(
    __in PVOID PickerDialog
    )
{
    IDsObjectPicker_Release((IDsObjectPicker *)PickerDialog);
}

PVOID PhCreateDsObjectPickerDialog(
    __in ULONG Flags
    )
{
    IDsObjectPicker *picker;
    DSOP_INIT_INFO initInfo;
    DSOP_SCOPE_INIT_INFO scopeInit[1];

    picker = PhpCreateDsObjectPicker();

    if (!picker)
        return NULL;

    memset(scopeInit, 0, sizeof(scopeInit));

    scopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    scopeInit[0].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    scopeInit[0].flScope =
        DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |
        DSOP_SCOPE_FLAG_WANT_SID_PATH |
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;
    scopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_INCLUDE_ADVANCED_VIEW |
        DSOP_FILTER_USERS |
        DSOP_FILTER_BUILTIN_GROUPS |
        DSOP_FILTER_WELL_KNOWN_PRINCIPALS;
    scopeInit[0].FilterFlags.flDownlevel =
        DSOP_DOWNLEVEL_FILTER_USERS |
        DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS |
        DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS |
        DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

    memset(&initInfo, 0, sizeof(DSOP_INIT_INFO));
    initInfo.cbSize = sizeof(DSOP_INIT_INFO);
    initInfo.pwzTargetComputer = NULL;
    initInfo.cDsScopeInfos = 1;
    initInfo.aDsScopeInfos = scopeInit;
    initInfo.flOptions = DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK;

    if (Flags & PH_DSPICK_MULTISELECT)
        initInfo.flOptions |= DSOP_FLAG_MULTISELECT;

    if (!SUCCEEDED(IDsObjectPicker_Initialize(picker, &initInfo)))
    {
        IDsObjectPicker_Release(picker);
        return NULL;
    }

    return picker;
}

PDS_SELECTION_LIST PhpGetDsSelectionList(
    __in IDataObject *Selections
    )
{
    FORMATETC format;
    STGMEDIUM medium;

    format.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(L"CFSTR_DSOP_DS_SELECTION_LIST");
    format.ptd = NULL;
    format.dwAspect = -1;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    if (SUCCEEDED(IDataObject_GetData(Selections, &format, &medium)))
    {
        if (medium.tymed != TYMED_HGLOBAL)
            return NULL;

        return (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);
    }
    else
    {
        return NULL;
    }
}

BOOLEAN PhShowDsObjectPickerDialog(
    __in HWND hWnd,
    __in PVOID PickerDialog,
    __out PPH_DSPICK_OBJECTS *Objects
    )
{
    IDsObjectPicker *picker;
    IDataObject *dataObject;
    PDS_SELECTION_LIST selections;
    PPH_DSPICK_OBJECTS objects;
    ULONG i;

    picker = (IDsObjectPicker *)PickerDialog;

    if (!SUCCEEDED(IDsObjectPicker_InvokeDialog(picker, hWnd, &dataObject)))
        return FALSE;

    if (!dataObject)
        return FALSE;

    selections = PhpGetDsSelectionList(dataObject);
    IDataObject_Release(dataObject);

    if (!selections)
        return FALSE;

    objects = PhAllocate(
        FIELD_OFFSET(PH_DSPICK_OBJECTS, Objects) +
        selections->cItems * sizeof(PH_DSPICK_OBJECT)
        );

    objects->NumberOfObjects = selections->cItems;

    for (i = 0; i < selections->cItems; i++)
    {
        PDS_SELECTION selection;
        PSID sid;
        PH_STRINGREF path;
        PH_STRINGREF prefix;

        selection = &selections->aDsSelection[i];

        objects->Objects[i].Name = PhCreateString(selection->pwzName);
        objects->Objects[i].Sid = NULL;

        if (selection->pwzADsPath && selection->pwzADsPath[0] != 0)
        {
            PhInitializeStringRef(&path, selection->pwzADsPath);
            PhInitializeStringRef(&prefix, L"LDAP://<SID=");

            if (PhStartsWithStringRef(&path, &prefix, TRUE))
            {
                path.Buffer += prefix.Length / sizeof(WCHAR);
                path.Length -= sizeof(WCHAR); // ignore ">" at end

                sid = PhAllocate(path.Length / sizeof(WCHAR) / 2);

                if (PhHexStringToBuffer(&path, (PUCHAR)sid))
                {
                    if (RtlValidSid(sid))
                        objects->Objects[i].Sid = sid;
                    else
                        PhFree(sid);
                }
                else
                {
                    PhFree(sid);
                }
            }
        }
        else
        {
            // Try to get the SID.
            PhLookupName(&objects->Objects[i].Name->sr, &objects->Objects[i].Sid, NULL, NULL);
        }
    }

    *Objects = objects;

    return TRUE;
}

VOID PhFreeDsObjectPickerObjects(
    __in PPH_DSPICK_OBJECTS Objects
    )
{
    ULONG i;

    for (i = 0; i < Objects->NumberOfObjects; i++)
    {
        PhDereferenceObject(Objects->Objects[i].Name);

        if (Objects->Objects[i].Sid)
            PhFree(Objects->Objects[i].Sid);
    }

    PhFree(Objects);
}
