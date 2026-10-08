/*
 * Win32k NT User definitions.
 *
 * This file is part of System Informer.
 */

#ifndef _NTUSER_H
#define _NTUSER_H

 // SDK rimext.h types used by the native raw input entry points.
struct RIM_DEVICE_PROPERTIES;
struct _RIM_USAGE_ANDPAGE;
struct _RIMIDE_GENERIC_HID_DEVICE_PROPERTIES;
// Private flip-property record; layout intentionally opaque.
struct FlipPropertyItem;
// DirectDraw and Direct3D driver structures used by GdiEntry routines.
struct _DDRAWI_DIRECTDRAW_GBL;
struct _DDRAWI_DIRECTDRAW_LCL;
struct _DDRAWI_DDRAWSURFACE_LCL;
struct _DDHALINFO;
struct _DDHAL_DDCALLBACKS;
struct _DDHAL_DDSURFACECALLBACKS;
struct _DDHAL_DDPALETTECALLBACKS;
struct _D3DHAL_CALLBACKS;
struct _D3DHAL_GLOBALDRIVERDATA;
struct _DDHAL_DDEXEBUFCALLBACKS;
struct _DDSURFACEDESC;
struct _VIDMEM;

typedef struct _DOCONNECTDATA* PDOCONNECTDATA;
typedef struct _CACHESTATISTICS* PCACHESTATISTICS;
typedef struct _DONOTIFYDATA* PDONOTIFYDATA;
typedef struct _LARGE_STRING* PLARGE_STRING;

typedef BOOL(NTAPI* PRIM_DEVICE_CHANGE_CALLBACK)(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DeviceIdentity,
    _In_ ULONG Code,
    _In_ ULONG DeviceType,
    _In_ ULONG InputType,
    _In_ USHORT Usage,
    _In_ USHORT UsagePage,
    _In_opt_ PVOID Context
    );

/**
 * Server-function index (SFI_*) selectors passed to the NtUserCall* dispatch family
 * (NtUserCallNoParam / OneParam / TwoParam / Hwnd*). Values are build-specific.
 */
#define SFI_CREATEMENU                              0 // NtUserCallNoParam
#define SFI_CREATEPOPUPMENU                         1 // NtUserCallNoParam
#define SFI_ALLOWFOREGROUNDACTIVATION               2 // NtUserCallNoParam
#define SFI_CANCELQUEUEEVENTCOMPLETIONPACKET        3 // NtUserCallNoParam
#define SFI_CLEARWAKEMASK                           4 // NtUserCallNoParam
#define SFI_CREATESYSTEMTHREADS                     5 // NtUserCallNoParam
#define SFI_DESTROYCARET                            6 // NtUserCallNoParam
#define SFI_DISABLEPROCESSWINDOWSGHOSTING           7 // NtUserCallNoParam
#define SFI_DRAINTHREADCOREMESSAGINGCOMPLETIONS     8 // NtUserCallNoParam
#define SFI_GETDEVICECHANGEINFO                     9 // NtUserCallNoParam
#define SFI_GETIMESHOWSTATUS                        10 // NtUserCallNoParam
#define SFI_GETINPUTDESKTOP                         11 // NtUserCallNoParam
#define SFI_GETMESSAGEPOS                           12 // NtUserCallNoParam
#define SFI_GETQUEUEIOCP                            13 // NtUserCallNoParam
#define SFI_GETUNPREDICTEDMESSAGEPOS                14 // NtUserCallNoParam
#define SFI_HANDLESYSTEMTHREADCREATIONFAILURE       15 // NtUserCallNoParam
#define SFI_HIDECURSORNOCAPTURE                     16 // NtUserCallNoParam
#define SFI_ISQUEUEATTACHED                         17 // NtUserCallNoParam
#define SFI_LOADCURSORSANDICONS                     18 // NtUserCallNoParam
#define SFI_LOADUSERAPIHOOK                         19 // NtUserCallNoParam
#define SFI_PREPAREFORLOGOFF                        20 // NtUserCallNoParam
#define SFI_REASSOCIATEQUEUEEVENTCOMPLETIONPACKET   21 // NtUserCallNoParam
#define SFI_RELEASECAPTURE                          22 // NtUserCallNoParam
#define SFI_REMOVEQUEUECOMPLETION                   23 // NtUserCallNoParam
#define SFI_RESETDBLCLK                             24 // NtUserCallNoParam
#define SFI_ZAPACTIVEANDFOCUS                       25 // NtUserCallNoParam
#define SFI_REMOTECONSOLESHADOWSTOP                 26 // NtUserCallNoParam
#define SFI_REMOTEDISCONNECT                        27 // NtUserCallNoParam
#define SFI_REMOTESHADOWSETUP                       30 // NtUserCallNoParam
#define SFI_REMOTESHADOWSTOP                        31 // NtUserCallNoParam
#define SFI_REMOTEPASSTHRUENABLE                    32 // NtUserCallNoParam
#define SFI_REMOTEPASSTHRUDISABLE                   33 // NtUserCallNoParam
#define SFI_REMOTECONNECTSTATE                      34 // NtUserCallNoParam
#define SFI_UPDATEPERUSERIMMENABLING                36 // NtUserCallNoParam
#define SFI_USERPOWERCALLOUTWORKER                  37 // NtUserCallNoParam
#define SFI_WAKERITFORSHUTDOWN                      38 // NtUserCallNoParam
#define SFI_DOINITMESSAGEPUMPHOOK                   39 // NtUserCallNoParam
#define SFI_DOUNINITMESSAGEPUMPHOOK                 40 // NtUserCallNoParam
#define SFI_ENABLEMOUSEINPOINTERFORTHREAD           41 // NtUserCallNoParam
#define SFI_DEFERREDDESKTOPROTATION                 42 // NtUserCallNoParam
#define SFI_ENABLEPERMONITORMENUSCALING             43 // NtUserCallNoParam
#define SFI_BEGINDEFERWINDOWPOS                     44 // NtUserCallOneParam(int NumWindows)
#define SFI_GETSENDMESSAGERECEIVER                  45 // NtUserCallOneParam
#define SFI_ALLOWSETFOREGROUNDWINDOW                46 // NtUserCallOneParam(DWORD ProcessId)
#define SFI_CSDDEUNINITIALIZE                       47 // NtUserCallOneParam
#define SFI_ENUMCLIPBOARDFORMATS                    49 // NtUserCallOneParam(UINT Format)
#define SFI_GETINPUTEVENT                           50 // NtUserCallOneParam
#define SFI_GETKEYBOARDTYPE                         51 // NtUserCallOneParam(int TypeFlag)
#define SFI_GETPROCESSDEFAULTLAYOUT                 52 // NtUserCallOneParam(PDWORD DefaultLayout)
#define SFI_GETWINSTATIONINFO                       53 // NtUserCallOneParam
#define SFI_LOCKSETFOREGROUNDWINDOW                 54 // NtUserCallOneParam(UINT LockCode)
#define SFI_LW_LOADFONTS                            55 // NtUserCallOneParam
#define SFI_MAPDESKTOPOBJECT                        56 // NtUserCallOneParam
#define SFI_MESSAGEBEEP                             57 // NtUserCallOneParam(UINT Type)
#define SFI_PLAYEVENTSOUND                          58 // NtUserCallOneParam
#define SFI_POSTQUITMESSAGE                         59 // NtUserCallOneParam(int ExitCode)
#define SFI_REALIZEPALETTE                          60 // NtUserCallOneParam
#define SFI_REGISTERLPK                             61 // NtUserCallOneParam
#define SFI_REGISTERSYSTEMTHREAD                    62 // NtUserCallOneParam
#define SFI_REMOTERECONNECT                         63 // NtUserCallOneParam
#define SFI_REMOTETHINWIRESTATS                     64 // NtUserCallOneParam
#define SFI_REMOTENOTIFY                            65 // NtUserCallOneParam
#define SFI_REPLYMESSAGE                            66 // NtUserCallOneParam(LRESULT Result)
#define SFI_SETCARETBLINKTIME                       67 // NtUserCallOneParam(UINT MSeconds)
#define SFI_SETDOUBLECLICKTIME                      68 // NtUserCallOneParam(UINT Interval)
#define SFI_SETMESSAGEEXTRAINFO                     69 // NtUserCallOneParam(LPARAM lParam)
#define SFI_SETPROCESSDEFAULTLAYOUT                 70 // NtUserCallOneParam(DWORD DefaultLayout)
#define SFI_SETWATERMARKSTRINGS                     71 // NtUserCallOneParam
#define SFI_SHOWSTARTGLASS                          72 // NtUserCallOneParam
#define SFI_SWAPMOUSEBUTTON                         73 // NtUserCallOneParam(BOOL Swap)
#define SFI_WOWMODULEUNLOAD                         74 // NtUserCallOneParam
#define SFI_DWMLOCKSCREENUPDATES                    75 // NtUserCallOneParam
#define SFI_ENABLESESSIONFORMMCSS                   76 // NtUserCallOneParam
#define SFI_SETWAITFORQUEUEATTACH                   77 // NtUserCallOneParam
#define SFI_THREADMESSAGEQUEUEATTACHED              78 // NtUserCallOneParam
#define SFI_ENSUREDPIDEPSYSMETCACHEFORPLATEAU       80 // NtUserCallOneParam
#define SFI_FORCEENABLENUMPADTRANSLATION            81 // NtUserCallOneParam
#define SFI_SETTSFEVENTSTATE                        82 // NtUserCallOneParam
#define SFI_SETSHELLCHANGENOTIFYHWND                83 // NtUserCallOneParam
#define SFI_DEREGISTERSHELLHOOKWINDOW               84 // NtUserCallHwnd
#define SFI_DWP_GETENABLEDPOPUPOFFSET               85 // NtUserCallHwnd
#define SFI_GETMODERNAPPWINDOW                      86 // NtUserCallHwnd
#define SFI_GETWINDOWCONTEXTHELPID                  87 // NtUserCallHwnd
#define SFI_REGISTERSHELLHOOKWINDOW                 88 // NtUserCallHwnd
#define SFI_SETMSGBOX                               89 // NtUserCallHwnd
#define SFI_INITTHREADCOREMESSAGINGIOCP             90 // NtUserCallHwnd, NtUserCallHwndSafe
#define SFI_SCHEDULEDISPATCHNOTIFICATION            91 // NtUserCallHwnd, NtUserCallHwndSafe
#define SFI_SETPROGMANWINDOW                        92 // NtUserCallHwndOpt
#define SFI_SETTASKMANWINDOW                        93 // NtUserCallHwndOpt
#define SFI_GETCLASSICOCUR                          94 // NtUserCallHwndParam
#define SFI_CLEARWINDOWSTATE                        95 // NtUserCallHwndParam
#define SFI_KILLSYSTEMTIMER                         96 // NtUserCallHwndParam
#define SFI_NOTIFYOVERLAYWINDOW                     97 // NtUserCallHwndParam
#define SFI_SETDIALOGPOINTER                        99 // NtUserCallHwndParam
#define SFI_SETVISIBLE                              100 // NtUserCallHwndParam
#define SFI_SETWINDOWCONTEXTHELPID                  101 // NtUserCallHwndParam(DWORD ContextHelpId)
#define SFI_SETWINDOWSTATE                          102 // NtUserCallHwndParam
#define SFI_REGISTERWINDOWARRANGEMENTCALLOUT        103 // NtUserCallHwndParam
#define SFI_ENABLEMODERNAPPWINDOWKEYBOARDINTERCEPT  104 // NtUserCallHwndParam
#define SFI_ARRANGEICONICWINDOWS                    105 // NtUserCallHwndLock
#define SFI_DRAWMENUBAR                             106 // NtUserCallHwndLock
#define SFI_CHECKIMESHOWSTATUSINTHREAD              107 // NtUserCallHwndLock, NtUserCallHwndLockSafe
#define SFI_GETSYSMENUOFFSET                        108 // NtUserCallHwndLock
#define SFI_REDRAWFRAME                             109 // NtUserCallHwndLock
#define SFI_REDRAWFRAMEANDHOOK                      110 // NtUserCallHwndLock
#define SFI_SETDIALOGSYSTEMMENU                     111 // NtUserCallHwndLock
#define SFI_SETFOREGROUNDWINDOW                     112 // NtUserCallHwndLock
#define SFI_SETSYSMENU                              113 // NtUserCallHwndLock
#define SFI_UPDATECLIENTRECT                        114 // NtUserCallHwndLock
#define SFI_UPDATEWINDOW                            115 // NtUserCallHwndLock
#define SFI_SETCANCELROTATIONDELAYHINTWINDOW        116 // NtUserCallHwndLock
#define SFI_GETWINDOWTRACKINFOASYNC                 117 // NtUserCallHwndLock
#define SFI_BROADCASTIMESHOWSTATUSCHANGE            118 // NtUserCallHwndParamLock
#define SFI_SETMODERNAPPWINDOW                      119 // NtUserCallHwndParamLock
#define SFI_REDRAWTITLE                             120 // NtUserCallHwndParamLock
#define SFI_SHOWOWNEDPOPUPS                         121 // NtUserCallHwndParamLock(BOOL Show)
#define SFI_SWITCHTOTHISWINDOW                      122 // NtUserCallHwndParamLock(BOOL AltTab)
#define SFI_UPDATEWINDOWS                           123 // NtUserCallHwndParamLock
#define SFI_VALIDATERGN                             124 // NtUserCallHwndParamLock
#define SFI_ENABLEWINDOW                            125 // NtUserCallHwndParamLock(BOOL Enable), NtUserCallHwndParamLockSafe
#define SFI_CHANGEWINDOWMESSAGEFILTER               126 // NtUserCallTwoParam(UINT Message, DWORD Flag)
#define SFI_GETCURSORPOS                            127 // NtUserCallTwoParam(LPPOINT Point, BOOL Mode)
#define SFI_INITANSIOEM                             128 // NtUserCallTwoParam
#define SFI_NLSKBDSENDIMENOTIFICATION               129 // NtUserCallTwoParam
#define SFI_REGISTERGHOSTWINDOW                     130 // NtUserCallTwoParam
#define SFI_REGISTERLOGONPROCESS                    131 // NtUserCallTwoParam
#define SFI_REGISTERSIBLINGFROSTWINDOW              132 // NtUserCallTwoParam
#define SFI_REGISTERUSERHUNGAPPHANDLERS             133 // NtUserCallTwoParam
#define SFI_REMOTESHADOWCLEANUP                     134 // NtUserCallTwoParam
#define SFI_REMOTESHADOWSTART                       135 // NtUserCallTwoParam
#define SFI_SETCARETPOS                             136 // NtUserCallTwoParam(int X, int Y)
#define SFI_SETTHREADQUEUEMERGESETTING              137 // NtUserCallTwoParam
#define SFI_UNHOOKWINDOWSHOOK                       138 // NtUserCallTwoParam
#define SFI_ENABLESHELLWINDOWMANAGEMENTBEHAVIOR     139 // NtUserCallTwoParam
#define SFI_CITSETINFO                              140 // NtUserCallTwoParam
#define SFI_SCALESYSTEMMETRICFORDPIWITHOUTCACHE     141 // NtUserCallTwoParam

typedef _Function_class_(FN_DISPATCH)
NTSTATUS FASTCALL FN_DISPATCH(
    _Inout_ PVOID Arguments
    );

typedef FN_DISPATCH* PFN_DISPATCH;

typedef struct _CAPTUREBUF
{
    PVOID pbData;
    ULONG cCapturedPointers;
    UCHAR pbReserved0C[12];
    ULONG ibPointerOffsets;
    ULONG aPointerOffsets[1];
} CAPTUREBUF, *PCAPTUREBUF;

typedef struct _CAPTUREBUF2 // sizeof=0x20
{
    ULONG Count;
    ULONG Offset;
} CAPTUREBUF2;

typedef NTSTATUS FASTCALL KCALLBACKPROC(
    _Inout_ PVOID Arguments
    );

typedef NTSTATUS FASTCALL KCALLBACKPROC_CAPTUREBUF(
    _Inout_ PCAPTUREBUF Arguments
    );

typedef NTSTATUS FASTCALL KCALLBACKPROC_ULONG(
    _Inout_ PULONG Arguments
    );

typedef NTSTATUS FASTCALL KCALLBACKPROC_POINTER(
    _Inout_ PVOID Arguments
    );

typedef NTSTATUS FASTCALL KCALLBACKPROC_NOARG(
    VOID
    );

typedef NTSTATUS FASTCALL KCALLBACKPROC_DUMMY(
    VOID
    );

/**
 * The FixupCallbackPointers routine fixes up relative pointers in a callback capture buffer to absolute addresses.
 *
 * \param CaptureBuffer Pointer to the capture buffer containing callback parameters.
 */
FORCEINLINE
VOID
FixupCallbackPointers(
    _Inout_ CAPTUREBUF2* CaptureBuffer
    )
{
    PULONG relativeOffsets;

    relativeOffsets = (PULONG)RTL_PTR_ADD(CaptureBuffer, CaptureBuffer->Offset);

    for (ULONG index = 0; index < CaptureBuffer->Count; index++)
    {
        PULONG pointerToFixup;

        pointerToFixup = (PULONG)RTL_PTR_ADD(CaptureBuffer, relativeOffsets[index]);
        *pointerToFixup += (ULONG)(ULONG_PTR)CaptureBuffer;
    }
}

// Peb!KernelCallbackTable = user32.dll!apfnDispatch
typedef struct _KERNEL_CALLBACK_TABLE
{
    PFN_DISPATCH __fnCOPYDATA;
    PFN_DISPATCH __fnCOPYGLOBALDATA;
    PFN_DISPATCH __fnEMPTY1;
    PFN_DISPATCH __fnNCDESTROY;
    PFN_DISPATCH __fnDWORDOPTINLPMSG;
    PFN_DISPATCH __fnINOUTDRAG;
    PFN_DISPATCH __fnGETTEXTLENGTHS1;
    KCALLBACKPROC_CAPTUREBUF* __fnINCNTOUTSTRING;
    KCALLBACKPROC_CAPTUREBUF* __fnINCNTOUTSTRINGNULL;
    PFN_DISPATCH __fnINLPCOMPAREITEMSTRUCT;
    KCALLBACKPROC_CAPTUREBUF* __fnINLPCREATESTRUCT;
    PFN_DISPATCH __fnINLPDELETEITEMSTRUCT;
    PFN_DISPATCH __fnINLPDRAWITEMSTRUCT;
    PFN_DISPATCH __fnPOPTINLPUINT1;
    PFN_DISPATCH __fnPOPTINLPUINT2;
    PFN_DISPATCH __fnINLPMDICREATESTRUCT;
    PFN_DISPATCH __fnINOUTLPMEASUREITEMSTRUCT;
    PFN_DISPATCH __fnINLPWINDOWPOS;
    PFN_DISPATCH __fnINOUTLPPOINT51;
    PFN_DISPATCH __fnINOUTLPSCROLLINFO;
    PFN_DISPATCH __fnINOUTLPRECT;
    PFN_DISPATCH __fnINOUTNCCALCSIZE;
    PFN_DISPATCH __fnINOUTLPPOINT52;
    PFN_DISPATCH __fnINPAINTCLIPBRD;
    PFN_DISPATCH __fnINSIZECLIPBRD;
    PFN_DISPATCH __fnINDESTROYCLIPBRD;
    KCALLBACKPROC_CAPTUREBUF* __fnINSTRINGNULL1;
    KCALLBACKPROC_CAPTUREBUF* __fnINSTRINGNULL2;
    PFN_DISPATCH __fnINDEVICECHANGE;
    PFN_DISPATCH __fnPOWERBROADCAST;
    PFN_DISPATCH __fnINLPUAHDRAWMENU1;
    PFN_DISPATCH __fnOPTOUTLPDWORDOPTOUTLPDWORD1;
    PFN_DISPATCH __fnOPTOUTLPDWORDOPTOUTLPDWORD2;
    PFN_DISPATCH __fnOUTDWORDINDWORD;
    PFN_DISPATCH __fnOUTLPRECT;
    KCALLBACKPROC_CAPTUREBUF* __fnOUTSTRING;
    PFN_DISPATCH __fnPOPTINLPUINT3;
    PFN_DISPATCH __fnPOUTLPINT;
    PFN_DISPATCH __fnSENTDDEMSG;
    PFN_DISPATCH __fnINOUTSTYLECHANGE1;
    PFN_DISPATCH __fnHkINDWORD;
    KCALLBACKPROC_ULONG* __fnHkINLPCBTACTIVATESTRUCT;
    PFN_DISPATCH __fnHkINLPCBTCREATESTRUCT;
    KCALLBACKPROC_ULONG* __fnHkINLPDEBUGHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMOUSEHOOKSTRUCTEX1;
    PFN_DISPATCH __fnHkINLPKBDLLHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMSLLHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMSG;
    PFN_DISPATCH __fnHkINLPRECT;
    KCALLBACKPROC_ULONG* __fnHkOPTINLPEVENTMSG;
    PFN_DISPATCH __xxxClientCallDelegateThread;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback1;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback2;
    PFN_DISPATCH __fnSHELLWINDOWMANAGEMENTCALLOUT;
    PFN_DISPATCH __fnSHELLWINDOWMANAGEMENTNOTIFY;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback3;
    PFN_DISPATCH __xxxClientCallDitThread;
    PFN_DISPATCH __xxxClientEnableMMCSS;
    PFN_DISPATCH __xxxClientUpdateDpi;
    PFN_DISPATCH __xxxClientExpandStringW;
    PFN_DISPATCH __ClientCopyDDEIn1;
    PFN_DISPATCH __ClientCopyDDEIn2;
    PFN_DISPATCH __ClientCopyDDEOut1;
    PFN_DISPATCH __ClientCopyDDEOut2;
    PFN_DISPATCH __ClientCopyImage;
    PFN_DISPATCH __ClientEventCallback;
    PFN_DISPATCH __ClientFindMnemChar;
    PFN_DISPATCH __ClientFreeDDEHandle;
    PFN_DISPATCH __ClientFreeLibrary;
    KCALLBACKPROC_ULONG* __ClientGetCharsetInfo;
    PFN_DISPATCH __ClientGetDDEFlags;
    PFN_DISPATCH __ClientGetDDEHookData;
    KCALLBACKPROC_CAPTUREBUF* __ClientGetListboxString;
    PFN_DISPATCH __ClientGetMessageMPH;
    PFN_DISPATCH __ClientLoadImage;
    KCALLBACKPROC_CAPTUREBUF* __ClientLoadLibrary;
    KCALLBACKPROC_CAPTUREBUF* __ClientLoadMenu;
    KCALLBACKPROC_NOARG* __ClientLoadLocalT1Fonts;
    PFN_DISPATCH __ClientPSMTextOut;
    PFN_DISPATCH __ClientLpkDrawTextEx;
    PFN_DISPATCH __ClientExtTextOutW;
    PFN_DISPATCH __ClientGetTextExtentPointW;
    PFN_DISPATCH __ClientCharToWchar;
    PFN_DISPATCH __ClientAddFontResourceW;
    KCALLBACKPROC_NOARG* __ClientThreadSetup;
    KCALLBACKPROC_NOARG* __ClientDeliverUserApc;
    KCALLBACKPROC_NOARG* __ClientNoMemoryPopup;
    PFN_DISPATCH __ClientMonitorEnumProc;
    PFN_DISPATCH __ClientCallWinEventProc;
    KCALLBACKPROC_ULONG* __ClientWaitMessageExMPH;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback4;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback5;
    PFN_DISPATCH __ClientImmLoadLayout;
    PFN_DISPATCH __ClientImmProcessKey;
    KCALLBACKPROC_CAPTUREBUF* __fnIMECONTROL;
    PFN_DISPATCH __fnINWPARAMDBCSCHAR;
    PFN_DISPATCH __fnGETTEXTLENGTHS2;
    KCALLBACKPROC_DUMMY* __ClientCallDummyCallback6;
    PFN_DISPATCH __ClientLoadStringW;
    KCALLBACKPROC_NOARG* __ClientLoadOLE;
    PFN_DISPATCH __ClientRegisterDragDrop;
    PFN_DISPATCH __ClientRevokeDragDrop;
    PFN_DISPATCH __fnINOUTMENUGETOBJECT;
    PFN_DISPATCH __ClientPrinterThunk;
    PFN_DISPATCH __fnOUTLPCOMBOBOXINFO;
    PFN_DISPATCH __fnOUTLPSCROLLBARINFO;
    PFN_DISPATCH __fnINLPUAHDRAWMENU2;
    PFN_DISPATCH __fnINLPUAHDRAWMENUITEM;
    PFN_DISPATCH __fnINLPUAHDRAWMENU3;
    PFN_DISPATCH __fnINOUTLPUAHMEASUREMENUITEM;
    PFN_DISPATCH __fnINLPUAHDRAWMENU4;
    PFN_DISPATCH __fnOUTLPTITLEBARINFOEX;
    PFN_DISPATCH __fnTOUCH;
    PFN_DISPATCH __fnGESTURE;
    PFN_DISPATCH __fnPOPTINLPUINT4;
    PFN_DISPATCH __fnPOPTINLPUINT5;
    PFN_DISPATCH __xxxClientCallDefaultInputHandler;
    PFN_DISPATCH __fnEMPTY2;
    PFN_DISPATCH __ClientRimDevCallback;
    PFN_DISPATCH __xxxClientCallMinTouchHitTestingCallback;
    KCALLBACKPROC_NOARG* __ClientCallLocalMouseHooks;
    PFN_DISPATCH __xxxClientBroadcastThemeChange;
    PFN_DISPATCH __xxxClientCallDevCallbackSimple;
    KCALLBACKPROC_ULONG* __xxxClientAllocWindowClassExtraBytes;
    PFN_DISPATCH __xxxClientFreeWindowClassExtraBytes;
    PFN_DISPATCH __fnGETWINDOWDATA;
    PFN_DISPATCH __fnINOUTSTYLECHANGE2;
    PFN_DISPATCH __fnHkINLPMOUSEHOOKSTRUCTEX2;
    PFN_DISPATCH __xxxClientCallDefWindowProc;
    PFN_DISPATCH __fnSHELLSYNCDISPLAYCHANGED;
    PFN_DISPATCH __fnHkINLPCHARHOOKSTRUCT;
    PFN_DISPATCH __fnINTERCEPTEDWINDOWACTION;
    KCALLBACKPROC_ULONG* __xxxTooltipCallback;
    PFN_DISPATCH __xxxClientInitPSBInfo;
    PFN_DISPATCH __xxxClientDoScrollMenu;
    PFN_DISPATCH __xxxClientEndScroll;
    PFN_DISPATCH __xxxClientDrawSize;
    PFN_DISPATCH __xxxClientDrawScrollBar;
    PFN_DISPATCH __xxxClientHitTestScrollBar;
    PFN_DISPATCH __xxxClientTrackInit;
} KERNEL_CALLBACK_TABLE, *PKERNEL_CALLBACK_TABLE;

//
// gapfnScSendMessage: USER32 exported data and historical kernel-table notes.
//
// The declaration below imports the USER32 data array, not a function and not
// win32kfull.sys kernel storage. The inspected user32.dll 10.0.26100.8117 exports
// it by name at ordinal 2583, .rdata RVA 0xB4230. Addresses and ordinals are
// build-specific; the older USER32 slot map below records RVA 0xAB440.
//
// The same symbol name also identifies a kernel-side Sfn* message-marshalling
// table. That table is distinct from USER32's array and from the user-mode
// callback dispatch table (PEB.KernelCallbackTable / user32!apfnDispatch).
// A shared name or slot index does not establish an identical callback ABI.
//
// Historical kernel evidence retained from the original reconstruction:
// win32kfull.sys (10.0), PDB 669FC53C-9730-AECB-4422-DB5F4BCE964E,
// .rdata RVA 0x34AFE0: 73 slots [0..72], followed by a NULL terminator and
// gapfnMessageCall. This layout has not been revalidated for every build.
//
// SFNSCSENDMESSAGE retains the historical kernel-side reconstruction for
// compatibility. Its first four arguments were described from SfnDWORD
// (RVA 0xD88E0), SfnCOPYDATA (RVA 0xFD8B0), and SfnINLPCREATESTRUCT
// (RVA 0x248930); the trailing arguments were reconstructed. These notes do
// not independently verify the signatures of USER32's exported array entries.
//
typedef LRESULT (NTAPI* SFNSCSENDMESSAGE)(
    _In_ PVOID Window,              // RCX - tagWND* (kernel window object)
    _In_ ULONG Message,             // EDX
    _In_ WPARAM wParam,            // R8
    _In_ LPARAM lParam,            // R9
    _In_ ULONG_PTR ExtraInfo,      // stack - xParam // rev
    _In_opt_ PVOID WindowProc,     // stack - xpfnProc // rev
    _In_ ULONG SendMessageFlags    // stack - dwSCMSFlags (bit 0 == ANSI) // rev
    );

#define FNSCSENDMESSAGE_COUNT 73

// Historical kernel slot map from the reconstruction above; build-specific:
//   [ 0] SfnDWORD
//   [ 1] SfnNCDESTROY
//   [ 2] SfnINLPCREATESTRUCT
//   [ 3] SfnINSTRINGNULL
//   [ 4] SfnOUTSTRING
//   [ 5] SfnINSTRING
//   [ 6] SfnINOUTLPPOINT5
//   [ 7] SfnINLPDRAWITEMSTRUCT
//   [ 8] SfnINOUTLPMEASUREITEMSTRUCT
//   [ 9] SfnINLPDELETEITEMSTRUCT
//   [10] SfnINWPARAMCHAR
//   [11] SfnINLPHLPSTRUCT
//   [12] SfnINLPCOMPAREITEMSTRUCT
//   [13] SfnINOUTLPWINDOWPOS
//   [14] SfnINLPWINDOWPOS
//   [15] SfnCOPYGLOBALDATA
//   [16] SfnCOPYDATA
//   [17] SfnINLPHELPINFOSTRUCT
//   [18] SfnGETWINDOWDATA
//   [19] SfnINOUTSTYLECHANGE
//   [20] SfnINOUTNCCALCSIZE
//   [21] SfnDWORDOPTINLPMSG
//   [22] SfnINOUTLPSIZE
//   [23] SfnINOUTLPRECT
//   [24] SfnOPTOUTLPDWORDOPTOUTLPDWORD
//   [25] SfnOUTLPRECT
//   [26] SfnINCNTOUTSTRING
//   [27] SfnPOPTINLPUINT
//   [28] SfnINOUTLPSCROLLINFO
//   [29] SfnINCBOXSTRING
//   [30] SfnOUTCBOXSTRING
//   [31] SfnINLBOXSTRING
//   [32] SfnOUTLBOXSTRING
//   [33] SfnPOUTLPINT
//   [34] SfnOUTDWORDINDWORD
//   [35] SfnINOUTNEXTMENU
//   [36] SfnINDEVICECHANGE
//   [37] SfnINLPMDICREATESTRUCT
//   [38] SfnINOUTDRAG
//   [39] SfnINDESTROYCLIPBRD
//   [40] SfnINPAINTCLIPBRD
//   [41] SfnINSIZECLIPBRD
//   [42] SfnINCNTOUTSTRINGNULL
//   [43] SfnDWORD
//   [44] SfnDWORD
//   [45] SfnSENTDDEMSG
//   [46] SfnGETDBCSTEXTLENGTHS
//   [47] SfnOPTOUTLPDWORDOPTOUTLPDWORD
//   [48] SfnDWORD
//   [49] SfnINWPARAMDBCSCHAR
//   [50] SfnOPTOUTLPDWORDOPTOUTLPDWORD
//   [51] SfnIMECONTROL
//   [52] SfnINOUTMENUGETOBJECT
//   [53] SfnPOWERBROADCAST
//   [54] SfnOUTLPCOMBOBOXINFO
//   [55] SfnOUTLPSCROLLBARINFO
//   [56] SfnINLPUAHDRAWMENU
//   [57] SfnINLPUAHDRAWMENUITEM
//   [58] SfnINLPUAHINITMENU
//   [59] SfnINOUTLPUAHMEASUREMENUITEM
//   [60] SfnINLPUAHNCPAINTMENUPOPUP
//   [61] SfnOUTLPTITLEBARINFOEX
//   [62] SfnTOUCH
//   [63] SfnGESTURE
//   [64] SfnINPGESTURENOTIFYSTRUCT
//   [65] SfnDWORD
//   [66] SfnDWORD
//   [67] SfnTOUCHHITTESTING
//   [68] SfnSHELLWINDOWMANAGEMENTCALLOUT
//   [69] SfnSHELLWINDOWMANAGEMENTNOTIFY
//   [70] SfnSHELLSYNCDISPLAYCHANGED
//   [71] SfnINTERCEPTEDWINDOWACTION
//   [72] SfnEMPTY
//
//
// Historical USER32 slot map (original recorded .rdata RVA 0xAB440).
// The original reconstruction lists 73 entries [0..72], indexed against the
// kernel map above, with NtUserMessageCall_0 as the common pass-through thunk.
// Retained as reference data, not a fresh verification of the inspected
// user32.dll 10.0.26100.8117 array at RVA 0xB4230, its size, or callback types.
//
// Historical USER32 slot -> client thunk:
//   [ 0] NtUserMessageCall_0
//   [ 1] NtUserMessageCall_0
//   [ 2] NtUserMessageCall_0
//   [ 3] NtUserMessageCall_0
//   [ 4] NtUserMessageCall_0
//   [ 5] NtUserMessageCall_0
//   [ 6] NtUserMessageCall_0
//   [ 7] NtUserMessageCall_0
//   [ 8] NtUserMessageCall_0
//   [ 9] NtUserMessageCall_0
//   [10] NtUserMessageCall_0
//   [11] NtUserMessageCall_0
//   [12] NtUserMessageCall_0
//   [13] NtUserMessageCall_0
//   [14] NtUserMessageCall_0
//   [15] fnCOPYGLOBALDATA
//   [16] NtUserMessageCall_0
//   [17] NtUserMessageCall_0
//   [18] NtUserMessageCall_0
//   [19] NtUserMessageCall_0
//   [20] NtUserMessageCall_0
//   [21] NtUserMessageCall_0
//   [22] NtUserMessageCall_0
//   [23] NtUserMessageCall_0
//   [24] NtUserMessageCall_0
//   [25] NtUserMessageCall_0
//   [26] NtUserMessageCall_0
//   [27] NtUserMessageCall_0
//   [28] NtUserMessageCall_0
//   [29] NtUserMessageCall_0
//   [30] NtUserMessageCall_0
//   [31] NtUserMessageCall_0
//   [32] NtUserMessageCall_0
//   [33] NtUserMessageCall_0
//   [34] NtUserMessageCall_0
//   [35] NtUserMessageCall_0
//   [36] fnINDEVICECHANGE
//   [37] NtUserMessageCall_0
//   [38] NtUserMessageCall_0
//   [39] NtUserMessageCall_0
//   [40] fnINPAINTCLIPBRD
//   [41] fnINPAINTCLIPBRD
//   [42] NtUserMessageCall_0
//   [43] NtUserMessageCall_0
//   [44] NtUserMessageCall_0
//   [45] NtUserMessageCall_0
//   [46] NtUserMessageCall_0
//   [47] fnEMGETSEL
//   [48] fnEMSETSEL
//   [49] fnINWPARAMDBCSCHAR
//   [50] fnCBGETEDITSEL
//   [51] fnIMECONTROL
//   [52] NtUserMessageCall_0
//   [53] fnPOWERBROADCAST
//   [54] NtUserMessageCall_0
//   [55] NtUserMessageCall_0
//   [56] NtUserMessageCall_0
//   [57] NtUserMessageCall_0
//   [58] NtUserMessageCall_0
//   [59] NtUserMessageCall_0
//   [60] NtUserMessageCall_0
//   [61] NtUserMessageCall_0
//   [62] fnTOUCH
//   [63] fnGESTURE
//   [64] NtUserMessageCall_0
//   [65] NtUserMessageCall_0
//   [66] NtUserMessageCall_0
//   [67] NtUserMessageCall_0
//   [68] NtUserMessageCall_0
//   [69] NtUserMessageCall_0
//   [70] NtUserMessageCall_0
//   [71] NtUserMessageCall_0
//   [72] NtUserMessageCall_0
//
// USER32 imported DATA array. NTSYSAPI supplies DLL-import decoration in
// consumer builds; it does not select a DLL or provide an import library.
// Direct references compile to __imp_gapfnScSendMessage and require a matching
// USER32 data import in the linked import library. The inspected Windows SDK
// 10.0.28000.0 x64 user32.lib has no matching symbol; linking was not validated.
//
// The existing SFNSCSENDMESSAGE type and FNSCSENDMESSAGE_COUNT (73) are retained
// unchanged. Neither the historical kernel signature nor the old slot map
// independently establishes the current USER32 callback contract. Verify the
// target build and entry signature before invoking an array element.
NTSYSAPI extern SFNSCSENDMESSAGE gapfnScSendMessage[FNSCSENDMESSAGE_COUNT];

//
// Window Stations (HWINSTA)
//

/**
 * Fixed character lengths for the WINSTATIONINFO protocol and audio-driver name buffers.
 */
#define WPROTOCOLNAME_LENGTH    10
#define WAUDIONAME_LENGTH       10

/**
 * WinStation protocol and audio-driver information returned by NtUserGetWinStationInfo.
 */
typedef struct _WINSTATIONINFO
{
    WCHAR ProtocolName[WPROTOCOLNAME_LENGTH];
    WCHAR AudioDriverName[WAUDIONAME_LENGTH];
} WINSTATIONINFO, *PWINSTATIONINFO;

/**
 * Compatibility type aliases for WINSTATIONINFO (WSINFO / PWSINFO).
 */
typedef WINSTATIONINFO WSINFO;
typedef PWINSTATIONINFO PWSINFO;

/**
 * NtUserCreateWindowStation
 *
 * win32kbase 10.0.26100.9444: offsets are 32-bit values, not pointers.
 * Argument 4 is preserved through R9D at 0x1401b4649; argument 5 is copied
 * through EAX at 0x14017c416. Other parameters retain their existing declarations.
 */
_Kernel_entry_
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserCreateWindowStation(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE KeyboardLayoutHandle,
    _In_ ULONG KeyboardLayoutOffset,
    _In_ ULONG NlsTableOffset,
    _In_opt_ PVOID KeyboardDescriptor,
    _In_opt_ PCUNICODE_STRING LanguageIdString,
    _In_opt_ ULONG KeyboardLocale
    );

// rev
/**
 * Window station creation flags for NtUserCreateWindowStationEx and CreateWindowStation.
 */
typedef enum _CREATE_WINDOW_STATION_FLAGS
{
    PH_CWF_NONE = 0x00000000,
    PH_CWF_CREATE_ONLY = 0x00000001,  ///< Case-sensitive name creation (CWS_FIRST); clears OBJ_CASE_INSENSITIVE.
    PH_CWF_DISABLE_DWM = 0x00000002,  ///< Disables DWM critical section tracking and enables multi-keyboard layouts across sessions. Mutually exclusive with CWF_AGENT.
    PH_CWF_AGENT = 0x00000004,        ///< Creates an agent window station (sets WSF_AGENT 0x0800 and skips default policy). Requires Feature_AgentSessionsBaseSupport.
    PH_CWF_SESSION_ID = 0x00000008,   ///< Specifies that the upper 16 bits (HIWORD) contain the target session ID. Requires Feature_AgentSessionsBaseSupport.
    PH_CWF_VALID_MASK = 0x0000000F,
    PH_CWF_SESSION_ID_MASK = 0xFFFF0000
} CREATE_WINDOW_STATION_FLAGS;

#define CWS_FIRST PH_CWF_CREATE_ONLY
#define CWF_SESSION_ID_SHIFT 16
#define CWF_SET_SESSION_ID(SessionId) (((ULONG)(SessionId) & 0xFFFF) << CWF_SESSION_ID_SHIFT)
#define CWF_GET_SESSION_ID(Flags) ((ULONG)(Flags) >> CWF_SESSION_ID_SHIFT)

// rev
/**
 * Internal window station attribute flags (tagWINDOWSTATIONFLAGS).
 */
typedef enum _tagWINDOWSTATIONFLAGS
{
    PH_WSF_NONE = 0x0000,
    PH_WSF_VISIBLE = 0x0001,
    PH_WSF_SERVICE = 0x0004,
    PH_WSF_AGENT = 0x0800
} tagWINDOWSTATIONFLAGS;

// rev
/**
 * The NtUserCreateWindowStationEx routine creates a new window station object with extended attributes.
 *
 * \param ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure specifying the window station object attributes.
 * \param DesiredAccess The type of access to the window station.
 * \param KeyboardLayoutHandle Optional handle to the keyboard layout.
 * \param KeyboardLayoutOffset Optional offset to the keyboard layout data.
 * \param NlsTableOffset Optional offset to the NLS table data.
 * \param KeyboardDescriptor Optional pointer to the keyboard descriptor.
 * \param LanguageIdString Optional pointer to a UNICODE_STRING containing the language ID.
 * \param KeyboardLocale Optional keyboard locale identifier.
 * \param Flags Window station creation flags (combination of CREATE_WINDOW_STATION_FLAGS).
 * \return A handle to the created window station, or NULL on failure. Argument mapping confirmed from user32!CommonCreateWindowStation and win32kfull!EditionCreateWindowStationEntryPointEx.
 */
_Kernel_entry_
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserCreateWindowStationEx(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE KeyboardLayoutHandle,
    _In_opt_ ULONG KeyboardLayoutOffset,
    _In_opt_ ULONG NlsTableOffset,
    _In_opt_ PVOID KeyboardDescriptor,
    _In_opt_ PCUNICODE_STRING LanguageIdString,
    _In_opt_ ULONG KeyboardLocale,
    _In_ ULONG Flags
    );

/**
 * The NtUserOpenWindowStation routine opens the specified window station.
 *
 * \param ObjectAttributes The name of the window station to be opened. Window station names are case-insensitive. This window station must belong to the current session.
 * \param DesiredAccess The access mask requested for the window station.
 * \return A handle to the opened window station, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserOpenWindowStation(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The GetWinStationInfo routine retrieves information about the current window station.
 *
 * \param WsInfo Pointer receiving the window station information structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserGetWinStationInfo.
 */
NTSYSAPI
LOGICAL
NTAPI
GetWinStationInfo(
    _Out_ PWSINFO WsInfo
    );

/**
 * The NtUserBuildNameList routine enumerates names of desktops within a window station.
 *
 * \param WindowStationHandle Handle to the window station to query.
 * \param NameListInformationLength Size of the output buffer in bytes.
 * \param NameListInformation Buffer receiving the array of desktop names.
 * \param ReturnLength Optional pointer receiving the number of bytes written or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildNameList(
    _In_ HWINSTA WindowStationHandle, // GetProcessWindowStation
    _In_ ULONG NameListInformationLength,
    _Out_writes_bytes_(NameListInformationLength) PVOID NameListInformation,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtUserBuildPropList routine retrieves a list of property names set on a window.
 *
 * \param WindowStationHandle Handle to the window whose property list is retrieved.
 * \param PropListInformationLength Size of the output buffer in bytes.
 * \param PropListInformation Buffer receiving property list data.
 * \param ReturnLength Optional pointer receiving the number of bytes written or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildPropList(
    _In_ HWINSTA WindowStationHandle,
    _In_ ULONG PropListInformationLength,
    _Out_writes_bytes_(PropListInformationLength) PVOID PropListInformation,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtUserGetProcessWindowStation routine retrieves the window station handle associated with the current process.
 *
 * \return A handle to the window station, or NULL if the operation fails.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getprocesswindowstation
 */
_Kernel_entry_
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserGetProcessWindowStation(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetWinStationInfo routine retrieves WinStation protocol and audio information.
 *
 * \param WsInfo Pointer to a WINSTATIONINFO structure that receives the protocol and audio information.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_GETWINSTATIONINFO) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetWinStationInfo(
    _Out_ PWSINFO WsInfo
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The LockWindowStation routine locks a window station.
 *
 * \param WindowStationHandle Handle to the window station to lock.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserLockWindowStation system call.
 */
NTSYSAPI
LOGICAL
NTAPI
LockWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

// rev
/**
 * The NtUserLockWindowStation routine locks the specified window station against unauthorized desktop access.
 *
 * \param WindowStationHandle Handle to the window station to lock.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLockWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

/**
 * The NtUserSetProcessWindowStation routine assigns the specified window station to the calling process.
 *
 * \param WindowStationHandle A handle to the window station to be assigned.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetProcessWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

/**
 * The NtUserSetWindowStationUser routine associates a user security identifier (SID) and logon ID with a window station.
 *
 * \param WindowStationHandle A handle to the window station.
 * \param Luid Pointer to the logon session identifier (LUID).
 * \param Sid Pointer to the user's security identifier (SID).
 * \param SidLength The length of the SID in bytes.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowStationUser(
    _In_ HWINSTA WindowStationHandle,
    _In_ PLUID Luid,
    _In_reads_bytes_(SidLength) PSID Sid,
    _In_ ULONG SidLength
    );

// rev
/**
 * The NtUserUnlockWindowStation routine unlocks the specified window station.
 *
 * \param WindowStationHandle A handle to the window station to unlock.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnlockWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

/**
 * The SetWindowStationUser routine associates a user security identifier (SID) and logon ID with a window station.
 *
 * \param WindowStationHandle A handle to the window station.
 * \param UserLogonId Pointer to the logon session identifier (LUID).
 * \param UserSid Pointer to the user's security identifier (SID).
 * \param UserSidLength The length of the SID in bytes.
 * \return TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
SetWindowStationUser(
    _In_ HWINSTA WindowStationHandle,
    _In_ PLUID UserLogonId,
    _In_ PSID UserSid,
    _In_ ULONG UserSidLength
    );

// rev
/**
 * The UnlockWindowStation routine unlocks a window station.
 *
 * \param WindowStationHandle Handle to the window station to unlock.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserUnlockWindowStation system call.
 */
NTSYSAPI
LOGICAL
NTAPI
UnlockWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

/**
 * The NtUserCloseWindowStation routine closes an open window station handle.
 *
 * \param WindowStationHandle Handle to the window station to close.
 * \return TRUE on success; FALSE on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserCloseWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

//
// Desktops (HDESK)
//

// rev
/**
 * The NtUserCreateDesktopEx routine creates a new desktop on a window station with extended options.
 *
 * \param ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure describing the desktop object.
 * The RootDirectory member specifies the target window station, the ObjectName member specifies
 * the desktop name, and Attributes/SecurityDescriptor carry inherit and security settings.
 * \param DeviceName Optional pointer to a UNICODE_STRING specifying the display device name.
 * \param DeviceModes Optional pointer to a DEVMODE structure specifying the desktop display mode.
 * \param Flags Desktop creation flags (DF_*).
 * \param DesiredAccess Desired access mask for the desktop.
 * \param HeapSize Size of the desktop heap, in kilobytes.
 * \return A handle to the newly created desktop, or NULL on failure. Argument mapping confirmed from user32!CommonCreateDesktop.
 */
_Kernel_entry_
NTSYSCALLAPI
HDESK
NTAPI
NtUserCreateDesktopEx(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PUNICODE_STRING DeviceName,
    _In_opt_ LPDEVMODEW DeviceModes,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HeapSize
    );

/**
 * The NtUserOpenDesktop routine opens the specified desktop object.
 *
 * \param ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure specifying the desktop object attributes.
 * \param Flags Desktop open flags.
 * \param DesiredAccess The access mask for the desktop object.
 * \return A handle to the desktop object, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserOpenDesktop(
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * The NtUserOpenInputDesktop routine opens the desktop that receives user input.
 *
 * \param Flags Desktop access flags.
 * \param Inherit If TRUE, processes created by this process will inherit the handle.
 * \param DesiredAccess The access mask for the desktop.
 * \return A handle to the input desktop, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDESK
NTAPI
NtUserOpenInputDesktop(
    _In_ ULONG Flags,
    _In_ BOOL Inherit,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The NtUserOpenThreadDesktop routine opens the desktop associated with the specified thread.
 *
 * \param ThreadId Identifier of the thread whose desktop is opened.
 * \param Flags Desktop flags. Only DF_ALLOWOTHERACCOUNTHOOK (0x00000001) is used; higher bits are ignored.
 * \param Inherit TRUE to make the returned handle inheritable; FALSE otherwise.
 * \param DesiredAccess Requested desktop access rights. DESKTOP_READOBJECTS and DESKTOP_WRITEOBJECTS are always added.
 * \return HDESK A handle to the desktop on success, or NULL on failure.
 * \remarks Resolves the thread's desktop, with a console-desktop fallback when no GUI thread information is found.
 *          Opens a new handle with DesiredAccess | 0x81 and OBJ_INHERIT when Inherit is nonzero.
 *          The open enforces the desktop's security in user mode and rejects desktops outside the current session.
 *          DF_ALLOWOTHERACCOUNTHOOK permits processes running under other accounts on the desktop to set hooks
 *          in this process. Native failure statuses are translated to Win32 last-error codes.
 */
_Success_(return != NULL)
_Kernel_entry_
NTSYSCALLAPI
HDESK
NTAPI
NtUserOpenThreadDesktop(
    _In_ ULONG ThreadId,
    _In_ ULONG Flags,
    _In_ BOOL Inherit,
    _In_ ACCESS_MASK DesiredAccess
    );

// Send to the window registered with NtUserRegisterCloakedNotification
// when cloak state of the window has changed
// wParam - if window cloak state changed contains cloaking value
//          which can be one/all of the below
//          DWM_CLOAKED_APP(0x0000001).The window was cloaked by its owner application.
//          DWM_CLOAKED_SHELL(0x0000002).The window was cloaked by the Shell.
//          0 - window is not cloaked
//
// lParam - 0 (unused)
//
#define WM_CLOAKED_STATE_CHANGED 0x0347

/**
 * The NtUserRegisterCloakedNotification routine registers or unregisters a window to receive WM_CLOAKED_STATE_CHANGED notifications.
 *
 * \param WindowHandle A handle to the window.
 * \param Register TRUE to register; FALSE to unregister.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterCloakedNotification(
    _In_ HWND WindowHandle,
    _In_ BOOL Register
    );

// rev
/**
 * The OpenThreadDesktop routine opens the desktop associated with a thread.
 *
 * \param ThreadId Identifier of the thread whose desktop is opened.
 * \param Flags Desktop flags. Only DF_ALLOWOTHERACCOUNTHOOK (0x00000001) is used; higher bits are ignored.
 * \param Inherit TRUE to make the returned handle inheritable; FALSE otherwise.
 * \param DesiredAccess Requested desktop access rights. DESKTOP_READOBJECTS and DESKTOP_WRITEOBJECTS are always added.
 * \return A handle to the desktop on success, or NULL on failure.
 * \remarks Forwards to the NtUserOpenThreadDesktop system call.
 * Resolves the thread's desktop, with a console-desktop fallback when no GUI thread information is found.
 * Opens a new handle with DesiredAccess | 0x81 and OBJ_INHERIT when Inherit is nonzero.
 * The open enforces the desktop's security in user mode and rejects desktops outside the current session.
 * DF_ALLOWOTHERACCOUNTHOOK permits processes running under other accounts on the desktop to set hooks
 * in this process. Native failure statuses are translated to Win32 last-error codes.
 */
_Success_(return != NULL)
NTSYSAPI
HDESK
NTAPI
OpenThreadDesktop(
    _In_ ULONG ThreadId,
    _In_ ULONG Flags,
    _In_ BOOL Inherit,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The CheckWindowThreadDesktop routine validates the desktop of a window's owning thread.
 *
 * \param WindowHandle Handle to the target window.
 * \param ThreadId Identifier of the thread to check.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserCheckWindowThreadDesktop system call.
 */
NTSYSAPI
LOGICAL
NTAPI
CheckWindowThreadDesktop(
    _In_ HWND WindowHandle,
    _In_ ULONG ThreadId
    );

// rev
/**
 * The GetDesktopID routine retrieves the identifier of a virtual desktop.
 *
 * \param DesktopIndex Index of the target desktop.
 * \param DesktopId Pointer receiving the desktop identifier GUID or structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserGetDesktopID system call.
 */
NTSYSAPI
LOGICAL
NTAPI
GetDesktopID(
    _In_ LONG DesktopIndex,
    _Out_ PVOID DesktopId
    );

// rev
/**
 * The GetInputDesktop routine retrieves a handle to the current input desktop.
 *
 * \return HDESK Handle to the input desktop, or NULL on failure.
 * \remarks Thin user32 wrapper over NtUserGetInputDesktop.
 */
NTSYSAPI
HDESK
NTAPI
GetInputDesktop(
    VOID
    );

// rev
/**
 * The IsInDesktopWindowBand routine tests whether a window belongs to a desktop window band.
 *
 * \param WindowHandle Handle to the window to test.
 * \return TRUE if the window belongs to a desktop window band, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IsInDesktopWindowBand(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The IsThreadDesktopComposited routine determines whether the calling thread's desktop is composited.
 *
 * \return BOOL TRUE if the calling thread's desktop is composited, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IsThreadDesktopComposited(
    VOID
    );

/**
 * The IsPseudoWindowHandle routine tests whether a window handle is a pseudo-handle (such as HWND_BROADCAST or HWND_TOP).
 *
 * \param WindowHandle The window handle to test.
 * \return TRUE if the handle is a pseudo-handle; otherwise, FALSE.
 */
FORCEINLINE
BOOLEAN
NTAPI
IsPseudoWindowHandle(
    _In_ HWND WindowHandle
    )
{
    ULONG_PTR value = (ULONG_PTR)WindowHandle;

    if (value == (ULONG_PTR)HWND_BROADCAST ||
        value <= (ULONG_PTR)HWND_BOTTOM ||
        value >= (ULONG_PTR)HWND_MESSAGE)
    {
        return TRUE;
    }

    return FALSE;
}

// rev
/**
 * The NtUserCheckWindowThreadDesktop routine verifies that the specified window belongs to the desktop of the given thread.
 *
 * \param WindowHandle Handle to the window to check.
 * \param ThreadId Identifier of the thread whose desktop association is compared.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCheckWindowThreadDesktop(
    _In_ HWND WindowHandle,
    _In_ ULONG ThreadId
    );

// rev
/**
 * The NtUserGetDesktopID routine retrieves the unique desktop identifier for a specified desktop index.
 *
 * \param DesktopIndex Zero-based desktop index.
 * \param DesktopId Pointer to a variable receiving the desktop identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetDesktopID(
    _In_ LONG DesktopIndex,
    _Out_ PVOID DesktopId
    );

// rev
/**
 * The NtUserGetDesktopVisualTransform routine retrieves the global visual transform matrix for the desktop.
 *
 * \param VisualTransform Pointer to a structure receiving desktop transform matrix parameters.
 * \return NTSTATUS Successful or errant status.
 * \remarks win32u.dll ordinal 1004, syscall 0x1438.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserGetDesktopVisualTransform(
    _Out_ PVOID VisualTransform
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetInputDesktop routine returns a handle to the current input desktop.
 *
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETINPUTDESKTOP) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HDESK
NTAPI
NtUserGetInputDesktop(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetThreadDesktop routine retrieves a handle to the desktop assigned to the specified thread.
 *
 * \param ThreadId The thread identifier.
 * \return A handle to the desktop assigned to the thread, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDESK
NTAPI
NtUserGetThreadDesktop(
    _In_ ULONG ThreadId
    );

// rev
/**
 * The NtDesktopCaptureBits routine captures pixel data from the desktop surface.
 *
 * \param CaptureInfo A pointer to desktop capture info.
 * \param Param2 Second capture parameter.
 * \param Param3 Third capture parameter.
 * \param Left Left coordinate of the capture rectangle.
 * \param Top Top coordinate of the capture rectangle.
 * \param Flags Capture flags.
 * \param SectionHandle A handle to the shared section backing the bitmap bits.
 * \param Param8 Eighth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDesktopCaptureBits(
    _In_ PVOID CaptureInfo,
    _In_ ULONG Param2,
    _In_ ULONG Param3,
    _In_ LONG Left,
    _In_ LONG Top,
    _In_ LONG Flags,
    _In_ HANDLE SectionHandle,
    _In_ HANDLE Param8
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDeferredDesktopRotation routine applies a deferred desktop rotation.
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_DEFERREDDESKTOPROTATION) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDeferredDesktopRotation(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserMapDesktopObject routine maps a desktop object handle into the calling process.
 *
 * \param ObjectHandle The object handle.
 * \return The resulting pointer, or NULL on failure.
 * \remarks Exposed via NtUserCallOneParam(SFI_MAPDESKTOPOBJECT) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Ret_maybenull_
_Kernel_entry_
NTSYSCALLAPI
PVOID
NTAPI
NtUserMapDesktopObject(
    _In_ HANDLE ObjectHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserPaintDesktop routine fills the clipping region in the specified device context with the desktop pattern or wallpaper.
 *
 * \param Hdc Handle to the destination device context.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPaintDesktop(
    _In_ HDC Hdc
    );

// rev
/**
 * The NtUserResolveDesktopForWOW routine resolves the desktop path or name for a WOW64 or 16-bit emulation process context.
 *
 * \param DesktopName A pointer to a UNICODE_STRING containing the input desktop name, updated on return with the resolved name.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserResolveDesktopForWOW(
    _Inout_ PUNICODE_STRING DesktopName
    );

// rev
/**
 * The NtUserSetDesktopColorTransform routine applies a system color transform matrix or profile across desktop rendering surfaces.
 *
 * \param ColorTransform A pointer to a 100-byte buffer containing color transformation matrix values.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDesktopColorTransform(
    _In_reads_bytes_(100) PVOID ColorTransform
    );

// rev
/**
 * The NtUserSetDesktopVisualInputSink routine sets the visual input sink interface for processing desktop-level input redirection.
 *
 * \param DesktopInputSink A pointer to the desktop input sink object or endpoint.
 * \param Capability A pointer to the capability descriptor structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDesktopVisualInputSink(
    _In_ PVOID DesktopInputSink,
    _In_ PVOID Capability
    );

/**
 * The NtUserSetThreadDesktop routine assigns the specified desktop to the calling thread.
 *
 * \param DesktopHandle A handle to the desktop to be assigned to the calling thread.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetThreadDesktop(
    _In_ HDESK DesktopHandle
    );

/**
 * The NtUserSwitchDesktop routine makes the specified desktop visible and activates it.
 *
 * \param DesktopHandle A handle to the desktop to make active.
 * \param Flags Desktop switch flags.
 * \param FadeTime Fade transition duration in milliseconds.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSwitchDesktop(
    _In_ HDESK DesktopHandle,
    _In_opt_ ULONG Flags,
    _In_opt_ ULONG FadeTime
    );

// rev
/**
 * The NtUserUpdateDefaultDesktopThumbnail routine updates the cached shell thumbnail image or visual preview for a desktop or window.
 *
 * \param WindowHandle An optional handle to the window whose thumbnail is updated.
 * \param Param2 Thumbnail configuration parameter.
 * \param Param3 Thumbnail display parameter.
 * \param Param4 Thumbnail format or quality flag.
 * \param Param5 Additional update options.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateDefaultDesktopThumbnail(
    _In_opt_ HWND WindowHandle,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _In_ CHAR Param4,
    _In_ LONG Param5
    );

// rev
/**
 * The ResolveDesktopForWOW routine resolves the desktop to use for a WOW (32-bit) process.
 *
 * \param DesktopName Pointer to a UNICODE_STRING receiving or containing the desktop name.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtUserResolveDesktopForWOW system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
ResolveDesktopForWOW(
    _Inout_ PUNICODE_STRING DesktopName
    );

// rev
/**
 * The SetDeskWallpaper routine sets the desktop wallpaper bitmap file.
 *
 * \param FileName Optional pointer to null-terminated wallpaper image file path.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
SetDeskWallpaper(
    _In_opt_ PCSTR FileName
    );

// rev
/**
 * The SetDesktopColorTransform routine sets the desktop color transform matrix.
 *
 * \param ColorTransform Pointer to the buffer containing color transform parameters.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetDesktopColorTransform system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetDesktopColorTransform(
    _In_reads_bytes_(100) PVOID ColorTransform
    );

// rev
/**
 * The SwitchDesktopWithFade routine switches the input desktop using a visual fade transition.
 *
 * \param DesktopHandle Handle to the desktop to make active.
 * \param Flags Desktop switching flags.
 * \param FadeTime Duration of the fade transition in milliseconds.
 * \return BOOL TRUE on success, FALSE otherwise.
 * \remarks Thin user32 wrapper over NtUserSwitchDesktop.
 */
NTSYSAPI
BOOL
NTAPI
SwitchDesktopWithFade(
    _In_ HDESK DesktopHandle,
    _In_ _In_opt_ ULONG Flags,
    _In_ _In_opt_ ULONG FadeTime
    );

// rev
/**
 * The UpdateDefaultDesktopThumbnail routine updates the default desktop thumbnail visual representation.
 *
 * \param ThumbnailHandle Handle to the desktop thumbnail visual.
 * \param SourceRect Pointer to the source bounding rectangle.
 * \param DestinationRect Pointer to the destination bounding rectangle.
 * \param Flags Thumbnail update options and display flags.
 * \param Opacity Opacity value for thumbnail blending (0-255).
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserUpdateDefaultDesktopThumbnail system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
UpdateDefaultDesktopThumbnail(
    _In_ HANDLE ThumbnailHandle,
    _In_opt_ PRECT SourceRect,
    _In_opt_ PRECT DestinationRect,
    _In_ ULONG Flags,
    _In_ ULONG Opacity
    );

/**
 * The NtUserCloseDesktop routine closes an open handle to a desktop object.
 *
 * \param DesktopHandle A handle to the desktop to be closed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserCloseDesktop(
    _In_ HDESK DesktopHandle
    );

//
// Accelerator Tables (HACCEL)
//

/**
 * The NtUserCreateAcceleratorTable routine creates an accelerator table.
 *
 * \param paccel Pointer to an array of ACCEL structures describing the accelerator table.
 * \param cAccel The number of ACCEL structures in the array.
 * \return A handle to the created accelerator table, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HACCEL
NTAPI
NtUserCreateAcceleratorTable(
    _In_reads_(cAccel) LPACCEL paccel,
    _In_ LONG cAccel
    );

/**
 * The NtUserCopyAcceleratorTable routine copies the specified accelerator table.
 *
 * \param hAccelSrc A handle to the accelerator table to be copied.
 * \param lpAccelDst Pointer to an array of ACCEL structures that receives accelerator-table information.
 * \param cAccelEntries The number of ACCEL structures to copy to the buffer.
 * \return The number of accelerator-table entries copied, or the required size if lpAccelDst is NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserCopyAcceleratorTable(
    _In_ HACCEL hAccelSrc,
    _Out_writes_to_opt_(cAccelEntries, return) LPACCEL lpAccelDst,
    _In_ LONG cAccelEntries
    );

// rev
/**
 * The NtUserTranslateAccelerator routine processes accelerator keys for menu commands.
 *
 * \param WindowHandle A handle to the window whose messages are to be translated.
 * \param AcceleratorTable A handle to the accelerator table.
 * \param Message A pointer to an MSG structure that contains message information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserTranslateAccelerator(
    _In_ HWND WindowHandle,
    _In_ HACCEL AcceleratorTable,
    _In_ PMSG Message
    );

// rev
/**
 * The NtUserDestroyAcceleratorTable routine destroys an accelerator table and frees its associated resources.
 *
 * \param AcceleratorTableHandle Handle to the accelerator table to be destroyed.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyAcceleratorTable(
    _In_ HACCEL AcceleratorTableHandle
    );

//
// Carets
//

// rev
/**
 * The NtUserCreateCaret routine creates a new shape for the system caret and assigns ownership of the caret to the specified window.
 *
 * \param WindowHandle Handle to the window that owns the caret.
 * \param BitmapHandle Optional handle to the bitmap that defines the caret shape, or NULL.
 * \param Width The width of the caret, in logical units.
 * \param Height The height of the caret, in logical units.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateCaret(
    _In_ HWND WindowHandle,
    _In_opt_ HBITMAP BitmapHandle,
    _In_ ULONG Width,
    _In_ ULONG Height
    );

/**
 * The NtUserGetCaretBlinkTime routine returns the time required to invert the caret's pixels.
 *
 * \return The blink time in milliseconds.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetCaretBlinkTime(
    VOID
    );

/**
 * The NtUserGetCaretPos routine copies the caret's position to the specified POINT structure.
 *
 * \param lpPoint Pointer to the POINT structure that is to receive the client coordinates of the caret.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCaretPos(
    _Out_ LPPOINT lpPoint
    );

// rev
/**
 * The NtUserHideCaret routine removes the caret from the screen.
 *
 * \param WindowHandle Optional handle to the window that owns the caret, or NULL for the active window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHideCaret(
    _In_opt_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetCaretBlinkTime routine sets the caret blink time.
 *
 * \param Milliseconds The time threshold in milliseconds.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETCARETBLINKTIME) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCaretBlinkTime(
    _In_ ULONG Milliseconds
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetCaretPos routine sets the caret position.
 *
 * \param x New horizontal coordinate of the caret.
 * \param y New vertical coordinate of the caret.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_SETCARETPOS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCaretPos(
    _In_ LONG x,
    _In_ LONG y
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserShowCaret routine makes the caret visible on the screen at the caret's current position.
 *
 * \param WindowHandle An optional handle to the window that owns the caret.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowCaret(
    _In_opt_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDestroyCaret routine destroys the current caret.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_DESTROYCARET) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyCaret(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

//
// Menus (HMENU)
//

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCreateMenu routine creates an empty menu.
 *
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallNoParam(SFI_CREATEMENU) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HMENU
NTAPI
NtUserCreateMenu(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCreatePopupMenu routine creates an empty pop-up menu.
 *
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallNoParam(SFI_CREATEPOPUPMENU) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HMENU
NTAPI
NtUserCreatePopupMenu(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserCheckMenuItem routine sets the state of the specified menu item's check-mark attribute.
 *
 * \param MenuHandle Handle to the menu of interest.
 * \param ItemId The menu item whose check-mark attribute is to be set (by command ID or position).
 * \param Check Flags that control the check-mark state and item addressing (e.g. MF_CHECKED, MF_UNCHECKED, MF_BYCOMMAND, MF_BYPOSITION).
 * \return ULONG The previous state of the menu item (MF_CHECKED or MF_UNCHECKED), or -1 if the item does not exist.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserCheckMenuItem(
    _In_ HMENU MenuHandle,
    _In_ ULONG ItemId,
    _In_ ULONG Check
    );

/**
 * The NtUserGetMenuBarInfo routine retrieves information about the specified menu bar.
 *
 * \param WindowHandle A handle to the window whose menu bar is to be queried.
 * \param idObject The menu object (OBJID_MENU, OBJID_CLIENT, OBJID_SYSMENU).
 * \param idItem The item for which to retrieve information.
 * \param pmbi Pointer to a MENUBARINFO structure that receives the information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetMenuBarInfo(
    _In_ HWND WindowHandle,
    _In_ LONG idObject,
    _In_ LONG idItem,
    _Inout_ PMENUBARINFO pmbi
    );

// rev
/**
 * The NtUserGetMenuIndex routine retrieves the zero-based position of a submenu item within a parent menu.
 *
 * \param MenuHandle Handle to the parent menu.
 * \param SubMenuHandle Handle to the submenu whose index is queried.
 * \return ULONG The zero-based index of the submenu item, or -1 if not found.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetMenuIndex(
    _In_ HMENU MenuHandle,
    _In_ HMENU SubMenuHandle
    );

/**
 * The NtUserGetMenuItemRect routine retrieves the bounding rectangle for the specified menu item.
 *
 * \param WindowHandle A handle to the window containing the menu.
 * \param MenuHandle A handle to the menu.
 * \param MenuIndex The zero-based position of the menu item.
 * \param MenuRect Pointer to a RECT structure that receives the screen coordinates of the bounding rectangle.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetMenuItemRect(
    _In_opt_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG MenuIndex,
    _Out_ PRECT MenuRect
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetSysMenuOffset routine returns the system-menu offset for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return System-menu offset for the window.
 * \remarks Exposed via NtUserCallHwndLock(SFI_GETSYSMENUOFFSET) before WIN11.
 */
_Success_(return != 0)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetSysMenuOffset(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetSystemMenu routine enables the application to access the window menu for copying and modifying.
 *
 * \param WindowHandle A handle to the window that will own a copy of the window menu.
 * \param Revert The action to be taken. If FALSE, returns handle to the copy of the window menu in use; if TRUE, resets window menu to default.
 * \return A handle to a copy of the window menu if Revert is FALSE; otherwise NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HMENU
NTAPI
NtUserGetSystemMenu(
    _In_ HWND WindowHandle,
    _In_ BOOL Revert
    );

// rev
/**
 * The CalcMenuBar routine calculates a window's menu-bar layout metrics.
 *
 * \param WindowHandle Handle to the target window.
 * \param Left Left coordinate boundary.
 * \param Right Right coordinate boundary.
 * \param Top Top coordinate boundary.
 * \param Rect Optional pointer receiving or providing the menu bar bounding rectangle.
 * \return ULONG The height of the menu bar in pixels, or 0 on failure.
 * \remarks Forwards to the NtUserCalcMenuBar system call.
 */
NTSYSAPI
ULONG
NTAPI
CalcMenuBar(
    _In_ HWND WindowHandle,
    _In_ LONG Left,
    _In_ LONG Right,
    _In_ LONG Top,
    _Inout_opt_ PRECT Rect
    );

// rev
/**
 * The DrawMenuBarTemp routine is an internal helper that draws a window's menu bar.
 *
 * \param WindowHandle Handle to the target window.
 * \param Hdc Handle to the device context to draw into.
 * \param Rect Pointer to the menu bar bounding rectangle.
 * \param MenuHandle Handle to the menu to draw.
 * \param FontHandle Optional handle to the font used for drawing menu text.
 * \return ULONG_PTR Status code or drawing result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DrawMenuBarTemp(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ HMENU MenuHandle,
    _In_opt_ HFONT FontHandle
    );

// rev
/**
 * The MenuWindowProcA routine is the window procedure for the internal menu window class (ANSI).
 *
 * \param WindowHandle Handle to the menu window.
 * \param ResultInfo Result context parameter.
 * \param Message Window message identifier.
 * \param wParam Additional message-specific parameter.
 * \param lParam Additional message-specific parameter.
 * \return ULONG_PTR Result of message processing.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MenuWindowProcA(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR ResultInfo,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// rev
/**
 * The MenuWindowProcW routine is the window procedure for the internal menu window class (Unicode).
 *
 * \param WindowHandle Handle to the menu window.
 * \param ResultInfo Result context parameter.
 * \param Message Window message identifier.
 * \param wParam Additional message-specific parameter.
 * \param lParam Additional message-specific parameter.
 * \return ULONG_PTR Result of message processing.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MenuWindowProcW(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR ResultInfo,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// rev
/**
 * The NtUserCalcMenuBar routine calculates the layout dimensions and positions of the menu bar for a window.
 *
 * \param WindowHandle Handle to the window containing the menu bar.
 * \param Left Left bounding coordinate for the menu bar calculation.
 * \param Right Right bounding coordinate for the menu bar calculation.
 * \param Top Top bounding coordinate for the menu bar calculation.
 * \param Rect Optional pointer to a RECT structure receiving the calculated menu bar bounding rectangle.
 * \return ULONG The height of the menu bar in pixels, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserCalcMenuBar(
    _In_ HWND WindowHandle,
    _In_ LONG Left,
    _In_ LONG Right,
    _In_ LONG Top,
    _Inout_opt_ PRECT Rect
    );

// rev
/**
 * The NtUserDoMenuOperation routine executes an internal menu operation or tracking command.
 *
 * \param MenuOperation Pointer to a menu operation descriptor structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDoMenuOperation(
    _In_ PVOID MenuOperation
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDrawMenuBar routine redraws the menu bar of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_DRAWMENUBAR) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawMenuBar(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDrawMenuBarTemp routine draws a menu bar into a device context using temporary state.
 *
 * \param Param1 Unconfirmed window handle or DC parameter.
 * \param Param2 Unconfirmed menu handle or coordinate parameter.
 * \param Param3 Unconfirmed bounding rectangle or menu item parameter.
 * \param Param4 Unconfirmed font or rendering parameter.
 * \param Param5 Unconfirmed flags or drawing options parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawMenuBarTemp(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2,
    _In_ ULONG_PTR Param3,
    _In_ ULONG_PTR Param4,
    _In_ ULONG_PTR Param5
    );

// rev
/**
 * The NtUserEnableMenuItem routine enables, disables, or grays the specified menu item.
 *
 * \param MenuHandle Handle to the menu containing the item.
 * \param ItemId Identifier or position of the menu item.
 * \param Enable Flags controlling item state and addressing (MF_ENABLED, MF_DISABLED, MF_GRAYED, MF_BYCOMMAND, MF_BYPOSITION).
 * \return ULONG The previous state of the menu item, or -1 if the item does not exist.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserEnableMenuItem(
    _In_ HMENU MenuHandle,
    _In_ ULONG ItemId,
    _In_ ULONG Enable
    );

/**
 * The NtUserHiliteMenuItem routine highlights or removes highlighting from an item in a menu bar.
 *
 * \param WindowHandle A handle to the window containing the menu.
 * \param MenuHandle A handle to the menu bar containing the item.
 * \param IDHiliteItem The menu item to be highlighted, as determined by the Hilite parameter.
 * \param Hilite Flags specifying whether the item is highlighted (MF_HILITE or MF_UNHILITE) and how IDHiliteItem is interpreted.
 * \return TRUE if the menu item is set to the specified highlight state; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserHiliteMenuItem(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG IDHiliteItem,
    _In_ ULONG Hilite
    );

// rev
/**
 * The NtUserInitializeUserModeMenus routine initializes client-side user-mode menu support.
 *
 * \return Result code.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserInitializeUserModeMenus(
    VOID
    );

/**
 * The NtUserMenuItemFromPoint routine determines which menu item is at the specified screen coordinates.
 *
 * \param WindowHandle A handle to the window containing the menu.
 * \param MenuHandle A handle to the menu.
 * \param ptScreen POINT structure specifying the point to test in screen coordinates.
 * \return The zero-based position of the menu item, or -1 if no item is at that point.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserMenuItemFromPoint(
    _In_opt_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ POINT ptScreen
    );

// rev
/**
 * The NtUserPaintMenuBar routine paints the menu bar of the specified window into a device context.
 *
 * \param WindowHandle Handle to the window containing the menu bar.
 * \param Hdc Handle to the destination device context.
 * \param LeftMargin Left bounding margin for menu bar painting.
 * \param RightMargin Right bounding margin for menu bar painting.
 * \param Top Top coordinate for menu bar painting.
 * \param Flags Menu bar painting control flags.
 * \return ULONG The height of the menu bar in pixels, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserPaintMenuBar(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ LONG LeftMargin,
    _In_ LONG RightMargin,
    _In_ LONG Top,
    _In_ LONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetDialogSystemMenu routine sets the system menu of the specified dialog window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_SETDIALOGSYSTEMMENU) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDialogSystemMenu(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetMenu routine assigns a new menu to the specified window and optionally redraws the window frame.
 *
 * \param WindowHandle A handle to the window to which the menu is to be assigned.
 * \param MenuHandle An optional handle to the new menu, or NULL to remove the current menu.
 * \param Redraw TRUE to redraw the window frame; FALSE otherwise.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetMenu(
    _In_ HWND WindowHandle,
    _In_opt_ HMENU MenuHandle,
    _In_ BOOL Redraw
    );

// rev
/**
 * The NtUserSetMenuContextHelpId routine associates a Help context identifier with a menu.
 *
 * \param MenuHandle A handle to the menu.
 * \param ContextHelpId The Help context identifier.
 * \return TRUE if successful, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetMenuContextHelpId.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetMenuContextHelpId(
    _In_ HMENU MenuHandle,
    _In_ ULONG ContextHelpId
    );

// rev
/**
 * The NtUserSetMenuDefaultItem routine sets the default menu item for the specified menu.
 *
 * \param HMenu A handle to the menu to set the default item for.
 * \param UItem The identifier or position of the new default menu item.
 * \param FByPos Nonzero to indicate UItem is a position; zero if UItem is an item identifier.
 * \return TRUE if the function succeeds, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetMenuDefaultItem.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetMenuDefaultItem(
    _In_ HMENU HMenu,
    _In_ ULONG UItem,
    _In_ ULONG FByPos
    );

// rev
/**
 * The NtUserSetMenuFlagRtoL routine enables right-to-left reading order and layout flags for the specified menu.
 *
 * \param MenuHandle A handle to the menu to configure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetMenuFlagRtoL(
    _In_ HMENU MenuHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetSysMenu routine sets the system menu of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_SETSYSMENU) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetSysMenu(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetSystemMenu routine replaces the window system menu with a new menu.
 *
 * \param WindowHandle A handle to the window whose system menu is to be changed.
 * \param MenuHandle A handle to the new system menu.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetSystemMenu(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle
    );

// rev
/**
 * The NtUserThunkedMenuInfo routine retrieves or sets menu information through internal thunking structures.
 *
 * \param MenuHandle A handle to the menu.
 * \param MenuInfo A pointer to a MENUINFO structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserThunkedMenuInfo(
    _In_ HMENU MenuHandle,
    _In_ PVOID MenuInfo
    );

// rev
/**
 * The NtUserThunkedMenuItemInfo routine retrieves or modifies menu item information through internal thunking structures.
 *
 * \param MenuHandle A handle to the menu that contains the menu item.
 * \param Item The identifier or position of the menu item.
 * \param ByPosition Nonzero if Item is a position; zero if Item is a command identifier.
 * \param Insert Nonzero if inserting a new item; zero if querying or modifying an existing item.
 * \param ItemInfo A pointer to a MENUITEMINFOW structure.
 * \param ItemName An optional pointer to a UNICODE_STRING for the item text.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserThunkedMenuItemInfo(
    _In_ HMENU MenuHandle,
    _In_ LONG Item,
    _In_ LONG ByPosition,
    _In_ LONG Insert,
    _In_ PVOID ItemInfo,
    _In_opt_ ULONG_PTR ItemName
    );

/**
 * The NtUserTrackPopupMenuEx routine displays a shortcut menu at the specified location and tracks the selection of items on the shortcut menu.
 *
 * \param MenuHandle A handle to the shortcut menu to be displayed.
 * \param Flags Position and tracking options (TPM_*).
 * \param x The horizontal location of the shortcut menu, in screen coordinates.
 * \param y The vertical location of the shortcut menu, in screen coordinates.
 * \param WindowHandle A handle to the window that owns the shortcut menu.
 * \param lptpm Pointer to a TPMPARAMS structure that specifies an area of the screen the menu should not overlap.
 * \return If TPM_RETURNCMD is specified, the menu item identifier; otherwise nonzero on success, 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserTrackPopupMenuEx(
    _In_ HMENU MenuHandle,
    _In_ ULONG Flags,
    _In_ LONG x,
    _In_ LONG y,
    _In_ HWND WindowHandle,
    _In_opt_ LPTPMPARAMS lptpm
    );

// rev
/**
 * The PaintMenuBar routine paints a window's menu bar.
 *
 * \param WindowHandle Handle to the window whose menu bar is painted.
 * \param Hdc Handle to the device context to paint into.
 * \param LeftMargin Left margin in pixels.
 * \param RightMargin Right margin in pixels.
 * \param Top Top margin in pixels.
 * \param Flags Menu bar drawing flags.
 * \return ULONG The height of the menu bar in pixels, or 0 on failure.
 * \remarks Forwards to the NtUserPaintMenuBar system call.
 */
NTSYSAPI
ULONG
NTAPI
PaintMenuBar(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ LONG LeftMargin,
    _In_ LONG RightMargin,
    _In_ LONG Top,
    _In_ LONG Flags
    );

// rev
/**
 * The SetSystemMenu routine sets the system (window) menu of a window.
 *
 * \param WindowHandle Handle to the window whose system menu is to be set.
 * \param MenuHandle Handle to the new system menu.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetSystemMenu system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetSystemMenu(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle
    );

/**
 * The NtUserDeleteMenu routine deletes an item from the specified menu.
 *
 * \param MenuHandle A handle to the menu to be changed.
 * \param Position The menu item to be deleted, as determined by the Flags parameter.
 * \param Flags Flags indicating how the Position parameter is interpreted (MF_BYCOMMAND or MF_BYPOSITION).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDeleteMenu(
    _In_ HMENU MenuHandle,
    _In_ ULONG Position,
    _In_ ULONG Flags
    );

/**
 * The NtUserDestroyMenu routine destroys the specified menu and frees any memory that the menu occupies.
 *
 * \param MenuHandle A handle to the menu to be destroyed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDestroyMenu(
    _In_ HMENU MenuHandle
    );

/**
 * The NtUserEndMenu routine ends the calling thread's active menu.
 *
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEndMenu(
    VOID
    );

/**
 * The NtUserRemoveMenu routine deletes a menu item or detaches a submenu from the specified menu.
 *
 * \param MenuHandle A handle to the menu to be changed.
 * \param Position The menu item to be removed, as determined by the Flags parameter.
 * \param Flags Flags specifying how Position is interpreted (MF_BYCOMMAND or MF_BYPOSITION).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRemoveMenu(
    _In_ HMENU MenuHandle,
    _In_ ULONG Position,
    _In_ ULONG Flags
    );

//
// Clipboards
//

// rev
/**
 * The NtUserGetOpenClipboardWindow routine retrieves the handle to the window that currently has the clipboard open.
 *
 * \return HWND Handle to the window that has the clipboard open, or NULL if the clipboard is not open.
 * \remarks Native entry point for USER32!GetOpenClipboardWindow.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetOpenClipboardWindow(
    VOID
    );

// rev
/**
 * The NtUserOpenClipboard routine opens the clipboard for examination and prevents other applications from modifying the clipboard content.
 *
 * \param WindowHandle Optional handle to the window to be associated with the open clipboard.
 * \param EmptyClient Receives a boolean flag set by the clipboard open operation.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserOpenClipboard(
    _In_opt_ HWND WindowHandle,
    _Out_ PULONG EmptyClient
    );

// rev
/**
 * The CheckProcessForClipboardAccess routine determines whether a process is granted clipboard access.
 *
 * \param ProcessId Process identifier to evaluate.
 * \param GrantedAccess Pointer that receives the granted access mask.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserCheckProcessForClipboardAccess.
 */
NTSYSAPI
LOGICAL
NTAPI
CheckProcessForClipboardAccess(
    _In_ ULONG ProcessId,
    _Out_ PULONG GrantedAccess
    );

// rev
/**
 * The GetClipboardAccessToken routine retrieves the access token associated with clipboard access.
 *
 * \param TokenHandle Pointer that receives the handle to the clipboard access token.
 * \param DesiredAccess Desired access mask for the token.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserGetClipboardAccessToken system call.
 */
NTSYSAPI
LOGICAL
NTAPI
GetClipboardAccessToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * The NtUserCheckProcessForClipboardAccess routine checks whether a process has permission to access the clipboard.
 *
 * \param ProcessId The process identifier to check.
 * \param GrantedAccess Receives granted access rights flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCheckProcessForClipboardAccess(
    _In_ ULONG ProcessId,
    _Out_ PULONG GrantedAccess
    );

// rev
/**
 * The NtUserCountClipboardFormats routine retrieves the number of different data formats currently available on the clipboard.
 *
 * \return ULONG The number of data formats currently on the clipboard, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserCountClipboardFormats(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnumClipboardFormats routine enumerates the next available clipboard format after the specified one.
 *
 * \param Format Known clipboard format to enumerate after, or 0 to retrieve the first available format.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_ENUMCLIPBOARDFORMATS) before WIN11.
 */
_Success_(return != 0)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserEnumClipboardFormats(
    _In_ ULONG Format
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetClipboardAccessToken routine retrieves the access token of the process currently owning the clipboard.
 *
 * \param TokenHandle Pointer to a handle variable receiving the clipboard owner's token handle.
 * \param DesiredAccess Desired access mask for the opened token handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetClipboardAccessToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The NtUserGetClipboardData routine retrieves data from the clipboard in the specified format.
 *
 * \param Format Clipboard format identifier (CF_*).
 * \param ClipboardData Pointer to clipboard data information or buffer receiving data.
 * \return HANDLE Handle to the clipboard object, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetClipboardData(
    _In_ ULONG Format,
    _Out_ PVOID ClipboardData
    );

// rev
/**
 * The NtUserGetClipboardFormatName routine retrieves the name of the specified registered clipboard format.
 *
 * \param Format Registered format identifier to query.
 * \param Buffer Pointer to a wide-character buffer that receives the format name.
 * \param BufferCount Maximum number of characters to copy into the buffer.
 * \return ULONG Number of characters copied to the buffer, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetClipboardFormatName(
    _In_ USHORT Format,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    );

// rev
/**
 * The NtUserGetClipboardMetadata routine queries metadata attributes for a specific clipboard format.
 *
 * \param Format Clipboard format identifier to query.
 * \param Metadata Pointer to a GETCLIPBMETADATA structure receiving format metadata.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetClipboardMetadata.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetClipboardMetadata(
    ULONG Format,
    _Inout_ PGETCLIPBMETADATA Metadata
    );

// rev
/**
 * The NtUserGetClipboardOwner routine retrieves the window handle of the current clipboard owner.
 *
 * \return HWND Handle to the window owning the clipboard, or NULL if unowned.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetClipboardOwner(
    VOID
    );

// rev
/**
 * The NtUserGetClipboardSequenceNumber routine retrieves the serial number of the clipboard contents for the current window station.
 *
 * \return ULONG The current clipboard sequence number.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetClipboardSequenceNumber(
    VOID
    );

// rev
/**
 * The NtUserGetClipboardViewer routine retrieves the handle of the first window in the clipboard viewer chain.
 *
 * \return HWND Handle to the first window in the clipboard viewer chain, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetClipboardViewer(
    VOID
    );

// rev
/**
 * The NtUserGetPriorityClipboardFormat routine retrieves the first available clipboard format in the specified list.
 *
 * \param PriorityList Pointer to an array of clipboard formats in priority order.
 * \param Count Number of format entries in PriorityList.
 * \return LONG The first format from the list that is present on the clipboard, 0 if empty, or -1 if none match.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetPriorityClipboardFormat(
    _In_reads_(Count) PULONG PriorityList,
    _In_ LONG Count
    );

// rev
/**
 * The NtUserGetUpdatedClipboardFormats routine retrieves the current list of clipboard formats that have updated.
 *
 * \param Formats Pointer to an array of format identifiers to receive clipboard formats.
 * \param FormatCount Maximum number of format identifiers that can be held in Formats.
 * \param FormatsOut Pointer to a variable receiving the actual number of formats copied or required.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetUpdatedClipboardFormats(
    _Out_writes_(FormatCount) PULONG Formats,
    _In_ LONG_PTR FormatCount,
    _Out_ PULONG FormatsOut
    );

// rev
/**
 * The NtUserIsClipboardFormatAvailable routine determines whether the clipboard contains data in the specified format.
 *
 * \param Format Standard or registered clipboard format identifier (CF_*).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsClipboardFormatAvailable(
    _In_ ULONG Format
    );

// rev
/**
 * The NtUserAddClipboardFormatListener routine registers the specified window to the clipboard format listener list.
 *
 * \param WindowHandle Handle to the window to be placed on the clipboard format listener list.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAddClipboardFormatListener(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserChangeClipboardChain routine removes a specified window from the chain of clipboard viewers.
 *
 * \param WindowHandle Handle to the window to be removed from the chain.
 * \param NextWindowHandle Optional handle to the window that follows WindowHandle in the clipboard viewer chain.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserChangeClipboardChain(
    _In_ HWND WindowHandle,
    _In_opt_ HWND NextWindowHandle
    );

// rev
/**
 * The NtUserSetClipboardData routine places data on the clipboard in a specified clipboard format.
 *
 * \param Format The standard or registered clipboard format.
 * \param ClipboardData A handle to the data in the specified format.
 * \param Flags Flags specifying clipboard ownership or memory management behavior.
 * \return The handle to the data placed on the clipboard, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserSetClipboardData(
    _In_ ULONG Format,
    _In_ PVOID ClipboardData,
    _In_ ULONG_PTR Flags
    );

// rev
/**
 * The NtUserSetClipboardViewer routine adds the specified window to the chain of clipboard viewers.
 *
 * \param WindowHandle A handle to the window to be added to the clipboard viewer chain.
 * \return The handle to the next window in the clipboard viewer chain, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserSetClipboardViewer(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserCloseClipboard routine closes the clipboard.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCloseClipboard(
    VOID
    );

// rev
/**
 * The NtUserEmptyClipboard routine empties the clipboard and frees handles to data in the clipboard.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEmptyClipboard(
    VOID
    );

// rev
/**
 * The NtUserRemoveClipboardFormatListener routine removes the specified window from the clipboard format listener list maintained by the system.
 *
 * \param WindowHandle A handle to the window to remove.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRemoveClipboardFormatListener(
    _In_ HWND WindowHandle
    );

//
// Cursors & Icons (HCURSOR / HICON)
//

// rev
/**
 * The NtUserCreateEmptyCursorObject routine allocates an empty cursor or icon object handle.
 *
 * \param Animated Non-zero if the cursor is an animated cursor; 0 for a static cursor.
 * \return HICON Handle to the newly created empty cursor or icon object, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HICON
NTAPI
NtUserCreateEmptyCursorObject(
    _In_ LONG Animated
    );

// rev
/**
 * The GetCursorFrameInfo routine retrieves animation frame information for a cursor.
 *
 * \param CursorHandle Optional handle to the cursor to query.
 * \param ResourceName Optional pointer to the resource name of the cursor.
 * \param Step Animation frame step index to query.
 * \param Rate Pointer that receives the frame display duration rate.
 * \param NumSteps Pointer that receives the total number of animation steps.
 * \return HICON Handle to the icon/cursor frame, or NULL on failure.
 * \remarks Forwards to the NtUserGetCursorFrameInfo system call.
 */
NTSYSAPI
HICON
NTAPI
GetCursorFrameInfo(
    _In_opt_ HICON CursorHandle,
    _In_opt_ PCWSTR ResourceName,
    _In_ ULONG Step,
    _Out_ PDWORD Rate,
    _Out_ PLONG NumSteps
    );

// rev
/**
 * The InternalGetWindowIcon routine retrieves an icon associated with a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param IconType Type of icon to retrieve (ICON_SMALL, ICON_BIG, or ICON_SMALL2).
 * \return HICON Handle to the icon, or NULL if unavailable.
 * \remarks Thin user32 wrapper over NtUserInternalGetWindowIcon.
 */
NTSYSAPI
HICON
NTAPI
InternalGetWindowIcon(
    _In_ HWND WindowHandle,
    _In_ ULONG IconType
    );

// rev
/**
 * The MITGetCursorUpdateHandle routine retrieves an event handle signaled on Modern Input Transport cursor updates.
 *
 * \return HANDLE The cursor update event handle.
 * \remarks Forwards to the NtMITGetCursorUpdateHandle system call.
 */
NTSYSAPI
HANDLE
NTAPI
MITGetCursorUpdateHandle(
    VOID
    );

// rev
/**
 * The NtUserFindExistingCursorIcon routine searches the cursor cache for an existing cursor or icon matching the resource name.
 *
 * \param ModuleName Pointer to a UNICODE_STRING naming the module owning the resource.
 * \param ResourceName Pointer to a UNICODE_STRING identifying the cursor or icon resource.
 * \param CursorInfo Pointer to the cursor search attributes structure.
 * \return HICON A handle to the matching cursor or icon, or NULL if no match exists.
 */
_Kernel_entry_
NTSYSCALLAPI
HICON
NTAPI
NtUserFindExistingCursorIcon(
    _In_ PUNICODE_STRING ModuleName,
    _In_ PUNICODE_STRING ResourceName,
    _In_ PVOID CursorInfo
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetClassIcoCur routine retrieves a class icon or cursor for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Index Class icon or cursor index (GCLP_HICON or GCLP_HCURSOR).
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallHwndParam(SFI_GETCLASSICOCUR) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HCURSOR
NTAPI
NtUserGetClassIcoCur(
    _In_ HWND WindowHandle,
    _In_ ULONG Index
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetClipCursor routine retrieves the screen coordinates of the rectangular area to which the cursor is confined.
 *
 * \param lpRect Pointer to a RECT structure that receives the screen coordinates of the confining rectangle.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetClipCursor(
    _Out_ LPRECT lpRect
    );

/**
 * The NtUserGetCursor routine retrieves a handle to the current cursor.
 *
 * \return A handle to the current cursor, or NULL if there is no cursor.
 */
_Kernel_entry_
NTSYSCALLAPI
HCURSOR
NTAPI
NtUserGetCursor(
    VOID
    );

// rev
/**
 * The NtUserGetCursorFrameInfo routine retrieves animation rate and frame count for an animated cursor.
 *
 * \param CursorHandle Handle to the animated cursor (ACON).
 * \param Step Frame step index to query.
 * \param Rate Pointer to a variable receiving the animation frame rate in jiffies (1/60s).
 * \param NumSteps Pointer to a variable receiving the total number of animation frames.
 * \return HCURSOR Handle to the cursor icon for the specified frame.
 */
_Kernel_entry_
NTSYSCALLAPI
HCURSOR
NTAPI
NtUserGetCursorFrameInfo(
    _In_ HICON CursorHandle,
    _In_ LONG Step,
    _Out_ PULONG Rate,
    _Out_ PULONG NumSteps
    );

/**
 * The NtUserGetCursorInfo routine retrieves information about the global cursor.
 *
 * \param pci Pointer to a CURSORINFO structure that receives the information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCursorInfo(
    _Inout_ PCURSORINFO pci
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetCursorPos routine retrieves the current cursor position.
 *
 * \param Point Pointer to a POINT structure that receives the coordinates.
 * \param CursorPosType The cursor pos type (CURSOR_POS_TYPE_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_GETCURSORPOS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetCursorPos(
    _Out_ PPOINT Point,
    _In_ ULONG CursorPosType // CURSOR_POS_TYPE_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetDwmCursorShape routine retrieves the cursor shape bitmaps and hot spot used by DWM hardware cursor rendering.
 *
 * \param CursorType Cursor shape type or query identifier.
 * \param Buffer Optional pointer to a buffer receiving cursor shape bitmap bits.
 * \param BufferSize Size, in bytes, of the destination buffer.
 * \param BytesReturned Pointer to a variable receiving the number of bytes written or required.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetDwmCursorShape(
    _In_ ULONG CursorType,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned
    );

/**
 * The NtUserGetIconInfo routine retrieves information about the specified icon or cursor.
 *
 * \param IconOrCursorHandle A handle to the icon or cursor.
 * \param Iconinfo A pointer to an ICONINFO structure that receives the icon or cursor information.
 * \param Name Optional pointer to a UNICODE_STRING receiving the resource name.
 * \param ResourceId Optional pointer to a UNICODE_STRING receiving the resource ID.
 * \param ColorBits Optional pointer to a variable receiving the color bit depth.
 * \param IsCursorHandle TRUE if the handle is a cursor; FALSE if it is an icon.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetIconInfo(
    _In_ HICON IconOrCursorHandle,
    _Out_ PICONINFO Iconinfo,
    _Inout_opt_ PUNICODE_STRING Name,
    _Inout_opt_ PUNICODE_STRING ResourceId,
    _Out_opt_ PULONG ColorBits,
    _In_ LOGICAL IsCursorHandle
    );

/**
 * The NtUserGetIconSize routine retrieves the dimensions of the specified icon or cursor.
 *
 * \param IconOrCursorHandle A handle to the icon or cursor.
 * \param IsCursorHandle TRUE if the handle is a cursor; FALSE if it is an icon.
 * \param XX Pointer to a variable that receives the width of the icon or cursor.
 * \param YY Pointer to a variable that receives the height of the icon or cursor.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetIconSize(
    _In_ HGDIOBJ IconOrCursorHandle,
    _In_ LOGICAL IsCursorHandle,
    _Out_ PULONG XX,
    _Out_ PULONG YY
    );

// rev
/**
 * The NtUserGetRequiredCursorSizes routine queries the required cursor bitmap sizes for the current display metrics.
 *
 * \param CursorHandle Handle to the cursor or icon to query.
 * \param CursorSizeInfo Pointer to a structure receiving the required cursor dimensions.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetRequiredCursorSizes(
    _In_ HICON CursorHandle,
    _Out_ PVOID CursorSizeInfo
    );

/**
 * The NtUserInternalGetWindowIcon routine retrieves a handle to the large or small icon associated with a window.
 *
 * \param WindowHandle A handle to the window.
 * \param IconType The type of icon to retrieve (e.g. ICON_SMALL, ICON_BIG).
 * \return A handle to the icon, or NULL if the window has no icon.
 */
_Kernel_entry_
NTSYSCALLAPI
HICON
NTAPI
NtUserInternalGetWindowIcon(
    _In_ HWND WindowHandle,
    _In_ ULONG IconType
    );

// rev
/**
 * The DrawCaptionTempA routine is an internal DrawCaption helper accepting explicit colors (ANSI).
 *
 * \param WindowHandle Optional handle to the window providing caption text and icons.
 * \param Hdc Handle to the device context to draw into.
 * \param Rect Pointer to the bounding rectangle.
 * \param FontHandle Optional font handle for the caption.
 * \param IconHandle Optional icon handle for the caption.
 * \param Text Optional pointer to the null-terminated caption string.
 * \param Flags Drawing flags (DC_*).
 * \return ULONG_PTR Status code or drawing result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DrawCaptionTempA(
    _In_opt_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_opt_ HFONT FontHandle,
    _In_opt_ HICON IconHandle,
    _In_opt_ PSTR Text,
    _In_ LONG Flags
    );

// rev
/**
 * The DrawCaptionTempW routine is an internal DrawCaption helper accepting explicit colors (Unicode).
 *
 * \param WindowHandle Optional handle to the window providing caption text and icons.
 * \param Hdc Handle to the device context to draw into.
 * \param Rect Pointer to the bounding rectangle.
 * \param FontHandle Optional font handle for the caption.
 * \param IconHandle Optional icon handle for the caption.
 * \param Text Optional pointer to the null-terminated caption string.
 * \param Flags Drawing flags (DC_*).
 * \return ULONG_PTR Status code or drawing result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DrawCaptionTempW(
    _In_opt_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_opt_ HFONT FontHandle,
    _In_opt_ HICON IconHandle,
    _In_opt_ PWSTR Text,
    _In_ LONG Flags
    );

// rev
/**
 * The NtSetCursorInputSpace routine sets the input space for the system cursor.
 *
 * \param InputSpaceLuid The locally unique identifier of the target input space.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtSetCursorInputSpace(
    _In_ LUID InputSpaceLuid
    );

// rev
/**
 * The NtSetShellCursorState routine configures the shell cursor state.
 *
 * \param CursorState The shell cursor state bitmask.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtSetShellCursorState(
    _In_ ULONG64 CursorState,
    _In_ PCWSTR Param2,
    _In_ ULONG Param3
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserArrangeIconicWindows routine arranges the minimized child windows of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallHwndLock(SFI_ARRANGEICONICWINDOWS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserArrangeIconicWindows(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserClipCursor routine confines the cursor to a rectangular area on the screen.
 *
 * \param lpRect Pointer to the structure that contains the screen coordinates of the confining rectangle. If NULL, cursor is unclipped.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserClipCursor(
    _In_opt_ const RECT* lpRect
    );

/**
 * The NtUserDragObject routine drags an object from one window to another.
 *
 * \param WindowHandleParent A handle to the parent window that owns the drag operation.
 * \param WindowHandleFrom A handle to the window where the drag operation began.
 * \param fmt Format of the data being dragged.
 * \param data User-defined data to pass to the target window.
 * \param hcur A handle to the custom cursor to display during dragging, or NULL for default.
 * \return A value indicating the drop result (e.g. DL_COPY, DL_MOVE, or 0 if cancelled).
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserDragObject(
    _In_ HWND WindowHandleParent,
    _In_ HWND WindowHandleFrom,
    _In_ ULONG fmt,
    _In_ ULONG_PTR data,
    _In_opt_ HCURSOR hcur
    );

// rev
/**
 * The NtUserDrawCaptionTemp routine draws a temporary window caption with custom visual attributes.
 *
 * \param WindowHandle Optional handle to the window whose caption attributes are used.
 * \param Hdc Handle to the destination device context.
 * \param Rect Pointer to a RECT structure defining the caption bounds.
 * \param FontHandle Optional handle to the font used for caption text.
 * \param IconHandle Optional handle to the icon drawn in the caption.
 * \param Text Optional pointer to the caption text string.
 * \param Flags Caption drawing flags (DC_*).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawCaptionTemp(
    _In_opt_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_opt_ HFONT FontHandle,
    _In_opt_ HICON IconHandle,
    _In_opt_ PWSTR Text,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserDrawIconEx routine draws an icon or cursor into the specified device context with extended options.
 *
 * \param Hdc Handle to the destination device context.
 * \param X Horizontal position of the upper-left corner of the icon.
 * \param Y Vertical position of the upper-left corner of the icon.
 * \param IconHandle Handle to the icon or cursor to draw.
 * \param Width Desired width of the icon, in logical units.
 * \param Height Desired height of the icon, in logical units.
 * \param StepIfAniCur Frame index if drawing an animated cursor.
 * \param BrushHandle Optional handle to a background brush.
 * \param Flags Icon drawing flags (DI_*).
 * \param Param10 Additional rendering parameter or DPI scaling options.
 * \param Param11 Additional rendering parameter or animation state.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawIconEx(
    _In_ HDC Hdc,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ HICON IconHandle,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG StepIfAniCur,
    _In_opt_ HBRUSH BrushHandle,
    _In_ LONG Flags,
    _In_ LONG Param10,
    _In_ ULONG_PTR Param11
    );

// rev
/**
 * The NtUserEnableMouseInputForCursorSuppression routine controls cursor suppression during mouse or touch interactions.
 *
 * \param Enable Non-zero to enable cursor suppression; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableMouseInputForCursorSuppression(
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserEnableSoftwareCursorForScreenCapture routine enables or disables rendering of a software cursor during screen capture sessions.
 *
 * \param Enable Non-zero to enable software cursor rendering; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableSoftwareCursorForScreenCapture(
    _In_ ULONG Enable
    );

// rev
/**
 * The NtUserFrostCrashedWindow routine replaces an unresponsive or crashed window with a ghost/frost window.
 *
 * \param WindowHandle Handle to the crashed target window.
 * \param FrostWindowHandle Optional handle to the frost/ghost window.
 * \return HICON or handle to the ghost window visual representation.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserFrostCrashedWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND FrostWindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserHideCursorNoCapture routine hides the cursor without affecting mouse capture.
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_HIDECURSORNOCAPTURE) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserHideCursorNoCapture(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserLinkDpiCursor routine links a DPI-specific cursor instance to a base cursor handle.
 *
 * \param CursorHandle Handle to the base cursor.
 * \param DpiCursorHandle Handle to the DPI-specific scaled cursor.
 * \param Dpi Target dots per inch (DPI) value for the link.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLinkDpiCursor(
    _In_ HICON CursorHandle,
    _In_ HICON DpiCursorHandle,
    _In_ ULONG Dpi
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserLoadCursorsAndIcons routine loads the standard system cursors and icons.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_LOADCURSORSANDICONS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLoadCursorsAndIcons(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserLockCursor routine confines the mouse cursor to a rectangular area on the screen.
 *
 * \param Rect Optional pointer to a RECT structure containing the screen coordinates of the confining rectangle, or NULL to unclip.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLockCursor(
    _In_opt_ PRECT Rect
    );

// rev
/**
 * The NtUserSetClassLongPtr routine replaces the specified pointer-sized value at the specified offset into the extra class memory or class structure.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based offset to the value to be replaced (e.g. GCLP_WNDPROC, GCLP_MENUNAME, GCLP_HICON).
 * \param Value The replacement value.
 * \param Ansi Nonzero if handling ANSI window classes; zero for Unicode.
 * \return The previous value of the specified offset, or zero on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG_PTR
NTAPI
NtUserSetClassLongPtr(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_ ULONG_PTR Value,
    _In_ ULONG Ansi
    );

// rev
/**
 * The NtUserSetCursor routine sets the cursor image for the current thread or desktop.
 *
 * \param HCursor A handle to the cursor to set, or NULL to remove the cursor from the screen.
 * \return A handle to the previous cursor, or NULL if there was no previous cursor.
 * \remarks Native entry point for USER32!SetCursor.
 */
_Kernel_entry_
NTSYSCALLAPI
HCURSOR
NTAPI
NtUserSetCursor(
    _In_opt_ HCURSOR HCursor
    );

// rev
/**
 * The NtUserSetCursorIconData routine sets the internal frame data and attributes for an icon or cursor handle.
 *
 * \param CursorHandle A handle to the icon or cursor.
 * \param Param2 Frame index or icon data type.
 * \param Param3 Icon or cursor configuration parameters.
 * \param Param4 Icon or cursor configuration parameters.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCursorIconData(
    _In_ HICON CursorHandle,
    _In_ LONG Param2,
    _In_ LONG Param3,
    _In_ LONG Param4
    );

// rev
/**
 * The NtUserSetCursorIconDataEx routine sets extended frame data and module association attributes for an icon or cursor handle.
 *
 * \param CursorHandle A handle to the icon or cursor.
 * \param ModuleName A pointer to a UNICODE_STRING specifying the source module name.
 * \param ResourceName A pointer to a UNICODE_STRING identifying the cursor or icon resource.
 * \param CursorData A pointer to the 136-byte cursor or icon data structure.
 * \param Flags Control flags for cursor icon initialization.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCursorIconDataEx(
    _In_ HICON CursorHandle,
    _In_ PUNICODE_STRING ModuleName,
    _In_ PUNICODE_STRING ResourceName,
    _In_ PVOID CursorData,
    _In_ ULONG Flags
    );

/**
 * The NtUserSetCursorPos routine moves the cursor to the specified screen coordinates.
 *
 * \param X The new x-coordinate of the cursor, in screen coordinates.
 * \param Y The new y-coordinate of the cursor, in screen coordinates.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetCursorPos(
    _In_ LONG X,
    _In_ LONG Y
    );

// rev
/**
 * The NtUserSetSystemCursor routine replaces a system cursor with a specified cursor resource.
 *
 * \param CursorHandle A handle to the new cursor.
 * \param CursorId The system cursor identifier (e.g. OCR_NORMAL, OCR_IBEAM) to replace.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetSystemCursor(
    _In_ HICON CursorHandle,
    _In_ ULONG CursorId
    );

/**
 * The NtUserShowCursor routine displays or hides the cursor.
 *
 * \param Show If TRUE, cursor display count is incremented; if FALSE, decremented.
 * \return The new display counter value.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserShowCursor(
    _In_ BOOL Show
    );

// rev
/**
 * The NtUserShowSystemCursor routine increments or decrements the system cursor display counter.
 *
 * \param Show Nonzero to increment the cursor display count; zero to decrement.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowSystemCursor(
    _In_ ULONG Show
    );

// rev
/**
 * The PrivateExtractIconExA routine extracts icons of multiple sizes from a file (private, ANSI).
 *
 * \param FileName Pointer to the null-terminated icon or executable file path.
 * \param IconIndex Zero-based index of the initial icon to extract.
 * \param IconLarge Optional array receiving large icon handles.
 * \param IconSmall Optional array receiving small icon handles.
 * \param Icons Number of icons to extract.
 * \return ULONG Number of icons extracted, or 0 on failure.
 */
NTSYSAPI
ULONG
NTAPI
PrivateExtractIconExA(
    _In_ LPCSTR FileName,
    _In_ LONG IconIndex,
    _Out_opt_ HICON* IconLarge,
    _Out_opt_ HICON* IconSmall,
    _In_ ULONG Icons
    );

// rev
/**
 * The PrivateExtractIconExW routine extracts icons of multiple sizes from a file (private, Unicode).
 *
 * \param FileName Pointer to the null-terminated icon or executable file path.
 * \param IconIndex Zero-based index of the initial icon to extract.
 * \param IconLarge Optional array receiving large icon handles.
 * \param IconSmall Optional array receiving small icon handles.
 * \param Icons Number of icons to extract.
 * \return ULONG Number of icons extracted, or 0 on failure.
 */
NTSYSAPI
ULONG
NTAPI
PrivateExtractIconExW(
    _In_ LPCWSTR FileName,
    _In_ LONG IconIndex,
    _Out_opt_ HICON* IconLarge,
    _Out_opt_ HICON* IconSmall,
    _In_ ULONG Icons
    );

// rev
/**
 * The ShowSystemCursor routine shows or hides the system cursor.
 *
 * \param Show Non-zero to show the cursor; zero to hide.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserShowSystemCursor system call.
 */
NTSYSAPI
LOGICAL
NTAPI
ShowSystemCursor(
    _In_ ULONG Show
    );

// rev
/**
 * The NtUserDestroyCursor routine destroys a cursor or icon and frees its allocated memory.
 *
 * \param CursorHandle Handle to the cursor or icon to be destroyed.
 * \param Flags Destruction control flags or cursor type specifiers.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyCursor(
    _In_ HICON CursorHandle,
    _In_ ULONG Flags
    );

//
// Hooks (HHOOK)
//

typedef _Function_class_(FNGETQUEUESTATUS)
ULONG NTAPI FNGETQUEUESTATUS(
    _In_ ULONG WakeMask
    );

typedef FNGETQUEUESTATUS* PFNGETQUEUESTATUS;

typedef _Function_class_(FNGETMESSAGE)
LONG NTAPI FNGETMESSAGE(
    _Out_ PMSG Message,
    _In_opt_ HWND WindowHandle,
    _In_ ULONG FilterMin,
    _In_ ULONG FilterMax,
    _In_ ULONG RemoveMsg,
    _In_ BOOL GetMessage
    );

typedef FNGETMESSAGE* PFNGETMESSAGE;

typedef _Function_class_(FNWAITMESSAGEEX)
LONG NTAPI FNWAITMESSAGEEX(
    _In_ ULONG WakeMask,
    _In_ ULONG TimeoutMilliseconds
    );

typedef FNWAITMESSAGEEX* PFNWAITMESSAGEEX;

typedef _Function_class_(FNMSGWAITFORMULTIPLEOBJECTSEX)
ULONG NTAPI FNMSGWAITFORMULTIPLEOBJECTSEX(
    _In_ ULONG Count,
    _In_reads_opt_(Count) const HANDLE* Handles,
    _In_ ULONG TimeoutMilliseconds,
    _In_ ULONG WakeMask,
    _In_ ULONG Flags
    );

typedef FNMSGWAITFORMULTIPLEOBJECTSEX* PFNMSGWAITFORMULTIPLEOBJECTSEX;

typedef struct _MESSAGEPUMPHOOK
{
    ULONG Size;
    PFNGETMESSAGE GetMessageCallback;
    PFNWAITMESSAGEEX WaitMessageExCallback;
    PFNGETQUEUESTATUS GetQueueStatusCallback;
    PFNMSGWAITFORMULTIPLEOBJECTSEX MsgWaitForMultipleObjectsExCallback;
} MESSAGEPUMPHOOK, *PMESSAGEPUMPHOOK;

// rev
/**
 * Initializes or releases the process message-pump hook callbacks.
 *
 * \param MessagePumpHookFlags Zero to initialize; one to uninitialize.
 * \param MessagePumpHook On initialization, the default callback table to update in place.
 * This parameter is NULL during uninitialization.
 * \return TRUE to accept initialization, FALSE to reject it. Ignored during uninitialization.
 */
typedef _Function_class_(FNINITMPH)
BOOL NTAPI FNINITMPH(
    _In_ ULONG MessagePumpHookFlags,
    _Inout_opt_ PMESSAGEPUMPHOOK MessagePumpHook
    );

typedef FNINITMPH* PFNINITMPH;

typedef struct _MPH_STATE
{
    LONG LoadCount;
    LONG MessagePumpHookEnabled;
    PFNINITMPH InitializeMessagePumpHook;
    MESSAGEPUMPHOOK MessagePumpHook;
} MPH_STATE, *PMPH_STATE;

typedef _Function_class_(FNREGISTERMESSAGEPUMPHOOK)
BOOL NTAPI FNREGISTERMESSAGEPUMPHOOK(
    _In_ PFNINITMPH InitializeMessagePumpHook
    );

typedef FNREGISTERMESSAGEPUMPHOOK* PFNREGISTERMESSAGEPUMPHOOK;

typedef _Function_class_(FNUNREGISTERMESSAGEPUMPHOOK)
BOOL NTAPI FNUNREGISTERMESSAGEPUMPHOOK(
    VOID
    );

typedef FNUNREGISTERMESSAGEPUMPHOOK* PFNUNREGISTERMESSAGEPUMPHOOK;

// rev
/**
 * Specifies the DLL and initialization export names for RegisterUserApiHook.
 *
 * Size must be sizeof(USERAPIHOOKINFO): 0x28 on x64 or 0x14 on x86.
 * All names, including export names, are Unicode strings.
 * The x86 wrapper ignores the second DLL/export pair.
 */
typedef struct _USERAPIHOOKINFO
{
    ULONG Size;
    PCWSTR DllName1;
    PCWSTR FunctionName1;
    PCWSTR DllName2;
    PCWSTR FunctionName2;
} USERAPIHOOKINFO, *PUSERAPIHOOKINFO;

// rev
/**
 * The NtUserRegisterDManipHook routine registers DirectManipulation hooks within the window manager.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterDManipHook(
    _In_ PVOID HookProc,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterShellHookWindow routine registers a shell-hook window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwnd(SFI_REGISTERSHELLHOOKWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterShellHookWindow(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRegisterUserApiHook routine registers user API hook DLLs and their initialization exports.
 *
 * \param DllName1 Name of the first hook DLL.
 * \param FunctionName1 Name of the initialization export in the first DLL, as a Unicode string.
 * \param DllName2 Name of the second hook DLL.
 * \param FunctionName2 Name of the initialization export in the second DLL, as a Unicode string.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks The native x64 implementation captures all four UNICODE_STRING descriptors.
 * The x86 RegisterUserApiHook wrapper passes NULL for the second pair.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterUserApiHook(
    _In_ PCUNICODE_STRING DllName1,
    _In_ PCUNICODE_STRING FunctionName1,
    _In_opt_ PCUNICODE_STRING DllName2,
    _In_opt_ PCUNICODE_STRING FunctionName2
    );

// rev
/**
 * The NtUserSetWindowsHookAW routine installs an application-defined hook procedure into a hook chain (ANSI/Unicode thunked).
 *
 * \param HookId The type of hook procedure to be installed (e.g. WH_CALLWNDPROC, WH_KEYBOARD).
 * \param HookProc A pointer to the hook procedure.
 * \param Ansi Nonzero if the hook procedure expects ANSI messages; zero for Unicode.
 * \return A handle to the hook procedure, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HHOOK
NTAPI
NtUserSetWindowsHookAW(
    _In_ LONG HookId,
    _In_ LONG_PTR HookProc,
    _In_ LONG Ansi
    );

// rev
/**
 * The NtUserSetWindowsHookEx routine installs an application-defined hook procedure into a hook chain with extended module context.
 *
 * \param ModuleHandle An optional handle to the DLL containing the hook procedure.
 * \param ModuleName A pointer to the module name or string identifier.
 * \param ThreadId The identifier of the thread with which the hook procedure is to be associated.
 * \param HookId The type of hook procedure to be installed.
 * \param HookProc A pointer to the hook procedure.
 * \param Ansi Nonzero if the hook procedure expects ANSI messages; zero for Unicode.
 * \return A handle to the hook procedure, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HHOOK
NTAPI
NtUserSetWindowsHookEx(
    _In_opt_ HMODULE ModuleHandle,
    _In_ ULONG_PTR ModuleName,
    _In_ ULONG ThreadId,
    _In_ ULONG HookId,
    _In_ LONG_PTR HookProc,
    _In_ LONG Ansi
    );

// rev
/**
 * The RegisterDManipHook routine installs a DirectManipulation hook for window interaction handling.
 *
 * \param HookProc Callback procedure invoked during DirectManipulation events.
 * \param Flags Operational flags controlling hook installation.
 * \return ULONG_PTR Status code or hook handle.
 * \remarks Forwards to the NtUserRegisterDManipHook system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
RegisterDManipHook(
    _In_ PVOID HookProc,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The RegisterMessagePumpHook routine registers a callback hook for the message pump.
 *
 * \param InitializeMessagePumpHook Pointer to the initialization callback routine.
 * \return TRUE if successful, FALSE otherwise.
 * \remarks The first registration calls the initializer with flags zero and a MESSAGEPUMPHOOK
 * containing the default callbacks. The callback updates this table in place and must preserve
 * a valid Size (0x28 on x64 or 0x14 on x86). Subsequent registrations must specify the same
 * initializer. The final UnregisterMessagePumpHook call invokes it with flags one and NULL.
 * A NULL initializer fails with ERROR_INVALID_PARAMETER.
 */
NTSYSAPI
BOOL
NTAPI
RegisterMessagePumpHook(
    _In_ PFNINITMPH InitializeMessagePumpHook
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The RegisterUserApiHook routine registers a user API hook.
 *
 * \param ApiHookInfo Pointer to the DLL and initialization export names.
 * \return TRUE if successful, FALSE otherwise, including when Size is incorrect.
 * \remarks Converts the names to UNICODE_STRING descriptors and calls NtUserRegisterUserApiHook.
 * The x86 wrapper uses only the first DLL/export pair.
 */
NTSYSAPI
BOOL
NTAPI
RegisterUserApiHook(
    _In_ PUSERAPIHOOKINFO ApiHookInfo
    );

// rev
/**
 * The SetWindowsHookExAW routine is the common backend for SetWindowsHookExA and SetWindowsHookExW.
 *
 * \param HookId Type of hook procedure to install (WH_*).
 * \param HookProc Pointer to the hook procedure callback.
 * \param ModuleHandle Optional handle to the DLL containing the hook procedure.
 * \param ThreadId Identifier of the thread with which the hook procedure is to be associated.
 * \param Ansi TRUE for ANSI message translation; FALSE for Unicode.
 * \return HHOOK Handle to the hook procedure on success, NULL on failure.
 * \remarks Resolves module file name and forwards to NtUserSetWindowsHookEx.
 */
NTSYSAPI
HHOOK
NTAPI
SetWindowsHookExAW(
    _In_ LONG HookId,
    _In_ HOOKPROC HookProc,
    _In_opt_ HINSTANCE ModuleHandle,
    _In_ ULONG ThreadId,
    _In_ BOOL Ansi
    );

// rev
/**
 * The NtUserGetDManipHookInitFunction routine retrieves the module and procedure name for DirectManipulation hook initialization.
 *
 * \param ModuleName Pointer to a wide-character buffer of 260 characters receiving the module name.
 * \param FunctionName Pointer to a wide-character buffer of 260 characters receiving the function export name.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetDManipHookInitFunction(
    _Out_writes_(260) PWSTR ModuleName,
    _Out_writes_(260) PWSTR FunctionName
    );

// rev
/**
 * The InitDManipHook routine initializes the Direct Manipulation hook.
 *
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
InitDManipHook(
    VOID
    );

// rev
/**
 * The InitializeLpkHooks routine initializes the Language Pack (LPK) hooks in user32.
 *
 * \param LpkHooks Pointer to the LPK function table structure.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
InitializeLpkHooks(
    _In_ PVOID LpkHooks
    );

// rev
/**
 * The NtUserCallNextHookEx routine passes the hook information to the next hook procedure in the current hook chain.
 *
 * \param Code Hook code passed to the current hook procedure.
 * \param wParam The wParam value passed to the current hook procedure.
 * \param lParam The lParam value passed to the current hook procedure.
 * \param Ansi Non-zero if ANSI messaging is active; 0 for Unicode.
 * \return LRESULT The value returned by the next hook procedure in the chain.
 */
_Kernel_entry_
NTSYSCALLAPI
LRESULT
NTAPI
NtUserCallNextHookEx(
    _In_ ULONG Code,
    _In_ WPARAM wParam,
    _Inout_ LPARAM lParam,
    _In_ LONG Ansi
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDeregisterShellHookWindow routine deregisters a shell-hook window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwnd(SFI_DEREGISTERSHELLHOOKWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDeregisterShellHookWindow(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDoInitMessagePumpHook routine installs the message-pump hook for the calling thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_DOINITMESSAGEPUMPHOOK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDoInitMessagePumpHook(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDoUninitMessagePumpHook routine removes the message-pump hook for the calling thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_DOUNINITMESSAGEPUMPHOOK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDoUninitMessagePumpHook(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserLoadUserApiHook routine loads the registered user API hook (uxtheme) provider.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_LOADUSERAPIHOOK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLoadUserApiHook(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRedrawFrameAndHook routine redraws the non-client frame of the specified window and runs frame hooks.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_REDRAWFRAMEANDHOOK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawFrameAndHook(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetWinEventHook routine sets an event hook function for an event range.
 *
 * \param EventMin Specifies the event constant for the lowest event value in the range of events handled by the hook function.
 * \param EventMax Specifies the event constant for the highest event value in the range of events handled by the hook function.
 * \param ModuleHandle Handle to the DLL that contains the hook function at WinEventProc, if WINEVENT_INCONTEXT flag is specified.
 * \param Callback Internal callback address or dispatch pointer.
 * \param WinEventProc Pointer to the event hook function.
 * \param ProcessId Specifies the ID of the process from which the hook function receives events.
 * \param ThreadId Specifies the ID of the thread from which the hook function receives events.
 * \param Flags Flag values that specify the location of the hook function and of the events to be skipped.
 * \return An event hook handle that identifies this event hook instance, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HHOOK
NTAPI
NtUserSetWinEventHook(
    _In_ ULONG EventMin,
    _In_ ULONG EventMax,
    _In_opt_ HMODULE ModuleHandle,
    _In_ ULONG_PTR Callback,
    _In_ LONG_PTR WinEventProc,
    _In_ ULONG ProcessId,
    _In_ ULONG ThreadId,
    _In_ LONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUnhookWindowsHook routine removes a Windows hook installed with the specified hook procedure.
 *
 * \param FilterType The filter type.
 * \param FilterProc The filter proc.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_UNHOOKWINDOWSHOOK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnhookWindowsHook(
    _In_ LONG FilterType,
    _In_ HOOKPROC FilterProc
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserUnhookWindowsHookEx routine removes a hook procedure installed in a hook chain by NtUserSetWindowsHookEx.
 *
 * \param HookHandle A handle to the hook to be removed.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnhookWindowsHookEx(
    _In_ HHOOK HookHandle
    );

// rev
/**
 * The NtUserUnregisterUserApiHook routine unregisters user-mode API hook callbacks within win32k.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserUnregisterUserApiHook(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The UnregisterMessagePumpHook routine unregisters a previously registered message pump hook.
 *
 * \return TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
UnregisterMessagePumpHook(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The UnregisterUserApiHook routine removes an installed user-mode API hook module.
 *
 * \return BOOL TRUE if successfully unregistered, FALSE otherwise.
 * \remarks Forwards to the NtUserUnregisterUserApiHook system call.
 */
NTSYSAPI
BOOL
NTAPI
UnregisterUserApiHook(
    VOID
    );

//
// Timers
//

// rev
/**
 * The NtUserValidateTimerCallback routine validates whether an application timer callback procedure pointer is safe and registered.
 *
 * \param TimerProc The timer callback function pointer to validate.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserValidateTimerCallback(
    _In_ LONG_PTR TimerProc
    );

// rev
/**
 * The NtUserSetSystemTimer routine creates or updates a system-level timer with a specified timeout value.
 *
 * \param WindowHandle A handle to the window associated with the timer.
 * \param TimerId A nonzero timer identifier.
 * \param Timeout The time-out value, in milliseconds.
 * \return ULONG_PTR If successful, an integer identifying the new timer; otherwise 0.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserSetSystemTimer(
    _In_ HWND WindowHandle,
    _In_ LONG TimerId,
    _In_ LONG Timeout
    );

/**
 * The NtUserSetTimer routine creates a timer with the specified time-out value and optional tolerance delay.
 *
 * \param WindowHandle A handle to the window associated with the timer.
 * \param IDEvent A nonzero timer identifier.
 * \param Elapse The time-out value, in milliseconds.
 * \param TimerFunc Pointer to the function to be notified when the time-out value elapses.
 * \param ToleranceDelay The tolerance delay in milliseconds.
 * \return If successful, an integer identifying the new timer; otherwise 0.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserSetTimer(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG_PTR IDEvent,
    _In_ ULONG Elapse,
    _In_opt_ TIMERPROC TimerFunc,
    _In_ ULONG ToleranceDelay
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserKillSystemTimer routine kills a system timer on the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param IDEvent The id event.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_KILLSYSTEMTIMER) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserKillSystemTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR IDEvent
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserKillTimer routine destroys the specified timer.
 *
 * \param WindowHandle A handle to the window associated with the specified timer.
 * \param IDEvent The identifier of the timer to be destroyed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserKillTimer(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG_PTR IDEvent
    );

// rev
/**
 * The ABI_Get_currentMonitorTopologyId routine reads the current monitor topology identifier from the live server information mapping.
 *
 * \param ServerInfo Pointer to the SERVERINFO mapping supplied by the running system.
 * \return The current monitor topology identifier.
 */
NTSYSAPI
ULONG
NTAPI
ABI_Get_currentMonitorTopologyId(
    _In_ PVOID ServerInfo // PSERVERINFO
    );

// rev
/**
 * The ABI_Get_ForegroundWindow routine reads the foreground window handle from the internal information mapping.
 *
 * \param ServerInfo Pointer to the SERVERINFO mapping supplied by the running system.
 * \return A handle to the current foreground window.
 */
NTSYSAPI
HWND
NTAPI
ABI_Get_ForegroundWindow(
    _In_ PVOID ServerInfo // PSERVERINFO
    );

// rev
/**
 * The DwmGetRemoteSessionOcclusionEvent routine retrieves the event signaled when remote-session occlusion changes.
 *
 * \param MonitorHandle Handle to the monitor to query.
 * \param Event Pointer that receives the occlusion event handle.
 * \return HANDLE value.
 * \remarks Forwards to the NtUserDwmGetRemoteSessionOcclusionEvent system call.
 */
NTSYSAPI
HANDLE
NTAPI
DwmGetRemoteSessionOcclusionEvent(
    _In_ HMONITOR MonitorHandle,
    _Out_ PVOID Event
    );

// rev
/**
 * The DwmGetRemoteSessionOcclusionState routine retrieves the current remote-session occlusion state.
 *
 * \param MonitorHandle Handle to the monitor to query.
 * \param OcclusionState Pointer that receives the occlusion state.
 * \return LONG value.
 * \remarks Forwards to the NtUserDwmGetRemoteSessionOcclusionState system call.
 */
NTSYSAPI
LONG
NTAPI
DwmGetRemoteSessionOcclusionState(
    VOID
    );

// rev
/**
 * The GetDpiForMonitorInternal routine retrieves the DPI of a monitor.
 *
 * \param MonitorHandle Handle to the monitor to query.
 * \param DpiType Type of DPI awareness to query (MDT_*).
 * \param DpiX Pointer receiving the horizontal DPI value.
 * \param DpiY Pointer receiving the vertical DPI value.
 * \return ULONG_PTR Status code or HRESULT.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetDpiForMonitorInternal(
    _In_ HMONITOR MonitorHandle,
    _In_ ULONG DpiType,
    _Out_ PULONG DpiX,
    _Out_ PULONG DpiY
    );

// rev
/**
 * The GetNumberOfPhysicalMonitors routine retrieves the number of physical monitors associated with an HMONITOR.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetNumberOfPhysicalMonitors(
    VOID
    );

// rev
/**
 * The GetPhysicalMonitorDescription routine retrieves the physical monitor description string.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetPhysicalMonitorDescription(
    VOID
    );

// rev
/**
 * The GetPhysicalMonitors routine enumerates physical monitors associated with an HMONITOR.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetPhysicalMonitors(
    VOID
    );

// rev
/**
 * The IsWindowDisplayChangeSuppressed routine determines whether display-change messages are suppressed for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserIsWindowDisplayChangeSuppressed system call.
 */
NTSYSAPI
LOGICAL
NTAPI
IsWindowDisplayChangeSuppressed(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtGdiGetNumberOfPhysicalMonitors routine retrieves the number of physical display monitors associated with an HMONITOR handle.
 *
 * \param MonitorHandle A handle to the display monitor.
 * \param NumberOfPhysicalMonitors An output pointer that receives the number of physical monitors.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetNumberOfPhysicalMonitors(
    _In_ HMONITOR MonitorHandle,
    _Out_ PULONG NumberOfPhysicalMonitors
    );

// rev
/**
 * The NtGdiGetPhysicalMonitorDescription routine retrieves a physical monitor's manufacturer and model description string.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param DescriptionSize The size, in bytes, of the Description buffer.
 * \param Description A pointer to a buffer that receives the description string.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetPhysicalMonitorDescription(
    _In_ HANDLE PhysicalMonitor,
    _In_ LONG DescriptionSize,
    _Out_writes_bytes_(DescriptionSize) PVOID Description
    );

// rev
/**
 * The NtGdiGetPhysicalMonitorFromTarget routine retrieves a physical monitor handle from an adapter LUID and video present target ID.
 *
 * \param AdapterId The adapter identifier.
 * \param TargetId The video present target identifier.
 * \param PhysicalMonitor An output pointer receiving the physical monitor handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetPhysicalMonitorFromTarget(
    _In_ LUID AdapterId,
    _In_ ULONG TargetId,
    _Out_ PVOID *PhysicalMonitor
    );

// rev
/**
 * The NtGdiGetPhysicalMonitors routine retrieves an array of physical monitor handles associated with an HMONITOR handle.
 *
 * \param MonitorHandle A handle to the display monitor.
 * \param ArraySize The number of elements in the PhysicalMonitorArray buffer.
 * \param PhysicalMonitorArray A pointer to an array of PHYSICAL_MONITOR structures.
 * \param PhysicalMonitorHandles Receives an array of 64-bit physical monitor handles.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetPhysicalMonitors(
    _In_ HMONITOR MonitorHandle,
    _In_ LONG ArraySize,
    _Out_ PVOID PhysicalMonitorArray,
    _Out_ PVOID PhysicalMonitorHandles
    );

// rev
/**
 * The NtUserDisplayConfigGetDeviceInfo routine retrieves display configuration device information.
 *
 * \param DeviceInfo Pointer to a DISPLAYCONFIG_DEVICE_INFO_HEADER structure describing the requested query and receiving output data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserDisplayConfigGetDeviceInfo(
    _Inout_ PVOID DeviceInfo
    );

// rev
/**
 * Enumerates display adapters or monitors in the current session.
 * \param DeviceName Optional adapter name; NULL enumerates adapters.
 * \param DeviceIndex Zero-based device index.
 * \param DisplayDevice Unicode device information. Initialize cb to sizeof(DISPLAY_DEVICEW).
 * \param Flags Device enumeration flags (EDD_*).
 * \return NTSTATUS, not a Win32 BOOL.
 * \remarks The inspected implementation limits the output to 840 bytes and updates cb
 * according to the fields returned.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserEnumDisplayDevices(
    _In_opt_ PCUNICODE_STRING DeviceName,
    _In_ ULONG DeviceIndex,
    _Inout_ PDISPLAY_DEVICEW DisplayDevice,
    _In_ ULONG Flags
    );

/**
 * The NtUserEnumDisplayMonitors routine enumerates display monitors that intersect a clipping region or device context.
 *
 * \param hdc A handle to a display device context, or NULL.
 * \param lprcClip Pointer to a RECT structure specifying a clipping rectangle, or NULL.
 * \param lpfnEnum Pointer to an application-defined callback function.
 * \param dwData Application-defined data that NtUserEnumDisplayMonitors passes to the callback function.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEnumDisplayMonitors(
    _In_opt_ HDC hdc,
    _In_opt_ LPCRECT lprcClip,
    _In_ MONITORENUMPROC lpfnEnum,
    _In_ LPARAM dwData
    );

// rev
/**
 * The NtUserEnumDisplaySettings routine retrieves information about one of the graphics modes for a display device.
 *
 * \param DeviceName Pointer to a UNICODE_STRING containing the display device name, or NULL.
 * \param ModeNum Graphics mode index (e.g. ENUM_CURRENT_SETTINGS, ENUM_REGISTRY_SETTINGS).
 * \param DevMode Pointer to a DEVMODE structure receiving graphics mode information.
 * \param Flags Settings query flags (EDS_*).
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserEnumDisplaySettings(
    _In_ PVOID DeviceName,
    _In_ ULONG ModeNum,
    _Inout_ PVOID DevMode,
    _In_ LONG Flags
    );

/**
 * The NtUserGetDisplayAutoRotationPreferences routine retrieves the display auto-rotation preferences for the current process.
 *
 * \param pOrientation Pointer to an ORIENTATION_PREFERENCE variable that receives the preference.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetDisplayAutoRotationPreferences(
    _Out_ ORIENTATION_PREFERENCE* pOrientation
    );

// rev
/**
 * The NtUserGetDisplayAutoRotationPreferencesByProcessId routine retrieves screen auto-rotation preferences set by a process.
 *
 * \param ProcessId Unique identifier of the process whose preferences are queried.
 * \param Preferences Pointer to a variable receiving orientation preference flags (ORIENTATION_PREFERENCE).
 * \param Param3 Pointer to a variable receiving additional rotation control flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetDisplayAutoRotationPreferencesByProcessId(
    _In_ ULONG ProcessId,
    _Out_ PULONG Preferences,
    _Out_ PULONG Param3
    );

// rev
/**
 * The NtUserGetDisplayConfigBufferSizes routine retrieves the size of the buffers required by QueryDisplayConfig.
 *
 * \param Flags Query type flags (QDC_ALL_PATHS, QDC_ONLY_ACTIVE_PATHS, QDC_DATABASE_CURRENT).
 * \param NumElements Pointer to a variable receiving the number of path elements required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserGetDisplayConfigBufferSizes(
    _In_ ULONG Flags,
    _Out_ PULONG NumElements
    );

// rev
/**
 * The NtUserGetDpiForMonitor routine queries the dots per inch (DPI) of a display monitor.
 *
 * \param MonitorHandle Handle to the monitor to query.
 * \param DpiType Monitor DPI query type (MDT_EFFECTIVE_DPI, MDT_ANGULAR_DPI, MDT_RAW_DPI).
 * \param DpiX Pointer to a variable receiving the horizontal DPI.
 * \param DpiY Pointer to a variable receiving the vertical DPI.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetDpiForMonitor(
    _In_ HMONITOR MonitorHandle,
    _In_ LONG DpiType,
    _Out_ PULONG DpiX,
    _Out_ PULONG DpiY
    );

// rev
/**
 * The NtUserGetOwnerTransformedMonitorRect routine retrieves the monitor bounding rectangle transformed into the coordinate space of the window's owner.
 *
 * \param WindowHandle Handle to the window whose owner coordinates are used.
 * \param MonitorHandle Handle to the display monitor to query.
 * \param Flags Transformation or adjustment flags.
 * \param Rect Pointer to a RECT structure receiving the transformed monitor rectangle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetOwnerTransformedMonitorRect(
    _In_ HWND WindowHandle,
    _In_ HMONITOR MonitorHandle,
    _In_ LONG Flags,
    _Out_ PRECT Rect
    );

// rev
/**
 * The NtUserGetWindowDisplayAffinity routine retrieves the current display affinity setting of any window.
 *
 * \param HWnd Handle to the window to query.
 * \param PdwAffinity Pointer to a variable receiving the display affinity flags (e.g. WDA_NONE, WDA_MONITOR, WDA_EXCLUDEFROMCAPTURE).
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetWindowDisplayAffinity.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetWindowDisplayAffinity(
    _In_ HWND HWnd,
    _Out_ ULONG* PdwAffinity
    );

// rev
/**
 * The NtUserIsWindowDisplayChangeSuppressed routine determines whether display change messages are suppressed for the window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsWindowDisplayChangeSuppressed(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserQueryDisplayConfig routine queries information about the display paths and modes for the current desktop.
 *
 * \param Flags Query type flags (QDC_ALL_PATHS, QDC_ONLY_ACTIVE_PATHS, QDC_DATABASE_CURRENT).
 * \param PathElementCount Pointer to a variable holding the number of elements in PathArray and receiving the returned count.
 * \param PathArray Pointer to an array of DISPLAYCONFIG_PATH_INFO structures.
 * \param ModeElementCount Pointer to a variable holding the number of elements in ModeArray and receiving the returned count.
 * \param ModeArray Pointer to an array of DISPLAYCONFIG_MODE_INFO structures.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserQueryDisplayConfig(
    _In_ ULONG Flags,
    _Inout_ PULONG PathElementCount,
    _Out_ PVOID PathArray,
    _Inout_ PULONG ModeElementCount,
    _Out_ PVOID ModeArray
    );

// rev
/**
 * The DisplayExitWindowsWarnings routine displays the warnings shown before exiting Windows.
 *
 * \param ExitWindowsFlags Flags specifying shutdown/exit options (EWX_*).
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DisplayExitWindowsWarnings(
    _In_ ULONG ExitWindowsFlags
    );

// rev
/**
 * The InitializeInputDeviceInjection routine initializes synthetic input device injection with custom usage pages.
 *
 * \param Page HID usage page identifier.
 * \param CaUsage HID collection usage identifier.
 * \param Usages Pointer to an array of USAGE_PROPERTIES structures.
 * \param UsageCount Number of usage property elements in Usages.
 * \param Monitor Handle to the target display monitor for coordinate mapping.
 * \param VisualMode Input injection visualization mode.
 * \param DeviceHandle Pointer to a variable receiving the created injection device handle.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInitializeInputDeviceInjection system call.
 */
NTSYSAPI
BOOL
NTAPI
InitializeInputDeviceInjection(
    _In_ USHORT Page,
    _In_ USHORT CaUsage,
    _In_reads_(UsageCount) const PUSAGE_PROPERTIES Usages,
    _In_ ULONG UsageCount,
    _In_ HMONITOR Monitor,
    _In_ ULONG VisualMode,
    _Out_writes_(1) HANDLE* DeviceHandle
    );

// rev
/**
 * The NtUserChangeDisplaySettings routine changes the settings of the default display device to the specified graphics mode.
 *
 * \param Param1 Pointer to a UNICODE_STRING containing the display device name, or NULL.
 * \param Param2 Pointer to a DEVMODE structure describing the new graphics mode, or NULL.
 * \param Param3 Display change flags (CDS_*).
 * \param Param4 Pointer to optional video parameters or caller-supplied context.
 * \return LONG DISP_CHANGE_SUCCESSFUL on success; otherwise one of the DISP_CHANGE_* codes.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserChangeDisplaySettings(
    _Inout_ PVOID Param1,
    _Inout_ PVOID Param2,
    _In_ ULONG Param3,
    _Inout_ PVOID Param4
    );

// rev
/**
 * The NtUserCtxDisplayIOCtl routine performs an I/O control operation on the Citrix or remote desktop display driver.
 *
 * \param IoControlCode Device I/O control code.
 * \param Buffer Pointer to the input/output buffer for the control operation.
 * \param BufferLength Size, in bytes, of the buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCtxDisplayIOCtl(
    _In_ ULONG IoControlCode,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

// rev
/**
 * The NtUserDisplayConfigSetDeviceInfo routine sets display configuration device information.
 *
 * \param DeviceInfo Pointer to a DISPLAYCONFIG_DEVICE_INFO_HEADER structure containing configuration parameters to set.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserDisplayConfigSetDeviceInfo(
    _In_ PVOID DeviceInfo
    );

// rev
/**
 * Functionalizes an internal display-path configuration.
 * \param Flags Internal flags. Only bits 0x1, 0x2 and 0x4 are accepted in the inspected implementation.
 * \param PathCount Input array capacity, from 1 through 1024 records, and returned path count.
 * \param Paths Array of 216-byte DISPLAYCONFIG_PATH_INFO_INTERNAL records, not public DISPLAYCONFIG_PATH_INFO.
 * \param DisplayState Optional 28-byte display-state snapshot used to detect stale configuration.
 * \param BrokerHandle Optional handle used to resolve broker-provided display paths.
 * \param Result Receives the first ULONG of the internal functionalization result.
 * \return NTSTATUS. STATUS_BUFFER_OVERFLOW is translated to STATUS_INFO_LENGTH_MISMATCH.
 * \remarks The internal path and display-state layouts remain opaque. Each input path must
 * have bit 63 of its first 64-bit word set. Flag 0x1 requires at least two records and treats
 * the first as a control record; flag 0x2 is rejected without flag 0x1.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserFunctionalizeDisplayConfig(
    _In_ ULONG Flags,
    _Inout_ PULONG PathCount,
    _Inout_updates_bytes_(*PathCount * 216) PVOID Paths,
    _In_reads_bytes_opt_(28) const VOID *DisplayState,
    _In_opt_ HANDLE BrokerHandle,
    _Out_ PULONG Result
    );

// rev
/**
 * The NtUserInheritWindowMonitor routine configures a window to inherit monitor association from another window.
 *
 * \param Hwnd Handle to the target window inheriting monitor association.
 * \param HwndInherit Optional handle to the window whose monitor is inherited, or NULL to reset.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InheritWindowMonitor.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInheritWindowMonitor(
    _In_ HWND Hwnd,
    _In_opt_ HWND HwndInherit
    );

// rev
/**
 * The NtUserInitializeInputDeviceInjection routine initializes synthetic input device injection with custom usage pages.
 *
 * \param Page HID usage page identifier.
 * \param CaUsage HID collection usage identifier.
 * \param Usages Pointer to an array of USAGE_PROPERTIES structures.
 * \param CUsages Number of usage property elements in Usages.
 * \param Monitor Handle to the target display monitor for coordinate mapping.
 * \param VisualMode Input injection visualization mode.
 * \param Device Pointer to a variable receiving the created injection device handle.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InitializeInputDeviceInjection.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInitializeInputDeviceInjection(
    _In_ USHORT Page,
    _In_ USHORT CaUsage,
    _In_reads_(CUsages) const PUSAGE_PROPERTIES Usages,
    _In_ ULONG CUsages,
    _In_ HMONITOR Monitor,
    _In_ ULONG VisualMode,
    _Out_writes_(1) HANDLE* Device
    );

// rev
/**
 * The NtUserLogicalToPerMonitorDPIPhysicalPoint routine converts a point in a window from logical coordinates into physical coordinates.
 *
 * \param HWnd Optional handle to the window whose coordinate space is used.
 * \param LpPoint Pointer to a POINT structure specifying the logical coordinates to convert and receiving the physical coordinates.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!LogicalToPhysicalPointForPerMonitorDPI.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserLogicalToPerMonitorDPIPhysicalPoint(
    _In_opt_ HWND HWnd,
    _Inout_ LPPOINT LpPoint
    );

// rev
/**
 * The NtUserPaintMonitor routine paints monitor background contents into a display device context.
 *
 * \param MonitorHandle Handle to the display monitor to paint.
 * \param Param2 Additional monitor painting parameter or DC handle.
 * \param Rect Pointer to a RECT structure defining the bounding update rectangle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPaintMonitor(
    _In_ HMONITOR MonitorHandle,
    _In_ LONG_PTR Param2,
    _In_ PRECT Rect
    );

// rev
/**
 * The NtUserPerMonitorDPIPhysicalToLogicalPoint routine converts a point in a window from physical coordinates into logical coordinates.
 *
 * \param HWnd Optional handle to the window whose coordinate space is used.
 * \param LpPoint Pointer to a POINT structure specifying the physical coordinates to convert and receiving the logical coordinates.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!PhysicalToLogicalPointForPerMonitorDPI.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserPerMonitorDPIPhysicalToLogicalPoint(
    _In_opt_ HWND HWnd,
    _Inout_ LPPOINT LpPoint
    );

// rev
/**
 * The NtUserSetActiveProcessForMonitor routine associates an active foreground process with a specified display monitor.
 *
 * \param ProcessId The process identifier to associate.
 * \param MonitorHandle A handle to the display monitor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetActiveProcessForMonitor(
    _In_ ULONG ProcessId,
    _In_ HMONITOR MonitorHandle
    );

// rev
/**
 * The NtUserSetDisplayAutoRotationPreferences routine sets the screen orientation preferences for the calling process.
 *
 * \param Orientation The orientation preferences to set for the process.
 * \return TRUE if the function succeeds, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetDisplayAutoRotationPreferences.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetDisplayAutoRotationPreferences(
    _In_ ORIENTATION_PREFERENCE Orientation
    );

// rev
/**
 * The NtUserSetDisplayConfig routine modifies the display topology, source modes, and target modes for display adapters.
 *
 * \param PathArrayElements The number of elements in the path information array.
 * \param PathArray A pointer to an array of display path structures.
 * \param ModeArrayElements The number of elements in the mode information array.
 * \param ModeArray A pointer to an array of display mode information structures.
 * \param Flags Flags specifying the display configuration operations to apply.
 * \return ERROR_SUCCESS if successful, or a Win32 error code on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetDisplayConfig(
    _In_ ULONG PathArrayElements,
    _In_ PVOID PathArray,
    _In_ ULONG ModeArrayElements,
    _In_ LONG_PTR ModeArray,
    _In_ LONG_PTR Flags
    );

// rev
/**
 * The NtUserSetDisplayMapping routine configures the display device mapping between physical display adapters and monitor handles.
 *
 * \param DeviceHandle A handle to the display device adapter.
 * \param MonitorHandle A handle to the target display monitor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDisplayMapping(
    _In_ HANDLE DeviceHandle,
    _In_ HMONITOR MonitorHandle
    );

// rev
/**
 * The NtUserSetMonitorWorkArea routine configures the usable work area coordinates for the specified display monitor.
 *
 * \param MonitorHandle A handle to the display monitor.
 * \param WorkArea A pointer to a RECT structure specifying the new work area coordinates.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetMonitorWorkArea(
    _In_ HMONITOR MonitorHandle,
    _In_ PVOID WorkArea
    );

// rev
/**
 * The NtUserSetWindowDisplayAffinity routine specifies where the window's contents can be displayed or captured.
 *
 * \param HWnd A handle to the window to configure.
 * \param DwAffinity The display affinity flag (e.g. WDA_NONE, WDA_MONITOR, WDA_EXCLUDEFROMCAPTURE).
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!SetWindowDisplayAffinity.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowDisplayAffinity(
    _In_ HWND HWnd,
    _In_ ULONG DwAffinity
    );

// rev
/**
 * The NtUserSuppressWindowDisplayChange routine suppresses display change message broadcasting and processing for a window hierarchy.
 *
 * \param WindowHandle A handle to the window to configure.
 * \param Suppress Nonzero to suppress display change messages; zero to allow them.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSuppressWindowDisplayChange(
    _In_ HWND WindowHandle,
    _In_ LONG Suppress
    );

// rev
/**
 * The NtUserTransformPoint routine transforms point coordinates across differing DPI contexts or monitor spaces.
 *
 * \param Point An in-out pointer to a POINT structure containing coordinates to transform.
 * \param FromDpiContext The source DPI context.
 * \param ToDpiContext The target DPI context.
 * \param MonitorHandle An optional handle to the display monitor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserTransformPoint(
    _Inout_ PPOINT Point,
    _In_ ULONG FromDpiContext,
    _In_ ULONG ToDpiContext,
    _In_opt_ HMONITOR MonitorHandle
    );

// rev
/**
 * The NtUserTransformRect routine transforms rectangle bounds across differing DPI contexts or monitor spaces.
 *
 * \param Rect An in-out pointer to a RECT structure containing rectangle bounds to transform.
 * \param FromDpiContext The source DPI context.
 * \param ToDpiContext The target DPI context.
 * \param MonitorHandle An optional handle to the display monitor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserTransformRect(
    _Inout_ PRECT Rect,
    _In_ ULONG FromDpiContext,
    _In_ ULONG ToDpiContext,
    _In_opt_ HMONITOR MonitorHandle
    );

// rev
/**
 * The NtUserUpdateInstance routine updates display monitor instance registration and topology state.
 *
 * \param MonitorHandle A handle to the display monitor.
 * \param Result An in-out pointer receiving status results.
 * \param Flags Flags specifying monitor instance update options.
 * \return ULONG 0 on success; otherwise a monitor error code.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserUpdateInstance(
    _In_ HMONITOR MonitorHandle,
    _Inout_ PULONG Result,
    _In_ ULONG Flags
    );

// rev
/**
 * The PaintMonitor routine initiates painting or updating on the specified display monitor.
 *
 * \param MonitorHandle Handle to the display monitor to paint.
 * \param Hdc Handle to the device context used for painting.
 * \param ClipRect Optional pointer to the clipping rectangle bounds.
 * \return BOOL TRUE if successful, FALSE otherwise.
 * \remarks Forwards to the NtUserPaintMonitor system call.
 */
NTSYSAPI
BOOL
NTAPI
PaintMonitor(
    _In_ HMONITOR MonitorHandle,
    _In_opt_ HDC Hdc,
    _In_opt_ PRECT ClipRect
    );

// rev
/**
 * The SuppressWindowDisplayChange routine suppresses or restores display-change messages for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Suppress Non-zero to suppress display change messages; zero to restore.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSuppressWindowDisplayChange system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SuppressWindowDisplayChange(
    _In_ HWND WindowHandle,
    _In_ LONG Suppress
    );

// rev
/**
 * The NtGdiDestroyPhysicalMonitor routine closes and releases a handle to a physical monitor.
 *
 * \param PhysicalMonitor A handle to the physical monitor to close.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDestroyPhysicalMonitor(
    _In_ HANDLE PhysicalMonitor
    );

//
// IME & Input Contexts (HIMC)
//

typedef PVOID LPIMEPROA;
typedef PVOID LPIMEPROW;

// rev
/**
 * The NtUserCreateInputContext routine creates a kernel-mode Input Method Context (HIMC) associated with client IMC data.
 *
 * \param ClientImcData Client-mode pointer or identifier for the input context data.
 * \return HIMC A handle to the new input context, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HIMC
NTAPI
NtUserCreateInputContext(
    _In_ ULONG_PTR ClientImcData
    );

// rev
/**
 * The IMPGetIMEA routine retrieves IME information for a window (ANSI).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImeInfo Pointer receiving the IMEPROA structure.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPGetIMEA(
    _In_ HWND WindowHandle,
    _Out_ LPIMEPROA ImeInfo
    );

// rev
/**
 * The IMPGetIMEW routine retrieves IME information for a window (Unicode).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImeInfo Pointer receiving the IMEPROW structure.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPGetIMEW(
    _In_ HWND WindowHandle,
    _Out_ LPIMEPROW ImeInfo
    );

// rev
/**
 * The IMPQueryIMEA routine queries IME properties (ANSI).
 *
 * \param ImePro Pointer to the IMEPROA structure to populate.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPQueryIMEA(
    _Inout_ LPIMEPROA ImePro
    );

// rev
/**
 * The IMPQueryIMEW routine queries IME properties (Unicode).
 *
 * \param ImePro Pointer to the IMEPROW structure to populate.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPQueryIMEW(
    _Inout_ LPIMEPROW ImePro
    );

// rev
/**
 * The NtUserBuildHimcList routine retrieves the list of Input Method Context (HIMC) handles for a thread.
 *
 * \param ThreadId Thread whose input contexts are enumerated (0 for all threads).
 * \param HimcListInformationLength Size, in bytes, of the HimcListInformation buffer.
 * \param HimcListInformation Buffer that receives the array of HIMC handles.
 * \param ReturnLength Receives the number of bytes written or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildHimcList(
    _In_ ULONG ThreadId,
    _In_ ULONG HimcListInformationLength,
    _Out_writes_bytes_(HimcListInformationLength) PVOID HimcListInformation,
    _Out_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCheckImeShowStatusInThread routine checks the IME show status in the calling thread.
 *
 * \param Ime Pointer to a variable receiving the thread IME show status.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock[Safe](SFI_CHECKIMESHOWSTATUSINTHREAD) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCheckImeShowStatusInThread(
    _In_ HWND Ime
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetAppImeLevel routine retrieves the IME compatibility level for the specified window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return ULONG The IME level for the window, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetAppImeLevel(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetIMEShowStatus routine retrieves the global IME show-status flag.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETIMESHOWSTATUS) before WIN11.
 */
_Success_(return != 0)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetIMEShowStatus(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetImeHotKey routine retrieves the hot key configuration for an Input Method Editor (IME).
 *
 * \param HotKeyId Hot key identifier (IME_JHOTKEY_*).
 * \param Modifiers Pointer to a variable receiving hot key modifier flags (e.g. MOD_ALT, MOD_CONTROL).
 * \param VirtualKey Pointer to a variable receiving the virtual-key code.
 * \param KeyboardLayout Optional pointer to a variable receiving the associated keyboard layout handle (HKL).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetImeHotKey(
    _In_ ULONG HotKeyId,
    _Out_ PULONG Modifiers,
    _Out_ PULONG VirtualKey,
    _Out_opt_ PVOID KeyboardLayout
    );

// rev
/**
 * The NtUserGetImeInfoEx routine retrieves extended information about an Input Method Editor (IME).
 *
 * \param ImeInfoEx Pointer to an IMEINFOEX structure specifying the search criteria and receiving the output information.
 * \param SearchType Search type specifier (e.g. ImeSearchByHKL, ImeSearchByName).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetImeInfoEx(
    _Inout_ PVOID ImeInfoEx,
    _In_ ULONG InfoType
    );

// rev
/**
 * The NtUserQueryInputContext routine queries internal attributes of an Input Method Context (HIMC).
 *
 * \param InputContext Handle to the input context (HIMC) to query.
 * \param InfoType Information class or property to query.
 * \return ULONG_PTR The queried input context attribute or value.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserQueryInputContext(
    _In_ HIMC InputContext,
    _In_ LONG InfoType
    );

// rev
/**
 * The IMPSetIMEA routine sets IME properties for a window (ANSI).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImePro Pointer to the IMEPROA structure containing properties.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPSetIMEA(
    _In_ HWND WindowHandle,
    _In_ LPIMEPROA ImePro
    );

// rev
/**
 * The IMPSetIMEW routine sets IME properties for a window (Unicode).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImePro Pointer to the IMEPROW structure containing properties.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IMPSetIMEW(
    _In_ HWND WindowHandle,
    _In_ LPIMEPROW ImePro
    );

// rev
/**
 * The NtUserAssociateInputContext routine associates an input context (HIMC) with the specified window.
 *
 * \param WindowHandle Handle to the window to receive the input context association.
 * \param InputContext Optional handle to the input context (HIMC) to associate, or NULL to disassociate.
 * \param Flags Association control flags (e.g. IAP_ALLWINDOWS).
 * \return ULONG One of the IACE_* association status codes.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserAssociateInputContext(
    _In_ HWND WindowHandle,
    _In_opt_ HIMC InputContext,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserBroadcastImeShowStatusChange routine broadcasts an IME show-status change.
 *
 * \param Ime Pointer to a variable receiving the thread IME show status.
 * \param Show Visibility state or show command flag.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_BROADCASTIMESHOWSTATUSCHANGE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserBroadcastImeShowStatusChange(
    _In_ HWND Ime,
    _In_ LOGICAL Show
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDisableThreadIme routine disables the Input Method Editor (IME) for the specified thread.
 *
 * \param ThreadId Unique identifier of the target thread, or 0 for the calling thread.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDisableThreadIme(
    _In_ ULONG ThreadId
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserNlsKbdSendIMENotification routine sends an NLS keyboard IME notification.
 *
 * \param ImeOpen The ime open.
 * \param ImeConversion The ime conversion.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_NLSKBDSENDIMENOTIFICATION) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNlsKbdSendIMENotification(
    _In_ ULONG ImeOpen,
    _In_ ULONG ImeConversion
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserNotifyIMEStatus routine notifies win32k of changes in Input Method Editor (IME) status or mode.
 *
 * \param WindowHandle Handle to the window associated with the IME.
 * \param Status IME status code or open mode.
 * \param Param3 Additional IME conversion mode flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNotifyIMEStatus(
    _In_ HWND WindowHandle,
    _In_ ULONG Status,
    _In_ ULONG Param3
    );

// rev
/**
 * The NtUserSetAppImeLevel routine sets the Input Method Editor (IME) interaction level for the specified application window.
 *
 * \param WindowHandle A handle to the window configuring IME level.
 * \param Level The IME interaction level.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetAppImeLevel(
    _In_ HWND WindowHandle,
    _In_ LONG Level
    );

// rev
/**
 * The NtUserSetImeHotKey routine sets an Input Method Editor (IME) hot key and its corresponding modifiers and keyboard layout.
 *
 * \param HotKeyId The hot key identifier to set.
 * \param Modifiers Modifier key combination flags (e.g. MOD_ALT, MOD_CONTROL, MOD_SHIFT).
 * \param VirtualKey The virtual-key code of the hot key.
 * \param KeyboardLayout The input locale identifier (HKL) for which the hot key applies.
 * \param Flags Additional IME hot key configuration flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetImeHotKey(
    _In_ ULONG HotKeyId,
    _In_ ULONG Modifiers,
    _In_ ULONG VirtualKey,
    _In_ HKL KeyboardLayout,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserSetImeInfoEx routine registers extended Input Method Editor (IME) information and layout associations.
 *
 * \param ImeInfoEx A pointer to an IMEINFOEX structure containing extended IME data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetImeInfoEx(
    _In_ PVOID ImeInfoEx
    );

// rev
/**
 * The NtUserSetImeOwnerWindow routine assigns an owner window for the default or active IME composition window.
 *
 * \param WindowHandle A handle to the window utilizing IME input.
 * \param OwnerWindowHandle An optional handle to the IME owner window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetImeOwnerWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND OwnerWindowHandle
    );

// rev
/**
 * The NtUserUpdateInputContext routine updates the state, status, or properties of an active Input Method Context (IMC).
 *
 * \param InputContext A handle to the input context.
 * \param InfoType The information classification type being updated.
 * \param Value The updated value or pointer to configuration data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateInputContext(
    _In_ HIMC InputContext,
    _In_ ULONG InfoType,
    _In_ LONG_PTR Value
    );

// rev
/**
 * The SendIMEMessageExA routine sends an IME message to a window (ANSI, legacy).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImeStruct Pointer or value passed as IME message parameter.
 * \return LRESULT Message processing result.
 */
NTSYSAPI
LRESULT
NTAPI
SendIMEMessageExA(
    _In_ HWND WindowHandle,
    _In_ LPARAM ImeStruct
    );

// rev
/**
 * The SendIMEMessageExW routine sends an IME message to a window (Unicode, legacy).
 *
 * \param WindowHandle Handle to the target window.
 * \param ImeStruct Pointer or value passed as IME message parameter.
 * \return LRESULT Message processing result.
 */
NTSYSAPI
LRESULT
NTAPI
SendIMEMessageExW(
    _In_ HWND WindowHandle,
    _In_ LPARAM ImeStruct
    );

// rev
/**
 * The WINNLSEnableIME routine enables or disables the IME for a window.
 *
 * \param WindowHandle Handle to the window.
 * \param Enable TRUE to enable the IME; FALSE to disable.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
WINNLSEnableIME(
    _In_ HWND WindowHandle,
    _In_ BOOL Enable
    );

// rev
/**
 * The NtUserDestroyInputContext routine destroys an Input Method Context (HIMC) and frees its resources.
 *
 * \param InputContext Handle to the input context (HIMC) to destroy.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyInputContext(
    _In_ HIMC InputContext
    );

//
// Messages & Message Queue
//

/**
 * Flags for NtUserRegisterSystemThread (RST_*).
 */
#define RST_DONTATTACHQUEUE     0x00000001
#define RST_DONTJOURNALATTACH   0x00000002

// rev
/**
 * Flags controlling input-queue merging for NtUserSetThreadQueueMergeSetting.
 */
typedef enum _THREAD_QUEUE_MERGE_FLAGS
{
    THREAD_QUEUE_MERGE_DEFAULT = 0x00000000, ///< Effect: Clears the thread's input-queue merge prohibition.
    THREAD_QUEUE_MERGE_DISABLE = 0x00000001  ///< Effect: Prohibits input-queue merging for the target thread.
} THREAD_QUEUE_MERGE_FLAGS;

// rev
/**
 * The IsThreadMessageQueueAttached routine determines whether the calling thread's message queue is attached.
 *
 * \param ThreadId Optional thread identifier to test (0 for current thread).
 * \return LOGICAL Non-zero if attached, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserThreadMessageQueueAttached.
 */
NTSYSAPI
LOGICAL
NTAPI
IsThreadMessageQueueAttached(
    _In_ _In_opt_ ULONG ThreadId
    );

// rev
/**
 * The NtUserRegisterWindowMessage routine defines a new window message that is guaranteed to be unique throughout the system.
 *
 * \param MessageName A pointer to a UNICODE_STRING specifying the message string to register.
 * \return ULONG The registered message atom, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserRegisterWindowMessage(
    _In_ PUNICODE_STRING MessageName
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetWaitForQueueAttach routine sets whether the calling thread waits for queue attach.
 *
 * \param Waiting TRUE to wait for queue attachment; FALSE otherwise.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETWAITFORQUEUEATTACH) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWaitForQueueAttach(
    _In_ LOGICAL Waiting
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserThreadMessageQueueAttached routine indicates whether the specified thread's message queue is attached.
 *
 * \param ThreadId The thread id.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_THREADMESSAGEQUEUEATTACHED) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserThreadMessageQueueAttached(
    _In_opt_ ULONG ThreadId
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The GdiGetSpoolMessage routine retrieves a print spooler message.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiGetSpoolMessage(
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The GetSendMessageReceiver routine retrieves the window handle receiving a sent message on the specified thread.
 *
 * \param ThreadId The thread identifier.
 * \return The window handle receiving the message, or NULL if none.
 */
NTSYSAPI
HWND
NTAPI
GetSendMessageReceiver(
    _In_ HANDLE ThreadId
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * NtGdiGetSpoolMessage
 *
 * win32k 10.0.26100.9444 dispatch (0x140022f68-0x140022f9d) forwards four
 * slots with widths 64, 32, 64, 32 bits on x64. Semantic types and direction
 * remain unrecovered; pointer-sized slots intentionally remain opaque.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetSpoolMessage(
    );

/**
 * The NtUserGetCurrentInputMessageSource routine retrieves the source of the input message.
 *
 * \param InputMessageSource Pointer to an INPUT_MESSAGE_SOURCE structure that receives message source information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCurrentInputMessageSource(
    _Out_ INPUT_MESSAGE_SOURCE* InputMessageSource
    );

// rev
/**
 * The NtUserGetLatestInputMessageData routine retrieves metadata and timestamp information for the most recent input message.
 *
 * \param InputMessageData Pointer to a structure receiving input message details.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetLatestInputMessageData(
    _Inout_ PVOID InputMessageData
    );

// rev
/**
 * The NtUserGetMessage routine retrieves a message from the calling thread's message queue.
 *
 * \param Message Pointer to an MSG structure that receives message information.
 * \param WindowHandle Optional handle to the window whose messages are to be retrieved, or NULL for all thread windows.
 * \param MsgFilterMin The integer value of the lowest message value to be retrieved.
 * \param MsgFilterMax The integer value of the highest message value to be retrieved.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetMessage(
    _Out_ PMSG Message,
    _In_opt_ HWND WindowHandle,
    _In_ LONG MsgFilterMin,
    _In_ LONG MsgFilterMax
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetMessagePos routine retrieves the cursor position for the last message retrieved from the queue.
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETMESSAGEPOS) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetMessagePos(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetQueueIocp routine retrieves the I/O completion port associated with the calling thread's message queue.
 *
 * \return Handle to the queue I/O completion port (tagTHREADINFO::pIocpPort), or NULL on failure.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETQUEUEIOCP) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetQueueIocp(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetQueueStatus routine indicates the type of messages present in the calling thread's message queue.
 *
 * \param Flags Queue status query flags (QS_*).
 * \return ULONG The queue status: pending message types in the high word, newly queued types in the low word.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetQueueStatus(
    _In_ SHORT Flags
    );

// rev
/**
 * The NtUserGetQueueStatusReadonly routine inspects the thread message queue status without clearing message-arrival change flags.
 *
 * \param Flags Queue status query flags (QS_*).
 * \return ULONG The queue status: pending message types in the high word, newly queued types in the low word.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetQueueStatusReadonly(
    _In_ ULONG_PTR Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetSendMessageReceiver routine returns the window receiving the message the specified thread is currently sending.
 *
 * \param ThreadId The thread identifier.
 * \return HWND The window handle receiving the message, or NULL if none.
 * \remarks Exposed via NtUserCallOneParam(SFI_GETSENDMESSAGERECEIVER) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetSendMessageReceiver(
    _In_ HANDLE ThreadId
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetUnpredictedMessagePos routine retrieves the unpredicted (raw) message cursor position.
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETUNPREDICTEDMESSAGEPOS) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetUnpredictedMessagePos(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserIsChildWindowDpiMessageEnabled routine determines whether child window DPI change notifications are enabled for a window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsChildWindowDpiMessageEnabled(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserIsWindowGDIScaledDpiMessageEnabled routine determines whether GDI-scaled DPI messages are enabled for the window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsWindowGDIScaledDpiMessageEnabled(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserRealInternalGetMessage routine performs low-level retrieval of messages from the thread message queue.
 *
 * \param Message Pointer to an MSG structure receiving the message.
 * \param WindowHandle Optional handle to the window whose messages are filtered.
 * \param MsgFilterMin The minimum message value to retrieve.
 * \param MsgFilterMax The maximum message value to retrieve.
 * \param Flags Message retrieval control flags (PM_*).
 * \param Param6 Additional internal filter or dispatch options.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRealInternalGetMessage(
    _Out_ PMSG Message,
    _In_opt_ HWND WindowHandle,
    _In_ LONG_PTR MsgFilterMin,
    _In_ LONG_PTR MsgFilterMax,
    _In_ ULONG_PTR Flags,
    _In_ LONG Param6
    );

// rev
/**
 * The CsrBroadcastSystemMessageExW routine broadcasts a system message through the CSR subsystem.
 *
 * \param Flags Broadcast flags (BSF_*).
 * \param Info In/out pointer specifying recipients and receiving recipient mask.
 * \param Message Message identifier to broadcast.
 * \param wParam Additional message parameter.
 * \param lParam Additional message parameter.
 * \param BsmInfo Optional pointer receiving broadcast information.
 * \return ULONG_PTR Positive value on success, 0 on failure.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CsrBroadcastSystemMessageExW(
    _In_ LONG Flags,
    _Inout_opt_ PULONG Info,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_opt_ PBSMINFO BsmInfo
    );

// rev
/**
 * The MessageBoxTimeoutA routine displays a message box that dismisses automatically after a specified timeout (ANSI).
 *
 * \param WindowHandle Optional handle to the owner window.
 * \param Text Optional pointer to the null-terminated message text.
 * \param Caption Optional pointer to the null-terminated title bar text.
 * \param Type Message box style and buttons (MB_* flags).
 * \param LanguageId Language identifier for message box buttons.
 * \param Timeout Timeout period in milliseconds before auto-dismissal.
 * \return LONG Identifier of the button selected or timeout code (MB_TIMEDOUT).
 */
NTSYSAPI
LONG
NTAPI
MessageBoxTimeoutA(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LPCSTR Text,
    _In_opt_ LPCSTR Caption,
    _In_ ULONG Type,
    _In_ WORD LanguageId,
    _In_ DWORD Timeout
    );

// rev
/**
 * The MessageBoxTimeoutW routine displays a message box that dismisses automatically after a specified timeout (Unicode).
 *
 * \param WindowHandle Optional handle to the owner window.
 * \param Text Optional pointer to the null-terminated message text.
 * \param Caption Optional pointer to the null-terminated title bar text.
 * \param Type Message box style and buttons (MB_* flags).
 * \param LanguageId Language identifier for message box buttons.
 * \param Timeout Timeout period in milliseconds before auto-dismissal.
 * \return LONG Identifier of the button selected or timeout code (MB_TIMEDOUT).
 */
NTSYSAPI
LONG
NTAPI
MessageBoxTimeoutW(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LPCWSTR Text,
    _In_opt_ LPCWSTR Caption,
    _In_ ULONG Type,
    _In_ WORD LanguageId,
    _In_ DWORD Timeout
    );

// rev
/**
 * The NtUserCallMsgFilter routine passes a message and filter code to the WH_MSGFILTER and WH_SYSMSGFILTER hook procedures.
 *
 * \param Message Pointer to an MSG structure containing the message to filter.
 * \param Code Filter hook code identifying the type of message (e.g. MSGF_DIALOGBOX, MSGF_MENU).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCallMsgFilter(
    _Inout_ PMSG Message,
    _In_ LONG Code
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserCancelQueueEventCompletionPacket routine cancels a pending queue event-completion packet for the calling thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_CANCELQUEUEEVENTCOMPLETIONPACKET) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCancelQueueEventCompletionPacket(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserChangeWindowMessageFilter routine changes the window message filter.
 *
 * \param Message The window message identifier.
 * \param Flag Filter action flag (MSGFLT_ADD or MSGFLT_REMOVE).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_CHANGEWINDOWMESSAGEFILTER) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserChangeWindowMessageFilter(
    _In_ ULONG Message,
    _In_ ULONG Flag // MSGFLT_* WinUser.h
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserChangeWindowMessageFilterEx routine modifies the User Interface Privilege Isolation (UIPI) message filter for a specified window.
 *
 * \param Hwnd Handle to the window whose UIPI message filter is being modified.
 * \param Message The message that the filter allows through or blocks.
 * \param Action Action to take (MSGFLT_ALLOW, MSGFLT_DISALLOW, MSGFLT_RESET).
 * \param PChangeFilterStruct Optional pointer to a CHANGEFILTERSTRUCT structure.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!ChangeWindowMessageFilterEx.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserChangeWindowMessageFilterEx(
    _In_ HWND Hwnd,
    _In_ ULONG Message,
    _In_ ULONG Action,
    _Inout_opt_ PCHANGEFILTERSTRUCT PChangeFilterStruct
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserClearWakeMask routine clears the calling thread's input wake mask.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_CLEARWAKEMASK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserClearWakeMask(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDispatchMessage routine dispatches a window message to a window procedure.
 *
 * \param Message Pointer to a CONST MSG structure containing the message to dispatch.
 * \return LRESULT The value returned by the window procedure.
 */
_Kernel_entry_
NTSYSCALLAPI
LRESULT
NTAPI
NtUserDispatchMessage(
    _In_ CONST MSG *Message
    );

// rev
/**
 * The NtUserEnableChildWindowDpiMessage routine enables or disables delivery of DPI change messages to child windows.
 *
 * \param WindowHandle Handle to the parent window.
 * \param Enable Non-zero to enable child DPI messages; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableChildWindowDpiMessage(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserEnableWindowGDIScaledDpiMessage routine enables or disables delivery of GDI-scaled DPI messages to a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable Non-zero to enable GDI scaled DPI messages; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableWindowGDIScaledDpiMessage(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserMessageBeep routine plays the system beep sound of the specified type.
 *
 * \param Type Sound type or alert icon identifier (MB_ICON*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_MESSAGEBEEP) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMessageBeep(
    _In_ ULONG Type // MB_* (MB_ICONMASK) WinUser.h
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserMessageCall routine delivers a window message or executes an internal window message helper.
 *
 * \param WindowHandle Handle to the window receiving the message call.
 * \param Message The message identifier (WM_*).
 * \param wParam Message-specific WPARAM value.
 * \param lParam Message-specific LPARAM value.
 * \param ResultInfo Pointer to result information or return context.
 * \param FnId Internal window message dispatch function index.
 * \param Ansi Non-zero if ANSI dispatch; 0 for Unicode.
 * \return LRESULT The result of the message processing; it depends on the message sent.
 */
_Kernel_entry_
NTSYSCALLAPI
LRESULT
NTAPI
NtUserMessageCall(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ ULONG_PTR ResultInfo,
    _In_ LONG FnId,
    _In_ LONG Ansi
    );

// rev
/**
 * The NtUserMsgWaitForMultipleObjectsEx routine waits until one or all specified objects are in the signaled state or the timeout interval elapses, while waking for message queue events.
 *
 * \param Count The number of object handles in the Handles array.
 * \param Handles Pointer to an array of object handles.
 * \param Milliseconds The time-out interval, in milliseconds.
 * \param WakeMask The input event types for which an input event message will be added to the queue (QS_*).
 * \param Flags The wait type flags (MWMO_*).
 * \return ULONG The index of the signaled object, or WAIT_FAILED on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserMsgWaitForMultipleObjectsEx(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID Handles,
    _In_ LONG Milliseconds,
    _In_ ULONG WakeMask,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserPeekMessage routine dispatches incoming sent messages, checks the thread message queue for a posted message, and retrieves the message (if any exist).
 *
 * \param Message Pointer to an MSG structure that receives message information.
 * \param WindowHandle Optional handle to the window whose messages are to be retrieved.
 * \param MsgFilterMin The integer value of the lowest message value to be retrieved.
 * \param MsgFilterMax The integer value of the highest message value to be retrieved.
 * \param RemoveMsg Command options determining how messages are processed (PM_*).
 * \param Flags Additional message filtering flags.
 * \return BOOL TRUE if a message is available, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserPeekMessage(
    _Out_ PMSG Message,
    _In_opt_ HWND WindowHandle,
    _In_ LONG_PTR MsgFilterMin,
    _In_ LONG_PTR MsgFilterMax,
    _In_ LONG RemoveMsg,
    _In_ CHAR Flags
    );

// rev
/**
 * Posts a keyboard input packet through the edition-specific input implementation.
 * \param TargetId A 32-bit target identifier passed to the edition implementation; namespace unconfirmed.
 * \param KeyboardInput A 20-byte input packet. Its complete field layout remains unconfirmed.
 * \param ExtraInfo Pointer-sized value forwarded unchanged to the edition implementation.
 * \return NTSTATUS. Non-DWM callers receive STATUS_ACCESS_DENIED.
 * \remarks The packet is captured from user memory before its keyboard fields are processed.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserPostKeyboardInputMessage(
    _In_ ULONG TargetId,
    _In_reads_bytes_(20) const VOID *KeyboardInput,
    _In_ ULONG_PTR ExtraInfo
    );

// rev
/**
 * The NtUserPostMessage routine places (posts) a message in the message queue associated with the thread that created the specified window.
 *
 * \param WindowHandle Optional handle to the window whose window procedure is to receive the message (or HWND_BROADCAST).
 * \param Message The message to be posted (WM_*).
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return BOOL TRUE on success, FALSE on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserPostMessage(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserPostQuitMessage routine posts a WM_QUIT message with the specified exit code.
 *
 * \param ExitCode The exit code.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_POSTQUITMESSAGE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPostQuitMessage(
    _In_ LONG ExitCode
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserPostThreadMessage routine posts a message to the message queue of the specified thread.
 *
 * \param ThreadId The identifier of the thread to which the message is to be posted.
 * \param Message The type of message to be posted.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPostThreadMessage(
    _In_ ULONG ThreadId,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// rev
/**
 * The NtUserRealWaitMessageEx routine suspends thread execution until a matching message arrives in the queue.
 *
 * \param WakeMask Mask of queue status flags specifying which input types will wake the thread (QS_*).
 * \param Timeout Maximum duration, in milliseconds, to wait for a message.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRealWaitMessageEx(
    _In_ ULONG WakeMask,
    _In_ ULONG Timeout
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserReassociateQueueEventCompletionPacket routine re-associates a queue event-completion packet with the calling thread's queue.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_REASSOCIATEQUEUEEVENTCOMPLETIONPACKET) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReassociateQueueEventCompletionPacket(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserReplyMessage routine replies to the current inter-thread sent message.
 *
 * \param Result The reply result value passed back to the sending thread.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_REPLYMESSAGE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReplyMessage(
    _In_ LRESULT Result
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSendEventMessage routine sends an event message to a window procedure, bypassing normal queuing mechanisms.
 *
 * \param WindowHandle A handle to the window whose procedure is to receive the message.
 * \param Message The message to be sent.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSendEventMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetMessageExtraInfo routine sets the extra message information for the calling thread.
 *
 * \param ExtraInfo The extra message information to associate with the calling thread.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETMESSAGEEXTRAINFO) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
LPARAM
NTAPI
NtUserSetMessageExtraInfo(
    _In_ LPARAM ExtraInfo
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetMsgBox routine marks the specified window as a message box.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwnd(SFI_SETMSGBOX) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetMsgBox(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetThreadQueueMergeSetting routine sets the specified thread's input-queue merge setting.
 *
 * \param ThreadId The thread id.
 * \param Flags A THREAD_QUEUE_MERGE_FLAGS value. Bits other than 0x00000001 are invalid.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_SETTHREADQUEUEMERGESETTING) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadQueueMergeSetting(
    _In_ ULONG ThreadId,
    _In_ ULONG Flags
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetWindowMessageCapability routine configures access control restrictions on window messages dispatched from processes running with a specific SID.
 *
 * \param WindowHandle A handle to the window to protect.
 * \param Message The message identifier to regulate.
 * \param Sid A pointer to the security identifier (SID) to permit or restrict.
 * \param Enable Nonzero to allow the message from the SID; zero to deny.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowMessageCapability(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PSID Sid,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserTranslateMessage routine translates virtual-key messages into character messages.
 *
 * \param Message A pointer to an MSG structure that contains message information retrieved from the calling thread's message queue.
 * \param Flags Translation control flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserTranslateMessage(
    _In_ CONST MSG *Message,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserWaitAvailableMessageEx routine waits for matching message-queue activity or available queue content.
 *
 * \param WakeMask Queue-status mask selecting the activity to wait for.
 * \param TimeoutMilliseconds Timeout in milliseconds. Zero specifies an indefinite wait.
 * \return TRUE when matching queue activity or content is available, FALSE otherwise.
 * \remarks Existing matching queue content can satisfy the wait. Expiration of the timeout
 *          returns FALSE and sets ERROR_TIMEOUT.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserWaitAvailableMessageEx(
    _In_ ULONG WakeMask,
    _In_ ULONG TimeoutMilliseconds
    );

// rev
/**
 * The NtUserWaitMessage routine yields control to other threads when a thread has no other messages in its message queue.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Native entry point for USER32!WaitMessage.
 *          Waits indefinitely using queue-status mask 0x3CFF. The normal sleep path checks
 *          new queue activity rather than also accepting existing wake bits, unlike NtUserWaitAvailableMessageEx.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserWaitMessage(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserWakeRITForShutdown routine wakes the Raw Input Thread for shutdown processing.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_WAKERITFORSHUTDOWN) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserWakeRITForShutdown(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The SetWindowMessageCapability routine sets the message capability (filter) for a window.
 *
 * \param WindowHandle Handle to the window whose message capability is configured.
 * \param Message Window message identifier.
 * \param Sid Pointer to the capability SID.
 * \param Enable Non-zero to enable message capability; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetWindowMessageCapability system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetWindowMessageCapability(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PSID Sid,
    _In_ LONG Enable
    );

// rev
/**
 * The SoftModalMessageBox routine displays a soft-modal message box.
 *
 * \param MsgBoxData Pointer to the internal message box configuration structure.
 * \return ULONG_PTR Button identifier selected by the user.
 * \remarks Core dispatcher of the MessageBox family.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SoftModalMessageBox(
    _Inout_ PVOID MsgBoxData
    );

// rev
/**
 * The TranslateMessageEx routine translates virtual-key messages into character messages (extended).
 *
 * \param Message Pointer to a MSG structure containing message information.
 * \param Flags Translation flags.
 * \return LONG Non-zero if the message is translated, zero otherwise.
 */
NTSYSAPI
LONG
NTAPI
TranslateMessageEx(
    _In_ const MSG *Message,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRemoveQueueCompletion routine removes a completion entry from the calling thread's message queue.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOVEQUEUECOMPLETION) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRemoveQueueCompletion(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

//
// Hung Applications & Ghost Windows
//

/**
 * End-task callback (FNW32ET) registered with NtUserRegisterUserHungAppHandlers.
 * \remarks Reverse-engineered.
 */
typedef _Function_class_(FNW32ET)
VOID APIENTRY FNW32ET(VOID);
typedef FNW32ET* PFNW32ET;

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterGhostWindow routine registers a ghost window for a hung window.
 *
 * \param Ghost Handle to the ghost window.
 * \param Hung Handle to the hung application window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_REGISTERGHOSTWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterGhostWindow(
    _In_ HWND Ghost,
    _In_ HWND Hung
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterSiblingFrostWindow routine registers a sibling frost (freeze) window.
 *
 * \param hwndFrost The WindowHandle frost.
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_REGISTERSIBLINGFROSTWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterSiblingFrostWindow(
    _In_ HWND hwndFrost,
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The RegisterFrostWindow routine registers a sibling frost window for a hung window.
 *
 * \param hwndFrost Handle to the frost window.
 * \param WindowHandle Handle to the hung target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserRegisterSiblingFrostWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterFrostWindow(
    _In_ HWND hwndFrost,
    _In_ HWND WindowHandle
    );

// rev
/**
 * The RegisterGhostWindow routine registers a ghost window standing in for a hung window.
 *
 * \param Ghost Handle to the ghost window.
 * \param Hung Handle to the hung window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserRegisterGhostWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterGhostWindow(
    _In_ HWND Ghost,
    _In_ HWND Hung
    );

// rev
/**
 * The FrostCrashedWindow routine freezes (ghosts) a hung or crashed window.
 *
 * \param WindowHandle Handle to the hung or crashed window.
 * \param FrostWindowHandle Optional handle to an existing frost window.
 * \return HWND Handle to the frost window, or NULL on failure.
 * \remarks Forwards to the NtUserFrostCrashedWindow system call.
 */
NTSYSAPI
HWND
NTAPI
FrostCrashedWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND FrostWindowHandle
    );

/**
 * The GhostWindowFromHungWindow routine retrieves the ghost window handle associated with a hung window.
 *
 * \param WindowHandle A handle to the hung window.
 * \return A handle to the ghost window, or NULL if none exists.
 */
NTSYSAPI
HWND
NTAPI
GhostWindowFromHungWindow(
    _In_ HWND WindowHandle
    );

/**
 * The HungWindowFromGhostWindow routine retrieves the hung window handle associated with a ghost window.
 *
 * \param WindowHandle A handle to the ghost window.
 * \return A handle to the hung window, or NULL if none exists.
 */
NTSYSAPI
HWND
NTAPI
HungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDisableProcessWindowsGhosting routine disables window ghosting for the calling process.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_DISABLEPROCESSWINDOWSGHOSTING) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDisableProcessWindowsGhosting(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGhostWindowFromHungWindow routine retrieves the ghost window handle associated with a hung window.
 *
 * \param WindowHandle A handle to the hung window.
 * \return A handle to the ghost window, or NULL if none exists.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGhostWindowFromHungWindow(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserHungWindowFromGhostWindow routine retrieves the hung window handle associated with a ghost window.
 *
 * \param WindowHandle A handle to the ghost window.
 * \return A handle to the hung window, or NULL if none exists.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserHungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    );

//
// Console Host Control
//

/**
 * The CONSOLECONTROL enumeration is the class selector for NtUserConsoleControl(Command, ConsoleInformation, ConsoleInformationLength).
 *
 * \remarks Validated against win32kfull.sys NtUserConsoleControl (0x540313C30) and its dispatcher xxxConsoleControl (0x540313D80).
 * The outer syscall guard enforces "cmp class,6 / ja" (classes 0..6 only) plus "cbInfo <= 0x18"; the dispatcher is a sub/jz
 * ladder over classes 0..6, and any class > 6 returns STATUS_INVALID_INFO_CLASS.
 */
typedef enum _CONSOLECONTROL
{
    ConsoleSetVDMCursorBounds = 0,                          // s: RECT // not implemented (falls through to STATUS_INVALID_PARAMETER; NTVDM removed on x64)
    ConsoleNotifyConsoleApplication = 1,                    // s: CONSOLE_PROCESS_INFO (cb 8) -> xxxUserNotifyConsoleApplication
    ConsoleFullscreenSwitch = 2,                            // s: CONSOLE_FULLSCREEN_SWITCH (cb 0x18) // not supported (deprecated stub -> STATUS_NOT_SUPPORTED)
    ConsoleSetCaretInfo = 3,                                // s: CONSOLE_CARET_INFO (cb 0x18) -> xxxSetConsoleCaretInfo
    ConsoleSetReserveKeys = 4,                              // s: CONSOLE_SET_RESERVE_KEYS (cb 0x10)
    ConsoleSetForeground = 5,                               // s: CONSOLE_SET_FOREGROUND (cb 0x10)
    ConsoleSetWindowOwner = 6,                              // s: CONSOLE_WINDOW_OWNER (cb 0x10)
    ConsoleEndTask = 7,                                     // s: CONSOLE_END_TASK // deprecated
} CONSOLECONTROL;

/**
 * CONSOLE_PROCESS_INFO.Flags bit: if set, apply foreground policy and launch the
 * console host in a new window; if clear, reuse the existing window context.
 */
#define CPI_NEWPROCESSWINDOW 0x0001

/**
 * The CONSOLE_PROCESS_INFO structure identifies a console subsystem process to the window manager.
 */
typedef struct _CONSOLE_PROCESS_INFO
{
    ULONG ProcessID;    // The client identifier (PID) of the console application.
    ULONG Flags;        // A combination of CPI_* flags (e.g. CPI_NEWPROCESSWINDOW).
} CONSOLE_PROCESS_INFO, *PCONSOLE_PROCESS_INFO;

/**
 * The CONSOLE_CARET_INFO structure describes the console caret position for accessibility notifications.
 */
typedef struct _CONSOLE_CARET_INFO
{
    HWND WindowHandle;  // A handle to the console window that owns the caret.
    RECT Rect;          // The bounding rectangle of the caret, in client coordinates.
} CONSOLE_CARET_INFO, *PCONSOLE_CARET_INFO;

/**
 * The CONSOLE_FULLSCREEN_SWITCH structure carries data for a legacy fullscreen mode switch.
 * \deprecated The handler is a stub that returns STATUS_NOT_SUPPORTED on current builds.
 */
typedef struct _CONSOLE_FULLSCREEN_SWITCH
{
    BYTE Reserved[24];  // Reserved; the layout is opaque and no longer consumed by the kernel.
} CONSOLE_FULLSCREEN_SWITCH, *PCONSOLE_FULLSCREEN_SWITCH;

/**
 * The CONSOLE_SET_RESERVE_KEYS structure specifies the reserved key combinations for a console window.
 */
typedef struct _CONSOLE_SET_RESERVE_KEYS
{
    HWND WindowHandle;  // A handle to the console window whose reserved keys are being set.
    ULONG Reserved[2];  // The reserved-key bitmask (element 0); element 1 is reserved.
} CONSOLE_SET_RESERVE_KEYS, *PCONSOLE_SET_RESERVE_KEYS;

/**
 * The CONSOLE_SET_FOREGROUND structure grants or revokes foreground activation rights for a process.
 */
typedef struct _CONSOLE_SET_FOREGROUND
{
    HANDLE ProcessHandle;   // A handle to the process whose foreground rights are being changed.
    BOOL Foreground;        // TRUE to grant foreground rights; FALSE to revoke them.
} CONSOLE_SET_FOREGROUND, *PCONSOLE_SET_FOREGROUND;

/**
 * The CONSOLE_WINDOW_OWNER structure assigns the owning process and thread of a console window.
 */
typedef struct _CONSOLE_WINDOW_OWNER
{
    HWND WindowHandle;      // A handle to the console window being reparented.
    ULONG OwnerProcessId;   // The process identifier (PID) of the new owner.
    ULONG OwnerThreadId;    // The thread identifier (TID) of the new owner.
} CONSOLE_WINDOW_OWNER, *PCONSOLE_WINDOW_OWNER;

/**
 * The CONSOLE_END_TASK structure requests termination of the console application attached to a window.
 * Passed with the ConsoleEndTask command of NtUserConsoleControl.
 * \deprecated No longer dispatched by NtUserConsoleControl; the ConsoleEndTask class is rejected by the class<=6 syscall guard.
 */
typedef struct _CONSOLE_END_TASK
{
    HANDLE ProcessId;           // A handle/identifier of the process to signal.
    HWND WindowHandle;          // A handle to the console window associated with the task.
    ULONG ConsoleEventCode;     // The control event to deliver (e.g. CTRL_C_EVENT / CTRL_BREAK_EVENT).
    ULONG ConsoleFlags;         // Flags controlling the end-task behavior.
} CONSOLE_END_TASK, *PCONSOLE_END_TASK;

/**
 * The ConsoleControl routine performs special kernel operations for console host applications.
 *
 * This includes reparenting the console window, allowing the console to pass foreground rights
 * on to launched console subsystem applications, and terminating attached processes.
 *
 * \param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * \param ConsoleInformation A pointer to a structure specifying additional data for the requested console control function.
 * \param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exported via user32.dll.
 */
NTSYSAPI
NTSTATUS
NTAPI
ConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

/**
 * The NtUserConsoleControl routine performs special kernel operations for console host applications.
 *
 * This includes reparenting the console window, allowing the console to pass foreground rights
 * on to launched console subsystem applications, and terminating attached processes.
 *
 * \param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * \param ConsoleInformation A pointer to a structure specifying additional data for the requested console control function.
 * \param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exported via win32u.dll.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

//
// Window Management (HWND)
//

#define FW_BOTH 0
#define FW_16BIT 1
#define FW_32BIT 2

/**
 * The WINDOWINFOCLASS enumeration selects which piece of window information NtUserQueryWindow returns.
 */
typedef enum _WINDOWINFOCLASS
{
    // Validated against win32kfull!NtUserQueryWindow (0x540524C50). pwnd = ValidateHwnd(WindowHandle);
    // pti = *(pwnd+0x10); pq = *(pti+0x1D8). Unhandled classes -> return 0.
    WindowProcess = 0,                                      // q: ULONG (owner process id)
    WindowRealProcess = 1,                                  // q: ULONG (real owner pid; may differ from WindowProcess for server-side owned windows)
    WindowThread = 2,                                       // q: ULONG (owner thread id)
    WindowActiveWindow = 3,                                 // q: HWND (pq->spwndActive)
    WindowFocusWindow = 4,                                  // q: HWND (pq->spwndFocus)
    WindowIsHung = 5,                                       // q: BOOLEAN (backs IsHungAppWindow)
    WindowClientBase = 6,                                   // q: not handled in this build (returns 0)
    WindowIsForegroundThread = 7,                           // q: BOOLEAN (thread queue == foreground queue)
    WindowDefaultImeWindow = 8,                             // q: HWND (default IME window)
    WindowDefaultInputContext = 9,                          // q: HIMC (default input context)
} WINDOWINFOCLASS, *PWINDOWINFOCLASS;

/**
 * Index values for GetKeyboardType (KEYBOARD_TYPE / SUBTYPE / FUNCTION_KEY).
 * \remarks Names per MSDN.
 */
#define KEYBOARD_TYPE           0
#define KEYBOARD_SUBTYPE        1
#define KEYBOARD_FUNCTION_KEY   2

/**
 * Packed window-word state values (byte offset in the high byte, bit mask in the low byte) used with NtUserSetWindowState and NtUserClearWindowState.
 */
#define WFWIN40COMPAT           0x0502 // [+0x05 state2, bit 0x02] internal: Win4.0 (Win95) compatibility behavior
#define WFNOANIMATE             0x0710 // [+0x07 state2, bit 0x10] internal: suppress open/close animation
#define WEFWINDOWEDGE           0x0901 // [+0x09 exStyle, bit 0x01] WS_EX_WINDOWEDGE (0x00000100) - raised outer edge
#define WEFCLIENTEDGE           0x0902 // [+0x09 exStyle, bit 0x02] WS_EX_CLIENTEDGE (0x00000200) - sunken client edge
#define WEFSTATICEDGE           0x0A02 // [+0x0A exStyle, bit 0x02] WS_EX_STATICEDGE (0x00020000) - 3D border for non-interactive items
#define BFRIGHTBUTTON           0x0C20 // [+0x0C style, bit 0x20] BS_LEFTTEXT/BS_RIGHTBUTTON (0x0020) - check/radio glyph on right
#define BFRIGHT                 0x0D02 // [+0x0D style, bit 0x02] BS_RIGHT (0x0200) - right-align button text
#define BFBOTTOM                0x0D08 // [+0x0D style, bit 0x08] BS_BOTTOM (0x0800) - bottom-align button text
#define DFLOCALEDIT             0x0C20 // [+0x0C style, bit 0x20] DS_LOCALEDIT (0x0020) - dialog edits use app-local memory
#define CBFOWNERDRAWVAR         0x0C20 // [+0x0C style, bit 0x20] CBS_OWNERDRAWVARIABLE (0x0020) - owner-draw, variable item height
#define CBFHASSTRINGS           0x0D02 // [+0x0D style, bit 0x02] CBS_HASSTRINGS (0x0200) - owner-draw combobox keeps item strings
#define CBFDISABLENOSCROLL      0x0D08 // [+0x0D style, bit 0x08] CBS_DISABLENOSCROLL (0x0800) - show disabled vertical scrollbar
#define EFPASSWORD              0x0C20 // [+0x0C style, bit 0x20] ES_PASSWORD (0x0020) - mask typed characters
#define EFCOMBOBOX              0x0D02 // [+0x0D style, bit 0x02] internal (0x0200): edit is the combobox child edit
#define EFREADONLY              0x0D08 // [+0x0D style, bit 0x08] ES_READONLY (0x0800) - text cannot be edited
#define SFWIDELINESPACING       0x0C20 // [+0x0C style, bit 0x20] SS_* (0x0020) - static wide line-spacing variant
#define SFCENTERIMAGE           0x0D02 // [+0x0D style, bit 0x02] SS_CENTERIMAGE (0x0200) - center bitmap/icon in static
#define SFREALSIZEIMAGE         0x0D08 // [+0x0D style, bit 0x08] SS_REALSIZEIMAGE (0x0800) - do not stretch image to control
#define WFMAXBOX                0x0E01 // [+0x0E style, bit 0x01] WS_MAXIMIZEBOX (0x00010000) - has Maximize button
#define WFTABSTOP               0x0E01 // [+0x0E style, bit 0x01] WS_TABSTOP (0x00010000) - alias of WS_MAXIMIZEBOX; tab-navigable control
#define WFSYSMENU               0x0E08 // [+0x0E style, bit 0x08] WS_SYSMENU (0x00080000) - has window (system) menu
#define WFHSCROLL               0x0E10 // [+0x0E style, bit 0x10] WS_HSCROLL (0x00100000) - has horizontal scrollbar
#define WFVSCROLL               0x0E20 // [+0x0E style, bit 0x20] WS_VSCROLL (0x00200000) - has vertical scrollbar
#define WFBORDER                0x0E80 // [+0x0E style, bit 0x80] WS_BORDER (0x00800000) - has thin-line border
#define WFCLIPCHILDREN          0x0F02 // [+0x0F style, bit 0x02] WS_CLIPCHILDREN (0x02000000) - exclude child areas when drawing parent

/**
 * Visibility state values for NtUserSetVisible (SV_*).
 */
#define SV_UNSET            0x0000
#define SV_SET              0x0001
#define SV_CLRFTRUEVIS      0x0002
#define SV_SKIPCOMPOSE      0x0004 // rev
#define SV_CLRFULLSCREEN    0x0008 // rev
#define SV_CLRGHOSTVIS      0x0010 // rev

/**
 * Caption-drawing flags (DC_*) used with NtUserRedrawTitle; entries marked WinUser.h are defined there.
 */
// #define DC_ACTIVE    0x0001 // WinUser.h
// #define DC_SMALLCAP  0x0002 // WinUser.h
// #define DC_ICON      0x0004 // WinUser.h
// #define DC_TEXT      0x0008 // WinUser.h
// #define DC_INBUTTON  0x0010 // WinUser.h
// #define DC_GRADIENT  0x0020 // WinUser.h
#define DC_LAMEBUTTON   0x0400
#define DC_NOVISIBLE    0x0800
// #define DC_BUTTONS   0x1000 // WinUser.h
#define DC_NOSENDMSG    0x2000
#define DC_CENTER       0x4000
#define DC_FRAME        0x8000

// rev
/**
 * The ZBID enumeration specifies z-order band identifiers used by win32kbase.sys and user32.dll to place top-level
 * windows into fixed z-order bands. GetWindowBand, SetWindowBand and CreateWindowInBand(Ex) take a ZBID.
 *
 * The bottom-to-top band order is fixed by an internal 18-entry ordinal table in win32kfull.sys and is
 * NOT the numeric order of the enumerators (ZBID_DESKTOP is lowest, ZBID_UIACCESS is highest):
 *   DESKTOP < IMMERSIVE_RESTRICTED < IMMERSIVE_BACKGROUND < IMMERSIVE_INACTIVEDOCK <
 *   IMMERSIVE_INACTIVEMOBODY < IMMERSIVE_ACTIVEDOCK < IMMERSIVE_ACTIVEMOBODY < IMMERSIVE_APPCHROME <
 *   IMMERSIVE_MOGO < IMMERSIVE_SEARCH < IMMERSIVE_NOTIFICATION < IMMERSIVE_EDGY < SYSTEM_TOOLS <
 *   LOCK < ABOVELOCK_UX < IMMERSIVE_IHM < GENUINE_WINDOWS < UIACCESS.
 *
 * \remarks Reverse-engineered. The enumerator values and band ordering are confirmed against the
 * win32kfull.sys band table (IsValidBand / GetBandOrdinal); the enumerator names follow the established
 * public convention. ZBID_DEFAULT is the default band and is not part of the ordinal table.
 */
typedef enum _ZBID
{
    ZBID_DEFAULT = 0,
    ZBID_DESKTOP = 1,
    ZBID_UIACCESS = 2,
    ZBID_IMMERSIVE_IHM = 3,
    ZBID_IMMERSIVE_NOTIFICATION = 4,
    ZBID_IMMERSIVE_APPCHROME = 5,
    ZBID_IMMERSIVE_MOGO = 6,
    ZBID_IMMERSIVE_EDGY = 7,
    ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
    ZBID_IMMERSIVE_INACTIVEDOCK = 9,
    ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
    ZBID_IMMERSIVE_ACTIVEDOCK = 11,
    ZBID_IMMERSIVE_BACKGROUND = 12,
    ZBID_IMMERSIVE_SEARCH = 13,
    ZBID_GENUINE_WINDOWS = 14,
    ZBID_ABOVELOCK_UX = 15,
    ZBID_SYSTEM_TOOLS = 16,
    ZBID_LOCK = 17,
    ZBID_IMMERSIVE_RESTRICTED = 18,
} ZBID;

// rev
/**
 * The CreateDialogIndirectParamAorW routine is the ANSI/Unicode-neutral core of CreateDialogIndirectParam.
 *
 * \param Instance Optional handle to the module instance containing the dialog template.
 * \param Template Pointer to a dialog box template.
 * \param OwnerWindow Optional handle to the dialog owner window.
 * \param DialogProc Optional pointer to the dialog box procedure.
 * \param InitParam Initialization value passed to the dialog procedure via WM_INITDIALOG.
 * \param Unicode TRUE for Unicode dialog handling; FALSE for ANSI.
 * \return HWND Handle to the created dialog box, or NULL on failure.
 */
NTSYSAPI
HWND
NTAPI
CreateDialogIndirectParamAorW(
    _In_opt_ HINSTANCE Instance,
    _In_ LPCDLGTEMPLATE Template,
    _In_opt_ HWND OwnerWindow,
    _In_opt_ DLGPROC DialogProc,
    _In_ LPARAM InitParam,
    _In_ BOOL Unicode
    );

// rev
/**
 * The CreateLayoutSyncForHwnd routine creates a layout-synchronization object for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param LayoutSyncHandle Pointer receiving the created layout synchronization handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserCreateLayoutSyncForHwnd system call.
 */
NTSYSAPI
LOGICAL
NTAPI
CreateLayoutSyncForHwnd(
    _In_ HWND WindowHandle,
    _Out_ PHANDLE LayoutSyncHandle
    );

// rev
/**
 * The CreateWindowInBand routine creates a Unicode window in the specified z-order band.
 *
 * \param ExStyle Extended window style flags.
 * \param ClassName Optional pointer to a null-terminated window class name string.
 * \param WindowName Optional pointer to a null-terminated window title string.
 * \param Style Window style flags.
 * \param X Initial horizontal position of the window.
 * \param Y Initial vertical position of the window.
 * \param Width Width of the window in pixels.
 * \param Height Height of the window in pixels.
 * \param ParentWindow Optional handle to the parent or owner window.
 * \param MenuHandle Optional handle to a menu, or specifies a child-window identifier.
 * \param InstanceHandle Optional handle to the module instance associated with the window.
 * \param Parameter Optional pointer to value passed via WM_CREATE lpCreateParams.
 * \param Band Window z-order band identifier.
 * \return HWND Handle to the created window, or NULL on failure.
 */
NTSYSAPI
HWND
NTAPI
CreateWindowInBand(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ HINSTANCE InstanceHandle,
    _In_opt_ PVOID Parameter,
    _In_ ULONG Band
    );

// rev
/**
 * The CreateWindowInBandEx routine creates a Unicode window in the specified z-order band with additional type flags.
 *
 * \param ExStyle Extended window style flags.
 * \param ClassName Optional pointer to a null-terminated window class name string.
 * \param WindowName Optional pointer to a null-terminated window title string.
 * \param Style Window style flags.
 * \param X Initial horizontal position of the window.
 * \param Y Initial vertical position of the window.
 * \param Width Width of the window in pixels.
 * \param Height Height of the window in pixels.
 * \param ParentWindow Optional handle to the parent or owner window.
 * \param MenuHandle Optional handle to a menu, or specifies a child-window identifier.
 * \param InstanceHandle Optional handle to the module instance associated with the window.
 * \param Parameter Optional pointer to value passed via WM_CREATE lpCreateParams.
 * \param Band Window z-order band identifier.
 * \param TypeFlags Additional window type creation flags.
 * \return HWND Handle to the created window, or NULL on failure.
 */
NTSYSAPI
HWND
NTAPI
CreateWindowInBandEx(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ HINSTANCE InstanceHandle,
    _In_opt_ PVOID Parameter,
    _In_ ULONG Band,
    _In_ ULONG TypeFlags
    );

// rev
/**
 * The CreateWindowIndirect routine is an internal window creation routine.
 *
 * \param CreateStruct Pointer to the internal creation structure.
 * \return ULONG_PTR Window handle or creation status.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CreateWindowIndirect(
    _In_ PVOID CreateStruct
    );

// rev
/**
 * The NtUserCreateActivationObject routine creates a window activation context object.
 *
 * \param WindowHandle Handle to the associated window.
 * \param ObjectInfo A pointer to the activation object information read from the caller.
 * \param ObjectHandle Receives a handle to the created activation object.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateActivationObject(
    _In_ HWND WindowHandle,
    _In_ PVOID ObjectInfo,
    _Out_ PHANDLE ObjectHandle
    );

// rev
/**
 * Creates a base-window handle for a thread supporting base windows.
 * \param Context Non-NULL pointer-sized value stored in the base-window object; its use is unconfirmed.
 * \param ExtraBytes Size, in bytes, of the additional zero-initialized allocation; zero allocates none.
 * \param AllocationFlags Value forwarded to HMAllocObjectEx; flag meanings remain unconfirmed.
 * \return The base-window handle, or NULL on failure.
 * \remarks A NULL Context sets ERROR_INVALID_PARAMETER. A thread lacking the required
 * base-window capability sets ERROR_INVALID_OPERATION. This is not an HWND creation API.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserCreateBaseWindow(
    _In_ PVOID Context,
    _In_ ULONG ExtraBytes,
    _In_ ULONG AllocationFlags
    );

// rev
/**
 * The NtUserCreateLayoutSyncForHwnd routine creates a layout synchronization object for a window.
 *
 * \param WindowHandle Handle to the window to synchronize layout for.
 * \param LayoutSyncHandle Pointer to a variable receiving the created layout synchronization handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateLayoutSyncForHwnd(
    _In_ HWND WindowHandle,
    _Out_ PHANDLE LayoutSyncHandle
    );

// rev
/**
 * The NtUserCreateWindowEx routine creates an overlapped, pop-up, or child window with an extended window style.
 *
 * \param ExStyle Extended window style (WS_EX_*).
 * \param ClassName Pointer to a LARGE_STRING containing the window class name or class atom.
 * \param ClassVersion Pointer to a LARGE_STRING containing the class version or manifest specification.
 * \param WindowName Pointer to a LARGE_STRING containing the window title or text.
 * \param Style Window style flags (WS_*).
 * \param X Initial horizontal position of the window.
 * \param Y Initial vertical position of the window.
 * \param Width Width of the window, in device units.
 * \param Height Height of the window, in device units.
 * \param ParentWindow Optional handle to the parent or owner window.
 * \param Menu Optional handle to a menu, or child-window identifier.
 * \param Instance Optional handle to the instance of the module associated with the window.
 * \param Param Optional pointer to window-creation data (lpCreateParams).
 * \param Band The Z-order band (ZBID_*) the window is created in.
 * \param ExpWinVer The expected Windows version for the window, combined with the high creation flag bits.
 * \param TypeFlags Window type flags, as passed to CreateWindowInBandEx; bit 0 and bits 3 and above are tested by the kernel.
 * \param ActivationContextBuffer Optional pointer to activation context activation data.
 * \return HWND A handle to the new window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserCreateWindowEx(
    _In_ ULONG ExStyle,
    _In_ PLARGE_STRING ClassName,
    _In_ PLARGE_STRING ClassVersion,
    _In_ PLARGE_STRING WindowName,
    _In_ ULONG Style,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU Menu,
    _In_opt_ HINSTANCE Instance,
    _In_opt_ PVOID Param,
    _In_ ULONG Band,
    _In_ ULONG ExpWinVer,
    _In_ ULONG TypeFlags,
    _In_opt_ PVOID ActivationContextBuffer
    );

// rev
/**
 * The NtUserRegisterBSDRWindow routine registers the session's BSDR window for shutdown-block reason reporting.
 *
 * \param WindowHandle Optional window to register. NULL leaves the current registration unchanged.
 * \param NotificationCode Value forwarded as wParam in an internal 0x329 thread notification.
 * Zero suppresses the notification.
 * \return Nonzero on success, zero for an invalid window or an unauthorized caller.
 * \remarks The caller must match the session's authorized process, or hold SeTcbPrivilege when
 * no authorized process is registered. Authorization failure sets ERROR_ACCESS_DENIED.
 * A nonzero NotificationCode queues an event for the session's notification thread, if present,
 * with lParam equal to 0xFFFFFFFF. Success does not guarantee that the notification was queued.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterBSDRWindow(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG NotificationCode
    );

// rev
/**
 * Registers an error-reporting dialog with DWM through an asynchronous ghost notification.
 * \param WindowHandle A valid window that is not being destroyed.
 * \param DialogInfo Opaque 32-bit value copied into the type-4 GHOSTINFO notification.
 * \return Nonzero if the asynchronous notification succeeds, zero otherwise.
 * \remarks The meaning of DialogInfo is not established; it is not validated as a flags mask.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterErrorReportingDialog(
    _In_ HWND WindowHandle,
    _In_ LONG DialogInfo
    );

// rev
/**
 * The NtUserRegisterForCustomDockTargets routine registers a window to participate as a custom dock target for shell window arrangement and docking gestures.
 *
 * \param WindowHandle A handle to the window to register.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterForCustomDockTargets(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserRegisterForTooltipDismissNotification routine registers a window to receive tooltip dismissal notifications when mouse or keyboard focus changes.
 *
 * \param HWnd A handle to the tooltip window.
 * \param TdFlags Flags that specify the conditions under which tooltip dismissal notifications are sent.
 * \return TRUE if registration succeeded, or FALSE otherwise.
 * \remarks Native entry point for USER32!RegisterForTooltipDismissNotification.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterForTooltipDismissNotification(
    _In_ HWND HWnd,
    _In_ TOOLTIP_DISMISS_FLAGS TdFlags
    );

/**
 * The NtUserRegisterHotKey routine defines a system-wide hot key.
 *
 * \param WindowHandle A handle to the window that will receive WM_HOTKEY messages generated by the hot key.
 * \param Id The identifier of the hot key.
 * \param fsModifiers The keys that must be pressed in combination with the key specified by vk (MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN).
 * \param vk The virtual-key code of the hot key.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterHotKey(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Id,
    _In_ ULONG fsModifiers,
    _In_ ULONG vk
    );

// rev
/**
 * The NtUserRegisterTasklist routine registers the top-level tasklist or shell taskbar window with the window manager.
 *
 * \param WindowHandle A handle to the shell tasklist window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterTasklist(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterWindowArrangementCallout routine registers the window-arrangement callout.
 *
 * \param WindowHandle Handle to the target window.
 * \param Register Nonzero to register the callout; zero to unregister.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_REGISTERWINDOWARRANGEMENTCALLOUT) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterWindowArrangementCallout(
    _In_ HWND WindowHandle,
    _In_ LOGICAL Register
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserShellRegisterHotKey routine registers a shell hot key with target routing to a specified recipient window.
 *
 * \param WindowHandle An optional handle to the window that receives hot key messages.
 * \param HotKeyId The hot key identifier.
 * \param Modifiers Modifier key combination flags (e.g. MOD_ALT, MOD_CONTROL, MOD_SHIFT).
 * \param VirtualKey The virtual-key code of the hot key.
 * \param TargetWindowHandle An optional handle to an alternate target recipient window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellRegisterHotKey(
    _In_opt_ HWND WindowHandle,
    _In_ LONG HotKeyId,
    _In_ LONG Modifiers,
    _In_ ULONG VirtualKey,
    _In_opt_ HWND TargetWindowHandle
    );

// rev
/**
 * The NtUserShutdownBlockReasonCreate routine indicates that the system cannot be shut down and sets a reason string to be displayed to the user.
 *
 * \param WindowHandle A handle to the main window of the application.
 * \param Reason A pointer to the reason string explaining why shutdown is blocked.
 * \param ReasonLength The length, in characters, of the Reason string.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShutdownBlockReasonCreate(
    _In_ HWND WindowHandle,
    _In_reads_(ReasonLength) PWSTR Reason,
    _In_ ULONG ReasonLength
    );

// rev
/**
 * The RegisterBSDRWindow routine registers the session's BSDR window for shutdown-block reason reporting.
 *
 * \param WindowHandle Optional window to register. NULL leaves the current registration unchanged.
 * \param NotificationCode Internal notification value; zero suppresses notification.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Forwards to NtUserRegisterBSDRWindow, including its authorization checks.
 */
NTSYSAPI
BOOL
NTAPI
RegisterBSDRWindow(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG NotificationCode
    );

// rev
/**
 * The RegisterErrorReportingDialog routine registers an error reporting dialog with the window manager.
 *
 * \param WindowHandle Handle to the error reporting dialog window.
 * \param Flags Registration flags controlling dialog display and interaction.
 * \return BOOL TRUE if successful, FALSE otherwise.
 * \remarks Forwards to the NtUserRegisterErrorReportingDialog system call.
 */
NTSYSAPI
BOOL
NTAPI
RegisterErrorReportingDialog(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags
    );

// rev
/**
 * The RegisterForCustomDockTargets routine registers a window for custom dock (snap) targets.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserRegisterForCustomDockTargets system call.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterForCustomDockTargets(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The RegisterTasklist routine registers the task-list window with the window manager.
 *
 * \param WindowHandle Handle to the task-list window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserRegisterTasklist system call.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterTasklist(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The ShellRegisterHotKey routine registers a shell hot key.
 *
 * \param WindowHandle Optional handle to the window that receives hot key messages.
 * \param HotKeyId Identifier of the hot key.
 * \param Modifiers Modifier keys (MOD_* flags).
 * \param VirtualKey Virtual key code.
 * \param TargetWindowHandle Optional handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserShellRegisterHotKey system call.
 */
NTSYSAPI
LOGICAL
NTAPI
ShellRegisterHotKey(
    _In_opt_ HWND WindowHandle,
    _In_ LONG HotKeyId,
    _In_ LONG Modifiers,
    _In_ ULONG VirtualKey,
    _In_opt_ HWND TargetWindowHandle
    );

// rev
/**
 * The DwmGetDxSharedSurface routine retrieves the DWM/DirectX shared surface backing a window's redirection bitmap.
 *
 * \param WindowHandle Handle to the target window.
 * \param SharedSurfaceHandle Optional pointer receiving the shared surface handle.
 * \param AdapterLuid Optional pointer to the adapter LUID.
 * \param FormatWindow Optional pointer receiving the window format.
 * \param PresentFlags Optional pointer to presentation flags.
 * \param Win32kUpdateId Optional pointer receiving the win32k update sequence ID.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Thin user32 wrapper over the NtUserHwndQueryRedirectionInfo system call.
 */
NTSYSAPI
BOOL
NTAPI
DwmGetDxSharedSurface(
    _In_ HWND WindowHandle,
    _Out_opt_ PHANDLE SharedSurfaceHandle,
    _Inout_opt_ PLUID AdapterLuid,
    _Out_opt_ PULONG FormatWindow,
    _Inout_opt_ PULONG PresentFlags,
    _Out_opt_ PULONGLONG Win32kUpdateId
    );

// rev
/**
 * The DwmValidateWindow routine validates a window with DWM.
 *
 * \param WindowHandle Handle to the window to validate.
 * \param ProcessId Process identifier of the window owner.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserDwmValidateWindow system call.
 */
NTSYSAPI
LOGICAL
NTAPI
DwmValidateWindow(
    _In_ HWND WindowHandle,
    _In_ LONG ProcessId
    );

// rev
/**
 * The GetInternalWindowPos routine retrieves the internal (restored) placement of a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param NormalPosition Optional pointer receiving the normal (restored) rectangle.
 * \param MinPosition Optional pointer receiving the minimized position.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetInternalWindowPos(
    _In_ HWND WindowHandle,
    _Out_opt_ PRECT NormalPosition,
    _Out_opt_ PPOINT MinPosition
    );

// rev
/**
 * The GetProgmanWindow routine retrieves the handle to the Program Manager window.
 *
 * \return HWND Handle to the Program Manager window, or NULL if not found.
 */
NTSYSAPI
HWND
NTAPI
GetProgmanWindow(
    VOID
    );

/**
 * The GetRealWindowOwner routine retrieves the process identifier of the real owner of the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \return A handle or identifier of the real window owner process.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
GetRealWindowOwner(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The GetShellChangeNotifyWindow routine retrieves the handle to the shell change-notification window.
 *
 * \return HWND Handle to the shell change-notification window, or NULL if not set.
 */
NTSYSAPI
HWND
NTAPI
GetShellChangeNotifyWindow(
    VOID
    );

// rev
/**
 * The GetTaskmanWindow routine retrieves the handle to the Task Manager window.
 *
 * \return HWND Handle to the taskman window, or NULL if not set.
 */
NTSYSAPI
HWND
NTAPI
GetTaskmanWindow(
    VOID
    );

// rev
/**
 * The GetTopLevelWindow routine retrieves the top-level ancestor of a window.
 *
 * \param WindowHandle Handle to the window whose top-level ancestor is queried.
 * \return HWND A handle to the top-level window, or NULL on failure.
 * \remarks Forwards to the NtUserGetTopLevelWindow system call.
 */
NTSYSAPI
HWND
NTAPI
GetTopLevelWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The GetWindowBand routine retrieves the z-order band of a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param BandId Pointer that receives the window band identifier (ZBID).
 * \return TRUE on success, FALSE otherwise without modifying BandId on failure.
 */
NTSYSAPI
BOOL
NTAPI
GetWindowBand(
    _In_ HWND WindowHandle,
    _Out_ PULONG BandId
    );

// rev
/**
 * The GetWindowMinimizeRect routine retrieves the rectangle a window minimizes to.
 *
 * \param WindowHandle Handle to the target window.
 * \param MinimizeRect Pointer receiving the minimized rectangle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserGetWindowMinimizeRect system call.
 */
NTSYSAPI
LOGICAL
NTAPI
GetWindowMinimizeRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT MinimizeRect
    );

/**
 * The GetWindowProcessHandle routine opens a handle to the process that owns the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \param DesiredAccess The access mask requested for the process handle.
 * \return A handle to the window's owner process, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
GetWindowProcessHandle(
    _In_ HWND WindowHandle,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The GetWindowRgnEx routine retrieves the window region with extended flags.
 *
 * \param WindowHandle Handle to the window whose region is queried.
 * \param RegionHandle Handle to the region to be modified.
 * \param Flags Extended region query flags.
 * \return LONG The region complexity: NULLREGION, SIMPLEREGION, COMPLEXREGION, or ERROR on failure.
 * \remarks Forwards to the NtUserGetWindowRgnEx system call.
 */
NTSYSAPI
LONG
NTAPI
GetWindowRgnEx(
    _In_ HWND WindowHandle,
    _In_ HRGN RegionHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The IsServerSideWindow routine tests whether a window has a server-side window procedure.
 *
 * \param WindowHandle Handle to the window to test.
 * \return BOOL TRUE if the window procedure executes in the server context (win32k), FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IsServerSideWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The IsTopLevelWindow routine determines whether a window is a top-level window.
 *
 * \param WindowHandle Handle to the window to test.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserIsTopLevelWindow system call.
 */
NTSYSAPI
LOGICAL
NTAPI
IsTopLevelWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The IsWindowInDestroy routine tests whether a window is currently in the process of being destroyed.
 *
 * \param WindowHandle Handle to the window to test.
 * \return BOOL TRUE if the window is being destroyed, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IsWindowInDestroy(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The IsWindowRedirectedForPrint routine determines whether a window is redirected for printing.
 *
 * \param WindowHandle Handle to the window to test.
 * \return ULONG_PTR Non-zero if redirected for print, zero otherwise.
 */
NTSYSAPI
ULONG_PTR
NTAPI
IsWindowRedirectedForPrint(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserBuildHwndList routine builds a list of window handles matching criteria across desktops or threads.
 *
 * \param DesktopHandle Optional desktop handle to search.
 * \param ParentWindowHandle Optional parent window handle.
 * \param IncludeChildren If TRUE, recursively includes child windows.
 * \param ExcludeImmersive If TRUE, excludes immersive (UWP) windows.
 * \param ThreadId Optional thread ID to filter windows.
 * \param HwndListInformationLength Size of the output buffer in bytes.
 * \param HwndListInformation Buffer receiving the array of window handles.
 * \param ReturnLength Receives the number of bytes written or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildHwndList(
    _In_opt_ HANDLE DesktopHandle,
    _In_opt_ HWND ParentWindowHandle,
    _In_opt_ BOOL IncludeChildren,
    _In_opt_ BOOL ExcludeImmersive,
    _In_opt_ ULONG ThreadId,
    _In_ ULONG HwndListInformationLength,
    _Out_writes_bytes_(HwndListInformationLength) PVOID HwndListInformation,
    _Out_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDWP_GetEnabledPopupOffset routine returns the enabled-popup offset used by DefWindowProc for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return Enabled-popup offset for the window.
 * \remarks Exposed via NtUserCallHwnd(SFI_DWP_GETENABLEDPOPUPOFFSET) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDWP_GetEnabledPopupOffset(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDwmValidateWindow routine validates or updates DWM composition tracking for a specified window and process.
 *
 * \param WindowHandle Handle to the window to validate.
 * \param ProcessId Identifier of the process owning the window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDwmValidateWindow(
    _In_ HWND WindowHandle,
    _In_ LONG ProcessId
    );

/**
 * The NtUserFindWindowEx routine retrieves a handle to a window whose class name and window name match the specified strings.
 *
 * \param hwndParent Handle to the parent window whose child windows are to be searched.
 * \param hwndChild Handle to a child window to begin search after.
 * \param ClassName Pointer to a UNICODE_STRING containing the window class name.
 * \param WindowName Pointer to a UNICODE_STRING containing the window title.
 * \param Type Window search flags (FW_*).
 * \return Handle to the matching window, or NULL if not found.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserFindWindowEx(
    _In_opt_ HWND hwndParent,
    _In_opt_ HWND hwndChild,
    _In_ PCUNICODE_STRING ClassName,
    _In_ PCUNICODE_STRING WindowName,
    _In_ ULONG Type // FW_*
    );

// rev
/**
 * The NtUserForceWindowToDpiForTest routine forces a window to scale to a test DPI value for testing and validation.
 *
 * \param WindowHandle Handle to the target window.
 * \param Dpi Test dots per inch (DPI) value to apply.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserForceWindowToDpiForTest(
    _In_ HWND WindowHandle,
    _In_ ULONG Dpi
    );

/**
 * The NtUserGetAncestor routine retrieves the ancestor window of the specified window.
 *
 * \param WindowHandle A handle to the window whose ancestor is to be retrieved.
 * \param gaFlags The ancestor to be retrieved (e.g. GA_PARENT, GA_ROOT, GA_ROOTOWNER).
 * \return A handle to the ancestor window, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetAncestor(
    _In_ HWND WindowHandle,
    _In_ ULONG gaFlags
    );

// rev
/**
 * The NtUserGetCPD routine retrieves or creates a CallProcData (CPD) thunk structure for window subclassing.
 *
 * \param WindowHandle Handle to the window whose procedure is thunked.
 * \param Flags CPD creation flags (e.g. CPD_ANSI_TO_UNICODE, CPD_UNICODE_TO_ANSI).
 * \param Proc Window procedure address to thunk.
 * \return ULONG_PTR The created or existing CPD handle.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetCPD(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _In_ LONG_PTR Proc
    );

/**
 * The NtUserGetClassName routine retrieves a string that specifies the window type.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param RealClassName Return the superclass or baseclass name when the window is a superclass.
 * \param ClassName A pointer to a string that receives the window type.
 * \return A handle to the foreground window, or NULL if no foreground window exists.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-realgetwindowclassw
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetClassName(
    _In_ HWND WindowHandle,
    _In_ BOOL RealClassName,
    _Out_ PUNICODE_STRING ClassName
    );

/**
 * The NtUserGetComboBoxInfo routine retrieves information about the specified combo box.
 *
 * \param WindowHandleCombo A handle to the combo box.
 * \param pcbi Pointer to a COMBOBOXINFO structure that receives the information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetComboBoxInfo(
    _In_ HWND WindowHandleCombo,
    _Inout_ PCOMBOBOXINFO pcbi
    );

// rev
/**
 * The NtUserGetControlColor routine retrieves color settings and brush for standard controls.
 *
 * \param ParentWindowHandle Handle to the parent window receiving WM_CTLCOLOR* messages.
 * \param WindowHandle Handle to the child control window.
 * \param Hdc Handle to the device context for drawing the control.
 * \param Message Color message identifier (e.g. WM_CTLCOLORSTATIC).
 * \return HBRUSH Handle to the control brush.
 */
_Kernel_entry_
NTSYSCALLAPI
HBRUSH
NTAPI
NtUserGetControlColor(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ LONG Message
    );

/**
 * The NtUserGetCurrentDpiInfoForWindow routine retrieves the DPI information for the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \param DpiInfo Pointer to a buffer receiving the DPI information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOLEAN
NTAPI
NtUserGetCurrentDpiInfoForWindow(
    _In_ HWND WindowHandle,
    _Out_ PVOID DpiInfo
    );

/**
 * The NtUserGetForegroundWindow routine retrieves a handle to the foreground window.
 *
 * \return A handle to the foreground window, or NULL if no foreground window exists.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getforegroundwindow
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetForegroundWindow(
    VOID
    );

/**
 * The NtUserGetLayeredWindowAttributes routine retrieves the opacity and transparency color key of a layered window.
 *
 * \param WindowHandle A handle to the layered window.
 * \param Key Optional pointer to a COLORREF value that receives the transparency color key.
 * \param Alpha Optional pointer to a BYTE that receives the alpha value used to describe the opacity of the layered window.
 * \param Flags Optional pointer to a variable that receives the layering flags.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    _In_ HWND WindowHandle,
    _Out_opt_ COLORREF* Key,
    _Out_opt_ PBYTE Alpha,
    _Out_opt_ PULONG Flags
    );

/**
 * The NtUserGetListBoxInfo routine retrieves the number of items per column in a specified list box.
 *
 * \param WindowHandle A handle to the list box.
 * \return The number of items per column.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetListBoxInfo(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserGetMinuserIdForBaseWindow routine retrieves the Minuser component identifier for a base window.
 *
 * \param BaseWindowHandle Integer handle or identifier of the base window.
 * \return ULONG_PTR The minuser identifier for the base window, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetMinuserIdForBaseWindow(
    _In_ LONG BaseWindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetModernAppWindow routine returns the modern (immersive) app window for the specified window.
 *
 * \param ShellFrame The shell frame.
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallHwnd(SFI_GETMODERNAPPWINDOW) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetModernAppWindow(
    _In_ HWND ShellFrame
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetProp routine retrieves a data handle from the property list of the specified window.
 *
 * \param WindowHandle A handle to the window whose property list is to be searched.
 * \param String A pointer to a null-terminated string or an atom that identifies a string.
 * \return If the property list contains the string, the return value is the associated data handle; otherwise, NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetProp(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String
    );

/**
 * The NtUserGetProp2 routine retrieves a data handle from the property list of the specified window using a Unicode string structure.
 *
 * \param WindowHandle A handle to the window whose property list is to be searched.
 * \param String A pointer to a UNICODE_STRING that identifies the property name.
 * \return If the property list contains the string, the return value is the associated data handle; otherwise, NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetProp2(
    _In_ HWND WindowHandle,
    _In_ PCUNICODE_STRING String
    );

// rev
/**
 * The NtUserGetScrollBarInfo routine retrieves information about the specified scroll bar.
 *
 * \param WindowHandle Handle to the window containing the scroll bar.
 * \param ObjectId The scroll bar object identifier (OBJID_HSCROLL, OBJID_VSCROLL, OBJID_CLIENT).
 * \param ScrollBarInfo Pointer to a SCROLLBARINFO structure that receives the information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetScrollBarInfo(
    _In_ HWND WindowHandle,
    _In_ ULONG ObjectId,
    _Inout_ PVOID ScrollBarInfo
    );

// rev
/**
 * Shared window data type selector used by NtUserGetSharedWindowData.
 *
 * \remarks Unlike GetWindowLongPtr / NtUserGetWindowLongPtr which operate directly on
 *          the client-mapped desktop heap (and are subject to desktop heap isolation
 *          restrictions across process and silo boundaries), NtUserGetSharedWindowData
 *          operates across modern Windows isolation boundaries (AppContainers, MinUser
 *          providers, and CoreMessaging subsystems).
 */
typedef enum _SHARED_WINDOW_DATA_TYPE
{
    SharedWindowDataWord = 0,
    SharedWindowDataDword = 1,
    SharedWindowDataPtr = 2,
    SharedWindowDataMax
} SHARED_WINDOW_DATA_TYPE;

#define SHARED_WINDOW_DATA_WORD 0
#define SHARED_WINDOW_DATA_DWORD 1
#define SHARED_WINDOW_DATA_PTR 2

#define WINDOW_DATA_TYPE_WORD 0
#define WINDOW_DATA_TYPE_DWORD 1
#define WINDOW_DATA_TYPE_PTR 2

// rev
/**
 * The NtUserGetSharedWindowData routine queries shared desktop or window data structures.
 *
 * \param WindowHandle Handle to the window to query.
 * \param Type Data type selector specifying the width of the data to retrieve (SharedWindowDataWord, SharedWindowDataDword, SharedWindowDataPtr).
 * \param SharedWindowData Provider-specific data buffer receiving the shared window data.
 * \param BufferSize Size of the data buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks Unlike GetWindowLongPtr which is subject to desktop heap isolation restrictions
 *          across process/silo boundaries, NtUserGetSharedWindowData works across modern
 *          Windows isolation boundaries (AppContainers, MinUser providers, CoreMessaging).
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserGetSharedWindowData(
    _In_ HWND WindowHandle,
    _In_ SHARED_WINDOW_DATA_TYPE Type,
    _Out_writes_bytes_(BufferSize) PVOID SharedWindowData,
    _In_ ULONG BufferSize
    );

/**
 * The NtUserGetTitleBarInfo routine retrieves information about the specified title bar.
 *
 * \param WindowHandle A handle to the window whose title bar is to be inspected.
 * \param pti Pointer to a TITLEBARINFO structure to receive the information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetTitleBarInfo(
    _In_ HWND WindowHandle,
    _Inout_ PTITLEBARINFO pti
    );

// rev
/**
 * The NtUserGetTopLevelWindow routine retrieves the top-level ancestor window of the specified window.
 *
 * \param WindowHandle Handle to the window whose top-level ancestor is retrieved.
 * \return HWND Handle to the top-level window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetTopLevelWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserGetUpdateRect routine retrieves the coordinates of the smallest rectangle that completely encloses the update region of the specified window.
 *
 * \param WindowHandle Handle to the window whose update region is to be retrieved.
 * \param Rect Optional pointer to the RECT structure that receives the coordinates of the update rectangle.
 * \param Erase Specifies whether the background in the update region should be erased.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetUpdateRect(
    _In_ HWND WindowHandle,
    _Out_opt_ PRECT Rect,
    _In_ ULONG Erase
    );

// rev
/**
 * The NtUserGetUpdateRgn routine retrieves the update region of a window by copying it into the specified region.
 *
 * \param WindowHandle Handle to the window with an update region that is to be retrieved.
 * \param RegionHandle Handle to the region which receives the update region.
 * \param Erase Specifies whether the window background should be erased.
 * \return LONG The region complexity: NULLREGION, SIMPLEREGION, COMPLEXREGION, or ERROR on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetUpdateRgn(
    _In_ HWND WindowHandle,
    _In_ HRGN RegionHandle,
    _In_ ULONG Erase
    );

// rev
/**
 * The NtUserGetWindowBand routine retrieves the Z-order band assignment for the specified window.
 *
 * \param WindowHandle Handle to the window to query.
 * \param Band Pointer to a variable receiving the Z-order band index (ZBID_*).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetWindowBand(
    _In_ HWND WindowHandle,
    _Out_ PULONG Band
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetWindowContextHelpId routine retrieves the context help identifier for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallHwnd(SFI_GETWINDOWCONTEXTHELPID) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetWindowContextHelpId(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetWindowFeedbackSetting routine retrieves the feedback configuration for a window.
 *
 * \param Hwnd Handle to the window to query.
 * \param Feedback The feedback type to retrieve (FEEDBACK_TYPE).
 * \param DwFlags Feedback query flags.
 * \param PSize Pointer to a variable holding the size in bytes of Config and receiving the required or copied size.
 * \param Config Optional pointer to a buffer receiving the feedback configuration.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetWindowFeedbackSetting.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetWindowFeedbackSetting(
    _In_ HWND Hwnd,
    _In_ FEEDBACK_TYPE Feedback,
    _In_ ULONG DwFlags,
    _Inout_ ULONG* PSize,
    _Out_writes_bytes_opt_(*PSize) VOID* Config
    );

// rev
/**
 * The NtUserGetWindowMinimizeRect routine retrieves the screen coordinates to which a window is minimized.
 *
 * \param WindowHandle Handle to the window being minimized.
 * \param MinimizeRect Pointer to a RECT structure receiving the minimize coordinates.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetWindowMinimizeRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT MinimizeRect
    );

/**
 * The NtUserGetWindowPlacement routine retrieves the show state and the restored, minimized, and maximized positions of a window.
 *
 * \param WindowHandle A handle to the window.
 * \param WindowPlacement Pointer to the WINDOWPLACEMENT structure that receives the show state and position information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetWindowPlacement(
    _In_ HWND WindowHandle,
    _Inout_ PWINDOWPLACEMENT WindowPlacement
    );

/**
 * The NtUserGetWindowProcessHandle routine opens a handle to the process that owns the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \param DesiredAccess The access mask requested for the process handle.
 * \return A handle to the window's owner process, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetWindowProcessHandle(
    _In_ HWND WindowHandle,
    _In_ ACCESS_MASK DesiredAccess
    );

// rev
/**
 * The NtUserGetWindowRgnEx routine retrieves the window region of a window with extended coordinate options.
 *
 * \param WindowHandle Handle to the window whose region is queried.
 * \param RegionHandle Handle to the region that receives the window region.
 * \param Flags Coordinate flags (e.g. 1 for client coordinates, 0 for window coordinates).
 * \return LONG The region complexity: NULLREGION, SIMPLEREGION, COMPLEXREGION, or ERROR on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetWindowRgnEx(
    _In_ HWND WindowHandle,
    _In_ HRGN RegionHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserGetWindowThreadProcessId routine retrieves the identifier of the thread that created the specified window and, optionally, the identifier of the process that created the window.
 *
 * \param WindowHandle Handle to the window.
 * \param ProcessId Optional pointer to a variable that receives the process identifier.
 * \return ULONG The identifier of the thread that created the window, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetWindowThreadProcessId(
    _In_ HWND WindowHandle,
    _Out_opt_ PULONG ProcessId
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetWindowTrackInfoAsync routine retrieves window tracking information asynchronously.
 *
 * \param WindowHandle Handle to the target window.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallHwndLock(SFI_GETWINDOWTRACKINFOASYNC) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetWindowTrackInfoAsync(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserHwndQueryRedirectionInfo routine queries redirection information for a window.
 *
 * \param WindowHandle A handle to the window to query.
 * \param Index The redirection information index to query.
 * \param Information A pointer to a buffer that receives the redirection information.
 * \param InfoLength Pointer to a variable that specifies the size of the buffer, and receives the returned data size.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHwndQueryRedirectionInfo(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _Out_writes_bytes_(*InfoLength) PVOID Information,
    _Inout_ PULONG InfoLength
    );

/**
 * The NtUserInternalGetWindowText routine copies the text of the specified window's title bar or body into a buffer.
 *
 * \param WindowHandle A handle to the window or control containing the text.
 * \param pString Pointer to the buffer that will receive the text.
 * \param cchMaxCount The maximum number of characters to copy to the buffer, including the null terminator.
 * \return The length of the string copied in characters, not including the terminating null character.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserInternalGetWindowText(
    _In_ HWND WindowHandle,
    _Out_writes_to_(cchMaxCount, return + 1) LPWSTR pString,
    _In_ ULONG cchMaxCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserIsInterceptWindow routine determines whether a window is configured as an input interception window.
 *
 * \param WindowHandle Handle to the window to test.
 * \param IsIntercept Receives TRUE if the window is an interception window, FALSE otherwise.
 * \return TRUE on success, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsInterceptWindow(
    _In_ HWND WindowHandle,
    _Out_ PBOOL IsIntercept
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserIsNonClientDpiScalingEnabled routine determines whether non-client high-DPI scaling is enabled for a window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsNonClientDpiScalingEnabled(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserIsResizeLayoutSynchronizationEnabled routine determines whether layout synchronization is enabled during window resize.
 *
 * \param WindowHandle Handle to the window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsResizeLayoutSynchronizationEnabled(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserIsTopLevelWindow routine determines whether the specified window is a top-level window.
 *
 * \param WindowHandle Handle to the window to test.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsTopLevelWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserIsWindowBroadcastingDpiToChildren routine determines whether DPI changes are automatically broadcast to child windows.
 *
 * \param WindowHandle Handle to the parent window to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsWindowBroadcastingDpiToChildren(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserQueryBSDRWindow routine retrieves the session's BSDR window for shutdown-block reason reporting.
 *
 * \return The registered window handle, or NULL if no window is registered.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserQueryBSDRWindow(
    VOID
    );

/**
 * The NtUserQueryWindow routine queries information about a window according to the specified information class.
 *
 * \param WindowHandle A handle to the window to query.
 * \param WindowInfo The window information class selector (WINDOWINFOCLASS).
 * \return Information about the window, such as PID, TID, or state flags depending on the information class.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserQueryWindow(
    _In_ HWND WindowHandle,
    _In_ WINDOWINFOCLASS WindowInfo
    );

// rev
/**
 * The NtUserSBGetParms routine retrieves scroll bar parameters and metrics for a standard scroll bar or scroll bar control.
 *
 * \param WindowHandle A handle to a scroll bar control or a window with a standard scroll bar.
 * \param ScrollBar Specifies the scroll bar type (SB_CTL, SB_HORZ, or SB_VERT).
 * \param ScrollBarData A pointer to the internal scroll bar data buffer.
 * \param ScrollInfo A pointer to a SCROLLINFO structure receiving the parameters.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSBGetParms(
    _In_ HWND WindowHandle,
    _In_ ULONG ScrollBar,
    _Inout_ PVOID ScrollBarData,
    _In_ ULONG_PTR ScrollInfo
    );

/**
 * The NtUserShutdownBlockReasonQuery routine retrieves the reason string set by NtUserShutdownBlockReasonCreate.
 *
 * \param WindowHandle A handle to the main window of the application.
 * \param Buffer Pointer to a buffer that receives the reason string.
 * \param BufferCount Pointer to a variable specifying buffer size in characters and receiving copied count.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserShutdownBlockReasonQuery(
    _In_ HWND WindowHandle,
    _Out_writes_opt_(*BufferCount) PWSTR Buffer,
    _Inout_ PULONG BufferCount
    );

/**
 * The NtUserValidateHandleSecure routine securely validates a window handle against a descriptor.
 *
 * \param ObjectHandle A handle to the object to validate.
 * \return TRUE if the handle is valid; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOLEAN
NTAPI
NtUserValidateHandleSecure(
    _In_ HANDLE ObjectHandle
    );

/**
 * The NtUserValidateRect routine validates the client area within a rectangle by removing the rectangle from the update region of the specified window.
 *
 * \param WindowHandle A handle to the window whose update region is to be modified.
 * \param Rect Pointer to a RECT structure that contains the client coordinates of the rectangle to be removed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserValidateRect(
    _In_opt_ HWND WindowHandle,
    _In_opt_ const RECT* Rect
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserValidateRgn routine validates a region of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param hrgn Handle to the region.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_VALIDATERGN) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserValidateRgn(
    _In_ HWND WindowHandle,
    _In_opt_ HRGN hrgn
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The QueryBSDRWindow routine retrieves the session's BSDR window for shutdown-block reason reporting.
 *
 * \return The registered window handle, or NULL if no window is registered.
 * \remarks Forwards to the NtUserQueryBSDRWindow system call.
 */
NTSYSAPI
HWND
NTAPI
QueryBSDRWindow(
    VOID
    );

// rev
/**
 * The TestWindowProcess routine tests whether the specified window belongs to the current process.
 *
 * \param WindowHandle A handle to the window.
 * \return TRUE if the window belongs to the current process; FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
TestWindowProcess(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The ValidateHwnd routine validates a window handle and returns a pointer to the client-mapped window object (WND).
 *
 * \param WindowHandle A handle to the window to validate.
 * \return A pointer to the client-mapped WND structure, or NULL if invalid.
 */
NTSYSAPI
PVOID
NTAPI
ValidateHwnd(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The WINNLSGetEnableStatus routine retrieves the Input Method Editor (IME) enable status for a window.
 *
 * \param WindowHandle Handle to the window to query.
 * \return BOOL TRUE if IME is enabled, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
WINNLSGetEnableStatus(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The CascadeChildWindows routine cascades the child windows of a parent window.
 *
 * \param ParentWindowHandle Handle to the parent window.
 * \param Flags Cascading layout flags (e.g. MDITILE_SKIPDISABLED).
 * \return ULONG_PTR Number of child windows cascaded.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CascadeChildWindows(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG Flags
    );

/**
 * The ChildWindowFromPoint routine determines which, if any, of the child windows belonging to a parent window contains the specified point.
 *
 * \param WindowHandle A handle to the parent window.
 * \param pt A structure that defines the client coordinates of the point to be checked.
 * \return A handle to the child window that contains the point, or WindowHandle if the point is within the parent window but not within any child window, or NULL.
 */
NTSYSAPI
HWND
NTAPI
ChildWindowFromPoint(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    );

// rev
/**
 * The DeferWindowPosAndBand routine defers a window position change together with a z-band change.
 *
 * \param DeferHandle Handle to a multiple-window-position structure.
 * \param WindowHandle Handle to the window to reposition.
 * \param ReferenceWindow Handle to the window that precedes the positioned window in the Z order.
 * \param PositionFlags Window positioning flags (SWP_*).
 * \param X New horizontal position of the window.
 * \param Y New vertical position of the window.
 * \param Width New width of the window.
 * \param Height New height of the window.
 * \param BandingFlags Banding configuration flags.
 * \param BandingOperation Banding operation code.
 * \return ULONG_PTR Handle to the updated deferred window position structure.
 * \remarks Forwards to the NtUserDeferWindowPosAndBand system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DeferWindowPosAndBand(
    _In_ HANDLE DeferHandle,
    _In_ HWND WindowHandle,
    _In_ HWND ReferenceWindow,
    _In_ ULONG PositionFlags,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ ULONG BandingFlags,
    _In_ LONG BandingOperation
    );

// rev
/**
 * The DelegateInput routine delegates input handling for a window to another thread or component.
 *
 * \param WindowHandle Handle to the window whose input is being delegated.
 * \param InputType Input event type to delegate.
 * \param Flags Delegation control flags.
 * \return ULONG_PTR Status code or previous delegation state.
 * \remarks Forwards to the NtUserDelegateInput system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DelegateInput(
    _In_ HWND WindowHandle,
    _In_ ULONG InputType,
    _In_ ULONG Flags
    );

// rev
/**
 * The DialogBoxIndirectParamAorW routine is the ANSI/Unicode-neutral core of DialogBoxIndirectParam.
 *
 * \param Instance Optional handle to the module instance containing the dialog template.
 * \param Template Pointer to a dialog box template.
 * \param OwnerWindow Optional handle to the dialog owner window.
 * \param DialogProc Optional pointer to the dialog box procedure.
 * \param InitParam Initialization value passed to the dialog procedure via WM_INITDIALOG.
 * \param Unicode TRUE for Unicode dialog handling; FALSE for ANSI.
 * \return INT_PTR Value passed to EndDialog when the dialog box was terminated.
 */
NTSYSAPI
INT_PTR
NTAPI
DialogBoxIndirectParamAorW(
    _In_opt_ HINSTANCE Instance,
    _In_ LPCDLGTEMPLATE Template,
    _In_opt_ HWND OwnerWindow,
    _In_opt_ DLGPROC DialogProc,
    _In_ LPARAM InitParam,
    _In_ BOOL Unicode
    );

// rev
/**
 * The DwmWindowNotificationsEnabled routine determines whether DWM window notifications are enabled.
 *
 * \param NotificationType Type of notification to query.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserDwmWindowNotificationsEnabled system call.
 */
NTSYSAPI
LOGICAL
NTAPI
DwmWindowNotificationsEnabled(
    _In_ ULONG NotificationType
    );

// rev
/**
 * The EditWndProc routine is the window procedure for the private Edit control class.
 *
 * \param WindowHandle Handle to the edit window.
 * \param Message Window message identifier.
 * \param wParam Additional message-specific parameter.
 * \param lParam Additional message-specific parameter.
 * \return ULONG_PTR Result of the message processing.
 */
NTSYSAPI
ULONG_PTR
NTAPI
EditWndProc(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// rev
/**
 * The EnableSynchronizedLayout routine enables synchronized layout for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserEnableSynchronizedLayout system call.
 */
NTSYSAPI
LOGICAL
NTAPI
EnableSynchronizedLayout(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The EnableWindowShellWindowManagementBehavior routine enables shell window-management behavior for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Mask Mask selecting behavior bits to update. Only bits 0x01, 0x02, and 0x04 participate.
 * \param Values New values for the selected bits. Higher bits are ignored.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Forwards to the NtUserEnableWindowShellWindowManagementBehavior system call.
 * Updates the window flags as (OldFlags & ~(Mask & 7)) | (Values & Mask & 7).
 * The caller must be the registered shell-management thread or have IAM access.
 * Failure sets ERROR_INVALID_WINDOW_HANDLE for an invalid window or ERROR_ACCESS_DENIED for insufficient access.
 */
_Success_(return != 0)
NTSYSAPI
LOGICAL
NTAPI
EnableWindowShellWindowManagementBehavior(
    _In_ HWND WindowHandle,
    _In_ ULONG Mask,
    _In_ ULONG Values
    );

// rev
/**
 * The EnterReaderModeHelper routine enters mouse reader (auto-scroll) mode for a window.
 *
 * \param WindowHandle Handle to the window entering reader mode.
 * \return HWND Handle to the created reader mode window or control.
 */
NTSYSAPI
HWND
NTAPI
EnterReaderModeHelper(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The ImpersonateDdeClientWindow routine impersonates the DDE client associated with a window.
 *
 * \param ClientWindowHandle Handle to the DDE client window.
 * \param ServerWindowHandle Handle to the DDE server window.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserImpersonateDdeClientWindow system call.
 */
NTSYSAPI
BOOL
NTAPI
ImpersonateDdeClientWindow(
    _In_ HWND ClientWindowHandle,
    _In_ HWND ServerWindowHandle
    );

// rev
/**
 * The MirrorRgn routine mirrors a region horizontally for right-to-left layout.
 *
 * \param WindowHandle A handle to the window.
 * \param RegionHandle A handle to the region to mirror.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
MirrorRgn(
    _In_ HWND WindowHandle,
    _In_ HRGN RegionHandle
    );

// rev
/**
 * The NotifyOverlayWindow routine notifies an overlay window of a state change.
 *
 * \param WindowHandle Handle to the overlay window.
 * \param Enable TRUE to enable overlay notifications; FALSE to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserNotifyOverlayWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
NotifyOverlayWindow(
    _In_ HWND WindowHandle,
    _In_ LOGICAL Enable
    );

// rev
/**
 * The NtCompositorNotifyExitWindows routine notifies the compositor that Windows is shutting down or logging off.
 *
 * \param Param1 First notification parameter.
 * \param Param2 Second notification parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompositorNotifyExitWindows(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtUserAcquireInteractiveControlBackgroundAccess routine acquires background input access for interactive controls.
 *
 * \param DeviceId Identifier of the interactive control device.
 * \param Usage Usage code or page for the interactive control.
 * \param WindowHandle Optional handle to the target window receiving background access.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAcquireInteractiveControlBackgroundAccess(
    _In_ ULONG DeviceId,
    _In_ ULONG Usage,
    _In_opt_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserAllowSetForegroundWindow routine allows the specified process to set the foreground window.
 *
 * \param ProcessId The process id.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_ALLOWSETFOREGROUNDWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAllowSetForegroundWindow(
    _In_ ULONG ProcessId
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserAlterWindowStyle routine sets and clears style bits on a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param SetFlags Style bits to set.
 * \param ClearFlags Style bits to clear.
 * \return TRUE on success, FALSE otherwise.
 *
 * \remarks Signature validated against win32k.sys (syscall 0x10C0; implementation at 0x14001209C):
 * the routine consumes exactly 3 arguments - RCX (HWND), EDX and R8D (32-bit); R9 is not an input.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAlterWindowStyle(
    _In_ HWND WindowHandle,
    _In_ ULONG SetFlags,
    _In_ ULONG ClearFlags
    );

// rev
/**
 * The NtUserApplyWindowAction routine applies a batched window-action descriptor to a window.
 *
 * \param WindowActions Handle to the window actions target.
 * \param Source Pointer to a WINDOW_ACTION descriptor describing the action.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Native entry point for USER32!ApplyWindowAction.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserApplyWindowAction(
    HWND WindowActions,
    WINDOW_ACTION *Source
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserBeginDeferWindowPos routine begins a deferred window-position update session.
 *
 * \param NumWindowsHint The num windows hint.
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallOneParam(SFI_BEGINDEFERWINDOWPOS) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HDWP
NTAPI
NtUserBeginDeferWindowPos(
    _In_ ULONG NumWindowsHint
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserBeginLayoutUpdate routine initiates a window layout update transaction.
 *
 * \param WindowHandle Handle to the window whose layout update is beginning.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserBeginLayoutUpdate(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserCalculatePopupWindowPosition routine calculates a valid popup window position based on an anchor point and exclusion rectangle.
 *
 * \param anchorPoint The anchor point for the popup window.
 * \param windowSize The size of the popup window.
 * \param flags Flags controlling popup positioning.
 * \param excludeRect Pointer to exclusion rectangle that the popup should avoid.
 * \param popupWindowPosition Receives the calculated popup window coordinates.
 * \return TRUE on success; FALSE on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserCalculatePopupWindowPosition(
    _In_ const POINT* anchorPoint,
    _In_ const SIZE* windowSize,
    _In_ ULONG flags,
    _In_opt_ RECT* excludeRect,
    _Out_ RECT* popupWindowPosition
    );

/**
 * The NtUserChildWindowFromPointEx routine determines which, if any, of the child windows belonging to the specified parent window contains the specified point, with skip options.
 *
 * \param WindowHandle A handle to the parent window.
 * \param Point A structure that defines the client coordinates of the point to be checked.
 * \param Flags Flags specifying which child windows to skip (e.g. CWP_SKIPINVISIBLE, CWP_SKIPDISABLED, CWP_SKIPTRANSPARENT).
 * \return A handle to the child window that contains the point and meets the criteria, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserChildWindowFromPointEx(
    _In_ HWND WindowHandle,
    _In_ POINT Point,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserClearWindowState routine clears the specified window state flags.
 *
 * \param WindowHandle Handle to the target window.
 * \param Flag Window state flag bits to configure (WF*, WEF*, BF*, DF*, CBF*, EF*, SF*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_CLEARWINDOWSTATE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserClearWindowState(
    _In_ HWND WindowHandle,
    _In_ ULONG Flag // WF*, WEF*, BF*, DF*, CBF*, EF*, SF*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserConfirmResizeCommit routine confirms that a window resize operation has completed and commits its layout.
 *
 * \param WindowHandle Handle to the window whose resize is confirmed.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserConfirmResizeCommit(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserConvertToInterceptWindow routine converts a top-level window into an input interception window.
 *
 * \param TopLevelWindow Handle to the top-level window to convert.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!ConvertToInterceptWindow.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserConvertToInterceptWindow(
    _In_ HWND TopLevelWindow
    );

// rev
/**
 * The NtUserDefSetText routine sets the default window title or text for a window.
 *
 * \param WindowHandle Handle to the window whose text is to be set.
 * \param String Optional pointer to a UNICODE_STRING or text buffer containing the new window text.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDefSetText(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID String
    );

// rev
/**
 * The NtUserDeferWindowDpiChanges routine defers DPI-scaling updates for the specified window hierarchy.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDeferWindowDpiChanges(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserDeferWindowPosAndBand routine updates multiple window positions and Z-order bands in a single batch operation.
 *
 * \param WindowPosInfo Handle to a multiple-window-position structure.
 * \param WindowHandle Handle to the window whose position or band is updated.
 * \param InsertAfterWindowHandle Optional handle to the window that precedes the positioned window in the Z-order.
 * \param X New horizontal position of the window.
 * \param Y New vertical position of the window.
 * \param Width New width of the window, in pixels.
 * \param Height New height of the window, in pixels.
 * \param Flags Window positioning flags (SWP_*).
 * \param Band Target Z-order band index (ZBID_*).
 * \param Param10 Reserved or additional position parameter.
 * \return HDWP A handle to the updated multiple-window-position structure, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDWP
NTAPI
NtUserDeferWindowPosAndBand(
    _In_ HDWP WindowPosInfo,
    _In_ HWND WindowHandle,
    _In_opt_ HWND InsertAfterWindowHandle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ ULONG Flags,
    _In_ LONG Band,
    _In_ LONG Param10
    );

// rev
/**
 * The NtUserDelegateInput routine delegates input processing for a window to a specified thread.
 *
 * \param ThreadId Identifier of the thread receiving delegated input.
 * \param Param2 Delegation control parameter or input source identifier.
 * \param Param3 Delegation flags or context parameter.
 * \param WindowHandle Handle to the window whose input is delegated.
 * \param Flags Additional delegation control flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDelegateInput(
    _In_ ULONG ThreadId,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _In_ HWND WindowHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserDisableImmersiveOwner routine disables immersive (UWP) window ownership relationships for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDisableImmersiveOwner(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtUserDisableProcessWindowFiltering routine disables window filtering
 * so you can enumerate immersive windows from the desktop.
 *
 * \return BOOL value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests#disableWindowFiltering
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDisableProcessWindowFiltering(
    VOID
    );

#endif

/**
 * The NtUserDragDetect routine captures the mouse and tracks its movement until the user releases the left button, presses ESC, or moves mouse outside drag rectangle.
 *
 * \param WindowHandle A handle to the window receiving mouse input.
 * \param pt Initial position of the mouse, in screen coordinates.
 * \return TRUE if the user moved the mouse outside the drag rectangle while holding down the left button; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDragDetect(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    );

// rev
/**
 * The NtUserDrainThreadCoreMessagingCompletions2 routine processes and drains pending CoreMessaging completion packets for a thread.
 *
 * \param WindowHandle Handle to the window associated with the CoreMessaging queue.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrainThreadCoreMessagingCompletions2(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserDrawAnimatedRects routine animates the caption of a window to indicate the opening of an icon or the minimizing or maximizing of a window.
 *
 * \param WindowHandle A handle to the window whose caption should be animated.
 * \param idAni The animation type (e.g. IDANI_CAPTION).
 * \param lprcFrom Pointer to a RECT structure specifying the location and size of the icon or minimized window.
 * \param lprcTo Pointer to a RECT structure specifying the location and size of the restored window.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDrawAnimatedRects(
    _In_opt_ HWND WindowHandle,
    _In_ int idAni,
    _In_ const RECT* lprcFrom,
    _In_ const RECT* lprcTo
    );

// rev
/**
 * The NtUserDrawCaption routine draws a window caption into the specified device context.
 *
 * \param WindowHandle Handle to the window whose caption is drawn.
 * \param Hdc Handle to the destination device context.
 * \param Rect Pointer to a RECT structure defining the bounding rectangle for the caption.
 * \param Flags Caption drawing flags (DC_*).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawCaption(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserDwmWindowNotificationsEnabled routine enables or disables DWM window state notifications.
 *
 * \param Enable Non-zero to enable notifications; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDwmWindowNotificationsEnabled(
    _In_ LONG Enable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnableModernAppWindowKeyboardIntercept routine enables keyboard interception for a modern app window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable TRUE to enable; FALSE to disable.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_ENABLEMODERNAPPWINDOWKEYBOARDINTERCEPT) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableModernAppWindowKeyboardIntercept(
    _In_ HWND WindowHandle,
    _In_ LOGICAL Enable
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnableNonClientDpiScaling routine enables non-client area high-DPI scaling for the specified window.
 *
 * \param Hwnd Handle to the window whose non-client area should be scaled.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!EnableNonClientDpiScaling.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEnableNonClientDpiScaling(
    _In_ HWND Hwnd
    );

// rev
/**
 * The NtUserEnableResizeLayoutSynchronization routine enables or disables synchronized layout updates during window resize operations.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable Non-zero to enable synchronization; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableResizeLayoutSynchronization(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserEnableScrollBar routine enables or disables one or both scroll bar arrows.
 *
 * \param WindowHandle Handle to a window or a scroll bar control.
 * \param ScrollBar Scroll bar type specifier (SB_HORZ, SB_VERT, SB_CTL, SB_BOTH).
 * \param Arrows Arrow enable flags (ESB_ENABLE_BOTH, ESB_DISABLE_BOTH, ESB_DISABLE_LTUP, ESB_DISABLE_RTDN).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableScrollBar(
    _In_ HWND WindowHandle,
    _In_ ULONG ScrollBar,
    _In_ ULONG Arrows
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnableShellWindowManagementBehavior routine enables shell window-management behavior.
 *
 * \param Mask Mask selecting the current desktop's behavior bits to update.
 * \param Values New values for the selected bits. Only bits in 0x700007FF are accepted.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_ENABLESHELLWINDOWMANAGEMENTBEHAVIOR) before WIN11.
 * The new flags are (OldFlags & ~Mask) | (Values & Mask). Values containing bits outside
 * 0x700007FF fail with ERROR_INVALID_PARAMETER; Values containing 0x0C trigger telemetry.
 * The calling thread must have IAM access, otherwise the call fails with ERROR_ACCESS_DENIED.
 * A nonzero result requires registered shell window management. If it is not registered,
 * the stored flags are cleared and the call fails with ERROR_INVALID_STATE.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableShellWindowManagementBehavior(
    _In_ ULONG Mask,
    _In_ ULONG Values
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnableSynchronizedLayout routine enables synchronized layout processing for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableSynchronizedLayout(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnableWindow routine enables or disables mouse and keyboard input to the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable TRUE to enable; FALSE to disable.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock[Safe](SFI_ENABLEWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableWindow(
    _In_ HWND WindowHandle,
    _In_ LOGICAL Enable
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * Configures session-wide GDI window-resize optimization values.
 * \param Param1 First 32-bit session setting; not a window handle.
 * \param Param2 Second 32-bit session setting; semantics unconfirmed.
 * \param Param3 Third 32-bit session setting; semantics unconfirmed.
 * \return The inspected implementation returns one, including when updates are suppressed.
 * \remarks GreEnableWindowResizeOptimization stores all three values only when its session
 * guard is zero. The meanings and units of these settings remain unconfirmed.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableWindowResizeOptimization(
    _In_ ULONG Param1,
    _In_ ULONG Param2,
    _In_ ULONG Param3
    );

// rev
/**
 * The NtUserEnableWindowShellWindowManagementBehavior routine updates shell window-management behavior for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Mask Mask selecting behavior bits to update. Only bits 0x01, 0x02, and 0x04 participate.
 * \param Values New values for the selected bits. Higher bits are ignored.
 * \return LOGICAL TRUE on success, FALSE otherwise.
 *          Updates the window flags as (OldFlags & ~(Mask & 7)) | (Values & Mask & 7).
 *          The caller must be the registered shell-management thread or have IAM access.
 *          Failure sets ERROR_INVALID_WINDOW_HANDLE for an invalid window or ERROR_ACCESS_DENIED for insufficient access.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableWindowShellWindowManagementBehavior(
    _In_ HWND WindowHandle,
    _In_ ULONG Mask,
    _In_ ULONG Values
    );

// rev
/**
 * The NtUserEnterMoveSizeLoop routine enters the modal window moving or sizing loop for the specified window.
 *
 * \param Hwnd Handle to the window entering the move or size loop.
 * \param PtCursor Starting cursor screen coordinates.
 * \param MoveSizeCode Move/size operation code indicating tracking behavior.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!EnterMoveSizeLoop.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEnterMoveSizeLoop(
    HWND Hwnd,
    POINT PtCursor,
    MOVESIZE_OPERATION MoveSizeCode
    );

// rev
/**
 * The NtUserExcludeUpdateRgn routine prevents drawing within invalid areas of a window by subtracting the updated region from the clipping region.
 *
 * \param HDC A handle to the device context associated with the clipping region.
 * \param WindowHandle A handle to the window that is being updated.
 * \return The return value specifies the complexity of the excluded region (SIMPLEREGION, COMPLEXREGION, NULLREGION, or ERROR).
 * \remarks Native entry point for USER32!ExcludeUpdateRgn.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserExcludeUpdateRgn(
    _In_ HDC HDC,
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserFillWindow routine paints the client area or background of a window using a brush.
 *
 * \param BrushWindowHandle Optional handle to the window providing brush context.
 * \param WindowHandle Handle to the window whose surface is filled.
 * \param Hdc Handle to the destination device context.
 * \param BrushHandle Optional handle to the brush used to fill the window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserFillWindow(
    _In_opt_ HWND BrushWindowHandle,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_opt_ HBRUSH BrushHandle
    );

/**
 * The NtUserFlashWindowEx routine flashes the specified window.
 *
 * \param pfwi Pointer to a FLASHWINFO structure.
 * \return TRUE if the window was active before the call; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserFlashWindowEx(
    _In_ PFLASHWINFO pfwi
    );

/**
 * The NtUserHwndSetRedirectionInfo routine sets redirection information for a window.
 *
 * \param WindowHandle A handle to the window to modify.
 * \param Index The redirection information index to set.
 * \param Information A pointer to a buffer containing the redirection information.
 * \param InfoLength The size of the buffer in bytes.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHwndSetRedirectionInfo(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_reads_bytes_(InfoLength) PVOID Information,
    _In_ ULONG InfoLength
    );

// rev
/**
 * The NtUserImpersonateDdeClientWindow routine impersonates a Dynamic Data Exchange (DDE) client application window.
 *
 * \param HWndClient Handle to the DDE client window to impersonate.
 * \param HWndServer Handle to the DDE server window.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!ImpersonateDdeClientWindow.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserImpersonateDdeClientWindow(
    HWND HWndClient,
    HWND HWndServer
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserInitThreadCoreMessagingIocp routine initializes core-messaging I/O completion for the calling thread.
 *
 * \param WindowHandle Handle associated with the thread's message queue.
 * \return Handle to the initialized completion port, or NULL on failure.
 * \remarks Exposed via NtUserCallHwnd[Safe](SFI_INITTHREADCOREMESSAGINGIOCP) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserInitThreadCoreMessagingIocp(
    _In_opt_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserInitThreadCoreMessagingIocp2 routine initializes CoreMessaging I/O completion port integration for a thread.
 *
 * \param WindowHandle Handle to the window associated with the CoreMessaging queue.
 * \param Result Pointer to a variable receiving the initialization result or completion port handle.
 * \return HANDLE A handle to the thread completion port, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserInitThreadCoreMessagingIocp2(
    _In_ HWND WindowHandle,
    _Out_ PVOID Result
    );

/**
 * The NtUserInvalidateRect routine adds a rectangle to the specified window's update region.
 *
 * \param WindowHandle A handle to the window whose update region has changed.
 * \param Rect Pointer to a RECT structure containing the client coordinates of the rectangle to add. If NULL, entire client area is added.
 * \param Erase Specifies whether the background within the update region is to be erased when the update region is processed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInvalidateRect(
    _In_opt_ HWND WindowHandle,
    _In_opt_ const RECT* Rect,
    _In_ BOOL Erase
    );

/**
 * The NtUserInvalidateRgn routine invalidates the client area within the specified region by adding it to the current update region of a window.
 *
 * \param WindowHandle A handle to the window with an update region that is to be modified.
 * \param RgnHandle A handle to the region to be added to the update region.
 * \param Erase Specifies whether the background within the update region is to be erased.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInvalidateRgn(
    _In_ HWND WindowHandle,
    _In_opt_ HRGN RgnHandle,
    _In_ BOOL Erase
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserLayoutCompleted routine notifies the window manager that layout has completed for the specified window.
 *
 * \param WindowHandle Handle to the window whose layout completed.
 * \return TRUE on success, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLayoutCompleted(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserLockSetForegroundWindow routine locks or unlocks foreground-window changes.
 *
 * \param LockCode The lock code (LSFW_* WinUser.h).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_LOCKSETFOREGROUNDWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLockSetForegroundWindow(
    _In_ ULONG LockCode // LSFW_* WinUser.h
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserLockWindowUpdate routine disables or enables drawing in the specified window.
 *
 * \param HWndLock Handle to the window whose updates are locked, or NULL to unlock.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!LockWindowUpdate.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserLockWindowUpdate(
    _In_opt_ HWND HWndLock
    );

// rev
/**
 * The NtUserLogicalToPhysicalDpiPointForWindow routine converts a point from logical to physical coordinates for a specific window DPI context.
 *
 * \param WindowHandle Handle to the target window.
 * \param Point Pointer to a POINT structure containing logical coordinates to convert and receiving physical coordinates.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLogicalToPhysicalDpiPointForWindow(
    _In_ HWND WindowHandle,
    _Inout_ PPOINT Point
    );

/**
 * The NtUserLogicalToPhysicalPoint routine converts the logical coordinates of a point in a window to physical coordinates.
 *
 * \param WindowHandle A handle to the window whose transform is used for the conversion.
 * \param lpPoint Pointer to a POINT structure that specifies the logical coordinates to be converted and receives the physical coordinates.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserLogicalToPhysicalPoint(
    _In_ HWND WindowHandle,
    _Inout_ LPPOINT lpPoint
    );

// rev
/**
 * The NtUserMarkWindowForRawMouse routine designates a window to receive raw mouse input.
 *
 * \param WindowHandle Handle to the window to be marked for raw mouse input.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserMarkWindowForRawMouse(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserMinMaximize routine minimizes or maximizes a window with animation options.
 *
 * \param WindowHandle Handle to the target window.
 * \param ShowCommand Window display command (SW_MINIMIZE, SW_MAXIMIZE, SW_RESTORE).
 * \param Flags Minimization or maximization animation control flags.
 * \return NTSTATUS value; the result contract remains unverified.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserMinMaximize(
    _In_ HWND WindowHandle,
    _In_ ULONG ShowCommand,
    _In_ LONG Flags
    );

/**
 * The NtUserMoveWindow routine changes the position and dimensions of the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \param X The new position of the left side of the window.
 * \param Y The new position of the top of the window.
 * \param Width The new width of the window.
 * \param Height The new height of the window.
 * \param Repaint Specifies whether the window is to be repainted.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserMoveWindow(
    _In_ HWND WindowHandle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ BOOL Repaint
    );

// rev
/**
 * The NtUserNavigateFocus routine navigates keyboard focus between controls or components.
 *
 * \param WindowHandle Handle to the base or container window.
 * \param Param2 Focus navigation direction or target identifier.
 * \return HWND A handle to the newly focused window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserNavigateFocus(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR Param2
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserNotifyOverlayWindow routine notifies the overlay window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable TRUE to enable; FALSE to disable.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_NOTIFYOVERLAYWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNotifyOverlayWindow(
    _In_ HWND WindowHandle,
    _In_ LOGICAL Enable
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserNotifyWinEvent routine signals an accessibility event to the system.
 *
 * \param Event The event constant describing the occurrence (EVENT_*).
 * \param WindowHandle Handle to the window that generates the event.
 * \param ObjectId The object identifier associated with the event (OBJID_*).
 * \param ChildId Identifies whether the event was triggered by an object or an element within the object.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNotifyWinEvent(
    _In_ ULONG Event,
    _In_ HWND WindowHandle,
    _In_ LONG ObjectId,
    _In_ LONG ChildId
    );

// rev
/**
 * The NtUserPhysicalToLogicalDpiPointForWindow routine converts a screen point from physical coordinates to logical coordinates for a specific window.
 *
 * \param WindowHandle Handle to the window whose DPI context is used.
 * \param Point Pointer to a POINT structure containing physical coordinates and receiving logical coordinates.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPhysicalToLogicalDpiPointForWindow(
    _In_ HWND WindowHandle,
    _Inout_ PPOINT Point
    );

/**
 * The NtUserPhysicalToLogicalPoint routine converts the physical coordinates of a point in a window to logical coordinates.
 *
 * \param WindowHandle A handle to the window whose transform is used for the conversion.
 * \param Point Pointer to a POINT structure that specifies the physical coordinates to be converted and receives the logical coordinates.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserPhysicalToLogicalPoint(
    _In_ HWND WindowHandle,
    _Inout_ LPPOINT Point
    );

/**
 * The NtUserPrintWindow routine copies a visual window into the specified device context (DC).
 *
 * \param WindowHandle A handle to the window that will be copied.
 * \param Hdc A handle to the device context.
 * \param Flags The drawing options (e.g. PW_CLIENTONLY).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserPrintWindow(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ ULONG Flags
    );

/**
 * The NtUserRaiseLowerShellWindow routine raises or lowers the shell window in the Z-order.
 *
 * \param WindowHandle A handle to the shell window.
 * \param SetWithOptions Specifies whether options are applied during the Z-order change.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRaiseLowerShellWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN SetWithOptions
    );

/**
 * The NtUserRealChildWindowFromPoint routine determines which child window contains the specified client coordinates.
 *
 * \param WindowHandleParent A handle to the parent window.
 * \param ptParentClientCoords A POINT structure that defines the client coordinates of the point to be checked.
 * \return A handle to the child window that contains the specified point, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserRealChildWindowFromPoint(
    _In_ HWND WindowHandleParent,
    _In_ POINT ptParentClientCoords
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRedrawFrame routine redraws the non-client frame of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_REDRAWFRAME) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawFrame(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRedrawTitle routine redraws the title bar of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Flags Caption drawing flags (DC_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_REDRAWTITLE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawTitle(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags // DC_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserRedrawWindow routine updates the specified rectangle or region in a window's client area.
 *
 * \param WindowHandle A handle to the window to be redrawn.
 * \param UpdateRect Pointer to a RECT structure containing the coordinates of the update rectangle.
 * \param hrgnUpdate A handle to the update region.
 * \param Flags Redraw flags (e.g. RDW_INVALIDATE, RDW_ERASE, RDW_ALLCHILDREN).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRedrawWindow(
    _In_opt_ HWND WindowHandle,
    _In_opt_ const RECT* UpdateRect,
    _In_opt_ HRGN hrgnUpdate,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserRestoreWindowDpiChanges routine restores previously modified or overridden DPI scaling states for the specified window hierarchy.
 *
 * \param WindowHandle A handle to the window whose DPI state is to be restored.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRestoreWindowDpiChanges(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserScheduleDispatchNotification routine schedules a dispatch notification for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwnd[Safe](SFI_SCHEDULEDISPATCHNOTIFICATION) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserScheduleDispatchNotification(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserScrollWindowEx routine scrolls the contents of the specified window's client area with extended options.
 *
 * \param WindowHandle A handle to the window whose client area is to be scrolled.
 * \param Dx The amount, in device units, of horizontal scrolling.
 * \param Dy The amount, in device units, of vertical scrolling.
 * \param ScrollRect An optional pointer to a RECT structure that specifies the portion of the client area to be scrolled.
 * \param ClipRect An optional pointer to a RECT structure that contains the coordinates of the clipping rectangle.
 * \param UpdateRegion An optional handle to the region modified to hold the uncovered area.
 * \param UpdateRect An optional pointer to a RECT structure that receives the bounding rectangle of the update region.
 * \param Flags Scrolling flags (e.g. SW_SCROLLCHILDREN, SW_INVALIDATE, SW_ERASE, SW_SMOOTHSCROLL).
 * \return The return value is SIMPLEREGION, COMPLEXREGION, or NULLREGION; otherwise, ERROR.
 */
_Kernel_entry_
NTSYSCALLAPI
HRGN
NTAPI
NtUserScrollWindowEx(
    _In_ HWND WindowHandle,
    _In_ LONG Dx,
    _In_ LONG Dy,
    _In_opt_ PRECT ScrollRect,
    _In_opt_ PRECT ClipRect,
    _In_opt_ HRGN UpdateRegion,
    _Out_opt_ PRECT UpdateRect,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserSetActivationFilter routine sets the activation filter value for a window.
 *
 * \param WindowHandle Handle to a window owned by an IAM thread.
 * \param FilterFlags Filter value stored in full without mask validation in the examined implementation.
 * \return TRUE when an entry is updated, FALSE otherwise.
 * \remarks Requires IAM access, otherwise ERROR_ACCESS_DENIED is set. An invalid window or a
 *          window not owned by an IAM thread causes ERROR_INVALID_PARAMETER. A nonzero value creates an
 *          entry when absent. Zero updates an existing entry to zero, but returns FALSE if no entry exists.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetActivationFilter(
    _In_ HWND WindowHandle,
    _In_ ULONG FilterFlags
    );

/**
 * The NtUserSetActiveWindow routine makes the specified window the active window for the calling thread.
 *
 * \param WindowHandle A handle to the top-level window to be activated.
 * \return The handle to the window that was previously active, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserSetActiveWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserSetAdditionalForegroundBoostProcesses routine associates additional processes with a window for foreground priority boosting.
 *
 * \param WindowHandle A handle to the window.
 * \param Count The number of process handles in the Processes array.
 * \param Processes Array of process handles to boost.
 * \return TRUE on success, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetAdditionalForegroundBoostProcesses(
    HWND WindowHandle,
    ULONG Count,
    _In_reads_(Count) HANDLE *Processes
    );

// rev
/**
 * The NtUserSetAdditionalPowerThrottlingProcess routine associates additional processes with a window for power-throttling management.
 *
 * \param WindowHandle Handle to the target window.
 * \param ProcessHandlesCount Number of entries in the ProcessHandles array.
 * \param ProcessHandles Array of process handles to associate.
 * \return ULONG Zero on success, otherwise a nonzero error code.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserSetAdditionalPowerThrottlingProcess(
    _In_ HWND WindowHandle,
    _In_ ULONG ProcessHandlesCount,
    _In_reads_(ProcessHandlesCount) PHANDLE ProcessHandles
    );

// rev
/**
 * The NtUserSetBridgeWindowChild routine sets the child window for a cross-process or bridge hosting window container.
 *
 * \param WindowHandle A handle to the bridge hosting parent window.
 * \param ChildWindowHandle A handle to the hosted child window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetBridgeWindowChild(
    _In_ HWND WindowHandle,
    _In_ HWND ChildWindowHandle
    );

// rev
/**
 * The NtUserSetBrokeredForeground routine sets foreground activation on a window via the brokered foreground policy mechanism.
 *
 * \param WindowHandle A handle to the window receiving brokered foreground focus.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetBrokeredForeground(
    _In_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetCancelRotationDelayHintWindow routine sets the cancel-rotation delay-hint window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_SETCANCELROTATIONDELAYHINTWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCancelRotationDelayHintWindow(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserSetCapture routine sets the mouse capture to the specified window belonging to the current thread.
 *
 * \param WindowHandle A handle to the window in the current thread that is to capture the mouse.
 * \return A handle to the window that previously had captured the mouse, or NULL if no such window exists.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserSetCapture(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserSetChildWindowNoActivate routine designates a child window to be positioned or configured without activation.
 *
 * \param WindowHandle A handle to the child window.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetChildWindowNoActivate(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserSetClassLong routine replaces the specified 32-bit (LONG) value at the specified offset into the extra class memory or class structure.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based offset to the value to be replaced (e.g. GCL_STYLE, GCL_CBCLSEXTRA).
 * \param Value The replacement value.
 * \param Ansi Nonzero if handling ANSI window classes; zero for Unicode.
 * \return LONG_PTR The previous value of the specified offset, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG_PTR
NTAPI
NtUserSetClassLong(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_ ULONG Value,
    _In_ ULONG Ansi
    );

/**
 * The NtUserSetClassWord routine replaces the 16-bit (two-byte) value at the specified offset into the extra class memory.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based byte offset of the value to be replaced.
 * \param NewWord The replacement value.
 * \return The previous 16-bit value, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
USHORT
NTAPI
NtUserSetClassWord(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ USHORT NewWord
    );

// rev
/**
 * The NtUserSetCoreWindow routine sets the core window hosting state or status flags for an application frame window.
 *
 * \param WindowHandle A handle to the core application window.
 * \param Status Core window status flags or configuration state.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCoreWindow(
    _In_ HWND WindowHandle,
    _In_ ULONG Status
    );

// rev
/**
 * The NtUserSetCoreWindowPartner routine sets a partner window relationship for an application frame or core window.
 *
 * \param WindowHandle A handle to the core application window.
 * \param PartnerType The partner relationship classification type.
 * \param PartnerWindowHandle A handle to the partner window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCoreWindowPartner(
    _In_ HWND WindowHandle,
    _In_ LONG PartnerType,
    _In_ HWND PartnerWindowHandle
    );

// rev
/**
 * The NtUserSetCoveredWindowStates routine sets covered and occlusion visibility states for an array of windows.
 *
 * \param WindowStates A pointer to an array of window state structures.
 * \param Count The number of elements in the WindowStates array.
 * \param Covered Nonzero to mark windows as covered; zero otherwise.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCoveredWindowStates(
    _In_reads_(Count) PVOID WindowStates,
    _In_ ULONG Count,
    _In_ LONG Covered
    );

// rev
/**
 * The NtUserSetDialogControlDpiChangeBehavior routine overrides the per-monitor DPI scaling behavior of a child window in a dialog.
 *
 * \param HWnd A handle to the dialog control window.
 * \param Mask A mask specifying the behavior flags to modify.
 * \param Values The DPI change behavior values to apply.
 * \return TRUE if the operation was successful, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetDialogControlDpiChangeBehavior.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetDialogControlDpiChangeBehavior(
    _In_ HWND HWnd,
    _In_ DIALOG_CONTROL_DPI_CHANGE_BEHAVIORS Mask,
    _In_ DIALOG_CONTROL_DPI_CHANGE_BEHAVIORS Values
    );

// rev
/**
 * The NtUserSetDpiForWindow routine sets an explicit DPI value for the specified window, triggering internal rescaling messages.
 *
 * \param WindowHandle A handle to the window whose DPI is being set.
 * \param Dpi The new DPI value to apply to the window.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetDpiForWindow(
    _In_ HWND WindowHandle,
    _In_ ULONG Dpi
    );

/**
 * The NtUserSetFocus routine sets the keyboard focus to the specified window.
 *
 * \param WindowHandle A handle to the window that will receive the keyboard input.
 * \return The handle to the window that previously had the keyboard focus, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserSetFocus(
    _In_opt_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetForegroundWindow routine brings the specified window to the foreground.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_SETFOREGROUNDWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetForegroundWindow(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserSetForegroundWindowForApplication routine brings the specified application window to the foreground.
 *
 * \param WindowHandle A handle to the application window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetForegroundWindowForApplication(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserSetInternalWindowPos routine sets the internal placement coordinates, dimensions, and show command for a window.
 *
 * \param WindowHandle A handle to the window to reposition.
 * \param ShowCommand The show window command (e.g. SW_SHOW, SW_MINIMIZE, SW_MAXIMIZE).
 * \param Rectangle A pointer to a RECT structure specifying window bounds.
 * \param Point A pointer to a POINT structure specifying the client origin.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetInternalWindowPos(
    _In_ HWND WindowHandle,
    _In_ ULONG ShowCommand,
    _In_ PRECT Rectangle,
    _In_ PPOINT Point
    );

/**
 * The NtUserSetLayeredWindowAttributes routine sets the opacity and transparency color key of a layered window.
 *
 * \param WindowHandle A handle to the layered window.
 * \param Key A COLORREF structure that specifies the transparency color key.
 * \param Alpha Value for the blend function used to describe the opacity of the layered window.
 * \param Flags Layering action flags (LWA_COLORKEY, LWA_ALPHA).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
    _In_ HWND WindowHandle,
    _In_ COLORREF Key,
    _In_ BYTE Alpha,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserSetMirrorRendering routine configures mirrored horizontal rendering for right-to-left layout orientations.
 *
 * \param WindowHandle A handle to the window to configure.
 * \param Enable Nonzero to enable mirror rendering; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetMirrorRendering(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetModernAppWindow routine sets the modern (immersive) app window.
 *
 * \param ShellFrame The shell frame.
 * \param App Handle to the modern application window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_SETMODERNAPPWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetModernAppWindow(
    _In_ HWND ShellFrame,
    _In_ HWND App
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetParent routine changes the parent window of the specified child window.
 *
 * \param WindowHandle A handle to the child window whose parent is being changed.
 * \param ParentWindowHandle An optional handle to the new parent window, or NULL to make it a top-level window.
 * \return A handle to the previous parent window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserSetParent(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetProgmanWindow routine sets the Program Manager window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndOpt(SFI_SETPROGMANWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProgmanWindow(
    _In_opt_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetProp routine adds a new entry or changes an existing entry in the property list of the specified window.
 *
 * \param WindowHandle A handle to the window whose property list receives the new entry.
 * \param Atom An atom identifying the property string.
 * \param Data A handle to the data to be copied to the property list.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProp(
    _In_ HWND WindowHandle,
    _In_ LONG Atom,
    _In_ HANDLE Data
    );

// rev
/**
 * The NtUserSetProp2 routine adds or replaces a window property entry by Unicode string name.
 *
 * \param WindowHandle A handle to the window whose property list receives the new entry.
 * \param PropName A pointer to a UNICODE_STRING specifying the property name.
 * \param Data A handle to the data to be associated with the property.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProp2(
    _In_ HWND WindowHandle,
    _In_ PUNICODE_STRING PropName,
    _In_ HANDLE Data
    );

// rev
/**
 * The NtUserSetScrollInfo routine sets the parameters of a scroll bar, including the minimum and maximum scrolling positions, page size, and scroll box position.
 *
 * \param WindowHandle A handle to a scroll bar control or a window with a standard scroll bar.
 * \param Bar Specifies the scroll bar type (SB_CTL, SB_HORZ, or SB_VERT).
 * \param ScrollInfo A pointer to a SCROLLINFO structure.
 * \param Redraw Specifies whether the scroll bar is redrawn to reflect the changes.
 * \return LONG The current scroll-box position.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserSetScrollInfo(
    _In_ HWND WindowHandle,
    _In_ LONG Bar,
    _In_ LPSCROLLINFO ScrollInfo,
    _In_ BOOL Redraw
    );

// rev
/**
 * The NtUserSetSharedWindowData routine sets cross-process or kernel-shared data associated with a window handle.
 *
 * \param Param1 Window handle or shared data key.
 * \param Param2 Configuration index or data length.
 * \param SharedWindowData A pointer to the shared window data structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetSharedWindowData(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _In_ PVOID SharedWindowData
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetShellChangeNotifyHWND routine registers the window that receives shell change notifications.
 *
 * \param WindowHandle Handle to the window that receives shell change notifications.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETSHELLCHANGENOTIFYHWND) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetShellChangeNotifyHWND(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetShellSpecialWindow routine designates special shell window roles (such as taskbar, start menu, or action center) in win32k.
 *
 * \param WindowHandle An optional handle to the special shell window, or NULL to unregister.
 * \param SpecialWindowType The special shell window role classification type.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetShellSpecialWindow(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG SpecialWindowType
    );

// rev
/**
 * The NtUserSetShellWindowEx routine registers the shell window and desktop list view window handles with the window manager.
 *
 * \param ShellWindow A handle to the shell window.
 * \param ListViewWindow A handle to the desktop list view window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetShellWindowEx(
    _In_ HWND ShellWindow,
    _In_ HWND ListViewWindow
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetTaskmanWindow routine sets the Task Manager window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndOpt(SFI_SETTASKMANWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetTaskmanWindow(
    _In_opt_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetVisible routine sets the visibility state of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Flags Visibility state selector (SV_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_SETVISIBLE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetVisible(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags // SV_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetWindowBand routine sets the Z-order window band for a specified window.
 *
 * \param WindowHandle A handle to the window whose band is being set.
 * \param InsertAfterWindowHandle An optional handle to the window to insert after within the band.
 * \param Band The window band identifier (e.g. ZBID_DESKTOP, ZBID_DEFAULT, ZBID_SYSTEM_TOOLS).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowBand(
    _In_ HWND WindowHandle,
    _In_opt_ HWND InsertAfterWindowHandle,
    _In_ ULONG Band
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetWindowContextHelpId routine sets the context help identifier for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param ContextId The context id.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_SETWINDOWCONTEXTHELPID) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowContextHelpId(
    _In_ HWND WindowHandle,
    _In_ ULONG ContextId
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetWindowFNID routine associates a predefined window procedure function identifier (FNID) with a window.
 *
 * \param WindowHandle A handle to the window.
 * \param FnId The predefined function identifier (e.g. FNID_BUTTON, FNID_EDIT).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowFNID(
    _In_ HWND WindowHandle,
    _In_ SHORT FnId
    );

// rev
/**
 * The NtUserSetWindowFeedbackSetting routine sets the feedback configuration for a window.
 *
 * \param Hwnd A handle to the window.
 * \param Feedback The feedback type (e.g. FEEDBACK_GESTURE_PRESSANDTAP).
 * \param DwFlags Feedback configuration flags.
 * \param Size The size, in bytes, of the Configuration buffer.
 * \param Configuration An optional pointer to the configuration buffer.
 * \return TRUE if successful, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetWindowFeedbackSetting.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowFeedbackSetting(
    _In_ HWND Hwnd,
    _In_ FEEDBACK_TYPE Feedback,
    _In_ ULONG DwFlags,
    _In_ ULONG Size,
    _In_reads_bytes_opt_(Size) const VOID* Configuration
    );

// rev
/**
 * The NtUserSetWindowLong routine changes an attribute of the specified window at the specified offset in the extra window memory.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based offset to the value to be set (e.g. GWL_STYLE, GWL_EXSTYLE).
 * \param NewValue The replacement value.
 * \param Ansi TRUE if handling ANSI window messages; FALSE for Unicode.
 * \return LONG_PTR The previous value of the specified offset, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG_PTR
NTAPI
NtUserSetWindowLong(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ LONG_PTR NewValue,
    _In_ BOOL Ansi
    );

// rev
/**
 * The NtUserSetWindowLongPtr routine changes an attribute of the specified window at the specified offset in the extra window memory with pointer-sized precision.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based offset to the value to be set (e.g. GWLP_WNDPROC, GWLP_HINSTANCE).
 * \param Value The replacement value.
 * \param Ansi Nonzero if handling ANSI window messages; zero for Unicode.
 * \return The previous value of the specified offset, or zero on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG_PTR
NTAPI
NtUserSetWindowLongPtr(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ ULONG_PTR Value,
    _In_ LONG Ansi
    );

/**
 * The NtUserSetWindowPlacement routine sets the show state and the restored, minimized, and maximized positions of the specified window.
 *
 * \param WindowHandle A handle to the window.
 * \param lpwndpl Pointer to a WINDOWPLACEMENT structure that specifies the new show state and positions.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowPlacement(
    _In_ HWND WindowHandle,
    _In_ const WINDOWPLACEMENT* lpwndpl
    );

/**
 * The NtUserSetWindowPos routine changes the size, position, and Z-order of a child, pop-up, or top-level window.
 *
 * \param WindowHandle A handle to the window.
 * \param WindowHandleInsertAfter A handle to the window to precede the positioned window in the Z-order.
 * \param X The new position of the left side of the window, in client coordinates.
 * \param Y The new position of the top of the window, in client coordinates.
 * \param cx The new width of the window, in pixels.
 * \param cy The new height of the window, in pixels.
 * \param Flags The window sizing and positioning flags (SWP_*).
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowPos(
    _In_ HWND WindowHandle,
    _In_opt_ HWND WindowHandleInsertAfter,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ ULONG Flags
    );

/**
 * The NtUserBringWindowToTop routine brings the specified window to the top of the Z-order.
 *
 * \param WindowHandle A handle to the window to bring to the top.
 * \return TRUE if successful, FALSE otherwise.
 */
FORCEINLINE
NTSYSAPI
BOOL
NTAPI
NtUserBringWindowToTop(
    _In_ HWND WindowHandle
    )
{
    return NtUserSetWindowPos(
        WindowHandle,
        NULL,
        0, 0, 0, 0,
        3
        );
}

// rev
/**
 * The NtUserSetWindowRgn routine sets the window region of a window, determining the area where the operating system permits drawing.
 *
 * \param WindowHandle A handle to the window whose window region is to be set.
 * \param RegionHandle An optional handle to a region.
 * \param Redraw Nonzero if the operating system redraws the window after setting the window region; zero otherwise.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowRgn(
    _In_ HWND WindowHandle,
    _In_opt_ HRGN RegionHandle,
    _In_ ULONG Redraw
    );

// rev
/**
 * The NtUserSetWindowRgnEx routine sets the window region of a window with extended flags.
 *
 * \param WindowHandle A handle to the window whose region is to be set.
 * \param RegionHandle An optional handle to the window region.
 * \param Flags Flags specifying region recalculation and redraw behavior.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowRgnEx(
    _In_ HWND WindowHandle,
    _In_opt_ HRGN RegionHandle,
    _In_ CHAR Flags
    );

// rev
/**
 * The NtUserSetWindowShowState routine sets the window visibility state, show command, and placement rect.
 *
 * \param WindowHandle A handle to the window to update.
 * \param ShowState The show window state code.
 * \param Flags Additional placement and visibility flags.
 * \param Rect An optional pointer to a RECT structure specifying window placement coordinates.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowShowState(
    _In_ HWND WindowHandle,
    _In_ ULONG ShowState,
    _In_ LONG Flags,
    _In_opt_ PRECT Rect
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetWindowState routine sets the specified window state flags.
 *
 * \param WindowHandle Handle to the target window.
 * \param Flag Window state flag bits to configure (WF*, WEF*, BF*, DF*, CBF*, EF*, SF*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_SETWINDOWSTATE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowState(
    _In_ HWND WindowHandle,
    _In_ ULONG Flag // WF*, WEF*, BF*, DF*, CBF*, EF*, SF*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserSetWindowWord routine replaces the 16-bit (two-byte) value at the specified offset into the extra window memory.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param Index The zero-based byte offset of the value to be replaced.
 * \param NewWord The replacement value.
 * \return The previous 16-bit value, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
USHORT
NTAPI
NtUserSetWindowWord(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ USHORT NewWord
    );

/**
 * The NtUserShellForegroundBoostProcess routine grants a foreground priority boost to the target process.
 *
 * \param ProcessHandle A handle to the process to boost.
 * \param WindowHandle A handle to the associated window.
 * \return A handle to the previous foreground window, or NULL.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserShellForegroundBoostProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserShellMigrateWindow routine migrates a window between virtual desktops or shell hosting environments.
 *
 * \param WindowHandle A handle to the window to migrate.
 * \param MonitorHandle A handle to the target monitor.
 * \param Param3 Migration options or animation flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellMigrateWindow(
    _In_ HWND WindowHandle,
    _In_ HMONITOR MonitorHandle,
    _In_ ULONG Param3
    );

// rev
/**
 * The NtUserShellSetWindowPos routine performs shell-orchestrated window positioning and Z-order placement.
 *
 * \param WindowHandle A handle to the window to reposition.
 * \param InsertAfterWindowHandle An optional handle to the window to precede the positioned window in Z order.
 * \param Param3 In-out pointer to shell positioning metrics or coordinates.
 * \param Param4 Window positioning flags.
 * \param Param5 Shell arrangement coordinate or state parameter.
 * \param Param6 Shell arrangement coordinate or state parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellSetWindowPos(
    _In_ HWND WindowHandle,
    _In_opt_ HWND InsertAfterWindowHandle,
    _Inout_ PVOID Param3,
    _In_ ULONG Param4,
    _In_ LONG Param5,
    _In_ LONG Param6
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserShowOwnedPopups routine shows or hides the pop-up windows owned by the specified window.
 *
 * \param Owner Handle to the owner window.
 * \param Show Visibility state or show command flag.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_SHOWOWNEDPOPUPS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowOwnedPopups(
    _In_ HWND Owner,
    _In_ LOGICAL Show
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserShowScrollBar routine shows or hides the specified scroll bar.
 *
 * \param WindowHandle A handle to a scroll bar control or a window with a standard scroll bar.
 * \param Bar Specifies the scroll bar to be shown or hidden (SB_CTL, SB_HORZ, SB_VERT, or SB_BOTH).
 * \param Show Nonzero to show the scroll bar; zero to hide it.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowScrollBar(
    _In_ HWND WindowHandle,
    _In_ LONG Bar,
    _In_ BOOL Show
    );

/**
 * The NtUserShowWindow routine sets the specified window's show state.
 *
 * \param WindowHandle A handle to the window.
 * \param CmdShow Controls how the window is to be shown (SW_*).
 * \return TRUE if the window was previously visible; FALSE if it was previously hidden.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindow(
    _In_ HWND WindowHandle,
    _In_ LONG CmdShow
    );

/**
 * The NtUserShowWindowAsync routine sets the show state of a window without waiting for the operation to complete.
 *
 * \param WindowHandle A handle to the window.
 * \param CmdShow Controls how the window is to be shown (SW_*).
 * \return TRUE if the operation was posted; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindowAsync(
    _In_ HWND WindowHandle,
    _In_ LONG CmdShow
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSwitchToThisWindow routine switches to the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param AltTab The alt tab.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_SWITCHTOTHISWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSwitchToThisWindow(
    _In_ HWND WindowHandle,
    _In_ LOGICAL AltTab
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserUndelegateInput routine revokes delegated input handling for the specified window and input type.
 *
 * \param WindowHandle A handle to the window from which input handling is undelegated.
 * \param InputType The input event type identifier being revoked.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUndelegateInput(
    _In_ HWND WindowHandle,
    _In_ ULONG InputType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUpdateClientRect routine updates the cached client rectangle of the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_UPDATECLIENTRECT) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateClientRect(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserUpdateLayeredWindow routine updates the position, size, shape, content, and translucency of a layered window.
 *
 * \param WindowHandle A handle to a layered window.
 * \param HdcDest A handle to a device context for the screen.
 * \param DestPoint A pointer to a POINT structure that specifies the new screen position.
 * \param Size A pointer to a SIZE structure that specifies the new size of the layered window.
 * \param HdcSrc A handle to a device context for the surface that defines the layered window.
 * \param SrcPoint A pointer to a POINT structure that specifies the location of the layer in the device context.
 * \param ColorKey An RGB color key to use when composing the layered window.
 * \param Blend A pointer to a BLENDFUNCTION structure that specifies the opacity of the layered window.
 * \param Flags Layer update flags (e.g. ULW_ALPHA, ULW_COLORKEY, ULW_OPAQUE).
 * \param DirtyRect An optional pointer to a RECT structure specifying the area that needs updating.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateLayeredWindow(
    _In_ HWND WindowHandle,
    _In_ HDC HdcDest,
    _In_ ULONG_PTR DestPoint,
    _In_ PSIZE Size,
    _In_ HDC HdcSrc,
    _In_ PPOINT SrcPoint,
    _In_ LONG ColorKey,
    _In_ LONG_PTR Blend,
    _In_ LONG Flags,
    _In_ ULONG_PTR DirtyRect
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUpdateWindow routine updates (repaints) the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndLock(SFI_UPDATEWINDOW) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindow(
    _In_ HWND WindowHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserUpdateWindowInputSinkHints routine updates window input sink redirection hint regions and hit-testing targets.
 *
 * \param Hints An array of input sink hint structures.
 * \param Count The number of items in the Hints array.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindowInputSinkHints(
    _In_reads_(Count) PVOID Hints,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserUpdateWindowTrackingInfo routine updates mouse hover and leave window tracking states for a window.
 *
 * \param WindowHandle A handle to the window being tracked.
 * \param TrackingInfo A pointer to updated tracking information.
 * \param PreviousTrackingInfo A pointer to previous tracking information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindowTrackingInfo(
    _In_ HWND WindowHandle,
    _In_ PVOID TrackingInfo,
    _In_ PVOID PreviousTrackingInfo
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUpdateWindows routine updates (repaints) the specified window and its related windows.
 *
 * \param WindowHandle Handle to the target window.
 * \param hrgn Handle to the region.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParamLock(SFI_UPDATEWINDOWS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindows(
    _In_ HWND WindowHandle,
    _In_ HRGN hrgn
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserWindowFromPhysicalPoint routine retrieves a handle to the window that contains the specified physical point.
 *
 * \param Point The physical coordinates of the point.
 * \return A handle to the window that contains the point, or NULL if no window exists at that point.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromPhysicalPoint(
    _In_ POINT Point
    );

/**
 * The NtUserWindowFromPoint routine retrieves a handle to the window that contains the specified point.
 *
 * \param Point The coordinates of the point to be tested.
 * \return A handle to the window that contains the point, or NULL if no window exists at that point.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromPoint(
    _In_ POINT Point
    );

// rev
/**
 * The RaiseLowerShellWindow routine raises or lowers the shell window in the z-order.
 *
 * \param WindowHandle Handle to the shell window.
 * \param SetWithOptions Flag specifying whether to apply shell positioning options.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserRaiseLowerShellWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
RaiseLowerShellWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN SetWithOptions
    );

// rev
/**
 * The ReportInertia routine reports inertia processing parameters.
 *
 * \param InertiaId Identifier for inertia processing.
 * \param Flags Operational flags for inertia processing.
 * \param Target Optional target pointer.
 * \param InertiaData Optional inertia data pointer.
 * \param ExtraData Optional extra data pointer.
 * \return BOOL TRUE if successful, FALSE otherwise.
 * \remarks Forwards to the NtUserReportInertia system call.
 */
NTSYSAPI
BOOL
NTAPI
ReportInertia(
    _In_ ULONG_PTR InertiaId,
    _In_ ULONG Flags,
    _In_opt_ PVOID Target,
    _In_opt_ PVOID InertiaData,
    _In_opt_ PVOID ExtraData
    );

// rev
/**
 * The ScrollChildren routine scrolls the child windows of a parent window.
 *
 * \param WindowHandle Handle to the parent window.
 * \param Message Scroll message identifier (WM_HSCROLL or WM_VSCROLL).
 * \param wParam Additional scroll parameter.
 */
NTSYSAPI
VOID
NTAPI
ScrollChildren(
    _In_ HWND WindowHandle,
    _In_ LONG Message,
    _In_ LONG wParam
    );

/**
 * The SetChildWindowNoActivate routine designates a child window to be positioned or configured without activation.
 *
 * \param WindowHandle A handle to the child window.
 * \return TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
SetChildWindowNoActivate(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The SetCoreWindow routine associates a CoreWindow with the calling thread.
 *
 * \param WindowHandle Handle to the CoreWindow.
 * \param Status Association status flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetCoreWindow system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetCoreWindow(
    _In_ HWND WindowHandle,
    _In_ ULONG Status
    );

// rev
/**
 * The SetCoveredWindowStates routine sets the covered or occluded state of an array of windows.
 *
 * \param WindowStates Pointer to array of window state structures.
 * \param Count Number of elements in the array.
 * \param Covered Non-zero if windows are covered; zero otherwise.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetCoveredWindowStates system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetCoveredWindowStates(
    _In_reads_(Count) PVOID WindowStates,
    _In_ ULONG Count,
    _In_ LONG Covered
    );

// rev
/**
 * The SetInternalWindowPos routine sets the internal (restored) placement of a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param ShowCommand Show command specifying window appearance (SW_*).
 * \param Placement Pointer to the window placement data structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetInternalWindowPos system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetInternalWindowPos(
    _In_ HWND WindowHandle,
    _In_ LONG ShowCommand,
    _Inout_ PVOID Placement
    );

// rev
/**
 * The SetMirrorRendering routine enables or disables mirror rendering for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable Non-zero to enable mirror rendering; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetMirrorRendering system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetMirrorRendering(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

// rev
/**
 * The SetProgmanWindow routine sets the Program Manager window.
 *
 * \param WindowHandle Optional handle to designate as Program Manager window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserSetProgmanWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
SetProgmanWindow(
    _In_ _In_opt_ HWND WindowHandle
    );

// rev
/**
 * The SetShellChangeNotifyWindow routine sets the shell change-notification window.
 *
 * \param WindowHandle Handle to the window receiving shell change notifications.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserSetShellChangeNotifyHWND.
 */
NTSYSAPI
LOGICAL
NTAPI
SetShellChangeNotifyWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The SetShellWindow routine registers the shell desktop window.
 *
 * \param ShellWindow Handle to the window designated as the shell desktop window.
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserSetShellWindowEx system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetShellWindow(
    _In_ HWND ShellWindow
    );

// rev
/**
 * The SetShellWindowEx routine registers the shell desktop and taskbar list-view windows.
 *
 * \param ShellWindow Handle to the shell desktop window.
 * \param ListViewWindow Handle to the shell desktop list-view window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetShellWindowEx system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetShellWindowEx(
    _In_ HWND ShellWindow,
    _In_ HWND ListViewWindow
    );

// rev
/**
 * The SetTaskmanWindow routine sets the task-manager (taskman) window.
 *
 * \param WindowHandle Optional handle to designate as task-manager window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserSetTaskmanWindow.
 */
NTSYSAPI
LOGICAL
NTAPI
SetTaskmanWindow(
    _In_ _In_opt_ HWND WindowHandle
    );

// rev
/**
 * The SetWindowBand routine sets the z-order band of a window.
 *
 * \param WindowHandle Handle to the window to place into the band.
 * \param InsertAfterWindowHandle Optional handle to the window to insert after in the band.
 * \param Band Window z-order band identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetWindowBand system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetWindowBand(
    _In_ HWND WindowHandle,
    _In_opt_ HWND InsertAfterWindowHandle,
    _In_ ULONG Band
    );

// rev
/**
 * The SetWindowRgnEx routine sets the window region with extended flags.
 *
 * \param WindowHandle Handle to the window whose region is to be set.
 * \param RegionHandle Optional handle to the region.
 * \param Flags Extended region flags.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetWindowRgnEx(
    _In_ HWND WindowHandle,
    _In_opt_ HRGN RegionHandle,
    _In_ CHAR Flags
    );

// rev
/**
 * The ShellForegroundBoostProcess routine applies a shell foreground boost to a process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param WindowHandle Handle to the window associated with the boost.
 * \return HWND Handle to the boosted window, or NULL on failure.
 * \remarks Thin user32 wrapper over NtUserShellForegroundBoostProcess.
 */
NTSYSAPI
HWND
NTAPI
ShellForegroundBoostProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    );

// rev
/**
 * The ShellHandwritingDelegateInput routine delegates handwriting input to the shell input dispatcher.
 *
 * \param WindowHandle Handle to the window receiving delegated handwriting input.
 * \param InputData Pointer to handwriting input event structure.
 * \param Flags Delegation control flags.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserShellHandwritingDelegateInput system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ShellHandwritingDelegateInput(
    _In_ HWND WindowHandle,
    _In_ PVOID InputData,
    _In_ ULONG Flags
    );

// rev
/**
 * The ShellMigrateWindow routine migrates a shell window to a new session or desktop.
 *
 * \param WindowHandle Handle to the shell window being migrated.
 * \param TargetSessionId Session identifier of the target destination session.
 * \param Flags Migration options and layout preservation flags.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserShellMigrateWindow system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ShellMigrateWindow(
    _In_ HWND WindowHandle,
    _In_ ULONG TargetSessionId,
    _In_ ULONG Flags
    );

// rev
/**
 * The ShellSetWindowPos routine changes the size, position, and Z-order of a shell window.
 *
 * \param WindowHandle Handle to the window to reposition.
 * \param InsertAfter Handle to the window that precedes the positioned window in the Z-order.
 * \param PositionData Pointer to shell window positioning coordinate structure.
 * \param Flags Window positioning flags (SWP_*).
 * \param X New horizontal coordinate of the window.
 * \param Y New vertical coordinate of the window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserShellSetWindowPos system call.
 */
NTSYSAPI
LOGICAL
NTAPI
ShellSetWindowPos(
    _In_ HWND WindowHandle,
    _In_opt_ HWND InsertAfter,
    _In_opt_ PVOID PositionData,
    _In_ ULONG Flags,
    _In_ LONG X,
    _In_ LONG Y
    );

// rev
/**
 * The TileChildWindows routine tiles the child windows of a parent window.
 *
 * \param ParentWindowHandle Handle to the parent window.
 * \param Flags Tiling arrangement flags (MDITILE_HORIZONTAL or MDITILE_VERTICAL).
 * \return ULONG_PTR Number of child windows tiled.
 */
NTSYSAPI
ULONG_PTR
NTAPI
TileChildWindows(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG Flags
    );

// rev
/**
 * The UndelegateInput routine cancels input delegation between windows.
 *
 * \param WindowHandle Handle to the window whose input delegation is canceled.
 * \param InputType Input event type to undelegate.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserUndelegateInput system call.
 */
NTSYSAPI
LOGICAL
NTAPI
UndelegateInput(
    _In_ HWND WindowHandle,
    _In_ ULONG InputType
    );

// rev
/**
 * The UpdateWindowInputSinkHints routine updates the input-sink hints for a window.
 *
 * \param Hints Pointer to an array of input sink hint structures.
 * \param Count Number of elements in the array.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserUpdateWindowInputSinkHints system call.
 */
NTSYSAPI
LOGICAL
NTAPI
UpdateWindowInputSinkHints(
    _In_reads_(Count) PVOID Hints,
    _In_ ULONG Count
    );

// rev
/**
 * The EndDeferWindowPosEx routine applies deferred window position and band changes.
 *
 * \param WindowPosInfo Handle to the multiple-window-position internal structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserEndDeferWindowPosEx system call.
 */
NTSYSAPI
LOGICAL
NTAPI
EndDeferWindowPosEx(
    _In_ HDWP WindowPosInfo
    );

/**
 * The NtUserDestroyWindow routine destroys the specified window.
 *
 * \param WindowHandle A handle to the window to be destroyed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDestroyWindow(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtUserEndDeferWindowPosEx routine simultaneously updates the position and size of one or more windows.
 *
 * \param WindowPosInfo Handle to the multiple-window-position structure.
 * \param Async TRUE to execute window position updates asynchronously; FALSE otherwise.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEndDeferWindowPosEx(
    _In_ HDWP WindowPosInfo,
    _In_ LOGICAL Async
    );

// rev
/**
 * The NtUserRemoveProp routine removes an entry from the property list of the specified window.
 *
 * \param WindowHandle A handle to the window whose property list is to be changed.
 * \param Atom An atom or string identifier identifying the property string.
 * \return The data handle associated with the specified property, or NULL if not found.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserRemoveProp(
    _In_ HWND WindowHandle,
    _In_ ULONG Atom
    );

/**
 * The NtUserShutdownReasonDestroy routine destroys the shutdown block reason associated with a window.
 *
 * \param WindowHandle A handle to the main window of the application.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserShutdownReasonDestroy(
    _In_ HWND WindowHandle
    );

/**
 * The NtUserUnregisterHotKey routine frees a hot key previously registered by the calling thread.
 *
 * \param WindowHandle A handle to the window associated with the hot key to be freed.
 * \param Id The identifier of the hot key to be freed.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserUnregisterHotKey(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Id
    );

//
// Raw Input & Raw Input Manager (RIM)
//

// rev
/**
 * The NtRIMAddInputObserver routine adds a raw input observer to the Raw Input Manager (RIM).
 *
 * \param PBuffer A pointer to the input observer buffer.
 * \param DwBufferSize The size of the buffer in bytes.
 * \param HInputReadyEvent A handle to an event signaled when input is ready.
 * \param DwInputType The raw input type.
 * \param DwUsagePage The HID usage page.
 * \param DwUsage The HID usage ID.
 * \param DwFlags Observation flags.
 * \param PhInputObserver A pointer receiving the created input observer handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMAddInputObserver(
    _In_reads_bytes_(DwBufferSize) PVOID PBuffer,
    _In_ ULONG DwBufferSize,
    _In_ HANDLE HInputReadyEvent,
    _In_ ULONG DwInputType,
    _In_ ULONG DwUsagePage,
    _In_ ULONG DwUsage,
    _In_ ULONG DwFlags,
    _Out_ PHANDLE PhInputObserver
    );

// rev
/**
 * The NtRIMAreSiblingDevices routine determines whether two raw input devices are siblings.
 *
 * \param Device1 First raw input device identifier.
 * \param Device2 Second raw input device identifier.
 * \param Result A pointer receiving the sibling check result.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMAreSiblingDevices(
    _In_ LONG_PTR Device1,
    _In_ LONG_PTR Device2,
    _Out_ PVOID Result
    );

// rev
/**
 * NtRIMDeviceIoControl
 *
 * win32kbase 10.0.26100.9444: nine syscall arguments; argument 9 is a DWORD
 * (0x1401f43f8), not an IO_STATUS_BLOCK pointer. Its semantic name is unrecovered.
 * OutputBuffer is captured before the operation and copied back afterwards.
 * ReturnLength is optional (0x1401f44c3-0x1401f44de).
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMDeviceIoControl(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Inout_updates_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ ULONG Param9
    );

// rev
/**
 * The NtRIMEnableMonitorMappingForDevice routine enables or disables monitor mapping for a raw input device.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param DeviceHandle The raw input device identifier.
 * \param EnableFlags Flags controlling whether monitor mapping is enabled.
 * \param MonitorId Optional pointer receiving the mapped monitor identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMEnableMonitorMappingForDevice(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _In_ ULONG EnableFlags,
    _Out_opt_ PULONG_PTR MonitorId
    );

// rev
/**
 * The NtRIMFreeInputBuffer routine frees an input buffer allocated by the Raw Input Manager.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param InputBuffer Pointer to the input buffer to free.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMFreeInputBuffer(
    _In_ HANDLE InputManagerHandle,
    _In_ PVOID InputBuffer
    );

// rev
/**
 * The NtRIMGetDevicePreparsedData routine retrieves the preparsed data for a raw input HID device.
 *
 * \param RimHandle Handle to the raw input manager.
 * \param DeviceHandle Handle to the target HID device.
 * \param Buffer Optional buffer receiving the HID preparsed data. Specify NULL to query the required size.
 * \param BufferSize Size of Buffer, in bytes. Receives the required size only when Buffer is NULL.
 * \return NTSTATUS Successful or errant status.
 *
 *
 * \remarks When Buffer is non-NULL, copies the smaller of the supplied size and the preparsed-data size
 * without updating BufferSize or reporting a short buffer.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetDevicePreparsedData(
    _In_ HANDLE RimHandle,
    _In_ HANDLE DeviceHandle,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

// rev
/**
 * The NtRIMGetDevicePreparsedDataLockfree routine retrieves preparsed data for a raw input device without acquiring locks.
 *
 * \param HRimDev Handle to the target HID device.
 * \param PBuffer Optional buffer receiving the HID preparsed data.
 * \param PdwBufferSize Size of PBuffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetDevicePreparsedDataLockfree(
    _In_ HANDLE HRimDev,
    _Out_writes_bytes_opt_(*PdwBufferSize) PVOID PBuffer,
    _Inout_ PULONG PdwBufferSize
    );

// rev
/**
 * The NtRIMGetDeviceProperties routine retrieves properties for a raw input device.
 *
 * \param HRimHandle Handle to the raw input manager.
 * \param HRimDev Handle to the target device.
 * \param PRimDevProps A pointer to a structure receiving device properties.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetDeviceProperties(
    _In_ HANDLE HRimHandle,
    _In_ HANDLE HRimDev,
    _Out_ struct RIM_DEVICE_PROPERTIES* PRimDevProps
    );

// rev
/**
 * The NtRIMGetDevicePropertiesLockfree routine retrieves properties for a raw input device without acquiring locks.
 *
 * \param HRimDev Handle to the target device.
 * \param PRimDevProps A pointer to a structure receiving device properties.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetDevicePropertiesLockfree(
    _In_ HANDLE HRimDev,
    _Out_ struct RIM_DEVICE_PROPERTIES* PRimDevProps
    );

// rev
/**
 * The NtRIMGetPhysicalDeviceRect routine retrieves the physical device rectangle for a raw input device.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param DeviceHandle The raw input device identifier.
 * \param PhysicalRect A pointer receiving the physical rectangle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetPhysicalDeviceRect(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _Out_ PRECT PhysicalRect
    );

// rev
/**
 * The NtRIMGetSourceProcessId routine retrieves the source process identifier for raw input.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param DeviceHandle The raw input device identifier.
 * \param ProcessId Pointer to a variable that receives the source process identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMGetSourceProcessId(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _Out_ PULONG_PTR ProcessId
    );

// rev
/**
 * The NtRIMObserveNextInput routine waits to observe the next input event.
 *
 * \param HInputObserver A handle to the input observer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMObserveNextInput(
    _In_ HANDLE HInputObserver
    );

// rev
/**
 * The NtRIMOnAsyncPnpWorkNotification routine notifies the Raw Input Manager of asynchronous PnP work completion.
 *
 * \param HRimHandle Handle to the raw input manager.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMOnAsyncPnpWorkNotification(
    _In_ HANDLE HRimHandle
    );

// rev
/**
 * The NtRIMOnPnpNotification routine notifies the Raw Input Manager of a PnP device change.
 *
 * \param HRimHandle Handle to the raw input manager.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMOnPnpNotification(
    _In_ HANDLE HRimHandle
    );

// rev
/**
 * The NtRIMOnTimerNotification routine processes a timer notification in the Raw Input Manager.
 *
 * \param Handle A pointer to timer notification data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMOnTimerNotification(
    _Inout_ PSTR Handle
    );

// rev
/**
 * The NtRIMQueryDevicePath routine queries the device interface path for a raw input device.
 *
 * \param PusDevicePath A pointer to a UNICODE_STRING containing the device interface path.
 * \param PhRimDev A pointer receiving the opened device handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMQueryDevicePath(
    _In_ PUNICODE_STRING PusDevicePath,
    _Out_ PHANDLE PhRimDev
    );

// rev
/**
 * The NtRIMReadInput routine reads raw input data packets from the Raw Input Manager.
 *
 * \param HRimHandle Handle to the raw input manager.
 * \param PpBuffer A pointer to a buffer receiving raw input packets.
 * \param UlLengthToRead The length in bytes of input data to read.
 * \param HReadCompletionEvent An event handle signaled when read completes.
 * \param PhRimDevice A pointer receiving the device handle producing the input.
 * \param PdwInputTypeRead A pointer receiving the input type read.
 * \param PioSb A pointer to an I/O status block receiving completion status.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMReadInput(
    _In_ HANDLE HRimHandle,
    _Inout_ PVOID* PpBuffer,
    _In_ ULONG UlLengthToRead,
    _In_ HANDLE HReadCompletionEvent,
    _Out_ PHANDLE PhRimDevice,
    _Out_ PULONG PdwInputTypeRead,
    _Inout_ PIO_STATUS_BLOCK PioSb
    );

// rev
/**
 * The NtRIMRegisterForInputEx routine registers for raw input notifications with extended options.
 *
 * \param DwInputType The raw input type.
 * \param PusDeviceName Optional pointer to the target device name.
 * \param CRimUsages Count of usages in the PRimUsages array.
 * \param PRimUsages Optional array of usage and page descriptors.
 * \param HPnpNotificationEvent Handle to an event signaled on PnP change.
 * \param HTimer Handle to a timer object.
 * \param HAsyncPnpWorkNotificationSemaphore Handle to a semaphore signaled on async PnP work.
 * \param PContext Caller-defined context pointer.
 * \param PfnRimDevChangeCbProc Optional pointer to device change callback routine.
 * \param PhRimHandle A pointer receiving the created RIM handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMRegisterForInputEx(
    _In_ ULONG DwInputType,
    _In_opt_ PUNICODE_STRING PusDeviceName,
    _In_opt_ ULONG CRimUsages,
    _In_opt_ struct _RIM_USAGE_ANDPAGE* PRimUsages,
    _In_ HANDLE HPnpNotificationEvent,
    _In_ HANDLE HTimer,
    _In_ HANDLE HAsyncPnpWorkNotificationSemaphore,
    _In_ PVOID PContext,
    _In_opt_ PRIM_DEVICE_CHANGE_CALLBACK PfnRimDevChangeCbProc,
    _Out_ PHANDLE PhRimHandle
    );

// rev
/**
 * The NtRIMRemoveInputObserver routine removes a raw input observer.
 *
 * \param HInputObserver Handle to the input observer to remove.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMRemoveInputObserver(
    _In_ HANDLE HInputObserver
    );

// rev
/**
 * The NtRIMSetDeadzoneRotation routine configures deadzone rotation parameters for a raw input device.
 *
 * \param Param1 Deadzone rotation configuration parameter.
 * \return ULONG 0 on success; otherwise a Win32 error code.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtRIMSetDeadzoneRotation(
    _In_ ULONG Rotation
    );

// rev
/**
 * The NtRIMSetExtendedDeviceProperty routine sets extended device properties for a raw input device.
 *
 * \param HRimDev Handle to the raw input device.
 * \param PProperty A pointer to the property buffer.
 * \param DwPropSize The size of the property buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMSetExtendedDeviceProperty(
    _In_ HANDLE HRimDev,
    _In_reads_bytes_(DwPropSize) PVOID PProperty,
    _In_ ULONG DwPropSize
    );

// rev
/**
 * The NtRIMSetTestModeStatus routine configures test mode status for the Raw Input Manager.
 *
 * \param Status Test mode status flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMSetTestModeStatus(
    _In_ ULONG Status
    );

// rev
/**
 * The NtRIMUnregisterForInput routine unregisters from raw input notifications.
 *
 * \param HRimHandle Handle to the raw input manager to unregister.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMUnregisterForInput(
    _In_ HANDLE HRimHandle
    );

// rev
/**
 * The NtRIMUpdateInputObserverRegistration routine updates registration parameters for a raw input observer.
 *
 * \param HInputObserver Handle to the input observer.
 * \param DwFlags Registration flags.
 * \param PBuffer Optional pointer to buffer data.
 * \param DwBufferSize Optional size of the buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRIMUpdateInputObserverRegistration(
    _In_ HANDLE HInputObserver,
    _In_ ULONG DwFlags,
    _In_reads_bytes_opt_(DwBufferSize) PVOID PBuffer,
    _In_opt_ ULONG DwBufferSize
    );

// rev
/**
 * The NtUserGetRawInputBuffer routine performs a buffered read of raw input data packets.
 *
 * \param Data Optional pointer to a buffer of RAWINPUT structures receiving input data.
 * \param Size Pointer to a variable holding the buffer size in bytes and receiving the required or read size.
 * \param HeaderSize Size, in bytes, of the RAWINPUTHEADER structure.
 * \return ULONG The number of messages copied, or -1 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputBuffer(
    _Out_opt_ PVOID Data,
    _Inout_ PULONG Size,
    _In_ LONG HeaderSize
    );

/**
 * The NtUserGetRawInputData routine retrieves raw input from the specified device.
 *
 * \param RawInputData Handle to the RAWINPUT structure.
 * \param RawInputCommand The command flag (RID_INPUT or RID_HEADER).
 * \param RawInputBuffer Pointer to the buffer that receives the RAWINPUT data.
 * \param RawInputBufferSize Pointer to a variable that specifies the size of the buffer and receives the required size.
 * \param RawInputHeaderSize The size of the RAWINPUTHEADER structure in bytes.
 * \return The number of bytes copied, or -1 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputData(
    _In_ HRAWINPUT RawInputData,
    _In_ ULONG RawInputCommand,
    _Out_writes_bytes_to_opt_(*RawInputBufferSize, return) PVOID RawInputBuffer,
    _Inout_ PULONG RawInputBufferSize,
    _In_ ULONG RawInputHeaderSize
    );

// rev
/**
 * The NtUserGetRawInputDeviceInfo routine retrieves information about a raw input device.
 *
 * \param HDevice Optional handle to the raw input device.
 * \param UiCommand Command code specifying the information to retrieve (RIDI_*).
 * \param PData Optional pointer to a buffer receiving the requested information.
 * \param PcbSize Pointer to a variable holding the buffer size and receiving the required size.
 * \return ULONG The number of bytes or characters copied, or (ULONG)-1 on failure.
 * \remarks Native entry point for USER32!GetRawInputDeviceInfoW.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputDeviceInfo(
    _In_opt_ HANDLE HDevice,
    _In_ ULONG UiCommand,
    _Inout_updates_bytes_to_opt_(*PcbSize, *PcbSize) PVOID PData,
    _Inout_ PULONG PcbSize
    );

/**
 * The NtUserGetRawInputDeviceList routine enumerates the raw input devices attached to the system.
 *
 * \param RawInputDeviceList Pointer to an array of RAWINPUTDEVICELIST structures.
 * \param RawInputDeviceCount Pointer to a variable that specifies the number of devices and receives the total number of attached devices.
 * \param RawInputDeviceSize The size of a RAWINPUTDEVICELIST structure in bytes.
 * \return The number of devices written to the buffer, or -1 on error.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputDeviceList(
    _Out_writes_opt_(*RawInputDeviceCount) PRAWINPUTDEVICELIST RawInputDeviceList,
    _Inout_ PULONG RawInputDeviceCount,
    _In_ ULONG RawInputDeviceSize
    );

/**
 * The NtUserGetRegisteredRawInputDevices routine retrieves the raw input devices for the current application.
 *
 * \param RawInputDevices Pointer to an array of RAWINPUTDEVICE structures.
 * \param RawInputDeviceCount Pointer to a variable that specifies the buffer capacity and receives the registered device count.
 * \param RawInputDeviceSize The size of a RAWINPUTDEVICE structure in bytes.
 * \return The number of registered raw input devices written to the array, or -1 on error.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRegisteredRawInputDevices(
    _Out_writes_opt_( *RawInputDeviceCount) PRAWINPUTDEVICE RawInputDevices,
    _Inout_ PULONG RawInputDeviceCount,
    _In_ ULONG RawInputDeviceSize
    );

// rev
/**
 * The NtUserRegisterRawInputDevices routine registers the devices that supply the raw input data for the calling application.
 *
 * \param PRawInputDevices An array of RAWINPUTDEVICE structures that represent the devices supplying raw input.
 * \param UiNumDevices The number of RAWINPUTDEVICE structures pointed to by PRawInputDevices.
 * \param CbSize The size, in bytes, of a RAWINPUTDEVICE structure.
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!RegisterRawInputDevices.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterRawInputDevices(
    _In_reads_(UiNumDevices) PCRAWINPUTDEVICE PRawInputDevices,
    _In_ ULONG UiNumDevices,
    _In_ ULONG CbSize
    );

// rev
/**
 * The RIMAddInputObserver routine registers an input observer callback with the Raw Input Manager (RIM).
 *
 * \param Buffer Pointer to the caller-supplied input observer buffer.
 * \param BufferSize Size of the input observer buffer in bytes.
 * \param InputReadyEvent Handle to an event signaled when observer input data is available.
 * \param InputType Raw input device type (RIM_TYPE*).
 * \param UsagePage HID usage page of the observed device.
 * \param Usage HID usage ID of the observed device.
 * \param Flags Observer registration flags controlling event delivery.
 * \param ObserverHandle Receives the handle to the registered input observer.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMAddInputObserver system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMAddInputObserver(
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_opt_ HANDLE InputReadyEvent,
    _In_ ULONG InputType,
    _In_ ULONG UsagePage,
    _In_ ULONG Usage,
    _In_ ULONG Flags,
    _Out_ PHANDLE ObserverHandle
    );

// rev
/**
 * The RIMAreSiblingDevices routine determines whether two raw input devices belong to the same physical device.
 *
 * \param DeviceHandle1 Handle to the first raw input device.
 * \param DeviceHandle2 Handle to the second raw input device.
 * \param Siblings Receives nonzero if the devices are siblings; otherwise 0.
 * \param Flags Operational flags.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMAreSiblingDevices system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMAreSiblingDevices(
    _In_ HANDLE DeviceHandle1,
    _In_ HANDLE DeviceHandle2,
    _Out_ PULONG Siblings,
    _In_ LONG Flags
    );

// rev
/**
 * The RIMDeviceIoControl routine issues a device I/O control request to a raw input device.
 *
 * \param InputManagerHandle Handle to the Raw Input Manager object.
 * \param DeviceHandle Handle to the target raw input device.
 * \param IoControlCode The I/O control code for the operation.
 * \param InputBuffer Pointer to the input buffer.
 * \param InputBufferLength Size of the input buffer in bytes.
 * \param OutputBuffer Pointer to the output buffer.
 * \param OutputBufferLength Size of the output buffer in bytes.
 * \param ReturnLength Receives the number of bytes returned in the output buffer.
 * \param IoStatusBlock Optional pointer to a block that receives the final I/O status.
 * \param Asynchronous TRUE to dispatch the request via NtDeviceIoControlFile; FALSE to issue it synchronously.
 * \param Synchronous TRUE to build the request as a synchronous (fsync) control.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMDeviceIoControl system call. Argument roles determined from win32kbase.sys.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMDeviceIoControl(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength,
    _Inout_opt_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ BOOLEAN Asynchronous,
    _In_ BOOLEAN Synchronous
    );

// rev
/**
 * The RIMEnableMonitorMappingForDevice routine enables monitor-coordinate mapping for a raw input device.
 *
 * \param InputManagerHandle Handle to the input manager.
 * \param DeviceHandle Handle to the raw input device.
 * \param EnableFlags Flags controlling the monitor mapping behavior.
 * \param MonitorId Optional pointer receiving the monitor identifier.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMEnableMonitorMappingForDevice system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMEnableMonitorMappingForDevice(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _In_ ULONG EnableFlags,
    _Out_opt_ PLONG_PTR MonitorId
    );

// rev
/**
 * The RIMFreeInputBuffer routine frees a raw input buffer previously returned by the Raw Input Manager.
 *
 * \param InputManagerHandle Handle to the input manager.
 * \param InputBuffer Pointer to the input buffer to free.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMFreeInputBuffer system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMFreeInputBuffer(
    _In_ HANDLE InputManagerHandle,
    _In_ PVOID InputBuffer
    );

// rev
/**
 * The RIMGetDevicePreparsedData routine retrieves the HID preparsed data describing a raw input device.
 *
 * \param RimHandle Handle to the raw input manager.
 * \param DeviceHandle Handle to the target HID device.
 * \param Buffer Optional buffer receiving the HID preparsed data. Specify NULL to query the required size.
 * \param BufferSize Size of Buffer, in bytes. Receives the required size only when Buffer is NULL.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetDevicePreparsedData system call.
 * When Buffer is non-NULL, copies the smaller of the supplied size and the preparsed-data size
 * without updating BufferSize or reporting a short buffer. Non-HID devices return STATUS_INVALID_PARAMETER.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetDevicePreparsedData(
    _In_ HANDLE RimHandle,
    _In_ HANDLE DeviceHandle,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

// rev
/**
 * The RIMGetDevicePreparsedDataLockfree routine is a lock-free variant that retrieves HID preparsed data for a raw input device.
 *
 * \param DeviceHandle Handle to the target HID device.
 * \param Buffer Optional buffer receiving the HID preparsed data.
 * \param BufferSize In/out pointer specifying buffer size and receiving required size.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetDevicePreparsedDataLockfree system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetDevicePreparsedDataLockfree(
    _In_ HANDLE DeviceHandle,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

// rev
/**
 * The RIMGetDeviceProperties routine retrieves properties of a raw input device.
 *
 * \param RimHandle Handle to the raw input manager.
 * \param DeviceHandle Handle to the target raw input device.
 * \param Properties Pointer to a RIM_DEVICE_PROPERTIES structure that receives device properties.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetDeviceProperties system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetDeviceProperties(
    _In_ HANDLE RimHandle,
    _In_ HANDLE DeviceHandle,
    _Out_ struct RIM_DEVICE_PROPERTIES* Properties
    );

// rev
/**
 * The RIMGetDevicePropertiesLockfree routine is a lock-free variant that retrieves properties of a raw input device.
 *
 * \param DeviceHandle Handle to the target raw input device.
 * \param Properties Pointer to a RIM_DEVICE_PROPERTIES structure that receives device properties.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetDevicePropertiesLockfree system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetDevicePropertiesLockfree(
    _In_ HANDLE DeviceHandle,
    _Out_ struct RIM_DEVICE_PROPERTIES* Properties
    );

// rev
/**
 * The RIMGetPhysicalDeviceRect routine retrieves the physical screen rectangle mapped to a raw input device.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param DeviceHandle Handle to the target input device.
 * \param PhysicalRect Pointer to a RECT structure that receives the physical coordinates.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetPhysicalDeviceRect system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetPhysicalDeviceRect(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _Out_ PRECT PhysicalRect
    );

// rev
/**
 * The RIMGetSourceProcessId routine retrieves the process identifier that sourced a raw input packet.
 *
 * \param InputManagerHandle Handle to the raw input manager.
 * \param DeviceHandle Handle to the target input device.
 * \param ProcessId Pointer to a variable that receives the source process identifier.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMGetSourceProcessId system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMGetSourceProcessId(
    _In_ HANDLE InputManagerHandle,
    _In_ HANDLE DeviceHandle,
    _Out_ PULONG_PTR ProcessId
    );

// rev
/**
 * The RIMObserveNextInput routine requests observation of the next raw input packet from a device.
 *
 * \param ObserverHandle Handle to the raw input observer.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMObserveNextInput system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMObserveNextInput(
    _In_ HANDLE ObserverHandle
    );

// rev
/**
 * The RIMOnAsyncPnpWorkNotification routine handles an asynchronous Plug and Play work notification for raw input devices.
 *
 * \param RimHandle Handle to the raw input manager.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMOnAsyncPnpWorkNotification system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMOnAsyncPnpWorkNotification(
    _In_ HANDLE RimHandle
    );

// rev
/**
 * The RIMOnPnpNotification routine handles a Plug and Play notification for raw input devices.
 *
 * \param RimHandle Handle to the raw input manager.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMOnPnpNotification system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMOnPnpNotification(
    _In_ HANDLE RimHandle
    );

// rev
/**
 * The RIMOnTimerNotification routine handles a timer notification inside the Raw Input Manager.
 *
 * \param Handle Pointer to the timer notification handle string or buffer.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMOnTimerNotification system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMOnTimerNotification(
    _Inout_ PSTR Handle
    );

// rev
/**
 * The RIMQueryDevicePath routine retrieves the device interface path of a raw input device.
 *
 * \param DevicePath Pointer to a UNICODE_STRING containing the device interface path.
 * \param DeviceHandle Pointer that receives the opened raw input device handle.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMQueryDevicePath system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMQueryDevicePath(
    _In_ PUNICODE_STRING DevicePath,
    _Out_ PHANDLE DeviceHandle
    );

// rev
/**
 * The RIMReadInput routine reads a raw input packet from a device.
 *
 * \param RimHandle Handle to the raw input manager.
 * \param Buffer Pointer to a variable receiving the raw input packet buffer.
 * \param LengthToRead Requested byte length to read.
 * \param ReadCompletionEvent Handle to an event signaled on completion of the read operation.
 * \param DeviceHandle Pointer to a variable receiving the source device handle.
 * \param InputTypeRead Pointer to a variable receiving the raw input type identifier.
 * \param IoStatusBlock Pointer to an I/O status block receiving status and bytes transferred.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMReadInput system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMReadInput(
    _In_ HANDLE RimHandle,
    _Inout_ PVOID* Buffer,
    _In_ ULONG LengthToRead,
    _In_ HANDLE ReadCompletionEvent,
    _Out_ PHANDLE DeviceHandle,
    _Out_ PULONG InputTypeRead,
    _Inout_ PIO_STATUS_BLOCK IoStatusBlock
    );

// rev
/**
 * The RIMRegisterForInput routine registers the calling thread or process to receive raw input from devices.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMRegisterForInput(
    VOID
    );

// rev
/**
 * The RIMRegisterForInputEx routine provides extended registration for the caller to receive raw input from a device.
 *
 * \param DeviceType Type of the raw input device (RIM_TYPE*).
 * \param DeviceName Optional pointer to the target device name string or handle.
 * \param UsageCount Count of usages in the Usages array.
 * \param Usages Optional array of usage and page descriptors.
 * \param PnpNotificationEvent Handle to an event signaled on PnP changes.
 * \param Timer Handle to a timer object for input polling.
 * \param AsyncPnpWorkSemaphore Handle to a semaphore signaled on asynchronous PnP work.
 * \param Context Caller-defined registration context pointer.
 * \param DeviceChangeCallback Optional pointer to device change callback routine.
 * \param RimHandle Receives the created RIM registration handle.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMRegisterForInputEx system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMRegisterForInputEx(
    _In_ ULONG DeviceType,
    _In_opt_ PUNICODE_STRING DeviceName,
    _In_opt_ ULONG UsageCount,
    _In_opt_ struct _RIM_USAGE_ANDPAGE* Usages,
    _In_ HANDLE PnpNotificationEvent,
    _In_ HANDLE Timer,
    _In_ HANDLE AsyncPnpWorkSemaphore,
    _In_opt_ PVOID Context,
    _In_opt_ PRIM_DEVICE_CHANGE_CALLBACK DeviceChangeCallback,
    _Out_ PHANDLE RimHandle
    );

// rev
/**
 * The RIMRemoveInputObserver routine removes a raw input observer previously added with RIMAddInputObserver.
 *
 * \param ObserverHandle Handle to the observer to remove.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMRemoveInputObserver system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMRemoveInputObserver(
    _In_ HANDLE ObserverHandle
    );

// rev
/**
 * The RIMSetExtendedDeviceProperty routine sets an extended property on a raw input device.
 *
 * \param DeviceHandle Handle to the target raw input device.
 * \param Property Pointer to the property data buffer.
 * \param PropertySize Size of the property data buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMSetExtendedDeviceProperty system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMSetExtendedDeviceProperty(
    _In_ HANDLE DeviceHandle,
    _In_reads_bytes_(PropertySize) PVOID Property,
    _In_ ULONG PropertySize
    );

// rev
/**
 * The RIMSetTestModeStatus routine enables or disables raw input test mode.
 *
 * \param Status Flags or status code controlling the test mode state.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMSetTestModeStatus system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMSetTestModeStatus(
    _In_ ULONG Status
    );

// rev
/**
 * The RIMUnregisterForInput routine cancels a raw input delivery registration for a device.
 *
 * \param RimHandle Handle to the raw input manager registration to unregister.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMUnregisterForInput system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMUnregisterForInput(
    _In_ HANDLE RimHandle
    );

// rev
/**
 * The RIMUpdateInputObserverRegistration routine updates the registration parameters of a raw input observer.
 *
 * \param ObserverHandle Handle to the raw input observer.
 * \param Flags Flags specifying updated observer registration settings.
 * \param Buffer Optional pointer to the observer configuration buffer.
 * \param BufferSize Optional size of the observer configuration buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtRIMUpdateInputObserverRegistration system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
RIMUpdateInputObserverRegistration(
    _In_ HANDLE ObserverHandle,
    _In_ ULONG Flags,
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_opt_ ULONG BufferSize
    );

//
// Pointer, Touch & Gesture Input
//

typedef enum _LOW_LATENCY_PROFILE_REQUEST_REASON
{
    LowLatencyProfileReasonUnknown = 0,
    LowLatencyProfileReasonGaming = 1,
    LowLatencyProfileReasonMedia = 2,
    LowLatencyProfileReasonAudio = 3,
    LowLatencyProfileReasonMax
} LOW_LATENCY_PROFILE_REQUEST_REASON;

// rev
/**
 * The GetExtendedPointerDeviceProperty routine retrieves an extended property of a pointer input device.
 *
 * \param DeviceHandle Handle to the pointer device.
 * \param Property Pointer to the property structure to receive data.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserGetExtendedPointerDeviceProperty system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetExtendedPointerDeviceProperty(
    _In_ HANDLE DeviceHandle,
    _In_ PVOID Property
    );

// rev
/**
 * The GetPointerDeviceInputSpace routine retrieves the coordinate input space mapping of a pointer device.
 *
 * \param DeviceHandle Handle to the pointer device.
 * \param InputSpace Pointer to a structure that receives the input space coordinates.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserGetPointerDeviceInputSpace system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetPointerDeviceInputSpace(
    _In_ HANDLE DeviceHandle,
    _Out_ PVOID InputSpace
    );

// rev
/**
 * The GetPointerDeviceOrientation routine retrieves the orientation of a pointer input device.
 *
 * \param DeviceHandle Handle to the pointer device.
 * \param Orientation Pointer to a variable receiving orientation information.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserGetPointerDeviceOrientation system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetPointerDeviceOrientation(
    _In_ HANDLE DeviceHandle,
    _Out_ PVOID Orientation
    );

// rev
/**
 * The GetPointerFrameTimes routine retrieves timing information for a pointer input frame.
 *
 * \param FrameId Identifier of the input frame.
 * \param FrameCount Number of frames in the buffer.
 * \param FrameTimes Buffer receiving frame timing records.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserGetPointerFrameTimes system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetPointerFrameTimes(
    _In_ LONG FrameId,
    _In_ ULONG FrameCount,
    _Out_writes_bytes_(144 * FrameCount) PVOID FrameTimes
    );

// rev
/**
 * The InitializePointerDeviceInjection routine creates a synthetic pointer device used for input injection.
 *
 * \param VendorId Vendor ID of the pointer device.
 * \param ProductId Product ID of the pointer device.
 * \param Padding Reserved alignment padding.
 * \param Description Device description or flags.
 * \param ReturnHandle Pointer that receives the synthetic device handle.
 * \return ULONG_PTR Status or pointer device handle.
 * \remarks Forwards to the NtUserCreateSyntheticPointerDevice2 system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
InitializePointerDeviceInjection(
    _In_ ULONG VendorId,
    _In_ ULONG ProductId,
    _In_ PVOID Padding,
    _In_ ULONG Description,
    _Out_ PVOID ReturnHandle
    );

// rev
/**
 * The InitializePointerDeviceInjectionEx routine provides extended creation of a synthetic pointer device used for input injection.
 *
 * \param DeviceType Type of the input device.
 * \param PointerType Type of pointer (e.g. touch or pen).
 * \param InputCount Maximum concurrent contacts supported.
 * \param InjectionFlags Operational flags for input injection.
 * \param InputBuffer Optional pointer to input buffer description.
 * \param InjectionHandle Optional pointer that receives the injection handle.
 * \return ULONG_PTR Handle or status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
InitializePointerDeviceInjectionEx(
    _In_ LONG DeviceType,
    _In_ LONG PointerType,
    _In_ LONG InputCount,
    _In_ LONG InjectionFlags,
    _In_opt_ PVOID InputBuffer,
    _Out_opt_ PVOID InjectionHandle
    );

// rev
/**
 * The InjectPointerInput routine injects synthetic pointer data packets into the system input pipeline.
 *
 * \param DeviceHandle Handle to the synthetic pointer device.
 * \param PointerInfo Pointer to an array of POINTER_TYPE_INFO structures describing pointer state.
 * \param Count Number of pointer info structures in the array.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInjectPointerInput system call.
 */
NTSYSAPI
BOOL
NTAPI
InjectPointerInput(
    _In_ HANDLE DeviceHandle,
    _In_reads_(Count) const POINTER_TYPE_INFO* PointerInfo,
    _In_ ULONG Count
    );

// rev
/**
 * The MITSynthesizeTouchInput routine injects synthesized touch input events via Modern Input Transport.
 *
 * \param TouchData Pointer to the touch input descriptor structure.
 * \return ULONG_PTR Status code or number of contacts synthesized.
 * \remarks Forwards to the NtMITSynthesizeTouchInput system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MITSynthesizeTouchInput(
    _In_ PVOID TouchData
    );

// rev
/**
 * The NtHWCursorUpdatePointer routine updates the hardware cursor pointer shape and position.
 *
 * \param CursorId The cursor identifier.
 * \param PointerInfo A pointer to pointer update information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtHWCursorUpdatePointer(
    _In_ ULONG64 CursorId,
    _In_ PVOID PointerInfo
    );

// rev
/**
 * The NtSetPointerDeviceInputSpace routine sets the input space for a pointer device.
 *
 * \param DeviceHandle The pointer device identifier.
 * \param InputSpaceId Optional pointer to the input space identifier.
 * \param InputSpaceInfo A pointer to input space configuration data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtSetPointerDeviceInputSpace(
    _In_ LONG_PTR DeviceHandle,
    _In_opt_ PVOID InputSpaceId,
    _In_ PVOID InputSpaceInfo
    );

// rev
/**
 * The NtUserAutoPromoteMouseInPointer routine enables or disables automatic promotion of mouse input to pointer messages.
 *
 * \param Promote Non-zero to enable auto-promotion of mouse messages; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAutoPromoteMouseInPointer(
    _In_ ULONG Promote
    );

// rev
/**
 * The NtUserConvertPrimaryPointerToMouseDrag routine converts an active primary pointer interaction into a traditional mouse drag operation.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserConvertPrimaryPointerToMouseDrag(
    VOID
    );

// rev
/**
 * The NtUserCreateSyntheticPointerDevice2 routine creates a synthetic pointer device for simulated touch or pen input.
 *
 * \param PointerDeviceInfo Pointer to a structure describing the synthetic pointer device configuration
 * (device type, maximum pointer count, and flags; copied from user memory).
 * \param PointerDeviceHandle Pointer to a variable that receives the created synthetic pointer device handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateSyntheticPointerDevice2(
    _In_ PVOID PointerDeviceInfo,
    _Out_ PULONG_PTR PointerDeviceHandle
    );

// rev
/**
 * The NtUserDelegateCapturePointers routine queries or delegates active pointer captures to an input target.
 *
 * \param PointerId Identifier of the primary pointer device.
 * \param CapturedPointers Pointer to an array receiving delegated pointer identifiers.
 * \param Count Pointer to a variable holding the maximum pointers to receive, and receiving the returned count.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDelegateCapturePointers(
    _In_ ULONG PointerId,
    _Out_ PULONG CapturedPointers,
    _Out_ PULONG Count
    );

// rev
/**
 * The NtUserDiscardPointerFrameMessages routine discards pending pointer frame messages for a pointer device.
 *
 * \param PointerId Identifier of the pointer whose pending frame messages are discarded.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!SkipPointerFrameMessages.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDiscardPointerFrameMessages(
    _In_ ULONG PointerId
    );

// rev
/**
 * The NtUserDownlevelTouchpad routine injects or processes downlevel legacy precision touchpad input packets.
 *
 * \param DeviceId Identifier of the precision touchpad device.
 * \param DownlevelInput Pointer to downlevel touchpad input data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDownlevelTouchpad(
    _In_ LONG DeviceId,
    _In_ PVOID DownlevelInput
    );

// rev
/**
 * The NtUserEnableMouseInPointer routine enables or disables mouse-in-pointer message translation for the calling process.
 *
 * \param FEnable TRUE to enable mouse-in-pointer; FALSE to disable.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!EnableMouseInPointer.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEnableMouseInPointer(
    _In_ BOOL FEnable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnableMouseInPointerForThread routine enables mouse-in-pointer message processing for the calling thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_ENABLEMOUSEINPOINTERFORTHREAD) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableMouseInPointerForThread(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnableMouseInPointerForWindow routine enables or disables mouse-in-pointer translation for a specific window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Enable Non-zero to enable mouse-in-pointer; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableMouseInPointerForWindow(
    _In_ HWND WindowHandle,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserEnableTouchPad routine enables or disables precision touchpad input processing.
 *
 * \param Enable Non-zero to enable touchpad input; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableTouchPad(
    _In_ LONG_PTR Enable
    );

// rev
/**
 * The NtUserGetExtendedPointerDeviceProperty routine retrieves extended properties for an input pointer device.
 *
 * \param DeviceHandle Handle to the pointer device.
 * \param Property Pointer to a structure specifying the requested property and receiving output data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetExtendedPointerDeviceProperty(
    _In_ HANDLE DeviceHandle,
    _Inout_ PVOID Property
    );

// rev
/**
 * The NtUserGetGestureConfig routine retrieves the gesture configuration for a window.
 *
 * \param Hwnd Handle to the window whose gesture configuration is queried.
 * \param DwReserved Reserved; must be 0.
 * \param DwFlags Configuration query flags.
 * \param PcIDs Pointer to a variable holding the number of GESTURECONFIG structures, and receiving the returned count.
 * \param PGestureConfig Pointer to an array of GESTURECONFIG structures receiving configuration.
 * \param CbSize Size, in bytes, of a single GESTURECONFIG structure.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetGestureConfig.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetGestureConfig(
    _In_ HWND Hwnd,
    _In_ ULONG DwReserved,
    _In_ ULONG DwFlags,
    _In_ PULONG PcIDs,
    _Inout_updates_(*PcIDs) PGESTURECONFIG PGestureConfig,
    _In_ ULONG CbSize
    );

// rev
/**
 * The NtUserGetGestureExtArgs routine retrieves extended argument data associated with a touch gesture.
 *
 * \param GestureHandle Handle to the gesture info object (HGESTUREINFO).
 * \param BufferSize Size, in bytes, of the ExtArgs buffer.
 * \param ExtArgs Pointer to a buffer receiving extended gesture arguments.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetGestureExtArgs(
    _In_ HANDLE GestureHandle,
    _In_ ULONG BufferSize,
    _Out_ PVOID ExtArgs
    );

// rev
/**
 * The NtUserGetGestureInfo routine retrieves information about a touch gesture from its handle.
 *
 * \param GestureHandle Handle to the gesture info structure (HGESTUREINFO).
 * \param GestureInfo Pointer to a GESTUREINFO structure receiving gesture details.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetGestureInfo(
    _In_ HANDLE GestureHandle,
    _Out_ PVOID GestureInfo
    );

// rev
/**
 * The NtUserGetPointerCursorId routine retrieves the cursor identifier associated with a specified pointer.
 *
 * \param PointerId Identifier of the pointer to query.
 * \param CursorId Pointer to a variable receiving the cursor identifier.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerCursorId.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerCursorId(
    _In_ ULONG PointerId,
    _Out_ PULONG CursorId
    );

// rev
/**
 * The NtUserGetPointerDevice routine retrieves information about a pointer device.
 *
 * \param Device Handle to the pointer device to query.
 * \param PointerDevice Pointer to a POINTER_DEVICE_INFO structure receiving device details.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerDevice.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerDevice(
    _In_ HANDLE Device,
    _Out_writes_(1) POINTER_DEVICE_INFO *PointerDevice
    );

// rev
/**
 * The NtUserGetPointerDeviceCursors routine retrieves cursor information for the specified pointer device.
 *
 * \param Device Handle to the pointer device.
 * \param CursorCount Pointer to a variable holding the maximum cursors to receive, and receiving the returned count.
 * \param DeviceCursors Optional pointer to an array of POINTER_DEVICE_CURSOR_INFO structures.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerDeviceCursors.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerDeviceCursors(
    _In_ HANDLE Device,
    _Inout_ ULONG* CursorCount,
    _Out_writes_opt_(*CursorCount) POINTER_DEVICE_CURSOR_INFO *DeviceCursors
    );

// rev
/**
 * The NtUserGetPointerDeviceInputSpace routine retrieves the coordinate input space boundaries for a pointer device.
 *
 * \param DeviceId Identifier of the pointer device.
 * \param InputSpace Pointer to a structure receiving the input space dimensions and coordinate bounds.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetPointerDeviceInputSpace(
    _In_ LONG_PTR DeviceId,
    _Out_ PVOID InputSpace
    );

// rev
/**
 * The NtUserGetPointerDeviceOrientation routine retrieves the physical mounting orientation of a pointer device.
 *
 * \param DeviceId Identifier of the pointer device.
 * \param Orientation Pointer to a variable receiving orientation degrees or transform data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetPointerDeviceOrientation(
    _In_ LONG_PTR DeviceId,
    _Out_ PVOID Orientation
    );

// rev
/**
 * The NtUserGetPointerDeviceProperties routine retrieves the extended properties of a pointer device.
 *
 * \param Device Handle to the pointer device.
 * \param PropertyCount Pointer to a variable holding the maximum properties to receive, and receiving the returned count.
 * \param PointerProperties Optional pointer to an array of POINTER_DEVICE_PROPERTY structures.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerDeviceProperties.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerDeviceProperties(
    _In_ HANDLE Device,
    _Inout_ ULONG* PropertyCount,
    _Out_writes_opt_(*PropertyCount) POINTER_DEVICE_PROPERTY *PointerProperties
    );

// rev
/**
 * The NtUserGetPointerDeviceRects routine retrieves the pointer device coordinates and display coordinates rectangles.
 *
 * \param Device Handle to the pointer device.
 * \param PointerDeviceRect Pointer to a RECT structure receiving the pointer device area.
 * \param DisplayRect Pointer to a RECT structure receiving the mapped display area.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerDeviceRects.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerDeviceRects(
    _In_ HANDLE Device,
    _Out_writes_(1) RECT* PointerDeviceRect,
    _Out_writes_(1) RECT* DisplayRect
    );

// rev
/**
 * The NtUserGetPointerDevices routine retrieves information about all pointer devices attached to the system.
 *
 * \param DeviceCount Pointer to a variable holding the maximum device count and receiving the returned count.
 * \param PointerDevices Optional pointer to an array of POINTER_DEVICE_INFO structures.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerDevices.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerDevices(
    _Inout_ ULONG* DeviceCount,
    _Out_writes_opt_(*DeviceCount) POINTER_DEVICE_INFO *PointerDevices
    );

// rev
/**
 * The NtUserGetPointerFrameTimes routine retrieves timestamp history for pointer input frames.
 *
 * \param PointerId Identifier of the pointer.
 * \param Count Number of historical frame records to retrieve.
 * \param FrameTimes Pointer to a buffer receiving the array of frame timestamp structures.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetPointerFrameTimes(
    _In_ LONG PointerId,
    _In_ ULONG Count,
    _Out_writes_bytes_(144 * Count) PVOID FrameTimes
    );

// rev
/**
 * The NtUserGetPointerIdForPromotion routine queries the pointer ID designated for promotion to mouse emulation.
 *
 * \return ULONG Pointer identifier.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetPointerIdForPromotion(
    VOID
    );

// rev
/**
 * The NtUserGetPointerInfoList routine retrieves a list of pointer information structures for an active pointer interaction.
 *
 * \param PointerId Identifier of the pointer.
 * \param PointerType Pointer device type (PT_*).
 * \param History Non-zero to retrieve history info.
 * \param Frame Non-zero to retrieve frame info.
 * \param ElementSize Size of each element in the info list buffer.
 * \param EntryCount In/out pointer to entry count.
 * \param PointerCount In/out pointer to pointer count.
 * \param PointerInfoList Pointer to a buffer receiving the list of pointer information structures.
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerInfoList(
    _In_ ULONG PointerId,
    _In_ ULONG PointerType,
    _In_ BOOL History,
    _In_ BOOL Frame,
    _In_ ULONG ElementSize,
    _Inout_ PULONG EntryCount,
    _Inout_ PULONG PointerCount,
    _Out_writes_bytes_opt_(*EntryCount * ElementSize) PVOID PointerInfoList
    );

// rev
/**
 * The NtUserGetPointerInputTransform routine retrieves the coordinate transform matrix for a pointer input message.
 *
 * \param PointerId Identifier of the pointer to query.
 * \param HistoryCount Number of transforms to retrieve.
 * \param InputTransform Pointer to an array of INPUT_TRANSFORM structures receiving the transform matrices.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerInputTransform.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerInputTransform(
    _In_ ULONG PointerId,
    _In_ ULONG HistoryCount,
    _Out_writes_(HistoryCount) INPUT_TRANSFORM *InputTransform
    );

// rev
/**
 * The NtUserGetPointerProprietaryId routine retrieves the proprietary hardware identifier for a pointer device.
 *
 * \param DeviceId Identifier of the pointer device.
 * \param ProprietaryId Pointer to a variable receiving the proprietary hardware identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetPointerProprietaryId(
    _In_ USHORT DeviceId,
    _Out_ PVOID ProprietaryId
    );

// rev
/**
 * The NtUserGetPointerType routine retrieves the pointer type for a specified pointer.
 *
 * \param PointerId Identifier of the pointer of interest.
 * \param PointerType Pointer to a POINTER_INPUT_TYPE variable receiving the pointer device type (PT_*).
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetPointerType.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetPointerType(
    _In_ ULONG PointerId,
    _Out_ POINTER_INPUT_TYPE *PointerType
    );

// rev
/**
 * The NtUserGetRawPointerDeviceData routine retrieves raw pointer device packet data.
 *
 * \param PointerId Identifier of the pointer of interest.
 * \param HistoryCount Number of historical input packets to retrieve.
 * \param PropertiesCount Number of device properties to retrieve per packet.
 * \param Properties Pointer to an array of POINTER_DEVICE_PROPERTY structures identifying the properties.
 * \param Values Pointer to an array of HistoryCount * PropertiesCount LONG values receiving the raw properties.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetRawPointerDeviceData.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetRawPointerDeviceData(
    _In_ ULONG PointerId,
    _In_ ULONG HistoryCount,
    _In_ ULONG PropertiesCount,
    _In_reads_(PropertiesCount) POINTER_DEVICE_PROPERTY* Properties,
    _Out_writes_(HistoryCount * PropertiesCount) PLONG Values
    );

// rev
/**
 * The NtUserGetTouchInputInfo routine retrieves detailed information about touch inputs associated with a touch input handle.
 *
 * \param TouchInputHandle Handle to the touch input structure received in a WM_TOUCH message.
 * \param Count Number of structures in the TouchInputs array.
 * \param TouchInputs Pointer to an array of TOUCHINPUT structures to receive touch data.
 * \param TouchInputSize Size, in bytes, of a TOUCHINPUT structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetTouchInputInfo(
    _In_ HANDLE TouchInputHandle,
    _In_ ULONG Count,
    _Out_ PVOID TouchInputs,
    _In_ LONG TouchInputSize
    );

// rev
/**
 * The NtUserGetTouchValidationStatus routine retrieves the validation status of a touch input sequence.
 *
 * \param TouchInputHandle Handle to the touch input sequence.
 * \return ULONG The touch validation status for the window.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetTouchValidationStatus(
    _In_ HANDLE TouchInputHandle
    );

// rev
/**
 * The NtUserHidePointerContactVisualization routine suppresses visual contact feedback for a specified pointer interaction.
 *
 * \param PointerId Identifier of the pointer whose visualization is hidden.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHidePointerContactVisualization(
    _In_ LONG PointerId
    );

// rev
/**
 * The NtUserInitializeTouchInjection routine configures touch input injection for the calling application.
 *
 * \param MaxCount The maximum number of simultaneous touch contact points to configure (up to 256).
 * \param DwMode Touch injection visualization and contact mode flags (TOUCH_FEEDBACK_*).
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InitializeTouchInjection.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInitializeTouchInjection(
    _In_ ULONG MaxCount,
    _In_ ULONG DwMode
    );

// rev
/**
 * The NtUserInjectGesture routine injects a synthetic gesture event into a target window.
 *
 * \param WindowHandle Handle to the target window receiving the gesture.
 * \param Param2 Gesture command or phase parameter.
 * \param Param3 Additional gesture control flags.
 * \param GestureInfo Pointer to a structure containing gesture parameters and coordinates.
 * \param Address Pointer to status or feedback variable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserInjectGesture(
    _In_ HWND WindowHandle,
    _In_ LONG Param2,
    _In_ LONG_PTR Param3,
    _In_ PVOID GestureInfo,
    _Inout_ PVOID Address
    );

// rev
/**
 * The NtUserInjectPointerInput routine injects synthetic pointer data packets into the system input pipeline.
 *
 * \param Device Handle to the synthetic pointer device.
 * \param PointerInfo Pointer to an array of POINTER_TYPE_INFO structures describing pointer state.
 * \param Count Number of pointer info structures in the array.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectPointerInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectPointerInput(
    _In_ HANDLE Device,
    _In_reads_(Count) const POINTER_TYPE_INFO* PointerInfo,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserInjectTouchInput routine injects a frame of touch contacts into the touch input pipeline.
 *
 * \param Count Number of touch contact points in the Contacts array.
 * \param Contacts Pointer to an array of POINTER_TOUCH_INFO structures describing touch contacts.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectTouchInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectTouchInput(
    _In_ ULONG Count,
    _In_reads_(Count) const POINTER_TOUCH_INFO *Contacts
    );

// rev
/**
 * The NtUserInjectTouchpadAction routine injects a high-level touchpad action or gesture into the system.
 *
 * \param HDevice Handle to the synthetic touchpad device.
 * \param Action Touchpad action code (TOUCHPAD_ACTION).
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectTouchpadAction.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectTouchpadAction(
    HSYNTHETICPOINTERDEVICE HDevice,
    TOUCHPAD_ACTION Action
    );

// rev
/**
 * The NtUserIsMouseInPointerEnabled routine indicates whether mouse input is being translated to pointer messages.
 *
 * \return BOOL TRUE if mouse-in-pointer mode is enabled, FALSE otherwise.
 * \remarks Native entry point for USER32!IsMouseInPointerEnabled.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserIsMouseInPointerEnabled(
    VOID
    );

// rev
/**
 * The NtUserIsTouchWindow routine checks whether a specified window is touch-capable and optionally retrieves event flags.
 *
 * \param WindowHandle A handle to the window to query.
 * \param Flags Optional pointer to a variable receiving the touch window flags.
 * \return TRUE if the window is registered for touch; FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserIsTouchWindow(
    _In_ HWND WindowHandle,
    _Out_opt_ PULONG Flags
    );

// rev
/**
 * The NtUserModifyWindowTouchCapability routine modifies touch input support and capabilities for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Flags Touch capability modification flags.
 * \param Enable Non-zero to enable specified capabilities; 0 to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserModifyWindowTouchCapability(
    _In_ HWND WindowHandle,
    _In_ LONG Flags,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserPromoteMouseInPointer routine manages the promotion of mouse messages to pointer messages.
 *
 * \param Param1 Unconfirmed promotion context parameter.
 * \param Param2 Unconfirmed promotion flags parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPromoteMouseInPointer(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtUserPromotePointer routine promotes an active pointer interaction to mouse input.
 *
 * \param PointerId Pointer identifier, from 2 through 65535.
 * \param Flags Promotion flags. Zero is accepted; otherwise low 24 bits select specific promotion forms.
 * \return LOGICAL Nonzero on success, zero otherwise.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPromotePointer(
    _In_ ULONG PointerId,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserRegisterManipulationThread routine registers the current thread or specified thread ID as an interaction manipulation processing thread.
 *
 * \param Param1 Thread identifier or manipulation thread registration parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterManipulationThread(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtUserRegisterPointerDeviceNotifications routine registers a window to receive notifications when pointer input devices are attached, detached, or modified.
 *
 * \param Window A handle to the window receiving notifications.
 * \param NotifyRange TRUE to receive range notifications; FALSE otherwise.
 * \return TRUE if registration succeeded, or FALSE otherwise.
 * \remarks Native entry point for USER32!RegisterPointerDeviceNotifications.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterPointerDeviceNotifications(
    _In_ HWND Window,
    _In_ BOOL NotifyRange
    );

// rev
/**
 * The NtUserRegisterPointerInputTarget routine registers or unregisters a window as a dedicated target for specified pointer input device types.
 *
 * \param WindowHandle A handle to the window to register.
 * \param Param2 Additional registration flags or target parameter.
 * \param PointerType The pointer input device type to register for.
 * \param Enable Nonzero to enable targeting; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterPointerInputTarget(
    _In_ HWND WindowHandle,
    _In_ LONG_PTR Param2,
    _In_ ULONG PointerType,
    _In_ ULONG Enable
    );

// rev
/**
 * The NtUserRegisterTouchHitTestingWindow routine registers a window to process WM_TOUCHHITTESTING notifications.
 *
 * \param Hwnd The handle to the window to register.
 * \param Value Registration options or flags specifying hit-testing processing behavior.
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!RegisterTouchHitTestingWindow.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterTouchHitTestingWindow(
    _In_ HWND Hwnd,
    _In_ ULONG Value
    );

// rev
/**
 * The NtUserRegisterTouchPadCapable routine registers or unregisters the calling thread as capable of handling precision touchpad messages.
 *
 * \param Enable TRUE to enable touchpad processing capability; FALSE to disable.
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!RegisterTouchpadCapableThread.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterTouchPadCapable(
    _In_ BOOL Enable
    );

// rev
/**
 * The NtUserRegisterTouchpadCapableWindow routine registers or unregisters a window as capable of handling precision touchpad input.
 *
 * \param HWnd A handle to the window to configure.
 * \param Enable TRUE to enable touchpad capability; FALSE to disable.
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!RegisterTouchpadCapableWindow.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterTouchpadCapableWindow(
    _In_ HWND HWnd,
    _In_ BOOL Enable
    );

// rev
/**
 * The NtUserRequestLowLatencyProfile routine submits a low-latency profile request.
 *
 * \param Profile Readable eight-byte buffer. Its original structure identity is unresolved.
 * \param Reason Request reason, either 0 or 1. This is not a bitmask.
 * \return TRUE when the request is submitted, FALSE otherwise.
 *
 * \remarks The examined implementation copies eight bytes from Profile but does not subsequently
 *          consume the copied contents. Invalid reasons set ERROR_INVALID_PARAMETER. Process eligibility
 *          and environment checks can fail with ERROR_ACCESS_DENIED or ERROR_NOT_SUPPORTED.
 *          Success indicates submission of the request; the helper can decline to arm the profile.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRequestLowLatencyProfile(
    _In_reads_bytes_(8) PVOID Profile,
    _In_ ULONG Reason
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetDialogPointer routine sets the dialog pointer for the specified window.
 *
 * \param WindowHandle Handle to the target window.
 * \param Ptr User-defined pointer value associated with the dialog window.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallHwndParam(SFI_SETDIALOGPOINTER) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDialogPointer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR Ptr
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetGestureConfig routine configures the messages that are sent from Windows Touch gestures to a window.
 *
 * \param Hwnd A handle to the window to configure gestures for.
 * \param DwReserved Reserved; must be zero.
 * \param CIDs The number of gesture configuration structures in the array.
 * \param PGestureConfig An array of GESTURECONFIG structures specifying gesture configuration.
 * \param CbSize The size, in bytes, of a GESTURECONFIG structure.
 * \return TRUE if the function succeeds, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetGestureConfig.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetGestureConfig(
    _In_ HWND Hwnd,
    _In_ ULONG DwReserved,
    _In_ ULONG CIDs,
    _In_reads_(CIDs) PGESTURECONFIG PGestureConfig,
    _In_ ULONG CbSize
    );

// rev
/**
 * The NtUserSetManipulationInputTarget routine sets manipulation processing input targets and routing boundaries for pointer sequences.
 *
 * \param Param1 Manipulation session identifier.
 * \param Param2 In-out pointer to manipulation target parameters.
 * \param PointerCount The number of active pointers participating in the manipulation.
 * \param PointerIds Pointer to the PointCount pointer identifiers, 4 bytes each.
 * \param ManipulationData A pointer to the manipulation configuration data structure.
 * \param Param6 Flags controlling manipulation targeting.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetManipulationInputTarget(
    _In_ ULONG Param1,
    _Inout_ PVOID Param2,
    _In_ ULONG PointerCount,
    _In_reads_bytes_(4 * PointerCount) PVOID PointerIds,
    _In_ PVOID ManipulationData,
    _In_ LONG Param6
    );

// rev
/**
 * The NtUserSetMaxTouchpadSensitivity routine configures maximum touch sensitivity threshold enforcement on precision touchpad devices.
 *
 * \param Enable Nonzero to enable maximum sensitivity; zero to restore standard sensitivity.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetMaxTouchpadSensitivity(
    _In_ LONG Enable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The RequestLowLatencyProfile routine requests a low-latency profile for a device or session.
 *
 * \param Luid The locally unique identifier (LUID) of the adapter or device.
 * \param Reason The reason for requesting low-latency mode (LOW_LATENCY_PROFILE_REQUEST_REASON).
 * \return TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
LOGICAL
NTAPI
RequestLowLatencyProfile(
    _In_ LUID Luid,
    _In_ LOW_LATENCY_PROFILE_REQUEST_REASON Reason
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The SetPointerDeviceInputSpace routine sets the input space (coordinate mapping) of a pointer device.
 *
 * \param DeviceHandle Handle to the target pointer device.
 * \param InputSpaceInfo1 Primary input space mapping data.
 * \param InputSpaceInfo2 Secondary input space mapping data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtSetPointerDeviceInputSpace system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetPointerDeviceInputSpace(
    _In_ HANDLE DeviceHandle,
    _In_opt_ PVOID InputSpaceInfo1,
    _In_opt_ PVOID InputSpaceInfo2
    );

//
// Magnification Services
//

// rev
/**
 * The GetMagnificationDesktopColorEffect routine retrieves the desktop color effect transformation matrix applied by the magnifier.
 *
 * \param ColorEffect Pointer to a buffer receiving the color effect matrix.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetMagnificationDesktopColorEffect(
    _Out_ PVOID ColorEffect
    );

// rev
/**
 * The GetMagnificationDesktopMagnification routine retrieves the current desktop magnification scale factor and coordinate offsets.
 *
 * \param MagnificationFactor Pointer receiving the magnification scale factor.
 * \param OffsetX Pointer receiving the horizontal pan offset.
 * \param OffsetY Pointer receiving the vertical pan offset.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetMagnificationDesktopMagnification(
    _Out_ PULONG64 MagnificationFactor,
    _Out_ PLONG OffsetX,
    _Out_ PULONG OffsetY
    );

// rev
/**
 * The GetMagnificationDesktopMonitorMagnification routine retrieves the magnification factor and panning offsets for a specific monitor.
 *
 * \param MonitorHandle Handle to the target monitor.
 * \param MagnificationFactor Pointer receiving the zoom scale factor.
 * \param OffsetX Pointer receiving the horizontal pan offset.
 * \param OffsetY Pointer receiving the vertical pan offset.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserMagControl system call (control group -3, code 0x0B).
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetMagnificationDesktopMonitorMagnification(
    _In_ HMONITOR MonitorHandle,
    _Out_ PDOUBLE MagnificationFactor,
    _Out_ PLONG OffsetX,
    _Out_ PLONG OffsetY
    );

// rev
/**
 * The GetMagnificationDesktopSamplingMode routine retrieves the image sampling mode used by the desktop magnifier.
 *
 * \param SamplingMode Pointer that receives the sampling mode identifier.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserMagGetContextInformation system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetMagnificationDesktopSamplingMode(
    _Out_ PULONG SamplingMode
    );

// rev
/**
 * The GetMagnificationInputTransformForMonitor routine retrieves coordinate input transform data for a specific monitor.
 *
 * \param MonitorHandle Handle to the target monitor.
 * \param Transform00 Output buffer for first transform component.
 * \param Transform01 Output buffer for second transform component.
 * \param Transform02 Output buffer for third transform component.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserMagControl system call (control group -3, code 0x0E).
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetMagnificationInputTransformForMonitor(
    _In_ HMONITOR MonitorHandle,
    _Out_ PVOID Transform00,
    _Out_ PVOID Transform01,
    _Out_ PVOID Transform02
    );

// rev
/**
 * The NtUserMagControl routine issues a control command to the system desktop magnification engine.
 *
 * \param ControlCode Magnification control code.
 * \param WindowHandle Optional handle to the magnification window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMagControl(
    _In_ ULONG ControlCode,
    _In_opt_ HWND WindowHandle
    );

// rev
/**
 * The NtUserMagGetContextInformation routine queries magnification context information for a magnifier window.
 *
 * \param WindowHandle Handle or context identifier of the magnification window.
 * \param InfoType Information class or property to query.
 * \param Information Pointer to a buffer receiving the context information.
 * \param InformationLength Size, in bytes, of the Information buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMagGetContextInformation(
    _In_ LONG_PTR WindowHandle,
    _In_ LONG InfoType,
    _Inout_ PVOID Information,
    _In_ ULONG_PTR InformationLength
    );

// rev
/**
 * The NtUserMagSetContextInformation routine configures magnification context settings for a magnifier window.
 *
 * \param WindowHandle Handle or context identifier of the magnification window.
 * \param InfoType Information class or property to set.
 * \param Information Pointer to a buffer containing the context settings to apply.
 * \param InformationLength Size, in bytes, of the Information buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMagSetContextInformation(
    _In_ LONG_PTR WindowHandle,
    _In_ LONG InfoType,
    _In_ PVOID Information,
    _In_ ULONG InformationLength
    );

// rev
/**
 * The NtUserSetFullscreenMagnifierOffsetsDWMUpdated routine updates Desktop Window Manager (DWM) fullscreen magnification viewport offsets and zoom level.
 *
 * \param Param1 Magnifier context identifier or display index.
 * \param Param2 Magnifier viewport configuration parameters.
 * \param Offset The magnification zoom offset or scaling factor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetFullscreenMagnifierOffsetsDWMUpdated(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _In_ FLOAT Offset
    );

// rev
/**
 * The NtUserSetMagnificationDesktopMagnifierOffsetsDWMUpdated routine updates Desktop Window Manager (DWM) desktop magnification offsets.
 *
 * \param Param1 Desktop magnification identifier or target monitor.
 * \param Param2 Magnification offset coordinates or parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetMagnificationDesktopMagnifierOffsetsDWMUpdated(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The SetFullscreenMagnifierOffsetsDWMUpdated routine notifies the magnifier that DWM has updated the fullscreen magnifier offsets.
 *
 * \param OffsetX Horizontal pan offset.
 * \param OffsetY Vertical pan offset.
 * \param Scale Current zoom scale factor.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserSetFullscreenMagnifierOffsetsDWMUpdated system call.
 */
NTSYSAPI
LOGICAL
NTAPI
SetFullscreenMagnifierOffsetsDWMUpdated(
    _In_ FLOAT OffsetX,
    _In_ FLOAT OffsetY,
    _In_ FLOAT Scale
    );

// rev
/**
 * The SetMagicColors routine sets magic color values for 16-color dithering.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagicColors(
    VOID
    );

// rev
/**
 * The SetMagnificationDesktopColorEffect routine sets the desktop color effect transformation matrix applied by the magnifier.
 *
 * \param ColorEffect Pointer to the color transformation matrix.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagnificationDesktopColorEffect(
    _In_ PVOID ColorEffect
    );

// rev
/**
 * The SetMagnificationDesktopMagnification routine sets the desktop magnification scale factor and panning offsets.
 *
 * \param MagnificationFactor The zoom factor to apply.
 * \param OffsetX Horizontal panning offset.
 * \param OffsetY Vertical panning offset.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagnificationDesktopMagnification(
    _In_ DOUBLE MagnificationFactor,
    _In_ LONG OffsetX,
    _In_ LONG OffsetY
    );

// rev
/**
 * The SetMagnificationDesktopMagnifierOffsetsDWMUpdated routine notifies the magnifier that DWM updated the desktop magnifier offsets.
 *
 * \param OffsetX Horizontal pan offset.
 * \param OffsetY Vertical pan offset.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtUserSetMagnificationDesktopMagnifierOffsetsDWMUpdated system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
SetMagnificationDesktopMagnifierOffsetsDWMUpdated(
    _In_ FLOAT OffsetX,
    _In_ FLOAT OffsetY
    );

// rev
/**
 * The SetMagnificationDesktopMonitorTransformList routine installs a list of per-monitor desktop transforms for the magnifier.
 *
 * \param Count Number of monitor transform entries in the array.
 * \param TransformList Pointer to the array of per-monitor transform records.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagnificationDesktopMonitorTransformList(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID TransformList
    );

// rev
/**
 * The SetMagnificationDesktopSamplingMode routine sets the image sampling mode used by the desktop magnifier.
 *
 * \param SamplingMode Pointer to the sampling mode setting.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagnificationDesktopSamplingMode(
    _In_ PLONG SamplingMode
    );

// rev
/**
 * The SetMagnificationInputTransformList routine installs a list of input coordinate transforms for the magnifier.
 *
 * \param Count Number of input transform entries in the array.
 * \param TransformList Pointer to the array of transform records.
 * \return ULONG_PTR Status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetMagnificationInputTransformList(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID TransformList
    );

//
// DirectComposition types
//
//
//enum DCOMPOSITION_BITMAP_INTERPOLATION_MODE
//{
//    DCOMPOSITION_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR = 0,
//    DCOMPOSITION_BITMAP_INTERPOLATION_MODE_LINEAR = 1,
//
//    DCOMPOSITION_BITMAP_INTERPOLATION_MODE_INHERIT = 0xffffffff
//};
//
//enum DCOMPOSITION_BORDER_MODE
//{
//    DCOMPOSITION_BORDER_MODE_SOFT = 0,
//    DCOMPOSITION_BORDER_MODE_HARD = 1,
//
//    DCOMPOSITION_BORDER_MODE_INHERIT = 0xffffffff
//};
//
//enum DCOMPOSITION_COMPOSITE_MODE
//{
//    DCOMPOSITION_COMPOSITE_MODE_SOURCE_OVER = 0,
//    DCOMPOSITION_COMPOSITE_MODE_DESTINATION_INVERT = 1,
//    DCOMPOSITION_COMPOSITE_MODE_MIN_BLEND = 2,
//
//    DCOMPOSITION_COMPOSITE_MODE_INHERIT = 0xffffffff
//};
//
//enum DCOMPOSITION_BACKFACE_VISIBILITY
//{
//    DCOMPOSITION_BACKFACE_VISIBILITY_VISIBLE = 0,
//    DCOMPOSITION_BACKFACE_VISIBILITY_HIDDEN = 1,
//
//    DCOMPOSITION_BACKFACE_VISIBILITY_INHERIT = 0xffffffff
//};
//
//enum DCOMPOSITION_OPACITY_MODE
//{
//    DCOMPOSITION_OPACITY_MODE_LAYER = 0,
//    DCOMPOSITION_OPACITY_MODE_MULTIPLY = 1,
//
//    DCOMPOSITION_OPACITY_MODE_INHERIT = 0xffffffff
//};
// 
//enum DCOMPOSITION_DEPTH_MODE
//{
//    DCOMPOSITION_DEPTH_MODE_TREE = 0,
//    DCOMPOSITION_DEPTH_MODE_SPATIAL = 1,
//    DCOMPOSITION_DEPTH_MODE_SORTED = 3,
//
//    DCOMPOSITION_DEPTH_MODE_INHERIT = 0xffffffff
//};

//typedef struct
//{
//    LARGE_INTEGER lastFrameTime;
//    DXGI_RATIONAL currentCompositionRate;
//    LARGE_INTEGER currentTime;
//    LARGE_INTEGER timeFrequency;
//    LARGE_INTEGER nextEstimatedFrameTime;
//} DCOMPOSITION_FRAME_STATISTICS;

//
// Composition object specific access flags
//

#define COMPOSITIONOBJECT_READ          0x0001L
#define COMPOSITIONOBJECT_WRITE         0x0002L

#define COMPOSITIONOBJECT_ALL_ACCESS    (COMPOSITIONOBJECT_READ | COMPOSITIONOBJECT_WRITE)

//
// Composition Stats
//

typedef ULONG64 COMPOSITION_FRAME_ID;

// The maximum nubmer of objects we allow users to wait on the compositor clock
#define DCOMPOSITION_MAX_WAITFORCOMPOSITORCLOCK_OBJECTS 32
// Maximum number of targets kept per frame
#define COMPOSITION_STATS_MAX_TARGETS 256//

//
// Composition object specific access flags
//

#define COMPOSITIONOBJECT_READ          0x0001L
#define COMPOSITIONOBJECT_WRITE         0x0002L
#define COMPOSITIONOBJECT_ALL_ACCESS    (COMPOSITIONOBJECT_READ | COMPOSITIONOBJECT_WRITE)

//
// Composition Stats
//

typedef enum COMPOSITION_FRAME_ID_TYPE COMPOSITION_FRAME_ID_TYPE;

typedef ULONG64 COMPOSITION_FRAME_ID;

struct tagCOMPOSITION_FRAME_STATS;
struct tagCOMPOSITION_TARGET_ID;
typedef struct tagCOMPOSITION_FRAME_STATS COMPOSITION_FRAME_STATS, *PCOMPOSITION_FRAME_STATS;
typedef struct tagCOMPOSITION_TARGET_ID COMPOSITION_TARGET_ID, *PCOMPOSITION_TARGET_ID;

// The maximum nubmer of objects we allow users to wait on the compositor clock
#define DCOMPOSITION_MAX_WAITFORCOMPOSITORCLOCK_OBJECTS 32

// Maximum number of targets kept per frame
#define COMPOSITION_STATS_MAX_TARGETS 256

// rev
/**
 * The CreateDCompositionHwndTarget routine creates a DirectComposition target bound to a window handle.
 *
 * \param WindowHandle Handle to the target window.
 * \param TargetFlags Flags controlling the composition target creation.
 * \param TargetHandle Pointer that receives the composition target handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserCreateDCompositionHwndTarget system call.
 */
NTSYSAPI
LOGICAL
NTAPI
CreateDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ ULONG TargetFlags,
    _Out_ PVOID TargetHandle
    );

// rev
/**
 * The DestroyDCompositionHwndTarget routine destroys a DirectComposition HWND target.
 *
 * \param WindowHandle Handle to the window associated with the composition target.
 * \param Flags Destruction flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserDestroyDCompositionHwndTarget system call.
 */
NTSYSAPI
LOGICAL
NTAPI
DestroyDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags
    );

// rev
/**
 * The GetDCompositionHwndBitmap routine retrieves the DirectComposition bitmap backing a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param BitmapHandle Pointer receiving the DirectComposition bitmap handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserGetDCompositionHwndBitmap system call.
 */
NTSYSAPI
LOGICAL
NTAPI
GetDCompositionHwndBitmap(
    _In_ HWND WindowHandle,
    _Out_ PVOID BitmapHandle
    );

// rev
/**
 * The GetWindowCompositionAttribute routine retrieves a DWM window composition attribute.
 *
 * \param WindowHandle Handle to the target window.
 * \param AttributeData Pointer to the composition attribute data buffer.
 * \return TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _Inout_ PVOID AttributeData
    );

// rev
/**
 * The GetWindowCompositionInfo routine retrieves Desktop Window Manager (DWM) composition attributes for a window.
 *
 * \param WindowHandle Handle to the target window.
 * \param CompositionInfo Pointer to a structure receiving the window's composition information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserGetWindowCompositionInfo system call.
 */
NTSYSAPI
LOGICAL
NTAPI
GetWindowCompositionInfo(
    _In_ HWND WindowHandle,
    _Out_ PVOID CompositionInfo
    );

// rev
/**
 * The NtBindCompositionSurface routine binds a composition surface to a visual or swap chain.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param Param2 The second binding parameter.
 * \param Flags Surface binding flags.
 * \param Param4 The fourth binding parameter.
 * \param BindInfo A pointer to bind configuration data.
 * \param SectionInfo A pointer to section information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtBindCompositionSurface(
    _In_ HANDLE CompositionSurface,
    _In_ LONG Param2,
    _In_ ULONG Flags,
    _In_ LONG Param4,
    _In_ PVOID BindInfo,
    _In_ PVOID SectionInfo
    );

// rev
/**
 * The NtCloseCompositionInputSink routine closes a composition input sink handle.
 *
 * \param Handle A handle to the composition input sink to close.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseCompositionInputSink(
    _In_ HANDLE Handle
    );

// rev
/**
 * The NtCompositionSetDropTarget routine sets the OLE/shell drop target for a composition object.
 *
 * \param CompositionObject A pointer or handle to the composition object.
 * \param DropTargetInfo A pointer to drop target information.
 * \param DropTargetContext Optional context passed with the drop target.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompositionSetDropTarget(
    _In_ PVOID CompositionObject,
    _In_ PVOID DropTargetInfo,
    _In_opt_ PVOID DropTargetContext
    );

// rev
/**
 * The NtConfirmCompositionSurfaceIndependentFlipEntry routine confirms an independent flip entry for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param FlipInfo A pointer to independent flip entry information.
 * \param Param3 Third confirmation parameter.
 * \param Param4 Fourth confirmation parameter.
 * \param Param5 Fifth confirmation parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtConfirmCompositionSurfaceIndependentFlipEntry(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID FlipInfo,
    _In_ ULONG Param3,
    _In_ ULONG Param4,
    _In_ ULONG Param5
    );

// rev
/**
 * The NtCreateCompositionInputSink routine creates a composition input sink.
 *
 * \param InputSinkInfo A pointer to input sink configuration info.
 * \param InputSink A pointer receiving the created input sink.
 * \return Integer status code.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtCreateCompositionInputSink(
    _In_ PVOID InputSinkInfo,
    _Out_ PVOID InputSink
    );

// rev
/**
 * The NtCreateCompositionSurfaceHandle routine creates a handle to a composition surface.
 *
 * \param Reserved Reserved for future use.
 * \param DesiredAccess The desired access mask for the composition surface.
 * \param CompositionSurface A pointer receiving the created composition surface handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateCompositionSurfaceHandle(
    _In_ LONG_PTR Reserved,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE CompositionSurface
    );

// rev
/**
 * The NtCreateImplicitCompositionInputSink routine creates an implicit composition input sink.
 *
 * \param InputSinkInfo A pointer to input sink configuration info.
 * \param InputSink A pointer receiving the created implicit input sink.
 * \return Integer status code.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtCreateImplicitCompositionInputSink(
    _In_ PVOID InputSinkInfo,
    _Out_ PVOID InputSink
    );

// rev
/**
 * The NtDCompositionAddCrossDeviceVisualChild routine adds a cross-device visual child to a DirectComposition visual.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param ParentVisual The parent visual identifier.
 * \param ChildDevice The child device identifier.
 * \param ChildVisual The child visual identifier.
 * \param InsertAbove Whether to insert the visual above the reference visual.
 * \param ReferenceVisual The reference visual identifier.
 * \param Flags Visual insertion control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionAddCrossDeviceVisualChild(
    _In_ ULONG Channel,
    _In_ ULONG ParentVisual,
    _In_ ULONG ChildDevice,
    _In_ ULONG ChildVisual,
    _In_ LONG InsertAbove,
    _In_ ULONG ReferenceVisual,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtDCompositionBeginFrame routine initiates a DirectComposition frame batch.
 *
 * \param Connection A handle to the DirectComposition connection.
 * \param BeginFrameInfo A pointer to a structure containing frame start parameters.
 * \param FrameInfo A pointer to a variable receiving frame information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionBeginFrame(
    _In_ HANDLE Connection,
    _In_ PVOID BeginFrameInfo,
    _Out_ PVOID FrameInfo
    );

// rev
/**
 * The NtDCompositionBoostCompositorClock routine boosts the DirectComposition compositor clock rate.
 *
 * \param Enable Specifies whether to enable or disable compositor clock boosting.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionBoostCompositorClock(
    _In_ LONG Enable
    );

// rev
/**
 * The NtDCompositionCommitChannel routine commits pending commands and resources on a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param CommitSize The size of the commit batch buffer.
 * \param BatchId The batch identifier.
 * \param Flags Channel commit flags.
 * \param BatchBuffer A pointer to the batch command buffer.
 * \param MarshalData A pointer to marshalling data.
 * \param ResourcesToRelease A pointer to an array of resources to release.
 * \param ResourceCount The number of resources to release.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCommitChannel(
    _In_ ULONG Channel,
    _Out_opt_ PULONG BatchId,
    _Out_ PBOOLEAN CommitResult,
    _In_ CHAR Flags,
    _In_opt_ HANDLE SynchronizationObject,
    _In_opt_ PVOID MarshalData,
    _In_reads_opt_(ResourceCount) const ULONG *Resources,
    _In_ ULONG ResourceCount
    );

// rev
/**
 * The NtDCompositionCommitSynchronizationObject routine commits a synchronization object for a DirectComposition channel.
 *
 * \param SynchronizationObject A pointer or handle to the synchronization object to commit.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCommitSynchronizationObject(
    _In_ PVOID SynchronizationObject
    );

// rev
/**
 * The NtDCompositionConfirmFrame routine confirms completion or status of a DirectComposition frame.
 *
 * \param Connection A handle to the DirectComposition connection.
 * \param FrameInfo A pointer to frame information to confirm.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionConfirmFrame(
    _In_ HANDLE Connection,
    _In_ PVOID FrameInfo
    );

// rev
/**
 * The NtDCompositionConnectPipe routine connects an inter-process DirectComposition pipe.
 *
 * \param Param1 The first connection parameter.
 * \param Param2 The second connection parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionConnectPipe(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtDCompositionCreateAndBindSharedSection routine creates and binds a shared memory section for DirectComposition.
 *
 * \param SectionSize The size of the shared memory section.
 * \param Flags Section creation flags.
 * \param SectionHandle The handle or identifier of the section.
 * \param MappedSection A pointer receiving the mapped section address.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateAndBindSharedSection(
    _In_ ULONG SectionSize,
    _In_ ULONG Flags,
    _In_ ULONG64 SectionHandle,
    _Out_ PVOID MappedSection
    );

// rev
/**
 * The NtDCompositionCreateBufferCollection routine creates a buffer collection for DirectComposition.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param BufferInfo A pointer to buffer collection information.
 * \param Buffers A pointer to the buffer descriptor array.
 * \param BufferCollectionHandle A pointer receiving the created buffer collection handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateBufferCollection(
    _In_ ULONG Channel,
    _In_ PVOID BufferInfo,
    _In_ PVOID Buffers,
    _Out_ PHANDLE BufferCollectionHandle
    );

// rev
/**
 * The NtDCompositionCreateChannel routine creates a channel for submitting DirectComposition commands.
 *
 * \param ChannelInfo A pointer to channel configuration information.
 * \param ChannelSection A pointer receiving channel section information.
 * \param ChannelHandle A pointer receiving the created channel handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateChannel(
    _In_ PVOID ChannelInfo,
    _Out_ PVOID ChannelSection,
    _Out_ PHANDLE ChannelHandle
    );

// rev
/**
 * The NtDCompositionCreateConnection routine creates a DirectComposition connection for the current DWM process.
 *
 * \param ConnectionOption Boolean connection option. Its specific meaning remains unresolved.
 * \param EventArgument Argument passed to creation of the connection event wrapper.
 * \param Connection Receives the opaque, pointer-sized connection handle on success.
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks Requires DWM and no existing connection for the process; otherwise returns
 * STATUS_ACCESS_DENIED. A NULL Connection pointer returns STATUS_INVALID_PARAMETER.
 * The output is written only after successful creation. ConnectionOption is normalized to Boolean.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateConnection(
    _In_ BOOL ConnectionOption,
    _In_ PVOID EventArgument,
    _Out_ PHANDLE Connection
    );

// rev
/**
 * The NtDCompositionCreateSharedResourceHandle routine creates a shared resource handle for DirectComposition.
 *
 * \param ResourceType The type of the shared resource to create.
 * \param ResourceHandle A pointer receiving the created shared resource handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateSharedResourceHandle(
    _In_ ULONG ResourceType,
    _Out_ PHANDLE ResourceHandle
    );

// rev
/**
 * The NtDCompositionCreateSynchronizationObject routine creates a DirectComposition synchronization object.
 *
 * \param SynchronizationObject A pointer receiving the created synchronization object handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionCreateSynchronizationObject(
    _Out_ PHANDLE SynchronizationObject
    );

// rev
/**
 * The NtDCompositionDestroyChannel routine destroys a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionDestroyChannel(
    _In_ ULONG Channel
    );

// rev
/**
 * The NtDCompositionDestroyConnection routine destroys a DirectComposition connection.
 *
 * \param Connection A handle to the DirectComposition connection to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionDestroyConnection(
    _In_ HANDLE Connection
    );

// rev
/**
 * The NtDCompositionDuplicateHandleToProcess routine duplicates a DirectComposition resource handle to a target process.
 *
 * \param ResourceHandle A handle or pointer to the resource to duplicate.
 * \param ProcessId The identifier of the target process.
 * \param TargetHandle A pointer receiving the duplicated handle in the target process.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionDuplicateHandleToProcess(
    _In_ PVOID ResourceHandle,
    _In_ LONG ProcessId,
    _Out_ PHANDLE TargetHandle
    );

// rev
/**
 * The NtDCompositionDuplicateSwapchainHandleToDwm routine duplicates a swap chain handle to the DWM process.
 *
 * \param SwapchainHandle A handle to the swap chain.
 * \param DwmHandle A pointer receiving the handle valid in the DWM process.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionDuplicateSwapchainHandleToDwm(
    _In_ HANDLE SwapchainHandle,
    _Out_ PHANDLE DwmHandle
    );

// rev
/**
 * The NtDCompositionEnableMMCSS routine enables or disables MMCSS scheduling for DirectComposition.
 *
 * \param Enable Specifies whether to enable or disable Multimedia Class Scheduler Service (MMCSS).
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionEnableMMCSS(
    _In_ LONG Enable
    );

// rev
/**
 * The NtDCompositionGetBatchId routine retrieves the current batch identifier for a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param Flags Operation flags.
 * \param BatchId A pointer receiving the batch identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetBatchId(
    _In_ ULONG Channel,
    _In_ LONG Flags,
    _Out_ PULONG BatchId
    );

// rev
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetChannels(
    ULONG_PTR Param1,
    ULONG Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    );

// rev
/**
 * The NtDCompositionGetConnectionBatch routine retrieves the connection batch buffer for a DirectComposition connection.
 *
 * \param Connection A handle to the DirectComposition connection.
 * \param BatchInfo A pointer to batch request parameters.
 * \param Batch A pointer receiving batch information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetConnectionBatch(
    _In_ HANDLE Connection,
    _In_ PVOID BatchInfo,
    _Out_ PVOID Batch
    );

// rev
/**
 * The NtDCompositionGetDeletedResources routine retrieves deleted DirectComposition resources on a channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param Count The number of resources to query.
 * \param InputBuffer A pointer to input buffer data.
 * \param DeletedResources A pointer receiving the deleted resource identifiers.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetDeletedResources(
    _In_ ULONG Channel,
    _In_ ULONG Count,
    _In_ PVOID InputBuffer,
    _Out_ PVOID DeletedResources
    );

// rev
/**
 * The NtDCompositionGetFrameId routine retrieves the frame identifier for a given frame type.
 *
 * \param FrameIdType The type of compositor frame.
 * \param FrameId The identifer of the most recent compositor frame of the specified type.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetFrameId(
    _In_ COMPOSITION_FRAME_ID_TYPE FrameIdType,
    _Out_ COMPOSITION_FRAME_ID* FrameId
    );

// rev
/**
 * The NtDCompositionGetFrameIdFromBatchId routine retrieves a frame identifier corresponding to a batch identifier.
 *
 * \param BatchId The batch identifier.
 * \param Flags Operation flags.
 * \param FrameId A pointer receiving the corresponding frame identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetFrameIdFromBatchId(
    _In_ ULONG BatchId,
    _In_ ULONG Flags,
    _Out_ COMPOSITION_FRAME_ID* FrameId
    );

// rev
/**
 * The NtDCompositionGetFrameLegacyTokens routine retrieves legacy composition frame tokens.
 *
 * \param FrameId A pointer to the frame identifier.
 * \param InputBuffer A pointer to input buffer data.
 * \param LegacyTokens A pointer receiving legacy frame token data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetFrameLegacyTokens(
    _In_ COMPOSITION_FRAME_ID FrameId,
    _In_ PVOID InputBuffer,
    _Out_ PVOID LegacyTokens
    );

// rev
/**
 * The NtDCompositionGetFrameStatistics routine retrieves statistics for a DirectComposition frame.
 *
 * \param FrameId A pointer to the frame identifier.
 * \param Statistics A pointer receiving frame statistics.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/mt589902(v=vs.85)
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetFrameStatistics(
    _In_ COMPOSITION_FRAME_ID FrameId,
    _Out_ VOID* Statistics // DCOMPOSITION_FRAME_STATISTICS 
    );

// rev
/**
 * The NtDCompositionGetFrameSurfaceUpdates routine retrieves surface update information for a DirectComposition frame.
 *
 * \param FrameId A pointer to the frame identifier.
 * \param InputBuffer A pointer to input buffer data.
 * \param SurfaceUpdates A pointer receiving surface update information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetFrameSurfaceUpdates(
    _In_ COMPOSITION_FRAME_ID FrameId,
    _In_ PVOID InputBuffer,
    _Out_ PVOID SurfaceUpdates
    );

// rev
/**
 * The NtDCompositionGetMaterialProperty routine retrieves a material property in DirectComposition.
 *
 * \param Param1 The first property parameter.
 * \param Param2 The second property parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetMaterialProperty(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtDCompositionGetStatistics routine retrieves basic information about the composition frame 
 * and a list of render target ID's that are part of the frame.
 *
 * \param FrameId The identifier of the composition frame about which to get information.
 * \param FrameStats A struct that contains information about the composition frame.
 * \param TargetIdCount The number of render targets about which to get information.
 * \param TargetIds The identifiers of the render targets about which to get information.
 * \param Statistics The actual number of render targets.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-dcompositiongetstatistics
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetStatistics(
    _In_ COMPOSITION_FRAME_ID FrameId,
    _Out_ COMPOSITION_FRAME_STATS* FrameStats,
    _In_ ULONG TargetIdCount,
    _Out_ COMPOSITION_TARGET_ID* TargetIds,
    _Out_ PULONG ActualTargetIdCount
    );

// rev
/**
 * The NtDCompositionGetTargetStatistics routine retrieves target statistics for a DirectComposition frame.
 *
 * \param FrameId The identifier of the composition frame about which to get information.
 * \param TargetId The identifier of the render target about which to get information.
 * \param TargetStatistics Information about the specified composition frame and render target.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nf-dcomp-dcompositiongettargetstatistics
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionGetTargetStatistics(
    _In_ COMPOSITION_FRAME_ID FrameId,
    _In_ const COMPOSITION_TARGET_ID* TargetId,
    _Out_ PVOID TargetStatistics // COMPOSITION_TARGET_STATS
    );

// rev
/**
 * The NtDCompositionNotifySuperWetInkWork routine notifies DirectComposition of pending super-wet ink work.
 *
 * \param Flags Notification control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionNotifySuperWetInkWork(
    _In_ ULONG Flags
    );

// rev
/**
 * The NtDCompositionProcessChannelBatchBuffer routine processes a batch command buffer on a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param Flags Batch processing flags.
 * \param InputBuffer A pointer to the batch input buffer.
 * \param OutputBuffer A pointer receiving batch output data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionProcessChannelBatchBuffer(
    _In_ ULONG Channel,
    _In_ ULONG Flags,
    _In_ PVOID InputBuffer,
    _Out_ PVOID OutputBuffer
    );

// rev
/**
 * The NtDCompositionRegisterThumbnailVisual routine registers a thumbnail visual with DirectComposition.
 *
 * \param WindowHandle A handle to the window.
 * \param ThumbnailVisual The thumbnail visual identifier.
 * \param Flags Registration flags.
 * \param SourceType The source type identifier.
 * \param SourceRect A pointer to the source bounding rectangle.
 * \param DestinationRect A pointer to the destination bounding rectangle.
 * \param Flags2 Extended registration flags.
 * \param ThumbnailInfo A pointer to thumbnail configuration info.
 * \param ThumbnailId A pointer receiving the thumbnail identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionRegisterThumbnailVisual(
    _In_ LONG_PTR WindowHandle,
    _In_ LONG_PTR ThumbnailVisual,
    _In_ CHAR Flags,
    _In_ LONG SourceType,
    _In_ PVOID SourceRect,
    _In_ PVOID DestinationRect,
    _In_ CHAR Flags2,
    _In_ PVOID ThumbnailInfo,
    _Out_ PVOID ThumbnailId
    );

// rev
/**
 * The NtDCompositionRegisterVirtualDesktopVisual routine registers a virtual desktop visual with DirectComposition.
 *
 * \param WindowHandle A handle to the virtual desktop window.
 * \param VisualInfo A pointer to visual configuration information.
 * \param VisualId A pointer receiving the registered visual identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionRegisterVirtualDesktopVisual(
    _In_ LONG_PTR WindowHandle,
    _In_ PVOID VisualInfo,
    _Out_ PVOID VisualId
    );

// rev
/**
 * The NtDCompositionReleaseAllResources routine releases all DirectComposition resources on a channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param ResourceInfo A pointer containing and receiving resource release info.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionReleaseAllResources(
    _In_ ULONG Channel,
    _Inout_ PVOID ResourceInfo
    );

// rev
/**
 * The NtDCompositionRemoveCrossDeviceVisualChild routine removes a cross-device visual child from a DirectComposition visual.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param ParentVisual The parent visual identifier.
 * \param ChildDevice The child device identifier.
 * \param ChildVisual The child visual identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionRemoveCrossDeviceVisualChild(
    _In_ ULONG Channel,
    _In_ ULONG ParentVisual,
    _In_ ULONG ChildDevice,
    _In_ ULONG ChildVisual
    );

// rev
/**
 * The NtDCompositionSendDwmLpcMessage routine sends an LPC message to the Desktop Window Manager (DWM).
 *
 * \param Message A pointer to the message buffer.
 * \param MessageSize The size of the message buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSendDwmLpcMessage(
    _In_ PVOID Message,
    _In_ ULONG_PTR MessageSize
    );

// rev
/**
 * The NtDCompositionSetBlurredWallpaperSurface routine sets a blurred wallpaper surface for DirectComposition.
 *
 * \param SurfaceInfo A pointer to blurred wallpaper surface info.
 * \param WallpaperSurface A pointer to the wallpaper surface descriptor.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSetBlurredWallpaperSurface(
    _In_ PVOID SurfaceInfo,
    _In_ PVOID WallpaperSurface
    );

// rev
/**
 * The NtDCompositionSetChannelCommitCompletionEvent routine sets an event signaled upon completion of a channel commit.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param EventHandle A handle to the event to signal on commit completion.
 * \param Flags Completion event flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSetChannelCommitCompletionEvent(
    _In_ ULONG Channel,
    _In_ HANDLE EventHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The NtDCompositionSetChannelConnectionId routine associates a connection identifier with a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param ConnectionId The connection identifier.
 * \param Reserved Reserved for future use.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSetChannelConnectionId(
    _In_ ULONG Channel,
    _In_ LONG ConnectionId,
    _In_ LONG_PTR Reserved
    );

// rev
/**
 * The NtDCompositionSetChildRootVisual routine sets the root visual for a child composition tree.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param RootVisualInfo A pointer to root visual configuration info.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSetChildRootVisual(
    _In_ LONG_PTR Channel,
    _In_ PVOID RootVisualInfo
    );

// rev
/**
 * The NtDCompositionSetDebugCounter routine sets a DirectComposition debug counter.
 *
 * \param Param1 The first debug counter parameter.
 * \param Param2 The second debug counter parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSetDebugCounter(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * NtDCompositionSuspendAnimations
 *
 * win32k 10.0.26100.9444 dispatch (0x1400249ae-0x1400249d5) preserves and
 * forwards two 32-bit arguments. Semantic names and signedness remain unrecovered.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSuspendAnimations(
    ULONG Param1,
    ULONG Param2
    );

// rev
/**
 * The NtDCompositionSyncWait routine waits for synchronization within DirectComposition.
 *
 * \param Timeout The timeout duration for synchronization.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSyncWait(
    _In_ LONG Timeout
    );

// rev
/**
 * The NtDCompositionSynchronize routine synchronizes operations on a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param SyncInfo A pointer to synchronization parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionSynchronize(
    _In_ ULONG Channel,
    _In_ PVOID SyncInfo
    );

// rev
/**
 * The NtDCompositionTelemetrySetApplicationId routine sets an application identifier for DirectComposition telemetry.
 *
 * \param Flags Telemetry configuration flags.
 * \param Size The size of the application identifier.
 * \param ApplicationId A pointer to the application identifier buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionTelemetrySetApplicationId(
    _In_ ULONG Flags,
    _In_ ULONG_PTR Size,
    _In_ PVOID ApplicationId
    );

// rev
/**
 * The NtDCompositionUpdatePointerCapture routine updates pointer capture state for a DirectComposition channel.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param Flags Pointer capture update flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionUpdatePointerCapture(
    _In_ ULONG Channel,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtDCompositionWaitForChannel routine waits for pending work on a DirectComposition channel to complete.
 *
 * \param Channel The DirectComposition channel identifier.
 * \param Timeout The timeout duration for waiting.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionWaitForChannel(
    _In_ ULONG Channel,
    _In_ LONG Timeout
    );

// rev
/**
 * The NtDCompositionWaitForCompositorClock routine waits for compositor clock signals.
 *
 * \param Count The number of compositor clock handles.
 * \param Handles A pointer to an array of clock handles.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDCompositionWaitForCompositorClock(
    _In_ ULONG Count,
    _In_ PVOID Handles
    );

// rev
/**
 * The NtDuplicateCompositionInputSink routine duplicates a composition input sink handle.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateCompositionInputSink(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2
    );

// rev
/**
 * The NtNotifyPresentToCompositionSurface routine notifies the compositor of a present operation to a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param PresentInfo A pointer to present notification data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyPresentToCompositionSurface(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID PresentInfo
    );

// rev
/**
 * The NtOpenCompositionSurfaceDirtyRegion routine opens the dirty region for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param InputInfo A pointer to input parameter data.
 * \param Param3 Third input parameter.
 * \param DirtyRegion A pointer receiving dirty region information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenCompositionSurfaceDirtyRegion(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID InputInfo,
    _In_ PVOID Param3,
    _Out_ PVOID DirtyRegion
    );

// rev
/**
 * The NtOpenCompositionSurfaceRealizationInfo routine opens realization information for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param InputInfo A pointer to input parameter data.
 * \param Param3 Third input parameter.
 * \param RealizationInfo A pointer receiving realization information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenCompositionSurfaceRealizationInfo(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID InputInfo,
    _In_ PVOID Param3,
    _Out_ PVOID RealizationInfo
    );

// rev
/**
 * The NtOpenCompositionSurfaceSectionInfo routine opens section information for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param InputInfo A pointer to input parameter data.
 * \param Param3 Third input parameter.
 * \param SectionInfo A pointer receiving surface section information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenCompositionSurfaceSectionInfo(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID InputInfo,
    _In_ PVOID Param3,
    _Out_ PVOID SectionInfo
    );

// rev
/**
 * The NtQueryCompositionInputIsImplicit routine queries whether a composition input sink is implicit.
 *
 * \param InputSink A pointer to the input sink.
 * \param IsImplicit A pointer receiving the boolean implicit flag.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionInputIsImplicit(
    _In_ PVOID InputSink,
    _Out_ PVOID IsImplicit
    );

// rev
/**
 * The NtQueryCompositionInputQueueAndTransform routine queries the input queue and transform associated with a composition sink.
 *
 * \param InputSink A handle to the input sink.
 * \param Param2 Second parameter.
 * \param InputQueue A pointer receiving the input queue.
 * \param Transform A pointer receiving the transform.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionInputQueueAndTransform(
    _In_ HANDLE InputSink,
    _In_ LONG Param2,
    _Out_ PVOID InputQueue,
    _Out_ PVOID Transform
    );

// rev
/**
 * The NtQueryCompositionInputSink routine queries information about a composition input sink.
 *
 * \param InputSink A pointer to the input sink.
 * \param InputSinkInfo A pointer receiving input sink information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionInputSink(
    _In_ PVOID InputSink,
    _Out_ PVOID InputSinkInfo
    );

// rev
/**
 * The NtQueryCompositionInputSinkLuid routine queries the locally unique identifier (LUID) of a composition input sink.
 *
 * \param InputSink A pointer to the input sink.
 * \param Luid A pointer receiving the LUID.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionInputSinkLuid(
    _In_ PVOID InputSink,
    _Out_ PVOID Luid
    );

// rev
/**
 * The NtQueryCompositionInputSinkViewId routine queries the view identifier of a composition input sink.
 *
 * \param InputSink A pointer to the input sink.
 * \param ViewId A pointer receiving the view identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionInputSinkViewId(
    _In_ PVOID InputSink,
    _Out_ PVOID ViewId
    );

// rev
/**
 * The NtQueryCompositionSurfaceBinding routine queries binding information for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param BindingInfo A pointer to binding query parameters.
 * \param Binding A pointer receiving binding details.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionSurfaceBinding(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID BindingInfo,
    _Out_ PVOID Binding
    );

// rev
/**
 * The NtQueryCompositionSurfaceFrameRate routine queries the frame rate configuration of a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param FrameRate A pointer receiving frame rate information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionSurfaceFrameRate(
    _In_ HANDLE CompositionSurface,
    _Out_ PVOID FrameRate
    );

// rev
/**
 * The NtQueryCompositionSurfaceHDRMetaData routine queries HDR metadata for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param InputInfo A pointer to input parameter data.
 * \param Param3 Third input parameter.
 * \param Size The size of the metadata buffer in bytes.
 * \param HDRMetaData A pointer receiving HDR metadata.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionSurfaceHDRMetaData(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID InputInfo,
    _In_ PVOID Param3,
    _In_ ULONG64 Size,
    _Out_ PVOID HDRMetaData
    );

// rev
/**
 * The NtQueryCompositionSurfaceRenderingRealization routine queries rendering realization parameters for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param RenderingRealization A pointer receiving rendering realization details.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionSurfaceRenderingRealization(
    _In_ HANDLE CompositionSurface,
    _Out_ PVOID RenderingRealization
    );

// rev
/**
 * The NtQueryCompositionSurfaceStatistics routine queries statistics for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param Statistics A pointer receiving surface statistics.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryCompositionSurfaceStatistics(
    _In_ HANDLE CompositionSurface,
    _Out_ PVOID Statistics
    );

// rev
/**
 * The NtSetCompositionSurfaceAnalogExclusive routine configures analog exclusive mode on a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param Enable Specifies whether analog exclusive mode is enabled.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCompositionSurfaceAnalogExclusive(
    _In_ HANDLE CompositionSurface,
    _In_ LONG Enable
    );

// rev
/**
 * The NtSetCompositionSurfaceBufferUsage routine sets the buffer usage pattern on a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param BufferInfo A pointer to buffer information.
 * \param Count The number of buffer elements.
 * \param Param4 Fourth configuration parameter.
 * \param Param5 Fifth configuration parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCompositionSurfaceBufferUsage(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID BufferInfo,
    _In_ ULONG Count,
    _In_ LONG Param4,
    _In_ LONG Param5
    );

// rev
/**
 * The NtSetCompositionSurfaceDirectFlipState routine sets the DirectFlip state on a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param FlipStateInfo A pointer to flip state information.
 * \param Param3 Third configuration parameter.
 * \param Param4 Fourth configuration parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCompositionSurfaceDirectFlipState(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID FlipStateInfo,
    _In_ LONG Param3,
    _In_ LONG Param4
    );

// rev
/**
 * The NtSetCompositionSurfaceIndependentFlipInfo routine sets independent flip parameters on a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param FlipInfo A pointer to independent flip info.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \param Param6 Sixth parameter.
 * \param Param7 Seventh parameter.
 * \param Param8 Eighth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCompositionSurfaceIndependentFlipInfo(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID FlipInfo,
    _In_ LONG Param3,
    _In_ LONG Param4,
    _In_ ULONG Param5,
    _In_ ULONG Param6,
    _In_ PVOID Param7,
    _In_ PVOID Param8
    );

// rev
/**
 * The NtSetCompositionSurfaceStatistics routine updates or resets statistics for a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface.
 * \param StatisticsInfo A pointer to statistics input parameters.
 * \param Statistics A pointer receiving updated statistics.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCompositionSurfaceStatistics(
    _In_ HANDLE CompositionSurface,
    _In_ PVOID StatisticsInfo,
    _Out_ PVOID Statistics
    );

// rev
/**
 * The NtUnBindCompositionSurface routine unbinds a composition surface.
 *
 * \param CompositionSurface A handle to the composition surface to unbind.
 * \param Flags Unbinding flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnBindCompositionSurface(
    _In_ HANDLE CompositionSurface,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserCompositionInputSinkLuidFromPoint routine retrieves the composition input sink LUID corresponding to a screen point.
 *
 * \param Param1 Input flags or coordinate specifier.
 * \param Point Pointer to a POINT structure containing the coordinates to hit-test.
 * \param InputSinkLuid Receives the 8-byte identifier of the input sink at the point.
 * \param Param4 Optionally receives an additional 8-byte hit-testing result.
 * \param SinkInfo Optionally receives a 64-byte input sink information block.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCompositionInputSinkLuidFromPoint(
    _In_ LONG Param1,
    _In_ PPOINT Point,
    _Out_ PLUID InputSinkLuid,
    _Out_opt_ PVOID Param4,
    _Out_writes_bytes_opt_(64) PVOID SinkInfo
    );

// rev
/**
 * The NtUserCompositionInputSinkViewInstanceIdFromPoint routine retrieves the view instance identifier of a composition input sink at the specified screen point.
 *
 * \param Point Pointer to a POINT structure indicating the screen coordinates.
 * \param ViewInstanceId Pointer to a variable receiving the view instance identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCompositionInputSinkViewInstanceIdFromPoint(
    _In_ PVOID Point,
    _Out_ PVOID ViewInstanceId
    );

// rev
/**
 * The NtUserCreateDCompositionHwndTarget routine creates a DirectComposition target object for a window.
 *
 * \param WindowHandle Handle to the window to be bound to the DirectComposition target.
 * \param TargetType Type or flags of the composition target.
 * \param TargetHandle Pointer to a variable receiving the created DirectComposition target handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ ULONG TargetType,
    _Out_ PHANDLE TargetHandle
    );

// rev
/**
 * The NtUserDestroyDCompositionHwndTarget routine destroys a DirectComposition target associated with a window.
 *
 * \param WindowHandle Handle to the window bound to the DirectComposition target.
 * \param TargetType Type or index of the composition target to destroy.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ ULONG TargetType
    );

// rev
/**
 * The NtUserGetDCompositionHwndBitmap routine retrieves the shared DirectComposition surface bitmap for a window.
 *
 * \param WindowHandle Handle to the window bound to DirectComposition.
 * \param BitmapHandle Pointer to a variable receiving the GDI/DComposition bitmap handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetDCompositionHwndBitmap(
    _In_ HWND WindowHandle,
    _Out_ PHANDLE BitmapHandle
    );

// rev
/**
 * The NtUserGetResizeDCompositionSynchronizationObject routine retrieves the DirectComposition synchronization object used during window resize.
 *
 * \param WindowHandle Handle to the target window.
 * \param SynchronizationObject Pointer to a variable receiving the synchronization object handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetResizeDCompositionSynchronizationObject(
    _In_ HWND WindowHandle,
    _Out_ PHANDLE SynchronizationObject
    );

// rev
/**
 * NtUserGetWindowCompositionAttribute
 *
 * win32kfull 10.0.26100.9444: 0x1401416f8 copies a 24-byte descriptor from argument 2.
 * The descriptor contains the attribute (ULONG at +0), output buffer pointer (+8),
 * and buffer size (SIZE_T at +16) on x64. AttributeData is the descriptor, not the payload.
 * Returns a Boolean success value, not NTSTATUS (0x1401418cc-0x1401418d2).
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PVOID AttributeData
    );

// rev
/**
 * The NtUserGetWindowCompositionInfo routine retrieves Desktop Window Manager (DWM) composition attributes for a window.
 *
 * \param Param1 Unconfirmed window handle parameter.
 * \param Param2 Pointer to a structure receiving window composition information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetWindowCompositionInfo(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtUserSetWindowCompositionAttribute routine sets Desktop Window Manager (DWM) composition attributes for a window.
 *
 * \param WindowHandle A handle to the window to configure.
 * \param AttributeData A pointer to a WINDOWCOMPOSITIONATTRIBDATA structure containing the attribute to set.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PVOID AttributeData
    );

// rev
/**
 * The NtUserSetWindowCompositionTransition routine configures DWM visual transition effects and animations for window state changes.
 *
 * \param WindowHandle A handle to the window undergoing the transition.
 * \param Param2 Transition type or animation identifier.
 * \param Param3 Transition timing or easing parameters.
 * \param Param4 A pointer to transition parameters or transformation data.
 * \param Param5 Transition control parameter.
 * \param Param6 Transition control parameter.
 * \param Param7 Additional transition flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowCompositionTransition(
    _In_ HWND WindowHandle,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _In_ PVOID Param4,
    _In_ ULONG_PTR Param5,
    _In_ ULONG_PTR Param6,
    _In_ ULONG_PTR Param7
    );

// rev
/**
 * The NtValidateCompositionSurfaceHandle routine validates a composition surface handle.
 *
 * \param CompositionSurface A handle to the composition surface to validate.
 * \param ValidationInfo A pointer receiving validation details.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtValidateCompositionSurfaceHandle(
    _In_ HANDLE CompositionSurface,
    _Out_ PVOID ValidationInfo
    );

// rev
/**
 * The SetWindowCompositionAttribute routine sets a DWM window composition attribute.
 *
 * \param WindowHandle Handle to the target window.
 * \param AttributeData Pointer to the composition attribute data buffer.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Forwards to the NtUserSetWindowCompositionAttribute system call.
 */
NTSYSAPI
BOOL
NTAPI
SetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PVOID AttributeData
    );

// rev
/**
 * The SetWindowCompositionTransition routine configures a window composition transition.
 *
 * \param WindowHandle Handle to the target window.
 * \param TransitionFlags Flags controlling the transition behavior.
 * \param CompositionStartData Optional pointer to starting composition parameters.
 * \param CompositionEndData Optional pointer to ending composition parameters.
 * \param TransitionData1 Optional pointer to additional transition data.
 * \param TransitionData2 Optional pointer to additional transition data.
 * \param TransitionData3 Optional pointer to additional transition data.
 * \return NTSTATUS Successful or errant status.
 * \remarks Forwards to the NtUserSetWindowCompositionTransition system call.
 */
NTSYSAPI
NTSTATUS
NTAPI
SetWindowCompositionTransition(
    _In_ HWND WindowHandle,
    _In_ ULONG TransitionFlags,
    _In_opt_ PVOID CompositionStartData,
    _In_opt_ PVOID CompositionEndData,
    _In_opt_ PVOID TransitionData1,
    _In_opt_ PVOID TransitionData2,
    _In_opt_ PVOID TransitionData3
    );

//
// FlipObject Presentation Services
//

// rev
/**
 * The NtFlipObjectAddContent routine adds content to a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param ContentInfo A pointer to content parameters.
 * \param Flags Operation flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectAddContent(
    _In_ HANDLE FlipObject,
    _In_ PVOID ContentInfo,
    _In_ ULONG Flags
    );

// rev
/**
 * NtFlipObjectAddPoolBuffer
 *
 * dxgkrnl 10.0.26100.9444: six arguments. Argument 5 is a property count;
 * argument 6 is the property-item pointer (0x140054734-0x140054743).
 * The property-item layout and the remaining opaque pointee types are not declared here.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectAddPoolBuffer(
    _In_ HANDLE FlipObject,
    _In_ PVOID BufferInfo,
    _In_ PVOID Param3,
    _In_ PVOID Param4,
    _In_ ULONG PropertyCount,
    _In_opt_ struct FlipPropertyItem *Properties
    );

// rev
/**
 * The NtFlipObjectConsumerAcquirePresent routine acquires a present frame by a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Flags Acquisition flags.
 * \param Timeout Timeout duration in milliseconds.
 * \param PresentInfo A pointer receiving present frame information.
 * \param Param5 Fifth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerAcquirePresent(
    _In_ HANDLE FlipObject,
    _In_ ULONG Flags,
    _In_ ULONG Timeout,
    _Out_ PVOID PresentInfo,
    _In_ LONG_PTR Param5
    );

// rev
/**
 * The NtFlipObjectConsumerAdjustUsageReference routine adjusts the usage reference count for a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \param Adjustment Usage reference delta adjustment.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerAdjustUsageReference(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2,
    _In_ LONG Adjustment
    );

// rev
/**
 * The NtFlipObjectConsumerBeginProcessPresent routine initiates processing of a present frame by a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \param Param3 A pointer receiving present processing parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerBeginProcessPresent(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2,
    _Out_ PVOID Param3
    );

// rev
/**
 * The NtFlipObjectConsumerEndProcessPresent routine finalizes processing of a present frame by a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerEndProcessPresent(
    _In_ HANDLE FlipObject
    );

// rev
/**
 * The NtFlipObjectConsumerPostMessage routine posts a message from a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Message The message identifier to post.
 * \param MessageData A pointer to message payload data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerPostMessage(
    _In_ HANDLE FlipObject,
    _In_ ULONG Message,
    _In_ PVOID MessageData
    );

// rev
/**
 * The NtFlipObjectConsumerQueryBufferInfo routine queries buffer information for a flip object consumer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \param BufferInfo A pointer receiving buffer information.
 * \param Param4 Fourth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectConsumerQueryBufferInfo(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2,
    _Out_ PVOID BufferInfo,
    _In_ LONG_PTR Param4
    );

// rev
/**
 * The NtFlipObjectCreate routine creates a flip presentation object.
 *
 * \param CreateInfo Pointer or configuration data for flip object creation.
 * \param FlipObject A pointer receiving the created flip object handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectCreate(
    _In_ LONG_PTR CreateInfo,
    _Out_ PHANDLE FlipObject
    );

// rev
/**
 * The NtFlipObjectDisconnectEndpoint routine disconnects an endpoint from a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Endpoint parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectDisconnectEndpoint(
    _In_ HANDLE FlipObject,
    _In_ LONG Param2
    );

// rev
/**
 * The NtFlipObjectEnablePresentStatisticsType routine enables present statistics reporting for a flip object.
 *
 * \param FlipObject A handle to the flip object.
 * \param StatisticsType The present statistics type identifier.
 * \param Enable Specifies whether statistics reporting is enabled.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectEnablePresentStatisticsType(
    _In_ HANDLE FlipObject,
    _In_ ULONG StatisticsType,
    _In_ LONG Enable
    );

// rev
/**
 * The NtFlipObjectOpen routine opens an existing flip presentation object.
 *
 * \param OpenInfo A pointer to open parameters.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param FlipObject A pointer receiving the opened flip object handle or context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectOpen(
    _In_ PVOID OpenInfo,
    _In_ LONG Param2,
    _In_ PVOID Param3,
    _Out_ PVOID FlipObject
    );

// rev
/**
 * The NtFlipObjectPresentCancel routine cancels a present operation on a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectPresentCancel(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtFlipObjectQueryBufferAvailableEvent routine queries the buffer availability event for a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \param Event A pointer receiving the event object or handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectQueryBufferAvailableEvent(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2,
    _Out_ PVOID Event
    );

// rev
/**
 * The NtFlipObjectQueryEndpointConnected routine queries whether an endpoint is connected to a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Endpoint parameter.
 * \param Connected A pointer receiving connection status.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectQueryEndpointConnected(
    _In_ HANDLE FlipObject,
    _In_ LONG Param2,
    _Out_ PVOID Connected
    );

// rev
/**
 * The NtFlipObjectQueryLostEvent routine queries the lost event for a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Event A pointer receiving the lost event handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectQueryLostEvent(
    _In_ HANDLE FlipObject,
    _Out_ PVOID Event
    );

// rev
/**
 * The NtFlipObjectQueryNextMessageToProducer routine queries the next message directed to a flip object producer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Message A pointer receiving message information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectQueryNextMessageToProducer(
    _In_ HANDLE FlipObject,
    _Out_ PVOID Message
    );

// rev
/**
 * The NtFlipObjectReadNextMessageToProducer routine reads the next message directed to a flip object producer.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \param Message A pointer receiving message payload.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectReadNextMessageToProducer(
    _In_ HANDLE FlipObject,
    _In_ LONG Param2,
    _Out_ PVOID Message
    );

// rev
/**
 * The NtFlipObjectRemoveContent routine removes content from a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectRemoveContent(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtFlipObjectRemovePoolBuffer routine removes a pool buffer from a flip presentation object.
 *
 * \param FlipObject A handle to the flip object.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectRemovePoolBuffer(
    _In_ HANDLE FlipObject,
    _In_ PVOID Param2
    );

// rev
/**
 * NtFlipObjectSetContent
 *
 * dxgkrnl 10.0.26100.9444: five arguments. Argument 4 is a property count;
 * argument 5 is the property-item pointer (0x14001e020-0x14001e02b).
 * Param3 is optional (0x14001dfb4). The property-item layout remains opaque.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlipObjectSetContent(
    _In_ HANDLE FlipObject,
    _In_ PVOID ContentInfo,
    _In_opt_ PVOID Param3,
    _In_ ULONG PropertyCount,
    _In_opt_ struct FlipPropertyItem *Properties
    );

//
// Modern Input Transport (MIT)
//

// rev
/**
 * The NtMITAccessibilityTimerNotification routine processes an accessibility timer notification for Modern Input Transport (MIT).
 *
 * \param TimerId The accessibility timer identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITAccessibilityTimerNotification(
    _In_ ULONG TimerId
    );

// rev
/**
 * The NtMITActivateInputProcessing routine activates input processing in the Modern Input Transport subsystem.
 *
 * \param Context The input processing context.
 * \param Result A pointer receiving activation results.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITActivateInputProcessing(
    _In_ LONG_PTR Context,
    _Out_ PVOID Result
    );

// rev
/**
 * The NtMITConfigureVirtualTouchpad routine configures a virtual touchpad device.
 *
 * \param TouchpadId Receives the identifier of the configured virtual touchpad.
 * \param Configuration A pointer to the virtual touchpad configuration.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITConfigureVirtualTouchpad(
    _Out_ PVOID TouchpadId,
    _In_ PVOID Configuration
    );

// rev
/**
 * The NtMITCoreMsgKOpenConnectionTo routine opens a CoreMessaging kernel connection to a target process.
 *
 * \param ProcessId The target process identifier.
 * \param ConnectionInfo A pointer to connection parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITCoreMsgKOpenConnectionTo(
    _In_ ULONG ProcessId,
    _Inout_ PVOID ConnectionInfo
    );

// rev
/**
 * The NtMITDeactivateInputProcessing routine deactivates input processing in the Modern Input Transport subsystem.
 *
 * \param Param1 Input context parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITDeactivateInputProcessing(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtMITDisableMouseIntercept routine disables mouse input interception.
 *
 * \param Param1 Mouse interception parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITDisableMouseIntercept(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtMITDispatchCompletion routine dispatches an input completion notification.
 *
 * \param CompletionType The completion type identifier.
 * \param Param2 Completion parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITDispatchCompletion(
    _In_ LONG CompletionType,
    _In_ ULONG Param2
    );

// rev
/**
 * The NtMITEnableMouseIntercept routine enables mouse input interception.
 *
 * \param Enable Specifies whether to enable mouse input interception.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITEnableMouseIntercept(
    _In_ ULONG Enable
    );

// rev
/**
 * The NtMITGetCursorUpdateHandle routine retrieves an update handle for the system cursor.
 *
 * \return HANDLE Handle to cursor update object.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtMITGetCursorUpdateHandle(
    VOID
    );

// rev
/**
 * The NtMITInitMinuserThread routine initializes a minuser input thread.
 *
 * \param Handle A handle to the thread or synchronization object.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITInitMinuserThread(
    _In_ HANDLE Handle
    );

// rev
/**
 * The NtMITMinuserSetInputTransformOffset routine sets an input transform offset for a minuser context.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITMinuserSetInputTransformOffset(
    _In_ LONG_PTR Param1,
    _In_ PVOID Param2,
    _In_ PVOID Param3
    );

// rev
/**
 * The NtMITMinuserWindowDestroyed routine notifies the input subsystem of a destroyed minuser window.
 *
 * \param WindowHandle A handle to the destroyed window.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITMinuserWindowDestroyed(
    _In_ HWND WindowHandle
    );

// rev
/**
 * The NtMITPostMouseInputMessage routine posts a mouse input message to the input stream.
 *
 * \param MouseInput A pointer to mouse input data.
 * \param Flags Input flags.
 * \param Param3 Third parameter.
 * \param Result A pointer receiving operation results.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITPostMouseInputMessage(
    _In_ PVOID MouseInput,
    _In_ ULONG Flags,
    _In_ LONG_PTR Param3,
    _Out_ PVOID Result
    );

// rev
/**
 * The NtMITPostThreadEventMessage routine posts an event message to a thread input queue.
 *
 * \param ThreadId The target thread identifier.
 * \param EventMessage A pointer to event message data.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITPostThreadEventMessage(
    _In_ ULONG ThreadId,
    _In_ PVOID EventMessage,
    _In_ LONG Param3,
    _In_ LONG Param4,
    _In_ ULONG Param5
    );

// rev
/**
 * The NtMITPostWindowEventMessage routine posts an event message to a window input queue.
 *
 * \param WindowHandle A handle to the target window.
 * \param EventMessage A pointer to event message data.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITPostWindowEventMessage(
    _In_ HWND WindowHandle,
    _In_ PVOID EventMessage,
    _In_ LONG Param3,
    _In_ LONG Param4,
    _In_ ULONG Param5
    );

// rev
/**
 * The NtMITPrepareReceiveInputMessage routine prepares the caller to receive an input message.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \param Param6 Sixth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITPrepareReceiveInputMessage(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _In_ ULONG Param3,
    _In_ LONG_PTR Param4,
    _In_ LONG_PTR Param5,
    _In_ LONG_PTR Param6
    );

// rev
/**
 * The NtMITPrepareSendInputMessage routine prepares an input message for transmission.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITPrepareSendInputMessage(
    _In_ LONG_PTR Param1,
    _In_ ULONG Param2,
    _In_ LONG_PTR Param3
    );

// rev
/**
 * The NtMITProcessDelegateCapturedPointers routine processes delegated captured pointers.
 *
 * \param Param1 Delegation parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITProcessDelegateCapturedPointers(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtMITSetInputCallbacks routine registers callback routines for input processing.
 *
 * \param Callbacks A pointer to input callback structures.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSetInputCallbacks(
    _In_ PVOID Callbacks
    );

// rev
/**
 * The NtMITSetInputDelegationMode routine configures the input delegation mode.
 *
 * \param Mode The input delegation mode.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSetInputDelegationMode(
    _In_ ULONG Mode,
    _In_ ULONG Param2,
    _In_ ULONG Param3,
    _In_ ULONG Param4
    );

// rev
/**
 * The NtMITSetInputObservationState routine configures the input observation state.
 *
 * \param Param1 First parameter.
 * \param State The input observation state.
 * \param Param3 Third parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITSetInputObservationState(
    _In_ LONG_PTR Param1,
    _In_ ULONG State,
    _In_ ULONG Param3
    );

// rev
/**
 * The NtMITSetKeyboardInputRoutingPolicy routine sets the routing policy for keyboard input.
 *
 * \param Policy The keyboard routing policy identifier.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMITSetKeyboardInputRoutingPolicy(
    _In_ LONG_PTR Policy
    );

// rev
/**
 * The NtMITSetKeyboardOverriderState routine sets the state of a keyboard overrider.
 *
 * \param State The keyboard overrider state.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtMITSetKeyboardOverriderState(
    _In_ LONG_PTR State
    );

// rev
/**
 * The NtMITSetLastInputRecipient routine sets the last input recipient for Modern Input Transport (MIT).
 *
 * \param Recipient The recipient identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSetLastInputRecipient(
    _In_ ULONG Recipient
    );

// rev
/**
 * The NtMITSynthesizeKeyboardInput routine synthesizes keyboard input events via Modern Input Transport (MIT).
 *
 * \param Count The number of keyboard events.
 * \param KeyboardInput A pointer to the keyboard input array.
 * \param KeyStateArray Optional 256-byte key state array updated with the synthesized keys.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSynthesizeKeyboardInput(
    _In_ ULONG Count,
    _In_ PVOID KeyboardInput,
    _Inout_updates_bytes_opt_(256) PVOID KeyStateArray
    );

// rev
/**
 * The NtMITSynthesizeMouseInput routine synthesizes mouse input events via Modern Input Transport (MIT).
 *
 * \param MouseInput A pointer to mouse input data.
 * \param Count The number of mouse events.
 * \param ExtraInfo Optional extra information associated with the synthesized input.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSynthesizeMouseInput(
    _In_ PVOID MouseInput,
    _In_ ULONG Count,
    _In_opt_ PVOID ExtraInfo
    );

// rev
/**
 * The NtMITSynthesizeTouchInput routine synthesizes touch input events via Modern Input Transport (MIT).
 *
 * \param TouchInput A pointer to touch input data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITSynthesizeTouchInput(
    _In_ PVOID TouchInput
    );

// rev
/**
 * The NtMITUninitMinuserThread routine uninitializes a minuser thread context.
 *
 * \param Param1 Thread parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITUninitMinuserThread(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtMITUpdateInputGlobals routine updates global input parameters in Modern Input Transport.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMITUpdateInputGlobals(
    _In_ ULONG Param1,
    _In_ ULONG Param2,
    _In_ USHORT Param3,
    _In_ LONG Param4,
    _In_ LONG Param5
    );

// rev
/**
 * The NtMinGetInputTransform routine retrieves an input transformation matrix.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param InputTransform A pointer receiving input transform data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMinGetInputTransform(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _Out_ PVOID InputTransform
    );

// rev
/**
 * The NtMinInteropCoreMessagingWithInput routine configures CoreMessaging interop with input processing.
 *
 * \param Param1 Configuration parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMinInteropCoreMessagingWithInput(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtMinQPeekForInput routine peeks for pending input messages in the minimal input queue.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \param Param6 Sixth parameter.
 * \param Param7 Seventh parameter.
 * \param Param8 Eighth parameter.
 * \param Param9 Ninth parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMinQPeekForInput(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _In_ PVOID Param3,
    _In_ PVOID Param4,
    _In_ LONG Param5,
    _In_ LONG_PTR Param6,
    _In_ LONG_PTR Param7,
    _In_ LONG_PTR Param8,
    _In_ LONG_PTR Param9
    );

// rev
/**
 * The NtMinQSuspendInputProcessing routine suspends input processing for a minimal input queue.
 *
 * \param Param1 Suspension parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMinQSuspendInputProcessing(
    _In_ PVOID Param1
    );

// rev
/**
 * The NtMinQUpdateWakeMask routine updates the wake mask for a minimal input queue.
 *
 * \param WakeMask The wake mask bitmask.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMinQUpdateWakeMask(
    _In_ ULONG WakeMask
    );

// rev
/**
 * The NtModerncoreBeginLayoutUpdate routine begins a layout update for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param LayoutUpdateInfo A pointer to layout update information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreBeginLayoutUpdate(
    _In_ HWND WindowHandle,
    _In_ PVOID LayoutUpdateInfo
    );

// rev
/**
 * The NtModerncoreCreateDCompositionHwndTarget routine creates a DirectComposition HWND target for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param TargetHandle A pointer receiving the created target handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreCreateDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _In_ PVOID Param3,
    _Out_ PHANDLE TargetHandle
    );

// rev
/**
 * The NtModerncoreCreateGDIHwndTarget routine creates a GDI HWND target for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param TargetHandle A pointer receiving the created target handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreCreateGDIHwndTarget(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _Out_ PHANDLE TargetHandle
    );

// rev
/**
 * The NtModerncoreDestroyDCompositionHwndTarget routine destroys a DirectComposition HWND target for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreDestroyDCompositionHwndTarget(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _In_ PVOID Param3
    );

// rev
/**
 * The NtModerncoreDestroyGDIHwndTarget routine destroys a GDI HWND target for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param TargetHandle A handle or pointer to the GDI target.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreDestroyGDIHwndTarget(
    _In_ HWND WindowHandle,
    _In_ PVOID TargetHandle
    );

// rev
/**
 * The NtModerncoreEnableResizeLayoutSynchronization routine enables or disables resize layout synchronization.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreEnableResizeLayoutSynchronization(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _In_ PVOID Param3
    );

// rev
/**
 * The NtModerncoreGetNavigationWindowVisual routine retrieves the navigation window visual for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param Visual A pointer receiving the visual.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreGetNavigationWindowVisual(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _Out_ PVOID Visual
    );

// rev
/**
 * The NtModerncoreGetResizeDCompositionSynchronizationObject routine retrieves the DirectComposition resize synchronization object.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param SynchronizationObject A pointer receiving the synchronization object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreGetResizeDCompositionSynchronizationObject(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _Out_ PVOID SynchronizationObject
    );

// rev
/**
 * The NtModerncoreGetWindowContentVisual routine retrieves the window content visual for a moderncore window.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Visual A pointer receiving the window content visual.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreGetWindowContentVisual(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2,
    _In_ PVOID Param3,
    _Out_ PVOID Visual
    );

// rev
/**
 * The NtModerncoreIdleTimerThread routine executes the idle timer thread loop for moderncore.
 *
 * \param Param1 Thread parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from the win32k.sys handler, and the win32u.dll stub carries none.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreIdleTimerThread(
    _In_ ULONG_PTR Param1
    );

// rev
/**
 * The NtModerncoreIsResizeLayoutSynchronizationEnabled routine queries whether resize layout synchronization is enabled.
 *
 * \param WindowHandle A handle to the target window.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreIsResizeLayoutSynchronizationEnabled(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtModerncoreProcessConnect routine connects a process to moderncore windowing services.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreProcessConnect(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2
    );

// rev
/**
 * The NtModerncoreRegisterEnhancedNavigationWindowHandle routine registers an enhanced navigation window handle.
 *
 * \param WindowHandle A handle to the window.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreRegisterEnhancedNavigationWindowHandle(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtModerncoreRegisterNavigationWindowHandle routine registers a navigation window handle.
 *
 * \param WindowHandle A handle to the window.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreRegisterNavigationWindowHandle(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtModerncoreSetNavigationServiceSid routine sets the SID for the navigation service.
 *
 * \param Sid A pointer to the SID.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreSetNavigationServiceSid(
    _In_ PSID Sid
    );

// rev
/**
 * The NtModerncoreUnregisterNavigationWindowHandle routine unregisters a navigation window handle.
 *
 * \param WindowHandle A handle to the window.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModerncoreUnregisterNavigationWindowHandle(
    _In_ HWND WindowHandle,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtTokenManagerConfirmOutstandingAnalogToken routine confirms an outstanding analog composition token.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerConfirmOutstandingAnalogToken(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtTokenManagerCreateCompositionTokenHandle routine creates a composition token handle.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Pointer to an 8-byte value read from user memory.
 * \param TokenHandle Pointer to a variable that receives the created token handle (written when non-NULL).
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks
 *          User-memory read/write shapes confirmed from dxgkrnl.sys (8-byte probe read of Param5,
 *          8-byte copy-to-user of TokenHandle). win32u.dll ordinal 772, syscall 0x139D
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerCreateCompositionTokenHandle(
    _In_ PVOID Param1,
    _In_ ULONG Param2,
    _In_ ULONG Param3,
    _In_ PVOID Param4,
    _In_ PULONG_PTR Param5,
    _Out_ PHANDLE TokenHandle
    );

// rev
/**
 * The NtTokenManagerCreateFlipObjectReturnTokenHandle routine creates a return token handle for a flip object.
 *
 * \param Param1 First parameter.
 * \param FlipObjectInfo A pointer to flip object information.
 * \param TokenHandle A pointer receiving the created token handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerCreateFlipObjectReturnTokenHandle(
    _In_ PVOID Param1,
    _In_ PVOID FlipObjectInfo,
    _Out_ PVOID TokenHandle
    );

// rev
/**
 * The NtTokenManagerCreateFlipObjectTokenHandle routine creates a token handle for a flip object.
 *
 * \param Param1 First parameter.
 * \param Param2 Optional pointer to an 8-byte value read from user memory (used with Param7).
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \param FlipProperties A pointer to flip properties.
 * \param Param7 Seventh parameter.
 * \param Param8 Eighth parameter.
 * \param TokenHandle Pointer to a 16-byte buffer that receives the created token handles
 * (written with an 8-byte copy-to-user when non-NULL).
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks
 *          User-memory shapes confirmed from dxgkrnl.sys (8-byte probe read of Param2, 16-byte
 *          copy-to-user of TokenHandle, FlipPropertyItem at Param6). win32u.dll ordinal 774, syscall 0x139F
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerCreateFlipObjectTokenHandle(
    _In_ PVOID Param1,
    _In_opt_ PULONG_PTR Param2,
    _In_ PVOID Param3,
    _In_ LONG Param4,
    _In_ ULONG Param5,
    _In_ PVOID FlipProperties,
    _In_opt_ PVOID Param7,
    _In_opt_ PVOID Param8,
    _Out_writes_bytes_(2 * sizeof(PVOID)) PVOID TokenHandle
    );

// rev
/**
 * The NtTokenManagerGetAnalogExclusiveSurfaceUpdates routine retrieves analog exclusive surface updates.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param SurfaceUpdates A pointer receiving surface update details.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerGetAnalogExclusiveSurfaceUpdates(
    _In_ ULONG Param1,
    _In_ LONG_PTR Param2,
    _In_ ULONG Param3,
    _In_ PVOID Param4,
    _Out_ PVOID SurfaceUpdates
    );

// rev
/**
 * The NtTokenManagerGetAnalogExclusiveTokenEvent routine retrieves the event associated with an analog exclusive token.
 *
 * \param Param1 Token parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerGetAnalogExclusiveTokenEvent(
    _In_ PVOID Param1
    );

// rev
/**
 * The NtTokenManagerOpenSectionAndEvents routine opens shared sections and events for token management.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param SectionAndEvents A pointer receiving section and event descriptors.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerOpenSectionAndEvents(
    _In_ PVOID Param1,
    _In_ PVOID Param2,
    _In_ PVOID Param3,
    _Out_ PVOID SectionAndEvents
    );

// rev
/**
 * The NtTokenManagerThread routine executes the token manager worker thread routine.
 *
 * \param Param1 Thread parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTokenManagerThread(
    _In_ PVOID Param1
    );

//
// DirectX Graphics Kernel (D3DKMT / Dxgk)
//

// rev
/**
 * The NtDxgkCancelPresents routine cancels pending present operations in the DirectX graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkCancelPresents(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkCheckSinglePlaneForMultiPlaneOverlaySupport routine checks whether a single plane supports multi-plane overlay presentation.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkCheckSinglePlaneForMultiPlaneOverlaySupport(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkConnectDoorbell routine connects a hardware doorbell for GPU work submission.
 *
 * \param Data A pointer to doorbell connection parameters.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkConnectDoorbell(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
/**
 * The NtDxgkCreateDoorbell routine creates a hardware doorbell for GPU work submission.
 *
 * \param Data A pointer to doorbell creation parameters.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkCreateDoorbell(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
/**
 * The NtDxgkCreateNativeFence routine creates a native GPU fence object.
 *
 * \param NativeFenceArguments Pointer to a 0x00D8-byte argument structure that the kernel reads
 * from user memory and writes results back into (at +4 and +72).
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks
 *          Single in/out argument buffer confirmed from dxgkrnl.sys (NtDxgkCreateNativeFenceInternal:
 *          RtlCopyVolatileMemory read of 0xD8 bytes; write-back at offsets +4 and +72).
 *          win32u.dll ordinal 74, syscall 0x1149
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkCreateNativeFence(
    _Inout_ PVOID NativeFenceArguments
    );

// rev
/**
 * The NtDxgkCreateTrackedWorkload routine creates a tracked GPU workload context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkCreateTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDDisplayEnum2 routine enumerates direct display devices.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDDisplayEnum2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDestroyDoorbell routine destroys a hardware doorbell object.
 *
 * \param Data A pointer to doorbell destruction parameters.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDestroyDoorbell(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
/**
 * The NtDxgkDestroyTrackedWorkload routine destroys a tracked GPU workload context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDestroyTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDisableProcessDebugBlobCollection routine disables collection of debug blobs for a process.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDisableProcessDebugBlobCollection(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtDxgkDispMgrOperation routine performs display manager operations in the DirectX graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDispMgrOperation(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDisplayMuxSwitchExecute routine executes a display multiplexer switch.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDisplayMuxSwitchExecute(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDisplayMuxSwitchFinish routine finalizes a display multiplexer switch.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDisplayMuxSwitchFinish(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDisplayMuxSwitchPrepare routine prepares for a display multiplexer switch.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDisplayMuxSwitchPrepare(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDisplayPortOperation routine performs DisplayPort operations in the DirectX graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDisplayPortOperation(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkDuplicateHandle routine duplicates a graphics kernel object handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkDuplicateHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * Enables debug-blob collection for the current graphics process.
 * \return STATUS_SUCCESS on success, STATUS_NOT_SUPPORTED when the feature is disabled,
 * or STATUS_ACCESS_DENIED when the current process has no DXGPROCESS.
 * \remarks Takes no arguments. The process is obtained internally through DXGPROCESS::GetCurrent.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkEnableProcessDebugBlobCollection(
    VOID
    );

// rev
struct _D3DKMT_ENUMADAPTERS3;

/**
 * Enumerates graphics adapters using the SDK D3DKMT_ENUMADAPTERS3 request.
 * \param Data Filter, adapter-array capacity and output array. Defined in d3dkmthk.h.
 * \return NTSTATUS. An insufficient adapter-array capacity returns STATUS_INFO_LENGTH_MISMATCH.
 * \remarks The inspected native x64 implementation captures a 24-byte request and returns
 * 20-byte D3DKMT_ADAPTERINFO entries. The SDK x86 request is 16 bytes; use the SDK
 * definition rather than hard-coding the native x64 pointer layout. A NULL pAdapters queries the maximum adapter count in the
 * session and returns STATUS_SUCCESS. Only filter bits 0x1 (IncludeComputeOnly),
 * 0x2 (IncludeDisplayOnly) and 0x4 (IncludeVirtualGpuOnly) are accepted; the SDK
 * IncludeTestOnly bit (0x8) is rejected with STATUS_INVALID_PARAMETER in this build.
 * On an insufficient-buffer failure the inspected implementation does not copy the
 * request back, so do not rely on NumAdapters being updated on that path.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkEnumAdapters3(
    _Inout_ struct _D3DKMT_ENUMADAPTERS3 *Data
    );

// rev
struct _D3DKMT_ENUMPROCESSES;

/**
 * Enumerates 32-bit process identifiers associated with the specified graphics adapter.
 * \param Data Private in/out enumeration request; layout remains opaque in this header.
 * \return STATUS_SUCCESS, STATUS_INFO_LENGTH_MISMATCH for a NULL or insufficient output
 * array, or STATUS_INVALID_PARAMETER for an invalid adapter or count.
 * \remarks The inspected native x64 request is 24 bytes: adapter LUID at offset 0,
 * process-ID array address at offset 8, and a 64-bit in/out element count at offset 16.
 * The input count must not exceed 0x3FFFFFFF. The count is updated on success and
 * STATUS_INFO_LENGTH_MISMATCH; array entries are 4-byte process IDs, not handles.
 * The private request tag is reconstructed; its WOW64 marshalling has not been established.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkEnumProcesses(
    _Inout_ struct _D3DKMT_ENUMPROCESSES *Data
    );

// rev
struct _D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX;

/**
 * Retrieves an available index from a tracked-workload object owned by the current graphics process.
 * \param Data Private in/out tracked-workload request; layout remains opaque in this header.
 * \return NTSTATUS. An undersized request or invalid workload handle returns STATUS_INVALID_PARAMETER.
 * \remarks The inspected x64 implementation requires the size at offset 0 to be at least
 * 0x218 (536) bytes and captures only 0x218 bytes. The 32-bit workload handle is at
 * offset 4, a 64-bit input value at offset 8, and an inline 64-bit-value array starts
 * at offset 16. The input value and array semantics remain unconfirmed.
 * On success, offset 0x210 receives a ULONG index and offset 0x214 receives a BOOL
 * indicating that active instance pairs were processed before retrying index allocation.
 * The private request tag is reconstructed; its WOW64 marshalling has not been established.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkGetAvailableTrackedWorkloadIndex(
    _Inout_ struct _D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX *Data
    );

// rev
/**
 * The NtDxgkGetNativeFenceLogDetail routine retrieves log details for native GPU fences.
 *
 * \param Data A pointer to native fence log query parameters.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkGetNativeFenceLogDetail(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
struct _D3DKMT_GETPROCESSLIST;

/**
 * Opens process handles for eligible processes associated with a graphics adapter.
 * \param Data Private in/out process-list request; layout remains opaque in this header.
 * \return NTSTATUS. A NULL or insufficient output array returns STATUS_INFO_LENGTH_MISMATCH.
 * \remarks The inspected native x64 request is 24 bytes: adapter LUID at offset 0,
 * ACCESS_MASK at offset 8, ULONG in/out element count at offset 12, and output-array
 * address at offset 16. DesiredAccess must equal PROCESS_QUERY_INFORMATION (0x400).
 * Each native x64 output entry is an 8-byte process handle, not a process ID. The caller
 * must close returned handles. The count is updated on success or STATUS_INFO_LENGTH_MISMATCH;
 * individual processes whose handles cannot be opened are skipped.
 * The private request tag is reconstructed; its WOW64 marshalling has not been established.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkGetProcessList(
    _Inout_ struct _D3DKMT_GETPROCESSLIST *Data
    );

// rev
/**
 * The NtDxgkGetProperties routine retrieves properties from the DirectX graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkGetProperties(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkGetTrackedWorkloadStatistics routine retrieves statistics for a tracked GPU workload.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkGetTrackedWorkloadStatistics(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkIsFeatureEnabled routine queries whether an optional graphics kernel feature is enabled.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkIsFeatureEnabled(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkMapProcessDebugBlob routine maps a process debug blob in the graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkMapProcessDebugBlob(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkNotifyWorkSubmission routine notifies the graphics kernel of work submission.
 *
 * \param Data A pointer to work submission notification data.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkNotifyWorkSubmission(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
/**
 * The NtDxgkOpenNativeFenceFromNtHandle routine opens a native GPU fence object from an NT handle.
 *
 * \param Data A pointer to native fence open parameters.
 * \param Flags Operation control flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkOpenNativeFenceFromNtHandle(
    _Inout_ PVOID Data,
    _In_ BOOLEAN Flags
    );

// rev
/**
 * The NtDxgkOutputDuplPresentToHwQueue routine presents an output duplication frame to a hardware queue.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkOutputDuplPresentToHwQueue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkPinResources routine pins graphics memory resources in physical memory.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkPinResources(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkRegisterVailProcess routine registers a VAIL process with the graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkRegisterVailProcess(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkResetTrackedWorkloadStatistics routine resets statistics for a tracked GPU workload.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkResetTrackedWorkloadStatistics(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkSetProperties routine sets properties in the DirectX graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkSetProperties(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkSubmitPresentBltToHwQueue routine submits a present bit block transfer (bitblt) operation to a hardware queue.
 *
 * \param Param1 First submission parameter.
 * \param Param2 Second submission parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: this export folds onto a shared/stub address in the binary, so neither the
// argument count nor the types below could be confirmed by disassembly.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkSubmitPresentBltToHwQueue(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtDxgkSubmitPresentToHwQueue routine submits a present operation to a hardware queue.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkSubmitPresentToHwQueue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkUnmapProcessDebugBlob routine unmaps a process debug blob in the graphics kernel.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkUnmapProcessDebugBlob(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkUnpinResources routine unpins graphics memory resources in physical memory.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkUnpinResources(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkUpdateTrackedWorkload routine updates a tracked GPU workload context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkUpdateTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkVailConnect routine connects a virtualized accelerated interface layer (VAIL) endpoint.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkVailConnect(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkVailDisconnect routine disconnects a VAIL endpoint.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkVailDisconnect(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtDxgkVailPromoteCompositionSurface routine promotes a composition surface through VAIL.
 *
 * \param CompositionSurface A pointer to the composition surface.
 * \param Param2 Second promotion parameter.
 * \param Param3 Third promotion parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDxgkVailPromoteCompositionSurface(
    _In_ PVOID CompositionSurface,
    _Inout_ PVOID Param2,
    _In_ LONG_PTR Param3
    );

// rev
/**
 * The NtGdiDdDDIAbandonSwapChain routine abandons a swap chain and releases associated presentation resources.
 *
 * \param Data A pointer to a swap chain abandonment parameters structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAbandonSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIAcquireKeyedMutex routine acquires a keyed mutex synchronization object.
 *
 * \param Data A pointer to a D3DKMT_ACQUIREKEYEDMUTEX structure that describes the keyed mutex to acquire.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAcquireKeyedMutex(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIAcquireKeyedMutex2 routine acquires a keyed mutex synchronization object with private data options.
 *
 * \param Data A pointer to a D3DKMT_ACQUIREKEYEDMUTEX2 structure that describes the keyed mutex to acquire.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAcquireKeyedMutex2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIAcquireSwapChain routine acquires ownership of a swap chain surface for presentation rendering.
 *
 * \param Data A pointer to a swap chain acquisition parameter structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAcquireSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIAddSurfaceToSwapChain routine adds a rendering surface to an existing swap chain.
 *
 * \param Data A pointer to a structure describing the surface to add to the swap chain.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAddSurfaceToSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIAdjustFullscreenGamma routine adjusts gamma ramp settings for a full-screen application.
 *
 * \param Data A pointer to a D3DKMT_ADJUSTFULLSCREENGAMMA structure that describes the gamma ramp to adjust.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIAdjustFullscreenGamma(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICacheHybridQueryValue routine caches hybrid graphics query values for cross-adapter GPU configurations.
 *
 * \param Data A pointer to a hybrid query value structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICacheHybridQueryValue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIChangeVideoMemoryReservation routine changes the video memory reservation for a process.
 *
 * \param Data A pointer to a D3DKMT_CHANGEVIDEOMEMORYRESERVATION structure that specifies the new video memory reservation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIChangeVideoMemoryReservation(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckExclusiveOwnership routine checks whether a device has exclusive ownership of a display adapter.
 *
 * \param Data Reserved or pointer to ownership query parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtGdiDdDDICheckExclusiveOwnership(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckMonitorPowerState routine checks the power state of a monitor.
 *
 * \param Data A pointer to a D3DKMT_CHECKMONITORPOWERSTATE structure that specifies the monitor whose power state to check.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckMonitorPowerState(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckMultiPlaneOverlaySupport routine checks hardware support for multiplane overlays.
 *
 * \param Data A pointer to a D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT structure that describes overlay configuration to check.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckMultiPlaneOverlaySupport(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckMultiPlaneOverlaySupport2 routine checks hardware support for multiplane overlays with extended parameters.
 *
 * \param Data A pointer to a D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT2 structure that describes overlay configuration to check.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckMultiPlaneOverlaySupport2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckMultiPlaneOverlaySupport3 routine checks hardware support for multiplane overlays with post-processing parameters.
 *
 * \param Data A pointer to a D3DKMT_CHECKMULTIPLANEOVERLAYSUPPORT3 structure that describes overlay configuration to check.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckMultiPlaneOverlaySupport3(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckOcclusion routine checks whether the client area of a window is occluded.
 *
 * \param Data A pointer to a D3DKMT_CHECKOCCLUSION structure that describes the window to check for occlusion.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckOcclusion(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckSharedResourceAccess routine validates access permissions for opening or modifying a shared resource.
 *
 * \param Data A pointer to a D3DKMT_CHECKSHAREDRESOURCEACCESS structure that specifies the shared resource to check.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckSharedResourceAccess(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICheckVidPnExclusiveOwnership routine determines whether a process has exclusive ownership of a video present network (VidPN) source.
 *
 * \param Data A pointer to a D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP structure that describes the VidPN source.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICheckVidPnExclusiveOwnership(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICloseAdapter routine closes an adapter that was opened by D3DKMTOpenAdapterFrom*.
 *
 * \param Data A pointer to a D3DKMT_CLOSEADAPTER structure that specifies the adapter to close.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICloseAdapter(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIConfigureSharedResource routine configures access and synchronization attributes of a shared resource.
 *
 * \param Data A pointer to a D3DKMT_CONFIGURESHAREDRESOURCE structure that specifies shared resource configuration.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIConfigureSharedResource(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateAllocation routine allocates video memory or system memory resources for graphics operations.
 *
 * \param Data A pointer to a D3DKMT_CREATEALLOCATION structure that describes parameters for creating allocations.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateAllocation(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateBundleObject routine creates a bundled graphics object grouping multiple allocations or states.
 *
 * \param Data A pointer to a bundle object creation parameters structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateBundleObject(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateContext routine creates a kernel-mode graphics device context.
 *
 * \param Data A pointer to a D3DKMT_CREATECONTEXT structure that describes parameters for creating the context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateContext(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateContextVirtual routine creates a context that uses virtual addressing.
 *
 * \param Data A pointer to a D3DKMT_CREATECONTEXTVIRTUAL structure that describes parameters for creating the virtual context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateContextVirtual(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateDCFromMemory routine creates a display context (DC) from a specified block of memory.
 *
 * \param Data A pointer to a D3DKMT_CREATEDCFROMMEMORY structure that describes parameters for creating the DC.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateDCFromMemory(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateDevice routine creates a kernel-mode graphics device context.
 *
 * \param Data A pointer to a D3DKMT_CREATEDEVICE structure that describes parameters for creating the device.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateDevice(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateHwContext routine creates a hardware context for hardware scheduling.
 *
 * \param Data A pointer to a D3DKMT_CREATEHWCONTEXT structure that describes parameters for creating the hardware context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateHwContext(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateHwQueue routine creates a hardware queue for hardware scheduling.
 *
 * \param Data A pointer to a D3DKMT_CREATEHWQUEUE structure that describes parameters for creating the hardware queue.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateHwQueue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateKeyedMutex routine creates a keyed mutex synchronization object.
 *
 * \param Data A pointer to a D3DKMT_CREATEKEYEDMUTEX structure that describes the keyed mutex to create.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateKeyedMutex(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateKeyedMutex2 routine creates a keyed mutex synchronization object with private data options.
 *
 * \param Data A pointer to a D3DKMT_CREATEKEYEDMUTEX2 structure that describes the keyed mutex to create.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateKeyedMutex2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateOutputDupl routine creates a desktop duplication instance for output replication.
 *
 * \param Data A pointer to a D3DKMT_CREATE_OUTPUTDUPL structure that describes parameters for creating the desktop duplication.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateOutputDupl(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateOverlay routine creates a kernel-mode overlay object.
 *
 * \param Data A pointer to a D3DKMT_CREATEOVERLAY structure that describes parameters for creating the overlay.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateOverlay(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreatePagingQueue routine creates a paging queue that can be used to synchronize video memory paging operations.
 *
 * \param Data A pointer to a D3DKMT_CREATEPAGINGQUEUE structure that describes parameters for creating the paging queue.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreatePagingQueue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateProtectedSession routine creates a protected session for DRM and content protection workflows.
 *
 * \param Data A pointer to a D3DKMT_CREATEPROTECTEDSESSION structure that describes parameters for creating the protected session.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateProtectedSession(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateSwapChain routine creates a swap chain presentation object.
 *
 * \param Data A pointer to a swap chain creation parameters structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDICreateSynchronizationObject routine creates a kernel-mode synchronization object for a graphics device.
 *
 * \param Data A pointer to a D3DKMT_CREATESYNCHRONIZATIONOBJECT structure that describes parameters for creating the synchronization object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDICreateSynchronizationObject(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDDisplayEnum routine enumerates graphics display devices and targets in the DirectX subsystem.
 *
 * \param Data A pointer to a display enumeration parameters structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDDisplayEnum(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyAllocation routine releases allocations of video memory, system memory, or a combination of both.
 *
 * \param Data A pointer to a D3DKMT_DESTROYALLOCATION structure that describes parameters for releasing allocations.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyAllocation(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyAllocation2 routine releases video or system memory allocations with extended options.
 *
 * \param Data A pointer to a D3DKMT_DESTROYALLOCATION2 structure that describes parameters for releasing allocations.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyAllocation2(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyContext routine destroys a kernel-mode graphics device context.
 *
 * \param Data A pointer to a D3DKMT_DESTROYCONTEXT structure that contains the handle to the context to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyContext(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyDCFromMemory routine destroys a display context (DC) that was created from memory.
 *
 * \param Data A pointer to a D3DKMT_DESTROYDCFROMMEMORY structure that describes the DC to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyDCFromMemory(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyDevice routine destroys a kernel-mode graphics device context.
 *
 * \param Data A pointer to a D3DKMT_DESTROYDEVICE structure that contains the handle to the device context to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyDevice(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyHwContext routine destroys a hardware context.
 *
 * \param Data A pointer to a D3DKMT_DESTROYHWCONTEXT structure that specifies the hardware context to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyHwContext(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyHwQueue routine destroys a hardware queue.
 *
 * \param Data A pointer to a D3DKMT_DESTROYHWQUEUE structure that specifies the hardware queue to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyHwQueue(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyKeyedMutex routine destroys a keyed mutex synchronization object.
 *
 * \param Data A pointer to a D3DKMT_DESTROYKEYEDMUTEX structure that specifies the keyed mutex to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyKeyedMutex(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyOutputDupl routine destroys a desktop duplication instance.
 *
 * \param Data A pointer to a D3DKMT_DESTROY_OUTPUTDUPL structure that specifies the desktop duplication instance to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyOutputDupl(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyOverlay routine destroys a kernel-mode overlay object.
 *
 * \param Data A pointer to a D3DKMT_DESTROYOVERLAY structure that specifies the overlay to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyOverlay(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyPagingQueue routine destroys a paging queue object.
 *
 * \param Data A pointer to a D3DDDI_DESTROYPAGINGQUEUE structure that specifies the paging queue to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyPagingQueue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroyProtectedSession routine destroys a protected content session.
 *
 * \param Data A pointer to a D3DKMT_DESTROYPROTECTEDSESSION structure that specifies the protected session to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroyProtectedSession(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDestroySynchronizationObject routine destroys a kernel-mode synchronization object for a graphics device.
 *
 * \param Data A pointer to a D3DKMT_DESTROYSYNCHRONIZATIONOBJECT structure that contains the handle to the synchronization object to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDestroySynchronizationObject(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDispMgrCreate routine creates a display manager instance for managing display topology and modes.
 *
 * \param Data A pointer to a display manager creation parameter structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDispMgrCreate(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDispMgrSourceOperation routine performs a display manager source operation on a video present source.
 *
 * \param Data A pointer to a display manager source operation structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDispMgrSourceOperation(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIDispMgrTargetOperation routine performs a display manager target operation on a video present target.
 *
 * \param Data A pointer to a display manager target operation structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIDispMgrTargetOperation(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIEnumAdapters routine enumerates all graphics adapters on the system.
 *
 * \param Data A pointer to a D3DKMT_ENUMADAPTERS structure that receives enumerated adapters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIEnumAdapters(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIEnumAdapters2 routine enumerates all graphics adapters on the system with extended adapter information.
 *
 * \param Data A pointer to a D3DKMT_ENUMADAPTERS2 structure that receives enumerated adapters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIEnumAdapters2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIEscape routine exchanges information with the display miniport driver via private escape codes.
 *
 * \param Data A pointer to a D3DKMT_ESCAPE structure that describes the shared information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIEscape(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIEvict routine evicts video memory allocations from resident device memory.
 *
 * \param Data A pointer to a D3DKMT_EVICT structure that describes allocations to evict.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIEvict(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIExtractBundleObject routine extracts an allocation or sub-object from a graphics bundle object.
 *
 * \param Data A pointer to a bundle object extraction parameters structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIExtractBundleObject(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIFlipOverlay routine changes the video memory allocation displayed on a hardware overlay.
 *
 * \param Data A pointer to a D3DKMT_FLIPOVERLAY structure that describes the overlay flip operation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIFlipOverlay(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIFlushHeapTransitions routine flushes pending video memory heap state transitions.
 *
 * \param Data A pointer to a D3DKMT_FLUSHHEAPTRANSITIONS structure that describes the transitions to flush.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIFlushHeapTransitions(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIFreeGpuVirtualAddress routine releases a range of GPU virtual addresses previously reserved or mapped.
 *
 * \param Data A pointer to a D3DKMT_FREEGPUVIRTUALADDRESS structure that describes the address range to release.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIFreeGpuVirtualAddress(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetAllocationPriority routine retrieves the eviction priority of a video memory resource or allocation.
 *
 * \param Data A pointer to a D3DKMT_GETALLOCATIONPRIORITY structure that specifies the resource to query.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetAllocationPriority(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetCachedHybridQueryValue routine retrieves cached hybrid GPU discrete and integrated query attributes.
 *
 * \param Data A pointer to a cached hybrid query value structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetCachedHybridQueryValue(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetContextInProcessSchedulingPriority routine retrieves the in-process scheduling priority for a device context.
 *
 * \param Data A pointer to a D3DKMT_GETCONTEXTINPROCESSSCHEDULINGPRIORITY structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetContextInProcessSchedulingPriority(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetContextSchedulingPriority routine retrieves the device context scheduling priority across processes.
 *
 * \param Data A pointer to a D3DKMT_GETCONTEXTSCHEDULINGPRIORITY structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetContextSchedulingPriority(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetDWMVerticalBlankEvent routine retrieves the vertical blank synchronization event for the Desktop Window Manager (DWM).
 *
 * \param Data A pointer to a D3DKMT_GETVERTICALBLANKEVENT structure that receives the event handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetDWMVerticalBlankEvent(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetDeviceState routine retrieves the execution or error state of a graphics device.
 *
 * \param Data A pointer to a D3DKMT_GETDEVICESTATE structure that receives the device state.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetDeviceState(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetDisplayModeList routine retrieves a list of available display modes for a video present source.
 *
 * \param Data A pointer to a D3DKMT_GETDISPLAYMODELIST structure that receives available modes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetDisplayModeList(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetMemoryBudgetTarget routine retrieves the video memory budget target and current usage for a process.
 *
 * \param Data A pointer to a memory budget target query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetMemoryBudgetTarget(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetMultiPlaneOverlayCaps routine retrieves hardware multi-plane overlay capabilities for a display adapter.
 *
 * \param Data A pointer to a D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS structure that receives overlay capabilities.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetMultiPlaneOverlayCaps(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetMultisampleMethodList routine retrieves the list of multiple-sampling methods available for a display format.
 *
 * \param Data A pointer to a D3DKMT_GETMULTISAMPLEMETHODLIST structure that receives the multisample methods.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetMultisampleMethodList(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetOverlayState routine retrieves the current state and positioning attributes of a hardware overlay.
 *
 * \param Data A pointer to a D3DKMT_GETOVERLAYSTATE structure that receives the overlay state.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetOverlayState(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetPostCompositionCaps routine retrieves hardware post-composition transformation and color conversion capabilities.
 *
 * \param Data A pointer to a D3DKMT_GET_POST_COMPOSITION_CAPS structure that receives post-composition capabilities.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetPostCompositionCaps(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetPresentHistory routine retrieves copying and flipping present history tokens for performance analysis.
 *
 * \param Data A pointer to a D3DKMT_GETPRESENTHISTORY structure that receives presentation history data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetPresentHistory(
    _Inout_ PVOID Data
    );

// rev
/**
 * NtGdiDdDDIGetPresentQueueEvent
 *
 * win32kfull 10.0.26100.9444: ECX carries a 32-bit identifier and RDX an
 * output handle pointer (0x14033e4ef-0x14033e4f2). The handle copy is 8 bytes
 * (0x14033e542-0x14033e550). PresentQueueId is a descriptive, not symbol-derived, name.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetPresentQueueEvent(
    _In_ ULONG PresentQueueId,
    _Out_ PHANDLE EventHandle
    );

// rev
/**
 * The NtGdiDdDDIGetProcessDeviceRemovalSupport routine queries whether a process supports graphics device removal handling.
 *
 * \param Data A pointer to a D3DKMT_GETPROCESSDEVICEREMOVALSUPPORT structure that receives support flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetProcessDeviceRemovalSupport(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetProcessSchedulingPriorityBand routine retrieves the GPU scheduling priority band assigned to a process.
 *
 * \param Data A pointer to a process scheduling priority band query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetProcessSchedulingPriorityBand(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetProcessSchedulingPriorityClass routine retrieves the scheduling priority class for a process.
 *
 * \param Data A pointer to a structure containing the process handle and output scheduling priority class.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetProcessSchedulingPriorityClass(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetResourcePresentPrivateDriverData routine retrieves private driver data associated with a resource presentation.
 *
 * \param Data A pointer to a D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA structure that receives the private data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetResourcePresentPrivateDriverData(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetRuntimeData routine retrieves runtime information and driver execution statistics for an adapter.
 *
 * \param Data A pointer to a D3DKMT_GETRUNTIMEDATA structure that receives runtime information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetRuntimeData(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetScanLine routine retrieves the current scan line number of the active video present path.
 *
 * \param Data A pointer to a D3DKMT_GETSCANLINE structure that receives the current scan line.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetScanLine(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetSetSwapChainMetadata routine queries or sets HDR metadata and color space attributes for a swap chain.
 *
 * \param Data A pointer to a swap chain metadata query and configuration structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetSetSwapChainMetadata(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetSharedPrimaryHandle routine retrieves the shared primary surface handle for a video present source.
 *
 * \param Data A pointer to a D3DKMT_GETSHAREDPRIMARYHANDLE structure that receives the shared handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetSharedPrimaryHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetSharedResourceAdapterLuid routine retrieves the LUID of the graphics adapter on which a shared resource was created.
 *
 * \param Data A pointer to a D3DKMT_GETSHAREDRESOURCEADAPTERLUID structure that receives the adapter LUID.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetSharedResourceAdapterLuid(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetSharedResourceAdapterLuidFlipManager routine retrieves the adapter LUID for a shared flip manager resource.
 *
 * \param Data A pointer to a flip manager shared resource adapter LUID query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetSharedResourceAdapterLuidFlipManager(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetSwapChainSurfacePhysicalAddress routine retrieves the physical address of a swap chain surface.
 *
 * \param Data A pointer to a swap chain surface physical address query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetSwapChainSurfacePhysicalAddress(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIGetYieldPercentage routine retrieves GPU compute yield percentage metrics for scheduling analysis.
 *
 * \param Data A pointer to a GPU compute yield percentage query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIGetYieldPercentage(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIInvalidateActiveVidPn routine invalidates the active video present network (VidPN) currently in use.
 *
 * \param Data A pointer to a D3DKMT_INVALIDATEACTIVEVIDPN structure that describes the VidPN to invalidate.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIInvalidateActiveVidPn(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIInvalidateCache routine invalidates cached CPU or GPU virtual address mappings and textures.
 *
 * \param Data A pointer to a D3DKMT_INVALIDATECACHE structure that describes the cache to invalidate.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIInvalidateCache(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDILock routine locks an entire allocation or specific subpages to obtain a CPU pointer.
 *
 * \param Data A pointer to a D3DKMT_LOCK structure that describes parameters for locking the allocation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDILock(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDILock2 routine locks an allocation with extended virtual addressing options.
 *
 * \param Data A pointer to a D3DKMT_LOCK2 structure that describes parameters for locking the allocation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDILock2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIMakeResident routine adds resources to the device residency list and increments residency counts.
 *
 * \param Data A pointer to a D3DDDI_MAKERESIDENT structure that describes resources to make resident.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIMakeResident(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIMapGpuVirtualAddress routine maps GPU virtual address ranges to allocation pages or reserved space.
 *
 * \param Data A pointer to a D3DDDI_MAPGPUVIRTUALADDRESS structure that describes the virtual address mapping.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIMapGpuVirtualAddress(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIMarkDeviceAsError routine marks a graphics context or device as having encountered a fatal hardware error.
 *
 * \param Data A pointer to a D3DKMT_MARKDEVICEASERROR structure that specifies the device context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIMarkDeviceAsError(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDINetDispGetNextChunkInfo routine retrieves Miracast or wireless display network frame chunk encoding information.
 *
 * \param Param1 Network display session handle or parameter.
 * \param Param2 Chunk query index.
 * \param Param3 Buffer size or chunk data offset.
 * \param Param4 Chunk information output buffer.
 * \param Param5 Timing or status parameter.
 * \param Param6 Additional chunk flags.
 * \param Param7 Additional session options.
 * \return NTSTATUS Successful or errant status.
 * \remarks DirectX Graphics Kernel (D3DKMT) thunk; forwards parameters to dxgkrnl.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDINetDispGetNextChunkInfo(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2,
    _In_ ULONG_PTR Param3,
    _In_ ULONG_PTR Param4,
    _In_ ULONG_PTR Param5,
    _In_ ULONG_PTR Param6,
    _In_ ULONG_PTR Param7
    );

// rev
/**
 * The NtGdiDdDDINetDispQueryMiracastDisplayDeviceStatus routine queries the status of a Miracast wireless display device.
 *
 * \param DeviceName A pointer to a null-terminated Unicode string specifying the device name.
 * \param Status An output pointer that receives the Miracast display device status.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDINetDispQueryMiracastDisplayDeviceStatus(
    _In_ PCWSTR DeviceName,
    _Out_ PVOID Status
    );

// rev
/**
 * The NtGdiDdDDINetDispQueryMiracastDisplayDeviceSupport routine queries whether the graphics driver supports Miracast wireless display devices.
 *
 * \param Data A pointer to a Miracast display device support query structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDINetDispQueryMiracastDisplayDeviceSupport(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDINetDispStartMiracastDisplayDevice routine starts a Miracast wireless display device session.
 *
 * \param Data A pointer to a Miracast display device start configuration structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDINetDispStartMiracastDisplayDevice(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDINetDispStopMiracastDisplayDevice routine stops an active Miracast wireless display session and disconnects the device.
 *
 * \param DeviceName A pointer to a null-terminated Unicode string specifying the device name.
 * \param DeviceHandle A handle to the Miracast display device.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDINetDispStopMiracastDisplayDevice(
    _In_ PCWSTR DeviceName,
    _In_ HANDLE DeviceHandle
    );

// rev
/**
 * The NtGdiDdDDIOfferAllocations routine offers video memory allocations for reuse when memory pressure occurs.
 *
 * \param Data A pointer to a D3DKMT_OFFERALLOCATIONS structure that describes allocations to offer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOfferAllocations(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenAdapterFromDeviceName routine opens a graphics adapter from a device name.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenAdapterFromDeviceName(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenAdapterFromHdc routine opens a graphics adapter from a device context handle (HDC).
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenAdapterFromHdc(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenAdapterFromLuid routine opens a graphics adapter from a locally unique identifier (LUID).
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenAdapterFromLuid(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenBundleObjectNtHandleFromName routine opens a bundle object NT handle from an object name.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenBundleObjectNtHandleFromName(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenKeyedMutex routine opens a keyed mutex object.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenKeyedMutex(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenKeyedMutex2 routine opens a keyed mutex object with extended parameters.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenKeyedMutex2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenKeyedMutexFromNtHandle routine opens a keyed mutex object from an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenKeyedMutexFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenNtHandleFromName routine opens an NT handle from an object name.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenNtHandleFromName(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenProtectedSessionFromNtHandle routine opens a protected session from an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenProtectedSessionFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenResource routine opens a shared graphics resource.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenResource(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenResourceFromNtHandle routine opens a shared graphics resource from an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenResourceFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenSwapChain routine opens a swap chain.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenSyncObjectFromNtHandle routine opens a synchronization object from an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenSyncObjectFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenSyncObjectFromNtHandle2 routine opens a synchronization object from an NT handle with extended attributes.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenSyncObjectFromNtHandle2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenSyncObjectNtHandleFromName routine opens a synchronization object NT handle from an object name.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenSyncObjectNtHandleFromName(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOpenSynchronizationObject routine opens a shared synchronization object.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOpenSynchronizationObject(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOutputDuplGetFrameInfo routine retrieves frame information for an output duplication context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOutputDuplGetFrameInfo(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOutputDuplGetMetaData routine retrieves metadata for an output duplication frame.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOutputDuplGetMetaData(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOutputDuplGetPointerShapeData routine retrieves pointer shape data for an output duplication context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOutputDuplGetPointerShapeData(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOutputDuplPresent routine presents an output duplication frame to a swap chain or surface.
 *
 * \param Data A pointer to a structure containing output duplication present parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOutputDuplPresent(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIOutputDuplReleaseFrame routine releases an output duplication frame.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIOutputDuplReleaseFrame(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPollDisplayChildren routine polls the display children of an adapter.
 *
 * \param Data A pointer to a structure containing poll display children parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPollDisplayChildren(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPresent routine presents video content to a display surface.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPresent(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPresentMultiPlaneOverlay routine presents multi-plane overlay content.
 *
 * \param Data A pointer to a structure containing multi-plane overlay presentation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPresentMultiPlaneOverlay(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPresentMultiPlaneOverlay2 routine presents multi-plane overlay content with extended configurations.
 *
 * \param Data A pointer to a structure containing multi-plane overlay presentation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPresentMultiPlaneOverlay2(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPresentMultiPlaneOverlay3 routine presents multi-plane overlay content with support for advanced multi-plane operations.
 *
 * \param Data A pointer to a structure containing multi-plane overlay presentation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPresentMultiPlaneOverlay3(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIPresentRedirected routine presents redirected surface content to the display.
 *
 * \param Data A pointer to a structure containing redirected presentation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIPresentRedirected(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryAdapterInfo routine queries configuration and capability information from a graphics adapter.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryAdapterInfo(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryAllocationResidency routine queries the residency status of a list of graphics allocations.
 *
 * \param Data A pointer to a structure containing allocation residency query parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryAllocationResidency(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryClockCalibration routine queries clock calibration data for an adapter.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryClockCalibration(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryFSEBlock routine queries full-screen exclusive (FSE) block status.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryFSEBlock(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryProcessOfferInfo routine queries offer and reclaim information for a process.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryProcessOfferInfo(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryProtectedSessionInfoFromNtHandle routine queries protected session information from an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryProtectedSessionInfoFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryProtectedSessionStatus routine queries the status of a protected session.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryProtectedSessionStatus(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryRemoteVidPnSourceFromGdiDisplayName routine queries a remote VidPN source identifier from a GDI display name.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryRemoteVidPnSourceFromGdiDisplayName(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryResourceInfo routine queries shared resource information.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryResourceInfo(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryResourceInfoFromNtHandle routine queries shared resource information using an NT handle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryResourceInfoFromNtHandle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryStatistics routine queries performance and memory statistics from the DirectX graphics subsystem.
 *
 * \param Data A pointer to a structure containing query statistics parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryStatistics(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryVidPnExclusiveOwnership routine queries exclusive ownership status of a VidPN source.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryVidPnExclusiveOwnership(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIQueryVideoMemoryInfo routine queries video memory budget and usage information.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIQueryVideoMemoryInfo(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReclaimAllocations routine reclaims discarded video memory allocations.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReclaimAllocations(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReclaimAllocations2 routine reclaims discarded video memory allocations with extended results.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReclaimAllocations2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReleaseKeyedMutex routine releases a keyed mutex object.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReleaseKeyedMutex(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReleaseKeyedMutex2 routine releases a keyed mutex object with extended state.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReleaseKeyedMutex2(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReleaseProcessVidPnSourceOwners routine releases ownership of all VidPN sources associated with a process.
 *
 * \param Data A handle to the process whose VidPN source ownership is being released.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReleaseProcessVidPnSourceOwners(
    _In_ HANDLE Data
    );

// rev
/**
 * The NtGdiDdDDIReleaseSwapChain routine releases a swap chain.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReleaseSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIRemoveSurfaceFromSwapChain routine removes a surface from a swap chain.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIRemoveSurfaceFromSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIRender routine submits a command buffer to a GPU context for execution.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIRender(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIReserveGpuVirtualAddress routine reserves a range of GPU virtual address space.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIReserveGpuVirtualAddress(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetAllocationPriority routine sets the eviction and scheduling priority of graphics allocations.
 *
 * \param Data A pointer to a structure containing allocation priority settings.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetAllocationPriority(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetContextInProcessSchedulingPriority routine sets the in-process scheduling priority for a context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetContextInProcessSchedulingPriority(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetContextSchedulingPriority routine sets the scheduling priority for a device context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetContextSchedulingPriority(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetDisplayMode routine sets the display mode for a display device.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetDisplayMode(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetDodIndirectSwapchain routine sets an indirect swap chain for a display-only driver (DOD).
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetDodIndirectSwapchain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetFSEBlock routine sets full-screen exclusive (FSE) block parameters.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetFSEBlock(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetGammaRamp routine sets the gamma ramp for an adapter.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetGammaRamp(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetHwProtectionTeardownRecovery routine configures teardown recovery for hardware-protected content.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetHwProtectionTeardownRecovery(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetMemoryBudgetTarget routine sets the memory budget target for a process.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetMemoryBudgetTarget(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetMonitorColorSpaceTransform routine sets the color space transform for a monitor.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetMonitorColorSpaceTransform(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetProcessDeviceRemovalSupport routine sets whether a process supports graphics device removal.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetProcessDeviceRemovalSupport(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetProcessSchedulingPriorityBand routine sets the process scheduling priority band.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetProcessSchedulingPriorityBand(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetProcessSchedulingPriorityClass routine sets the process scheduling priority class.
 *
 * \param Data The process scheduling priority class or parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetProcessSchedulingPriorityClass(
    _In_ ULONG_PTR Data
    );

// rev
/**
 * The NtGdiDdDDISetQueuedLimit routine sets the limit on queued rendering operations.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetQueuedLimit(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetStablePowerState routine enables or disables stable power state on a GPU adapter.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetStablePowerState(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetStereoEnabled routine enables or disables stereoscopic 3D display support.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetStereoEnabled(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetSyncRefreshCountWaitTarget routine sets the target refresh count for vertical sync synchronization.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetSyncRefreshCountWaitTarget(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetVidPnSourceHwProtection routine sets hardware protection on a VidPN source.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetVidPnSourceHwProtection(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetVidPnSourceOwner routine sets or releases the ownership of a VidPN source.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetVidPnSourceOwner(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISetYieldPercentage routine sets the execution yield percentage for a GPU context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISetYieldPercentage(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIShareObjects routine shares graphics resource objects across processes.
 *
 * \param Data A pointer to a structure containing object sharing parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIShareObjects(
    _Inout_ PVOID * Data
    );

// rev
/**
 * The NtGdiDdDDISharedPrimaryLockNotification routine notifies the kernel when locking a shared primary surface.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISharedPrimaryLockNotification(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISharedPrimaryUnLockNotification routine notifies the kernel when unlocking a shared primary surface.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISharedPrimaryUnLockNotification(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISignalSynchronizationObject routine signals a synchronization object.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISignalSynchronizationObject(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISignalSynchronizationObjectFromCpu routine signals a monitored fence or synchronization object from the CPU.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISignalSynchronizationObjectFromCpu(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISignalSynchronizationObjectFromGpu routine submits a signal operation for a synchronization object from the GPU.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISignalSynchronizationObjectFromGpu(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISignalSynchronizationObjectFromGpu2 routine submits a signal operation for a synchronization object from the GPU with extended parameters.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISignalSynchronizationObjectFromGpu2(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISubmitCommand routine submits a command buffer to a GPU context.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISubmitCommand(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISubmitCommandToHwQueue routine submits a command buffer to a hardware queue.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISubmitCommandToHwQueue(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISubmitSignalSyncObjectsToHwQueue routine submits a signal operation for synchronization objects to a hardware queue.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISubmitSignalSyncObjectsToHwQueue(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDISubmitWaitForSyncObjectsToHwQueue routine submits a wait operation for synchronization objects to a hardware queue.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDISubmitWaitForSyncObjectsToHwQueue(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDITrimProcessCommitment routine trims video memory allocations committed by a process.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDITrimProcessCommitment(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUnOrderedPresentSwapChain routine presents swap chain buffers in unordered mode.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUnOrderedPresentSwapChain(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUnlock routine unlocks a list of graphics allocations.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUnlock(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUnlock2 routine unlocks a list of graphics allocations with extended options.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUnlock2(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUpdateAllocationProperty routine updates properties of graphics allocations.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUpdateAllocationProperty(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUpdateGpuVirtualAddress routine updates mappings in the GPU virtual address space.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUpdateGpuVirtualAddress(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIUpdateOverlay routine updates or displays a hardware overlay.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIUpdateOverlay(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForIdle routine waits until a GPU device or context becomes idle.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForIdle(
    _Inout_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForSynchronizationObject routine waits for one or more synchronization objects to be signaled.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForSynchronizationObject(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForSynchronizationObjectFromCpu routine waits for a monitored fence or synchronization object from the CPU.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForSynchronizationObjectFromCpu(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForSynchronizationObjectFromGpu routine inserts a wait operation into a GPU context command stream.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForSynchronizationObjectFromGpu(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForVerticalBlankEvent routine waits for a vertical blanking event on a display adapter.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForVerticalBlankEvent(
    _In_ PVOID Data
    );

// rev
/**
 * The NtGdiDdDDIWaitForVerticalBlankEvent2 routine waits for a vertical blanking event on a display adapter with extended options.
 *
 * \param Data A pointer to a structure containing operation parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDDIWaitForVerticalBlankEvent2(
    _In_ PVOID Data
    );

//
// GDI Graphics & Client Exports
//

/**
 * Language pack (LPK) draw-call entry-point indices used with NtUserRegisterLPK.
 */
#define LPK_TABBED_TEXT_OUT 0
#define LPK_PSM_TEXT_OUT    1
#define LPK_DRAW_TEXT_EX    2
#define LPK_EDIT_CONTROL    3

/**
 * Bit-mask forms (LPK_FLAG_*) of the LPK_* language-pack draw-call selectors.
 * \remarks Reverse-engineered.
 */
#define LPK_FLAG_TABBED_TEXT_OUT (1 << LPK_TABBED_TEXT_OUT)
#define LPK_FLAG_PSM_TEXT_OUT    (1 << LPK_PSM_TEXT_OUT)
#define LPK_FLAG_DRAW_TEXT_EX    (1 << LPK_DRAW_TEXT_EX)
#define LPK_FLAG_EDIT_CONTROL    (1 << LPK_EDIT_CONTROL)

// rev
/**
 * The AddFontResourceTracking routine adds font resource tracking for a font file.
 *
 * \param MultiByteString A pointer to the font resource path string.
 * \param Flags Tracking flags.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
AddFontResourceTracking(
    _Inout_ PSTR MultiByteString,
    _In_ LONG Flags
    );

// rev
/**
 * The AnyLinkedFonts routine determines whether any linked fonts are currently registered.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
AnyLinkedFonts(
    VOID
    );

// rev
/**
 * The BeginGdiRendering routine initiates GDI rendering operations.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
BeginGdiRendering(
    VOID
    );

// rev
/**
 * The ClearBitmapAttributes routine clears attributes on a bitmap object.
 *
 * \param BitmapHandle A handle to the bitmap.
 * \param Flags Attributes to clear.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
ClearBitmapAttributes(
    _In_ HBITMAP BitmapHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The ClearBrushAttributes routine clears attributes on a brush object.
 *
 * \param BrushHandle A handle to the brush.
 * \param Flags Attributes to clear.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
ClearBrushAttributes(
    _In_ HBRUSH BrushHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The CreateBitmapFromDxSurface routine creates a GDI bitmap from a DirectX surface.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CreateBitmapFromDxSurface(
    VOID
    );

// rev
/**
 * The CreateBitmapFromDxSurface2 routine creates a GDI bitmap from a DirectX surface with extended options.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CreateBitmapFromDxSurface2(
    VOID
    );

// rev
/**
 * The CreateDCExW routine creates a device context with extended options.
 *
 * \param Driver Optional driver name.
 * \param Device Optional device name.
 * \param Output Optional output port or file.
 * \param InitData Optional initialization devmode data.
 * \param Flags Device context creation flags.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
CreateDCExW(
    _In_opt_ PCWSTR Driver,
    _In_opt_ PCWSTR Device,
    _In_opt_ PVOID Output,
    _In_opt_ PVOID InitData,
    _In_ CHAR Flags
    );

// rev
/**
 * The CreateScaledCompatibleBitmap routine creates a scaled compatible bitmap for a device context.
 *
 * \param Hdc A handle to the device context.
 * \param Width Bitmap width in pixels.
 * \param Height Bitmap height in pixels.
 * \param Scale DPI scaling factor.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
CreateScaledCompatibleBitmap(
    _In_ HDC Hdc,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ SHORT Scale
    );

// rev
/**
 * The DDCCIGetCapabilitiesString routine retrieves the DDC/CI capabilities string from a physical monitor.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCIGetCapabilitiesString(
    VOID
    );

// rev
/**
 * The DDCCIGetCapabilitiesStringLength routine retrieves the length of the DDC/CI capabilities string.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCIGetCapabilitiesStringLength(
    VOID
    );

// rev
/**
 * The DDCCIGetTimingReport routine retrieves the timing report from a physical monitor via DDC/CI.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCIGetTimingReport(
    VOID
    );

// rev
/**
 * The DDCCIGetVCPFeature routine queries a Virtual Control Panel (VCP) feature from a monitor via DDC/CI.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCIGetVCPFeature(
    VOID
    );

// rev
/**
 * The DDCCISaveCurrentSettings routine saves current settings to non-volatile monitor memory via DDC/CI.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCISaveCurrentSettings(
    VOID
    );

// rev
/**
 * The DDCCISetVCPFeature routine sets a Virtual Control Panel (VCP) feature on a monitor via DDC/CI.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DDCCISetVCPFeature(
    VOID
    );

// rev
/**
 * The DwmCreatedBitmapRemotingOutput routine creates a remoting bitmap output context for DWM.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DwmCreatedBitmapRemotingOutput(
    VOID
    );

// rev
/**
 * The EnableEUDC routine enables or disables End-User Defined Characters (EUDC) font support.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
EnableEUDC(
    VOID
    );

// rev
/**
 * The EndGdiRendering routine finalizes GDI rendering operations.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
EndGdiRendering(
    VOID
    );

// rev
/**
 * The FontIsLinked routine determines whether a font has linked font fallback associations.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
FontIsLinked(
    VOID
    );

// rev
/**
 * The Gdi32DllInitialize routine initializes the GDI32 client library.
 *
 * \param Instance Module handle of the DLL.
 * \param Reason Reason code for calling the function (DLL_PROCESS_ATTACH, etc.).
 * \param Reserved Reserved parameter context.
 * \return BOOL TRUE if initialization succeeds, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
Gdi32DllInitialize(
    _In_ HINSTANCE Instance,
    _In_ DWORD Reason,
    _In_opt_ PVOID Reserved
    );

// rev
/**
 * The GdiAddFontResourceW routine adds a font resource from a file with extended design vector options.
 *
 * \param FileName Path to the font resource file.
 * \param Flags Font resource installation flags.
 * \param DesignVector Optional pointer to a DESIGNVECTOR structure.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiAddFontResourceW(
    _In_ PCWSTR FileName,
    _In_ LONG Flags,
    _In_opt_ PVOID DesignVector
    );

// rev
/**
 * The GdiAddGlsBounds routine adds OpenGL (GLS) bounding rectangle records to an EMF spool stream.
 *
 * \param Hdc A handle to the device context.
 * \param Bounds A pointer to a RECT structure defining the bounding box.
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiAddGlsBounds(
    _In_ HDC Hdc,
    _In_ PRECT Bounds
    );

// rev
/**
 * The GdiAddGlsRecord routine adds an OpenGL (GLS) record to an EMF spool stream.
 *
 * \param Hdc A handle to the device context.
 * \param RecordSize The size in bytes of the GLS record.
 * \param Record A pointer to the record buffer.
 * \param Bounds Optional pointer to bounding rectangle.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiAddGlsRecord(
    _In_ LONG Hdc,
    _In_ ULONG RecordSize,
    _In_reads_bytes_(RecordSize) PVOID Record,
    _In_opt_ PRECT Bounds
    );

// rev
/**
 * The GdiAddInitialFonts routine loads initial system fonts into the GDI font table.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiAddInitialFonts(
    VOID
    );

// rev
/**
 * The GdiArtificialDecrementDriver routine artificially decrements the reference count of a graphics driver.
 *
 * \param DriverName The name of the graphics driver.
 * \param Flags Operation flags.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiArtificialDecrementDriver(
    _In_ PCWSTR DriverName,
    _In_ CHAR Flags
    );

// rev
/**
 * GdiBatchLimit is a gdi32.dll DATA export, not a function.
 * 32-bit batch limit, compared with EAX at gdi32!0x18000135e.
 */
NTSYSAPI extern ULONG GdiBatchLimit;

// rev
/**
 * The GdiCleanCacheDC routine cleans cached device contexts.
 *
 * \param Hdc Handle to the device context.
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiCleanCacheDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiConsoleTextOut routine outputs console text directly through GDI accelerated routines.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiConsoleTextOut(
    VOID
    );

// rev
/**
 * The GdiConvertAndCheckDC routine validates and converts a user-mode device context handle.
 *
 * \param Hdc A handle to the device context to check and convert.
 * \return HDC The converted device context handle, or NULL on failure.
 */
NTSYSAPI
HDC
NTAPI
GdiConvertAndCheckDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiConvertBitmap routine converts a bitmap format for GDI rendering.
 *
 * \param Bitmap Handle to the bitmap.
 * \return HBITMAP The converted bitmap handle.
 */
NTSYSAPI
HBITMAP
NTAPI
GdiConvertBitmap(
    _In_ HBITMAP Bitmap
    );

// rev
/**
 * The GdiConvertBitmapV5 routine converts a BITMAPV5HEADER structure to a GDI bitmap handle.
 *
 * \param Source A pointer to the BITMAPV5HEADER structure.
 * \param Size The size of the header structure.
 * \param Palette Third parameter.
 * \param Format Color table usage.
 * \return HBITMAP The created bitmap handle, or NULL on failure.
 */
NTSYSAPI
HBITMAP
NTAPI
GdiConvertBitmapV5(
    _In_ PVOID Source,
    _In_ ULONG Size,
    _In_ HPALETTE Palette,
    _In_ ULONG Format
    );

// rev
/**
 * The GdiConvertBrush routine converts a brush handle across process or subsystem boundaries.
 *
 * \param Brush Handle to the brush.
 * \return HBRUSH The converted brush handle.
 */
NTSYSAPI
HBRUSH
NTAPI
GdiConvertBrush(
    _In_ HBRUSH Brush
    );

// rev
/**
 * The GdiConvertDC routine converts a device context handle across subsystems.
 *
 * \param Hdc Handle to the device context.
 * \return HDC The converted device context handle.
 */
NTSYSAPI
HDC
NTAPI
GdiConvertDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiConvertEnhMetaFile routine converts an enhanced metafile handle.
 *
 * \param MetaFileHandle A handle to the enhanced metafile.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiConvertEnhMetaFile(
    _In_ HENHMETAFILE MetaFileHandle
    );

// rev
/**
 * The GdiConvertFont routine converts a font handle across subsystems.
 *
 * \param Font Handle to the font.
 * \return HFONT The converted font handle.
 */
NTSYSAPI
HFONT
NTAPI
GdiConvertFont(
    _In_ HFONT Font
    );

// rev
/**
 * The GdiConvertMetaFilePict routine converts a metafile picture handle across subsystems.
 *
 * \param HMem Handle or memory descriptor to the metafile picture.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiConvertMetaFilePict(
    _In_ ULONG_PTR HMem
    );

// rev
/**
 * The GdiConvertPalette routine converts a palette handle across subsystems.
 *
 * \param Palette Handle to the palette.
 * \return HPALETTE The converted palette handle.
 */
NTSYSAPI
HPALETTE
NTAPI
GdiConvertPalette(
    _In_ HPALETTE Palette
    );

// rev
/**
 * The GdiConvertRegion routine converts a region handle across subsystems.
 *
 * \param Region Handle to the region.
 * \return HRGN The converted region handle.
 */
NTSYSAPI
HRGN
NTAPI
GdiConvertRegion(
    _In_ HRGN Region
    );

// rev
/**
 * GdiConvertToDevmodeW
 */
NTSYSAPI
PDEVMODEW
NTAPI
GdiConvertToDevmodeW(
    _In_ const DEVMODEA *DeviceMode
    );

// rev
/**
 * The GdiCreateLocalEnhMetaFile routine creates a local enhanced metafile object.
 *
 * \param MetaFileHandle A handle to the enhanced metafile.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiCreateLocalEnhMetaFile(
    _In_ HANDLE MetaFileHandle
    );

// rev
/**
 * The GdiCreateLocalMetaFilePict routine creates a local metafile picture object.
 *
 * \param MetaFileHandle A handle to the metafile picture.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiCreateLocalMetaFilePict(
    _In_ HANDLE MetaFileHandle
    );

// rev
/**
 * The GdiCurrentProcessSplWow64 routine checks whether the current process is running under WOW64 spooler interop.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiCurrentProcessSplWow64(
    VOID
    );

// rev
/**
 * The GdiDeleteLocalDC routine deletes a local client-side device context object.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiDeleteLocalDC(
    VOID
    );

// rev
/**
 * GdiDeleteSpoolFileHandle
 */
NTSYSAPI
BOOL
NTAPI
GdiDeleteSpoolFileHandle(
    _In_ HANDLE SpoolFileHandle
    );

// rev
/**
 * The GdiDescribePixelFormat routine describes a pixel format supported by a device context.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiDescribePixelFormat(
    VOID
    );

// rev
/**
 * The GdiDisableUMPDSandboxing routine disables User-Mode Printer Driver (UMPD) sandboxing.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiDisableUMPDSandboxing(
    VOID
    );

// rev
/**
 * The GdiDllInitialize routine initializes or cleans up the GDI client DLL state.
 *
 * \param HinstDLL Instance handle of the DLL.
 * \param Reason The reason for calling DLL entry.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiDllInitialize(
    _In_ PVOID HinstDLL,
    _In_ LONG Reason
    );

// rev
/**
 * The GdiDrawStream routine renders a primitive draw stream to a device context.
 *
 * \param Hdc A handle to the device context.
 * \param Size The size in bytes of the draw stream buffer.
 * \param Stream A pointer to the stream payload buffer.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiDrawStream(
    _In_ HDC Hdc,
    _In_ ULONG_PTR Size,
    _In_reads_bytes_(Size) PVOID Stream
    );

// rev
/**
 * GdiEndDocEMF
 */
NTSYSAPI
BOOL
NTAPI
GdiEndDocEMF(
    _In_ HANDLE SpoolFileHandle
    );

// rev
/**
 * GdiEndPageEMF
 */
NTSYSAPI
BOOL
NTAPI
GdiEndPageEMF(
    _In_ HANDLE SpoolFileHandle,
    _In_ ULONG Optimization
    );

// rev
/**
 * The GdiEntry1 (DdCreateDirectDrawObject) routine creates a DirectDraw object for the specified device context.
 *
 * \param DirectDrawGlobal Pointer to a DDRAWI_DIRECTDRAW_GBL structure that receives the DirectDraw object data.
 * \param Hdc Handle to the device context for which to create the DirectDraw object.
 * \return TRUE if the DirectDraw object was created successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry1(
    _Inout_ struct _DDRAWI_DIRECTDRAW_GBL *DirectDrawGlobal,
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiEntry10 (DdReenableDirectDrawObject) routine re-enables a DirectDraw object after a display mode change.
 *
 * \param DirectDrawGlobal Pointer to the DDRAWI_DIRECTDRAW_GBL structure representing the DirectDraw object.
 * \param NewMode Pointer to a boolean receiving whether a mode change occurred.
 * \return TRUE if the DirectDraw object was re-enabled successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry10(
    _In_ struct _DDRAWI_DIRECTDRAW_GBL *DirectDrawGlobal,
    _Out_ PBOOL NewMode
    );

// rev
/**
 * The GdiEntry11 (DdAttachSurface) routine attaches one DirectDraw surface to another.
 *
 * \param SurfaceFrom Pointer to the source DDRAWI_DDRAWSURFACE_LCL structure.
 * \param SurfaceTo Pointer to the target DDRAWI_DDRAWSURFACE_LCL structure to attach.
 * \return TRUE if the surfaces were attached successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry11(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceFrom,
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceTo
    );

// rev
/**
 * The GdiEntry12 (DdUnattachSurface) routine detaches an attached DirectDraw surface.
 *
 * \param Surface Pointer to the parent DDRAWI_DDRAWSURFACE_LCL structure.
 * \param SurfaceAttached Pointer to the attached DDRAWI_DDRAWSURFACE_LCL structure to detach.
 */
NTSYSAPI
VOID
NTAPI
GdiEntry12(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *Surface,
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceAttached
    );

// rev
/**
 * The GdiEntry13 (DdQueryDisplaySettingsUniqueness) routine retrieves the display settings uniqueness counter.
 *
 * \return The current display settings uniqueness counter value.
 */
NTSYSAPI
ULONG
NTAPI
GdiEntry13(
    VOID
    );

// rev
/**
 * The GdiEntry14 (DdGetDxHandle) routine retrieves or releases a DirectX kernel handle for a DirectDraw object or surface.
 *
 * \param DirectDrawLocal Optional pointer to a DDRAWI_DIRECTDRAW_LCL structure representing the DirectDraw object.
 * \param SurfaceLocal Optional pointer to a DDRAWI_DDRAWSURFACE_LCL structure representing the surface.
 * \param Release TRUE to release the handle, FALSE to retrieve the handle.
 * \return The kernel DirectX handle, or NULL on failure.
 */
NTSYSAPI
HANDLE
NTAPI
GdiEntry14(
    _In_opt_ struct _DDRAWI_DIRECTDRAW_LCL *DirectDrawLocal,
    _In_opt_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal,
    _In_ BOOL Release
    );

// rev
/**
 * The GdiEntry15 (DdSetGammaRamp) routine sets the gamma ramp for a DirectDraw device or device context.
 *
 * \param DirectDrawLocal Optional pointer to a DDRAWI_DIRECTDRAW_LCL structure.
 * \param Hdc Handle to the device context.
 * \param GammaRamp Pointer to a buffer containing the gamma ramp data.
 * \return TRUE if the gamma ramp was set successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry15(
    _In_opt_ struct _DDRAWI_DIRECTDRAW_LCL *DirectDrawLocal,
    _In_ HDC Hdc,
    _In_ PVOID GammaRamp
    );

// rev
/**
 * The GdiEntry16 (DdSwapTextureHandles) routine swaps the driver texture handles between two DirectDraw surfaces.
 *
 * \param DirectDrawLocal Pointer to a DDRAWI_DIRECTDRAW_LCL structure.
 * \param SurfaceLocal1 Pointer to the first DDRAWI_DDRAWSURFACE_LCL structure.
 * \param SurfaceLocal2 Pointer to the second DDRAWI_DDRAWSURFACE_LCL structure.
 * \return Status code (DD_OK / 0 on success).
 */
NTSYSAPI
ULONG
NTAPI
GdiEntry16(
    _In_ struct _DDRAWI_DIRECTDRAW_LCL *DirectDrawLocal,
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal1,
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal2
    );

// rev
/**
 * The GdiEntry2 (DdQueryDirectDrawObject) routine queries capabilities and driver callbacks of a DirectDraw object.
 *
 * \param DirectDrawGlobal Pointer to the DDRAWI_DIRECTDRAW_GBL structure representing the DirectDraw object.
 * \param HalInfo Pointer to a DDHALINFO structure that receives driver capability information.
 * \param Callbacks Pointer to a DDHAL_DDCALLBACKS structure that receives driver callback table.
 * \param SurfaceCallbacks Pointer to a DDHAL_DDSURFACECALLBACKS structure that receives surface callback table.
 * \param PaletteCallbacks Pointer to a DDHAL_DDPALETTECALLBACKS structure that receives palette callback table.
 * \param D3dCallbacks Pointer to a D3DHAL_CALLBACKS structure that receives Direct3D callback table.
 * \param D3dDriverData Pointer to a D3DHAL_GLOBALDRIVERDATA structure that receives Direct3D driver data.
 * \param D3dBufferCallbacks Pointer to a DDHAL_DDEXEBUFCALLBACKS structure that receives execute buffer callbacks.
 * \param D3dTextureFormats Pointer to an array of DDSURFACEDESC structures receiving supported texture formats.
 * \param FourCC Optional pointer to an array of DWORDs receiving supported FourCC formats.
 * \param VmList Optional pointer to an array of VIDMEM structures receiving video memory heap descriptions.
 * \return TRUE if the query succeeded, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry2(
    _In_ struct _DDRAWI_DIRECTDRAW_GBL *DirectDrawGlobal,
    _Out_ struct _DDHALINFO *HalInfo,
    _Out_ struct _DDHAL_DDCALLBACKS *Callbacks,
    _Out_ struct _DDHAL_DDSURFACECALLBACKS *SurfaceCallbacks,
    _Out_ struct _DDHAL_DDPALETTECALLBACKS *PaletteCallbacks,
    _Out_ struct _D3DHAL_CALLBACKS *D3dCallbacks,
    _Out_ struct _D3DHAL_GLOBALDRIVERDATA *D3dDriverData,
    _Out_ struct _DDHAL_DDEXEBUFCALLBACKS *D3dBufferCallbacks,
    _Out_ struct _DDSURFACEDESC *D3dTextureFormats,
    _Out_opt_ PULONG FourCC,
    _Out_opt_ struct _VIDMEM *VmList
    );

// rev
/**
 * The GdiEntry3 (DdDeleteDirectDrawObject) routine deletes a previously created kernel-mode DirectDraw object.
 *
 * \param DirectDrawGlobal Pointer to the DDRAWI_DIRECTDRAW_GBL structure representing the DirectDraw object.
 * \return TRUE if the DirectDraw object was deleted successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry3(
    _In_ struct _DDRAWI_DIRECTDRAW_GBL *DirectDrawGlobal
    );

// rev
/**
 * The GdiEntry4 (DdCreateSurfaceObject) routine creates a kernel-mode surface object corresponding to a local DirectDraw surface.
 *
 * \param SurfaceLocal Pointer to the DDRAWI_DDRAWSURFACE_LCL structure representing the local surface.
 * \param PrimarySurface TRUE if the surface is a primary surface, FALSE otherwise.
 * \return TRUE if the surface object was created successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry4(
    _Inout_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal,
    _In_ BOOL PrimarySurface
    );

// rev
/**
 * The GdiEntry5 (DdDeleteSurfaceObject) routine deletes a kernel-mode DirectDraw surface object.
 *
 * \param SurfaceLocal Pointer to the DDRAWI_DDRAWSURFACE_LCL structure representing the surface object to delete.
 * \return TRUE if the surface object was deleted successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry5(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal
    );

// rev
/**
 * The GdiEntry6 (DdResetVisrgn) routine resets the visible region for a DirectDraw surface associated with a window.
 *
 * \param SurfaceLocal Pointer to the DDRAWI_DDRAWSURFACE_LCL structure representing the surface.
 * \param WindowHandle Optional handle to the window whose visible region is being reset.
 * \return TRUE if the visible region was reset successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry6(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal,
    _In_opt_ HWND WindowHandle
    );

// rev
/**
 * The GdiEntry7 (DdGetDC) routine creates a device context for the specified DirectDraw surface.
 *
 * \param SurfaceLocal Pointer to the DDRAWI_DDRAWSURFACE_LCL structure representing the surface.
 * \param ColorTable Optional pointer to a palette/color table of PALETTEENTRY structures.
 * \return Handle to the created device context, or NULL on failure.
 */
NTSYSAPI
HDC
NTAPI
GdiEntry7(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal,
    _In_opt_ PALETTEENTRY *ColorTable
    );

// rev
/**
 * The GdiEntry8 (DdReleaseDC) routine releases a device context created for a DirectDraw surface.
 *
 * \param SurfaceLocal Pointer to the DDRAWI_DDRAWSURFACE_LCL structure representing the surface.
 * \return TRUE if the device context was released successfully, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiEntry8(
    _In_ struct _DDRAWI_DDRAWSURFACE_LCL *SurfaceLocal
    );

// rev
/**
 * The GdiEntry9 routine creates a DIB section (internal GdiEntry9 implementation).
 *
 * \param Hdc Handle to the device context.
 * \param BitmapInfo Pointer to a BITMAPINFO structure.
 * \param Usage Color table format.
 * \param Bits Pointer to a variable receiving the location of the bitmap bit values.
 * \param Section Handle to a file-mapping object.
 * \param Offset Offset from the beginning of the file-mapping object.
 * \return HBITMAP Handle to the DIB section bitmap, or NULL on failure.
 */
NTSYSAPI
HBITMAP
NTAPI
GdiEntry9(
    _In_ HDC Hdc,
    _In_ const BITMAPINFO *BitmapInfo,
    _In_ ULONG Usage,
    _Out_ PVOID *Bits,
    _In_opt_ HANDLE Section,
    _In_ ULONG Offset
    );

// rev
/**
 * The GdiFixUpHandle routine fixes up a client-side GDI handle index.
 *
 * \param Handle A handle or index to fix up.
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiFixUpHandle(
    _In_ LONG_PTR Handle
    );

// rev
/**
 * The GdiFullscreenControl routine controls fullscreen display modes in GDI.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiFullscreenControl(
    VOID
    );

// rev
/**
 * The GdiGetBitmapBitsSize routine calculates the size in bytes of bitmap image bits.
 *
 * \param BitmapInfo A pointer to the bitmap information structure.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiGetBitmapBitsSize(
    _In_ PVOID BitmapInfo
    );

// rev
/**
 * GdiGetCharDimensions
 */
NTSYSAPI
LONG
NTAPI
GdiGetCharDimensions(
    _In_ HDC Hdc,
    _Out_opt_ TEXTMETRICW *TextMetric,
    _Out_ LONG *Height
    );

// rev
/**
 * GdiGetDC
 */
NTSYSAPI
HDC
NTAPI
GdiGetDC(
    _In_ HANDLE SpoolFileHandle
    );

// rev
/**
 * GdiGetDevmodeForPage
 */
NTSYSAPI
BOOL
NTAPI
GdiGetDevmodeForPage(
    _In_ HANDLE SpoolFileHandle,
    _In_ DWORD PageNumber,
    _Out_opt_ PDEVMODEW *CurrentDeviceMode,
    _Out_opt_ PDEVMODEW *LastDeviceMode
    );

// rev
/**
 * The GdiGetEntry routine retrieves an internal GDI handle table entry.
 *
 * \param IndexOrHandle The GDI table index or handle.
 * \param Entry Pointer to a 24-byte entry buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
GdiGetEntry(
    _In_ ULONG IndexOrHandle,
    _Out_writes_bytes_(24) PVOID Entry
    );

// rev
/**
 * The GdiGetLocalBrush routine retrieves a local client-side brush object.
 *
 * \param Brush Handle to the brush.
 * \return HBRUSH Handle to the local brush.
 */
NTSYSAPI
HBRUSH
NTAPI
GdiGetLocalBrush(
    _In_ HBRUSH Brush
    );

// rev
/**
 * The GdiGetLocalDC routine retrieves a local client-side device context object.
 *
 * \param Hdc Handle to the device context.
 * \return HDC Handle to the local device context.
 */
NTSYSAPI
HDC
NTAPI
GdiGetLocalDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiGetLocalFont routine retrieves a local client-side font object.
 *
 * \param Font Handle to the font.
 * \return HFONT Handle to the local font.
 */
NTSYSAPI
HFONT
NTAPI
GdiGetLocalFont(
    _In_ HFONT Font
    );

// rev
/**
 * GdiGetPageCount
 */
NTSYSAPI
DWORD
NTAPI
GdiGetPageCount(
    _In_ HANDLE SpoolFileHandle
    );

// rev
/**
 * GdiGetPageHandle
 */
NTSYSAPI
HANDLE
NTAPI
GdiGetPageHandle(
    _In_ HANDLE SpoolFileHandle,
    _In_ DWORD PageNumber,
    _Out_opt_ DWORD *PageType
    );

// rev
/**
 * GdiGetSpoolFileHandle
 *
 * DeviceName is passed to CreateDCW; SpoolerName is passed to OpenPrinterW.
 */
NTSYSAPI
HANDLE
NTAPI
GdiGetSpoolFileHandle(
    _In_ PWSTR DeviceName,
    _In_opt_ PDEVMODEW DeviceMode,
    _In_ PWSTR SpoolerName
    );

// rev
/**
 * The GdiGetVariationStoreDelta routine retrieves variation store deltas for OpenType variable fonts.
 *
 * \param Param1 First parameter.
 * \param AxisCount Count of design variation axes.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \param Param6 Sixth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiGetVariationStoreDelta(
    _In_ LONG_PTR Param1,
    _In_ ULONG AxisCount,
    _In_ LONG_PTR Param3,
    _In_ ULONG Param4,
    _In_ USHORT Param5,
    _In_ USHORT Param6
    );

// rev
/**
 * The GdiHandleBeingTracked routine determines whether a GDI object handle is currently tracked.
 *
 * \param Object Handle to the GDI object.
 * \return BOOL TRUE if the handle is tracked, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiHandleBeingTracked(
    _In_ HGDIOBJ Object
    );

// rev
/**
 * The GdiInitSpool routine initializes the GDI print spooler interface.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiInitSpool(
    VOID
    );

// rev
/**
 * The GdiInitializeLanguagePack routine initializes language pack support in GDI.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiInitializeLanguagePack(
    _In_ ULONG Parameter
    );

// rev
/**
 * The GdiIsMetaFileDC routine determines whether a device context is a metafile DC.
 *
 * \param Hdc Handle to the device context.
 * \return BOOL TRUE if the device context is a metafile DC, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiIsMetaFileDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiIsMetaPrintDC routine determines whether a device context is a metafile print DC.
 *
 * \param Hdc Device context handle or identifier.
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiIsMetaPrintDC(
    _In_ LONG Hdc
    );

// rev
/**
 * The GdiIsPlayMetafileDC routine determines whether a device context is currently playing a metafile.
 *
 * \param Hdc Device context handle or identifier.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiIsPlayMetafileDC(
    _In_ LONG Hdc
    );

// rev
/**
 * The GdiIsScreenDC routine determines whether a device context represents a screen display.
 *
 * \param Hdc A handle to the device context.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiIsScreenDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiIsUMPDSandboxingEnabled routine determines whether User-Mode Printer Driver sandboxing is enabled.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiIsUMPDSandboxingEnabled(
    VOID
    );

// rev
/**
 * The GdiLoadType1Fonts routine loads Type 1 PostScript fonts into the GDI font table.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiLoadType1Fonts(
    VOID
    );

// rev
/**
 * The GdiPlayDCScript routine plays a device context script.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiPlayDCScript(
    VOID
    );

// rev
/**
 * The GdiPlayEMF routine plays an enhanced metafile print job to a printer.
 *
 * \param PrinterName Name of the target printer.
 * \param DevMode A pointer to DEVMODE printer configuration.
 * \param DocName Document title string.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiPlayEMF(
    _In_ PCWSTR PrinterName,
    _In_ PVOID DevMode,
    _In_ PCWSTR DocName
    );

// rev
/**
 * The GdiPlayJournal routine plays a journaling record stream.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiPlayJournal(
    VOID
    );

// rev
/**
 * The GdiPlayPageEMF routine plays an EMF page during document printing.
 *
 * \param SpoolFile Handle to the print spool file.
 * \param Emf Handle to the enhanced metafile.
 * \param DocumentRect Pointer to the page document rectangle.
 * \param BorderRect Pointer to the border rectangle.
 * \param ClipRect Pointer to the clip rectangle.
 * \return BOOL TRUE if playback succeeds, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiPlayPageEMF(
    _In_opt_ HANDLE SpoolFile,
    _In_opt_ HANDLE Emf,
    _In_opt_ RECT *DocumentRect,
    _In_opt_ RECT *BorderRect,
    _In_opt_ RECT *ClipRect
    );

// rev
/**
 * The GdiPlayPrivatePageEMF routine plays a private EMF page during spooling.
 *
 * \param SpoolFileHandle A handle to the print spool file.
 * \param MetaFileHandle A handle to the enhanced metafile.
 * \param DocumentRect A pointer to the page document rectangle.
 * \return TRUE if playback succeeds, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
GdiPlayPrivatePageEMF(
    _In_ PVOID SpoolFileHandle,
    _In_ LONG_PTR MetaFileHandle,
    _In_ PRECT DocumentRect
    );

// rev
/**
 * The GdiPlayScript routine plays a GDI rendering script.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiPlayScript(
    VOID
    );

// rev
/**
 * The GdiPrinterThunk routine executes an internal printer driver thunk.
 *
 * \param Buffer A pointer to the I/O buffer.
 * \param BufferSize The size of the buffer in bytes.
 * \param Param3 A pointer receiving returned parameter data.
 * \param Param4 Fourth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiPrinterThunk(
    _Inout_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Inout_ PULONG Param3,
    _In_ ULONG Param4
    );

// rev
/**
 * The GdiProcessSetup routine sets up GDI subsystem state for a newly initialized process.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiProcessSetup(
    VOID
    );

// rev
/**
 * The GdiQueryFonts routine queries installed font families from GDI.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiQueryFonts(
    VOID
    );

// rev
/**
 * The GdiQueryTable routine queries the global GDI handle table address.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiQueryTable(
    VOID
    );

// rev
/**
 * The GdiRealizationInfo routine retrieves font realization information for a font handle.
 *
 * \param FontHandle A handle to the font.
 * \param RealizationInfo A pointer receiving font realization information.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiRealizationInfo(
    _In_ HFONT FontHandle,
    _Inout_ PVOID RealizationInfo
    );

// rev
/**
 * The GdiReleaseDC routine releases a device context and deletes its local DC object.
 *
 * \param Hdc Handle to the device context.
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
GdiReleaseDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The GdiReleaseLocalDC routine releases a local client-side device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiReleaseLocalDC(
    VOID
    );

// rev
NTSYSAPI
BOOL
NTAPI
GdiResetDCEMF(
    _In_ HANDLE SpoolFileHandle,
    _In_opt_ PDEVMODEW DeviceMode
    );

// rev
/**
 * The GdiSetAttrs routine sets internal attributes on a GDI object.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiSetAttrs(
    VOID
    );

// rev
NTSYSAPI
VOID
NTAPI
GdiSetLastError(
    _In_ ULONG ErrorCode
    );

// rev
/**
 * The GdiSetPixelFormat routine sets the pixel format for a device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiSetPixelFormat(
    VOID
    );

// rev
/**
 * The GdiSetServerAttr routine sets server-side attributes for a device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiSetServerAttr(
    VOID
    );

// rev
NTSYSAPI
BOOL
NTAPI
GdiStartDocEMF(
    _In_ HANDLE SpoolFileHandle,
    _In_ DOCINFOW *DocumentInfo
    );

// rev
/**
 * GdiStartPageEMF
 */
NTSYSAPI
BOOL
NTAPI
GdiStartPageEMF(
    _In_ HANDLE SpoolFileHandle
    );

// rev
/**
 * The GdiSupportsFontChangeEvent routine determines whether font change events are supported.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiSupportsFontChangeEvent(
    VOID
    );

// rev
/**
 * The GdiSwapBuffers routine swaps front and back rendering buffers for a device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiSwapBuffers(
    VOID
    );

// rev
/**
 * The GdiTrackHCreate routine tracks creation of a client-side GDI handle.
 *
 * \param Handle The handle value to track.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiTrackHCreate(
    _In_ LONG_PTR Handle
    );

// rev
/**
 * The GdiTrackHDelete routine tracks deletion of a client-side GDI handle.
 *
 * \param Handle A pointer to the handle value being deleted.
 */
NTSYSAPI
VOID
NTAPI
GdiTrackHDelete(
    _In_ PVOID Handle
    );

// rev
/**
 * The GdiValidateHandle routine validates a GDI object handle.
 *
 * \param Handle The handle value to validate.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GdiValidateHandle(
    _In_ LONG_PTR Handle
    );

// rev
/**
 * The GdiWaitForTextReady routine waits for pending text rendering operations to complete.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GdiWaitForTextReady(
    VOID
    );

// rev
/**
 * The GdipIsAssertEnabled routine determines whether GDI+ internal asserts are enabled.
 *
 * \param AssertionId Identifier of the assertion to check.
 * \return LOGICAL Non-zero if enabled, 0 otherwise.
 */
NTSYSAPI
LOGICAL
NTAPI
GdipIsAssertEnabled(
    _In_ ULONG AssertionId
    );

// rev
/**
 * The GditGetCallerTLStorage routine retrieves thread-local storage for GDI tracing.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GditGetCallerTLStorage(
    VOID
    );

// rev
/**
 * The GditPopCallerInfo routine pops caller information from the GDI trace stack.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GditPopCallerInfo(
    VOID
    );

// rev
/**
 * The GditPushCallerInfo routine pushes caller information onto the GDI trace stack.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GditPushCallerInfo(
    VOID
    );

// rev
/**
 * The GetBitmapAttributes routine retrieves attributes for a bitmap object.
 *
 * \param Bitmap Handle to the bitmap object.
 * \return ULONG Bitmap attributes flag.
 */
NTSYSAPI
ULONG
NTAPI
GetBitmapAttributes(
    _In_ HBITMAP Bitmap
    );

// rev
/**
 * The GetBitmapDpiScaleValue routine retrieves the DPI scaling factor for a bitmap.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetBitmapDpiScaleValue(
    VOID
    );

// rev
/**
 * The GetBrushAttributes routine retrieves attributes for a brush handle.
 *
 * \param BrushHandle The brush handle value or index.
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetBrushAttributes(
    _In_ LONG BrushHandle
    );

// rev
/**
 * The GetDCDpiScaleValue routine retrieves the DPI scaling value for a device context.
 *
 * \param Hdc A handle to the device context.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetDCDpiScaleValue(
    _In_ HDC Hdc
    );

// rev
/**
 * The GetEUDCTimeStamp routine retrieves the global EUDC font timestamp.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetEUDCTimeStamp(
    VOID
    );

// rev
/**
 * The GetEUDCTimeStampExW routine retrieves the EUDC timestamp for a specific typeface.
 *
 * \param FaceName Optional typeface name string.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetEUDCTimeStampExW(
    _In_opt_ PCWSTR FaceName
    );

// rev
/**
 * The GetFontAssocStatus routine retrieves the font association status for a device context.
 *
 * \param Hdc A handle to the device context.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetFontAssocStatus(
    _In_ HDC Hdc
    );

// rev
/**
 * The GetFontFileData routine retrieves raw font file data for a font handle.
 *
 * \param FontHandle The font handle.
 * \param Param2 Second parameter.
 * \param Buffer A pointer receiving font file data.
 * \return A handle, size, or result status.
 */
NTSYSAPI
BOOL
NTAPI
GetFontFileData(
    _In_ ULONG FontInstanceId,
    _In_ ULONG FileIndex,
    _In_ ULONGLONG FileOffset,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize
    );

// rev
/**
 * The GetFontFileInfo routine retrieves font file metadata and version information.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param FileInfo A pointer receiving font file information.
 * \param BufferSize The size of the FileInfo buffer.
 * \param BytesNeeded Optional pointer receiving bytes needed.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetFontFileInfo(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2,
    _Inout_ PVOID FileInfo,
    _In_ ULONG_PTR BufferSize,
    _Out_opt_ PULONG64 BytesNeeded
    );

// rev
/**
 * The GetFontRealizationInfo routine retrieves font realization flags and metrics for an HFONT.
 *
 * \param FontHandle A handle to the font.
 * \param RealizationInfo A pointer receiving realization flags and metrics.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetFontRealizationInfo(
    _In_ HFONT FontHandle,
    _Inout_ PULONG RealizationInfo
    );

// rev
/**
 * The GetFontResourceInfoW routine retrieves font resource information for a font file.
 *
 * \param LpFileName Pointer to or handle of the font file name.
 * \param BufferSize A pointer to the buffer size in bytes.
 * \param Buffer Optional pointer receiving font resource information.
 * \param InfoType The type of font resource information requested.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetFontResourceInfoW(
    _In_ ULONG_PTR LpFileName,
    _Inout_ PULONG BufferSize,
    _Out_opt_ PVOID Buffer,
    _In_ LONG InfoType
    );

// rev
/**
 * The GetProcessSessionFonts routine retrieves session-specific fonts loaded for a process.
 */
NTSYSAPI
BOOL
NTAPI
GetProcessSessionFonts(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_opt_(*FontCount * sizeof(HANDLE)) PVOID FontHandleBuffer,
    _Inout_ PULONG FontCount,
    _Out_writes_bytes_opt_(*MetadataElementCount * sizeof(USHORT)) PVOID MetadataBuffer,
    _Inout_ PULONG MetadataElementCount
    );

// rev
/**
 * The GetStringBitmapA routine retrieves glyph bitmap data for an ANSI string.
 *
 * \param Hdc A handle to the device context.
 * \param String A pointer to the ANSI string.
 * \param StringLength Length of the string in characters.
 * \param BufferSize Size of the output buffer in bytes.
 * \param Buffer A pointer receiving glyph bitmap data.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetStringBitmapA(
    _In_ HDC Hdc,
    _In_ PCSTR String,
    _In_ ULONG StringLength,
    _In_ ULONG BufferSize,
    _Out_ PVOID Buffer
    );

// rev
/**
 * The GetStringBitmapW routine retrieves glyph bitmap data for a Unicode string.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetStringBitmapW(
    VOID
    );

// rev
/**
 * The InternalDeleteDC routine internally deletes a GDI device context.
 *
 * \param Hdc A handle to the device context to delete.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
InternalDeleteDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The LoadLocalFonts routine loads the locally installed fonts into the session.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserLW_LoadFonts.
 */
NTSYSAPI
LOGICAL
NTAPI
LoadLocalFonts(
    VOID
    );

// rev
/**
 * The LoadRemoteFonts routine loads remote or End-User Defined Characters (EUDC) fonts into the session.
 */
NTSYSAPI
VOID
NTAPI
LoadRemoteFonts(
    VOID
    );

// rev
/**
 * The LpkDrawTextEx routine renders formatted text using language pack processing.
 *
 * \param Hdc A handle to the device context.
 * \param Param2 Second parameter.
 * \param Y The vertical starting coordinate.
 * \param Param4 Fourth parameter.
 * \param StringLength Length of the string.
 * \param Param6 Sixth parameter.
 * \param Param7 Seventh parameter.
 * \param Param8 Eighth parameter.
 * \param Param9 Ninth parameter.
 * \param Param10 Tenth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkDrawTextEx(
    _In_ HDC Hdc,
    _In_ LONG_PTR Param2,
    _In_ LONG Y,
    _In_ LONG_PTR Param4,
    _In_ LONG StringLength,
    _In_ LONG Param6,
    _In_ LONG Param7,
    _In_ LONG_PTR Param8,
    _In_ LONG Param9,
    _In_ LONG Param10
    );

// rev
/**
 * The ModerncoreDeleteDC routine deletes a moderncore device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ModerncoreDeleteDC(
    VOID
    );

// rev
/**
 * The ModerncoreGdiInit routine initializes GDI moderncore support.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ModerncoreGdiInit(
    VOID
    );

// rev
/**
 * The NtGdiConfigureOPMProtectedOutput routine configures an Output Protection Manager (OPM) protected output object.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param Parameters A pointer to an OPM_CONFIGURE_PROTECTED_OUTPUT_PARAMETERS structure.
 * \param AdditionalParametersSize The size, in bytes, of the AdditionalParameters buffer.
 * \param AdditionalParameters An optional pointer to additional configuration parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiConfigureOPMProtectedOutput(
    _In_ PVOID ProtectedOutput,
    _In_ PVOID Parameters,
    _In_ ULONG AdditionalParametersSize,
    _In_reads_bytes_opt_(AdditionalParametersSize) PVOID AdditionalParameters
    );

// rev
/**
 * The NtGdiCreateOPMProtectedOutput routine creates an Output Protection Manager (OPM) protected output object for a display adapter.
 *
 * \param Parameters A pointer to output creation parameters.
 * \param ProtectedOutput An output pointer receiving the protected output object handle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiCreateOPMProtectedOutput(
    _In_ PVOID Parameters,
    _Out_ PVOID * ProtectedOutput
    );

// rev
/**
 * The NtGdiCreateOPMProtectedOutputs routine creates an array of Output Protection Manager (OPM) protected output objects.
 *
 * \param Param1 Device context or display adapter context parameter.
 * \param Param2 Output creation options or flags.
 * \param ArraySize The number of elements in the output array.
 * \param Param4 In-out parameter pointer.
 * \param ProtectedOutputArray A pointer to an array receiving protected output handles.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiCreateOPMProtectedOutputs(
    _In_ PVOID Param1,
    _In_ LONG_PTR Param2,
    _In_ LONG ArraySize,
    _Inout_ PVOID Param4,
    _Out_ PVOID ProtectedOutputArray
    );

// rev
/**
 * The NtGdiDDCCIGetCapabilitiesString routine retrieves a DDC/CI capabilities string describing a physical monitor.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param String A pointer to a buffer that receives the capabilities string.
 * \param StringLength The length, in characters, of the String buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCIGetCapabilitiesString(
    _In_ HANDLE PhysicalMonitor,
    _Out_writes_(StringLength) PSTR String,
    _In_ ULONG StringLength
    );

// rev
/**
 * The NtGdiDDCCIGetCapabilitiesStringLength routine retrieves the length in characters of a physical monitor's DDC/CI capabilities string.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param StringLength A pointer to a variable that receives the length of the string in characters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCIGetCapabilitiesStringLength(
    _In_ HANDLE PhysicalMonitor,
    _Out_ PULONG StringLength
    );

// rev
/**
 * The NtGdiDDCCIGetTimingReport routine retrieves horizontal and vertical synchronization timing metrics from a monitor via DDC/CI.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param TimingReport A pointer to an MC_TIMING_REPORT structure receiving the timing metrics.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCIGetTimingReport(
    _In_ HANDLE PhysicalMonitor,
    _Out_ PVOID TimingReport
    );

// rev
/**
 * The NtGdiDDCCIGetVCPFeature routine retrieves the current and maximum values of a Virtual Control Panel (VCP) feature for a physical monitor.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param VCPCode The VCP code to query.
 * \param VCPCodeType A pointer to a variable that receives the VCP code type (MC_MOMENTARY or MC_SET_PARAMETER).
 * \param CurrentValue A pointer to a variable that receives the current value.
 * \param MaximumValue A pointer to a variable that receives the maximum value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCIGetVCPFeature(
    _In_ HANDLE PhysicalMonitor,
    _In_ ULONG VCPCode,
    _Out_ PULONG VCPCodeType,
    _Out_ PULONG CurrentValue,
    _Out_ PULONG MaximumValue
    );

// rev
/**
 * The NtGdiDDCCISaveCurrentSettings routine saves current monitor settings and VCP values to non-volatile monitor storage via DDC/CI.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCISaveCurrentSettings(
    _In_ HANDLE PhysicalMonitor
    );

// rev
/**
 * The NtGdiDDCCISetVCPFeature routine sets the value of a Virtual Control Panel (VCP) code for a physical monitor via DDC/CI.
 *
 * \param PhysicalMonitor A handle to the physical monitor.
 * \param VCPCode The VCP code to set.
 * \param Value The value to assign to the VCP code.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDDCCISetVCPFeature(
    _In_ HANDLE PhysicalMonitor,
    _In_ ULONG VCPCode,
    _In_ ULONG Value
    );

// rev
/**
 * The NtGdiDestroyOPMProtectedOutput routine destroys an Output Protection Manager (OPM) protected output object.
 *
 * \param ProtectedOutput A pointer to the protected output object to destroy.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDestroyOPMProtectedOutput(
    _In_ PVOID ProtectedOutput
    );

// rev
/**
 * The NtGdiFullscreenControl routine controls full-screen display mode configuration and exclusivity states.
 *
 * \param Param1 Fullscreen control operation code.
 * \param Param2 Operation parameter.
 * \param Param3 Operation parameter.
 * \param Param4 Operation parameter.
 * \param Param5 Operation parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: this export folds onto a shared/stub address in the binary, so neither the
// argument count nor the types below could be confirmed by disassembly.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiFullscreenControl(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2,
    _In_ ULONG_PTR Param3,
    _In_ ULONG_PTR Param4,
    _In_ ULONG_PTR Param5
    );

// rev
/**
 * The NtGdiGetAndSetDCDword routine retrieves and sets an internal device context DWORD attribute.
 *
 * \param Hdc A handle to the device context.
 * \param Command The DC attribute command or index to query and modify.
 * \param Value The new value to set for the DC attribute.
 * \param PreviousValue An output pointer receiving the previous attribute value.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtGdiGetAndSetDCDword(
    _In_ HDC Hdc,
    _In_ LONG Command,
    _In_ LONG Value,
    _Out_ PULONG PreviousValue
    );

// rev
/**
 * The NtGdiGetCOPPCompatibleOPMInformation routine retrieves COPP-compatible output protection information from an OPM protected output.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param Parameters A pointer to an OPM_COPP_COMPATIBLE_GET_INFO_PARAMETERS structure.
 * \param RequestedInformation A pointer to an OPM_REQUESTED_INFORMATION structure receiving the data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetCOPPCompatibleOPMInformation(
    _In_ PVOID ProtectedOutput,
    _In_ PVOID Parameters,
    _Out_ PVOID RequestedInformation
    );

// rev
/**
 * The NtGdiGetCertificate routine retrieves the display adapter's Output Protection Manager (OPM) certificate for cryptographic validation.
 *
 * \param Hdc A handle to the device context.
 * \param CertificateType The type of certificate to retrieve.
 * \param Certificate A buffer that receives the certificate bytes.
 * \param CertificateLength The size, in bytes, of the Certificate buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetCertificate(
    _In_ HDC Hdc,
    _In_ ULONG CertificateType,
    _Out_writes_bytes_(CertificateLength) PVOID Certificate,
    _In_ ULONG CertificateLength
    );

// rev
/**
 * The NtGdiGetCertificateByHandle routine retrieves an OPM certificate from a protected output object handle.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param CertificateType The type of certificate to retrieve.
 * \param Certificate A buffer that receives the certificate bytes.
 * \param CertificateLength The size, in bytes, of the Certificate buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetCertificateByHandle(
    _In_ PVOID ProtectedOutput,
    _In_ ULONG_PTR CertificateType,
    _Out_writes_bytes_(CertificateLength) PVOID Certificate,
    _In_ ULONG CertificateLength
    );

// rev
/**
 * The NtGdiGetCertificateSize routine retrieves the length in bytes of the display driver's OPM certificate.
 *
 * \param Hdc A handle to the device context.
 * \param CertificateType The type of certificate whose size is queried.
 * \param CertificateLength An output pointer receiving the certificate size in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetCertificateSize(
    _In_ HDC Hdc,
    _In_ ULONG CertificateType,
    _Out_ PULONG CertificateLength
    );

// rev
/**
 * The NtGdiGetCertificateSizeByHandle routine retrieves the size in bytes of the OPM certificate from a protected output object handle.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param CertificateType The type of certificate whose size is queried.
 * \param CertificateLength An output pointer receiving the certificate size in bytes.
 * \return LONG value.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetCertificateSizeByHandle(
    _In_ PVOID ProtectedOutput,
    _In_ ULONG_PTR CertificateType,
    _Out_ PULONG CertificateLength
    );

// rev
/**
 * The NtGdiGetDCDword routine queries an internal DWORD property or metric associated with a device context.
 *
 * \param Hdc A handle to the device context.
 * \param Command The DC attribute command or index to query.
 * \param Result An output pointer receiving the attribute value.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtGdiGetDCDword(
    _In_ HDC Hdc,
    _In_ LONG Command,
    _Out_ PULONG Result
    );

// rev
/**
 * The NtGdiGetOPMInformation routine retrieves output protection status and security information from an OPM protected output.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param Parameters Input parameters for the information query.
 * \param RequestedInformation A pointer to an OPM_REQUESTED_INFORMATION structure receiving the data.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetOPMInformation(
    _In_ PVOID ProtectedOutput,
    _In_ LONG_PTR Parameters,
    _Out_ PVOID RequestedInformation
    );

// rev
/**
 * The NtGdiGetOPMRandomNumber routine retrieves a cryptographically secure 128-bit random number from an OPM protected output.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param RandomNumber A pointer to a 16-byte buffer that receives the 128-bit random number.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetOPMRandomNumber(
    _In_ PVOID ProtectedOutput,
    _Out_ PVOID RandomNumber
    );

// rev
/**
 * The NtGdiGetSuggestedOPMProtectedOutputArraySize routine queries the suggested array size for allocating protected output objects for a device context.
 *
 * \param Hdc A handle to the device context.
 * \param ArraySize An output pointer receiving the suggested array size.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetSuggestedOPMProtectedOutputArraySize(
    _In_ HDC Hdc,
    _Out_ PULONG ArraySize
    );

// rev
/**
 * The NtGdiInitSpool routine initializes print spooling communication and data queues in the GDI print subsystem.
 *
 * \param Param1 Spool initialization parameter or queue identifier.
 * \param Param2 Additional spool configuration flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtGdiInitSpool(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// rev
/**
 * The NtGdiSetOPMSigningKeyAndSequenceNumbers routine sets the Output Protection Manager (OPM) AES signing key and sequence numbers for secure communications.
 *
 * \param ProtectedOutput A pointer to the protected output object.
 * \param Parameters A pointer to an OPM_SET_SIGNING_KEY_AND_SEQUENCE_NUMBERS structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiSetOPMSigningKeyAndSequenceNumbers(
    _In_ PVOID ProtectedOutput,
    _In_ LONG_PTR Parameters
    );

/**
 * The NtUserBeginPaint routine prepares the specified window for painting and fills a PAINTSTRUCT structure.
 *
 * \param WindowHandle Handle to the window to prepare for painting.
 * \param lpPaint Pointer to the PAINTSTRUCT that receives painting information.
 * \return The handle to the display device context for the window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDC
NTAPI
NtUserBeginPaint(
    _In_ HWND WindowHandle,
    _Out_ LPPAINTSTRUCT lpPaint
    );

/**
 * The NtUserEndPaint routine marks the end of painting in the specified window.
 *
 * \param WindowHandle A handle to the window that has been repainted.
 * \param lpPaint Pointer to a PAINTSTRUCT structure that contains painting information retrieved by NtUserBeginPaint.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserEndPaint(
    _In_ HWND WindowHandle,
    _In_ const PAINTSTRUCT* lpPaint
    );

// rev
/**
 * The NtUserEnsureOemBitmapInfoForDpi routine ensures that OEM/system bitmaps are cached and sized for the given DPI.
 *
 * \param Dpi Dots per inch (DPI) value to ensure OEM bitmaps for.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnsureOemBitmapInfoForDpi(
    _In_ USHORT Dpi
    );

// rev
/**
 * The NtUserGetControlBrush routine retrieves a brush used to paint standard control backgrounds.
 *
 * \param WindowHandle Handle to the control window.
 * \param Hdc Handle to the device context used to paint the control.
 * \param Message Color notification message identifier (e.g. WM_CTLCOLORSTATIC, WM_CTLCOLORBTN).
 * \return HBRUSH Handle to the background brush, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HBRUSH
NTAPI
NtUserGetControlBrush(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ ULONG Message
    );

// rev
/**
 * The NtUserGetDC routine retrieves a device context (DC) for the client area of a specified window or for the entire screen.
 *
 * \param WindowHandle Optional handle to the window whose DC is to be retrieved, or NULL for the screen DC.
 * \return HDC Handle to the device context for the window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDC
NTAPI
NtUserGetDC(
    _In_opt_ HWND WindowHandle
    );

/**
 * The NtUserGetDCEx routine retrieves a handle to a device context (DC) for the client area of a specified window or for the entire screen.
 *
 * \param WindowHandle A handle to the window whose DC is to be retrieved.
 * \param hrgnClip A clipping region that may be combined with the visible region of the DC.
 * \param flags Device context retrieval flags (e.g. DCX_WINDOW, DCX_CACHE, DCX_CLIPCHILDREN).
 * \return A handle to the device context for the specified window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDC
NTAPI
NtUserGetDCEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ HRGN hrgnClip,
    _In_ ULONG flags
    );

// rev
/**
 * The NtUserGetOemBitmapSize routine retrieves the pixel dimensions of an OEM/system bitmap resource.
 *
 * \param OemBitmapIndex Index of the OEM bitmap (OBM_*).
 * \param Size Pointer to a SIZE structure receiving the bitmap dimensions.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetOemBitmapSize(
    _In_ ULONG OemBitmapIndex,
    _Out_ PVOID Size
    );

/**
 * The NtUserGetWindowDC routine retrieves the device context (DC) for the entire window, including title bar, menus, and scroll bars.
 *
 * \param WindowHandle A handle to the window whose display context is to be retrieved.
 * \return A handle to a device context for the specified window, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HDC
NTAPI
NtUserGetWindowDC(
    _In_opt_ HWND WindowHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserLW_LoadFonts routine loads the logon/window fonts.
 *
 * \param Remote Nonzero to load remote/session fonts; zero for local logon fonts.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_LW_LOADFONTS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLW_LoadFonts(
    _In_ LOGICAL Remote
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterLPK routine registers the language pack (LPK) entry points.
 *
 * \param LpkEntryPoints The lpk entry points (LPK_FLAG_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_REGISTERLPK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterLPK(
    _In_ ULONG LpkEntryPoints // LPK_FLAG_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserReleaseDC routine releases a device context (DC), freeing it for use by other applications.
 *
 * \param Hdc A handle to the device context to be released.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReleaseDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The NtUserScrollDC routine scrolls a rectangle of bits horizontally and vertically in the specified device context.
 *
 * \param Hdc A handle to the device context that contains the bits to be scrolled.
 * \param Dx The amount, in device units, of horizontal scrolling.
 * \param Dy The amount, in device units, of vertical scrolling.
 * \param ScrollRectangle An optional pointer to a RECT structure containing the coordinates of the scrolling rectangle.
 * \param ClipRectangle An optional pointer to a RECT structure containing the coordinates of the clipping rectangle.
 * \param UpdateRegion An optional handle to the region uncovered by the scrolling process.
 * \param UpdateRectangle An optional pointer to a RECT structure receiving the bounding rectangle of the update region.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserScrollDC(
    _In_ HDC Hdc,
    _In_ LONG Dx,
    _In_ LONG Dy,
    _In_opt_ PRECT ScrollRectangle,
    _In_opt_ PRECT ClipRectangle,
    _In_opt_ HRGN UpdateRegion,
    _Out_opt_ PRECT UpdateRectangle
    );

/**
 * The NtUserWindowFromDC routine returns a handle to the window associated with the specified display device context (DC).
 *
 * \param Hdc A handle to the device context from which the associated window handle is to be retrieved.
 * \return A handle to the window associated with the specified DC, or NULL if no window is associated.
 */
_Kernel_entry_
NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromDC(
    _In_ HDC Hdc
    );

// rev
/**
 * The QueryFontAssocStatus routine queries the font association status flags.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
QueryFontAssocStatus(
    VOID
    );

// rev
/**
 * The RemoveFontResourceTracking routine removes font resource tracking for a font file.
 *
 * \param MultiByteString A pointer to the font path string.
 * \param Flags Tracking flags.
 * \return TRUE if tracking was removed successfully, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RemoveFontResourceTracking(
    _Inout_ PSTR MultiByteString,
    _In_ ULONG Flags
    );

// rev
/**
 * The SelectBrushLocal routine selects a local brush into a device context.
 *
 * \param Hdc Handle to the device context.
 * \param Brush Handle to the brush.
 * \return HBRUSH Handle to the previously selected brush, or NULL on failure.
 */
NTSYSAPI
HBRUSH
NTAPI
SelectBrushLocal(
    _In_ HDC Hdc,
    _In_ HBRUSH Brush
    );

// rev
/**
 * The SelectFontLocal routine selects a local font into a device context.
 *
 * \param Hdc Handle to the device context.
 * \param Font Handle to the font.
 * \return HFONT Handle to the previously selected font, or NULL on failure.
 */
NTSYSAPI
HFONT
NTAPI
SelectFontLocal(
    _In_ HDC Hdc,
    _In_ HFONT Font
    );

// rev
/**
 * The SetBitmapAttributes routine sets attributes on a bitmap object.
 *
 * \param BitmapHandle A handle to the bitmap.
 * \param Flags Bitmap attribute flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
SetBitmapAttributes(
    _In_ HBITMAP BitmapHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The SetBrushAttributes routine sets attributes on a brush object.
 *
 * \param BrushHandle A handle to the brush.
 * \param Flags Brush attribute flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
SetBrushAttributes(
    _In_ HBRUSH BrushHandle,
    _In_ LONG Flags
    );

// rev
/**
 * The SetFontEnumeration routine configures font enumeration filtering modes.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetFontEnumeration(
    VOID
    );

// rev
/**
 * The UnloadNetworkFonts routine unloads network-installed fonts.
 *
 * \param Flags Font unloading flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
UnloadNetworkFonts(
    _In_ LONG_PTR Flags
    );

// rev
/**
 * The bCreateDCW routine creates a GDI device context from Unicode parameters.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
bCreateDCW(
    VOID
    );

// rev
/**
 * The bDeleteLDC routine deletes a local device context object.
 *
 * \param LocalDc Pointer or handle to the local device context.
 * \return BOOL TRUE if deletion succeeds, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
bDeleteLDC(
    _In_ PVOID LocalDc
    );

// rev
/**
 * The bInitSystemAndFontsDirectoriesW routine initializes system and font directory path pointers.
 *
 * \param SystemDirectory A pointer receiving the system directory path string.
 * \param FontsDirectory A pointer receiving the fonts directory path string.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
bInitSystemAndFontsDirectoriesW(
    _Inout_ PVOID * SystemDirectory,
    _Inout_ PVOID * FontsDirectory
    );

// rev
/**
 * gMaxGdiHandleCount is a gdi32.dll DATA export, not a function.
 * 32-bit handle limit, compared with EAX at gdi32!0x180001118.
 */
NTSYSAPI extern ULONG gMaxGdiHandleCount;

// rev
/**
 * pGdiDevCaps is a gdi32.dll DATA export, not a function.
 * Pointer to device capability data (gdi32!0x1800019d8); pointee layout opaque.
 */
NTSYSAPI extern PVOID pGdiDevCaps;

// rev
/**
 * pGdiSharedHandleTable is a gdi32.dll DATA export, not a function.
 * Pointer to the shared handle table (gdi32!0x180001010); pointee layout opaque.
 */
NTSYSAPI extern PVOID pGdiSharedHandleTable;

// rev
/**
 * pGdiSharedMemory is a gdi32.dll DATA export, not a function.
 * Pointer to GDI shared memory (gdi32!0x1800045f0); pointee layout opaque.
 */
NTSYSAPI extern PVOID pGdiSharedMemory;

//
// System Parameters, Session & Process User State
//

/**
 * [KERNEL-VALIDATED against win32kbase.sys / win32kfull.sys, Win11 build]
 * Public user32 enum: user32!GetProcessUIContextInformation ->
 * win32k syscall NtUserGetProcessUIContextInformation (string @ win32kbase 0x140262030;
 * referenced from the retail service-name tables). Fills PROCESS_UICONTEXT_INFORMATION
 * { PROCESS_UICONTEXT ProcessUIContext; ULONG dwFlags; }.
 *
 * The UI-context / immersive-type discriminator is stored in the process flags QWORD at
 * tagPROCESSINFO+0x330, bits 4-5 (mask 0x30). Setter: SetProcessType @ win32kbase
 * 0x1401A11F0 writes (immersiveType << 4) into ppi+0x330 (RMW mask 0xCF). Readers cross-
 * confirm the numeric ordering:
 *   (ppi+0x330 & 0x30) == 0x00 -> DESKTOP           [PARTIAL - implied 0, no dedicated reader]
 *   (ppi+0x330 & 0x30) == 0x10 -> IMMERSIVE (app)   [CONFIRMED - IsImmersiveAppRestricted @0x1400D8720 tests ==0x10]
 *   (ppi+0x330 & 0x30) == 0x20 -> IMMERSIVE_BROKER  [CONFIRMED - IsImmersiveBroker        @0x1400C0B00 tests ==0x20]
 *   IMMERSIVE_BROWSER = 3       -> UICONTEXT-only 4th value; not encoded in these 2 type
 *                                  bits (SetProcessType's _PROCESS_IMMERSIVE_TYPE callout
 *                                  asserts type>=3 invalid, handling only 0/1/2). [UNVERIFIED - value trivially sequential]
 * NOTE: tagPROCESSINFO is a PDB forward-decl (no named bitfields) so bit-NAMES are not
 * symbol-confirmed; the numeric enum values are behaviorally supported as above.
 */
typedef enum _PROCESS_UICONTEXT
{
    PROCESS_UICONTEXT_DESKTOP,           // 0 [PARTIAL]   - (ppi+0x330 & 0x30)==0x00
    PROCESS_UICONTEXT_IMMERSIVE,         // 1 [CONFIRMED] - ==0x10 (IsImmersiveAppRestricted @0x1400D8720)
    PROCESS_UICONTEXT_IMMERSIVE_BROKER,  // 2 [CONFIRMED] - ==0x20 (IsImmersiveBroker @0x1400C0B00)
    PROCESS_UICONTEXT_IMMERSIVE_BROWSER  // 3 [UNVERIFIED] - public 4th value, not seen written to type bits
} PROCESS_UICONTEXT;

/**
 * [KERNEL-VALIDATED] dwFlags field of PROCESS_UICONTEXT_INFORMATION.
 *   PROCESS_UIF_AUTHORING_MODE (0x1): SetProcessType @0x1401A11F0 checks the
 *     WIN://DESIGN_MODE security attribute (SeSecurityAttributePresent) and records the
 *     result as bit 0x2000 in ppi+0x330. "Design mode" == authoring mode -> semantic CONFIRMED.
 *   PROCESS_UIF_RESTRICTIONS_DISABLED (0x2): no dedicated stored flag bit observed; immersive
 *     UI restriction is gated at runtime via ExemptedFromImmersiveRestrictions @win32kfull
 *     0x54051D3AC / IAMThreadAccessGranted rather than a persisted flag. [PARTIAL]
 */
typedef enum _PROCESS_UI_FLAGS
{
    PROCESS_UIF_NONE,                    // 0
    PROCESS_UIF_AUTHORING_MODE,          // 1 [CONFIRMED] - WIN://DESIGN_MODE attr -> ppi+0x330 bit 0x2000
    PROCESS_UIF_RESTRICTIONS_DISABLED    // 2 [PARTIAL]   - runtime-gated, no persisted flag bit found
} PROCESS_UI_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(PROCESS_UI_FLAGS);

/**
 * Output buffer for NtUserGetProcessUIContextInformation: the process's immersive
 * UI-context class and associated UI flags.
 */
typedef struct _PROCESS_UICONTEXT_INFORMATION
{
    PROCESS_UICONTEXT ProcessUIContext;  // Immersive UI-context discriminator (PROCESS_UICONTEXT_*).
    PROCESS_UI_FLAGS Flags;              // UI flags (PROCESS_UIF_*).
} PROCESS_UICONTEXT_INFORMATION, *PPROCESS_UICONTEXT_INFORMATION;

/**
 * Input/output buffer for the Input Access Manager (IAM) key APIs: carries the
 * per-desktop IAM access key exchanged by NtUserAcquireIAMKey / NtUserEnableIAMAccess.
 * \remarks Reverse-engineered.
 */
typedef struct _IAM_ACCESS_KEY_INPUT
{
    ULONG_PTR IamDesktopKey;    // Opaque per-desktop IAM access key.
} IAM_ACCESS_KEY_INPUT, *PIAM_ACCESS_KEY_INPUT;

/**
 * Valid bit masks enforced by NtUserSetProcessWin32Capabilities for each
 * USER_PROCESS_CAP_ENTRY lane.
 * \remarks Reverse-engineered.
 */
#define PROC_CAP_FLAGS1_VALID_MASK     0x00000007u    // bits 0-2
#define PROC_CAP_FLAGS2_VALID_MASK     0x00000007u    // bits 0-2
#define PROC_CAP_ENABLE_VALID_MASK     0x00000001u    // bit 0
#define PROC_CAP_DISABLE_VALID_MASK    0x00000001u    // bit 0

/**
 * Capability values accepted by Win32ProcessCapability::CheckAccess.
 * \remarks Reverse-engineered.
 */
#define PROC_CAP_VALUE_UNKNOWN0                0x00000001u // accepted; no stable callsite name yet
#define PROC_CAP_VALUE_CAPTURE_SURFACE         0x00000002u // PrintWindow and magnifier capture paths
#define PROC_CAP_VALUE_ZBID_SYSTEM_BAND        0x00000004u // band validation for privileged/system bands

/**
 * Lane-specific aliases (PROC_CAP_FLAGS1_* / PROC_CAP_FLAGS2_*) mapping the accepted
 * capability values onto the NtUserSetProcessWin32Capabilities request lanes.
 * \remarks Reverse-engineered.
 */
#define PROC_CAP_FLAGS1_UNKNOWN0               PROC_CAP_VALUE_UNKNOWN0
#define PROC_CAP_FLAGS1_CAPTURE_SURFACE        PROC_CAP_VALUE_CAPTURE_SURFACE
#define PROC_CAP_FLAGS1_ZBID_SYSTEM_BAND       PROC_CAP_VALUE_ZBID_SYSTEM_BAND

#define PROC_CAP_FLAGS2_UNKNOWN0               PROC_CAP_VALUE_UNKNOWN0
#define PROC_CAP_FLAGS2_CAPTURE_SURFACE        PROC_CAP_VALUE_CAPTURE_SURFACE
#define PROC_CAP_FLAGS2_ZBID_SYSTEM_BAND       PROC_CAP_VALUE_ZBID_SYSTEM_BAND

#define PROC_CAP_ENABLE_APPLY_BIT0             0x00000001u
#define PROC_CAP_DISABLE_APPLY_BIT0            0x00000001u

#define PROC_CAP_FLAGS1_INVALID(x)     (((x) & ~PROC_CAP_FLAGS1_VALID_MASK) != 0)
#define PROC_CAP_FLAGS2_INVALID(x)     (((x) & ~PROC_CAP_FLAGS2_VALID_MASK) != 0)
#define PROC_CAP_ENABLE_INVALID(x)     (((x) & ~PROC_CAP_ENABLE_VALID_MASK) != 0)
#define PROC_CAP_DISABLE_INVALID(x)    (((x) & ~PROC_CAP_DISABLE_VALID_MASK) != 0)

/**
 * One capability request passed to NtUserSetProcessWin32Capabilities. Selects a
 * target process and the win32k capability lanes to grant or revoke.
 * \remarks Reverse-engineered. Field masks are validated against PROC_CAP_*_VALID_MASK.
 */
typedef struct _USER_PROCESS_CAP_ENTRY
{
    HANDLE ProcessHandle;   // Target process.
    ULONG Flags1;           // Capability lane 1 values (PROC_CAP_FLAGS1_*); PROC_CAP_FLAGS1_VALID_MASK.
    ULONG Flags2;           // Capability lane 2 values (PROC_CAP_FLAGS2_*); PROC_CAP_FLAGS2_VALID_MASK.
    ULONG EnableMask;       // Capabilities to enable (PROC_CAP_ENABLE_VALID_MASK).
    ULONG DisableMask;      // Capabilities to disable (PROC_CAP_DISABLE_VALID_MASK).
} USER_PROCESS_CAP_ENTRY, *PUSER_PROCESS_CAP_ENTRY;

#ifndef GR_GDIOBJECTS
#define GR_GDIOBJECTS 0
#endif
#ifndef GR_USEROBJECTS
#define GR_USEROBJECTS 1
#endif
#ifndef GR_GDIOBJECTS_PEAK
#define GR_GDIOBJECTS_PEAK 2
#endif
#ifndef GR_USEROBJECTS_PEAK
#define GR_USEROBJECTS_PEAK 4
#endif

/**
 * The USERTHREAD_DESKTOP_CONTEXT structure pairs a referenced desktop object with its opened handle.
 * Used by the UserThreadSetCsrssDesktop family of USERTHREADINFOCLASS operations.
 */
typedef struct _USERTHREAD_DESKTOP_CONTEXT
{
    PVOID DesktopObject;    // A pointer to the referenced desktop object.
    HANDLE DesktopHandle;   // The opened handle for the desktop.
} USERTHREAD_DESKTOP_CONTEXT, *PUSERTHREAD_DESKTOP_CONTEXT;

/**
 * The USERTHREAD_CSRSS_DESKTOP_INFO structure returns the CSRSS thread desktop state.
 * Queried via the UserThreadCsrssDesktopInfo class of USERTHREADINFOCLASS.
 */
typedef struct _USERTHREAD_CSRSS_DESKTOP_INFO
{
    ULONG_PTR InputValue;                       // A window-like value returned from the desktop state query path.
    ULONG DesktopState;                         // The desktop state (values 0, 1, or 2 observed).
    ULONG Flags;                                // Flags: bit 0x1 is output; bit 0x800 is input.
    USERTHREAD_DESKTOP_CONTEXT DesktopContext;  // The desktop context used with xxxRestore/xxxSetCsrssThreadDesktop.
} USERTHREAD_CSRSS_DESKTOP_INFO, *PUSERTHREAD_CSRSS_DESKTOP_INFO;

/**
 * The USERTHREAD_RESTORE_DESKTOP_INFO structure supplies the desktop context to restore.
 * Passed with the UserThreadRestoreCsrssDesktop class of USERTHREADINFOCLASS.
 */
typedef struct _USERTHREAD_RESTORE_DESKTOP_INFO
{
    USERTHREAD_DESKTOP_CONTEXT DesktopContext;  // The desktop context to restore.
    ULONG Flags;                                // Optional flags; read only when the input size is 0x20.
    ULONG Reserved;                             // Reserved.
} USERTHREAD_RESTORE_DESKTOP_INFO, *PUSERTHREAD_RESTORE_DESKTOP_INFO;

/**
 * The USERTHREADINFOCLASS enumeration selects the thread information queried or set via the win32k user-thread information calls.
 */
typedef enum _USERTHREADINFOCLASS
{
    // Validated against win32kfull xxxQueryInformationThread (0x5403182F4) and xxxSetInformationThread (0x5403CAE20).
    // Query handles classes {0,1,2,4,11}; Set handles {1,5,6,7,8,9,10,12,13,14,15,16,17}. Unhandled classes -> STATUS_INVALID_INFO_CLASS (0xC0000003).
    UserThreadCsrssDesktopInfo = 0,                         // q: USERTHREAD_CSRSS_DESKTOP_INFO
    UserThreadFlags = 1,                                    // q: ULONG; s: ULONGLONG (len 8; only flag bit 0x20000 is settable)
    UserThreadTaskName = 2,                                 // q: PWSTR (UTF-16); s: not implemented -> STATUS_INVALID_INFO_CLASS
    UserThreadInformation3 = 3,                             // not implemented (q and s) -> STATUS_INVALID_INFO_CLASS
    UserThreadHungStatus = 4,                               // q: ULONG (in: timeout ms; out: ULONG boolean)
    UserThreadInitiateShutdown = 5,                         // s: ULONG (shutdown flags)
    UserThreadSetShutdownDesktop = 6,                       // s: ULONG (EndShutdown(*buf); value is NOT ignored)
    UserThreadSetCsrssDesktop = 7,                          // s: USERTHREAD_DESKTOP_CONTEXT
    UserThreadSetCsrssDesktopFromThread = 8,                // s: HANDLE (thread handle; THREAD_QUERY)
    UserThreadRestoreCsrssDesktop = 9,                      // s: USERTHREAD_RESTORE_DESKTOP_INFO
    UserThreadSetCsrApiPort = 10,                           // s: HANDLE (CSR API port)
    UserThreadShutdownThreadList = 11,                      // q: HANDLE[] (session shutdown thread list; len >= 8*(count+1))
    UserThreadSetShutdownWindow = 12,                       // s: HWND (session shutdown window)
    UserThreadQueueShutdownRequest = 13,                    // s: ULONG_PTR
    UserThreadClearShutdownRequest = 14,                    // s: ULONG_PTR
    UserThreadSetConvertibleState = 15,                     // s: ULONG (toggles session state bit 0x8)
    UserThreadSetDockState = 16,                            // s: ULONG (toggles session state bit 0x10)
    UserThreadRefreshShellState = 17,                       // s: ignored (sequences classes 7 -> shell hook -> 9)
} USERTHREADINFOCLASS, *PUSERTHREADINFOCLASS;

/**
 * Remote-session Win32 connection states returned by NtUserRemoteConnectState.
 */
#define CTX_W32_CONNECT_STATE_CONSOLE           0
#define CTX_W32_CONNECT_STATE_IDLE              1
#define CTX_W32_CONNECT_STATE_EXIT_IN_PROGRESS  2
#define CTX_W32_CONNECT_STATE_CONNECTED         3
#define CTX_W32_CONNECT_STATE_DISCONNECTED      4

/**
 * System event sound identifiers for NtUserPlayEventSound.
 */
#define USER_SOUND_DEFAULT              0
#define USER_SOUND_SYSTEMHAND           1
#define USER_SOUND_SYSTEMQUESTION       2
#define USER_SOUND_SYSTEMEXCLAMATION    3
#define USER_SOUND_SYSTEMASTERISK       4
#define USER_SOUND_MENUPOPUP            5
#define USER_SOUND_MENUCOMMAND          6
#define USER_SOUND_OPEN                 7
#define USER_SOUND_CLOSE                8
#define USER_SOUND_RESTOREUP            9
#define USER_SOUND_RESTOREDOWN          10
#define USER_SOUND_MINIMIZE             11
#define USER_SOUND_MAXIMIZE             12
#define USER_SOUND_SNAPSHOT             13

// rev
/**
 * The AddVisualIdentifier routine adds a visual identifier used to map injected pointer input to a surface.
 *
 * \param CompositionInputSink Handle to the composition input sink.
 * \param Luid Pointer to the locally unique identifier (LUID).
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserAddVisualIdentifier system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
AddVisualIdentifier(
    _In_ HANDLE CompositionInputSink,
    _In_ PLUID Luid
    );

// rev
/**
 * The AlignRects routine aligns an array of rectangles.
 *
 * \param Rects Pointer to an array of RECT structures to align.
 * \param Count Number of rectangles in the array.
 * \param Index Base rectangle index used for alignment.
 * \param Flags Alignment formatting flags.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
AlignRects(
    _Inout_updates_(Count) PRECT Rects,
    _In_ ULONG Count,
    _In_ ULONG Index,
    _In_ BYTE Flags
    );

// rev
/**
 * The AllowForegroundActivation routine allows the calling process to perform foreground window activation.
 *
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
AllowForegroundActivation(
    VOID
    );

// rev
/**
 * The BuildReasonArray routine constructs an array of valid shutdown reasons configured on the system.
 *
 * \param ReasonData Receives a pointer to the allocated array of reason records.
 * \param Flags Filtering flags controlling which reason categories to include.
 * \param MaximumReasons Maximum number of reason records to enumerate.
 * \return ULONG_PTR Count of reason records built, or 0 on failure.
 * \remarks Free the returned buffer with DestroyReasons.
 */
NTSYSAPI
ULONG_PTR
NTAPI
BuildReasonArray(
    _Out_ PVOID* ReasonData,
    _In_ ULONG Flags,
    _In_ ULONG MaximumReasons
    );

// rev
/**
 * The CheckBannedOneCoreTransformApi routine rejects APIs that are banned while OneCore transform mode is active.
 *
 * \return BOOL TRUE if the API is allowed, FALSE if banned.
 */
NTSYSAPI
BOOL
NTAPI
CheckBannedOneCoreTransformApi(
    VOID
    );

// rev
/**
 * The CheckDBCSEnabledExt routine determines whether DBCS input is enabled in the current session.
 *
 * \return BOOL TRUE if DBCS is enabled, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
CheckDBCSEnabledExt(
    VOID
    );

// rev
/**
 * The CheckProcessSession routine validates the session that a process belongs to.
 *
 * \param ProcessId Client process identifier.
 * \return ULONG_PTR Status code or session validation result.
 * \remarks Forwards to the NtUserCheckProcessSession system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CheckProcessSession(
    _In_ LONG_PTR ProcessId
    );

// rev
/**
 * The CliImmSetHotKey routine sets an Input Method Manager (IMM) hot key from the client side.
 *
 * \param HotKeyId Identifier of the hot key.
 * \param Modifiers Modifier keys (MOD_* flags).
 * \param VirtualKey Virtual key code.
 * \param KeyboardLayout Handle to the target input locale (keyboard layout).
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CliImmSetHotKey(
    _In_ ULONG HotKeyId,
    _In_ ULONG Modifiers,
    _In_ ULONG VirtualKey,
    _In_ HKL KeyboardLayout
    );

// rev
/**
 * The ClientThreadSetup routine performs per-thread client-side user32 initialization.
 *
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
ClientThreadSetup(
    VOID
    );

// rev
/**
 * The ConfigureOPMProtectedOutput routine configures an Output Protection Manager (OPM) protected output.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ConfigureOPMProtectedOutput(
    VOID
    );

// rev
/**
 * The CreateDPIScaledDIBSection routine creates a DPI-scaled device-independent bitmap (DIB) section.
 *
 * \param Width Width of the DIB section in pixels.
 * \param Height Height of the DIB section in pixels.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param BitmapInfo Pointer or descriptor of the BITMAPINFO structure.
 * \param Usage Color table usage.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
CreateDPIScaledDIBSection(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Param3,
    _In_ LONG Param4,
    _In_ LONG_PTR BitmapInfo,
    _In_ LONG Usage
    );

// rev
/**
 * The CreateOPMProtectedOutput routine creates an Output Protection Manager (OPM) protected output context.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CreateOPMProtectedOutput(
    VOID
    );

// rev
/**
 * The CreateOPMProtectedOutputs routine creates multiple Output Protection Manager (OPM) protected output contexts.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
CreateOPMProtectedOutputs(
    VOID
    );

// rev
/**
 * The CreateSessionMappedDIBSection routine creates a session-mapped device-independent bitmap (DIB) section.
 *
 * \param Width Width of the DIB section in pixels.
 * \param Height Height of the DIB section in pixels.
 * \param BitCount Color bit depth per pixel.
 * \param BitmapInfo Pointer or descriptor of the BITMAPINFO structure.
 * \param Usage Color table usage.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
CreateSessionMappedDIBSection(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG BitCount,
    _In_ LONG_PTR BitmapInfo,
    _In_ LONG Usage
    );

// rev
/**
 * The CreateSystemThreads routine creates the user32 system worker threads.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserCreateSystemThreads.
 */
NTSYSAPI
LOGICAL
NTAPI
CreateSystemThreads(
    VOID
    );

// rev
/**
 * The CtxInitUser32 routine initializes user32 for a Terminal Services (context) session.
 *
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
CtxInitUser32(
    VOID
    );

// rev
/**
 * The DdCreateFullscreenSprite routine creates a fullscreen sprite for DirectDraw/GDI rendering.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DdCreateFullscreenSprite(
    VOID
    );

// rev
/**
 * The DdDestroyFullscreenSprite routine destroys a fullscreen sprite object.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DdDestroyFullscreenSprite(
    VOID
    );

// rev
/**
 * The DdNotifyFullscreenSpriteUpdate routine notifies the display driver of an update to a fullscreen sprite.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DdNotifyFullscreenSpriteUpdate(
    VOID
    );

// rev
/**
 * The DdQueryVisRgnUniqueness routine queries the visible region uniqueness serial number.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DdQueryVisRgnUniqueness(
    VOID
    );

// rev
/**
 * The DestroyOPMProtectedOutput routine destroys an Output Protection Manager (OPM) protected output context.
 *
 * \return A pointer-sized status or handle.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DestroyOPMProtectedOutput(
    VOID
    );

// rev
/**
 * The DestroyReasons routine frees a shutdown-reason array built by BuildReasonArray.
 *
 * \param ReasonData Pointer to the reason data pointer to free and clear.
 * \return BOOLEAN TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
DestroyReasons(
    _Inout_ PVOID* ReasonData
    );

// rev
/**
 * The DeviceCapabilitiesExA routine queries extended device capabilities using ANSI character strings.
 *
 * \return An LONG result; operation-specific semantics are not recovered here.
 * \remarks Six-argument A signature recovered from the decorated PDB symbol
 *          in gdi32full.dll 10.0.26100.9444, RVA 0x9fdb0 (ordinal 1012).
 *          The inspected implementation returns -1 without consuming inputs.
 *          Parameter names are placeholders; direction and optionality are
 *          deliberately not asserted without supporting evidence.
 */
NTSYSAPI
LONG
NTAPI
DeviceCapabilitiesExA(
    PCSTR Param1,
    PCSTR Param2,
    PCSTR Param3,
    LONG Param4,
    PCSTR Param5,
    const DEVMODEA *DeviceMode
    );

// rev
/**
 * The DoSoundConnect routine establishes an audio redirection connection for the user session.
 *
 * \param Flags Connection configuration flags for audio redirection.
 * \param SessionContext Context value identifying the audio session.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserDoSoundConnect system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DoSoundConnect(
    _In_ ULONG Flags,
    _In_opt_ PVOID SessionContext
    );

// rev
/**
 * The DoSoundDisconnect routine terminates an audio redirection connection for the user session.
 *
 * \param Flags Disconnect configuration flags.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserDoSoundDisconnect system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DoSoundDisconnect(
    _In_ ULONG Flags
    );

// rev
/**
 * The DrawFrame routine draws a window frame element.
 *
 * \param Hdc Handle to the device context to draw into.
 * \param Rect Pointer to the bounding rectangle of the frame.
 * \param Thickness Width of the frame border in pixels.
 * \param Flags Frame drawing flags (DF_*).
 * \return ULONG_PTR Status code or drawing result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DrawFrame(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ LONG Thickness,
    _In_ LONG Flags
    );

// rev
/**
 * The DwmKernelShutdown routine notifies user32 that the DWM kernel component is shutting down.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 * \remarks Forwards to the NtUserDwmKernelShutdown system call.
 */
NTSYSAPI
BOOL
NTAPI
DwmKernelShutdown(
    VOID
    );

// rev
/**
 * The DwmKernelStartup routine notifies user32 that the DWM kernel component has started.
 *
 * \param StartupContext Optional pointer to the startup context.
 * \param StartupData Optional pointer to startup data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserDwmKernelStartup system call.
 */
NTSYSAPI
LOGICAL
NTAPI
DwmKernelStartup(
    _In_opt_ PVOID StartupContext,
    _In_opt_ PVOID StartupData
    );

// rev
/**
 * The DwmLockScreenUpdates routine locks or unlocks DWM screen updates.
 *
 * \param LockUpdates TRUE to lock screen updates; FALSE to unlock.
 * \return ULONG_PTR Status code.
 * \remarks Thin user32 wrapper over NtUserDwmLockScreenUpdates.
 */
NTSYSAPI
ULONG_PTR
NTAPI
DwmLockScreenUpdates(
    _In_ LOGICAL LockUpdates
    );

// rev
/**
 * The EnableOneCoreTransformMode routine enables or disables OneCore transform mode for the session.
 *
 * \param Enable Non-zero to enable OneCore transform mode; zero to disable.
 * \return BOOL value.
 * \remarks Forwards to the NtEnableOneCoreTransformMode system call.
 */
NTSYSAPI
BOOL
NTAPI
EnableOneCoreTransformMode(
    VOID
    );

// rev
/**
 * The EnableSessionForMMCSS routine enables or disables MMCSS for the current session.
 *
 * \param Enable TRUE to enable MMCSS for the session; FALSE to disable.
 * \return ULONG_PTR Status code or result.
 * \remarks Thin user32 wrapper over NtUserEnableSessionForMMCSS.
 */
NTSYSAPI
ULONG_PTR
NTAPI
EnableSessionForMMCSS(
    _In_ LOGICAL Enable
    );

// rev
/**
 * The EndFormPage routine ends a form page during document printing.
 *
 * \param Hdc Handle to the device context.
 * \return LONG Result code from InternalEndPage.
 */
NTSYSAPI
LONG
NTAPI
EndFormPage(
    _In_ HDC Hdc
    );

// rev
/**
 * The EudcLoadLinkW routine loads an EUDC font link for a typeface.
 *
 * \param FaceName Optional typeface name.
 * \param FontFileName The font file name.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
EudcLoadLinkW(
    _In_opt_ PCWSTR FaceName,
    _In_ PCWSTR FontFileName
    );

// rev
/**
 * The EudcUnloadLinkW routine unloads an EUDC font link for a typeface.
 *
 * \param FaceName Optional typeface name.
 * \param FontFileName The font file name.
 * \return A handle or status code representing the result.
 */
NTSYSAPI
LONG_PTR
NTAPI
EudcUnloadLinkW(
    _In_opt_ PCWSTR FaceName,
    _In_ PCWSTR FontFileName
    );

// rev
/**
 * The FreeDDElParam routine frees a DDE message parameter allocated by PackDDElParam.
 *
 * \param Msg The posted DDE message identifier.
 * \param LParam The message parameter to be freed.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
FreeDDElParam(
    _In_ ULONG Msg,
    _In_ LPARAM LParam
    );

// rev
/**
 * The GetAppCompatFlags routine retrieves the application-compatibility flags for the current process.
 *
 * \return ULONG Compatibility flags (GACF_*).
 */
NTSYSAPI
ULONG
NTAPI
GetAppCompatFlags(
    VOID
    );

// rev
/**
 * The GetAppCompatFlags2 routine retrieves the second set of application compatibility flags for the specified version.
 *
 * \param Version Target Windows subsystem version.
 * \return ULONG Extended compatibility flags.
 */
NTSYSAPI
ULONG
NTAPI
GetAppCompatFlags2(
    _In_ USHORT Version
    );

// rev
/**
 * The GetCOPPCompatibleOPMInformation routine retrieves COPP-compatible Output Protection Manager (OPM) information.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCOPPCompatibleOPMInformation(
    VOID
    );

// rev
/**
 * The GetCertificate routine retrieves an OPM display certificate.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCertificate(
    VOID
    );

// rev
/**
 * The GetCertificateByHandle routine retrieves an OPM display certificate using a protected output handle.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCertificateByHandle(
    VOID
    );

// rev
/**
 * The GetCertificateSize routine retrieves the size of an OPM display certificate.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCertificateSize(
    VOID
    );

// rev
/**
 * The GetCertificateSizeByHandle routine retrieves the size of an OPM display certificate using a handle.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCertificateSizeByHandle(
    VOID
    );

// rev
/**
 * The GetCharABCWidthsFloatI routine retrieves ABC widths of characters in floating-point format.
 *
 * \param Hdc A handle to the device context.
 * \param Param2 Second parameter.
 * \return A handle, size, or result status.
 */
NTSYSAPI
BOOL
NTAPI
GetCharABCWidthsFloatI(
    _In_ HDC Hdc,
    _In_ ULONG First,
    _In_ ULONG Count,
    _In_reads_opt_(Count) const USHORT *Indices,
    _Out_writes_(Count) PVOID Widths
    );

// rev
/**
 * The GetCharWidthInfo routine retrieves character width information for a device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCharWidthInfo(
    VOID
    );

// rev
/**
 * The GetCurrentDpiInfo routine retrieves the current DPI configuration.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetCurrentDpiInfo(
    VOID
    );

// rev
/**
 * The GetETM routine retrieves extended text metrics for a device context.
 *
 * \param Hdc A handle to the device context.
 * \param ExtTextMetric A pointer receiving the extended text metrics.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetETM(
    _In_ HDC Hdc,
    _Out_ PVOID ExtTextMetric
    );

// rev
// Inactive, unverified declaration: GetGlyphOutline.
//NTSYSAPI
//ULONG_PTR
//NTAPI
//GetGlyphOutline(
//    VOID
//    );

// rev
/**
 * The GetGlyphOutlineWow routine retrieves glyph outline metrics under WOW64.
 *
 * \param Hdc Handle to the device context.
 * \param Character Character for which glyph data is to be returned.
 * \param Format Format of the data that the function returns.
 * \param Metrics Pointer to the GLYPHMETRICS structure describing the placement of the glyph in the character cell.
 * \param BufferSize Size, in bytes, of the buffer where the function is to copy information about the glyph.
 * \param Buffer Pointer to the buffer that receives information about the glyph.
 * \param Transform Pointer to a MAT2 structure specifying a transformation matrix for the character.
 * \return ULONG Size of the buffer required for the retrieved information, or GDI_ERROR on failure.
 */
NTSYSAPI
ULONG
NTAPI
GetGlyphOutlineWow(
    _In_ HDC Hdc,
    _In_ ULONG Character,
    _In_ ULONG Format,
    _Out_ LPGLYPHMETRICS Metrics,
    _In_ ULONG BufferSize,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ const MAT2 *Transform
    );

// rev
/**
 * The GetHFONT routine retrieves the HFONT currently selected into a device context.
 *
 * \param Hdc A handle to the device context.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetHFONT(
    _In_ HDC Hdc
    );

// rev
/**
 * The GetInputLocaleInfo routine retrieves input-locale (keyboard layout) information.
 *
 * \param KeyboardLayout Handle to the keyboard layout to query.
 * \param LocaleInfo Pointer to the buffer receiving the locale information.
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserGetInputLocaleInfo system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetInputLocaleInfo(
    _In_ HKL KeyboardLayout,
    _Out_ PVOID LocaleInfo
    );

// rev
/**
 * The GetOPMInformation routine retrieves Output Protection Manager (OPM) configuration details.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetOPMInformation(
    VOID
    );

// rev
/**
 * The GetOPMRandomNumber routine retrieves a cryptographically secure random number from an OPM session.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetOPMRandomNumber(
    VOID
    );

// rev
/**
 * The GetProcessDpiAwarenessInternal routine retrieves the DPI-awareness of a process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param Awareness Pointer receiving the DPI awareness value (PROCESS_DPI_AWARENESS).
 * \return ULONG_PTR Status code or HRESULT.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetProcessDpiAwarenessInternal(
    _In_ HANDLE ProcessHandle,
    _Out_ PLONG Awareness
    );

/**
 * The GetProcessUIContextInformation routine retrieves the UI context and immersive status of a process.
 *
 * \param ProcessHandle A handle to the target process.
 * \param UIContext Pointer to a PROCESS_UICONTEXT_INFORMATION structure receiving the UI context information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
GetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    );

// rev
/**
 * The GetReasonTitleFromReasonCode routine retrieves the display title for a shutdown reason code.
 *
 * \param ReasonCode Shutdown reason code.
 * \param Buffer Output character buffer receiving the title string.
 * \param BufferCount Size of the output buffer in characters.
 * \return ULONG_PTR Length of the title string or status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetReasonTitleFromReasonCode(
    _In_ LONG ReasonCode,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    );

// rev
/**
 * The GetRelAbs routine retrieves the relative/absolute coordinate mode for a device context.
 *
 * \param Hdc A handle to the device context.
 * \return A handle, size, or result status.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetRelAbs(
    _In_ HDC Hdc
    );

// rev
/**
 * The GetSuggestedOPMProtectedOutputArraySize routine suggests the array size for OPM protected outputs.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetSuggestedOPMProtectedOutputArraySize(
    VOID
    );

// rev
/**
 * The GetTextExtentExPointWPri routine retrieves text extent points for a string using internal metrics.
 *
 * \param Hdc A handle to the device context.
 * \param String A pointer to the Unicode string.
 * \param Count Number of characters in the string.
 * \param Size A pointer receiving the size dimensions.
 * \return A status code, size, or result value.
 */
NTSYSAPI
BOOL
NTAPI
GetTextExtentExPointWPri(
    _In_ HDC Hdc,
    _In_reads_(Count) PCWSTR String,
    _In_ ULONG Count,
    _In_ ULONG MaxExtent,
    _Out_opt_ PULONG Fit,
    _Out_writes_to_opt_(Count, *Fit) PLONG Advances,
    _Out_ PSIZE Size
    );

// rev
/**
 * The GetTextFaceAliasW routine retrieves the alias typeface name for a font.
 *
 * \param Hdc A handle to the device context.
 * \param Count The character capacity of the FaceName buffer.
 * \param FaceName A buffer receiving the typeface alias name.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
GetTextFaceAliasW(
    _In_ HDC Hdc,
    _In_ LONG Count,
    _Out_writes_opt_(Count) PWSTR FaceName
    );

// rev
/**
 * The GetTransform routine retrieves the current coordinate transform matrix.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
GetTransform(
    VOID
    );

/**
 * The HMValidateHandleNoRip routine validates a user handle without raising a RIP error on failure.
 *
 * \param Handle The user object handle to validate.
 * \return A pointer to the validated object, or NULL if invalid.
 */
NTSYSAPI
PVOID
NTAPI
HMValidateHandleNoRip(
    _In_ ULONG_PTR Handle
    );

/**
 * The HMValidateHandleNoSecure routine validates a user handle without secure verification.
 *
 * \param Handle The user object handle to validate.
 * \return A pointer to the validated object, or NULL if invalid.
 */
NTSYSAPI
PVOID
NTAPI
HMValidateHandleNoSecure(
    _In_ ULONG_PTR Handle
    );

/**
 * The HMValidateHandleWithDescriptor routine validates a user handle against the specified descriptor type.
 *
 * \param Handle The user object handle to validate.
 * \param Descriptor The descriptor type expected for the handle.
 * \return A pointer to the validated object, or NULL if invalid.
 */
NTSYSAPI
PVOID
NTAPI
HMValidateHandleWithDescriptor(
    _In_ ULONG_PTR Handle,
    _In_ ULONG Descriptor
    );

/**
 * The HMValidateSharedHandle routine validates a shared user handle.
 *
 * \param Handle The shared user object handle to validate.
 * \return A pointer to the validated shared object, or NULL if invalid.
 */
NTSYSAPI
PVOID
NTAPI
HMValidateSharedHandle(
    _In_ ULONG_PTR Handle
    );

// rev
/**
 * The HandleDelegatedInput routine processes input delegated from another window.
 *
 * \param Input Pointer to the input buffer containing delegated input data.
 * \param InputType Type of delegated input.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserHandleDelegatedInput system call.
 */
NTSYSAPI
LOGICAL
NTAPI
HandleDelegatedInput(
    _In_reads_bytes_(48) PVOID Input,
    _In_ ULONG InputType
    );

// rev
/**
 * The InitializeGenericHidInjection routine initializes a session for injecting generic HID input reports.
 *
 * \param DeviceProperties Pointer to a _RIMIDE_GENERIC_HID_DEVICE_PROPERTIES structure describing the device.
 * \param DeviceHandle Pointer to a variable receiving the created HID injection device handle.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInitializeGenericHidInjection system call.
 */
NTSYSAPI
BOOL
NTAPI
InitializeGenericHidInjection(
    _In_ struct _RIMIDE_GENERIC_HID_DEVICE_PROPERTIES* DeviceProperties,
    _Out_writes_(1) HANDLE* DeviceHandle
    );

// rev
/**
 * The InjectDeviceInput routine injects custom input data packets into an initialized input injection device.
 *
 * \param DeviceHandle Handle to the synthetic input device created by InitializeInputDeviceInjection.
 * \param Values Pointer to an array of INPUT_INJECTION_VALUE structures containing input samples.
 * \param Count Number of input values in the Values array.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInjectDeviceInput system call.
 */
NTSYSAPI
BOOL
NTAPI
InjectDeviceInput(
    _In_ HANDLE DeviceHandle,
    _In_reads_(Count) const PINPUT_INJECTION_VALUE Values,
    _In_ ULONG Count
    );

// rev
/**
 * The InjectGenericHidInput routine injects a raw HID report into an initialized synthetic HID device.
 *
 * \param DeviceHandle Handle to the synthetic generic HID device.
 * \param InputReport Pointer to the raw input report bytes.
 * \param InputReportLength Size, in bytes, of the raw input report.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInjectGenericHidInput system call.
 */
NTSYSAPI
BOOL
NTAPI
InjectGenericHidInput(
    _In_ HANDLE DeviceHandle,
    _In_reads_(InputReportLength) const PUCHAR InputReport,
    _In_ ULONG InputReportLength
    );

// rev
/**
 * The InjectKeyboardInput routine injects synthetic keyboard events into the system input stream.
 *
 * \param KeyboardInput Pointer to an array of KEYBDINPUT structures containing keyboard events.
 * \param Count Number of keyboard event structures in KeyboardInput.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInjectKeyboardInput system call.
 */
NTSYSAPI
BOOL
NTAPI
InjectKeyboardInput(
    _In_reads_(Count) const PKEYBDINPUT KeyboardInput,
    _In_ ULONG Count
    );

// rev
/**
 * The InjectMouseInput routine injects synthetic mouse events into the system input stream.
 *
 * \param MouseInput Pointer to an array of MOUSEINPUT structures containing mouse events.
 * \param Count Number of mouse event structures in MouseInput.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserInjectMouseInput system call.
 */
NTSYSAPI
BOOL
NTAPI
InjectMouseInput(
    _In_reads_(Count) const PMOUSEINPUT MouseInput,
    _In_ ULONG Count
    );

// rev
/**
 * The InputSpaceRegionFromPoint routine identifies the input space region containing the specified coordinate point.
 *
 * \param Point Coordinates of the query point.
 * \param InputSpace Identifier of the target input space.
 * \param Flags Region query flags.
 * \param Transform Receives coordinate transform matrix for the region.
 * \param RegionHandle Receives the handle to the matching input space region.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtInputSpaceRegionFromPoint system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
InputSpaceRegionFromPoint(
    _In_ POINT Point,
    _In_ ULONG InputSpace,
    _In_ ULONG Flags,
    _Out_opt_ PVOID Transform,
    _Out_opt_ PHANDLE RegionHandle
    );

// rev
/**
 * The InternalDeleteObject routine internally deletes a GDI object handle.
 *
 * \param Object Handle to the GDI object to delete.
 * \return BOOL TRUE if the object was deleted, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
InternalDeleteObject(
    _In_ HGDIOBJ Object
    );

// rev
/**
 * The IsOneCoreTransformMode routine determines whether OneCore coordinate transform mode is enabled.
 *
 * \param QueryFlags Query flags controlling transform mode inspection.
 * \return BOOL TRUE if OneCore transform mode is active, FALSE otherwise.
 * \remarks Forwards to the NtIsOneCoreTransformMode system call.
 */
NTSYSAPI
BOOL
NTAPI
IsOneCoreTransformMode(
    VOID
    );

// rev
/**
 * The IsSETEnabled routine determines whether the Simple Edit Terminal (SET) input feature is enabled.
 *
 * \return BOOL TRUE if SET is enabled, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
IsSETEnabled(
    VOID
    );

// rev
/**
 * The IsThreadTSFEventAware routine determines whether the calling thread is Text Services Framework event-aware.
 *
 * \param Event Event identifier or flags to check.
 * \return ULONG_PTR Status code or awareness mask.
 */
NTSYSAPI
ULONG_PTR
NTAPI
IsThreadTSFEventAware(
    _In_ ULONG_PTR Event
    );

// rev
/**
 * The IsValidEnhMetaRecord routine validates an enhanced metafile record header.
 *
 * \param HandleTable Handle or descriptor of the metafile table.
 * \param Record A pointer to the enhanced metafile record.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
IsValidEnhMetaRecord(
    _In_ LONG_PTR HandleTable,
    _Inout_ PULONG Record
    );

// rev
/**
 * The IsValidEnhMetaRecordOffExt routine validates an enhanced metafile record offset extension.
 *
 * \param HandleTable Pointer to the handle table.
 * \param This Pointer to the enhanced metafile record context.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
IsValidEnhMetaRecordOffExt(
    _In_ PVOID HandleTable,
    _Inout_ PVOID This,
    _In_ ULONG Param3,
    _In_ ULONG Param4
    );

// rev
/**
 * The LoadKeyboardLayoutEx routine loads a keyboard layout onto an existing layout handle.
 *
 * \param KeyboardLayout Existing keyboard layout handle.
 * \param LayoutName Name of the input locale identifier to load.
 * \param Flags Behavior flags (KLF_*).
 * \return HKL Handle to the loaded keyboard layout, or NULL on failure.
 */
NTSYSAPI
HKL
NTAPI
LoadKeyboardLayoutEx(
    _In_ HKL KeyboardLayout,
    _In_ PCWSTR LayoutName,
    _In_ ULONG Flags
    );

// rev
/**
 * LpkEditControl is a gdi32.dll DATA export, not a function.
 * In-place edit-control hook table, not a pointer variable or callable export.
 * gdi32!LpkpInitializeEditControl copies 112 bytes at 0x18000614d-0x18000618f.
 * Its field signatures remain opaque; only the object address is exposed.
 */
struct _LPK_EDIT_CONTROL_DATA;
NTSYSAPI extern struct _LPK_EDIT_CONTROL_DATA LpkEditControl;

// rev
/**
 * The LpkExtTextOut routine draws text with complex script shaping via language pack support.
 *
 * \param Hdc A handle to the device context.
 * \param X The x-coordinate.
 * \param Y The y-coordinate.
 * \param Options Text output options.
 * \param Rect Optional clipping or opaquing rectangle.
 * \param String A pointer to the Unicode text string.
 * \param StringLength The string length in characters.
 * \param Dx Optional pointer to inter-character spacing array.
 * \param Flags Language pack rendering flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkExtTextOut(
    _In_ HDC Hdc,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG Options,
    _In_opt_ PRECT Rect,
    _In_ PCWSTR String,
    _In_ ULONG StringLength,
    _In_opt_ PLONG Dx,
    _In_ LONG Flags
    );

// rev
/**
 * The LpkGetCharacterPlacement routine retrieves character placement information for complex scripts.
 *
 * \param Hdc A handle to the device context.
 * \param String A pointer to the Unicode string.
 * \param Count The character count.
 * \param MaxExtent The maximum extent in pixels.
 * \param Results A pointer receiving character placement results.
 * \param Flags Placement flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkGetCharacterPlacement(
    _In_ HDC Hdc,
    _In_ PCWSTR String,
    _In_ ULONG Count,
    _In_ LONG MaxExtent,
    _Inout_ PVOID Results,
    _In_ ULONG Flags
    );

// rev
/**
 * The LpkGetEditControl routine retrieves the language pack edit control interface.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
LpkGetEditControl(
    VOID
    );

// rev
/**
 * The LpkGetTextExtentExPoint routine computes text extent points for complex script strings.
 *
 * \param Hdc A handle to the device context.
 * \param String A pointer to the Unicode string.
 * \param StringLength The string length.
 * \param MaxExtent The maximum extent in pixels.
 * \param Fit Optional pointer receiving fitted character count.
 * \param Dx Optional pointer receiving character extent array.
 * \param Size A pointer receiving the total dimensions.
 * \param Flags Formatting flags.
 * \param Param9 Ninth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkGetTextExtentExPoint(
    _In_ HDC Hdc,
    _In_ PCWSTR String,
    _In_ LONG StringLength,
    _In_ LONG MaxExtent,
    _Out_opt_ PLONG Fit,
    _Out_opt_ PLONG Dx,
    _Out_ PSIZE Size,
    _In_ LONG Flags,
    _In_ LONG Param9
    );

// rev
/**
 * The LpkInitialize routine initializes language pack support in the GDI client.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
LpkInitialize(
    VOID
    );

// rev
/**
 * The LpkPSMTextOut routine renders print-style shaped text via language pack support.
 *
 * \param Hdc A handle to the device context.
 * \param X The x-coordinate.
 * \param Y The y-coordinate.
 * \param String A pointer to the Unicode string.
 * \param StringLength The string length.
 * \param Flags Formatting flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkPSMTextOut(
    _In_ HDC Hdc,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ PCWSTR String,
    _In_ ULONG StringLength,
    _In_ LONG Flags
    );

// rev
/**
 * The LpkTabbedTextOut routine renders tabbed text with complex script support.
 *
 * \param Hdc A handle to the device context.
 * \param X The x-coordinate.
 * \param Y The y-coordinate.
 * \param String A pointer to the Unicode string.
 * \param StringLength The string length.
 * \param TabPositionCount Count of tab stops.
 * \param TabStopPositions Array of tab stop positions.
 * \param TabOrigin Origin x-coordinate for tab expansion.
 * \param Param9 Ninth parameter.
 * \param Param10 Tenth parameter.
 * \param Param11 Eleventh parameter.
 * \param Param12 Twelfth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
LpkTabbedTextOut(
    _In_ HDC Hdc,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ PCWSTR String,
    _In_ LONG StringLength,
    _In_ LONG TabPositionCount,
    _In_ PLONG TabStopPositions,
    _In_ LONG TabOrigin,
    _In_ LONG Param9,
    _In_ LONG Param10,
    _In_ LONG Param11,
    _In_ LONG Param12
    );

// rev
/**
 * The LpkUseGDIWidthCache routine determines whether to use the GDI character width cache.
 *
 * \param Hdc A handle to the device context.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Flags Cache configuration flags.
 * \param Param5 Fifth parameter.
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
LpkUseGDIWidthCache(
    _In_ HDC Hdc,
    _In_ LONG_PTR Param2,
    _In_ LONG Param3,
    _In_ SHORT Flags,
    _In_ LONG Param5
    );

// rev
/**
 * LpkpEditControlSize is a gdi32.dll DATA export, not a function.
 * Read-only edit-control slot count (14) at gdi32!0x180012180.
 */
NTSYSAPI extern const ULONG LpkpEditControlSize;

// rev
/**
 * The LpkpInitializeEditControl routine initializes the edit control for language pack support.
 *
 * \param Table Pointer to the edit control callback table.
 * \param Count Number of entries in the table (must be 14).
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
LpkpInitializeEditControl(
    _In_ const VOID *Table,
    _In_ ULONG Count
    );

// rev
/**
 * The MBToWCSEx routine converts a multibyte character string to a wide character string (extended).
 *
 * \param CodePage Code page identifier used for conversion.
 * \param MultiByteString Pointer to the input multibyte character string.
 * \param BytesInMultiByteString Size of the input string in bytes.
 * \param UnicodeString In/out pointer to buffer receiving wide characters, or receiving allocated buffer.
 * \param CharsInUnicodeString Capacity of destination buffer in characters.
 * \param Allocate Non-zero to allocate buffer if needed; zero otherwise.
 * \return ULONG_PTR Number of characters converted, or status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MBToWCSEx(
    _In_ USHORT CodePage,
    _In_ PCSTR MultiByteString,
    _In_ LONG BytesInMultiByteString,
    _Inout_ PWCH* UnicodeString,
    _In_ LONG CharsInUnicodeString,
    _In_ LONG Allocate
    );

// rev
/**
 * The MBToWCSExt routine converts a multibyte character string to wide characters.
 *
 * \param MultiByteString Pointer to the input multibyte string.
 * \param BytesInMultiByteString Size of the input string in bytes.
 * \param UnicodeString In/out pointer to destination buffer or receiving allocated pointer.
 * \param CharsInUnicodeString Capacity of destination buffer in characters.
 * \param Allocate Non-zero to allocate buffer if needed; zero otherwise.
 * \return ULONG_PTR Number of characters converted, or status code.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MBToWCSExt(
    _In_ PSTR MultiByteString,
    _In_ LONG BytesInMultiByteString,
    _Inout_ PVOID* UnicodeString,
    _In_ LONG CharsInUnicodeString,
    _In_ LONG Allocate
    );

// rev
/**
 * The MB_GetString routine retrieves the localized default text for a message-box push button.
 *
 * \param StringIndex Identifier of the standard button text string.
 * \return ULONG_PTR Pointer to the localized string resource.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MB_GetString(
    _In_ ULONG StringIndex
    );

// rev
/**
 * The MITSetLastInputRecipient routine designates the recipient thread or window for subsequent modern input delivery.
 *
 * \param ThreadId Identifier of the target thread receiving input focus.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtMITSetLastInputRecipient system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MITSetLastInputRecipient(
    _In_ ULONG ThreadId
    );

// rev
/**
 * The MakeThreadTSFEventAware routine marks the calling thread as Text Services Framework event-aware.
 *
 * \param Flags TSF event awareness flags.
 * \return ULONG_PTR Status code or previous awareness state.
 */
NTSYSAPI
ULONG_PTR
NTAPI
MakeThreadTSFEventAware(
    _In_ LONG Flags
    );

// rev
/**
 * The MapPointsByVisualIdentifier routine maps a set of points from the coordinate space of one visual to another.
 *
 * \param SourceVisualId Identifier of the source visual element.
 * \param TargetVisualId Identifier of the target visual element.
 * \param PointCount Number of points in the Points array.
 * \param Points Array of POINT structures to transform.
 * \param Flags Transformation control flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserMapPointsByVisualIdentifier system call.
 */
NTSYSAPI
LOGICAL
NTAPI
MapPointsByVisualIdentifier(
    _In_ ULONG_PTR SourceVisualId,
    _In_ ULONG_PTR TargetVisualId,
    _In_ ULONG PointCount,
    _Inout_updates_(PointCount) PPOINT Points,
    _In_ ULONG Flags
    );

// rev
/**
 * The MapVisualRelativePoints routine maps points relative to visual elements across DPI or spatial boundaries.
 *
 * \param SourceVisual Identifier of the source visual element.
 * \param TargetVisual Identifier of the target visual element.
 * \param PointCount Number of points in the coordinate array.
 * \param Points In-out array of POINT structures transformed in place.
 * \param SourceDpi DPI awareness context or value of source visual.
 * \param TargetDpi DPI awareness context or value of target visual.
 * \param Flags Mapping flags controlling coordinate scaling and rounding.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtMapVisualRelativePoints system call.
 */
NTSYSAPI
LOGICAL
NTAPI
MapVisualRelativePoints(
    _In_ ULONG_PTR SourceVisual,
    _In_ ULONG_PTR TargetVisual,
    _In_ ULONG PointCount,
    _Inout_updates_(PointCount) PPOINT Points,
    _In_ ULONG SourceDpi,
    _In_ ULONG TargetDpi,
    _In_ ULONG Flags
    );

// rev
/**
 * The ModerncoreCreateICW routine creates an information context for moderncore windowing.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ModerncoreCreateICW(
    VOID
    );

// rev
/**
 * The NamedEscape routine sends an escape code to a named printer driver.
 *
 * \param Hdc A handle to the device context.
 * \param DriverName Optional printer driver name.
 * \param Escape Escape function code.
 * \param InputLength Length of input data in bytes.
 * \param InputData Pointer or descriptor to input buffer.
 * \param OutputLength Length of output buffer in bytes.
 * \param OutputData Pointer or descriptor to output buffer.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
NamedEscape(
    _In_ HDC Hdc,
    _In_opt_ PCWSTR DriverName,
    _In_ ULONG Escape,
    _In_ ULONG InputLength,
    _In_ LONG_PTR InputData,
    _In_ LONG OutputLength,
    _In_ LONG_PTR OutputData
    );

// rev
/**
 * The NtConfigureInputSpace routine configures an input space in the windowing subsystem.
 *
 * \param InputSpaceInfo A pointer to input space configuration data.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtConfigureInputSpace(
    _In_ PVOID InputSpaceInfo,
    _In_ PVOID Param2,
    _In_ ULONG Param3
    );

// rev
/**
 * The NtEnableOneCoreTransformMode routine enables or disables OneCore coordinate transform mode.
 *
 * \param Enable Specifies whether to enable OneCore transform mode.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtEnableOneCoreTransformMode(
    VOID
    );

// rev
/**
 * The NtInputSpaceRegionFromPoint routine retrieves an input space region containing a given point.
 *
 * \param InputSpaceLuid Locally unique identifier of the input space.
 * \param Point The test coordinate point.
 * \param Region A pointer receiving the containing region.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtInputSpaceRegionFromPoint(
    _In_ ULONG_PTR InputSpaceLuid,
    _In_ LONG_PTR Point,
    _Inout_ PVOID Region,
    _In_ ULONG_PTR Param4,
    _In_ ULONG_PTR Param5
    );

// rev
/**
 * The NtIsOneCoreTransformMode routine queries whether OneCore coordinate transform mode is enabled.
 *
 * \param Param1 Parameter.
 * \return BOOL value.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtIsOneCoreTransformMode(
    VOID
    );

// rev
/**
 * The NtKSTInitialize routine initializes the kernel spatial transform (KST) subsystem.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtKSTInitialize(
    _In_ PVOID Param1,
    _In_ PVOID Param2
    );

// rev
/**
 * The NtKSTWait routine waits on a kernel spatial transform event.
 *
 * \param Param1 Wait parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtKSTWait(
    _In_ ULONG_PTR Param1
    );

// rev
/**
 * The NtMapVisualRelativePoints routine maps coordinates relative to composition visuals.
 *
 * \param SourceVisual A pointer to the source visual.
 * \param TargetVisual A pointer to the target visual.
 * \param PointCount The count of points to map.
 * \param SourcePoints A pointer or address to source points.
 * \param TargetPoints A pointer receiving mapped target points.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtMapVisualRelativePoints(
    _In_ PVOID SourceVisual,
    _In_ PVOID TargetVisual,
    _In_ ULONG PointCount,
    _In_ LONG_PTR SourcePoints,
    _Out_ PVOID TargetPoints
    );

// rev
/**
 * The NtUpdateInputSinkTransforms routine updates coordinate transforms on an input sink.
 *
 * \param InputSink The input sink identifier.
 * \param TransformCount The number of transforms to update.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUpdateInputSinkTransforms(
    _In_ LONG_PTR InputSink,
    _In_ ULONG TransformCount
    );

// rev
/**
 * The NtUserAcquireIAMKey routine acquires the Input Access Manager (IAM) access key for the current desktop.
 *
 * \param IamDesktopKey Receives the per-desktop IAM access key.
 * \return TRUE on success, FALSE otherwise.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAcquireIAMKey(
    _Out_ PIAM_ACCESS_KEY_INPUT IamDesktopKey
    );

// rev
/**
 * The NtUserActivateKeyboardLayout routine sets the input locale identifier (formerly called the keyboard layout handle) for the calling thread or the current process.
 *
 * \param Hkl Handle to the keyboard layout to be activated, or layout flags (e.g. HKL_NEXT, HKL_PREV).
 * \param Flags Keyboard layout options (e.g. KLF_ACTIVATE, KLF_NOTELLSHELL, KLF_REORDER).
 * \return HKL The previous keyboard layout handle on success, or 0 on failure.
 * \remarks Native entry point for USER32!ActivateKeyboardLayout.
 */
_Kernel_entry_
NTSYSCALLAPI
HKL
NTAPI
NtUserActivateKeyboardLayout(
    _In_ HKL Hkl,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserAddVisualIdentifier routine associates a visual identifier LUID with a composition input sink.
 *
 * \param CompositionInputSink Handle to the composition input sink object.
 * \param Luid Pointer to the locally unique identifier (LUID) of the visual.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAddVisualIdentifier(
    _In_ HANDLE CompositionInputSink,
    _In_ PLUID Luid
    );

// rev
/**
 * The NtUserAttachThreadInput routine attaches or detaches the input processing mechanism of one thread to that of another.
 *
 * \param IdAttach The identifier of the thread to be attached.
 * \param IdAttachTo The identifier of the thread to which IdAttach will be attached.
 * \param Attach TRUE to attach the threads; FALSE to detach them.
 * \return TRUE if successful, FALSE otherwise.
 * \remarks Native entry point for USER32!AttachThreadInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserAttachThreadInput(
    _In_ ULONG IdAttach,
    _In_ ULONG IdAttachTo,
    _In_ BOOL Attach
    );

// rev
/**
 * The NtUserAutoRotateScreen routine enables or disables automatic display screen rotation based on orientation sensor data.
 *
 * \param Enable Non-zero to enable auto-rotation; 0 to disable.
 * \return LONG The resulting screen rotation, or -1 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserAutoRotateScreen(
    _In_ ULONG Enable
    );

// rev
/**
 * The NtUserBitBltSysBmp routine bit-block transfers a system bitmap into the specified device context.
 *
 * \param Hdc Handle to the destination device context.
 * \param Param2 Destination X coordinate or packed coordinate parameter.
 * \param Param3 Destination Y coordinate or packed dimension parameter.
 * \param OemBitmapIndex System/OEM bitmap index (OBM_*).
 * \param Rop Raster operation code (ROP).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserBitBltSysBmp(
    _In_ HDC Hdc,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _In_ ULONG OemBitmapIndex,
    _In_ LONG Rop
    );

/**
 * The NtUserBlockInput routine blocks or unblocks mouse and keyboard input events.
 *
 * \param BlockInput TRUE to block input; FALSE to unblock input.
 * \return TRUE on success; FALSE on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserBlockInput(
    _In_ BOOL BlockInput
    );

// rev
/**
 * The NtUserBroadcastThemeChangeEvent routine broadcasts a desktop theme change notification event to top-level windows.
 *
 * \param wParam Message-specific WPARAM parameter.
 * \param lParam Message-specific LPARAM parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserBroadcastThemeChangeEvent(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
/**
 * The NtUserCanCurrentThreadChangeForeground routine determines whether the calling thread is permitted to set the foreground window.
 *
 * \return TRUE if the current thread can set the foreground window; otherwise, FALSE.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCanCurrentThreadChangeForeground(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserCheckProcessSession routine verifies that the specified process belongs to the calling session.
 *
 * \param ProcessId Unique identifier of the process to check.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCheckProcessSession(
    _In_ LONG_PTR ProcessId
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCitSetInfo routine sets Composable Input Tree (CIT) information.
 *
 * \param InfoFlags The info flags.
 * \param Info Pointer to composition input transport information structure.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallTwoParam(SFI_CITSETINFO) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCitSetInfo(
    _In_ ULONG_PTR InfoFlags,
    _In_ ULONG_PTR Info
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * Invokes the legacy foreground-clearing entry point.
 * \return The inspected kernel implementation returns zero without changing foreground state.
 * \remarks Takes no arguments, confirmed by the x86 syscall stub. The inspected x64 body
 * is a shared xor eax,eax; ret stub; zero alone does not establish the historical return semantics.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserClearForeground(
    VOID
    );

// rev
/**
 * Updates the selected state bits of an activation object.
 * \param ActivationObjectId Pointer to the 8-byte activation-object LUID.
 * \param Reason ACTIVATIONOBJECTSTATECHANGE_REASON value: zero or one; one requires DWM.
 * \param Behavior ACTIVATION_OBJECT_CONFIG_BEHAVIOR value. For reason zero, only zero and one are accepted.
 * \param StateMask ACTIVATION_OBJECT_STATE bit mask selecting state bits to update.
 * \param State ACTIVATION_OBJECT_STATE values for the selected bits.
 * \return Nonzero on success, zero on failure; helper NTSTATUS failures are converted to Win32 last error.
 * \remarks The inspected manager handles state bits 0x1, 0x2 and 0x4; 0x4 controls foreground.
 * Reason zero with behavior one additionally checks object ownership and foreground eligibility.
 * Enum member names and the meanings of state bits 0x1 and 0x2 remain unconfirmed.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserConfigureActivationObject(
    _In_ const LUID *ActivationObjectId,
    _In_ ULONG Reason,
    _In_ ULONG Behavior,
    _In_ ULONG StateMask,
    _In_ ULONG State
    );

// rev
/**
 * The NtUserConvertMemHandle routine converts a shared memory block into a clipboard memory handle.
 *
 * \param Address Pointer to the memory buffer to convert.
 * \param Length Length, in bytes, of the memory buffer.
 * \return HANDLE value.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserConvertMemHandle(
    _Inout_ PVOID Address,
    _In_ SIZE_T Length
    );

// rev
/**
 * The NtUserCreateLocalMemHandle routine retrieves clipboard data into a local memory buffer.
 *
 * \param ClipboardDataHandle Handle to the clipboard data object.
 * \param Buffer Optional pointer to the buffer receiving the clipboard data.
 * \param Size Size, in bytes, of the destination buffer.
 * \param BytesNeeded Optional pointer to a variable receiving the required buffer size.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCreateLocalMemHandle(
    _In_ HANDLE ClipboardDataHandle,
    _Out_writes_bytes_opt_(Size) PVOID Buffer,
    _In_ ULONG Size,
    _Out_opt_ PULONG BytesNeeded
    );

// rev
/**
 * The NtUserCreatePalmRejectionDelayZone routine registers a palm rejection exclusion or delay zone for touch input.
 *
 * \param VisualInputSink Pointer to the input sink of the visual that owns the delay zone.
 * \param DelayZoneRect Pointer to a RECT structure describing the delay zone.
 * \param TargetVisualInputSink Optional pointer to the input sink of a second visual.
 * \param TargetRect Optional pointer to a RECT structure describing a second zone.
 * \param Delay Palm rejection delay parameter.
 * \return Identifier of the created delay zone; zero indicates failure.
 */
// Note: confirmed from user32!CreatePalmRejectionDelayZoneInternal and win32kbase
// (AddPalmRejectionDelayZone forwards: sink1, RECT, sink2, RECT, delay). The kernel
// returns the zone identifier; zero indicates failure (GetLastError is set).
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserCreatePalmRejectionDelayZone(
    _In_ PVOID VisualInputSink,
    _In_ PRECT DelayZoneRect,
    _In_opt_ PVOID TargetVisualInputSink,
    _In_opt_ PRECT TargetRect,
    _In_ ULONG Delay
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCreateSystemThreads routine creates the win32k system (raw input / desktop) threads.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_CREATESYSTEMTHREADS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateSystemThreads(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserCsDdeUninitialize routine uninitializes client-side DDE for the specified module.
 *
 * \param hInst The h inst.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_CSDDEUNINITIALIZE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCsDdeUninitialize(
    _In_ HANDLE hInst
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDdeInitialize routine initializes a Dynamic Data Exchange (DDE) conversation context for the caller.
 *
 * \param Instance Pointer to a variable receiving the DDE instance identifier.
 * \param Reserved Reserved parameter; must be NULL.
 * \param Result Pointer to a variable receiving the DDE initialization status or flags.
 * \param Flags DDE initialization options (e.g. APPCMD_CLIENTONLY, CBF_*).
 * \param Callback Pointer or handle to the DDE callback function.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDdeInitialize(
    _Out_ PVOID Instance,
    _Out_ PVOID Reserved,
    _Out_ PULONG Result,
    _In_ LONG Flags,
    _In_ LONG_PTR Callback
    );

// rev
/**
 * The NtUserDestroyActivationObject routine destroys a window activation context object.
 *
 * \param ActivationObject Pointer to or handle of the activation object to destroy.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyActivationObject(
    _In_ PVOID ActivationObject
    );

// rev
/**
 * The NtUserDestroyPalmRejectionDelayZone routine destroys a previously registered touch palm rejection delay zone.
 *
 * \param DelayZoneId Identifier of the palm rejection delay zone to destroy.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyPalmRejectionDelayZone(
    _In_ ULONG DelayZoneId
    );

// rev
/**
 * The NtUserDoSoundConnect routine connects the win32k sound driver notification port.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserDoSoundConnect(
    VOID
    );

// rev
/**
 * The NtUserDoSoundDisconnect routine disconnects the win32k sound driver notification port.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserDoSoundDisconnect(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDrainThreadCoreMessagingCompletions routine drains outstanding core-messaging completions for the calling thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_DRAINTHREADCOREMESSAGINGCOMPLETIONS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrainThreadCoreMessagingCompletions(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserDwmGetRemoteSessionOcclusionEvent routine retrieves or triggers the remote session occlusion notification event.
 *
 * \return HANDLE value.
 */
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserDwmGetRemoteSessionOcclusionEvent(
    VOID
    );

// rev
/**
 * The NtUserDwmGetRemoteSessionOcclusionState routine retrieves the occlusion state for a remote desktop session.
 *
 * \param SessionId Identifier of the remote session to query.
 * \return LONG value.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserDwmGetRemoteSessionOcclusionState(
    VOID
    );

// rev
/**
 * The NtUserDwmKernelShutdown routine signals kernel-mode DWM components that the Desktop Window Manager is shutting down.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserDwmKernelShutdown(
    VOID
    );

// rev
/**
 * The NtUserDwmKernelStartup routine initializes kernel-mode Desktop Window Manager (DWM) support structures.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDwmKernelStartup(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserDwmLockScreenUpdates routine locks or unlocks DWM screen updates.
 *
 * \param LockUpdates The lock updates.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_DWMLOCKSCREENUPDATES) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDwmLockScreenUpdates(
    _In_ LOGICAL LockUpdates
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnableIAMAccess routine enables or disables Input Access Manager (IAM) access using a previously acquired key.
 *
 * \param AccessInput IAM access key obtained from NtUserAcquireIAMKey.
 * \param Enable TRUE to enable access, FALSE to disable.
 * \return TRUE on success, FALSE otherwise.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableIAMAccess(
    _In_ const IAM_ACCESS_KEY_INPUT* AccessInput,
    _In_ LOGICAL Enable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnableSessionForMMCSS routine enables the session for the Multimedia Class Scheduler Service.
 *
 * \param Enable TRUE to enable; FALSE to disable.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_ENABLESESSIONFORMMCSS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserEnableSessionForMMCSS(
    _In_ LOGICAL Enable
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserEnsureDpiDepSysMetCacheForPlateau routine ensures the DPI-dependent system-metrics cache for the DPI plateau.
 *
 * \param Dpi Target DPI plateau value.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_ENSUREDPIDEPSYSMETCACHEFORPLATEAU) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnsureDpiDepSysMetCacheForPlateau(
    _In_ ULONG Dpi
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserEnsureDpiMetricsForDpi routine ensures that system DPI metrics are initialized for the specified DPI value.
 *
 * \param Dpi Dots per inch (DPI) value to ensure metrics for.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnsureDpiMetricsForDpi(
    _In_ USHORT Dpi
    );

// rev
/**
 * The NtUserEnsureDpiServerInfoForDpi routine ensures that win32k server-side DPI information is initialized for a DPI value.
 *
 * \param Dpi Dots per inch (DPI) value to ensure server information for.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnsureDpiServerInfoForDpi(
    _In_ USHORT Dpi
    );

// rev
/**
 * The NtUserEvent routine signals a user-mode or kernel-mode event object in win32k.
 *
 * \param Event Pointer to an event notification descriptor or handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEvent(
    _In_ PVOID Event
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserForceEnableNumpadTranslation routine forces numeric-keypad key translation on.
 *
 * \param ForceNumlockTranslation The force numlock translation.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_FORCEENABLENUMPADTRANSLATION) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserForceEnableNumpadTranslation(
    _In_ LOGICAL ForceNumlockTranslation
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetActiveProcessesDpis routine queries DPI awareness and scaling settings across active desktop processes.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetActiveProcessesDpis(
    VOID
    );

// rev
/**
 * The NtUserGetAsyncKeyState routine determines whether a key is up or down at the time the function is called, and whether the key was pressed after a previous call.
 *
 * \param VirtualKey The virtual-key code of interest.
 * \return LONG The key state: the high bit is set if the key is down, the low bit if it was pressed since the last call.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetAsyncKeyState(
    _In_ ULONG VirtualKey
    );

// rev
/**
 * The NtUserGetAtomName routine retrieves the string associated with the specified global atom in win32k.
 *
 * \param Atom The atom whose associated string is to be retrieved.
 * \param AtomName Pointer to a UNICODE_STRING structure that receives the atom string.
 * \return ULONG The number of characters copied to the buffer, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetAtomName(
    _In_ USHORT Atom,
    _Inout_ PUNICODE_STRING AtomName
    );

// rev
/**
 * The NtUserGetAutoRotationState routine retrieves the auto-rotation state for the current display.
 *
 * \param PState Pointer to an AR_STATE variable receiving the auto-rotation state flags.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetAutoRotationState.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetAutoRotationState(
    _Out_ PAR_STATE PState
    );

// rev
/**
 * The NtUserGetCIMSSM routine retrieves current input message source information for the current input message.
 *
 * \param InputMessageSource Pointer to an INPUT_MESSAGE_SOURCE structure receiving the source device and identifier.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetCIMSSM.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCIMSSM(
    _Out_ INPUT_MESSAGE_SOURCE *InputMessageSource
    );

// rev
/**
 * The NtUserGetClassInfoEx routine retrieves information about a window class.
 *
 * \param Instance Module instance owning the class, or 0 for system classes.
 * \param ClassName Pointer to a UNICODE_STRING containing the class name or class atom.
 * \param WndClassEx Pointer to a WNDCLASSEX structure receiving class attributes.
 * \param MenuName Pointer to a buffer receiving the menu resource name.
 * \param Ansi Non-zero if querying ANSI class information; 0 for Unicode.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetClassInfoEx(
    _In_ LONG Instance,
    _In_ PUNICODE_STRING ClassName,
    _Out_ PVOID WndClassEx,
    _Out_ PVOID MenuName,
    _In_ LONG Ansi
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetDeviceChangeInfo routine retrieves pending device-change information for the calling queue.
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_GETDEVICECHANGEINFO) before WIN11.
 */
_Success_(return != 0)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetDeviceChangeInfo(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetDoubleClickTime routine retrieves the current double-click time for the mouse.
 *
 * \return The double-click time in milliseconds.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetDoubleClickTime(
    VOID
    );

// rev
/**
 * The NtUserGetDpiForCurrentProcess routine retrieves the DPI value associated with the calling process.
 *
 * \return ULONG DPI value for the process.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetDpiForCurrentProcess(
    VOID
    );

/**
 * The NtUserGetGUIThreadInfo routine retrieves information about the active window or a specified GUI thread.
 *
 * \param idThread The identifier of the thread for which information is to be retrieved.
 * \param pgui Pointer to a GUITHREADINFO structure that receives information describing the thread.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetGUIThreadInfo(
    _In_ ULONG idThread,
    _Inout_ PGUITHREADINFO pgui
    );

/**
 * The NtUserGetGuiResources routine retrieves the count of handles to graphical user interface (GUI) objects in use by the specified process.
 *
 * \param ProcessHandle A handle to the process.
 * \param Flags The GUI object type (GR_GDIOBJECTS, GR_USEROBJECTS, etc.).
 * \return The count of GUI objects in use, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetGuiResources(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserGetHDevName routine retrieves the device name string associated with an HDEV graphics device.
 *
 * \param DeviceHandle Handle or identifier of the display device (HDEV).
 * \param DeviceName Pointer to a buffer or UNICODE_STRING receiving the display device name.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetHDevName(
    _In_ LONG DeviceHandle,
    _Out_ PVOID DeviceName
    );

// rev
/**
 * The NtUserGetHimetricScaleFactorFromPixelLocation routine calculates HIMETRIC-to-pixel scaling factors for a screen coordinate.
 *
 * \param PointerId Pointer or touch device identifier.
 * \param Point Packed pixel coordinate or point structure.
 * \param ScaleX Pointer to a variable receiving the horizontal scaling factor.
 * \param ScaleY Pointer to a variable receiving the vertical scaling factor.
 * \param Param5 Additional conversion options or DPI parameter.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetHimetricScaleFactorFromPixelLocation(
    _In_ LONG_PTR PointerId,
    _In_ LONG_PTR Point,
    _Out_ PULONG ScaleX,
    _Out_ PULONG ScaleY,
    _In_ ULONG_PTR Param5
    );

// rev
/**
 * The NtUserGetInputContainerId routine retrieves the device container identifier (GUID) for an input device.
 *
 * \param Param1 Pointer to a 16-byte input structure read from the caller.
 * \param ContainerId Receives the 4-byte container identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
// Note: the implementation forwards its arguments unchanged to an internal helper
// with no recoverable naming; the parameters below are unconfirmed.
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetInputContainerId(
    _In_ PVOID Param1,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _Out_ PULONG ContainerId
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetInputEvent routine waits for and returns an input event matching the specified wake mask.
 *
 * \param WakeMask The wake mask (QS_* WinUser.h).
 * \return The resulting handle, or NULL on failure.
 * \remarks Exposed via NtUserCallOneParam(SFI_GETINPUTEVENT) before WIN11.
 */
_Success_(return != NULL)
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetInputEvent(
    _In_ ULONG WakeMask // QS_* WinUser.h
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetInputLocaleInfo routine retrieves information about a specific input locale or keyboard layout.
 *
 * \param KeyboardLayout Handle to the keyboard layout (HKL) to query.
 * \param LocaleInfo Pointer to a structure receiving the input locale information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetInputLocaleInfo(
    _In_ HKL KeyboardLayout,
    _Out_ PVOID LocaleInfo
    );

// rev
/**
 * The NtUserGetInteractiveControlDeviceInfo routine retrieves device information for an interactive control device.
 *
 * \param DeviceId Identifier of the interactive control device.
 * \param DeviceInfo Pointer to a structure receiving the interactive control device information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetInteractiveControlDeviceInfo(
    _In_ ULONG DeviceId,
    _Out_ PVOID DeviceInfo
    );

// rev
/**
 * The NtUserGetInteractiveControlInfo routine retrieves configuration and state information for an interactive control.
 *
 * \param ControlId Identifier of the interactive control.
 * \param ControlInfo Pointer to a structure receiving the interactive control information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetInteractiveControlInfo(
    _In_ ULONG ControlId,
    _Out_ PVOID ControlInfo
    );

// rev
/**
 * The NtUserGetInteractiveCtrlSupportedWaveforms routine retrieves the supported haptic waveforms for an interactive control device.
 *
 * \param DeviceId Identifier of the interactive control device.
 * \param Buffer Optional pointer to a buffer receiving the array of supported waveform IDs.
 * \param BufferSize Pointer to a variable holding the buffer size in bytes and receiving the required or copied size.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetInteractiveCtrlSupportedWaveforms(
    _In_ USHORT DeviceId,
    _Out_opt_ PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

// rev
/**
 * The NtUserGetKeyNameText routine retrieves a string that represents the name of a key.
 *
 * \param lParam The second parameter of the keyboard message (such as WM_KEYDOWN) to be processed.
 * \param Buffer Pointer to a buffer that will receive the key name string.
 * \param BufferCount The maximum length, in characters, of the key name.
 * \return LONG The number of characters copied to the buffer, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetKeyNameText(
    _In_ ULONG lParam,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    );

// rev
/**
 * The NtUserGetKeyState routine retrieves the status of the specified virtual key.
 *
 * \param VirtualKey The virtual key code of interest.
 * \return LONG The key state: the high bit is set if the key is down, the low bit if it is toggled.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetKeyState(
    _In_ ULONG VirtualKey
    );

// rev
/**
 * The NtUserGetKeyboardInputThreadId routine retrieves the thread ID currently receiving keyboard focus or input.
 *
 * \param Param1 Unconfirmed thread or queue query parameter.
 * \return ULONG The identifier of the thread receiving keyboard input.
 */
// Note: the kernel implementation takes no arguments; the parameters below could
// not be confirmed from win32kfull.sys/win32kbase.sys and the win32u.dll stub
// carries no argument information.
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetKeyboardInputThreadId(
    VOID
    );

// rev
/**
 * The NtUserGetKeyboardLayout routine retrieves the active input locale identifier (keyboard layout) for the specified thread.
 *
 * \param ThreadId Identifier of the thread to query, or 0 for the current thread.
 * \return HKL The input locale identifier for the thread, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HKL
NTAPI
NtUserGetKeyboardLayout(
    _In_ ULONG ThreadId
    );

// rev
/**
 * The NtUserGetKeyboardLayoutList routine retrieves the input locale identifiers (keyboard layout handles) corresponding to the current set of input locales in the system.
 *
 * \param NBuff The maximum number of handles that the buffer can hold.
 * \param LpList Pointer to the buffer that receives the array of input locale identifiers.
 * \return LONG The number of input locale identifiers copied to the buffer, or required if NBuff is 0.
 * \remarks Native entry point for USER32!GetKeyboardLayoutList.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetKeyboardLayoutList(
    _In_ LONG NBuff,
    _Out_writes_to_opt_(NBuff, return) HKL  *LpList
    );

// rev
/**
 * The NtUserGetKeyboardLayoutName routine retrieves the name of the active input locale identifier (keyboard layout) for the calling thread.
 *
 * \param Name Pointer to a UNICODE_STRING that receives the name of the input locale identifier.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetKeyboardLayoutName(
    _Inout_ PUNICODE_STRING Name
    );

// rev
/**
 * The NtUserGetKeyboardState routine copies the status of the 256 virtual keys to the specified buffer.
 *
 * \param LpKeyState Pointer to the 256-byte array that receives the status data for each virtual key.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!GetKeyboardState.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetKeyboardState(
    _Out_writes_(256) PBYTE LpKeyState
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetKeyboardType routine returns keyboard type information selected by the specified flag.
 *
 * \param TypeFlag The type flag (KEYBOARD_*).
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_GETKEYBOARDTYPE) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetKeyboardType(
    _In_ ULONG TypeFlag // KEYBOARD_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtUserGetMouseMovePointsEx routine retrieves a history of up to 64 previous coordinates of the mouse or pen.
 *
 * \param MouseMovePointsSize The size of the MOUSEMOVEPOINT structure in bytes.
 * \param InputBuffer Pointer to a MOUSEMOVEPOINT structure containing a mouse point.
 * \param OutputBuffer Pointer to a buffer that receives the points.
 * \param OutputBufferCount The number of points to retrieve.
 * \param Resolution The resolution of the points (GMMP_USE_DISPLAY_POINTS or GMMP_USE_HIGH_RESOLUTION_POINTS).
 * \return The number of points retrieved, or -1 on error.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserGetMouseMovePointsEx(
    _In_ ULONG MouseMovePointsSize,
    _In_ LPMOUSEMOVEPOINT InputBuffer,
    _Out_writes_(OutputBufferCount) LPMOUSEMOVEPOINT OutputBuffer,
    _In_ LONG OutputBufferCount,
    _In_ ULONG Resolution
    );

/**
 * The NtUserGetObjectInformation routine retrieves information about a window station or desktop object.
 *
 * \param ObjectHandle A handle to the window station or desktop object.
 * \param Index The information to be retrieved (e.g. UOI_NAME, UOI_TYPE, UOI_USER_SID).
 * \param vInfo Pointer to a buffer to receive the object information.
 * \param Length The size of the buffer pointed to by vInfo in bytes.
 * \param LengthNeeded Pointer to a variable that receives the number of bytes required to store the requested information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetObjectInformation(
    _In_ HANDLE ObjectHandle,
    _In_ LONG Index,
    _Out_writes_bytes_opt_(Length) PVOID vInfo,
    _In_ ULONG Length,
    _Out_opt_ PULONG LengthNeeded
    );

// rev
/**
 * The NtUserGetPhysicalDeviceRect routine retrieves the physical bounding rectangle for a display or pointer device.
 *
 * \param DeviceId Identifier of the physical device.
 * \param Rect Pointer to a RECT structure receiving the physical device bounds.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetPhysicalDeviceRect(
    _In_ LONG_PTR DeviceId,
    _Out_ PRECT Rect
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserGetProcessDefaultLayout routine retrieves the process default window layout.
 *
 * \param DefaultLayout The default layout.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_GETPROCESSDEFAULTLAYOUT) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetProcessDefaultLayout(
    _Out_ PULONG DefaultLayout
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserGetProcessDpiAwarenessContext routine retrieves the DPI_AWARENESS_CONTEXT for the specified process.
 *
 * \param Handle Pointer or handle receiving the DPI awareness context identifier.
 * \return ULONG The DPI awareness context for the process, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetProcessDpiAwarenessContext(
    _Out_ PSTR Handle
    );

/**
 * The NtUserGetProcessUIContextInformation routine retrieves the UI context and immersive status of a process.
 *
 * \param ProcessHandle A handle to the target process.
 * \param UIContext Pointer to a PROCESS_UICONTEXT_INFORMATION structure receiving the UI context information.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    );

// rev
/**
 * The NtUserGetSystemContentRects routine retrieves system content bounding rectangles across active display monitors.
 *
 * \param Count Pointer to a variable holding the maximum rectangle count and receiving the returned count.
 * \param Rects Pointer to an array of RECT structures receiving system content bounds.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserGetSystemContentRects(
    _Inout_ PULONG Count,
    _Out_ PVOID Rects
    );

// rev
/**
 * The NtUserGetSystemDpiForProcess routine retrieves the system DPI associated with a specific process.
 *
 * \param Process Handle to the process to query.
 * \return ULONG The system DPI of the process, or 0 on error.
 * \remarks Native entry point for USER32!GetSystemDpiForProcess.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetSystemDpiForProcess(
    _In_ HANDLE HProcess
    );

/**
 * The NtUserGetThreadState routine retrieves state information for the current user thread.
 *
 * \param UserThreadState The thread state information class or selector to query.
 * \return A pointer or status value depending on the requested thread state.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetThreadState(
    _In_ ULONG UserThreadState
    );

// rev
/**
 * The NtUserGetUniformSpaceMapping routine retrieves the uniform coordinate space mapping for an input target.
 *
 * \param Handle Handle or identifier of the input target or display device.
 * \param Mapping Pointer to a structure receiving coordinate space transform data.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetUniformSpaceMapping(
    _In_ LONG_PTR Handle,
    _Out_ PVOID Mapping
    );

// rev
/**
 * The NtUserGetWOWClass routine retrieves the 16-bit WOW window class information for a window class.
 *
 * \param Instance Module instance identifier.
 * \param ClassName Pointer to a UNICODE_STRING specifying the class name.
 * \return ULONG_PTR The WOW class pointer, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetWOWClass(
    _In_ LONG_PTR Instance,
    _In_ PUNICODE_STRING ClassName
    );

// rev
/**
 * The NtUserHandleDelegatedInput routine dispatches delegated input data to the destination thread or window.
 *
 * \param Input Pointer to a 48-byte buffer containing delegated input data.
 * \param InputType Type of input being handled (e.g. pointer, touch, keyboard).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHandleDelegatedInput(
    _In_reads_bytes_(48) PVOID Input,
    _In_ ULONG InputType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserHandleSystemThreadCreationFailure routine reports a failure to create a win32k system thread.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_HANDLESYSTEMTHREADCREATIONFAILURE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHandleSystemThreadCreationFailure(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserHardErrorControl routine controls system hard error dialog processing and display.
 *
 * \param Command Hard error command code (e.g. display, dismiss, query).
 * \param Param2 Command-specific parameter or message identifier.
 * \param HardErrorInfo Optional pointer to a hard error info structure or context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserHardErrorControl(
    _In_ ULONG Command,
    _In_ LONG_PTR Param2,
    _Inout_opt_ PVOID HardErrorInfo
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserInitAnsiOem routine initializes the ANSI/OEM translation tables.
 *
 * \param OemToAnsi The oem to ansi.
 * \param AnsiToOem The ansi to oem.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_INITANSIOEM) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserInitAnsiOem(
    _In_reads_(256) PCHAR OemToAnsi,
    _In_reads_(256) PCHAR AnsiToOem
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserInitialize routine performs one-time initialization of the user subsystem for the calling process.
 *
 * \param Param1 Initialization parameter or configuration flags.
 * \param Param2 Additional initialization context.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserInitialize(
    _In_ LONG_PTR Param1,
    _In_ LONG_PTR Param2
    );

// rev
/**
 * The NtUserInitializeClientPfnArrays routine registers user32 client dispatch function pointer arrays with win32k.
 *
 * \param ClientProcsA Optional pointer to the ANSI client window procedure dispatch table.
 * \param ClientProcsW Optional pointer to the Unicode client window procedure dispatch table.
 * \param ClientWorkerProcs Optional pointer to client worker procedure table.
 * \param Instance Module instance of user32.dll.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserInitializeClientPfnArrays(
    _In_opt_ PVOID ClientProcsA,
    _In_opt_ PVOID ClientProcsW,
    _In_opt_ PVOID ClientWorkerProcs,
    _In_ LONG_PTR Instance
    );

// rev
/**
 * The NtUserInitializeGenericHidInjection routine initializes generic HID input injection for a simulated device.
 *
 * \param PDeviceProperties Pointer to a _RIMIDE_GENERIC_HID_DEVICE_PROPERTIES structure describing the device.
 * \param PhDevice Pointer to a variable receiving the created HID injection device handle.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InitializeGenericHidInjection.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInitializeGenericHidInjection(
    _In_ struct _RIMIDE_GENERIC_HID_DEVICE_PROPERTIES* PDeviceProperties,
    _Out_writes_(1) HANDLE* PhDevice
    );

// rev
/**
 * The NtUserInjectDeviceInput routine injects custom input data packets into an initialized input injection device.
 *
 * \param Device Handle to the synthetic input device created by NtUserInitializeInputDeviceInjection.
 * \param Values Pointer to an array of INPUT_INJECTION_VALUE structures containing input samples.
 * \param Count Number of input values in the Values array.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectDeviceInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectDeviceInput(
    _In_ HANDLE Device,
    _In_reads_(Count) const PINPUT_INJECTION_VALUE Values,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserInjectGenericHidInput routine injects a raw HID report into an initialized synthetic HID device.
 *
 * \param HDevice Handle to the synthetic generic HID device.
 * \param PInputReport Pointer to the raw input report bytes.
 * \param UlInputReportLength Size, in bytes, of the raw input report.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectGenericHidInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectGenericHidInput(
    _In_ HANDLE HDevice,
    _In_reads_(UlInputReportLength) const PUCHAR PInputReport,
    _In_ ULONG UlInputReportLength
    );

// rev
/**
 * The NtUserInjectKeyboardInput routine injects synthetic keyboard events into the system input stream.
 *
 * \param PKeyboardInput Pointer to an array of KEYBDINPUT structures containing keyboard events.
 * \param Count Number of keyboard event structures in PKeyboardInput.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectKeyboardInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectKeyboardInput(
    _In_reads_(Count) const PKEYBDINPUT PKeyboardInput,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserInjectMouseInput routine injects synthetic mouse events into the system input stream.
 *
 * \param PMouseInput Pointer to an array of MOUSEINPUT structures containing mouse events.
 * \param Count Number of mouse event structures in PMouseInput.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Native entry point for USER32!InjectMouseInput.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserInjectMouseInput(
    _In_reads_(Count) const PMOUSEINPUT PMouseInput,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserInteractiveControlQueryUsage routine queries usage capabilities for an interactive control device.
 *
 * \param ControlId Identifier of the interactive control.
 * \param Param2 Usage query filter or page identifier.
 * \param Param3 Additional usage query parameter.
 * \param Param4 Additional usage query parameter.
 * \param Usage Pointer to a variable receiving usage attribute flags or values.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserInteractiveControlQueryUsage(
    _In_ ULONG ControlId,
    _In_ USHORT Param2,
    _In_ USHORT Param3,
    _In_ USHORT Param4,
    _Out_ PULONG Usage
    );

// rev
/**
 * The NtUserInternalToUnicode routine translates a virtual key code and keyboard state to Unicode characters using internal state.
 *
 * \param Param1 Unconfirmed pointer parameter.
 * \param Param2 Unconfirmed pointer parameter.
 * \param Param3 Unconfirmed pointer-sized parameter.
 * \param Param4 Unconfirmed pointer parameter.
 * \param Param5 Unconfirmed pointer-sized parameter.
 * \param Param6 Unconfirmed pointer-sized parameter.
 * \return NTSTATUS value; the result contract remains unverified.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserInternalToUnicode(
    _In_ PVOID Param1,
    _In_ PVOID Param2,
    _In_ LONG_PTR Param3,
    _In_ PVOID Param4,
    _In_ LONG_PTR Param5,
    _In_ LONG_PTR Param6
    );

// rev
/**
 * The NtUserIsMouseInputEnabled routine determines whether mouse input is currently enabled on the system.
 *
 * \return BOOL TRUE if mouse input is enabled, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserIsMouseInputEnabled(
    VOID
    );

// rev
/**
 * The NtUserIsUtilitySession routine determines whether the current caller is executing in a Windows utility session.
 *
 * \return BOOLEAN TRUE if the caller is running in a utility session, otherwise FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserIsUtilitySession(
    VOID
    );

// rev
/**
 * The NtUserLoadKeyboardLayoutEx routine loads a keyboard layout into the system.
 *
 * \param KeyboardFileHandle Optional handle to the keyboard layout file.
 * \param OffsetTable Offset to the layout translation tables.
 * \param Param3 Additional layout configuration parameter.
 * \param KeyboardTables Optional pointer to layout table structures.
 * \param KeyboardLayout Optional handle to the previous layout (HKL).
 * \param Klid Pointer to a UNICODE_STRING specifying the layout identifier.
 * \param KlidValue Numeric layout value.
 * \param Flags Keyboard layout loading flags (KLF_*).
 * \return HKL Handle to the loaded keyboard layout, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HKL
NTAPI
NtUserLoadKeyboardLayoutEx(
    _In_opt_ HANDLE KeyboardFileHandle,
    _In_ ULONG OffsetTable,
    _In_ ULONG Param3,
    _In_opt_ PVOID KeyboardTables,
    _In_opt_ HKL KeyboardLayout,
    _In_ PCUNICODE_STRING Klid,
    _In_ ULONG KlidValue,
    _In_ ULONG Flags
    );

/**
 * The NtUserLockWorkStation routine locks the workstation's display.
 *
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserLockWorkStation(
    VOID
    );

// rev
/**
 * The NtUserMNDragLeave routine notifies the active menu drag session that the pointer has left the menu window.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserMNDragLeave(
    VOID
    );

// rev
/**
 * The NtUserMNDragOver routine processes drag-over movement notifications within an active menu tracking session.
 *
 * \param Point Pointer to a POINT structure indicating current screen coordinates.
 * \param DragOverInfo Pointer to a structure receiving menu drag-over hit test and command results.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMNDragOver(
    _In_ PPOINT Point,
    _Out_ PVOID DragOverInfo
    );

// rev
/**
 * The NtUserMapPointsByVisualIdentifier routine transforms coordinates between visual spaces identified by visual IDs.
 *
 * \param SourceVisual A pointer to the 8-byte identifier of the source visual.
 * \param TargetVisual A pointer to the 8-byte identifier of the destination visual.
 * \param PointCount Number of points in the transform array.
 * \param Points A pointer to the array of PointCount points to transform.
 * \param MappedPoints Receives the PointCount transformed points.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMapPointsByVisualIdentifier(
    _In_ PVOID SourceVisual,
    _In_ PVOID TargetVisual,
    _In_ ULONG PointCount,
    _In_ PVOID Points,
    _Out_ PVOID MappedPoints
    );

// rev
/**
 * The NtUserMapVirtualKeyEx routine translates a virtual-key code into a scan code or character value, or vice versa.
 *
 * \param Code The virtual-key code or scan code for a key.
 * \param MapType The translation to be performed (MAPVK_*).
 * \param KeyboardLayout Optional handle to the keyboard layout (HKL) to use for translation.
 * \param UseLayout Non-zero to use the specified keyboard layout; 0 for default.
 * \return NTSTATUS value; the result contract remains unverified.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserMapVirtualKeyEx(
    _In_ ULONG Code,
    _In_ ULONG MapType,
    _In_opt_ HKL KeyboardLayout,
    _In_ LONG UseLayout
    );

// rev
/**
 * The NtUserMinInitialize routine performs minimal user-mode subsystem initialization for a process.
 *
 * \param Param1 Unconfirmed initialization context parameter.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserMinInitialize(
    _In_ LONG_PTR Param1
    );

// rev
/**
 * The NtUserModifyUserStartupInfoFlags routine performs a masked update of the calling process's USER
 * startup-information flags (the dwFlags carried in the process's STARTUPINFO, for example STARTF_*).
 *
 * The stored value is recomputed as (Flags & Mask) | (Current & ~Mask): only the bits selected by
 * Mask are replaced with the corresponding bits of Flags; all other bits are left unchanged.
 *
 * \param Mask Bit mask selecting which startup-info flag bits are modified.
 * \param Flags New values for the bits selected by Mask (bits outside Mask are ignored).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Per-process. To set bits use Mask = Flags = bits; to clear bits use Mask = bits, Flags = 0.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserModifyUserStartupInfoFlags(
    _In_ ULONG Mask,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserNotifyProcessCreate routine notifies win32k of process creation or initialization hints.
 *
 * \param ProcessId Identifier of the target process.
 * \param Param2 Unused in the examined implementation; original meaning is unresolved.
 * \param Param3 Unused in the examined implementation; original meaning is unresolved.
 * \param Flags Process creation hints.
 * \return NTSTATUS Successful or errant status.
 *
 * \remarks Returns STATUS_SUCCESS without further work when (Flags & ~0x30) is zero.
 *          Otherwise the caller must match the session's designated process or STATUS_ACCESS_DENIED is returned.
 *          The helper uses bit 0x10 to select Win32 process allocation. Without it, bit 0x01 selects
 *          initialization state 1; otherwise state 2 is selected. Bits 0x0C, or neither low bit being set,
 *          trigger telemetry assertions rather than immediate rejection.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserNotifyProcessCreate(
    _In_ ULONG ProcessId,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserPlayEventSound routine plays the specified user event sound.
 *
 * \param idSound The id sound (USER_SOUND_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_PLAYEVENTSOUND) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPlayEventSound(
    _In_ ULONG idSound // USER_SOUND_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserPrepareForLogoff routine prepares the session for user logoff.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_PREPAREFORLOGOFF) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPrepareForLogoff(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserProcessConnect routine connects a process to the win32k subsystem and establishes shared memory mapping.
 *
 * \param ProcessHandle Handle to the process connecting to win32k.
 * \param ConnectInfoLength Size, in bytes, of the ConnectInfo structure.
 * \param ConnectInfo Pointer to user-mode connection information structure.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserProcessConnect(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ConnectInfoLength,
    _Inout_ PVOID ConnectInfo
    );

// rev
/**
 * The NtUserProcessInkFeedbackCommand routine processes an inking feedback control command.
 *
 * \param Command Ink feedback command identifier.
 * \param Buffer Pointer to an input/output buffer containing command arguments.
 * \param BufferLength Size, in bytes, of Buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserProcessInkFeedbackCommand(
    _In_ ULONG Command,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

/**
 * The NtUserQueryInformationThread routine queries information about a GUI thread according to the specified information class.
 *
 * \param ThreadHandle A handle to the target thread.
 * \param ThreadInformationClass The thread information class selector (USERTHREADINFOCLASS).
 * \param ThreadInformation Pointer to a buffer receiving the thread information.
 * \param ThreadInformationLength The size of the buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _Inout_updates_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRealizePalette routine realizes the logical palette for the specified device context.
 *
 * \param hdc Handle to the device context whose palette is to be realized.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallOneParam(SFI_REALIZEPALETTE) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserRealizePalette(
    _In_ HDC hdc
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRegisterClassExWOW routine registers a window class with the window manager for 32-bit or WOW64 subsystems with extended attributes.
 *
 * \param WndClassEx A pointer to a WNDCLASSEXW structure containing class registration attributes.
 * \param Param2 Additional registration flags or WOW context parameter.
 * \param Param3 Additional WOW subsystem parameter.
 * \param Param4 Optional in-out pointer to class registration output data.
 * \param FnId Function identifier for internal window classes.
 * \param Flags Registration control flags.
 * \param Param7 Additional class registration parameter.
 * \return ULONG The atom identifying the registered class, or 0 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserRegisterClassExWOW(
    _In_ PVOID WndClassEx,
    _In_ ULONG_PTR Param2,
    _In_ ULONG_PTR Param3,
    _In_ PVOID Param4,
    _In_ USHORT FnId,
    _In_ ULONG Flags,
    _In_ ULONG_PTR Param7
    );

// rev
/**
 * The NtUserRegisterCoreMessagingEndPoint routine registers a CoreMessaging communications endpoint with the window manager subsystem.
 *
 * \param EndPointType The type identifier of the CoreMessaging endpoint.
 * \param EndPointInfo A pointer to the endpoint configuration information structure.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterCoreMessagingEndPoint(
    _In_ LONG_PTR EndPointType,
    _In_ PVOID EndPointInfo
    );

// rev
/**
 * The NtUserRegisterEdgy routine registers or unregisters edge gesture recognition listeners for touch and pointer input along screen edges.
 *
 * \param Count The number of listener entries in the buffer.
 * \param Listeners A pointer to an array of 32-byte listener registration structures.
 * \param Register Nonzero to register the listeners; zero to unregister.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterEdgy(
    _In_ ULONG Count,
    _In_reads_bytes_(32 * Count) PVOID Listeners,
    _In_ LONG Register
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterLogonProcess routine registers the logon process.
 *
 * \param ProcessId The process id.
 * \param LuidConnect The luid connect.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallTwoParam(SFI_REGISTERLOGONPROCESS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterLogonProcess(
    _In_ ULONG ProcessId,
    _In_ PLUID LuidConnect
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRegisterServicesProcess routine registers a process as the service controller/services process with the user subsystem.
 *
 * \param ProcessId The process identifier of the services process.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterServicesProcess(
    _In_ LONG ProcessId
    );

// rev
/**
 * The NtUserRegisterSessionPort routine registers an LPC or ALPC communication port for session management and notifications.
 *
 * \param SessionPort A pointer to the session communication port object or handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterSessionPort(
    _In_ PVOID SessionPort
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRegisterSystemThread routine registers the calling thread as a win32k system thread.
 *
 * \param Flags Thread registration flags (RST_*).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_REGISTERSYSTEMTHREAD) before WIN11.
 * In the examined implementation, RST_DONTATTACHQUEUE sets the calling thread's queue-merge
 * prohibition. Other bits are ignored, the prohibition is not cleared, and the call returns TRUE.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterSystemThread(
    _In_ ULONG Flags // RST_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserReleaseCapture routine releases the mouse capture held by the calling thread's window.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_RELEASECAPTURE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReleaseCapture(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserReleaseDwmHitTestWaiters routine releases threads or synchronization objects waiting on Desktop Window Manager (DWM) hit-testing results.
 *
 * \param Param1 Hit-test waiter identifier or notification parameter.
 * \return NTSTATUS Successful or errant status.
 */
// Note: this export folds onto a shared/stub address in the binary, so neither the
// argument count nor the types below could be confirmed by disassembly.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserReleaseDwmHitTestWaiters(
    _In_ ULONG_PTR Param1
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRemoteConnect routine performs remote-session connection setup for the USER subsystem.
 *
 * \param DoConnectData Pointer to the remote-connection descriptor.
 * \param ConnectFlags Remote connection flags.
 * \param DeviceName Name of the display or terminal device.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteConnect(
    _In_reads_bytes_(0x140) PVOID DoConnectData,
    _In_ ULONG ConnectFlags,
    _In_ PWSTR DeviceName
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteConnectState routine returns the remote connection state (CTX_W32_CONNECT_STATE_*).
 *
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTECONNECTSTATE) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserRemoteConnectState(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteConsoleShadowStop routine stops console shadowing for a remote session.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTECONSOLESHADOWSTOP) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteConsoleShadowStop(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteDisconnect routine disconnects the remote session.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTEDISCONNECT) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteDisconnect(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteNotify routine delivers a remote-session notification.
 *
 * \param DoNotifyData The do notify data.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallOneParam(SFI_REMOTENOTIFY) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteNotify(
    _In_ PDONOTIFYDATA DoNotifyData
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemotePassthruDisable routine disables remote pass-through drawing.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTEPASSTHRUDISABLE) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemotePassthruDisable(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemotePassthruEnable routine enables remote pass-through drawing.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTEPASSTHRUENABLE) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemotePassthruEnable(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteReconnect routine reconnects a remote session using the supplied connect data.
 *
 * \param DoConnectData The do connect data.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallOneParam(SFI_REMOTERECONNECT) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteReconnect(
    _In_ PDOCONNECTDATA DoConnectData
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteRedrawRectangle routine redraws a rectangular region of the remote screen.
 *
 * \param Left Left edge of the rectangle.
 * \param Top Top edge of the rectangle.
 * \param Right Right edge of the rectangle.
 * \param Bottom Bottom edge of the rectangle.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteRedrawRectangle(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRemoteRedrawScreen routine forces a full redraw of the remote session screen.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteRedrawScreen(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteShadowCleanup routine cleans up remote-session shadow state.
 *
 * \param ThinwireData The thinwire data.
 * \param ThinwireDataLength The thinwire data length.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallTwoParam(SFI_REMOTESHADOWCLEANUP) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowCleanup(
    _In_reads_bytes_(ThinwireDataLength) PVOID ThinwireData,
    _In_ ULONG ThinwireDataLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteShadowSetup routine prepares a remote session for shadowing.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTESHADOWSETUP) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowSetup(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteShadowStart routine starts shadowing of a remote session.
 *
 * \param ThinwireData The thinwire data.
 * \param ThinwireDataLength The thinwire data length.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallTwoParam(SFI_REMOTESHADOWSTART) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowStart(
    _In_reads_bytes_(ThinwireDataLength) PVOID ThinwireData,
    _In_ ULONG ThinwireDataLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteShadowStop routine stops shadowing of a remote session.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallNoParam(SFI_REMOTESHADOWSTOP) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowStop(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRemoteStopScreenUpdates routine suspends screen update broadcasting and rasterization for remote desktop display sessions.

 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteStopScreenUpdates(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserRemoteThinwireStats routine retrieves remote thinwire cache statistics.
 *
 * \param Stats Pointer to a CACHESTATISTICS structure receiving thinwire cache statistics.
 * \return NTSTATUS Successful or errant status.
 * \remarks Exposed via NtUserCallOneParam(SFI_REMOTETHINWIRESTATS) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteThinwireStats(
    _Out_ PCACHESTATISTICS Stats
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserRemoveInjectionDevice routine removes a synthetic input injection device handle previously created for device simulation.
 *
 * \param HDevice A handle to the injection device to remove.
 * \return TRUE if successful, or FALSE otherwise.
 * \remarks Native entry point for USER32!RemoveInjectionDevice.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRemoveInjectionDevice(
    _In_ HANDLE HDevice
    );

// rev
/**
 * The NtUserRemoveVisualIdentifier routine removes a visual identifier associated with a desktop or composition surface.
 *
 * \param Luid A pointer to the locally unique identifier (LUID) of the visual to remove.
 * \return TRUE if successful; otherwise, FALSE. Failure is reported via GetLastError (a status is
 * converted through RtlNtStatusToDosError).
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserRemoveVisualIdentifier(
    _In_ PLUID Luid
    );

// rev
/**
 * The NtUserReportInertia routine reports inertia physics calculations and friction modeling updates for an active pointer interaction.
 *
 * \param PointerId The identifier of the pointer interaction generating inertia.
 * \param Param2 Inertia calculation status or timing parameter.
 * \param Param3 In-out pointer to inertia velocity or state parameters.
 * \param Param4 In-out pointer to inertia displacement parameters.
 * \param Param5 In-out pointer to inertia limits or bounding information.
 * \param Param6 In-out pointer to inertia friction coefficients or deceleration rates.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReportInertia(
    _In_ ULONG_PTR PointerId,
    _In_ LONG Param2,
    _Inout_ PVOID Param3,
    _Inout_ PVOID Param4,
    _Inout_ PVOID Param5,
    _Inout_ PVOID Param6
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserResetDblClk routine resets the double-click tracking state.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_RESETDBLCLK) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserResetDblClk(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserScaleSystemMetricForDPIWithoutCache routine scales a system metric for the specified DPI without using the cache.
 *
 * \param Metric System metric index (SM_*) to scale.
 * \param ToDpi The to dpi.
 * \return The routine-specific result value.
 * \remarks Exposed via NtUserCallTwoParam(SFI_SCALESYSTEMMETRICFORDPIWITHOUTCACHE) before WIN11.
 */
_Must_inspect_result_
_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserScaleSystemMetricForDPIWithoutCache(
    _In_ ULONG Metric,
    _In_ ULONG ToDpi
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSelectPalette routine selects the specified logical palette into a device context.
 *
 * \param Hdc A handle to the device context.
 * \param PaletteHandle A handle to the logical palette to be selected.
 * \param ForceBackground Specifies whether the logical palette is forced to be a background palette.
 * \return A handle to the device context's previous logical palette, or NULL on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
HPALETTE
NTAPI
NtUserSelectPalette(
    _In_ HDC Hdc,
    _In_ HPALETTE PaletteHandle,
    _In_ ULONG ForceBackground
    );

/**
 * The NtUserSendInput routine synthesizes keystrokes, mouse motions, and button clicks.
 *
 * \param Count The number of structures in the Inputs array.
 * \param Inputs An array of INPUT structures that represent events to insert into the input stream.
 * \param Size The size, in bytes, of an INPUT structure.
 * \return The number of events successfully inserted into the input stream.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserSendInput(
    _In_ ULONG Count,
    _In_reads_(Count) LPINPUT Inputs,
    _In_ LONG Size
    );

// rev
/**
 * The NtUserSendInteractiveControlHapticsReport routine sends a haptic feedback vibration or waveform report to an interactive control device.
 *
 * \param DeviceId The identifier of the target interactive control device.
 * \param DataSize The size, in bytes, of the feedback data buffer.
 * \param FeedbackData A pointer to the haptic feedback waveform or instruction report buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSendInteractiveControlHapticsReport(
    _In_ USHORT DeviceId,
    _In_ LONG DataSize,
    _In_ PVOID FeedbackData
    );

// rev
/**
 * The NtUserSetAutoRotation routine enables or disables display auto-rotation based on orientation sensor data.
 *
 * \param Enable Nonzero to enable auto-rotation; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetAutoRotation(
    _In_ ULONG Enable
    );

// rev
/**
 * The NtUserSetCalibrationData routine configures digitizer or touch input calibration data in the window manager.
 *
 * \param Handle A handle to the calibration target object.
 * \param Param2 Calibration point count or mode index.
 * \param CalibrationData A pointer to the calibration data structure.
 * \param DataType The format or data type of the calibration buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCalibrationData(
    _In_ HANDLE Handle,
    _In_ ULONG Param2,
    _In_ PVOID CalibrationData,
    _In_ ULONG DataType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetDoubleClickTime routine sets the mouse double-click time.
 *
 * \param Milliseconds The time threshold in milliseconds.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETDOUBLECLICKTIME) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDoubleClickTime(
    _In_ ULONG Milliseconds
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetFeatureReportResponse routine sets HID feature report responses for synthetic or injected devices.
 *
 * \param Device A handle to the injection device.
 * \param Values An array of input injection value structures specifying the feature report response.
 * \param Count The number of items in the Values array.
 * \return TRUE if successful, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetFeatureReportResponse.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetFeatureReportResponse(
    _In_ HANDLE Device,
    _In_reads_(Count) const PINPUT_INJECTION_VALUE Values,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserSetForegroundRedirectionForActivationObject routine sets foreground activation redirection policies for an activation token or object.
 *
 * \param ActivationObject A pointer to the activation token or object.
 * \param RedirectionInfo A pointer to the foreground redirection parameters structure.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetForegroundRedirectionForActivationObject(
    _In_ PVOID ActivationObject,
    _In_ PVOID RedirectionInfo
    );

// rev
/**
 * Sets GUI system information; the inspected implementation supports Copilot-key remapping.
 * \param InformationClass Must be zero for Copilot-key remapping.
 * \param Information Pointer to a USHORT containing 0, VK_APPS (93), or VK_RCONTROL (163).
 * \param InformationLength Must be 2 for information class zero.
 * \return Nonzero on success, zero otherwise.
 * \remarks A disabled feature sets ERROR_NOT_SUPPORTED; a disallowed process role sets
 * ERROR_ACCESS_DENIED; an invalid class or key sets ERROR_INVALID_PARAMETER; a NULL buffer
 * or incorrect length sets ERROR_BAD_LENGTH; a missing I/O window station sets ERROR_INVALID_HANDLE.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetGUISystemInformation(
    _In_ ULONG InformationClass,
    _In_reads_bytes_(InformationLength) const VOID *Information,
    _In_ ULONG InformationLength
    );

/**
 * The NtUserSetInformationThread routine sets information for a GUI thread according to the specified information class.
 *
 * \param ThreadHandle A handle to the target thread.
 * \param ThreadInformationClass The thread information class selector (USERTHREADINFOCLASS).
 * \param ThreadInformation Pointer to a buffer containing the thread information.
 * \param ThreadInformationLength The size of the buffer in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

// rev
/**
 * The NtUserSetInputServiceState routine sets the operational state and availability flags of the system input service.
 *
 * \param State Input service state identifier or operating mode.
 * \param Enable Nonzero to enable the state; zero to disable.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetInputServiceState(
    _In_ ULONG State,
    _In_ LONG Enable
    );

// rev
/**
 * The NtUserSetInteractiveControlFocus routine sets input routing focus to a specific interactive control device or usage page.
 *
 * \param DeviceId The identifier of the interactive control device.
 * \param Usage The HID usage page or control usage identifier.
 * \param WindowHandle Optional handle to the window receiving focus, or NULL.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetInteractiveControlFocus(
    _In_ USHORT DeviceId,
    _In_ ULONG Usage,
    _In_opt_ HWND WindowHandle
    );

// rev
/**
 * The NtUserSetInteractiveCtrlRotationAngle routine configures the rotational orientation angle for an interactive controller component.
 *
 * \param DeviceId The identifier of the interactive control device.
 * \param Component The control component or axis index.
 * \param Angle The rotation angle value in degrees or device units.
 * \param Param4 Additional rotational configuration flags.
 * \param Param5 Reserved parameter.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetInteractiveCtrlRotationAngle(
    _In_ ULONG DeviceId,
    _In_ ULONG Component,
    _In_ LONG Angle,
    _In_ ULONG Param4,
    _In_ ULONG_PTR Param5
    );

// rev
/**
 * The NtUserSetJobUILimits routine configures user interface restrictions and permission limits for a job object.
 *
 * \param Handle A handle to the job object.
 * \param UIRestrictionsClass UI limit flags (e.g. JOB_OBJECT_UILIMIT_HANDLES, JOB_OBJECT_UILIMIT_READCLIPBOARD).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetJobUILimits(
    _In_ HANDLE Handle,
    _In_ ULONG UIRestrictionsClass
    );

// rev
/**
 * The NtUserSetKeyboardState routine copies an array of 256 byte keyboard key states into the calling thread's keyboard input state table.
 *
 * \param LpKeyState A pointer to a 256-byte array that contains keyboard key states.
 * \return TRUE if the function succeeds, or FALSE otherwise.
 * \remarks Native entry point for USER32!SetKeyboardState.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetKeyboardState(
    _In_reads_(256) PBYTE LpKeyState
    );

// rev
/**
 * The NtUserSetObjectInformation routine sets information about a window station or desktop object.
 *
 * \param ObjectHandle A handle to the window station or desktop object.
 * \param Index The object information category index (e.g. UOI_FLAGS).
 * \param Information A pointer to a buffer containing the object information to set.
 * \param Length The size, in bytes, of the Information buffer.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetObjectInformation(
    _In_ HANDLE ObjectHandle,
    _In_ LONG Index,
    _In_ PVOID Information,
    _In_ ULONG Length
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetProcessDefaultLayout routine sets the process default window layout.
 *
 * \param DefaultLayout The default layout (LAYOUT_* wingdi.h).
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETPROCESSDEFAULTLAYOUT) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessDefaultLayout(
    _In_ ULONG DefaultLayout // LAYOUT_* wingdi.h
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetProcessDpiAwarenessContext routine sets the process default DPI awareness context to a specified DPI_AWARENESS_CONTEXT value.
 *
 * \param DpiAwarenessContext The DPI awareness context to set for the process.
 * \param Flags Additional DPI configuration flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessDpiAwarenessContext(
    _In_ ULONG DpiAwarenessContext,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserSetProcessInteractionFlags routine configures per-process interaction behaviors. Each
 * parameter is stored as an independent boolean in the calling process's win32k PROCESSINFO state.
 *
 * \param EnableForegroundBoost TRUE to enable foreground activation boost.
 * \param EnableEnergyTracking TRUE to enable energy tracking.
 * \param EnableInputRouting TRUE to enable input routing.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Per-process. The three flags are written to separate PROCESSINFO fields.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessInteractionFlags(
    _In_ BOOL EnableForegroundBoost,
    _In_ BOOL EnableEnergyTracking,
    _In_ BOOL EnableInputRouting
    );

// rev
/**
 * The NtUserSetProcessLaunchForegroundPolicy routine configures foreground activation and window launch priority policy for a process.
 *
 * \param ProcessId The identifier of the target process.
 * \param Policy The foreground launch priority policy to apply.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetProcessLaunchForegroundPolicy(
    _In_ ULONG ProcessId,
    _In_ ULONG Policy
    );

// rev
/**
 * The NtUserSetProcessMousewheelRoutingMode routine sets the mouse wheel routing mode for the calling process (routing to active window or pointer window).
 *
 * \param Mode The mouse wheel routing mode to set (e.g. MOUSEWHEEL_ROUTING_FOCUS or MOUSEWHEEL_ROUTING_HYBRID).
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessMousewheelRoutingMode(
    _In_ ULONG Mode
    );

// rev
/**
 * The NtUserSetProcessRestrictionExemption routine enables or disables the calling process's exemption
 * from USER UI restrictions (for example the restrictions applied to processes in a UI-limited job).
 *
 * \param EnableExemption TRUE to grant the exemption to the current process, FALSE to revoke it.
 * \return TRUE if the exemption state was updated, FALSE otherwise.
 * \remarks Native entry point for USER32!SetProcessRestrictionExemption. Affects the calling process only.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetProcessRestrictionExemption(
    _In_ BOOL EnableExemption
    );

// rev
/**
 * The NtUserSetProcessUIAccessZorder routine marks the calling process as UIAccess for the purpose of
 * window z-order placement, allowing its top-level windows to sit above ordinary application windows.
 *
 * \return TRUE if successful; otherwise, FALSE.
 * \remarks Takes no parameters; it acts on the current process. Intended for UIAccess processes that
 * are not elevated (an elevated process already receives the equivalent z-order treatment).
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetProcessUIAccessZorder(
    VOID
    );

// rev
/**
 * The NtUserSetProcessWin32Capabilities routine grants or revokes win32k capabilities for one or more processes.
 *
 * Each entry is copied from user mode and validated against the PROC_CAP_*_VALID_MASK masks
 * (Flags1/Flags2 accept bits 0-2, EnableMask/DisableMask accept bit 0); an entry whose lanes set
 * any reserved bit is rejected. For every valid entry the routine attaches to the target process's
 * session and applies the requested capability changes there.
 *
 * \param Capabilities Array of per-process capability requests (USER_PROCESS_CAP_ENTRY).
 * \param Count Number of entries in the Capabilities array.
 * \return TRUE if the capabilities were applied, FALSE otherwise. On failure the thread's last
 * error is set (ERROR_ACCESS_DENIED when the caller lacks SeTcbPrivilege).
 * \remarks The caller must hold SeTcbPrivilege (TCB); the routine calls HasTcbPrivilege first and,
 * if the privilege is not held, sets the last error to ERROR_ACCESS_DENIED and returns without
 * touching any target process. Reserved for the window manager / session broker.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessWin32Capabilities(
    _In_reads_(Count) const USER_PROCESS_CAP_ENTRY* Capabilities,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserSetSensorPresence routine reports sensor presence states (such as human presence or proximity sensors) to the window manager.
 *
 * \param Presence Bitmask or indicator of sensor presence state.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetSensorPresence(
    _In_ ULONG Presence
    );

// rev
/**
 * The NtUserSetSysColors routine sets the colors for the specified display elements in the user interface.
 *
 * \param Count The number of display elements in the Elements array.
 * \param Elements An array of integers specifying the display elements to be set.
 * \param Colors An array of COLORREF values containing the new color values.
 * \param Flags Flags specifying display update and notification behavior.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetSysColors(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID Elements,
    _In_reads_(Count) PVOID Colors,
    _In_ ULONG Flags
    );

// rev
/**
 * The NtUserSetSystemContentRects routine configures bounding rectangle coordinates for system content areas and shell chrome.
 *
 * \param Count The number of rectangle structures in the buffer.
 * \param Rects A pointer to an array of RECT structures defining content boundaries.
 * \return TRUE if successful; otherwise, FALSE.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSetSystemContentRects(
    _In_ ULONG Count,
    _In_ PVOID Rects
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetTSFEventState routine sets the Text Services Framework (TSF) event state for the calling thread.
 *
 * \param StateFlags TSF event state flags.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETTSFEVENTSTATE) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetTSFEventState(
    _In_ ULONG StateFlags
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * Sets or clears the calling GUI thread's resource-brokering association.
 * \param Reserved Must be zero; nonzero sets ERROR_INVALID_PARAMETER.
 * \param ThreadId Target GUI thread identifier. Zero or the current thread clears the association.
 * \return Nonzero on success, zero on failure.
 * \remarks A missing target sets ERROR_INVALID_PARAMETER. Caller and target process-role
 * checks can set ERROR_ACCESS_DENIED. A different target must have Win32k syscall filtering
 * or filtering audit enabled.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetTargetForResourceBrokering(
    _In_ ULONG Reserved,
    _In_ ULONG ThreadId
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetThreadInputBlocked routine blocks or unblocks input delivery to the specified thread.
 *
 * \param ThreadId Identifier of the target thread.
 * \param InputBlocked TRUE to block input, FALSE to unblock.
 * \return TRUE on success, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadInputBlocked(
    _In_ ULONG ThreadId,
    _In_ BOOL InputBlocked
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetThreadLayoutHandles routine sets the input locale identifier and keyboard layout handles for the calling thread.
 *
 * \param KeyboardLayout The input locale identifier (HKL) to set.
 * \param LayoutHandle The internal keyboard layout object handle.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadLayoutHandles(
    _In_ HKL KeyboardLayout,
    _In_ HKL LayoutHandle
    );

// rev
/**
 * The NtUserSetThreadState routine sets thread-specific GUI state flags and attributes in the window manager.
 *
 * \param State The thread state bitmask to apply.
 * \param Mask The mask specifying which state bits are being modified.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadState(
    _In_ LONG State,
    _In_ LONG Mask
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSetUserObjectCapability routine adds or removes a capability access-allowed ACE on a USER object.
 *
 * \param UserObjectHandle Handle to a USER object owned by the current process.
 * \param AccessMask Access rights for the capability access-allowed ACE.
 * \param CapabilitySid SID identifying the capability.
 * \param Remove FALSE to add the ACE; TRUE to remove a matching ACE.
 * \return TRUE on success, FALSE otherwise.
 * \remarks The ACE is stored in the DACL of the USER object's internal security descriptor.
 * Operations using HMValidateHandleWithDescriptor check this descriptor through HMSDCheck,
 * which calls SeAccessCheck with the caller's captured security subject context, the operation's
 * requested access mask, and the object type's generic mapping. A failed check causes handle
 * validation to fail. Adding an ACE grants the specified rights to callers whose tokens satisfy
 * the ACE, subject to the complete access check. Removing an ACE removes that grant; other ACEs
 * may still grant access. This object-level DACL is distinct from the per-message capability
 * descriptors configured by NtUserSetWindowMessageCapability.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetUserObjectCapability(
    _In_ HANDLE UserObjectHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID CapabilitySid,
    _In_ BOOL Remove
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSetWatermarkStrings routine sets the desktop watermark strings.
 *
 * \param StringTable The string table.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SETWATERMARKSTRINGS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWatermarkStrings(
    _In_ PCUNICODE_STRING StringTable
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserShellHandwritingDelegateInput routine delegates shell handwriting input streams to a target thread or handwriting recognition service.
 *
 * \param ThreadId The thread identifier receiving the delegated handwriting input.
 * \param Param2 Input stream configuration or target parameters.
 * \param Param3 Additional handwriting delegation flags.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellHandwritingDelegateInput(
    _In_ ULONG ThreadId,
    _In_ LONG_PTR Param2,
    _In_ LONG_PTR Param3
    );

// rev
/**
 * The NtUserShellHandwritingHandleDelegatedInput routine processes delegated handwriting input packets in the shell input pipeline.
 *
 * \param Input A pointer to a 48-byte buffer containing handwriting input data.
 * \param InputType The handwriting input data packet type.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellHandwritingHandleDelegatedInput(
    _In_reads_bytes_(48) PVOID Input,
    _In_ ULONG InputType
    );

// rev
/**
 * The NtUserShellHandwritingUndelegateInput routine revokes handwriting input delegation from the specified target thread.
 *
 * \param ThreadId The thread identifier from which handwriting input is undelegated.
 * \param InputType The handwriting input type being revoked.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShellHandwritingUndelegateInput(
    _In_ ULONG ThreadId,
    _In_ ULONG InputType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserShowStartGlass routine shows the application start-glass (busy) cursor.
 *
 * \param Timeout The duration in milliseconds to display the startup cursor.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SHOWSTARTGLASS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowStartGlass(
    _In_ ULONG Timeout
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSignalRedirectionStartComplete routine signals completion of visual input redirection initialization in the window manager.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSignalRedirectionStartComplete(
    VOID
    );

// rev
/**
 * The NtUserSlicerControl routine controls display slice buffers and frame presentation timing.
 *
 * \param Param1 Slicer control operation or device handle.
 * \param Param2 Slicer configuration parameter.
 * \param Param3 Slicer buffer parameter.
 * \param Param4 Additional slicer flags.
 * \return NTSTATUS Successful or errant status.
 */
// Note: this export folds onto a shared/stub address in the binary, so neither the
// argument count nor the types below could be confirmed by disassembly.
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSlicerControl(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2,
    _In_ ULONG_PTR Param3,
    _In_ ULONG_PTR Param4
    );

// rev
/**
 * The NtUserSoundSentry routine triggers a visual signal when an application sounds a system alert sound.
 *
 * \return TRUE if the function succeeds; otherwise, FALSE.
 * \remarks Native entry point for USER32!SoundSentry.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserSoundSentry(
    VOID
    );

// rev
/**
 * The NtUserStopAndEndInertia routine terminates active inertia physics calculation processing for a pointer interaction.
 *
 * \param PointerId The identifier of the pointer interaction.
 * \param Flags Flags specifying inertia stopping behavior.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserStopAndEndInertia(
    _In_ LONG_PTR PointerId,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserSwapMouseButton routine swaps or restores the left and right mouse buttons.
 *
 * \param SwapButtons The swap buttons.
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallOneParam(SFI_SWAPMOUSEBUTTON) before WIN11.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSwapMouseButton(
    _In_ LOGICAL SwapButtons
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserSystemParametersInfo routine queries or sets the values of various system-wide parameters.
 *
 * \param Action The system-wide parameter to be queried or set (e.g. SPI_GETNONCLIENTMETRICS).
 * \param UiParam A parameter whose usage and format depend on the system parameter being queried or set.
 * \param PvParam A parameter whose usage and format depend on the system parameter being queried or set.
 * \param Flags If a system parameter is being set, specifies whether the user profile is updated.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSystemParametersInfo(
    _In_ ULONG Action,
    _In_ ULONG UiParam,
    _Inout_ ULONG_PTR PvParam,
    _In_ CHAR Flags
    );

// rev
/**
 * The NtUserSystemParametersInfoForDpi routine queries or sets the values of system-wide parameters scaled for a specific DPI value.
 *
 * \param Action The system parameter to be queried or set.
 * \param UiParam Parameter whose usage depends on the action.
 * \param PvParam Pointer to buffer whose format depends on the action.
 * \param Flags Profile update flags.
 * \param Dpi The DPI value to scale parameters to.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSystemParametersInfoForDpi(
    _In_ LONG Action,
    _In_ LONG UiParam,
    _Inout_ PVOID PvParam,
    _In_ LONG_PTR Flags,
    _In_ LONG Dpi
    );

/**
 * The NtUserTestForInteractiveUser routine tests whether the specified logon session belongs to the interactive user.
 *
 * \param AuthenticationId Pointer to the logon authentication ID (LUID) to test.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserTestForInteractiveUser(
    _In_ PLUID AuthenticationId
    );

// rev
/**
 * The NtUserToUnicodeEx routine translates the specified virtual-key code and keyboard state to the corresponding Unicode character or characters.
 *
 * \param VirtualKey The virtual-key code to be translated.
 * \param ScanCode The hardware scan code of the key to be translated.
 * \param KeyState A pointer to a 256-byte array that contains the current keyboard state.
 * \param Buffer The buffer that receives the translated Unicode character or characters.
 * \param BufferSize The size, in characters, of the buffer.
 * \param Flags Behavior flags (e.g. menu is active).
 * \param KeyboardLayout An optional input locale identifier (HKL) used to translate the code.
 * \return LONG The number of characters written, 0 if none, or a negative value for a dead key.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserToUnicodeEx(
    _In_ ULONG VirtualKey,
    _In_ ULONG ScanCode,
    _In_reads_bytes_(256) PVOID KeyState,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ LONG BufferSize,
    _In_ CHAR Flags,
    _In_opt_ HKL KeyboardLayout
    );

// rev
/**
 * The NtUserTraceLoggingSendMixedModeTelemetry routine emits mixed-mode application telemetry events via TraceLogging.
 *
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserTraceLoggingSendMixedModeTelemetry(
    VOID
    );

/**
 * The NtUserTrackMouseEvent routine posts messages when the mouse pointer leaves a window or hovers over a window for a specified amount of time.
 *
 * \param EventTrack Pointer to a TRACKMOUSEEVENT structure that contains tracking information.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserTrackMouseEvent(
    _Inout_ LPTRACKMOUSEEVENT EventTrack
    );

/**
 * The NtUserUnhookWinEvent routine removes an event hook function created by a previous call to SetWinEventHook.
 *
 * \param WinEventHookHandle Handle to the event hook returned in the previous call to SetWinEventHook.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserUnhookWinEvent(
    _In_ HWINEVENTHOOK WinEventHookHandle
    );

// rev
/**
 * The NtUserUnloadKeyboardLayout routine unloads an input locale identifier (formerly called a keyboard layout).
 *
 * \param KeyboardLayout The input locale identifier to be unloaded.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnloadKeyboardLayout(
    _In_ HKL KeyboardLayout
    );

// rev
/**
 * The NtUserUnregisterClass routine unregisters a window class, freeing the memory required for the class.
 *
 * \param ClassName A pointer to a UNICODE_STRING specifying the class name or class atom.
 * \param Instance A handle to the instance of the module that created the class.
 * \param ClassMenuNames An output pointer receiving class menu resource names.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnregisterClass(
    _In_ PUNICODE_STRING ClassName,
    _In_ HINSTANCE Instance,
    _Out_ PVOID ClassMenuNames
    );

// rev
/**
 * Unregisters the current session DWM port and performs DWM process shutdown cleanup.
 * \return TRUE on success. Non-DWM callers receive FALSE and ERROR_ACCESS_DENIED.
 * \remarks This routine takes no arguments.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserUnregisterSessionPort(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUpdatePerUserImmEnabling routine updates the per-user IMM (IME) enabling state.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_UPDATEPERUSERIMMENABLING) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdatePerUserImmEnabling(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserUpdatePerUserSystemParameters routine updates user-specific system settings and environment parameters from the registry.
 *
 * \param Flags Flags specifying which user profile sections to reload.
 * \return LOGICAL Non-zero on success, zero otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdatePerUserSystemParameters(
    _In_ ULONG Flags
    );

/**
 * The NtUserUserHandleGrantAccess routine grants or denies access to a handle to a User object to a job that has a user-interface restriction.
 *
 * \param UserHandle A handle to the User object.
 * \param Job A handle to the job to be granted access to the User object.
 * \param Grant If TRUE, access is granted; if FALSE, access is denied.
 * \return TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserUserHandleGrantAccess(
    _In_ HANDLE UserHandle,
    _In_ HANDLE Job,
    _In_ BOOL Grant
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserUserPowerCalloutWorker routine runs the user-mode power callout worker.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_USERPOWERCALLOUTWORKER) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUserPowerCalloutWorker(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtUserVkKeyScanEx routine translates a character to the corresponding virtual-key code and shift state for the specified keyboard layout.
 *
 * \param Character The character to be translated into a virtual-key code.
 * \param KeyboardLayout An optional input locale identifier (HKL) used to translate the character.
 * \param UseLayout Nonzero to use KeyboardLayout; zero to use the thread's default layout.
 * \return LONG The virtual-key code in the low byte and the shift state in the high byte, or -1 on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtUserVkKeyScanEx(
    _In_ USHORT Character,
    _In_opt_ HKL KeyboardLayout,
    _In_ LONG UseLayout
    );

// rev

// rev
/**
 * The NtUserWaitForInputIdle routine waits until the specified process is waiting for user input with no input pending, or until the time-out interval has elapsed.
 *
 * \param ProcessHandle A handle to the process.
 * \param Timeout The time-out interval, in milliseconds.
 * \param Flags Wait flags or options.
 * \return ULONG 0 if the process is idle, WAIT_TIMEOUT on time-out, or WAIT_FAILED on failure.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtUserWaitForInputIdle(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Timeout,
    _In_ LONG Flags
    );

// rev
/**
 * The NtUserWaitForRedirectionStartComplete routine waits for visual input redirection subsystem startup to complete.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtUserWaitForRedirectionStartComplete(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtUserZapActiveAndFocus routine clears the active-window and focus-window state.
 *
 * \return TRUE on success, FALSE otherwise.
 * \remarks Exposed via NtUserCallNoParam(SFI_ZAPACTIVEANDFOCUS) before WIN11.
 */
_Success_(return != 0)
_Kernel_entry_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserZapActiveAndFocus(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

// rev
/**
 * The NtVisualCaptureBits routine captures pixel bits from a composition visual.
 *
 * \param VisualId The target visual identifier.
 * \param Param2 Second capture parameter.
 * \param Param3 Third capture parameter.
 * \param Param4 Fourth capture parameter.
 * \param Left Left coordinate of the capture rectangle.
 * \param Top Top coordinate of the capture rectangle.
 * \param Flags Capture flags.
 * \param CaptureInfo A pointer to capture information.
 * \param SectionHandle A handle to the shared section backing the capture bits.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtVisualCaptureBits(
    _In_ ULONG VisualId,
    _In_ ULONG Param2,
    _In_ ULONG Param3,
    _In_ ULONG Param4,
    _In_ LONG Left,
    _In_ LONG Top,
    _In_ LONG Flags,
    _In_ PVOID CaptureInfo,
    _In_ HANDLE SectionHandle
    );

// rev
/**
 * The PackDDElParam routine packs DDE message parameters into a single lParam.
 *
 * \param Msg The posted DDE message identifier.
 * \param UiLo The low-order value of the parameter pair.
 * \param UiHi The high-order value of the parameter pair.
 * \return LPARAM Packed DDE parameter handle.
 */
NTSYSAPI
LPARAM
NTAPI
PackDDElParam(
    _In_ ULONG Msg,
    _In_ ULONG_PTR UiLo,
    _In_ ULONG_PTR UiHi
    );

// rev
/**
 * The PolyPatBlt routine executes multiple pattern bit-block transfer operations.
 *
 * \param Hdc A handle to the device context.
 * \param Rop Raster operation code.
 * \param PolyData A pointer to an array of PATRECT structures.
 * \param Count The number of rectangles.
 * \param Mode Pattern blit mode.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
PolyPatBlt(
    _In_ HDC Hdc,
    _In_ ULONG Rop,
    _In_reads_(Count) PVOID PolyData,
    _In_ ULONG Count,
    _In_ LONG Mode
    );

// rev
/**
 * The PrivateRegisterICSProc routine registers a private input-context-switch callback.
 *
 * \param Callback Pointer to the input-context-switch callback procedure.
 */
NTSYSAPI
BOOL
NTAPI
PrivateRegisterICSProc(
    _In_ PVOID Callback
    );

// rev
/**
 * The ReasonCodeNeedsBugID routine determines whether a shutdown reason code requires a bug identifier.
 *
 * \param ReasonCode Shutdown reason code to test.
 * \return BOOL TRUE if the code requires a bug identifier, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
ReasonCodeNeedsBugID(
    _In_ LONG ReasonCode
    );

// rev
/**
 * The ReasonCodeNeedsComment routine determines whether a shutdown reason code requires a comment.
 *
 * \param ReasonCode Shutdown reason code to test.
 * \return BOOL TRUE if the code requires a comment, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
ReasonCodeNeedsComment(
    _In_ LONG ReasonCode
    );

// rev
/**
 * The RecordShutdownReason routine records the reason for a system shutdown.
 *
 * \param ShutdownReason Pointer to the shutdown reason data structure.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
RecordShutdownReason(
    _In_ PVOID ShutdownReason
    );

// rev
/**
 * The RegisterLogonProcess routine registers the logon process (winlogon) with the window manager.
 *
 * \param ProcessId Client process identifier of the logon process.
 * \param LuidConnect Pointer to the locally unique identifier (LUID) for the logon session.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserRegisterLogonProcess.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterLogonProcess(
    _In_ ULONG ProcessId,
    _In_ PLUID LuidConnect
    );

// rev
/**
 * The RegisterServicesProcess routine registers the services (session 0) process with the window manager.
 *
 * \param ProcessId Process identifier of the services host.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserRegisterServicesProcess system call.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterServicesProcess(
    _In_ LONG ProcessId
    );

// rev
/**
 * The RegisterSessionPort routine registers a session port with the window manager.
 *
 * \param SessionPort Pointer to the session port communication channel.
 * \return ULONG_PTR Status code or port registration token.
 * \remarks Forwards to the NtUserRegisterSessionPort system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
RegisterSessionPort(
    _In_ PVOID SessionPort
    );

// rev
/**
 * The RegisterSystemThread routine registers the calling thread as a window-manager system thread.
 *
 * \param Flags Registration flags forwarded to NtUserRegisterSystemThread (RST_*).
 * \param Reserved Must be zero to invoke the native routine.
 * \return LOGICAL The native result when Reserved is zero. Undefined otherwise.
 * When Reserved is nonzero, returns without invoking the native routine or initializing the result.
 * RST_DONTATTACHQUEUE sets the calling thread's queue-merge prohibition; other flag bits are
 * ignored by the examined native implementation. This call does not clear the prohibition.
 */
NTSYSAPI
LOGICAL
NTAPI
RegisterSystemThread(
    _In_ ULONG Flags, // RST_*
    _In_ ULONG Reserved
    );

// rev
/**
 * The ReleaseDwmHitTestWaiters routine releases threads waiting on DWM hit-testing operations.
 *
 * \param WaitFlags Flags specifying which DWM hit-test wait operations to release.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserReleaseDwmHitTestWaiters system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ReleaseDwmHitTestWaiters(
    _In_ ULONG WaitFlags
    );

// rev
/**
 * The RemoveInjectionDevice routine removes a synthetic input injection device handle previously created for device simulation.
 *
 * \param DeviceHandle A handle to the injection device to remove.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserRemoveInjectionDevice system call.
 */
NTSYSAPI
BOOL
NTAPI
RemoveInjectionDevice(
    _In_ HANDLE DeviceHandle
    );

// rev
/**
 * The RemoveThreadTSFEventAwareness routine removes Text Services Framework event awareness from the calling thread.
 *
 * \param StateFlags State flags to remove.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserSetTSFEventState.
 */
NTSYSAPI
LOGICAL
NTAPI
RemoveThreadTSFEventAwareness(
    _In_ ULONG StateFlags
    );

// rev
/**
 * The RemoveVisualIdentifier routine removes a visual identifier previously added with AddVisualIdentifier.
 *
 * \param Luid Pointer to the locally unique identifier (LUID) to remove.
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserRemoveVisualIdentifier system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
RemoveVisualIdentifier(
    _In_ PLUID Luid
    );

// rev
/**
 * The ReuseDDElParam routine reuses a packed DDE lParam for a reply message.
 *
 * \param LParam The packed lParam parameter being reused.
 * \param MsgIn The received incoming DDE message identifier.
 * \param MsgOut The outgoing response DDE message identifier.
 * \param UiLo Low-order parameter value for the new message.
 * \param UiHi High-order parameter value for the new message.
 * \return LPARAM Reused packed DDE parameter handle.
 */
NTSYSAPI
LPARAM
NTAPI
ReuseDDElParam(
    _In_ LPARAM LParam,
    _In_ ULONG MsgIn,
    _In_ ULONG MsgOut,
    _In_ ULONG_PTR UiLo,
    _In_ ULONG_PTR UiHi
    );

// rev
/**
 * The ScaleRgn routine scales a region by specified factors.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ScaleRgn(
    VOID
    );

// rev
/**
 * The ScaleValues routine scales an array of coordinate values.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
ScaleValues(
    VOID
    );

// rev
/**
 * The SetFeatureReportResponse routine sets HID feature report responses for synthetic or injected devices.
 *
 * \param DeviceHandle A handle to the injection device.
 * \param Values An array of input injection value structures specifying the feature report response.
 * \param Count The number of items in the Values array.
 * \return BOOL TRUE on success, FALSE on failure.
 * \remarks Forwards to the NtUserSetFeatureReportResponse system call.
 */
NTSYSAPI
BOOL
NTAPI
SetFeatureReportResponse(
    _In_ HANDLE DeviceHandle,
    _In_reads_(Count) const PINPUT_INJECTION_VALUE Values,
    _In_ ULONG Count
    );

// rev
/**
 * The SetForegroundRedirectionForActivationObject routine sets foreground redirection for an activation object.
 *
 * \param ActivationObject Pointer to the activation object.
 * \param RedirectionInfo Pointer to redirection information data.
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserSetForegroundRedirectionForActivationObject system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetForegroundRedirectionForActivationObject(
    _In_ PVOID ActivationObject,
    _In_ PVOID RedirectionInfo
    );

// rev
/**
 * The SetLayoutWidth routine sets the layout width for a device context.
 *
 * \param Hdc A handle to the device context.
 * \param Width The layout width in pixels.
 * \param Layout Layout flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
SetLayoutWidth(
    _In_ HDC Hdc,
    _In_ ULONG Width,
    _In_ ULONG Layout
    );
    
// rev
/**
 * The SetOPMSigningKeyAndSequenceNumbers routine sets OPM signing keys and initial sequence numbers.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetOPMSigningKeyAndSequenceNumbers(
    VOID
    );

// rev
/**
 * The SetProcessDpiAwarenessInternal routine sets the DPI-awareness of the calling process.
 *
 * \param Awareness DPI awareness value (PROCESS_DPI_AWARENESS).
 * \return ULONG_PTR Status code or HRESULT.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetProcessDpiAwarenessInternal(
    _In_ LONG Awareness
    );

// rev
/**
 * The SetProcessLaunchForegroundPolicy routine sets the foreground policy applied when the process launches windows.
 *
 * \param ProcessId Client process identifier.
 * \param Policy Foreground policy flags.
 * \return ULONG_PTR Status code or result.
 * \remarks Forwards to the NtUserSetProcessLaunchForegroundPolicy system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetProcessLaunchForegroundPolicy(
    _In_ LONG ProcessId,
    _In_ LONG Policy
    );

// rev
/**
 * The SetRelAbs routine sets relative or absolute coordinate mode for a device context.
 *
 * \param Hdc A handle to the device context.
 * \param Mode The coordinate mode (RELABS enum / RELATIVE = 1, ABSOLUTE = 2).
 * \return Previous coordinate mode, or 0 on failure.
 */
NTSYSAPI
LONG
NTAPI
SetRelAbs(
    _In_ HDC Hdc,
    _In_ LONG Mode
    );

// rev
/**
 * The SetSysColorsTemp routine temporarily overrides the system colors.
 *
 * \param Colors Optional pointer to an array of RGB color values.
 * \param Reserved Reserved parameter (or brushes array).
 * \param Count Number of color elements (at most 31).
 * \return ULONG_PTR Handle to previous temporary color state, or 1 on restore, or 0 on failure.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetSysColorsTemp(
    _In_reads_opt_(Count) const ULONG *Colors,
    _In_opt_ PVOID Reserved,
    _In_ ULONG_PTR Count
    );

// rev
/**
 * The SetThreadInputBlocked routine blocks or unblocks input processing for a thread.
 *
 * \param ThreadId Identifier of the thread whose input is blocked or unblocked.
 * \param InputBlocked TRUE to block input processing; FALSE to unblock.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserSetThreadInputBlocked.
 */
NTSYSAPI
LOGICAL
NTAPI
SetThreadInputBlocked(
    _In_ ULONG ThreadId,
    _In_ BOOL InputBlocked
    );

// rev
/**
 * The SetUserObjectCapability routine adds or removes a capability access-allowed ACE on a USER object.
 *
 * \param UserObjectHandle Handle to a USER object owned by the current process.
 * \param AccessMask Access rights for the capability access-allowed ACE.
 * \param CapabilitySid SID identifying the capability.
 * \param Remove FALSE to add the ACE; TRUE to remove a matching ACE.
 * \return LOGICAL TRUE on success, FALSE otherwise.
 * \remarks Thin user32 wrapper over NtUserSetUserObjectCapability.
 */
_Success_(return != 0)
NTSYSAPI
LOGICAL
NTAPI
SetUserObjectCapability(
    _In_ HANDLE UserObjectHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID CapabilitySid,
    _In_ BOOL Remove
    );

// rev
/**
 * The SetVirtualResolution routine configures the virtual resolution of a device context.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SetVirtualResolution(
    VOID
    );

// rev
/**
 * The ShellHandwritingHandleDelegatedInput routine handles input delegated to the shell handwriting surface.
 *
 * \param Input Pointer to input buffer containing handwriting data.
 * \param InputType Input event type.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserShellHandwritingHandleDelegatedInput system call.
 */
NTSYSAPI
LOGICAL
NTAPI
ShellHandwritingHandleDelegatedInput(
    _In_reads_bytes_(48) PVOID Input,
    _In_ ULONG InputType
    );

// rev
/**
 * The ShellHandwritingUndelegateInput routine cancels input delegation to the shell handwriting surface.
 *
 * \param ThreadId Target thread identifier.
 * \param InputType Input event type to cancel.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Forwards to the NtUserShellHandwritingUndelegateInput system call.
 */
NTSYSAPI
LOGICAL
NTAPI
ShellHandwritingUndelegateInput(
    _In_ ULONG ThreadId,
    _In_ ULONG InputType
    );

// rev
/**
 * The ShowStartGlass routine shows or hides the Start menu glass effect.
 *
 * \param Timeout Animation timeout in milliseconds.
 * \return LOGICAL Non-zero on success, zero otherwise.
 * \remarks Thin user32 wrapper over NtUserShowStartGlass.
 */
NTSYSAPI
LOGICAL
NTAPI
ShowStartGlass(
    _In_ ULONG Timeout
    );

// rev
/**
 * The SignalRedirectionStartComplete routine signals that visual redirection initialization has finished.
 *
 * \param RedirectionToken Token identifying the redirection session.
 * \param Status Completion status code.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserSignalRedirectionStartComplete system call.
 */
NTSYSAPI
ULONG_PTR
NTAPI
SignalRedirectionStartComplete(
    _In_ HANDLE RedirectionToken,
    _In_ NTSTATUS Status
    );

// rev
/**
 * The StartFormPage routine starts a form page during document printing.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
StartFormPage(
    VOID
    );

// rev
/**
 * The UnpackDDElParam routine unpacks DDE message parameters from a packed lParam.
 *
 * \param Msg The posted DDE message identifier.
 * \param LParam The packed message parameter.
 * \param PuiLo Pointer receiving the low-order parameter value.
 * \param PuiHi Pointer receiving the high-order parameter value.
 * \return BOOL TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOL
NTAPI
UnpackDDElParam(
    _In_ ULONG Msg,
    _In_ LPARAM LParam,
    _In_ PULONG_PTR PuiLo,
    _In_ PULONG_PTR PuiHi
    );

// rev
/**
 * The UnregisterSessionPort routine unregisters a session communication port from the window manager.
 *
 * \param PortHandle Handle or identifier of the session port to unregister.
 * \return ULONG_PTR Status code.
 * \remarks Forwards to the NtUserUnregisterSessionPort system call.
 */
NTSYSAPI
BOOL
NTAPI
UnregisterSessionPort(
    VOID
    );

// rev
/**
 * The UpdatePerUserSystemParameters routine reloads the per-user system parameters.
 *
 * \param Flags System parameters update flags (SPI_*).
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
UpdatePerUserSystemParameters(
    _In_ ULONG Flags
    );

// rev
/**
 * The User32InitializeImmEntryTable routine initializes the IMM entry-point table inside user32.
 *
 * \param Magic Initialization magic number or flags.
 * \return ULONG_PTR Status code or result.
 */
NTSYSAPI
ULONG_PTR
NTAPI
User32InitializeImmEntryTable(
    _In_ LONG Magic
    );

// rev
/**
 * The UserClientDllInitialize routine performs client-side DLL initialization for user32.dll.
 *
 * \param ModuleHandle Handle to the user32 module instance.
 * \param Reason Reason code for the initialization call (e.g. DLL_PROCESS_ATTACH).
 * \param Context Context parameter passed from the loader.
 * \return BOOL TRUE on successful initialization, FALSE on failure.
 * \remarks Internal user32 DLL entry point.
 */
NTSYSAPI
BOOL
NTAPI
UserClientDllInitialize(
    _In_ HINSTANCE ModuleHandle,
    _In_ ULONG Reason,
    _In_opt_ PVOID Context
    );

// rev
/**
 * The UserLpkPSMTextOut routine is a Language Pack callback that draws prefix or mnemonic text.
 *
 * \param Hdc Handle to the device context.
 * \param X Horizontal coordinate of the reference point.
 * \param Y Vertical coordinate of the reference point.
 * \param String Pointer to the character string.
 * \param StringLength Number of characters in the string.
 * \param Flags Text formatting flags.
 * \return BYTE TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BYTE
NTAPI
UserLpkPSMTextOut(
    _In_ HDC Hdc,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ PCWSTR String,
    _In_ ULONG StringLength,
    _In_ LONG Flags
    );

// rev
/**
 * The UserLpkTabbedTextOut routine renders tabbed text using Language Pack (LPK) formatting.
 *
 * \param Hdc Handle to the device context.
 * \param X Initial horizontal coordinate for text output.
 * \param Y Initial vertical coordinate for text output.
 * \param String Pointer to the character string to draw.
 * \param Count Number of characters in the string.
 * \param TabPositionsCount Number of tab stops in the TabPositions array.
 * \param TabPositions Array of horizontal tab stop positions.
 * \param TabOrigin Horizontal coordinate from which tab stops are measured.
 * \param Format Text formatting and layout flags.
 * \param Chp Optional character property overrides.
 * \param DrawState Optional draw state context structure.
 * \return LONG Tabbed text dimensions or status.
 * \remarks Internal LPK text output routine.
 */
NTSYSAPI
LONG
NTAPI
UserLpkTabbedTextOut(
    _In_ HDC Hdc,
    _In_ LONG X,
    _In_ LONG Y,
    _In_reads_(Count) LPCWSTR String,
    _In_ LONG Count,
    _In_ LONG TabPositionsCount,
    _In_reads_opt_(TabPositionsCount) CONST LONG *TabPositions,
    _In_ LONG TabOrigin,
    _In_ ULONG Format,
    _In_opt_ PVOID Chp,
    _In_opt_ PVOID DrawState
    );

// rev
/**
 * The UserRealizePalette routine realizes a GDI palette through the window manager.
 *
 * \param hdc Handle to the device context whose palette is to be realized.
 * \return ULONG Number of entries in the logical palette mapped to the system palette.
 * \remarks Thin user32 wrapper over NtUserRealizePalette.
 */
NTSYSAPI
ULONG
NTAPI
UserRealizePalette(
    _In_ HDC hdc
    );

// rev
/**
 * The UserRegisterWowHandlers routine registers the WOW (16/32-bit) thunk dispatch handlers with user32.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
UserRegisterWowHandlers(
    VOID
    );

// rev
/**
 * The UspAllocCache routine allocates cache memory for Uniscribe text shaping.
 *
 * \param Size The buffer size to allocate.
 * \param Buffer A pointer receiving the allocated cache buffer.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
UspAllocCache(
    _In_ LONG Size,
    _Out_ PVOID* Buffer
    );

    
// rev
/**
 * The UspAllocTemp routine allocates temporary buffer memory for Uniscribe.
 *
 * \param Size The buffer size to allocate.
 * \param Buffer A pointer receiving the allocated temporary buffer.
 * \return A status code, size, or result value.
 *
NTAPI
UspAllocTemp(
    _In_ LONG Size,
    _Out_ PVOID* Buffer
    );

// rev
/**
 * The UspFreeMem routine frees memory allocated by Uniscribe.
 *
 * \param LpMem A pointer to the memory pointer to free.
 * \return Integer status code.
 */
NTSYSAPI
LONG
NTAPI
UspFreeMem(
    _Inout_ PVOID * LpMem
    );

#ifdef PHNT_ENABLE_DEPRECATED_STUBS

// rev
/**
 * The VRipOutput routine writes diagnostic error information to the debug logging destination.
 *
 * \param OutputLevel Verbosity or severity level of the diagnostic output.
 * \param Format Null-terminated format string.
 * \return VOID
 * \remarks Internal debug routine.
 */
NTSYSAPI
VOID
NTAPIV
VRipOutput(
    _In_ ULONG OutputLevel,
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    );
#endif // PHNT_ENABLE_DEPRECATED_STUBS

#ifdef PHNT_ENABLE_DEPRECATED_STUBS

// rev
/**
 * The VTagOutput routine writes tagged diagnostic information to the debug logging destination.
 *
 * \param TagLevel Diagnostic tag category or severity level.
 * \param Format Null-terminated format string.
 * \return VOID
 * \remarks Internal debug routine.
 */
NTSYSAPI
VOID
NTAPIV
VTagOutput(
    _In_ ULONG TagLevel,
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    );
#endif // PHNT_ENABLE_DEPRECATED_STUBS

// rev
/**
 * The WCSToMBEx routine converts a wide-character Unicode string to a multibyte character string using extended options.
 *
 * \param CodePage Code page identifier to use for the conversion.
 * \param UnicodeString Pointer to the input wide-character string.
 * \param CharsInUnicodeString Number of characters in the input string, or -1 if null-terminated.
 * \param MultiByteString Pointer to a buffer receiving the converted multibyte string, or receiving an allocated buffer.
 * \param MbSize Size, in bytes, of the destination buffer.
 * \param Allocate Non-zero to request allocation of the output buffer if necessary.
 * \return ULONG_PTR The number of bytes written to the destination buffer, or 0 on failure.
 */
NTSYSAPI
ULONG_PTR
NTAPI
WCSToMBEx(
    _In_ USHORT CodePage,
    _In_ PCWSTR UnicodeString,
    _In_ LONG CharsInUnicodeString,
    _Inout_ PCHAR* MultiByteString,
    _In_ ULONG MbSize,
    _In_ LONG Allocate
    );

// rev
/**
 * The WaitForRedirectionStartComplete routine waits for visual input redirection initialization to complete.
 *
 * \return BOOL TRUE if successful, FALSE otherwise.
 * \remarks Forwards to the NtUserWaitForRedirectionStartComplete system call.
 */
NTSYSAPI
BOOL
NTAPI
WaitForRedirectionStartComplete(
    VOID
    );

// rev
/**
 * The _UserTestTokenForInteractive routine verifies whether a security token represents an interactive logon session.
 *
 * \param TokenHandle Handle to the access token to test.
 * \param InteractiveFlags Pointer receiving interactive session validation flags or attributes.
 * \return NTSTATUS Successful or errant status.
 * \remarks Internal security routine.
 */
NTSYSAPI
NTSTATUS
NTAPI
_UserTestTokenForInteractive(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PULONG64 InteractiveFlags
    );

// rev
/**
 * The bMakePathNameW routine constructs a fully qualified Unicode path name.
 *
 * \param LpBuffer Pointer to buffer receiving path name.
 * \param LpFileName Input file name string.
 * \param LpFilePart Pointer receiving address of file part within buffer.
 * \param Flags Optional pointer receiving flags.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
bMakePathNameW(
    _In_ ULONG_PTR LpBuffer,
    _In_ ULONG_PTR LpFileName,
    _Inout_ PVOID LpFilePart,
    _Out_opt_ PULONG Flags
    );

// rev
/**
 * The cGetTTFFromFOT routine extracts a TrueType font (TTF) file name from a font resource (FOT).
 *
 * \param SourceString Source FOT file path.
 * \param Param2 Second parameter.
 * \param Param3 Buffer receiving TTF file path.
 * \param Param4 Size of output buffer in bytes.
 * \param Param5 Fifth parameter.
 * \param Param6 Sixth parameter.
 * \param Param7 Seventh parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
cGetTTFFromFOT(
    _Inout_ PVOID SourceString,
    _In_ ULONG Param2,
    _Inout_ PSTR Param3,
    _Inout_ PULONG Param4,
    _In_ LONG_PTR Param5,
    _In_ LONG_PTR Param6,
    _In_ LONG Param7
    );

// rev
/**
 * fpClosePrinter is a gdi32.dll DATA export, not a function.
 * Encoded ClosePrinter pointer, not a directly callable function.
 * gdi32full!GdiGetSpoolFileHandle decodes it with RtlDecodePointer before use.
 */
NTSYSAPI extern PVOID fpClosePrinter;

// rev
/**
 * The ftsWordBreak routine performs word break processing on text strings.
 *
 * \param Param1 First parameter.
 * \param Param2 Second parameter.
 * \param Param3 Third parameter.
 * \param Param4 Fourth parameter.
 * \param Param5 Fifth parameter.
 * \return A status code, size, or result value.
 */
NTSYSAPI
LONG_PTR
NTAPI
ftsWordBreak(
    _In_ LONG Param1,
    _In_ LONG Param2,
    _Inout_ PVOID Param3,
    _In_ LONG Param4,
    _In_ LONG Param5
    );

// rev
/**
 * gCookie is a gdi32.dll DATA export, not a function.
 * Pointer-sized cookie loaded into RDX at gdi32!0x1800013f6.
 */
NTSYSAPI extern ULONG_PTR gCookie;

// rev
/**
 * gW32PID is a gdi32.dll DATA export, not a function.
 * 32-bit GDI process identifier, compared with EAX at gdi32!0x180001170.
 */
NTSYSAPI extern ULONG gW32PID;

// rev
/**
 * g_systemCallFilterId is a gdi32.dll DATA export, not a function.
 * 32-bit system-call filter identifier, stored from EAX at gdi32!0x180005f32.
 */
NTSYSAPI extern ULONG g_systemCallFilterId;

// rev
/**
 * The gdiPlaySpoolStream routine plays a print spool stream.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
gdiPlaySpoolStream(
    VOID
    );

// rev
/**
 * ghICM is a gdi32.dll DATA export, not a function.
 * Pointer-sized ICM state/handle storage; semantic pointee type remains opaque.
 */
NTSYSAPI 
extern 
PVOID ghICM;

// rev
/**
 * The hGetPEBHandle routine retrieves a GDI client handle from the Process Environment Block (PEB).
 *
 * \param CacheType GDI PEB cache type selector.
 * \param Value Cached handle value or key.
 * \return ULONG_PTR Cached handle value, or 0 on failure.
 */
NTSYSAPI
ULONG_PTR
NTAPI
hGetPEBHandle(
    _In_ LONG CacheType,
    _In_ ULONG Value
    );
/**
 * The pldcGet routine retrieves a pointer to the local device context (LDC).
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
pldcGet(
    VOID
    );
/**
 * The vSetPldc routine sets the local device context (LDC) pointer.
 *
 * \return A pointer-sized status, handle, or value.
 */
NTSYSAPI
ULONG_PTR
NTAPI
vSetPldc(
    VOID
    );

#include <ntusermissing.h>

#endif // _NTUSER_H
