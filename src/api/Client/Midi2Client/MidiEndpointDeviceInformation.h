// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once
#include "MidiEndpointDeviceInformation.g.h"

namespace winrt::Windows::Devices::Midi2::implementation
{
    struct MidiEndpointDeviceInformation : MidiEndpointDeviceInformationT<MidiEndpointDeviceInformation>
    {
        MidiEndpointDeviceInformation() = default;

        static midi2::MidiEndpointDeviceInformation CreateFromId(_In_ winrt::hstring const& id) noexcept;

        //static winrt::hstring UniversalMidiPacketBidirectionalInterfaceClassId() noexcept { return L"" /* STRING_DEVINTERFACE_UNIVERSALMIDIPACKET_BIDI */; }

        static winrt::hstring DiagnosticsLoopbackAEndpointId() noexcept { return LOOPBACK_BIDI_ID_A; }
        static winrt::hstring DiagnosticsLoopbackBEndpointId() noexcept { return LOOPBACK_BIDI_ID_B; }

        static midi2::MidiEndpointDeviceWatcher CreateWatcher(_In_ bool includeDiagnosticsEndpoints);
        static collections::IVectorView<midi2::MidiEndpointDeviceInformation> FindAll(_In_ bool includeDiagnosticsEndpoints);

        static collections::IVectorView<winrt::hstring> GetAdditionalPropertiesList() noexcept;


        winrt::hstring Id() const noexcept;
        winrt::hstring ParentDeviceId() const noexcept;

        winrt::Windows::Devices::Enumeration::DeviceInformation GetParentDeviceInformation();
        winrt::Windows::Devices::Enumeration::DeviceInformation DeviceInformation() noexcept { return m_deviceInformation; }

        winrt::hstring Name() const noexcept;

        winrt::hstring TransportSuppliedName() const noexcept { return m_deviceInformation.Name(); }  // todo: may need to update this later
        winrt::hstring EndpointSuppliedName() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_EndpointProvidedName, L""); }
        winrt::hstring UserSuppliedName() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_UserSuppliedEndpointName, L""); }

        winrt::hstring TransportId() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_AbstractionLayer, L""); }
        winrt::hstring TransportMnemonic() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_TransportMnemonic, L""); }
        winrt::hstring Description() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_UserSuppliedDescription, L""); }
        winrt::hstring ImagePath() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_UserSuppliedImagePath, L""); }


        winrt::hstring UniqueIdentifier() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_UniqueIdentifier, L"");}

        bool SupportsMultiClient() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_SupportsMultiClient, true); }

        midi2::MidiEndpointNativeDataFormat NativeDataFormat() const noexcept;
        midi2::MidiEndpointDevicePurpose EndpointPurpose() const noexcept;


        collections::IVectorView<midi2::MidiFunctionBlock> FunctionBlocks() const noexcept;
        collections::IVectorView<midi2::MidiGroupTerminalBlock> GroupTerminalBlocks() const noexcept;


        winrt::hstring ProductInstanceId() const noexcept { return GetStringProperty(STRING_PKEY_MIDI_EndpointProvidedProductInstanceId, L""); }
        uint8_t SpecificationVersionMajor() const noexcept { return GetByteProperty(STRING_PKEY_MIDI_EndpointUmpVersionMajor, (uint8_t)0); }
        uint8_t SpecificationVersionMinor() const noexcept { return GetByteProperty(STRING_PKEY_MIDI_EndpointUmpVersionMinor, (uint8_t)0); }
        bool HasStaticFunctionBlocks() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_EndpointUmpVersionMinor, false); }
        bool SupportsMidi10Protocol() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_EndpointSupportsMidi1Protocol, false); }
        bool SupportsMidi20Protocol() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_EndpointSupportsMidi2Protocol, false); }
        bool SupportsReceivingJRTimestamps() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_EndpointSupportsReceivingJRTimestamps, false); }
        bool SupportsSendingJRTimestamps() const noexcept { return GetBoolProperty(STRING_PKEY_MIDI_EndpointSupportsSendingJRTimestamps, false); }

        midi2::MidiProtocol ConfiguredProtocol() const noexcept;


        void InternalUpdateFromDeviceInformation(_In_ winrt::Windows::Devices::Enumeration::DeviceInformation const& info) noexcept;
        

    private:
        winrt::hstring GetStringProperty(
            _In_ winrt::hstring key,
            _In_ winrt::hstring defaultValue) const noexcept;

        uint8_t GetByteProperty(
            _In_ winrt::hstring key,
            _In_ uint8_t defaultValue) const noexcept;

        bool GetBoolProperty(
            _In_ winrt::hstring key,
            _In_ bool defaultValue) const noexcept;


        winrt::Windows::Devices::Enumeration::DeviceInformation m_deviceInformation{ nullptr };

    };
}
namespace winrt::Windows::Devices::Midi2::factory_implementation
{
    struct MidiEndpointDeviceInformation : MidiEndpointDeviceInformationT<MidiEndpointDeviceInformation, implementation::MidiEndpointDeviceInformation>
    {
    };
}
