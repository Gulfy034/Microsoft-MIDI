// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once

class CMidi2NetworkMidiEndpointManager :
    public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        IMidiEndpointManager>

{
public:
    STDMETHOD(Initialize(_In_ IUnknown*, _In_ IUnknown*));
    STDMETHOD(Cleanup)();

private:
    GUID m_containerId{};
    GUID m_transportAbstractionId{};
    std::wstring m_parentDeviceId{};

    HRESULT CreateParentDevice();
    HRESULT CreateEndpoint(_In_ MidiNetworkDeviceDefinition& deviceEndpoint);


    wil::com_ptr_nothrow<IMidiDeviceManagerInterface> m_MidiDeviceManager;
    wil::com_ptr_nothrow<IMidiEndpointProtocolManagerInterface> m_MidiProtocolManager;
};
