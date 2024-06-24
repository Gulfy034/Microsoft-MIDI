// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================


#include "pch.h"

_Use_decl_annotations_
HRESULT
CMidi2VirtualPatchBayAbstraction::Activate(
    REFIID Riid,
    void **Interface
)
{
    OutputDebugString(L"" __FUNCTION__ " Enter");

    RETURN_HR_IF(E_INVALIDARG, nullptr == Interface);

    if (__uuidof(IMidiBiDi) == Riid)
    {
       OutputDebugString(L"" __FUNCTION__ " Activating IMidiBiDi");

        TraceLoggingWrite(
            MidiVirtualPatchBayAbstractionTelemetryProvider::Provider(),
            __FUNCTION__ "- IMidiBiDi",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingValue(__FUNCTION__),
            TraceLoggingPointer(this, "this")
            );

        wil::com_ptr_nothrow<IMidiBiDi> midiBiDi;
        RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CMidi2VirtualPatchBayBiDi>(&midiBiDi));
        *Interface = midiBiDi.detach();
    }


    else if (__uuidof(IMidiEndpointManager) == Riid)
    {
        TraceLoggingWrite(
            MidiVirtualPatchBayAbstractionTelemetryProvider::Provider(),
            __FUNCTION__ "- IMidiEndpointManager",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingValue(__FUNCTION__),
            TraceLoggingPointer(this, "this")
        );

        // check to see if this is the first time we're creating the endpoint manager. If so, create it.
        if (m_EndpointManager == nullptr)
        {
            RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CMidi2VirtualPatchBayEndpointManager>(&m_EndpointManager));
        }

        // TODO: Not sure if this is the right pattern for this or not. There's no detach call here, so does this leak?
        RETURN_IF_FAILED(m_EndpointManager->QueryInterface(Riid, Interface));
    }

    else if (__uuidof(IMidiAbstractionConfigurationManager) == Riid)
    {
        //TraceLoggingWrite(
        //    MidiVirtualPatchBayAbstractionTelemetryProvider::Provider(),
        //    __FUNCTION__ "- IMidiEndpointManager",
        //    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        //    TraceLoggingValue(__FUNCTION__),
        //    TraceLoggingPointer(this, "this")
        //);

        //// check to see if this is the first time we're creating the endpoint manager. If so, create it.
        //if (m_EndpointManager == nullptr)
        //{
        //    RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CMidi2VirtualPatchBayEndpointManager>(&m_EndpointManager));
        //}

        //// TODO: Not sure if this is the right pattern for this or not. There's no detach call here, so does this leak?
        //RETURN_IF_FAILED(m_EndpointManager->QueryInterface(Riid, Interface));
    }

    else if (__uuidof(IMidiServiceAbstractionPluginMetadataProvider) == Riid)
    {
        TraceLoggingWrite(
            MidiVirtualPatchBayAbstractionTelemetryProvider::Provider(),
            __FUNCTION__ "- IMidiServiceAbstractionPluginMetadataProvider",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingValue(__FUNCTION__),
            TraceLoggingPointer(this, "this")
        );

        wil::com_ptr_nothrow<IMidiServiceAbstractionPluginMetadataProvider> metadataProvider;
        RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CMidi2VirtualPatchBayPluginMetadataProvider>(&metadataProvider));
        *Interface = metadataProvider.detach();
    }


    else
    {
        OutputDebugString(L"" __FUNCTION__ " Returning E_NOINTERFACE. Was an interface added that isn't handled in the Abstraction?");

        return E_NOINTERFACE;
    }

    return S_OK;
}

