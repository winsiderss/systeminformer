#ifndef _PH_DSPICK_H
#define _PH_DSPICK_H

#define PH_DSPICK_MULTISELECT 0x1

typedef struct _PH_DSPICK_OBJECT
{
    PPH_STRING Name;
    PSID Sid;
} PH_DSPICK_OBJECT, *PPH_DSPICK_OBJECT;

typedef struct _PH_DSPICK_OBJECTS
{
    ULONG NumberOfObjects;
    PH_DSPICK_OBJECT Objects[1];
} PH_DSPICK_OBJECTS, *PPH_DSPICK_OBJECTS;

PHLIBAPI
VOID PhFreeDsObjectPickerDialog(
    __in PVOID PickerDialog
    );

PHLIBAPI
PVOID PhCreateDsObjectPickerDialog(
    __in ULONG Flags
    );

PHLIBAPI
BOOLEAN PhShowDsObjectPickerDialog(
    __in HWND hWnd,
    __in PVOID PickerDialog,
    __out PPH_DSPICK_OBJECTS *Objects
    );

PHLIBAPI
VOID PhFreeDsObjectPickerObjects(
    __in PPH_DSPICK_OBJECTS Objects
    );

#endif
