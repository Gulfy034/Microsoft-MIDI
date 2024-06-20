// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#pragma once
#include "MidiServiceSessionInfo.g.h"


namespace winrt::Microsoft::Windows::Devices::Midi2::Diagnostics::implementation
{
    struct MidiServiceSessionInfo : MidiServiceSessionInfoT<MidiServiceSessionInfo>
    {
        MidiServiceSessionInfo() = default;

        winrt::guid SessionId() { return m_sessionId; }
        uint64_t ProcessId() { return m_processId; }
        winrt::hstring ProcessName() { return m_processName; }
        winrt::hstring SessionName() { return m_sessionName; }
        foundation::DateTime StartTime() { return m_startTime; }

        collections::IVectorView<diag::MidiServiceSessionConnectionInfo> Connections() { return m_connections.GetView(); }


        void InternalInitialize(
            _In_ winrt::guid sessionId,
            _In_ winrt::hstring sessionName,
            _In_ uint64_t processId,
            _In_ winrt::hstring processName,
            _In_ foundation::DateTime startTime
        );

        void InternalAddConnection(
            _In_ diag::MidiServiceSessionConnectionInfo const& info
        );

    private:
        winrt::guid m_sessionId{};
        uint64_t m_processId{};
        winrt::hstring m_processName{};
        winrt::hstring m_sessionName{};
        foundation::DateTime m_startTime{};

        foundation::Collections::IVector<diag::MidiServiceSessionConnectionInfo>
            m_connections{ winrt::single_threaded_vector<diag::MidiServiceSessionConnectionInfo>() };


    };
}
