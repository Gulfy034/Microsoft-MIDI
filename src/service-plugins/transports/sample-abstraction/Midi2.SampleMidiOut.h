// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once

class CMidi2SampleMidiOut : 
    public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        IMidiOut>
{
public:

    STDMETHOD(Initialize(_In_ LPCWSTR, _In_ PABSTRACTIONCREATIONPARAMS, _In_ DWORD *, _In_ GUID));
    STDMETHOD(SendMidiMessage(_In_ PVOID, _In_ UINT, LONGLONG));
    STDMETHOD(Cleanup)();

private:
};


