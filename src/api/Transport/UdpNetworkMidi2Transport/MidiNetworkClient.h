// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once

struct MidiNetworkClientDefinition
{
    winrt::hstring EntryIdentifier;         // internal 
    bool Enabled{ true };

    // protocol
//    MidiNetworkHostProtocol NetworkProtocol{ MidiNetworkHostProtocol::ProtocolDefault };


};



class MidiNetworkClient
{
public:
    HRESULT Initialize(_In_ MidiNetworkClientDefinition& clientDefinition);

    HRESULT Shutdown();

private:



};