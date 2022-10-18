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

#include <notiftoast.h>

#include <wrl.h>
#include <roapi.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>

#include <atomic>
#include <memory>

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(_HR_) do { const auto __hr = _HR_; if (FAILED(__hr)) { return __hr; }} while (false)
#endif

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

static decltype(RoGetActivationFactory)* g_RoGetActivationFactory = nullptr;
static decltype(RoInitialize)* g_RoInitialize = nullptr;
static decltype(RoUninitialize)* g_RoUninitialize = nullptr;
static decltype(WindowsCreateStringReference)* g_WindowsCreateStringReference = nullptr;

namespace PH
{
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
        *FunctionPointer = static_cast<T>(PhGetModuleProcAddress(
            const_cast<PWSTR>(Module),
            const_cast<PSTR>(Function)));
        return (*FunctionPointer != nullptr);
    }

    /*!
        @brief Alternate to Windows::WRL::Wrappers::HStringReference which requires some imports
         from combase.dll, we will dynamically import them through our runtime wrapper. It also
         throws in some lame cases. Ours won't we'll bail on failure without exceptions.
    */
    class HStringReference
    {
    public:

        HStringReference() = default;

        ~HStringReference()
        {
            //
            // A HSTRING reference doesn't need cleaned up, it's a "fast-pass" (yeah that's what
            // the call it in the docs) string. Really it's just a pointer to the actual string
            // with the length. Saves allocations when passing strings to the runtime.
            // Guess UNICODE_STRING wasn't cool enough...
            //
        }

        HRESULT Set(_In_ const wchar_t* String, _In_ size_t Length)
        {
            if (!g_WindowsCreateStringReference)
            {
                return E_NOTIMPL;
            }

            UINT32 length;
            RETURN_IF_FAILED(SizeTToUInt32(Length, &length));

            return g_WindowsCreateStringReference(String, length, &m_Header, &m_String);
        }

        HRESULT Set(_In_ const wchar_t* String)
        {
            if (!g_WindowsCreateStringReference)
            {
                return E_NOTIMPL;
            }

            return Set(String, ::wcslen(String));
        }

        HSTRING Get() const
        {
            return m_String;
        }

    private:

        HSTRING_HEADER m_Header{};
        HSTRING m_String{ nullptr };

    };

    /*!
        @brief Wrapper around RoGetActivationFactory to make it a little less clunky to use.

        @details PhActivateToastRuntime must be called prior to this else the function returns
         R_NOTIMPL.

        @tparam T - Interface to retrieve.
        @tparam t_SizeDest - Length of the constant string to use.

        @param[in] ActivatableClassId - Activatable class ID to look up the interface from.
        @param[out] Interface - On success, set to a referenced interface of type T.

        @return Appropriate result status.
    */
    template <typename T, size_t t_SizeDest>
    _Must_inspect_result_
    HRESULT RoGetActivationFactory(
        _In_ wchar_t const (&ActivatableClassId)[t_SizeDest],
        _COM_Outptr_ T** Interface
        )
    {
        if (!g_RoGetActivationFactory)
        {
            *Interface = nullptr;
            return E_NOTIMPL;
        }

        HStringReference stringRef;
        HRESULT hr = stringRef.Set(ActivatableClassId, t_SizeDest - 1);
        if (FAILED(hr))
        {
            *Interface = nullptr;
            return hr;
        }

        hr = g_RoGetActivationFactory(stringRef.Get(),
                                      __uuidof(T),
                                      reinterpret_cast<void**>(Interface));
        if (FAILED(hr))
        {
            *Interface = nullptr;
        }
        return hr;
    }

    using IToastActivatedHandler = ITypedEventHandler<ToastNotification*, IInspectable*>;
    using IToastDismissedHandler = ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*>;
    using IToastFailedHandler = ITypedEventHandler<ToastNotification*, ToastFailedEventArgs*>;

    /*!
        @brief Process Hacker toast event handler, this class implements the
         handler interfaces for IToastNotification and store the callback
         and context from the C interface.
    */
    class ToastEventHandler : virtual public IToastActivatedHandler,
                              virtual public IToastDismissedHandler,
                              virtual public IToastFailedHandler,
                              virtual public IUnknown
    {
    public:

        virtual ~ToastEventHandler() = default;

        ToastEventHandler(
            PPH_TOAST_CALLBACK ToastCallback,
            PVOID Context
            ) : m_ToastCallback(ToastCallback),
                m_Context(Context)
        {
        }

        virtual HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastDismissedEventArgs* Args
            ) override;

        virtual HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IToastFailedEventArgs* Args
            ) override;

        virtual HRESULT STDMETHODCALLTYPE Invoke(
            _In_ IToastNotification* Sender,
            _In_ IInspectable* Args
            ) override;

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            _In_ REFIID InterfaceId,
            _COM_Outptr_ void** Interface
            ) override;

        virtual ULONG STDMETHODCALLTYPE AddRef() override
        {
            return ++m_RefCount;
        }

        virtual ULONG STDMETHODCALLTYPE Release() override
        {
            auto res = --m_RefCount;
            if (res == 0)
            {
                delete this;
            }
            return res;
        }

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

    private:

        std::atomic<ULONG> m_RefCount = 0;
        PPH_TOAST_CALLBACK m_ToastCallback;
        PVOID m_Context;

        EventRegistrationToken m_ActivatedToken{};
        EventRegistrationToken m_DismissedToken{};
        EventRegistrationToken m_FailedToken{};

    };

    /*!
        @brief Process hacker toast object.
    */
    class Toast
    {
    public:

        ~Toast() = default;

        Toast() = default;

        _Must_inspect_result_
        HRESULT Initialize(
            _In_ PCWSTR ApplicationId,
            _In_ PCWSTR ToastXml,
            _In_opt_ ULONG TimeoutMilliseconds,
            _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
            _In_opt_ PVOID Context
            );

        HRESULT Show();

    private:

        ComPtr<IToastNotifier> m_Notifier{ nullptr };
        ComPtr<IToastNotification> m_Toast{ nullptr };

    };

}

_Must_inspect_result_
HRESULT PH::Toast::Initialize(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    HStringReference stringRef;

    ComPtr<IToastNotificationManagerStatics> manager;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationManagerStatics>(
        L"Windows.UI.Notifications.ToastNotificationManager",
        &manager));

    ComPtr<IToastNotificationFactory> factory;
    RETURN_IF_FAILED(PH::RoGetActivationFactory<IToastNotificationFactory>(
        L"Windows.UI.Notifications.ToastNotification",
        &factory));

    ComPtr<IToastNotifier> notifier;
    RETURN_IF_FAILED(stringRef.Set(ApplicationId));
    RETURN_IF_FAILED(manager->CreateToastNotifierWithId(stringRef.Get(),
                                                        &notifier));

    //
    // Couldn't find a nice way to just create a IXmlDocument... There is
    // probably a way to but I guess I'm lazy. I'm just going to grab a
    // template one from the manager interface then overwrite it with the
    // input.
    //
    ComPtr<IXmlDocument> xmlDocument;
    RETURN_IF_FAILED(manager->GetTemplateContent(
        ToastTemplateType::ToastTemplateType_ToastText01,
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
    RETURN_IF_FAILED(stringRef.Set(ToastXml));
    RETURN_IF_FAILED(xmlIo->LoadXml(stringRef.Get()));

    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(factory->CreateToastNotification(xmlDocument.Get(), &toast));

    if (TimeoutMilliseconds > 0)
    {
        //
        // We have a timeout, set it.
        //
        ComPtr<IPropertyValueStatics> propertyStats;
        RETURN_IF_FAILED(PH::RoGetActivationFactory<IPropertyValueStatics>(
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

        callbacks = new ToastEventHandler(ToastCallback, Context);

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

HRESULT PH::Toast::Show()
{
    if ((m_Notifier == nullptr) || (m_Toast == nullptr))
    {
        return E_NOT_VALID_STATE;
    }
    return m_Notifier->Show(m_Toast.Get());
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::QueryInterface(
    _In_ REFIID InterfaceId,
    _COM_Outptr_ void** Interface
    )
{
    if (InterfaceId == __uuidof(IToastDismissedHandler))
    {
        *Interface = static_cast<IToastDismissedHandler*>(this);
        AddRef();
        return S_OK;
    }

    if (InterfaceId == __uuidof(IToastActivatedHandler))
    {
        *Interface = static_cast<IToastActivatedHandler*>(this);
        AddRef();
        return S_OK;
    }
    if (InterfaceId == __uuidof(IToastFailedHandler))
    {
        *Interface = static_cast<IToastFailedHandler*>(this);
        AddRef();
        return S_OK;
    }
    if (InterfaceId == __uuidof(IUnknown))
    {
        *Interface = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }

    *Interface = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastDismissedEventArgs* Args
    )
{
    PH_TOAST_REASON phReason = PhToastReasonUnknown;

    ToastDismissalReason reason;
    HRESULT hr = Args->get_Reason(&reason);
    if (SUCCEEDED(hr))
    {
        switch (reason)
        {
            case ToastDismissalReason_UserCanceled:
            {
                phReason = PhToastReasonUserCanceled;
                break;
            }
            case ToastDismissalReason_ApplicationHidden:
            {
                phReason = PhToastReasonApplicationHidden;
                break;
            }
            case ToastDismissalReason_TimedOut:
            {
                phReason = PhToastReasonTimedOut;
                break;
            }
        }
    }

    m_ToastCallback(S_OK, phReason, m_Context);

    //
    // Without this the ref is never decremented and the runtime leaks...
    // We fire to the callback once anyway, so remove the callback from
    // processing.
    //
    Sender->remove_Dismissed(m_DismissedToken);
    Sender->remove_Activated(m_ActivatedToken);
    Sender->remove_Failed(m_FailedToken);
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
    Sender->remove_Dismissed(m_DismissedToken);
    Sender->remove_Activated(m_ActivatedToken);
    Sender->remove_Failed(m_FailedToken);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PH::ToastEventHandler::Invoke(
    _In_ IToastNotification* Sender,
    _In_ IToastFailedEventArgs* Args
    )
{
    HRESULT err;
    HRESULT hr = Args->get_ErrorCode(&err);
    if (FAILED(hr))
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
    Sender->remove_Dismissed(m_DismissedToken);
    Sender->remove_Activated(m_ActivatedToken);
    Sender->remove_Failed(m_FailedToken);
    return S_OK;
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
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        //
        // We dynamically import to support OSes that don't have this functionality.
        //
        PH::GetModuleProcAddress(&g_RoGetActivationFactory,
                                 L"combase.dll",
                                 "RoGetActivationFactory");
        PH::GetModuleProcAddress(&g_RoInitialize,
                                 L"combase.dll",
                                 "RoInitialize");
        PH::GetModuleProcAddress(&g_RoUninitialize,
                                 L"combase.dll",
                                 "RoUninitialize");
        PH::GetModuleProcAddress(&g_WindowsCreateStringReference,
                                 L"combase.dll",
                                 "WindowsCreateStringReference");

        PhEndInitOnce(&initOnce);
    }

    if (!g_RoGetActivationFactory ||
        !g_RoInitialize ||
        !g_RoUninitialize ||
        !g_WindowsCreateStringReference)
    {
        return E_NOTIMPL;
    }

    return g_RoInitialize(RO_INIT_MULTITHREADED);
}

/*!
    @brief This is a wrapper to call RoUninitialize. This should only be called when
     PhInitializeToastRuntime return a success (including S_FALSE).
*/
VOID PhUninitializeToastRuntime()
{
    if (g_RoInitialize)
    {
        g_RoUninitialize();
    }
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
HRESULT PhShowToast(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    )
{
    auto toast = std::make_unique<PH::Toast>();

    RETURN_IF_FAILED(toast->Initialize(ApplicationId,
                                       ToastXml,
                                       TimeoutMilliseconds,
                                       ToastCallback,
                                       Context));
    return toast->Show();
}
