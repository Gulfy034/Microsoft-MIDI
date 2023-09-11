// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#include "pch.h"
#include "MidiGroupEndpointListener.h"
#include "MidiGroupEndpointListener.g.cpp"


namespace winrt::Windows::Devices::Midi2::implementation
{
    _Use_decl_annotations_
    foundation::Collections::IVector<midi2::MidiGroup> MidiGroupEndpointListener::IncludeGroups()
    {
        throw hresult_not_implemented();
    }

    _Use_decl_annotations_
    void MidiGroupEndpointListener::Initialize()
    {
        throw hresult_not_implemented();
    }

    _Use_decl_annotations_
    void MidiGroupEndpointListener::OnEndpointConnectionOpened()
    {
        throw hresult_not_implemented();
    }

    _Use_decl_annotations_
    void MidiGroupEndpointListener::Cleanup()
    {
        throw hresult_not_implemented();
    }

    _Use_decl_annotations_
    void MidiGroupEndpointListener::ProcessIncomingMessage(
        midi2::MidiMessageReceivedEventArgs const& /*args*/,
        bool& skipFurtherListeners, 
        bool& skipMainMessageReceivedEvent)
    {

        skipFurtherListeners = false;
        skipMainMessageReceivedEvent = false;

        throw hresult_not_implemented();
    }
}
