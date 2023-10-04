// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once

#include "MidiMessageReceivedEventArgs.h"

#include <pch.h>

namespace midi2 = ::winrt::Windows::Devices::Midi2;


namespace Windows::Devices::Midi2::Internal
{

    template<typename TEndpointAbstraction>
    class InternalMidiInputConnection

    {
    public:
        winrt::hstring InputEndpointDeviceId() const noexcept { return m_inputDeviceId; }

        winrt::event_token MessageReceived(_In_ foundation::TypedEventHandler<foundation::IInspectable, midi2::MidiMessageReceivedEventArgs> const& handler)
        {
            return m_messageReceivedEvent.add(handler);
        }

        void MessageReceived(_In_ winrt::event_token const& token) noexcept
        {
            if (m_messageReceivedEvent) m_messageReceivedEvent.remove(token);
        }


        winrt::event_token StreamConfigurationComplete(_In_ foundation::TypedEventHandler<foundation::IInspectable, midi2::MidiStreamConfigurationCompleteEventArgs> const& handler)
        {
            return m_streamConfigurationCompleteEvent.add(handler);
        }

        void StreamConfigurationComplete(_In_ winrt::event_token const& token) noexcept
        {
            if (m_streamConfigurationCompleteEvent) m_streamConfigurationCompleteEvent.remove(token);
        }



        winrt::Windows::Foundation::Collections::IVector<midi2::IMidiEndpointMessageProcessingPlugin> MessageProcessingPlugins() const noexcept
        {
            return m_messageProcessingPlugins;
        }

    protected:

        void SetInputConnectionOnPlugins(_In_ midi2::IMidiInputConnection const inputConnection) noexcept;
        void SetOutputConnectionOnPlugins(_In_ midi2::IMidiOutputConnection const outputConnection) noexcept;

        void SetRequestedStreamConfigurationOnPlugins(_In_ midi2::MidiStreamConfigurationRequestedSettings const settings) noexcept;

        void InitializePlugins() noexcept;
        void CallOnConnectionOpenedOnPlugins() noexcept;
        void CleanupPlugins() noexcept;


        winrt::hstring m_inputDeviceId{ };

        winrt::com_ptr<TEndpointAbstraction> m_inputAbstraction{ nullptr };

        void InputIsOpen(_In_ bool const isOpen) noexcept { m_isOpen = isOpen; }
        bool InputIsOpen() const noexcept { return m_isOpen; }

        HRESULT CallbackImpl(_In_ foundation::IInspectable eventSource, _In_ PVOID data, _In_ UINT size, _In_ LONGLONG timestamp);

        foundation::Collections::IVector<midi2::IMidiEndpointMessageProcessingPlugin>
            m_messageProcessingPlugins{ winrt::single_threaded_vector<midi2::IMidiEndpointMessageProcessingPlugin>() };

    private:
        bool m_isOpen{ false };


        winrt::event<foundation::TypedEventHandler<foundation::IInspectable, midi2::MidiMessageReceivedEventArgs>> m_messageReceivedEvent;
        winrt::event<foundation::TypedEventHandler<foundation::IInspectable, midi2::MidiStreamConfigurationCompleteEventArgs>> m_streamConfigurationCompleteEvent;

    };


    // Implementation must be in header or other include file





    // Callback handler from the Midi Service endpoint abstraction

    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    HRESULT InternalMidiInputConnection<TEndpointAbstraction>::CallbackImpl(
            foundation::IInspectable eventSource, 
            PVOID data, 
            UINT size, 
            LONGLONG timestamp)
    {
        try
        {
            // one copy of the event args for this gets sent to all listeners and the main event
            auto args = winrt::make_self<midi2::implementation::MidiMessageReceivedEventArgs>(data, size, timestamp);

            // we failed to create the event args
            if (args == nullptr)
            {
                internal::LogGeneralError(__FUNCTION__, L"Unable to create MidiMessageReceivedEventArgs");

                return E_FAIL;
            }

            bool skipMainMessageReceivedEvent = false;
            bool skipFurtherListeners = false;

            // If any listeners are hooked up, use them

            if (m_messageProcessingPlugins && m_messageProcessingPlugins.Size() > 0)
            {
                // loop through listeners
                for (const auto& plugin : m_messageProcessingPlugins)
                {
                    auto listener = plugin.try_as<midi2::IMidiEndpointMessageListener>();

                    if (listener != nullptr)
                    {
                        // This is synchronous by design, but that requires the listener (and the client app which sinks any event) to not block

                        listener.ProcessIncomingMessage(*args, skipFurtherListeners, skipMainMessageReceivedEvent);
                    }

                    // if the listener has told us to skip further listeners, effectively 
                    // removing this message from the queue, then break out of the loop
                    if (skipFurtherListeners) break;
                }
            }

            // if the main message received event is hooked up, and we're not skipping it, use it
            if (m_messageReceivedEvent && !skipMainMessageReceivedEvent)
            {
                m_messageReceivedEvent(eventSource, *args);
            }

            return S_OK;
        }
        catch (winrt::hresult_error const& ex)
        {
            internal::LogHresultError(__FUNCTION__, L"hresult exception calling message handlers", ex);

            return E_FAIL;
        }
    }








    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::SetInputConnectionOnPlugins(midi2::IMidiInputConnection inputConnection) noexcept
    {
        try
        {
            for (const auto& plugin : m_messageProcessingPlugins)
            {
                auto listener = plugin.try_as<midi2::IMidiEndpointMessageListener>();

                if (listener != nullptr)
                {
                    listener.InputConnection(inputConnection);
                }
            }
        }
        catch (...)
        {
            internal::LogGeneralError(__FUNCTION__, L"Exception setting input connection on plugins.");
        }
    }

    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::SetOutputConnectionOnPlugins(midi2::IMidiOutputConnection outputConnection) noexcept
    {
        try
        {
            for (const auto& plugin : m_messageProcessingPlugins)
            {
                auto responder = plugin.try_as<midi2::IMidiEndpointMessageResponder>();

                if (responder != nullptr)
                {
                    responder.OutputConnection(outputConnection);
                }
            }
        }
        catch (...)
        {
            internal::LogGeneralError(__FUNCTION__, L"Exception setting output connection on plugins.");
        }
    }

    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::SetRequestedStreamConfigurationOnPlugins(midi2::MidiStreamConfigurationRequestedSettings settings) noexcept
    {
        try
        {
            for (const auto& plugin : m_messageProcessingPlugins)
            {
                auto configurator = plugin.try_as<midi2::IMidiEndpointConfiguratorPlugin>();

                if (configurator != nullptr)
                {
                    configurator.RequestedStreamConfiguration(settings);
                }
            }
        }
        catch (...)
        {
            internal::LogGeneralError(__FUNCTION__, L"Exception setting requested stream configuration.");
        }
    }



    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::InitializePlugins() noexcept
    {
        for (const auto& plugin : m_messageProcessingPlugins)
        {
            try
            {
                plugin.Initialize();
            }
            catch (...)
            {
                internal::LogGeneralError(__FUNCTION__, L"Exception initializing plugins.");

            }
        }
    }

    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::CallOnConnectionOpenedOnPlugins() noexcept
    {
        for (const auto& plugin : m_messageProcessingPlugins)
        {
            try
            {
                plugin.OnEndpointConnectionOpened();
            }
            catch (...)
            {
                internal::LogGeneralError(__FUNCTION__, L"Exception calling Open on plugins.");
            }
        }
    }

    template<typename TEndpointAbstraction>
    _Use_decl_annotations_
    void InternalMidiInputConnection<TEndpointAbstraction>::CleanupPlugins() noexcept
    {
        for (const auto& plugin : m_messageProcessingPlugins)
        {
            try
            {
                plugin.Cleanup();
            }
            catch (...)
            {
                internal::LogGeneralError(__FUNCTION__, L"Exception cleaning up plugins.");
            }
        }
    }

}