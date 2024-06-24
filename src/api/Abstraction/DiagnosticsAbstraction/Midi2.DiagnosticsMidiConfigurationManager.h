// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#pragma once


class CMidi2DiagnosticsMidiConfigurationManager :
    public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IMidiAbstractionConfigurationManager>

{
public:
    STDMETHOD(Initialize(_In_ GUID AbstractionId, _In_ IUnknown* MidiDeviceManager, _In_ IUnknown* MidiServiceConfigurationManagerInterface));
    STDMETHOD(UpdateConfiguration(_In_ LPCWSTR ConfigurationJsonSection, _In_ BOOL IsFromConfigurationFile, _Out_ BSTR* Response));
    STDMETHOD(Cleanup)();

private:
    wil::com_ptr_nothrow<IMidiDeviceManagerInterface> m_MidiDeviceManager;

    //GUID m_abstractionId;   // kept for convenience
};
