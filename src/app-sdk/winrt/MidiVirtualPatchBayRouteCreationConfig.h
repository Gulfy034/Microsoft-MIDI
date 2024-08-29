// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#pragma once
#include "VirtualPatchBay.MidiVirtualPatchBayRouteCreationConfig.g.h"

namespace winrt::Microsoft::Windows::Devices::Midi2::VirtualPatchBay::implementation
{
    struct MidiVirtualPatchBayRouteCreationConfig : MidiVirtualPatchBayRouteCreationConfigT<MidiVirtualPatchBayRouteCreationConfig>
    {
        MidiVirtualPatchBayRouteCreationConfig() = default;
        MidiVirtualPatchBayRouteCreationConfig(_In_ winrt::guid const& routeId);
        
        winrt::guid RouteId() { return m_routeId; }
        void RouteId(_In_ winrt::guid const& value) { m_routeId = value; }

        winrt::guid TransportId() { return m_transportId; }
        bool IsFromCurrentConfigFile() { return m_isFromCurrentConfigFile; }
        winrt::hstring GetConfigJson();

    private:
        winrt::guid m_routeId{};
        winrt::guid m_transportId{};
        bool m_isFromCurrentConfigFile{ false };

    };
}
namespace winrt::Microsoft::Windows::Devices::Midi2::VirtualPatchBay::factory_implementation
{
    struct MidiVirtualPatchBayRouteCreationConfig : MidiVirtualPatchBayRouteCreationConfigT<MidiVirtualPatchBayRouteCreationConfig, implementation::MidiVirtualPatchBayRouteCreationConfig>
    {
    };
}
