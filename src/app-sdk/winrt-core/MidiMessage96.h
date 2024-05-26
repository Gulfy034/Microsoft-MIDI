// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#pragma once

#include "pch.h"

#include "MidiMessage96.g.h"


namespace winrt::Microsoft::Windows::Devices::Midi2::implementation
{
    struct MidiMessage96 : MidiMessage96T<MidiMessage96>
    {
        MidiMessage96() = default;
        MidiMessage96(
            _In_ internal::MidiTimestamp const timestamp, 
            _In_ uint32_t const word0, 
            _In_ uint32_t const word1, 
            _In_ uint32_t const word2);

        // TODO: This doesn't do any bounds checking, and it should
        MidiMessage96(
            _In_ internal::MidiTimestamp const timestamp, 
            _In_ array_view<uint32_t const> words)
        {
            if (words.size() == 3) InternalInitializeFromPointer(timestamp, (PVOID)words.data());
        }


        // internal
        void InternalInitializeFromPointer(
            _In_ internal::MidiTimestamp timestamp, 
            _In_ PVOID data);


        uint32_t Word0() const noexcept { return m_ump.word0; }
        void Word0(_In_ uint32_t value) noexcept { m_ump.word0 = value; }

        uint32_t Word1() const noexcept { return m_ump.word1; }
        void Word1(_In_ uint32_t value) noexcept { m_ump.word1 = value; }

        uint32_t Word2() const noexcept { return m_ump.word2; }
        void Word2(_In_ uint32_t value) noexcept { m_ump.word2 = value; }

        internal::MidiTimestamp Timestamp() const noexcept { return m_timestamp; }
        void Timestamp(_In_ internal::MidiTimestamp value) noexcept { m_timestamp = value; }

        midi2::MidiMessageType MessageType() const noexcept 
            { return (midi2::MidiMessageType)(internal::GetUmpMessageTypeFromFirstWord(m_ump.word0)); }

        void MessageType(_In_ midi2::MidiMessageType const& value) noexcept
            { internal::SetUmpMessageType(m_ump.word0, (uint8_t)value); }

        midi2::MidiPacketType PacketType() const noexcept
            { return midi2::MidiPacketType::UniversalMidiPacket96; }

        collections::IVector<uint32_t> GetAllWords() const noexcept;
        uint8_t AppendAllMessageWordsToList(
            _Inout_ collections::IVector<uint32_t> targetVector) const noexcept;

        uint8_t FillBuffer(
            _In_ uint32_t const byteOffset,
            _In_ foundation::IMemoryBuffer const& buffer
        ) const noexcept;


        // IStringable
        winrt::hstring ToString();

        uint32_t PeekFirstWord() { return Word0(); }


        // internal for the sending code
        internal::PackedUmp96* GetInternalUmpDataPointer() { return &m_ump; }

    private:
        internal::MidiTimestamp m_timestamp{};

        internal::PackedUmp96 m_ump{};

    };
}
namespace winrt::Microsoft::Windows::Devices::Midi2::factory_implementation
{
    struct MidiMessage96 : MidiMessage96T<MidiMessage96, implementation::MidiMessage96>
    {
    };
}
