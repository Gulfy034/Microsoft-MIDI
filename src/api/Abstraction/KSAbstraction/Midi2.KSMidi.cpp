// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#include "pch.h"
#include "midi2.ksabstraction.h"



_Use_decl_annotations_
HRESULT
CMidi2KSMidi::Initialize(
    LPCWSTR Device,
    MidiFlow Flow,
    PABSTRACTIONCREATIONPARAMS CreationParams,
    DWORD * MmCssTaskId,
    IMidiCallback * Callback,
    LONGLONG Context
)
{
    RETURN_HR_IF(E_INVALIDARG, Flow == MidiFlowIn && nullptr == Callback);
    RETURN_HR_IF(E_INVALIDARG, Flow == MidiFlowBidirectional && nullptr == Callback);
    RETURN_HR_IF(E_INVALIDARG, nullptr == Device);
    RETURN_HR_IF(E_INVALIDARG, nullptr == MmCssTaskId);
    RETURN_HR_IF(E_INVALIDARG, nullptr == CreationParams);

    TraceLoggingWrite(
        MidiKSAbstractionTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(Device, "Device"),
        TraceLoggingHexUInt32(CreationParams->DataFormat, "MidiDataFormat"),
        TraceLoggingHexUInt32(Flow, "MidiFlow"),
        TraceLoggingHexUInt32(*MmCssTaskId, "MmCssTaskId"),
        TraceLoggingPointer(Callback, "callback")
        );

    wil::unique_handle filter;
    std::unique_ptr<KSMidiInDevice> midiInDevice;
    std::unique_ptr<KSMidiOutDevice> midiOutDevice;
    winrt::guid interfaceClass;

    ULONG inPinId{ 0 };
    ULONG outPinId{ 0 };
    MidiTransport transportCapabilities;
    MidiTransport transport;

    std::wstring filterInterfaceId;
    auto additionalProperties = winrt::single_threaded_vector<winrt::hstring>();

    additionalProperties.Append(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_KsFilterInterfaceId));
    additionalProperties.Append(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_KsPinId));
    additionalProperties.Append(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_InPinId));
    additionalProperties.Append(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_OutPinId));
    additionalProperties.Append(winrt::to_hstring(STRING_DEVPKEY_KsTransport));
    additionalProperties.Append(winrt::to_hstring(L"System.Devices.InterfaceClassGuid"));

    auto deviceInfo = DeviceInformation::CreateFromIdAsync(Device, additionalProperties, winrt::Windows::Devices::Enumeration::DeviceInformationKind::DeviceInterface).get();

    auto prop = deviceInfo.Properties().Lookup(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_KsFilterInterfaceId));
    RETURN_HR_IF_NULL(E_INVALIDARG, prop);
    filterInterfaceId = winrt::unbox_value<winrt::hstring>(prop).c_str();

    prop = deviceInfo.Properties().Lookup(winrt::to_hstring(L"System.Devices.InterfaceClassGuid"));
    RETURN_HR_IF_NULL(E_INVALIDARG, prop);
    interfaceClass = winrt::unbox_value<winrt::guid>(prop);

    prop = deviceInfo.Properties().Lookup(winrt::to_hstring(STRING_DEVPKEY_KsTransport));
    RETURN_HR_IF_NULL(E_INVALIDARG, prop);
    transportCapabilities = (MidiTransport) winrt::unbox_value<uint32_t>(prop);

    // if creation params specifies MIDI_DATAFORMAT_ANY, the best available transport
    // will be chosen.

    // if creation params specifies that bytestream is requested, then
    // limit the transport to bytestream
    if (CreationParams->DataFormat == MidiDataFormat_ByteStream)
    {
        transportCapabilities = (MidiTransport) ((DWORD) transportCapabilities & (DWORD) MidiTransport_StandardAndCyclicByteStream);
    }
    // if creation params specifies that UMP is requested, then
    // limit the transport to UMP
    else if (CreationParams->DataFormat == MidiDataFormat_UMP)
    {
        transportCapabilities = (MidiTransport) ((DWORD) transportCapabilities & (DWORD) MidiTransport_CyclicUMP);
    }

    // choose the best available transport among the available transports.
    // fill in the MidiDataFormat that is going to be used for the caller.
    if (0 != ((DWORD) transportCapabilities & (DWORD) MidiTransport_CyclicUMP))
    {
        // Cyclic UMP is available, that's the most preferred transport.
        transport = MidiTransport_CyclicUMP;
        CreationParams->DataFormat = MidiDataFormat_UMP;
    }
    else if (0 != ((DWORD) transportCapabilities & (DWORD) MidiTransport_CyclicByteStream))
    {
        // Cyclic bytestream is available, that's the next preferred transport.
        transport = MidiTransport_CyclicByteStream;
        CreationParams->DataFormat = MidiDataFormat_ByteStream;
    }
    else if (0 != ((DWORD) transportCapabilities & (DWORD) MidiTransport_StandardByteStream))
    {
        // Standard bytestream is available, that's the least transport.
        transport = MidiTransport_StandardByteStream;
        CreationParams->DataFormat = MidiDataFormat_ByteStream;
    }
    else
    {
        // invalid/no transport available, error.
        RETURN_IF_FAILED(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
    }


    ULONG requestedBufferSize = PAGE_SIZE;
    RETURN_IF_FAILED(GetRequiredBufferSize(requestedBufferSize));
    RETURN_IF_FAILED(FilterInstantiate(filterInterfaceId.c_str(), &filter));


    if (Flow == MidiFlowBidirectional)
    {
        prop = deviceInfo.Properties().Lookup(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_InPinId));
        RETURN_HR_IF_NULL(E_INVALIDARG, prop);
        inPinId = outPinId = winrt::unbox_value<uint32_t>(prop);

        prop = deviceInfo.Properties().Lookup(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_OutPinId));
        RETURN_HR_IF_NULL(E_INVALIDARG, prop);
        outPinId = outPinId = winrt::unbox_value<uint32_t>(prop);
    }
    else
    {
        prop = deviceInfo.Properties().Lookup(winrt::to_hstring(STRING_DEVPKEY_KsMidiPort_KsPinId));
        RETURN_HR_IF_NULL(E_INVALIDARG, prop);
        inPinId = outPinId = winrt::unbox_value<uint32_t>(prop);
    }

    if (Flow == MidiFlowIn || Flow == MidiFlowBidirectional)
    {
        midiInDevice.reset(new (std::nothrow) KSMidiInDevice());
        RETURN_IF_NULL_ALLOC(midiInDevice);
        RETURN_IF_FAILED(midiInDevice->Initialize(Device, filter.get(), inPinId, transport, requestedBufferSize, MmCssTaskId, Callback, Context));
        m_MidiInDevice = std::move(midiInDevice);
    }

    if (Flow == MidiFlowOut || Flow == MidiFlowBidirectional)
    {
        midiOutDevice.reset(new (std::nothrow) KSMidiOutDevice());
        RETURN_IF_NULL_ALLOC(midiOutDevice);
        RETURN_IF_FAILED(midiOutDevice->Initialize(Device, filter.get(), outPinId, transport, requestedBufferSize, MmCssTaskId));
        m_MidiOutDevice = std::move(midiOutDevice);
    }

    return S_OK;
}

HRESULT
CMidi2KSMidi::Cleanup()
{
    TraceLoggingWrite(
        MidiKSAbstractionTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
        );

    if (m_MidiOutDevice)
    {
        m_MidiOutDevice->Cleanup();
        m_MidiOutDevice.reset();
    }
    if (m_MidiInDevice)
    {
        m_MidiInDevice->Cleanup();
        m_MidiInDevice.reset();
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT
CMidi2KSMidi::SendMidiMessage(
    PVOID Data,
    UINT Length,
    LONGLONG Position
)
{
    if (m_MidiOutDevice)
    {
        return m_MidiOutDevice->SendMidiMessage(Data, Length, Position);
    }

    return E_ABORT;
}

