// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#include "pch.h"
#include "MidiEndpointDeviceInformationAddedEventArgs.h"
#include "MidiEndpointDeviceInformationAddedEventArgs.g.cpp"


namespace winrt::Microsoft::Windows::Devices::Midi2::implementation
{
    _Use_decl_annotations_
    void MidiEndpointDeviceInformationAddedEventArgs::InternalInitialize(
        midi2::MidiEndpointDeviceInformation const& addedDevice
    ) noexcept
    {
        m_addedDevice = addedDevice;
    }

}
