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
#include <mapldr.h>
#include <appresolver.h>

#include <wrl.h>
#include <roapi.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>

#include <atomic>
#include <memory>

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(_HR_) do { const auto __hr = _HR_; if (HR_FAILED(__hr)) { return __hr; }} while (false)
#endif

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

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
        @brief Wrapper around RoGetActivationFactory to make it a little less clunky to use.

        @details PhActivateToastRuntime must be called prior to this else the function returns
         R_NOTIMPL.

        @tparam T - Interface to retrieve.
        @tparam t_SizeDest - Length of the constant string to use.

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

    private:

        PPH_TOAST_CALLBACK m_ToastCallback{ nullptr };
        PVOID m_Context{ nullptr };

        EventRegistrationToken m_ActivatedToken{};
        EventRegistrationToken m_DismissedToken{};
        EventRegistrationToken m_FailedToken{};

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
            _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
            _In_opt_ PVOID Context
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
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
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
    RETURN_IF_FAILED(PhCreateWindowsRuntimeStringRef(ToastXml, &stringToastXmlRef));
    RETURN_IF_FAILED(xmlIo->LoadXml(HSTRING_FROM_STRING(stringToastXmlRef)));

    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(factory->CreateToastNotification(xmlDocument.Get(), &toast));

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
    //if (g_RoInitialize)
    //{
    //    g_RoUninitialize();
    //}
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
    auto toast = std::make_unique<PH::Toast>();

    RETURN_IF_FAILED(toast->Initialize(ApplicationId,
                                       ToastXml,
                                       TimeoutMilliseconds,
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
    PH_STRINGREF applicationIdStringRef;
    PH_STRINGREF toastXmlStringRef;

    PhInitializeStringRefLongHint(&applicationIdStringRef, (PWSTR)ApplicationId);
    PhInitializeStringRefLongHint(&toastXmlStringRef, (PWSTR)ToastXml);

    return PhShowToastStringRef(&applicationIdStringRef,
                                &toastXmlStringRef,
                                TimeoutMilliseconds,
                                ToastCallback,
                                Context);
}
