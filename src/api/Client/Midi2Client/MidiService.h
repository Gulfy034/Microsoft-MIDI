// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once
#include "MidiService.g.h"

namespace winrt::Windows::Devices::Midi2::implementation
{
    struct MidiService : MidiServiceT<MidiService>
    {
        MidiService() = default;

        static winrt::Windows::Devices::Midi2::MidiServicePingResponseSummary PingService(uint8_t pingCount);
        static winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Midi2::MidiTransportInformation> GetInstalledTransports();
    };
}
namespace winrt::Windows::Devices::Midi2::factory_implementation
{
    struct MidiService : MidiServiceT<MidiService, implementation::MidiService>
    {
    };
}
