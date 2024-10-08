// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once


struct MidiNetworkUdpPacket
{
    uint32_t MidiHeader;
};




enum MidiNetworkCommandCode : uint8_t
{
    CommandClientToHost_Invitation = 0x01,
    CommandClientToHost_InvitationWithAuthentication = 0x02,
    CommandClientToHost_InvitationWithUserAuthentication = 0x03,

    CommandHostToClient_InvitationReplyAccepted = 0x10,
    CommandHostToClient_InvitationReplyPending = 0x11,
    CommandHostToClient_InvitationReplyAuthenticationRequired = 0x12,
    CommandHostToClient_InvitationReplyUserAuthenticationRequired = 0x13,

    CommandCommon_UmpData = 0xFF,

    CommandCommon_Ping = 0x20,
    CommandCommon_PingReply = 0x21,
    CommandCommon_RetransmitRequest = 0x80,
    CommandCommon_RetransmitError = 0x81,
    CommandCommon_SessionReset = 0x82,
    CommandCommon_SessionResetReply = 0x83,
    CommandCommon_NAK = 0x8F,
    CommandCommon_Bye = 0xF0,
    CommandCommon_ByeReply = 0xF1,

};

enum MidiNetworkCommandNAKReasons : uint8_t
{
    CommandNAKReason_CommandNotSupported = 0x01,
    CommandNAKReason_CommandNotExpected = 0x02,
    CommandNAKReason_CommandMalformed = 0x03,
    CommandNAKReason_BadPingReply = 0x20,
    
};

enum MidiNetworkCommandByeReasons : uint8_t
{
    CommandByeReasonCommon_Undefined = 0x00,
    CommandByeReasonCommon_UserTerminated = 0x01,
    CommandByeReasonCommon_PowerDown = 0x02,
    CommandByeReasonCommon_TooManyMissingUmps = 0x03,
    CommandByeReasonCommon_Timeout = 0x04,
    CommandByeReasonCommon_SessionNotEstablished = 0x05,
    CommandByeReasonCommon_NoPendingSession = 0x06,
    CommandByeReasonCommon_ProtocolError = 0x07,

    CommandByeReasonHostToClient_TooManyOpenSessions = 0x40,
    CommandByeReasonHostToClient_InvitationWithAuthRejectedMissingPriorAttempt = 0x41,
    CommandByeReasonHostToClient_InvitationRejectedUserDidNotAccept = 0x42,
    CommandByeReasonHostToClient_InvitationRejectedAuthFailed = 0x43,
    CommandByeReasonHostToClient_InvitationRejectedUsernameNotFound = 0x44,
    CommandByeReasonHostToClient_NoMatchingAuthenticationMethod = 0x45,

    CommandByeReasonClientToHost_InvitationCanceled = 0x80,
};


// The order of elements in this is super important. It's
// designed to fit the protocol as it is represented on an
// incoming socket stream. Don't move anything around.

union MidiNetworkCommandPacketHeader
{
    struct
    {
        union
        {
            uint16_t AsUInt16;

            struct
            {
                uint8_t Byte2; // can't use an array endianess is wrong
                uint8_t Byte1;
            } AsBytes;
        } CommandSpecificData;

        uint8_t CommandPayloadLength;
        MidiNetworkCommandCode CommandCode;
    } HeaderData;

    uint32_t HeaderWord;
};


struct MidiNetworkCommandPacket
{
private:
    inline bool IsBufferSize32BitAligned(_In_ size_t const bufferSizeInBytes)
    {
        return (bufferSizeInBytes % sizeof(uint32_t)) == 0;
    }

    inline size_t RoundUpBufferSizeToNext32BitAlignmentIfNeeded(_In_ size_t const currentBufferSizeInBytes)
    {
        if (IsBufferSize32BitAligned(currentBufferSizeInBytes)) return currentBufferSizeInBytes;

        return (currentBufferSizeInBytes + sizeof(uint32_t) - (currentBufferSizeInBytes % sizeof(uint32_t)));
    }


    inline bool IsInternalBuffer32BitAligned()
    {
        return IsBufferSize32BitAligned(DataBuffer.size());
    }

    inline uint8_t CalculateCommandPayloadLength()
    {
        return (uint8_t)(DataBuffer.size() / sizeof(uint32_t));
    }

public:
    MidiNetworkCommandPacket() {}

    MidiNetworkCommandPacketHeader Header{};
    std::vector<uint8_t> DataBuffer{};            // remaining data

    inline void Ensure32BitAlignedInternalBuffer()
    {
        if (!IsInternalBuffer32BitAligned())
        {
            DataBuffer.resize(RoundUpBufferSizeToNext32BitAlignmentIfNeeded(DataBuffer.size()));
        }

    }

    inline void SetMinimumBufferDataSizeAndAlign(_In_ uint16_t const newBufferSizeInBytesExcludingHeader)
    {
        auto newSize = RoundUpBufferSizeToNext32BitAlignmentIfNeeded(newBufferSizeInBytesExcludingHeader);

        DataBuffer.resize(newSize);

        // resize zero-fills, so we don't have to
        //std::fill(DataBuffer.begin(), DataBuffer.end(), 0);

        Header.HeaderData.CommandPayloadLength = CalculateCommandPayloadLength();
    }

    inline void CopyInData(_In_ std::vector<uint8_t> const& source)
    {
        DataBuffer = source;

        Ensure32BitAlignedInternalBuffer();

        Header.HeaderData.CommandPayloadLength = CalculateCommandPayloadLength();
    }

    // assumes the source is null terminated
    inline void CopyInData(_In_ std::string const& source)
    {
        SetMinimumBufferDataSizeAndAlign((uint16_t)source.length());

        std::copy(source.begin(), source.end(), DataBuffer.begin());

        Ensure32BitAlignedInternalBuffer();

        Header.HeaderData.CommandPayloadLength = CalculateCommandPayloadLength();
    }


};


struct MidiNetworkOutOfBandOutgoingCommandPacket : public MidiNetworkCommandPacket
{
public:
    MidiNetworkOutOfBandOutgoingCommandPacket() {}

    winrt::hstring DestinationHostName;
    winrt::hstring DestinationPort;

};

struct MidiNetworkOutOfBandIncomingCommandPacket : public MidiNetworkCommandPacket
{
public:
    MidiNetworkOutOfBandIncomingCommandPacket() {}

    winrt::hstring SourceHostName;
    winrt::hstring SourcePort;

};