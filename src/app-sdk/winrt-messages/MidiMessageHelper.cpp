// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App SDK and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#include "pch.h"
#include "MidiMessageHelper.h"
#include "MidiMessageHelper.g.cpp"

namespace winrt::Microsoft::Windows::Devices::Midi2::Messages::implementation
{
    _Use_decl_annotations_
    collections::IVector<midi2::IMidiUniversalPacket> MidiMessageHelper::GetPacketListFromWordList(
        uint64_t const timestamp, 
        collections::IIterable<uint32_t> const& words)
    {
        auto result = winrt::single_threaded_vector<midi2::IMidiUniversalPacket>();

        auto iter = words.First();

        while (iter.HasCurrent())
        {
           // auto wordsLeft = words.Size() - index;

            uint8_t numWords = internal::GetUmpLengthInMidiWordsFromFirstWord(iter.Current());

            if (numWords == 1)
            {
                midi2::MidiMessage32 ump{};
                //auto ump = winrt::make<MidiMessage32>();

                ump.Timestamp(timestamp);

                ump.Word0(iter.Current());
                iter.MoveNext();

                result.Append(std::move(ump));
            }

            else if (numWords == 2)
            {
                midi2::MidiMessage64 ump{};
                //auto ump = winrt::make<MidiMessage64>();

                ump.Timestamp(timestamp);

                ump.Word0(iter.Current());

                if (iter.MoveNext()) ump.Word1(iter.Current()); else break;
                
                iter.MoveNext();

                result.Append(std::move(ump));
            }

            else if (numWords == 3)
            {
                midi2::MidiMessage96 ump{};
                //auto ump = winrt::make<MidiMessage96>();

                ump.Timestamp(timestamp);

                ump.Word0(iter.Current());
                iter.MoveNext();

                if (iter.MoveNext()) ump.Word1(iter.Current()); else break;
                if (iter.MoveNext()) ump.Word2(iter.Current()); else break;

                iter.MoveNext();

                result.Append(std::move(ump));
            }

            else if (numWords == 4)
            {
                midi2::MidiMessage128 ump{};
                //auto ump = winrt::make<MidiMessage128>();

                ump.Timestamp(timestamp);

                ump.Word0(iter.Current());
                iter.MoveNext();

                if (iter.MoveNext()) ump.Word1(iter.Current()); else break;
                if (iter.MoveNext()) ump.Word2(iter.Current()); else break;
                if (iter.MoveNext()) ump.Word3(iter.Current()); else break;

                result.Append(std::move(ump));
            }
        }

        return result;
    }

    _Use_decl_annotations_
    collections::IVector<uint32_t> MidiMessageHelper::GetWordListFromPacketList(
        collections::IIterable<midi2::IMidiUniversalPacket> const& messages) noexcept
    {
        // we're doing this the safe and easy way, but there's likely a more efficient way to copy the memory over

        auto result = winrt::single_threaded_vector<uint32_t>();

        for (auto const& message : messages)
        {
            message.AppendAllMessageWordsToList(result);
        }

        return result;
    }



    _Use_decl_annotations_
    bool MidiMessageHelper::ValidateMessage32MessageType(uint32_t const word0) noexcept
    {
        return internal::GetUmpLengthInMidiWordsFromFirstWord(word0) == 1;
    }

    _Use_decl_annotations_
    bool MidiMessageHelper::ValidateMessage64MessageType(uint32_t const word0) noexcept
    {
        return internal::GetUmpLengthInMidiWordsFromFirstWord(word0) == 2;
    }

    _Use_decl_annotations_
    bool MidiMessageHelper::ValidateMessage96MessageType(uint32_t const word0) noexcept
    {
        return internal::GetUmpLengthInMidiWordsFromFirstWord(word0) == 3;
    }

    _Use_decl_annotations_
    bool MidiMessageHelper::ValidateMessage128MessageType(uint32_t const word0) noexcept
    {
        return internal::GetUmpLengthInMidiWordsFromFirstWord(word0) == 4;
    }

    _Use_decl_annotations_
    midi2::MidiMessageType MidiMessageHelper::GetMessageTypeFromMessageFirstWord(uint32_t const word0) noexcept
    {
        return (midi2::MidiMessageType)internal::GetUmpMessageTypeFromFirstWord(word0);
    }

    _Use_decl_annotations_
    midi2::MidiPacketType MidiMessageHelper::GetPacketTypeFromMessageFirstWord(uint32_t const word0) noexcept
    {
        return (midi2::MidiPacketType)internal::GetUmpLengthInMidiWordsFromFirstWord(word0);
    }

    _Use_decl_annotations_
    midi2::MidiGroup MidiMessageHelper::GetGroupFromMessageFirstWord(_In_ uint32_t const word0)
    {
        return midi2::MidiGroup(internal::GetGroupIndexFromFirstWord(word0));
    }


    // It's expected for the user to check to see if the message type has a group field before calling this
    _Use_decl_annotations_
    uint32_t MidiMessageHelper::ReplaceGroupInMessageFirstWord(uint32_t const word0, midi2::MidiGroup const newGroup) noexcept
    {
        return internal::GetFirstWordWithNewGroup(word0, newGroup.Index());
    }

    _Use_decl_annotations_
    bool MidiMessageHelper::MessageTypeHasGroupField(midi2::MidiMessageType const messageType) noexcept
    {
        return internal::MessageTypeHasGroupField((uint8_t)messageType);
    }


    _Use_decl_annotations_
    midi2::MidiChannel MidiMessageHelper::GetChannelFromMessageFirstWord(_In_ uint32_t const word0)
    {
        return midi2::MidiChannel(internal::GetChannelIndexFromFirstWord(word0));
    }


    // It's expected for the user to check to see if the message type has a channel field before calling this
    _Use_decl_annotations_
    uint32_t MidiMessageHelper::ReplaceChannelInMessageFirstWord(uint32_t const word0, midi2::MidiChannel const newChannel) noexcept
    {
        return internal::GetFirstWordWithNewChannel(word0, newChannel.Index());
    }

    _Use_decl_annotations_
    bool MidiMessageHelper::MessageTypeHasChannelField(midi2::MidiMessageType const messageType) noexcept
    {
        return internal::MessageTypeHasChannelField((uint8_t)messageType);
    }


    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusFromUtilityMessage(uint32_t const word0) noexcept
    {
        return internal::GetStatusFromUmp32FirstWord(word0);
    }

    _Use_decl_annotations_
    Midi1ChannelVoiceMessageStatus MidiMessageHelper::GetStatusFromMidi1ChannelVoiceMessage(uint32_t const word0) noexcept
    {
        return (Midi1ChannelVoiceMessageStatus)internal::GetStatusFromChannelVoiceMessage(word0);
    }

    _Use_decl_annotations_
    Midi2ChannelVoiceMessageStatus MidiMessageHelper::GetStatusFromMidi2ChannelVoiceMessageFirstWord(uint32_t const word0) noexcept
    {
        return (Midi2ChannelVoiceMessageStatus)internal::GetStatusFromChannelVoiceMessage(word0);
    }


    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetFormFromStreamMessageFirstWord(uint32_t const word0) noexcept
    {
        return internal::GetFormFromStreamMessageFirstWord(word0);
    }

    _Use_decl_annotations_
    uint16_t MidiMessageHelper::GetStatusFromStreamMessageFirstWord(uint32_t const word0) noexcept
    {
        return internal::GetStatusFromStreamMessageFirstWord(word0);
    }


    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusBankFromFlexDataMessageFirstWord(uint32_t const word0) noexcept
    {
        return internal::GetStatusBankFromFlexDataMessageFirstWord(word0);
    }
    
    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusFromFlexDataMessageFirstWord(uint32_t const word0) noexcept
    {
        return internal::GetStatusFromFlexDataMessageFirstWord(word0);
    }

    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusFromSystemCommonMessage(_In_ uint32_t const word0) noexcept
    {
        return internal::GetStatusFromSystemCommonMessage(word0);
    }


    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusFromDataMessage64FirstWord(_In_ uint32_t const word0) noexcept
    {
        return internal::GetStatusFromDataMessage64FirstWord(word0);
    }

    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetNumberOfBytesFromDataMessage64FirstWord(_In_ uint32_t const word0) noexcept
    {
        return internal::GetNumberOfBytesFromDataMessage64FirstWord(word0);
    }


    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetStatusFromDataMessage128FirstWord(_In_ uint32_t const word0) noexcept
    {
        return internal::GetStatusFromDataMessage128FirstWord(word0);
    }

    _Use_decl_annotations_
    uint8_t MidiMessageHelper::GetNumberOfBytesFromDataMessage128FirstWord(_In_ uint32_t const word0) noexcept
    {
        return internal::GetNumberOfBytesFromDataMessage128FirstWord(word0);
    }


    // this works for classic note indexes 0-127
    _Use_decl_annotations_
    winrt::hstring MidiMessageHelper::GetNoteDisplayNameFromNoteIndex(uint8_t const noteIndex) noexcept
    {
        static const winrt::hstring noteNames[]{ L"C", L"C#/Db", L"D", L"D#/Eb", L"E", L"F", L"F#/Gb", L"G", L"G#/Ab", L"A", L"A#/Bb", L"B" };

        if (noteIndex > 0x7F) return internal::ResourceGetHString(IDS_NOTE_INVALID);

        return noteNames[noteIndex % _countof(noteNames)];
    }

    // this works for classic note indexes 0-127
    _Use_decl_annotations_
    int16_t MidiMessageHelper::GetNoteOctaveFromNoteIndex(uint8_t const noteIndex) noexcept
    {
        // default octave range is -2 to 8 with Middle C as C3. Note 0 is C -2, C0 is index 24

        return GetNoteOctaveFromNoteIndex(noteIndex, 3);
    }

    // this works for classic note indexes 0-127
    _Use_decl_annotations_
    int16_t MidiMessageHelper::GetNoteOctaveFromNoteIndex(uint8_t const noteIndex, uint8_t middleCOctave) noexcept
    {
        if (noteIndex > 0x7F) return 0;
        if ((middleCOctave < 1) || (middleCOctave > 7)) return 0;   // Middle C is typically 3, 4, or even 5. We allow a bit more.

        return static_cast<int16_t>((noteIndex / 12) - (middleCOctave - 1));
    }


    // Names used in this function are those used in the MIDI 2.0 Specification
    // This will need to grow over time as new messages are added
    _Use_decl_annotations_
    winrt::hstring MidiMessageHelper::GetMessageDisplayNameFromFirstWord(uint32_t const word0) noexcept
    {
        switch (GetMessageTypeFromMessageFirstWord(word0))
        {
        case midi2::MidiMessageType::UtilityMessage32:
            switch (GetStatusFromUtilityMessage(word0))
            {
            case 0x0:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_NOOP);
            case 0x1:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_JR_CLOCK);
            case 0x2:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_JR_TS);
            case 0x3:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_TICKS_PQN);
            case 0x4:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_DELTA_TICKS);

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT0_UNKNOWN);
            }
            break;

        case midi2::MidiMessageType::SystemCommon32:
            switch (GetStatusFromSystemCommonMessage(word0))
            {
            case 0xF0:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F0_RESERVED); //L"Reserved 0xF0";
            case 0xF1:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F1_TIME_CODE); //L"MIDI Time Code";
            case 0xF2:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F2_SONG_POSITION); //L"Song Position Pointer";
            case 0xF3:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F3_SONG_SELECT); //L"Song Select";
            case 0xF4:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F4_RESERVED); //L"Reserved 0xF4";
            case 0xF5:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F5_RESERVED); //L"Reserved 0xF5";
            case 0xF6:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F6_TUNE_REQUEST); //L"Tune Request";
            case 0xF7:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F7_RESERVED); //L"Reserved 0xF7";
            case 0xF8:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F8_TIMING_CLOCK); //L"Timing Clock";
            case 0xF9:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_F9_RESERVED); //L"Reserved 0xF9";
            case 0xFA:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FA_START); //L"Start";
            case 0xFB:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FB_CONTINUE); //L"Continue";
            case 0xFC:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FC_STOP); //L"Stop";
            case 0xFD:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FD_RESERVED); //L"Reserved 0xFD";
            case 0xFE:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FE_ACTIVE_SENSE); //L"Active Sensing";
            case 0xFF:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_FF_RESET); //L"Reset";

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT1_UNKNOWN); //L"System Common/Realtime Unknown";
            }
            break;

        case midi2::MidiMessageType::Midi1ChannelVoice32:
            switch (GetStatusFromMidi1ChannelVoiceMessage(word0))
            {
            case msgs::Midi1ChannelVoiceMessageStatus::NoteOff:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_8_NOTE_OFF); //L"MIDI 1.0 Note Off";
            case msgs::Midi1ChannelVoiceMessageStatus::NoteOn:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_9_NOTE_ON); //L"MIDI 1.0 Note On";
            case msgs::Midi1ChannelVoiceMessageStatus::PolyPressure:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_A_POLY_PRESSURE); //L"MIDI 1.0 Poly Pressure";
            case msgs::Midi1ChannelVoiceMessageStatus::ControlChange:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_B_CONTROL_CHANGE); //L"MIDI 1.0 Control Change";
            case msgs::Midi1ChannelVoiceMessageStatus::ProgramChange:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_C_PROGRAM_CHANGE); //L"MIDI 1.0 Program Change";
            case msgs::Midi1ChannelVoiceMessageStatus::ChannelPressure:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_D_CHANNEL_PRESSURE); //L"MIDI 1.0 Channel Pressure";
            case msgs::Midi1ChannelVoiceMessageStatus::PitchBend:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_E_PITCH_BEND); //L"MIDI 1.0 Pitch Bend";

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT2_UNKNOWN); //L"MIDI 1.0 Channel Voice";
            }
            break;

        case midi2::MidiMessageType::DataMessage64:
            switch (GetStatusFromDataMessage64FirstWord(word0))
            {
            case 0x0:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT3_0_SYSEX7_COMPLETE); //L"SysEx 7-bit Complete";
            case 0x1:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT3_1_SYSEX7_START); //L"SysEx 7-bit Start";
            case 0x2:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT3_2_SYSEX7_CONTINUE); //L"SysEx 7-bit Continue";
            case 0x3:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT3_3_SYSEX7_END); //L"SysEx 7-bit End";

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT3_DATA64_UNKNOWN); //L"Data Message 64";
            }
            break;

        case midi2::MidiMessageType::Midi2ChannelVoice64:
            switch (GetStatusFromMidi2ChannelVoiceMessageFirstWord(word0))
            {
            case msgs::Midi2ChannelVoiceMessageStatus::RegisteredPerNoteController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_0_RPNC); //L"MIDI 2.0 Registered Per-Note Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::AssignablePerNoteController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_1_APNC); //L"MIDI 2.0 Assignable Per-Note Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::RegisteredController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_2_RC); //L"MIDI 2.0 Registered Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::AssignableController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_3_AC); //L"MIDI 2.0 Assignable Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::RelativeRegisteredController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_4_REL_RC); //L"MIDI 2.0 Relative Registered Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::RelativeAssignableController:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_5_REL_AC); //L"MIDI 2.0 Relative Assignable Controller";
            case msgs::Midi2ChannelVoiceMessageStatus::PerNotePitchBend:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_6_PER_NOTE_BEND); //L"MIDI 2.0 Per-Note Pitch Bend";
            case msgs::Midi2ChannelVoiceMessageStatus::NoteOff:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_8_NOTE_OFF); //L"MIDI 2.0 Note Off";
            case msgs::Midi2ChannelVoiceMessageStatus::NoteOn:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_9_NOTE_ON); //L"MIDI 2.0 Note On";
            case msgs::Midi2ChannelVoiceMessageStatus::PolyPressure:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_A_POLY_PRESSURE); //L"MIDI 2.0 Poly Pressure";
            case msgs::Midi2ChannelVoiceMessageStatus::ControlChange:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_B_CONTROL_CHANGE); //L"MIDI 2.0 Control Change";
            case msgs::Midi2ChannelVoiceMessageStatus::ProgramChange:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_C_PROGRAM_CHANGE); //L"MIDI 2.0 Program Change";
            case msgs::Midi2ChannelVoiceMessageStatus::ChannelPressure:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_D_CHANNEL_PRESSURE); //L"MIDI 2.0 Channel Pressure";
            case msgs::Midi2ChannelVoiceMessageStatus::PitchBend:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_E_PITCH_BEND); //L"MIDI 2.0 Pitch Bend";

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT4_MIDI2_CV_UNKNOWN); //L"MIDI 2.0 Channel Voice";
            }
            break;

        case midi2::MidiMessageType::DataMessage128:
            switch (GetStatusFromDataMessage128FirstWord(word0))
            {
            case 0x0:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_0_SYSEX8_COMPLETE); //L"SysEx 8-bit Complete";
            case 0x1:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_1_SYSEX8_START); //L"SysEx 8-bit Start";
            case 0x2:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_2_SYSEX8_CONTINUE); //L"SysEx 8-bit Continue";
            case 0x3:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_3_SYSEX8_END); //L"SysEx 8-bit End";

            case 0x8:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_8_MIXED_DATA_HEADER); //L"Mixed Data Set Header";
            case 0x9:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_9_MIXED_DATA_PAYLOAD); //L"Mixed Data Set Payload";

            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT5_UNKNOWN); //L"Data Message Unknown";
            }
            break;

        case midi2::MidiMessageType::FutureReserved632:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT6_RESERVED); //L"Unknown Type 6";

        case midi2::MidiMessageType::FutureReserved732:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT7_RESERVED); //L"Unknown Type 7";

        case midi2::MidiMessageType::FutureReserved864:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT8_RESERVED); //L"Unknown Type 8";

        case midi2::MidiMessageType::FutureReserved964:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT9_RESERVED); //L"Unknown Type 9";

        case midi2::MidiMessageType::FutureReservedA64:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTA_RESERVED); //L"Unknown Type A";

        case midi2::MidiMessageType::FutureReservedB96:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTB_RESERVED); //L"Unknown Type B";

        case midi2::MidiMessageType::FutureReservedC96:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTC_RESERVED); //L"Unknown Type C";

        case midi2::MidiMessageType::FlexData128:
            {
                uint8_t statusBank = GetStatusBankFromFlexDataMessageFirstWord(word0);
                uint8_t status = GetStatusFromFlexDataMessageFirstWord(word0);

                switch (statusBank)
                {
                case 0x01:
                    switch (status)
                    {
                    case 0x00:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_00_METADATA_TEXT); //L"Metadata Text Event Status 0x00";
                    case 0x01:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_01_PROJECT_NAME); //L"Project Name";
                    case 0x02:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_02_SONG_NAME); //L"Composition (Song) Name";
                    case 0x03:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_03_CLIP_NAME); //L"MIDI Clip Name";
                    case 0x04:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_04_COPYRIGHT); //L"Copyright Notice";
                    case 0x05:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_05_COMPOSER_NAME); //L"Composer Name";
                    case 0x06:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_06_LYRICIST_NAME); //L"Lyricist Name";
                    case 0x07:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_07_ARRANGER_NAME); //L"Arranger Name";
                    case 0x08:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_08_PUBLISHER_NAME); //L"Publisher Name";
                    case 0x09:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_09_PRIMARY_PERFORMER_NAME); //L"Primary Performer Name";
                    case 0x0A:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_0A_ACCOMPANY_PERFORMER_NAME); //L"Accompanying Performer Name";
                    case 0x0B:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_0B_RECORDING_DATE); //L"Recording / Concert Date";
                    case 0x0C:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_0C_RECORDING_LOCATION); //L"Recording / Concert Location";
                    default:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_01_UNKNOWN); //L"Flex Data with Bank 0x01";
                    }
                    break;
                case 0x02:
                    switch (status)
                    {
                    case 0x00:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_00_PERF_TEXT_EVENT); //L"Performance Text Event Status 0x00";
                    case 0x01:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_01_LYRICS); //L"Lyrics";
                    case 0x02:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_02_LYRICS_LANGUAGE); //L"Lyrics Language";
                    case 0x03:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_03_RUBY); //L"Ruby";
                    case 0x04:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_04_RUBY_LANGUAGE); //L"Ruby Language";
                    default:
                        return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_02_UNKNOWN); //L"Flex Data with Bank 0x02";
                    }
                    break;
                default:
                    return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTD_UNKNOWN); //L"Flex Data Unknown";

                }
            }

        case midi2::MidiMessageType::FutureReservedE128:
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTE_RESERVED); //L"Type E Unknown";

        case midi2::MidiMessageType::Stream128:
            switch (GetStatusFromStreamMessageFirstWord(word0))
            {
            case MIDI_STREAM_MESSAGE_STATUS_ENDPOINT_DISCOVERY:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_00_DISCOVERY); //L"Endpoint Discovery";
            case MIDI_STREAM_MESSAGE_STATUS_ENDPOINT_INFO_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_01_ENDPOINT_INFO); //L"Endpoint Info Notification";
            case MIDI_STREAM_MESSAGE_STATUS_DEVICE_IDENTITY_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_02_DEVICE_IDENTITY); //L"Device Identity Notification";
            case MIDI_STREAM_MESSAGE_STATUS_ENDPOINT_NAME_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_03_ENDPOINT_NAME); //L"Endpoint Name Notification";
            case MIDI_STREAM_MESSAGE_STATUS_ENDPOINT_PRODUCT_INSTANCE_ID_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_04_PRODUCT_INSTANCE_ID); //L"Product Instance Id Notification";
            case MIDI_STREAM_MESSAGE_STATUS_STREAM_CONFIGURATION_REQUEST:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_05_CONFIG_REQUEST); //L"Stream Configuration Request";
            case MIDI_STREAM_MESSAGE_STATUS_STREAM_CONFIGURATION_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_06_CONFIG_NOTIFICATION); //L"Stream Configuration Notification";

            case MIDI_STREAM_MESSAGE_STATUS_FUNCTION_BLOCK_DISCOVERY:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_10_FUNCTION_BLOCK_DISCOVERY); //L"Function Block Info Notification";
            case MIDI_STREAM_MESSAGE_STATUS_FUNCTION_BLOCK_INFO_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_11_FUNCTION_BLOCK_INFO); //L"Function Block Info Notification";
            case MIDI_STREAM_MESSAGE_STATUS_FUNCTION_BLOCK_NAME_NOTIFICATION:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_12_FUNCTION_BLOCK_NAME); //L"Function Block Name Notification";
            default:
                return internal::ResourceGetHString(IDS_MESSAGE_DESC_MTF_UNKNOWN); //L"Stream Message Unknown";
            }
            break;
        default:
            // this is here just to satisfy the compiler because it doesn't understand 4-bit values
            return internal::ResourceGetHString(IDS_MESSAGE_DESC_MT_UNKNOWN); //L"Unknown";
        };


    }

}

