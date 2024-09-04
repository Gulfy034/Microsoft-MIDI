// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#pragma once
#include "Diagnostics.MidiServicePingResponseSummary.g.h"


namespace winrt::Microsoft::Windows::Devices::Midi2::Diagnostics::implementation
{
    struct MidiServicePingResponseSummary : MidiServicePingResponseSummaryT<MidiServicePingResponseSummary>
    {
        MidiServicePingResponseSummary() = default;

        bool Success() const noexcept { return m_success; }
        winrt::hstring FailureReason() const noexcept { return m_failureReason; }

        internal::MidiTimestamp TotalPingRoundTripMidiClock() const noexcept { return m_totalPingMidiClockTicks; }
        internal::MidiTimestamp AveragePingRoundTripMidiClock() const noexcept { return m_averagePingMidiClockTicks; }

        foundation::Collections::IVectorView<diag::MidiServicePingResponse> Responses() const noexcept { return m_responses.GetView(); }


        void InternalSetFailed(_In_ winrt::hstring failureReason) { m_success = false; m_failureReason = failureReason; }
        void InternalSetSucceeded() { m_success = true; }

        void InternalSetTotals(
            _In_ internal::MidiTimestamp const totalPingRoundTrip, 
            _In_ internal::MidiTimestamp const averagePingRoundTrip
            )
        {
            m_totalPingMidiClockTicks = totalPingRoundTrip;
            m_averagePingMidiClockTicks = averagePingRoundTrip;
        }

        void InternalAddResponse(
            _In_ diag::MidiServicePingResponse response
            )
        {
            m_responses.Append(response);
        }

    private:
        bool m_success{ false };
        winrt::hstring m_failureReason{};

        internal::MidiTimestamp m_totalPingMidiClockTicks{ 0 };
        internal::MidiTimestamp m_averagePingMidiClockTicks{ 0 };

        foundation::Collections::IVector<diag::MidiServicePingResponse>
            m_responses { winrt::single_threaded_vector<diag::MidiServicePingResponse>() };

    };
}
