// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once
#include "MidiEndpointConfigurator.g.h"

#include "string_util.h"

namespace winrt::Windows::Devices::Midi2::implementation
{
    struct MidiEndpointConfigurator : MidiEndpointConfiguratorT<MidiEndpointConfigurator>
    {
        MidiEndpointConfigurator() = default;

        hstring Id() const noexcept { return m_id; }
        void Id(_In_ hstring const& value) noexcept { m_id = internal::ToUpperTrimmedHStringCopy(value); }

        hstring Name() const noexcept { return m_name; }
        void Name(_In_ hstring const& value) noexcept { m_name = internal::TrimmedHStringCopy(value); }

        bool IsEnabled() const noexcept { return m_enabled; }
        void IsEnabled(_In_ bool const& value) noexcept { m_enabled = value; }

        winrt::Windows::Foundation::IInspectable Tag() const noexcept { return m_tag; }
        void Tag(_In_ winrt::Windows::Foundation::IInspectable const& value) { m_tag = value; }

        midi2::IMidiInputConnection InputConnection() const noexcept { return m_inputConnection; }
        void InputConnection(_In_ midi2::IMidiInputConnection const& value) noexcept { m_inputConnection = value; }

        midi2::IMidiOutputConnection OutputConnection() const noexcept { return m_outputConnection; }
        void OutputConnection(_In_ midi2::IMidiOutputConnection const& value) noexcept { m_outputConnection = value; }

        //midi2::MidiBidirectionalEndpointOpenOptions BidirectionalEndpointOpenOptions() const noexcept { return m_openOptions; }
        //void BidirectionalEndpointOpenOptions(midi2::MidiBidirectionalEndpointOpenOptions const& value) noexcept { m_openOptions = value; }

        midi2::MidiStreamConfigurationRequestedSettings RequestedStreamConfiguration() const noexcept { return m_configurationRequested; }
        void RequestedStreamConfiguration(_In_ midi2::MidiStreamConfigurationRequestedSettings const& value) noexcept { m_configurationRequested = value; }

        void ProcessIncomingMessage(
            _In_ winrt::Windows::Devices::Midi2::MidiMessageReceivedEventArgs const& args,
            _Out_ bool& skipFurtherListeners,
            _Out_ bool& skipMainMessageReceivedEvent) noexcept;

        void Initialize() noexcept;
        void OnEndpointConnectionOpened() noexcept;
        void Cleanup() noexcept;

        bool BeginDiscovery() noexcept;

        bool BeginNegotiation() noexcept;
        
        //bool RequestStreamConfiguration() noexcept;

        bool RequestAllFunctionBlocks() noexcept;

        bool RequestSingleFunctionBlock(
            _In_ uint8_t functionBlockNumber) noexcept;

    private:
        winrt::hstring m_id{};
        winrt::hstring m_name{};
        bool m_enabled{ false };
        foundation::IInspectable m_tag{ nullptr };
        midi2::IMidiInputConnection m_inputConnection{ nullptr };
        midi2::IMidiOutputConnection m_outputConnection{ nullptr };
        midi2::MidiStreamConfigurationRequestedSettings m_configurationRequested{ nullptr };

        //midi2::MidiBidirectionalEndpointOpenOptions m_openOptions;

    };
}
namespace winrt::Windows::Devices::Midi2::factory_implementation
{
    struct MidiEndpointConfigurator : MidiEndpointConfiguratorT<MidiEndpointConfigurator, implementation::MidiEndpointConfigurator>
    {
    };
}