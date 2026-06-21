/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2021
 *
 */

#include <phapp.h>
#include <mapldr.h>
#include <appresolver.h>

#include <mainwnd.h>
#include <notifico.h>
#include <notiftoast.h>

#include <wrl.h>
#include <roapi.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>

#include <atomic>
#include <memory>

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(_HR_) do { const auto __hr = _HR_; if (HR_FAILED(__hr)) { return __hr; }} while (false)
#endif

#define PH_TOAST_THREAD_QUIT (WM_APP + 0x7f)

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

namespace PH
{
    enum ToastNotificationPriority
    {
        ToastNotificationPriority_Default = 0,
        ToastNotificationPriority_High = 1
    };

    MIDL_INTERFACE("15154935-28ea-4727-88e9-c58680e2d118")
    IToastNotification4 : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_Data(
            _COM_Outptr_ IInspectable** Value
            ) = 0;

        virtual HRESULT STDMETHODCALLTYPE put_Data(
            _In_ IInspectable* Value
            ) = 0;

        virtual HRESULT STDMETHODCALLTYPE get_Priority(
            _Out_ ToastNotificationPriority* Value
            ) = 0;

        virtual HRESULT STDMETHODCALLTYPE put_Priority(
            _In_ ToastNotificationPriority Value
            ) = 0;
    };

    /*!
        @brief Simple template wrapper around PhGetModuleProcAddress

        @tparam T - Function pointer type.

        @param[out] FunctionPointer - On success set to the address of the
         function.
        @param[in] Module - Module name to look up the function from.
        @param[in] Function - Function name.

        @return True when the function pointer container the address of the
         function, false otherwise.
    */
    template <typename T>
    bool GetModuleProcAddress(
        _Out_ T* FunctionPointer,
        _In_ const wchar_t* Module,
        _In_ const char* Function
        )
    {
        *FunctionPointer = static_cast<T>(PhGetModuleProcAddress(Module, Function));
        return (*FunctionPointer != nullptr);
    }

    /*!
        @brief Wrapper around RoGetActivationFactory to make it a little less clunky to use.

        @details PhActivateToastRuntime must be called prior to this else the function returns
         R_NOTIMPL.

        @tparam I - Interface to retrieve.
        @tparam t_SizeDestName - Length of the name string to use.
        @tparam t_SizeDestClass - Length of the class string to use.

        @param[in] ActivatableClassId - Activatable class ID to look up the interface from.
        @param[out] Interface - On success, set to a referenced interface of type T.

        @return Appropriate result status.
    */
    template <typename I, size_t t_SizeDestName, size_t t_SizeDestClass>
    _Must_inspect_result_
    HRESULT RoGetActivationFactory(
        _In_ wchar_t const (&DllName)[t_SizeDestName],
        _In_ wchar_t const (&ActivatableClassId)[t_SizeDestClass],
        _COM_Outptr_ I** Interface
        )
    {
        HRESULT hr = PhGetActivationFactory(
                                      DllName,
                                      ActivatableClassId,
                                      __uuidof(I),
                                      reinterpret_cast<void**>(Interface)
                                      );
        if (HR_FAILED(hr))
        {
            *Interface = nullptr;
        }
        return hr;
    }

    using IToastActivatedHandler = ITypedEventHandler<ToastNotification*, IInspectable*>;
    using IToastDismissedHandler = ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*>;
    using IToastFailedHandler = ITypedEventHandler<ToastNotification*, ToastFailedEventArgs*>;

    /*!
        @brief System Informer toast event handler, this class implements the
         handler interfaces for IToastNotification and store the callback
         and context from the C interface.
    */
    class ToastEventHandler final : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                                  IToastActivatedHandler,
                                                  IToastDismissedHandler,
                                                  IToastFailedHandler>
    {
    public:

        HRESULT RuntimeClassInitialize(
            PPH_TOAST_CALLBACK ToastCallback,
            PVOID Context
            )
        {
            m_ToastCallback = ToastCallback;
            m_Context = Context;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IInspectable* Args
            ) override;

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastDismissedEventArgs* Args
            ) override;

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastFailedEventArgs* Args
            ) override;

        void SetActivatedToken(const EventRegistrationToken& Token)
        {
            m_ActivatedToken = Token;
        }

        void SetDismissedToken(const EventRegistrationToken& Token)
        {
            m_DismissedToken = Token;
        }

        void SetFailedToken(const EventRegistrationToken& Token)
        {
            m_FailedToken = Token;
        }

        void SetCallbackThreadId(ULONG ThreadId)
        {
            m_CallbackThreadId = ThreadId;
        }

        void Complete(
            _In_ IToastNotification* Sender
            )
        {
            if (!m_Unhooked.exchange(true))
            {
                Sender->remove_Dismissed(m_DismissedToken);
                Sender->remove_Activated(m_ActivatedToken);
                Sender->remove_Failed(m_FailedToken);
            }

            if (m_CallbackThreadId)
                PostThreadMessage(m_CallbackThreadId, PH_TOAST_THREAD_QUIT, 0, 0);
        }

    private:

        PPH_TOAST_CALLBACK m_ToastCallback{ nullptr };
        PVOID m_Context{ nullptr };
        ULONG m_CallbackThreadId{ 0 };

        EventRegistrationToken m_ActivatedToken{};
        EventRegistrationToken m_DismissedToken{};
        EventRegistrationToken m_FailedToken{};
        std::atomic<bool> m_Unhooked{ false };

    };

    /*!
        @brief System Informer toast object.
    */
    class Toast
    {
    public:

        ~Toast() = default;

        Toast() = default;

        _Must_inspect_result_
        HRESULT Initialize(
            _In_ PPH_STRINGREF ApplicationId,
            _In_ PPH_STRINGREF ToastXml,
            _In_opt_ ULONG TimeoutMilliseconds,
            _In_opt_ PH_TOAST_PRIORITY Priority,
            _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
            _In_opt_ PVOID Context,
            _In_opt_ ULONG CallbackThreadId = 0
            );

        [[nodiscard]]
        HRESULT Show() const;

    private:

        ComPtr<IToastNotifier> m_Notifier{ nullptr };
        ComPtr<IToastNotification> m_Toast{ nullptr };

    };
}

_Must_inspect_result_
HRESULT PH::Toast::Initialize(
    _In_ PPH_STRINGREF ApplicationId,
    _In_ PPH_STRINGREF ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PH_TOAST_PRIORITY Priority,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context,
    _In_opt_ ULONG CallbackThreadId
    )
{
    HSTRING_REFERENCE stringAppIdRef;
    HSTRING_REFERENCE stringToastXmlRef;

    ComPtr<IToastNotificationManagerStatics> manager;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationManagerStatics>(
        L"WpnApps.dll",
        L"Windows.UI.Notifications.ToastNotificationManager",
        &manager));

    ComPtr<IToastNotificationFactory> factory;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationFactory>(
        L"WpnApps.dll",
        L"Windows.UI.Notifications.ToastNotification",
        &factory));

    ComPtr<IToastNotifier> notifier;
    RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(ApplicationId, &stringAppIdRef));
    RETURN_IF_FAILED(manager->CreateToastNotifierWithId(HSTRING_FROM_STRING(stringAppIdRef), &notifier));

    //
    // Couldn't find a nice way to just create a IXmlDocument... There is
    // probably a way to but I guess I'm lazy. I'm just going to grab a
    // template one from the manager interface then overwrite it with the
    // input.
    //
    ComPtr<IXmlDocument> xmlDocument;
    RETURN_IF_FAILED(manager->GetTemplateContent(
        ToastTemplateType::ToastTemplateType_ToastImageAndText02,
        &xmlDocument));

    //
    // The IXmlDocument we get here implements IXmlDocumentIO thankfully so
    // just query for it, we'll replace the content using this interface.
    //
    ComPtr<IXmlDocumentIO> xmlIo;
    RETURN_IF_FAILED(xmlDocument.As(&xmlIo));

    //
    // Load the supplied XML, if it's invalid this will return an error.
    //
    // Note, if it's valid XAML but you include an image file that is not
    // found we will end up returning success but the callback will never
    // get invoked. So if you allocated the context through to the C interface
    // it will leak... We could solve this by requiring a context cleanup
    // callback from the C interface. Honestly, that's kind of overkill, don't
    // include an image that can't be found.
    //
    RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(ToastXml, &stringToastXmlRef));
    RETURN_IF_FAILED(xmlIo->LoadXml(HSTRING_FROM_STRING(stringToastXmlRef)));

    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(factory->CreateToastNotification(xmlDocument.Get(), &toast));

    if (Priority != PhToastPriorityDefault)
    {
        ComPtr<PH::IToastNotification4> toast4;
        HRESULT status;

        if (Priority != PhToastPriorityHigh)
            return E_INVALIDARG;

        status = toast.As(&toast4);

        if (status != E_NOINTERFACE)
        {
            RETURN_IF_FAILED(status);
            RETURN_IF_FAILED(toast4->put_Priority(PH::ToastNotificationPriority_High));
        }
    }

    if (TimeoutMilliseconds > 0)
    {
        //
        // We have a timeout, set it.
        //
        ComPtr<IPropertyValueStatics> propertyStats;
        RETURN_IF_FAILED(PH::RoGetActivationFactory<IPropertyValueStatics>(
            L"WinTypes.dll",
            L"Windows.Foundation.PropertyValue",
            &propertyStats));

        LARGE_INTEGER sysTime;
        PhQuerySystemTime(&sysTime);

        sysTime.QuadPart += UInt32x32To64(TimeoutMilliseconds, PH_TIMEOUT_MS);

        DateTime time;
        time.UniversalTime = sysTime.QuadPart;

        ComPtr<IInspectable> inspectable;
        RETURN_IF_FAILED(propertyStats->CreateDateTime(time, &inspectable));

        ComPtr<IReference<DateTime>> dateTime;
        RETURN_IF_FAILED(inspectable.As(&dateTime));

        RETURN_IF_FAILED(toast->put_ExpirationTime(dateTime.Get()));
    }

    if (ToastCallback)
    {
        //
        // We have a timeout callback. Create the handler with the callback
        // and context, then shove it into the toast notification.
        //
        ComPtr<PH::ToastEventHandler> callbacks;
        ComPtr<PH::IToastActivatedHandler> activatedNotif;
        ComPtr<PH::IToastDismissedHandler> dismissedNotif;
        ComPtr<PH::IToastFailedHandler> failedNotif;

        RETURN_IF_FAILED(MakeAndInitialize<PH::ToastEventHandler>(
            &callbacks,
            ToastCallback,
            Context));
        callbacks->SetCallbackThreadId(CallbackThreadId);

        RETURN_IF_FAILED(callbacks.As(&activatedNotif));
        RETURN_IF_FAILED(callbacks.As(&dismissedNotif));
        RETURN_IF_FAILED(callbacks.As(&failedNotif));

        EventRegistrationToken token;

        RETURN_IF_FAILED(toast->add_Activated(activatedNotif.Get(), &token));
        callbacks->SetActivatedToken(token);

        RETURN_IF_FAILED(toast->add_Dismissed(dismissedNotif.Get(), &token));
        callbacks->SetDismissedToken(token);

        RETURN_IF_FAILED(toast->add_Failed(failedNotif.Get(), &token));
        callbacks->SetFailedToken(token);
    }

    m_Notifier = notifier;
    m_Toast = toast;

    return S_OK;
}

HRESULT PH::Toast::Show() const
{
    if ((m_Notifier == nullptr) || (m_Toast == nullptr))
    {
        return E_NOT_VALID_STATE;
    }
    return m_Notifier->Show(m_Toast.Get());
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastDismissedEventArgs* Args
    )
{
    PH_TOAST_REASON phReason = PhToastReasonUnknown;
    ToastDismissalReason reason;
    HRESULT result;

    result = Args->get_Reason(&reason);

    if (HR_SUCCESS(result))
    {
        switch (reason)
        {
        case ToastDismissalReason_UserCanceled:
            phReason = PhToastReasonUserCanceled;
            break;
        case ToastDismissalReason_ApplicationHidden:
            phReason = PhToastReasonApplicationHidden;
            break;
        case ToastDismissalReason_TimedOut:
            phReason = PhToastReasonTimedOut;
            break;
        }
    }

    m_ToastCallback(result, phReason, m_Context);

    //
    // Without this the ref is never decremented and the runtime leaks...
    // We fire to the callback once anyway, so remove the callback from
    // processing.
    //
    Complete(Sender);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IInspectable* Args
    )
{
    m_ToastCallback(S_OK, PhToastReasonActivated, m_Context);

    //
    // Without this the ref is never decremented and the runtime leaks...
    // We fire to the callback once anyway, so remove the callback from
    // processing.
    //
    Complete(Sender);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastFailedEventArgs* Args
    )
{
    HRESULT err;
    HRESULT hr = Args->get_ErrorCode(&err);
    if (HR_FAILED(hr))
    {
        //
        // You wut? I doubt this could realistically fail... if so give
        // the callback this error, I guess...
        //
        err = hr;
    }

    m_ToastCallback(err, PhToastReasonError, m_Context);

    //
    // Without this the ref is never decremented and the runtime leaks...
    // We fire to the callback once anyway, so remove the callback from
    // processing.
    //
    Complete(Sender);
    return S_OK;
}

// ---------------------------------------------------------------------------
// Dedicated toast pump thread
//
// WinRT toast notifications deliver Activated/Dismissed/Failed events as COM
// callbacks that are only dispatched while the creating thread pumps messages
// in an STA. Rather than require every caller to run its own message loop, all
// V2 toast COM work (create/show/update/hide/release) is marshaled onto a
// single long-lived STA pump thread owned here. Callbacks fire on this thread;
// callers are fire-and-forget. See PhpToastThreadInvoke.
// ---------------------------------------------------------------------------

#define WM_PH_TOAST_INVOKE (WM_APP + 0x80)
#define WM_PH_TOAST_QUIT   (WM_APP + 0x81)

typedef HRESULT (NTAPI* PPH_TOAST_INVOKE_ROUTINE)(
    _In_opt_ PVOID Context
    );

typedef struct _PH_TOAST_INVOKE_WORK
{
    SLIST_ENTRY ListEntry;
    PPH_TOAST_INVOKE_ROUTINE Routine;
    PVOID Context;
    HANDLE CompletionEvent;
    HRESULT Result;
} PH_TOAST_INVOKE_WORK, *PPH_TOAST_INVOKE_WORK;

static PH_INITONCE PhpToastThreadInitOnce = PH_INITONCE_INIT;
static HANDLE PhpToastThreadHandle = NULL;
static HANDLE PhpToastThreadReadyEvent = NULL;
static ULONG PhpToastThreadId = 0;
static SLIST_HEADER PhpToastInvokeQueue;
static volatile LONG PhpToastInvokePending = 0;

static VOID PhpToastDrainInvokeQueue(
    VOID
    )
{
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedFlushSList(&PhpToastInvokeQueue);

    while (listEntry)
    {
        PPH_TOAST_INVOKE_WORK work = CONTAINING_RECORD(listEntry, PH_TOAST_INVOKE_WORK, ListEntry);
        listEntry = listEntry->Next;

        work->Result = work->Routine(work->Context);
        NtSetEvent(work->CompletionEvent, NULL);
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PhpToastPumpThread(
    _In_opt_ PVOID Parameter
    )
{
    MSG message;

    PhSetThreadName(NtCurrentThread(), L"ToastPumpThread");

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Force this thread to own a message queue before anyone posts to it.
    PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    PhpToastThreadId = HandleToUlong(NtCurrentThreadId());
    NtSetEvent(PhpToastThreadReadyEvent, NULL);

    while (GetMessage(&message, NULL, 0, 0))
    {
        if (message.message == WM_PH_TOAST_QUIT)
            break;

        if (message.message == WM_PH_TOAST_INVOKE)
        {
            // Clear the pending flag before draining so an item pushed during
            // the drain always triggers a fresh post (never a lost wakeup).
            InterlockedExchange(&PhpToastInvokePending, 0);
            PhpToastDrainInvokeQueue();
            continue;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    // Release any stragglers so their waiters don't hang on shutdown.
    PhpToastDrainInvokeQueue();

    CoUninitialize();

    return STATUS_SUCCESS;
}

static VOID PhpToastEnsurePumpThread(
    VOID
    )
{
    if (PhBeginInitOnce(&PhpToastThreadInitOnce))
    {
        PhInitializeSListHead(&PhpToastInvokeQueue);

        if (NT_SUCCESS(PhCreateEvent(
            &PhpToastThreadReadyEvent,
            EVENT_ALL_ACCESS,
            SynchronizationEvent,
            FALSE
            )))
        {
            if (NT_SUCCESS(PhCreateThreadEx(&PhpToastThreadHandle, PhpToastPumpThread, NULL)))
            {
                NtWaitForSingleObject(PhpToastThreadReadyEvent, FALSE, NULL);
            }
        }

        PhEndInitOnce(&PhpToastThreadInitOnce);
    }
}

// Executes Routine on the toast pump thread and returns its result
// synchronously. If the caller is already on the pump thread the routine runs
// directly to avoid self-deadlock.
static HRESULT PhpToastThreadInvoke(
    _In_ PPH_TOAST_INVOKE_ROUTINE Routine,
    _In_opt_ PVOID Context
    )
{
    PH_TOAST_INVOKE_WORK work;

    PhpToastEnsurePumpThread();

    if (!PhpToastThreadHandle)
        return E_FAIL;

    if (HandleToUlong(NtCurrentThreadId()) == PhpToastThreadId)
        return Routine(Context);

    memset(&work, 0, sizeof(work));
    work.Routine = Routine;
    work.Context = Context;
    work.Result = E_FAIL;

    if (!NT_SUCCESS(PhCreateEvent(
        &work.CompletionEvent,
        EVENT_ALL_ACCESS,
        SynchronizationEvent,
        FALSE
        )))
    {
        return E_OUTOFMEMORY;
    }

    RtlInterlockedPushEntrySList(&PhpToastInvokeQueue, &work.ListEntry);

    if (InterlockedCompareExchange(&PhpToastInvokePending, 1, 0) == 0)
        PostThreadMessage(PhpToastThreadId, WM_PH_TOAST_INVOKE, 0, 0);

    NtWaitForSingleObject(work.CompletionEvent, FALSE, NULL);
    NtClose(work.CompletionEvent);

    return work.Result;
}

/*!
    @brief Initializes the required functionality for toast notifications.

    @details Each successful call to PhInitializeToastRuntime should be paired with a call to
     PhUninitializeToastRuntime (including S_FALSE). This function must be called prior to a call
     to PhShowToast. Generally, this should be called once during start of the process.

    @return Appropriate result status. On success (including S_FALSE) this call must be paired
     with a call to PhUninitializeToastRuntime.
*/
_Must_inspect_result_
HRESULT PhInitializeToastRuntime()
{
//    HRESULT res;
//
//    res = g_RoInitialize(RO_INIT_MULTITHREADED);
//    if (res == RPC_E_CHANGED_MODE)
//        res = g_RoInitialize(RO_INIT_SINGLETHREADED);
//    if (res == S_FALSE) // already initialized
//        res = S_OK;
//
//    return res;
    return S_OK;
}

/*!
    @brief This is a wrapper to call RoUninitialize. This should only be called when
     PhInitializeToastRuntime return a success (including S_FALSE).
*/
VOID PhUninitializeToastRuntime()
{
    if (PhpToastThreadHandle)
    {
        if (PhpToastThreadId)
            PostThreadMessage(PhpToastThreadId, WM_PH_TOAST_QUIT, 0, 0);

        NtWaitForSingleObject(PhpToastThreadHandle, FALSE, NULL);
        NtClose(PhpToastThreadHandle);
        PhpToastThreadHandle = NULL;
    }

    if (PhpToastThreadReadyEvent)
    {
        NtClose(PhpToastThreadReadyEvent);
        PhpToastThreadReadyEvent = NULL;
    }

    //if (g_RoInitialize)
    //{
    //    g_RoUninitialize();
    //}
}

typedef struct _PH_TOAST_THREAD_CONTEXT
{
    PPH_STRING ApplicationId;
    PPH_STRING ToastXml;
    ULONG TimeoutMilliseconds;
    PH_TOAST_PRIORITY Priority;
    PPH_TOAST_CALLBACK ToastCallback;
    PVOID Context;
    HANDLE InitializedEvent;
    HANDLE ResultReadEvent;
    HRESULT Result;
} PH_TOAST_THREAD_CONTEXT, *PPH_TOAST_THREAD_CONTEXT;

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PhpToastNotificationThread(
    _In_opt_ PVOID Parameter
    )
{
    PPH_TOAST_THREAD_CONTEXT context = (PPH_TOAST_THREAD_CONTEXT)Parameter;
    MSG message;

    PhSetThreadName(NtCurrentThread(), L"ToastNotificationThread");

    // Ensure this thread owns a message queue before registering callbacks.
    PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    {
        auto toast = std::make_unique<PH::Toast>();

        context->Result = toast->Initialize(
            &context->ApplicationId->sr,
            &context->ToastXml->sr,
            context->TimeoutMilliseconds,
            context->Priority,
            context->ToastCallback,
            context->Context,
            HandleToUlong(NtCurrentThreadId())
            );

        if (HR_SUCCESS(context->Result))
            context->Result = toast->Show();

        NtSetEvent(context->InitializedEvent, NULL);
        NtWaitForSingleObject(context->ResultReadEvent, FALSE, NULL);

        if (HR_SUCCESS(context->Result))
        {
            while (GetMessage(&message, NULL, 0, 0))
            {
                if (message.message == PH_TOAST_THREAD_QUIT)
                    break;

                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }

    PhClearReference(&context->ApplicationId);
    PhClearReference(&context->ToastXml);
    PhFree(context);

    return STATUS_SUCCESS;
}

/*!
    @brief Shows a toast notification.

    @details PhInitializeToastRuntime must be called prior. This is an asynchronous operation. If
     the caller supplies a context and the function returns a success, the context is owned by the
     toast object, any allocated memory should be freed when the toast callback is invoked. On
     failure, the caller should free any memory allocated for the toast callback context, callback
     will not be invoked. Note, if you ToastXml is valid but contains something like an image
     that can't be located by the runtime this function will still return success, don't write . See comments
     in PH::Toast::Initialize.

    @code
            hr = PhShowToast(L"System Informer",
                             L"<toast>"
                             L"    <visual>"
                             L"       <binding template=\"ToastImageAndText02\">"
                             L"            <image id=\"1\" src=\".\\ToastImage.png\" alt=\"red graphic\"/>"
                             L"            <text id=\"1\">Example Title</text>"
                             L"            <text id=\"2\">This is an example toast notification, hello world!</text>"
                             L"        </binding>"
                             L"    </visual>"
                             L"</toast>",
                             30 * 1000,
                             ToastCallback,
                             NULL);
    @endcode

    @param[in] ApplicationId - Name for the application showing the toast.
    @param[in] ToastXml - XAML formatted \<toast\> to display.
    @param[in] TimeoutMilliseconds - Optional, when elapsed the toast is considered timed out. If
     0 is passed the default system defined timeout for toast notifications is used..
    @param[in] ToastCallback - Optional, if supplied the toast registers handlers for toast
     interaction and the callback is invoked with the relevant PH_TOAST_REASON.
    @param[in] Context - Option, context passed to the toast notification callback.

    @return Appropriate result status. On S_OK the toast is dispatched and the callback
     will be invoked when the interaction occurs. On failure the callback will not be invoked
     the caller should free any context.
*/
_Must_inspect_result_
HRESULT PhShowToastStringRef(
    _In_ PPH_STRINGREF ApplicationId,
    _In_ PPH_STRINGREF ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    return PhShowToastStringRefEx(
        ApplicationId,
        ToastXml,
        TimeoutMilliseconds,
        PhToastPriorityDefault,
        ToastCallback,
        Context
        );
}

_Must_inspect_result_
HRESULT PhShowToastStringRefEx(
    _In_ PPH_STRINGREF ApplicationId,
    _In_ PPH_STRINGREF ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PH_TOAST_PRIORITY Priority,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    if (ToastCallback)
    {
        PPH_TOAST_THREAD_CONTEXT threadContext;
        HANDLE initializedEvent;
        HANDLE resultReadEvent;
        HANDLE threadHandle;
        NTSTATUS status;
        HRESULT result;

        status = PhCreateEvent(
            &initializedEvent,
            EVENT_ALL_ACCESS,
            SynchronizationEvent,
            FALSE
            );

        if (!NT_SUCCESS(status))
            return HRESULT_FROM_NT(status);

        status = PhCreateEvent(
            &resultReadEvent,
            EVENT_ALL_ACCESS,
            SynchronizationEvent,
            FALSE
            );

        if (!NT_SUCCESS(status))
        {
            NtClose(initializedEvent);
            return HRESULT_FROM_NT(status);
        }

        threadContext = (PPH_TOAST_THREAD_CONTEXT)PhAllocateZero(sizeof(PH_TOAST_THREAD_CONTEXT));
        threadContext->ApplicationId = PhCreateString2(ApplicationId);
        threadContext->ToastXml = PhCreateString2(ToastXml);
        threadContext->TimeoutMilliseconds = TimeoutMilliseconds;
        threadContext->Priority = Priority;
        threadContext->ToastCallback = ToastCallback;
        threadContext->Context = Context;
        threadContext->InitializedEvent = initializedEvent;
        threadContext->ResultReadEvent = resultReadEvent;
        threadContext->Result = E_FAIL;

        status = PhCreateThreadEx(
            &threadHandle,
            PhpToastNotificationThread,
            threadContext
            );

        if (!NT_SUCCESS(status))
        {
            PhClearReference(&threadContext->ApplicationId);
            PhClearReference(&threadContext->ToastXml);
            PhFree(threadContext);
            NtClose(initializedEvent);
            NtClose(resultReadEvent);
            return HRESULT_FROM_NT(status);
        }

        NtWaitForSingleObject(initializedEvent, FALSE, NULL);
        result = threadContext->Result;
        NtSetEvent(resultReadEvent, NULL);

        NtClose(threadHandle);
        NtClose(initializedEvent);
        NtClose(resultReadEvent);

        return result;
    }

    auto toast = std::make_unique<PH::Toast>();

    RETURN_IF_FAILED(toast->Initialize(ApplicationId,
                                       ToastXml,
                                       TimeoutMilliseconds,
                                       Priority,
                                       ToastCallback,
                                       Context));
    return toast->Show();
}

_Must_inspect_result_
HRESULT PhShowToast(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    return PhShowToastEx(
        ApplicationId,
        ToastXml,
        TimeoutMilliseconds,
        PhToastPriorityDefault,
        ToastCallback,
        Context
        );
}

_Must_inspect_result_
HRESULT PhShowToastEx(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PH_TOAST_PRIORITY Priority,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF applicationIdStringRef;
    PH_STRINGREF toastXmlStringRef;

    PhInitializeStringRefLongHint(&applicationIdStringRef, ApplicationId);
    PhInitializeStringRefLongHint(&toastXmlStringRef, ToastXml);

    return PhShowToastStringRefEx(
        &applicationIdStringRef,
        &toastXmlStringRef,
        TimeoutMilliseconds,
        Priority,
        ToastCallback,
        Context
        );
}

// ---------------------------------------------------------------------------
// V2: action-aware + updatable progress toasts
// ---------------------------------------------------------------------------

namespace PH
{
    using IToastActivatedHandler2 = ITypedEventHandler<ToastNotification*, IInspectable*>;
    using IToastDismissedHandler2 = ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*>;
    using IToastFailedHandler2 = ITypedEventHandler<ToastNotification*, ToastFailedEventArgs*>;

    class ToastV2;

    /*!
        @brief V2 toast event handler. Unlike v1, Activated events are not
         treated as terminal when the toast carries <actions>; the handler
         keeps the tokens registered so subsequent dismissal can still fire.
         Dismissed/Failed are terminal and unhook all tokens.
    */
    class ToastEventHandlerV2 final : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                                  IToastActivatedHandler2,
                                                  IToastDismissedHandler2,
                                                  IToastFailedHandler2>
    {
    public:

        HRESULT RuntimeClassInitialize(
            PPH_TOAST_CALLBACK2 ToastCallback,
            PVOID Context
            )
        {
            m_ToastCallback = ToastCallback;
            m_Context = Context;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IInspectable* Args
            ) override;

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastDismissedEventArgs* Args
            ) override;

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastFailedEventArgs* Args
            ) override;

        void SetActivatedToken(const EventRegistrationToken& Token)
        {
            m_ActivatedToken = Token;
        }
        void SetDismissedToken(const EventRegistrationToken& Token)
        {
            m_DismissedToken = Token;
        }
        void SetFailedToken(const EventRegistrationToken& Token)
        {
            m_FailedToken = Token;
        }

        void UnhookAll(IToastNotification* Sender)
        {
            if (!m_Unhooked.exchange(true))
            {
                Sender->remove_Dismissed(m_DismissedToken);
                Sender->remove_Activated(m_ActivatedToken);
                Sender->remove_Failed(m_FailedToken);
            }
        }

    private:

        PPH_TOAST_CALLBACK2 m_ToastCallback{ nullptr };
        PVOID m_Context{ nullptr };

        EventRegistrationToken m_ActivatedToken{};
        EventRegistrationToken m_DismissedToken{};
        EventRegistrationToken m_FailedToken{};

        std::atomic<bool> m_Unhooked{ false };
    };

    class ToastV2
    {
    public:
        ComPtr<IToastNotifier> Notifier;
        ComPtr<IToastNotification> Toast;
        ComPtr<ToastEventHandlerV2> Handler;
        PPH_STRING Tag{ nullptr };
        PPH_STRING Group{ nullptr };
        std::atomic<unsigned int> Sequence{ 1 };

        ~ToastV2()
        {
            PhClearReference((PVOID*)&Tag);
            PhClearReference((PVOID*)&Group);
        }
    };
}

static VOID PhpLogToastNotifierSetting(
    _In_ IToastNotifier* Notifier
    )
{
    NotificationSetting setting;

    if (SUCCEEDED(Notifier->get_Setting(&setting)))
    {
        if (setting != NotificationSetting_Enabled)
        {
            dprintf(
                "Toast: notifier setting is not enabled (%lu)\n",
                (ULONG)setting
                );
        }
    }
}

_Must_inspect_result_
static HRESULT PhpCreateToastNotifierWithFallback(
    _In_ IToastNotificationManagerStatics* Manager,
    _In_ PPH_STRINGREF ApplicationId,
    _COM_Outptr_ IToastNotifier** Notifier
    )
{
    HSTRING_REFERENCE appIdRef;
    HRESULT status;

    *Notifier = nullptr;

    status = PhCreateWindowsRuntimeStringRef(ApplicationId, &appIdRef);
    if (HR_SUCCESS(status))
    {
        status = Manager->CreateToastNotifierWithId(HSTRING_FROM_STRING(appIdRef), Notifier);
        if (HR_SUCCESS(status))
            return status;
    }

    {
        PPH_STRING resolvedAppId = NULL;

        if (SUCCEEDED(PhAppResolverGetAppIdForProcess(NtCurrentProcessId(), &resolvedAppId)) &&
            !PhIsNullOrEmptyString(resolvedAppId))
        {
            HSTRING_REFERENCE resolvedAppIdRef;
            PH_STRINGREF resolvedAppIdSr;
            HRESULT fallbackStatus;

            resolvedAppIdSr = resolvedAppId->sr;

            fallbackStatus = PhCreateWindowsRuntimeStringRef(&resolvedAppIdSr, &resolvedAppIdRef);
            if (HR_SUCCESS(fallbackStatus))
            {
                fallbackStatus = Manager->CreateToastNotifierWithId(HSTRING_FROM_STRING(resolvedAppIdRef), Notifier);

                if (HR_SUCCESS(fallbackStatus))
                {
                    dprintf(
                        "Toast: fallback AppUserModelID '%ls' used (requested '%.*ls')\n",
                        resolvedAppId->Buffer,
                        (int)(ApplicationId->Length / sizeof(WCHAR)),
                        ApplicationId->Buffer
                        );
                    PhDereferenceObject(resolvedAppId);
                    return fallbackStatus;
                }
            }
        }

        PhClearReference(&resolvedAppId);
    }

    return status;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandlerV2::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IInspectable* Args
    )
{
    PCWSTR arguments = nullptr;
    HSTRING argsHandle = nullptr;
    ComPtr<IToastActivatedEventArgs> activatedArgs;

    if (Args && SUCCEEDED(Args->QueryInterface(IID_PPV_ARGS(&activatedArgs))))
    {
        if (SUCCEEDED(activatedArgs->get_Arguments(&argsHandle)) && argsHandle)
        {
            arguments = PhGetWindowsRuntimeStringBuffer(argsHandle, nullptr);
        }
    }

    m_ToastCallback(S_OK, PhToastReasonAction, arguments, m_Context);

    if (argsHandle)
    {
        PhDeleteWindowsRuntimeString(argsHandle);
    }

    // Activated is NOT terminal for v2 -- progress/multi-event toasts may
    // still receive Dismissed after a button press. Cleanup occurs on
    // Dismissed/Failed or on explicit PhReleaseToast.
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandlerV2::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastDismissedEventArgs* Args
    )
{
    PH_TOAST_REASON phReason = PhToastReasonUnknown;
    ToastDismissalReason reason;
    HRESULT result;

    result = Args->get_Reason(&reason);

    if (HR_SUCCESS(result))
    {
        switch (reason)
        {
        case ToastDismissalReason_UserCanceled:
            phReason = PhToastReasonUserCanceled;
            break;
        case ToastDismissalReason_ApplicationHidden:
            phReason = PhToastReasonApplicationHidden;
            break;
        case ToastDismissalReason_TimedOut:
            phReason = PhToastReasonTimedOut;
            break;
        }
    }

    m_ToastCallback(result, phReason, nullptr, m_Context);
    UnhookAll(Sender);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandlerV2::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastFailedEventArgs* Args
    )
{
    HRESULT err;
    HRESULT hr = Args->get_ErrorCode(&err);
    if (HR_FAILED(hr))
    {
        err = hr;
    }

    m_ToastCallback(err, PhToastReasonError, nullptr, m_Context);
    UnhookAll(Sender);
    return S_OK;
}

PPH_STRING PhEscapeStringForXml(
    _In_opt_ PCWSTR Input
    )
{
    if (!Input)
        return nullptr;

    static const WCHAR specialChars[] = { L'&', L'<', L'>', L'"', L'\'' };

    PH_STRING_BUILDER sb;
    PhInitializeStringBuilder(&sb, 64);

    SIZE_T length = PhCountStringZ((PWSTR)Input);
    SIZE_T pos = 0;

    while (pos < length)
    {
        // Emit the literal run up to the next character that needs escaping in
        // one block instead of appending character-by-character.
        SIZE_T runLength = PhFindFirstOfCharsW(&Input[pos], length - pos, specialChars, RTL_NUMBER_OF(specialChars));

        if (runLength != 0)
            PhAppendStringBuilderEx(&sb, (PWCHAR)&Input[pos], runLength * sizeof(WCHAR));

        pos += runLength;

        if (pos >= length)
            break;

        switch (Input[pos])
        {
        case L'&':  PhAppendStringBuilder2(&sb, L"&amp;"); break;
        case L'<':  PhAppendStringBuilder2(&sb, L"&lt;"); break;
        case L'>':  PhAppendStringBuilder2(&sb, L"&gt;"); break;
        case L'"':  PhAppendStringBuilder2(&sb, L"&quot;"); break;
        case L'\'': PhAppendStringBuilder2(&sb, L"&apos;"); break;
        }

        pos++;
    }

    return PhFinalStringBuilderString(&sb);
}

typedef struct _PH_SHOW_TOAST_EX2_PARAMS
{
    PPH_STRINGREF ApplicationId;
    PPH_STRINGREF ToastXml;
    PPH_STRINGREF Tag;
    PPH_STRINGREF Group;
    ULONG TimeoutMilliseconds;
    PH_TOAST_PRIORITY Priority;
    PPH_TOAST_CALLBACK2 ToastCallback;
    PVOID Context;
    PPH_TOAST* OutToast;
} PH_SHOW_TOAST_EX2_PARAMS, *PPH_SHOW_TOAST_EX2_PARAMS;

// Runs on the toast pump thread. All COM objects in the PH::ToastV2 handle are
// created here so they live on (and are released on) the pump thread apartment.
static HRESULT NTAPI PhpShowToastEx2Worker(
    _In_opt_ PVOID Parameter
    )
{
    PPH_SHOW_TOAST_EX2_PARAMS params = (PPH_SHOW_TOAST_EX2_PARAMS)Parameter;
    PPH_STRINGREF ApplicationId = params->ApplicationId;
    PPH_STRINGREF ToastXml = params->ToastXml;
    PPH_STRINGREF Tag = params->Tag;
    PPH_STRINGREF Group = params->Group;
    ULONG TimeoutMilliseconds = params->TimeoutMilliseconds;
    PH_TOAST_PRIORITY Priority = params->Priority;
    PPH_TOAST_CALLBACK2 ToastCallback = params->ToastCallback;
    PVOID Context = params->Context;
    PPH_TOAST* OutToast = params->OutToast;

    HSTRING_REFERENCE stringToastXmlRef;

    if (OutToast) *OutToast = nullptr;

    ComPtr<IToastNotificationManagerStatics> manager;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationManagerStatics>(
        L"WpnApps.dll",
        L"Windows.UI.Notifications.ToastNotificationManager",
        &manager));

    ComPtr<IToastNotificationFactory> factory;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationFactory>(
        L"WpnApps.dll",
        L"Windows.UI.Notifications.ToastNotification",
        &factory));

    ComPtr<IToastNotifier> notifier;
    RETURN_IF_FAILED(PhpCreateToastNotifierWithFallback(manager.Get(), ApplicationId, &notifier));
    PhpLogToastNotifierSetting(notifier.Get());

    ComPtr<IXmlDocument> xmlDocument;
    RETURN_IF_FAILED(manager->GetTemplateContent(
        ToastTemplateType::ToastTemplateType_ToastImageAndText02,
        &xmlDocument));

    ComPtr<IXmlDocumentIO> xmlIo;
    RETURN_IF_FAILED(xmlDocument.As(&xmlIo));

    RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(ToastXml, &stringToastXmlRef));
    RETURN_IF_FAILED(xmlIo->LoadXml(HSTRING_FROM_STRING(stringToastXmlRef)));

    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(factory->CreateToastNotification(xmlDocument.Get(), &toast));

    // Priority
    if (Priority == PhToastPriorityHigh)
    {
        ComPtr<PH::IToastNotification4> toast4;
        HRESULT status = toast.As(&toast4);
        if (status != E_NOINTERFACE)
        {
            RETURN_IF_FAILED(status);
            RETURN_IF_FAILED(toast4->put_Priority(PH::ToastNotificationPriority_High));
        }
    }

    // Tag/Group via IToastNotification2 (required for IToastNotifier2::Update)
    ComPtr<IToastNotification2> toast2;
    if (SUCCEEDED(toast.As(&toast2)))
    {
        if (Tag && Tag->Length)
        {
            HSTRING_REFERENCE tagRef;
            if (SUCCEEDED(PhCreateWindowsRuntimeStringRef(Tag, &tagRef)))
                toast2->put_Tag(HSTRING_FROM_STRING(tagRef));
        }
        if (Group && Group->Length)
        {
            HSTRING_REFERENCE groupRef;
            if (SUCCEEDED(PhCreateWindowsRuntimeStringRef(Group, &groupRef)))
                toast2->put_Group(HSTRING_FROM_STRING(groupRef));
        }
    }

    // Expiration (timeout)
    if (TimeoutMilliseconds > 0)
    {
        ComPtr<IPropertyValueStatics> propertyStats;
        RETURN_IF_FAILED(PH::RoGetActivationFactory<IPropertyValueStatics>(
            L"WinTypes.dll",
            L"Windows.Foundation.PropertyValue",
            &propertyStats));

        LARGE_INTEGER sysTime;
        PhQuerySystemTime(&sysTime);
        sysTime.QuadPart += UInt32x32To64(TimeoutMilliseconds, PH_TIMEOUT_MS);

        DateTime time;
        time.UniversalTime = sysTime.QuadPart;

        ComPtr<IInspectable> inspectable;
        RETURN_IF_FAILED(propertyStats->CreateDateTime(time, &inspectable));

        ComPtr<IReference<DateTime>> dateTime;
        RETURN_IF_FAILED(inspectable.As(&dateTime));

        RETURN_IF_FAILED(toast->put_ExpirationTime(dateTime.Get()));
    }

    // Build handle (owns ComPtrs for lifetime)
    auto entry = std::make_unique<PH::ToastV2>();
    entry->Notifier = notifier;
    entry->Toast = toast;
    if (Tag && Tag->Length) entry->Tag = PhCreateString2(Tag);
    if (Group && Group->Length) entry->Group = PhCreateString2(Group);

    if (ToastCallback)
    {
        ComPtr<PH::ToastEventHandlerV2> callbacks;
        ComPtr<PH::IToastActivatedHandler2> activatedNotif;
        ComPtr<PH::IToastDismissedHandler2> dismissedNotif;
        ComPtr<PH::IToastFailedHandler2> failedNotif;

        RETURN_IF_FAILED(MakeAndInitialize<PH::ToastEventHandlerV2>(
            &callbacks, ToastCallback, Context));

        RETURN_IF_FAILED(callbacks.As(&activatedNotif));
        RETURN_IF_FAILED(callbacks.As(&dismissedNotif));
        RETURN_IF_FAILED(callbacks.As(&failedNotif));

        EventRegistrationToken token;
        RETURN_IF_FAILED(toast->add_Activated(activatedNotif.Get(), &token));
        callbacks->SetActivatedToken(token);
        RETURN_IF_FAILED(toast->add_Dismissed(dismissedNotif.Get(), &token));
        callbacks->SetDismissedToken(token);
        RETURN_IF_FAILED(toast->add_Failed(failedNotif.Get(), &token));
        callbacks->SetFailedToken(token);

        entry->Handler = callbacks;
    }

    RETURN_IF_FAILED(notifier->Show(toast.Get()));

    if (OutToast)
    {
        *OutToast = reinterpret_cast<PPH_TOAST>(entry.release());
    }
    // If caller didn't ask for a handle, the entry is still alive via the
    // notification's own lifetime in the OS; we leak the wrapper. Discourage
    // this by warning callers via header doc -- pass OutToast=NULL only for
    // truly fire-and-forget v2 use. For now drop the wrapper.
    return S_OK;
}

_Must_inspect_result_
HRESULT PhShowToastEx2(
    _In_ PPH_STRINGREF ApplicationId,
    _In_ PPH_STRINGREF ToastXml,
    _In_opt_ PPH_STRINGREF Tag,
    _In_opt_ PPH_STRINGREF Group,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PH_TOAST_PRIORITY Priority,
    _In_opt_ PPH_TOAST_CALLBACK2 ToastCallback,
    _In_opt_ PVOID Context,
    _Outptr_opt_result_maybenull_ PPH_TOAST* OutToast
    )
{
    PH_SHOW_TOAST_EX2_PARAMS params;

    params.ApplicationId = ApplicationId;
    params.ToastXml = ToastXml;
    params.Tag = Tag;
    params.Group = Group;
    params.TimeoutMilliseconds = TimeoutMilliseconds;
    params.Priority = Priority;
    params.ToastCallback = ToastCallback;
    params.Context = Context;
    params.OutToast = OutToast;

    // Marshal the COM work onto the pump thread; callbacks are delivered there.
    return PhpToastThreadInvoke(PhpShowToastEx2Worker, &params);
}

typedef struct _PH_UPDATE_TOAST_PROGRESS_PARAMS
{
    PPH_TOAST Toast;
    double Value;
    PCWSTR String;
    PCWSTR Status;
    PCWSTR Title;
} PH_UPDATE_TOAST_PROGRESS_PARAMS, *PPH_UPDATE_TOAST_PROGRESS_PARAMS;

// Runs on the toast pump thread (same apartment that owns the toast ComPtrs).
static HRESULT NTAPI PhpUpdateToastProgressWorker(
    _In_opt_ PVOID Parameter
    )
{
    PPH_UPDATE_TOAST_PROGRESS_PARAMS params = (PPH_UPDATE_TOAST_PROGRESS_PARAMS)Parameter;
    PPH_TOAST Toast = params->Toast;
    double Value = params->Value;
    PCWSTR String = params->String;
    PCWSTR Status = params->Status;
    PCWSTR Title = params->Title;

    if (!Toast)
        return E_INVALIDARG;

    auto entry = reinterpret_cast<PH::ToastV2*>(Toast);

    if (!entry->Notifier)
        return E_NOT_VALID_STATE;

    ComPtr<IToastNotifier2> notifier2;
    RETURN_IF_FAILED(entry->Notifier.As(&notifier2));

    // Build NotificationData via the runtime factory.
    ComPtr<INotificationDataFactory> dataFactory;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<INotificationDataFactory>(
        L"WpnApps.dll",
        L"Windows.UI.Notifications.NotificationData",
        &dataFactory));

    ComPtr<INotificationData> data;
    RETURN_IF_FAILED(dataFactory->CreateNotificationDataWithValues(nullptr, &data));

    //{
    //    ComPtr<IActivationFactory> activationFactory;
    //    RETURN_IF_FAILED(PH::RoGetActivationFactory<IActivationFactory>(
    //        L"WpnApps.dll",
    //        L"Windows.UI.Notifications.NotificationData",
    //        &activationFactory));

    //    ComPtr<IInspectable> inspectable;
    //    RETURN_IF_FAILED(activationFactory->ActivateInstance(&inspectable));

    //    ComPtr<INotificationData> data;
    //    RETURN_IF_FAILED(inspectable.As(&data));
    //}



    ComPtr<IMap<HSTRING, HSTRING>> values;
    RETURN_IF_FAILED(data->get_Values(&values));

    auto insertValue = [&](PCWSTR key, PCWSTR value) -> HRESULT
    {
        if (!value) return S_OK;
        HSTRING_REFERENCE keyRef, valRef;
        PH_STRINGREF keySr, valSr;
        PhInitializeStringRefLongHint(&keySr, key);
        PhInitializeStringRefLongHint(&valSr, value);
        RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(&keySr, &keyRef));
        RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(&valSr, &valRef));
        boolean replaced;
        return values->Insert(HSTRING_FROM_STRING(keyRef), HSTRING_FROM_STRING(valRef), &replaced);
    };

    // Format the numeric progress value.
    WCHAR valueBuffer[32];
    if (Value < 0.0) Value = 0.0;
    if (Value > 1.0) Value = 1.0;
    _snwprintf_s(valueBuffer, _TRUNCATE, L"%.4f", Value);

    RETURN_IF_FAILED(insertValue(L"progressValue", valueBuffer));
    RETURN_IF_FAILED(insertValue(L"progressValueString", String));
    RETURN_IF_FAILED(insertValue(L"progressStatus", Status));
    RETURN_IF_FAILED(insertValue(L"progressTitle", Title));

    // Monotonically increasing sequence number (0 means "always overwrite").
    unsigned int seq = entry->Sequence.fetch_add(1) + 1;
    RETURN_IF_FAILED(data->put_SequenceNumber(seq));

    // Address by Tag (+ optional Group)
    HSTRING_REFERENCE tagRef{};
    HSTRING_REFERENCE groupRef{};
    HSTRING tagHandle = nullptr;
    HSTRING groupHandle = nullptr;

    if (entry->Tag)
    {
        PhCreateWindowsRuntimeStringRef(&entry->Tag->sr, &tagRef);
        tagHandle = HSTRING_FROM_STRING(tagRef);
    }
    if (entry->Group)
    {
        PhCreateWindowsRuntimeStringRef(&entry->Group->sr, &groupRef);
        groupHandle = HSTRING_FROM_STRING(groupRef);
    }

    NotificationUpdateResult updateResult;
    HRESULT hr;

    if (groupHandle)
    {
        hr = notifier2->UpdateWithTagAndGroup(data.Get(), tagHandle, groupHandle, &updateResult);
    }
    else
    {
        hr = notifier2->UpdateWithTag(data.Get(), tagHandle, &updateResult);
    }
    return hr;
}

EXTERN_C
HRESULT PhUpdateToastProgress(
    _In_ PPH_TOAST Toast,
    _In_ double Value,
    _In_opt_ PCWSTR String,
    _In_opt_ PCWSTR Status,
    _In_opt_ PCWSTR Title
    )
{
    PH_UPDATE_TOAST_PROGRESS_PARAMS params;

    params.Toast = Toast;
    params.Value = Value;
    params.String = String;
    params.Status = Status;
    params.Title = Title;

    return PhpToastThreadInvoke(PhpUpdateToastProgressWorker, &params);
}

static HRESULT NTAPI PhpHideToastWorker(
    _In_opt_ PVOID Parameter
    )
{
    auto entry = reinterpret_cast<PH::ToastV2*>(Parameter);

    if (entry->Notifier && entry->Toast)
        entry->Notifier->Hide(entry->Toast.Get());

    return S_OK;
}

VOID PhHideToast(
    _In_opt_ PPH_TOAST Toast
    )
{
    if (!Toast)
        return;

    PhpToastThreadInvoke(PhpHideToastWorker, Toast);
}

static HRESULT NTAPI PhpReleaseToastWorker(
    _In_opt_ PVOID Parameter
    )
{
    auto entry = reinterpret_cast<PH::ToastV2*>(Parameter);

    if (entry->Handler && entry->Toast)
    {
        entry->Handler->UnhookAll(entry->Toast.Get());
    }

    // ComPtrs are released here on the pump thread apartment.
    delete entry;

    return S_OK;
}

VOID PhReleaseToast(
    _In_opt_ PPH_TOAST Toast
    )
{
    if (!Toast)
        return;

    PhpToastThreadInvoke(PhpReleaseToastWorker, Toast);
}
