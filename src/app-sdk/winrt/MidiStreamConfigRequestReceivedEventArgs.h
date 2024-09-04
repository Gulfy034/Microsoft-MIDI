// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#pragma once
#include "Endpoints.Virtual.MidiStreamConfigRequestReceivedEventArgs.g.h"

namespace winrt::Microsoft::Windows::Devices::Midi2::Endpoints::Virtual::implementation
{
    struct MidiStreamConfigRequestReceivedEventArgs : MidiStreamConfigRequestReceivedEventArgsT<MidiStreamConfigRequestReceivedEventArgs>
    {
        MidiStreamConfigRequestReceivedEventArgs() = default;

        internal::MidiTimestamp Timestamp() { return m_timestamp; }
        midi2::MidiProtocol PreferredMidiProtocol() { return m_protocol; }
        bool RequestEndpointTransmitJitterReductionTimestamps() { return m_requestTransmitJRTimestamps; }
        bool RequestEndpointReceiveJitterReductionTimestamps() { return m_requestReceiveJRTimestamps; }

        void InternalInitialize(
            _In_ internal::MidiTimestamp timestamp,
            _In_ midi2::MidiProtocol protocol,
            _In_ bool requestReceiveJRTimestamps,
            _In_ bool requestTransmitJRTimestamps
            );

    private:
        internal::MidiTimestamp m_timestamp{ 0 };
        midi2::MidiProtocol m_protocol{ midi2::MidiProtocol::Default };
        bool m_requestTransmitJRTimestamps{ false };
        bool m_requestReceiveJRTimestamps{ false };

    };
}
