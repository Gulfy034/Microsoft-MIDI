// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#include "pch.h"
#include "MidiMessage96.h"
#include "MidiMessage96.g.cpp"

namespace winrt::Microsoft::Windows::Devices::Midi2::implementation
{
    collections::IVector<uint32_t> MidiMessage96::GetAllWords() const noexcept
    {
        auto vec = winrt::single_threaded_vector<uint32_t>();

        vec.Append(m_ump.word0);
        vec.Append(m_ump.word1);
        vec.Append(m_ump.word2);

        return vec;
    }

    _Use_decl_annotations_
    uint8_t MidiMessage96::AppendAllMessageWordsToList(collections::IVector<uint32_t> targetVector) const noexcept
    {
        targetVector.Append(m_ump.word0);
        targetVector.Append(m_ump.word1);
        targetVector.Append(m_ump.word2);

        return 3;
    }


    _Use_decl_annotations_
    uint8_t MidiMessage96::FillBuffer(uint32_t const byteOffset, foundation::IMemoryBuffer const& buffer) const noexcept
    {
        const uint8_t numWordsInPacket = 3;
        const uint8_t numBytesInPacket = numWordsInPacket * sizeof(uint32_t);

        try
        {
            auto ref = buffer.CreateReference();
            auto interop = ref.as<IMemoryBufferByteAccess>();

            uint8_t* value{};
            uint32_t valueSize{};

            // get a pointer to the buffer
            if (SUCCEEDED(interop->GetBuffer(&value, &valueSize)))
            {
                if (byteOffset + numBytesInPacket > valueSize)
                {
                    // no room
                    return 0;
                }
                else
                {
                    uint32_t* bufferWordPointer = reinterpret_cast<uint32_t*>(value + byteOffset);

                    // copy the number of valid bytes in our internal UMP structure
                    memcpy(bufferWordPointer, &m_ump, numBytesInPacket);

                    return numBytesInPacket;
                }
            }
            else
            {
                return 0;
            }

        }
        catch (...)
        {
            return 0;
        }

    }


    _Use_decl_annotations_
    MidiMessage96::MidiMessage96(
        internal::MidiTimestamp timestamp, 
        uint32_t const word0,
        uint32_t const word1,
        uint32_t const word2)
    {
        m_timestamp = timestamp;

        m_ump.word0 = word0;
        m_ump.word1 = word1;
        m_ump.word2 = word2;
    }

    // internal constructor for reading from the service callback
    _Use_decl_annotations_
    void MidiMessage96::InternalInitializeFromPointer(
        internal::MidiTimestamp timestamp, 
        PVOID data)
    {
        if (data == nullptr) return;

        m_timestamp = timestamp;

        // need to have some safeties around this
        memcpy((void*)&m_ump, data, sizeof(internal::PackedUmp96));
    }



    winrt::hstring MidiMessage96::ToString()
    {
        std::stringstream stream;

        stream << "64-bit MIDI message:"
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << Word0()
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << Word1()
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << Word2();

        return winrt::to_hstring(stream.str());
    }

}
