// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#pragma once



class CMidi2DiagnosticsEndpointManager :
    public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        IMidiEndpointManager>

{
public:
    STDMETHOD(Initialize(_In_ IUnknown*, _In_ IUnknown*));
    //STDMETHOD(UpdateConfiguration(_In_ LPCWSTR configurationJson));
    STDMETHOD(Cleanup)();

private:
    GUID m_ContainerId{};
    GUID m_TransportAbstractionId{ __uuidof(Midi2DiagnosticsAbstraction) };

    HRESULT CreateLoopbackEndpoint(
        _In_ std::wstring const InstanceId,
        _In_ std::wstring const UniqueId,
        _In_ std::wstring const Name,
        _In_ MidiFlow const Flow);

    HRESULT CreatePingEndpoint(
        _In_ std::wstring const InstanceId,
        _In_ std::wstring const UniqueId,
        _In_ std::wstring const Name,
        _In_ MidiFlow const Flow);

    HRESULT CreateParentDevice();

    wil::com_ptr_nothrow<IMidiDeviceManagerInterface> m_MidiDeviceManager;

    std::wstring m_parentDeviceId{};

};
