// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#include "stdafx.h"

using namespace winrt::Windows::Devices::Enumeration;

const WCHAR driver32Path[]    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32";
const WCHAR midisrvTransferComplete[] =  L"MidisrvTransferComplete";

const WCHAR szzMidiDeviceHardwareId[] =  L"MIDISRV\\MidiEndpoints" L"\0";
const WCHAR szzMidiDeviceCompatibleId[] =  L"GenericMidiEndpoint" L"\0";

// a cap to ensure we don't have runaway port numbers
#define MAX_WINMM_PORT_NUMBER 5000

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::Initialize(
    std::shared_ptr<CMidiPerformanceManager>& performanceManager, 
    std::shared_ptr<CMidiEndpointProtocolManager>& endpointProtocolManager,
    std::shared_ptr<CMidiConfigurationManager>& configurationManager)
{
    DWORD transferState = 0;
    DWORD dataSize = sizeof(DWORD);

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
    );

    m_configurationManager = configurationManager;

    // query for midisrv control keys
    if (ERROR_SUCCESS == RegGetValue(HKEY_LOCAL_MACHINE, driver32Path, midisrvTransferComplete, RRF_RT_DWORD, NULL, &transferState, &dataSize) && 
        transferState != 1)
    {
        // This assumes that AudioEndpointBuilder state is cycled after MidiSrv is installed, before MidiSrv starts.
        // Should we instead coordinate endpoint builder and midisrv via named events or WNF, so endpoint builder is aware when
        // midisrv starts for the first time, and midisrv is notified once endpoint builder completes the transition of control?

        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"AudioEndpointBuilder retains control of Midi port registration, unable to take exclusive control of midi ports.", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );
    }

    //wil::com_ptr_nothrow<IMidiServiceConfigurationManagerInterface> serviceConfigManagerInterface;
    //RETURN_IF_FAILED(configurationManager->QueryInterface(__uuidof(IMidiServiceConfigurationManagerInterface), (void**)&serviceConfigManagerInterface));

    // Get the enabled transport layers from the registry

    for (auto const& TransportLayer : m_configurationManager->GetEnabledTransports())
    {
        wil::com_ptr_nothrow<IMidiTransport> midiTransport;
        wil::com_ptr_nothrow<IMidiEndpointManager> endpointManager;
        wil::com_ptr_nothrow<IMidiTransportConfigurationManager> transportConfigurationManager;

        try
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Getting transport configuration JSON", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );

            // provide the initial settings for these transports
            auto transportSettingsJson = m_configurationManager->GetSavedConfigurationForTransport(TransportLayer);

            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"CoCreating transport layer", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );

            // changed these from a return-on-fail to just log, so we don't prevent service startup
            // due to one bad transport

            LOG_IF_FAILED(CoCreateInstance(TransportLayer, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&midiTransport)));

            if (midiTransport != nullptr)
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating transport configuration manager.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingGuid(TransportLayer, "transport layer")
                );

                // we only log. This interface is optional
                // If present, an transport device manager may make use of its configuration manager, so activate
                // and initialize the configuration manager before activating the device manager so that configuration
                // is present.
                auto configHR = midiTransport->Activate(__uuidof(IMidiTransportConfigurationManager), (void**)&transportConfigurationManager);
                if (SUCCEEDED(configHR))
                {
                    if (transportConfigurationManager != nullptr)
                    {
                        // init the transport's config manager
                        auto initializeResult = transportConfigurationManager->Initialize(
                            TransportLayer,
                            this, 
                            m_configurationManager.get());

                        if (FAILED(initializeResult))
                        {
                            LOG_IF_FAILED(initializeResult);

                            TraceLoggingWrite(
                                MidiSrvTelemetryProvider::Provider(),
                                MIDI_TRACE_EVENT_ERROR,
                                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                                TraceLoggingPointer(this, "this"),
                                TraceLoggingWideString(L"Failed to initialize transport configuration manager.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                TraceLoggingHResult(initializeResult, MIDI_TRACE_EVENT_HRESULT_FIELD),
                                TraceLoggingGuid(TransportLayer, "transport layer")
                            );
                        }
                        else
                        {
                            // don't std::move this because we sill need it
                            m_midiTransportConfigurationManagers[TransportLayer] = transportConfigurationManager;

                            if (!transportSettingsJson.empty())
                            {
                                TraceLoggingWrite(
                                    MidiSrvTelemetryProvider::Provider(),
                                    MIDI_TRACE_EVENT_INFO,
                                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                                    TraceLoggingPointer(this, "this"),
                                    TraceLoggingWideString(L"Updating transport configuration", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                    TraceLoggingGuid(TransportLayer, "transport layer")
                                );

                                LPWSTR response{};

                                auto updateConfigHR = transportConfigurationManager->UpdateConfiguration(transportSettingsJson.c_str(), &response);

                                if (FAILED(updateConfigHR))
                                {
                                    if (updateConfigHR == E_NOTIMPL)
                                    {
                                        TraceLoggingWrite(
                                            MidiSrvTelemetryProvider::Provider(),
                                            MIDI_TRACE_EVENT_WARNING,
                                            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
                                            TraceLoggingPointer(this, "this"),
                                            TraceLoggingWideString(L"Config update not implemented for this transport.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                            TraceLoggingHResult(updateConfigHR, MIDI_TRACE_EVENT_HRESULT_FIELD),
                                            TraceLoggingGuid(TransportLayer, "transport layer")
                                        );
                                    }
                                    else
                                    {
                                        LOG_IF_FAILED(updateConfigHR);

                                        TraceLoggingWrite(
                                            MidiSrvTelemetryProvider::Provider(),
                                            MIDI_TRACE_EVENT_ERROR,
                                            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                                            TraceLoggingPointer(this, "this"),
                                            TraceLoggingWideString(L"Failed to update configuration.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                            TraceLoggingHResult(updateConfigHR, MIDI_TRACE_EVENT_HRESULT_FIELD),
                                            TraceLoggingGuid(TransportLayer, "transport layer")
                                        );
                                    }
                                }
                                else
                                {

                                    TraceLoggingWrite(
                                        MidiSrvTelemetryProvider::Provider(),
                                        MIDI_TRACE_EVENT_INFO,
                                        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                                        TraceLoggingPointer(this, "this"),
                                        TraceLoggingWideString(L"Configuration updated.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                        TraceLoggingGuid(TransportLayer, "transport layer")
                                    );
                                }

                            }


                        }
                    }
                    else
                    {
                        TraceLoggingWrite(
                            MidiSrvTelemetryProvider::Provider(),
                            MIDI_TRACE_EVENT_ERROR,
                            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                            TraceLoggingPointer(this, "this"),
                            TraceLoggingWideString(L"Transport transport configuration manager activation failed with null result.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                            TraceLoggingGuid(TransportLayer, "transport layer")
                        );
                    }
                }
                else
                {
                    TraceLoggingWrite(
                        MidiSrvTelemetryProvider::Provider(),
                        MIDI_TRACE_EVENT_ERROR,
                        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingPointer(this, "this"),
                        TraceLoggingHResult(configHR, MIDI_TRACE_EVENT_HRESULT_FIELD),
                        TraceLoggingWideString(L"Transport transport configuration manager activation failed.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                        TraceLoggingGuid(TransportLayer, "transport layer")
                    );
                }

                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating transport layer IMidiEndpointManager", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingGuid(TransportLayer, "transport layer")
                );

                LOG_IF_FAILED(midiTransport->Activate(__uuidof(IMidiEndpointManager), (void**)&endpointManager));

                if (endpointManager != nullptr)
                {
                    wil::com_ptr_nothrow<IMidiEndpointProtocolManagerInterface> protocolManager = endpointProtocolManager.get();

                    TraceLoggingWrite(
                        MidiSrvTelemetryProvider::Provider(),
                        MIDI_TRACE_EVENT_INFO,
                        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                        TraceLoggingPointer(this, "this"),
                        TraceLoggingWideString(L"Initializing transport layer Midi Endpoint Manager", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                        TraceLoggingGuid(TransportLayer, "transport layer")
                    );

                    auto initializeResult = endpointManager->Initialize(this, protocolManager.get());

                    if (SUCCEEDED(initializeResult))
                    {
                        m_midiEndpointManagers.emplace(TransportLayer, std::move(endpointManager));

                        TraceLoggingWrite(
                            MidiSrvTelemetryProvider::Provider(),
                            MIDI_TRACE_EVENT_INFO,
                            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                            TraceLoggingPointer(this, "this"),
                            TraceLoggingWideString(L"Midi Endpoint Manager initialized", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                            TraceLoggingGuid(TransportLayer, "transport layer")
                        );
                    }
                    else
                    {
                        TraceLoggingWrite(
                            MidiSrvTelemetryProvider::Provider(),
                            MIDI_TRACE_EVENT_ERROR,
                            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                            TraceLoggingPointer(this, "this"),
                            TraceLoggingWideString(L"Transport transport initialization failed.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                            TraceLoggingGuid(TransportLayer, "transport layer")
                        );
                    }

                }
                else
                {
                    TraceLoggingWrite(
                        MidiSrvTelemetryProvider::Provider(),
                        MIDI_TRACE_EVENT_ERROR,
                        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingPointer(this, "this"),
                        TraceLoggingWideString(L"Transport transport endpoint manager initialization failed.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                        TraceLoggingGuid(TransportLayer, "transport layer")
                    );

                }
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Transport Transport activation failed (nullptr return).", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingGuid(TransportLayer, "transport layer")
                );
            }

        }

        catch (wil::ResultException rex)
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Result Exception loading transport transport.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingString(rex.what(), "error"),
                TraceLoggingHResult(rex.GetErrorCode(), MIDI_TRACE_EVENT_HRESULT_FIELD),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );

        }
        catch (std::runtime_error& err)
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Runtime error loading transport transport.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingString(err.what(), "error"),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );
        }
        catch (const std::exception& ex)
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Exception loading transport transport.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingString(ex.what(), "exception"),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );
        }        
        catch (...)
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Unknown exception loading transport transport.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingGuid(TransportLayer, "transport layer")
            );

        }
    }

    m_performanceManager = performanceManager;

    return S_OK;
}

typedef struct _CREATECONTEXT
{
    PMIDIPORT MidiPort{ nullptr };
    wil::unique_event CreationCompleted{wil::EventOptions::None};
    DEVPROPERTY* InterfaceDevProperties{ nullptr };
    ULONG IntPropertyCount{ 0 };
} CREATECONTEXT, * PCREATECONTEXT;

typedef struct _PARENTDEVICECREATECONTEXT
{
    PMIDIPARENTDEVICE MidiParentDevice{ nullptr };
    wil::unique_event CreationCompleted{ wil::EventOptions::None };
} PARENTDEVICECREATECONTEXT, * PPARENTDEVICECREATECONTEXT;



void SwMidiPortCreateCallback(__in HSWDEVICE swDevice, __in HRESULT creationResult, __in PVOID context, __in_opt PCWSTR /* deviceInstanceId */)
{
    // fix code analysis complaint
    if (context == nullptr) return;

    PCREATECONTEXT creationContext = (PCREATECONTEXT)context;

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(nullptr, "this"),
        TraceLoggingWideString(L"Enter", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(creationContext->MidiPort->InstanceId.c_str(), "instance id"),
        TraceLoggingULong(creationContext->IntPropertyCount, "property count")
    );


    // interface registration has started, assume
    // failure
    creationContext->MidiPort->SwDeviceState = SWDEVICESTATE::Failed;
    creationContext->MidiPort->hr = creationResult;

    if (SUCCEEDED(creationContext->MidiPort->hr))
    {
        creationContext->MidiPort->hr = SwDeviceInterfaceRegister(
            swDevice,
            creationContext->MidiPort->InterfaceCategory,
            nullptr,
            creationContext->IntPropertyCount,
            creationContext->InterfaceDevProperties,
            TRUE,
            wil::out_param(creationContext->MidiPort->DeviceInterfaceId));
    }

    if (SUCCEEDED(creationContext->MidiPort->hr))
    {
        // success, mark the port as created
        creationContext->MidiPort->SwDeviceState = SWDEVICESTATE::Created;

        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(nullptr, "this"),
            TraceLoggingWideString(L"Creation succeeded", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(creationContext->MidiPort->InstanceId.c_str(), "instance id"),
            TraceLoggingWideString(creationContext->MidiPort->DeviceInterfaceId.get(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD)
        );
    }

    // success or failure, signal we have completed.
    creationContext->CreationCompleted.SetEvent();
}


void
SwMidiParentDeviceCreateCallback(
    __in HSWDEVICE /*swDevice*/,
    __in HRESULT creationResult,
    __in_opt PVOID context,
    __in_opt PCWSTR deviceInstanceId
)
{
    if (context == nullptr)
    {
        // TODO: Should log this.

        return;
    }

    PPARENTDEVICECREATECONTEXT creationContext = (PPARENTDEVICECREATECONTEXT)context;

    // interface registration has started, assume failure
    creationContext->MidiParentDevice->SwDeviceState = SWDEVICESTATE::Failed;
    creationContext->MidiParentDevice->hr = creationResult;

    LOG_IF_FAILED(creationResult);

    if (SUCCEEDED(creationResult))
    {
        // success, mark the port as created
        creationContext->MidiParentDevice->SwDeviceState = SWDEVICESTATE::Created;

        // get the new device instance ID. This is usually modified from what we started with
        creationContext->MidiParentDevice->InstanceId = std::wstring(deviceInstanceId);
    }

    // success or failure, signal we have completed.
    creationContext->CreationCompleted.SetEvent();
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::ActivateVirtualParentDevice
(
    ULONG devicePropertyCount,
    const DEVPROPERTY* deviceProperties,
    const SW_DEVICE_CREATE_INFO* createInfo,
    LPWSTR* createdSwDeviceId
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
    );


    RETURN_HR_IF_NULL(E_INVALIDARG, createInfo);
    RETURN_HR_IF_NULL(E_INVALIDARG, createdSwDeviceId);

    auto pcreateinfo = (SW_DEVICE_CREATE_INFO *)createInfo;

    std::unique_ptr<MIDIPARENTDEVICE> midiParent = std::make_unique<MIDIPARENTDEVICE>();
    RETURN_IF_NULL_ALLOC(midiParent);

    midiParent->SwDeviceState = SWDEVICESTATE::CreatePending;

    PARENTDEVICECREATECONTEXT creationContext;
    creationContext.MidiParentDevice = midiParent.get();

    std::vector<DEVPROPERTY> devProperties{};

    DEVPROP_BOOLEAN devPropTrue = DEVPROP_TRUE;

    if (devicePropertyCount > 0 && deviceProperties != nullptr)
    {
        for (ULONG i = 0; i < devicePropertyCount; i++)
        {
            devProperties.push_back(((DEVPROPERTY*)deviceProperties)[i]);
        }
    }

    devProperties.push_back({ {DEVPKEY_Device_PresenceNotForDevice, DEVPROP_STORE_SYSTEM, nullptr},
            DEVPROP_TYPE_BOOLEAN, static_cast<ULONG>(sizeof(devPropTrue)), &devPropTrue });

    devProperties.push_back({ {DEVPKEY_Device_NoConnectSound, DEVPROP_STORE_SYSTEM, nullptr},
            DEVPROP_TYPE_BOOLEAN, static_cast<ULONG>(sizeof(devPropTrue)),&devPropTrue });


    RETURN_IF_FAILED(SwDeviceCreate(
        MIDI_DEVICE_ENUMERATOR,                 
        MIDI_SWD_VIRTUAL_PARENT_ROOT,           // root device
        pcreateinfo,
        (ULONG)devProperties.size(),            // count of properties
        (DEVPROPERTY*)devProperties.data(),     // pointer to properties
        SwMidiParentDeviceCreateCallback,       // callback
        &creationContext,
        wil::out_param(creationContext.MidiParentDevice->SwDevice)));

    // wait 5 seconds for creation to complete
    creationContext.CreationCompleted.wait(5000);

    // confirm we were able to register the interface
    RETURN_IF_FAILED(creationContext.MidiParentDevice->hr);
    RETURN_HR_IF(E_FAIL, creationContext.MidiParentDevice->SwDeviceState != SWDEVICESTATE::Created);

    wil::unique_cotaskmem_string newDeviceId;
    newDeviceId = wil::make_cotaskmem_string_nothrow(creationContext.MidiParentDevice->InstanceId.c_str());
    RETURN_IF_NULL_ALLOC(newDeviceId.get());
    *createdSwDeviceId = newDeviceId.release();

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(*createdSwDeviceId)
    );

    if (SUCCEEDED(creationContext.MidiParentDevice->hr))
    {
        m_midiParents.push_back(std::move(midiParent));
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::ActivateEndpoint
(
    PCWSTR parentInstanceId,
    BOOL umpOnly,
    MidiFlow flow,
    PMIDIENDPOINTCOMMONPROPERTIES commonProperties,
    ULONG intPropertyCount,
    ULONG devPropertyCount,
    const DEVPROPERTY* interfaceDevProperties,
    const DEVPROPERTY* deviceDevProperties,
    const SW_DEVICE_CREATE_INFO* createInfo,
    LPWSTR *createdDeviceInterfaceId
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Enter", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(parentInstanceId, "parent"),
        TraceLoggingBool(commonProperties != nullptr, "Common Properties Provided"),
        TraceLoggingULong(intPropertyCount, "Interface Property Count"),
        TraceLoggingULong(devPropertyCount, "Device Property Count"),
        TraceLoggingBool(umpOnly, "UMP Only")
    );

    auto lock = m_midiPortsLock.lock();

    const bool alreadyActivated = std::find_if(m_midiPorts.begin(), m_midiPorts.end(), [&](const std::unique_ptr<MIDIPORT>& Port)
    {
        // if this instance id already activated, then we cannot activate/create a second time,
        if (((SW_DEVICE_CREATE_INFO*)createInfo)->pszInstanceId == Port->InstanceId)
        {
            return true;
        }

        return false;
    }) != m_midiPorts.end();

    if (alreadyActivated)
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Already Activated", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent"),
            TraceLoggingWideString(((SW_DEVICE_CREATE_INFO*)createInfo)->pszInstanceId, "port instance")
            );

        return S_FALSE;
    }
    else
    {
        std::vector<DEVPROPERTY> allInterfaceProperties{};

        if (intPropertyCount > 0 && interfaceDevProperties != nullptr)
        {
            // copy the incoming array into a vector so that we can add additional items.
            std::vector<DEVPROPERTY> suppliedInterfaceProperties((DEVPROPERTY*)interfaceDevProperties, (DEVPROPERTY*)(interfaceDevProperties) + intPropertyCount);

            // copy to the main vector that we're going to keep using.
            allInterfaceProperties = suppliedInterfaceProperties;


            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingULong((ULONG)intPropertyCount, "provided interface props count"),
                TraceLoggingULong((ULONG)allInterfaceProperties.size(), "copied interface props count")
            );
        }

        DEVPROP_BOOLEAN devPropTrue = DEVPROP_TRUE;
        DEVPROP_BOOLEAN devPropFalse = DEVPROP_FALSE;

        // Build common properties so these are consistent from endpoint to endpoint
        if (commonProperties != nullptr)
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingWideString(L"Common properties object supplied", MIDI_TRACE_EVENT_MESSAGE_FIELD)
            );


            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_TransportLayer, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_GUID, (ULONG)(sizeof(GUID)), (PVOID)(&(commonProperties->TransportId)) });

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointDevicePurpose, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(commonProperties->EndpointDeviceType)) });
            
            if (commonProperties->FriendlyName != nullptr && wcslen(commonProperties->FriendlyName) > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {DEVPKEY_DeviceInterface_FriendlyName, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->FriendlyName) + 1)), (PVOID)(commonProperties->FriendlyName) });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Common Properties: Friendly Name is null or empty", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingWideString(parentInstanceId, "parent")
                    );

                return E_INVALIDARG;
            }

            // transport information

            if (commonProperties->TransportCode != nullptr && wcslen(commonProperties->TransportCode) > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_TransportCode, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->TransportCode) + 1)), (PVOID)commonProperties->TransportCode });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Common Properties: Transport Code is null or empty", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                return E_INVALIDARG;
            }


            if (commonProperties->EndpointName != nullptr && wcslen(commonProperties->EndpointName) > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointName, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->EndpointName) + 1)), (PVOID)commonProperties->EndpointName });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_EndpointName", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );
                
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointName, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            if (commonProperties->EndpointDescription != nullptr && wcslen(commonProperties->EndpointDescription) > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_Description, DEVPROP_STORE_SYSTEM, nullptr},
                   DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->EndpointDescription) + 1)), (PVOID)commonProperties->EndpointDescription });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_Description", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_Description, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            if (commonProperties->UniqueIdentifier != nullptr && wcslen(commonProperties->UniqueIdentifier) > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SerialNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->UniqueIdentifier) + 1)), (PVOID)commonProperties->UniqueIdentifier });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_SerialNumber", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SerialNumber, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            if (commonProperties->ManufacturerName != nullptr && wcslen(commonProperties->ManufacturerName) != 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_ManufacturerName, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->ManufacturerName) + 1)), (PVOID)commonProperties->ManufacturerName });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_ManufacturerName", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_ManufacturerName, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            // user-supplied information, if available

            if (commonProperties->CustomEndpointName != nullptr && wcslen(commonProperties->CustomEndpointName) != 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomEndpointName, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->CustomEndpointName) + 1)), (PVOID)commonProperties->CustomEndpointName });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_CustomEndpointName", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomEndpointName, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            if (commonProperties->CustomEndpointDescription != nullptr && wcslen(commonProperties->CustomEndpointDescription) != 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomDescription, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(commonProperties->CustomEndpointDescription) + 1)), (PVOID)commonProperties->CustomEndpointDescription });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing PKEY_MIDI_CustomDescription", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomDescription, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingWideString(L"Adding other non-string properties", MIDI_TRACE_EVENT_MESSAGE_FIELD)
            );

            // data format. These each have different uses. Native format is the device's format. Supported
            // format is how we talk to it from the service (the driver may translate to UMP, for example)

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_NativeDataFormat, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_BYTE, (ULONG)(sizeof(BYTE)), (PVOID)(&(commonProperties->NativeDataFormat)) });

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SupportedDataFormats, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(commonProperties->SupportedDataFormats)) });

            // Protocol defaults for cases when protocol negotiation may not happen or may not complete
            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointSupportsMidi1Protocol, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_BOOLEAN, (ULONG)(sizeof(DEVPROP_BOOLEAN)), (PVOID)((commonProperties->Capabilities & MidiEndpointCapabilities_SupportsMidi1Protocol)? &devPropTrue : &devPropFalse) });

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointSupportsMidi2Protocol, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_BOOLEAN, (ULONG)(sizeof(DEVPROP_BOOLEAN)), (PVOID)((commonProperties->Capabilities & MidiEndpointCapabilities_SupportsMidi2Protocol) ? &devPropTrue : &devPropFalse) });

            // behavior

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_GenerateIncomingTimestamp, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_BOOLEAN, (ULONG)(sizeof(DEVPROP_BOOLEAN)), (PVOID)((commonProperties->Capabilities & MidiEndpointCapabilities_GenerateIncomingTimestamps) ? &devPropTrue : &devPropFalse) });

            allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SupportsMulticlient, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_BOOLEAN, (ULONG)(sizeof(DEVPROP_BOOLEAN)), (PVOID)((commonProperties->Capabilities & MidiEndpointCapabilities_SupportsMultiClient) ? &devPropTrue : &devPropFalse) });
        
            if (commonProperties->CustomEndpointPortNumber > 0)
            {
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(commonProperties->CustomEndpointPortNumber)) });
            }
            else
            {
                // this is needed to clear any cached property, or garbage value
                allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_EMPTY, 0, nullptr });
            }
        }
        else
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_WARNING,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingWideString(L"NO Common properties object supplied", MIDI_TRACE_EVENT_MESSAGE_FIELD)
            );

        }

        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(parentInstanceId, "parent"),
            TraceLoggingWideString(L"Adding clearing properties", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        // Clear almost all of the protocol negotiation-discovered properties. We keep the "supports MIDI 1.0" and "supports MIDI 2.0" 
        // defaulted based on the native transport type of the device as per above.

        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointProvidedName, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointProvidedNameLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointProvidedProductInstanceId, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointProvidedProductInstanceIdLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointConfiguredProtocol, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointConfiguredToReceiveJRTimestamps, DEVPROP_STORE_SYSTEM, nullptr},DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointConfiguredToSendJRTimestamps, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointConfigurationLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointSupportsReceivingJRTimestamps, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointSupportsSendingJRTimestamps, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointUmpVersionMajor, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointUmpVersionMinor, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_EndpointInformationLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_DeviceIdentity, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_DeviceIdentityLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_FunctionBlocksAreStatic, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_FunctionBlockCount, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        allInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_FunctionBlocksLastUpdateTime, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        // remove actual function blocks and their names

        //// TEMP
        //TraceLoggingWrite(
        //    MidiSrvTelemetryProvider::Provider(),
        //    __FUNCTION__,
        //    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        //    TraceLoggingPointer(this, "this"),
        //    TraceLoggingWideString(parentInstanceId, "parent"),
        //    TraceLoggingWideString(L"Adding clearing properties for function blocks", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        //);

        //for (int i = 0; i < MIDI_MAX_FUNCTION_BLOCKS; i++)
        //{
        //    // clear out the function block data
        //    DEVPROPKEY fbkey;
        //    fbkey.fmtid = PKEY_MIDI_FunctionBlock00.fmtid;    // hack
        //    fbkey.pid = MIDI_FUNCTION_BLOCK_PROPERTY_INDEX_START + i;

        //    allInterfaceProperties.push_back(DEVPROPERTY{ {fbkey, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });

        //    // clear out the function block name data
        //    DEVPROPKEY namekey;
        //    namekey.fmtid = PKEY_MIDI_FunctionBlockName00.fmtid;    // hack
        //    namekey.pid = MIDI_FUNCTION_BLOCK_NAME_PROPERTY_INDEX_START + i;

        //    allInterfaceProperties.push_back(DEVPROPERTY{ {namekey, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
        //}

        // TEMP
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Registering cleanupOnFailure", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent")
            );

        std::wstring deviceInterfaceId{ };

        auto cleanupOnFailure = wil::scope_exit([&]()
        {
            // in the event that creation fails, clean up all of the interfaces associated with this InstanceId
            DeactivateEndpoint(((SW_DEVICE_CREATE_INFO*)createInfo)->pszInstanceId);
        });


        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Activating UMP Endpoint", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent")
        );
        
        // Activate the UMP version of this endpoint.
        auto activationResult = ActivateEndpointInternal(
            parentInstanceId, 
            nullptr, 
            FALSE, 
            flow, 
            (ULONG)allInterfaceProperties.size(),
            devPropertyCount, 
            (DEVPROPERTY*)(allInterfaceProperties.data()),
            (DEVPROPERTY*)deviceDevProperties, 
            (SW_DEVICE_CREATE_INFO*)createInfo, 
            &deviceInterfaceId);

        if (FAILED(activationResult))
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Failed to activate UMP endpoint", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingHResult(activationResult, "Activation HRESULT")
            );

            RETURN_IF_FAILED(activationResult);
        }


        if (!umpOnly)
        {
            std::wstring friendlyName;
            std::vector<DEVPROPERTY> midi1OutInterfaceProperties{};
            std::vector<DEVPROPERTY> midi1InInterfaceProperties{};

            if (commonProperties->CustomEndpointName)
            {
                friendlyName = commonProperties->CustomEndpointName;
            }
            else
            {
                if (commonProperties->ManufacturerName)
                {
                    friendlyName += commonProperties->ManufacturerName;
                    friendlyName += L" ";
                }

                if (commonProperties->EndpointName)
                {
                    friendlyName += commonProperties->EndpointName;
                }

#define MAX_WINMM_ENDPOINT_NAME_SIZE_WITHOUT_GROUP_SUFFIX (32-6)

                if (friendlyName.size() > MAX_WINMM_ENDPOINT_NAME_SIZE_WITHOUT_GROUP_SUFFIX)
                {
                    // if the size of name + " O-nn\0" > 32, we need to lose the manufacturer name for WinMM compat

                    std::wstring shortName{ commonProperties->EndpointName };
                    shortName.resize(MAX_WINMM_ENDPOINT_NAME_SIZE_WITHOUT_GROUP_SUFFIX);

                    friendlyName = shortName.c_str();
                }

            }

            // O for Output, I for input, then the group number after that.
            // we have to economize space due to 32 character WinMM name limit
            // Group numbers should be 1-16 here, so index+1
            std::wstring midiOutFriendlyName = friendlyName + L" O-1";
            std::wstring midiInFriendlyName = friendlyName + L" I-1";


            midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {DEVPKEY_DeviceInterface_FriendlyName, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(midiOutFriendlyName.c_str()) + 1)), (PVOID)midiOutFriendlyName.c_str() });

            midi1InInterfaceProperties.push_back(DEVPROPERTY{ {DEVPKEY_DeviceInterface_FriendlyName, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_STRING, (ULONG)(sizeof(wchar_t) * (wcslen(midiInFriendlyName.c_str()) + 1)), (PVOID)midiInFriendlyName.c_str() });

            if (commonProperties->CustomEndpointPortNumber > 0 && commonProperties->CustomEndpointPortNumber <= MAX_WINMM_PORT_NUMBER)
            {
                midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(commonProperties->CustomEndpointPortNumber)) });

                midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(commonProperties->CustomEndpointPortNumber)) });
            }
            else
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(parentInstanceId),
                    TraceLoggingWideString(L"Clearing User supplied port number properties", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                // if you don't clear the values, the next time the same SWD is created, the old values will be pulled from cache

                midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_EMPTY, 0, nullptr });

                midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_CustomPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_EMPTY, 0, nullptr });
            }

            // The transport layer GUID is required for various tests for targeting the tests on this SWD.
            midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_TransportLayer, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_GUID, (ULONG)(sizeof(GUID)), (PVOID)(&(commonProperties->TransportId)) });

            midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_TransportLayer, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_GUID, (ULONG)(sizeof(GUID)), (PVOID)(&(commonProperties->TransportId)) });

            // The Midi 1 ports are an transport generated and handled by Midisrv, as such it can do both bytestream
            // and UMP.
            MidiDataFormats supportedDataFormats { MidiDataFormats_Any };

            midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SupportedDataFormats, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(supportedDataFormats)) });

            midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_SupportedDataFormats, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(supportedDataFormats)) });

            if (commonProperties->NativeDataFormat > 0)
            {
                // Native data format is used by various tests to determine which messages are valid to send to the device,
                // copy it over if we have it.
                midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_NativeDataFormat, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_BYTE, (ULONG)(sizeof(BYTE)), (PVOID)(&(commonProperties->NativeDataFormat)) });

                midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_NativeDataFormat, DEVPROP_STORE_SYSTEM, nullptr},
                    DEVPROP_TYPE_BYTE, (ULONG)(sizeof(BYTE)), (PVOID)(&(commonProperties->NativeDataFormat)) });
            }

            // TODO: temporary hard coded group index value
            // group index of 0 is group 1. Use group 1 so that messages sent through
            // the usbmidi2 driver to legacy peripherals will loop back correctly for now, until
            // the real group indexes are plumbed.
            UINT32 groupIndex = 0;

            midi1OutInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_PortAssignedGroupIndex, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(groupIndex)) });
            
            midi1InInterfaceProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_PortAssignedGroupIndex, DEVPROP_STORE_SYSTEM, nullptr},
                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(groupIndex)) });

            // now activate the midi 1 SWD's for this endpoint
            if (flow == MidiFlowBidirectional)
            {
                // This logic will need to change. We need one endpoint for each group, but we
                // don't know that until we get the function blocks. We know a little due to
                // the Group Terminal Blocks in the case of USB, but that's only for one transport
                // 
                // It may be easier to have the transport create the MIDI 1.0 SWDs by calling
                // functions when it's ready? That way it can get the info it needs first, and then
                // create the MIDI 1.0 interfaces. Need to think through that flow.

                // if this is a bidirectional endpoint, it gets an in and an out midi 1 swd's.

                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating MIDI 1.0 Output Endpoint for BiDi", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                RETURN_IF_FAILED(ActivateEndpointInternal(
                    parentInstanceId,
                    deviceInterfaceId.c_str(),
                    TRUE,
                    MidiFlowOut,
                    (ULONG)midi1OutInterfaceProperties.size(),
                    0,
                    (DEVPROPERTY*)(midi1OutInterfaceProperties.data()),
                    nullptr,
                    (SW_DEVICE_CREATE_INFO*)createInfo,
                    nullptr));



                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating MIDI 1.0 Input Endpoint for BiDi", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                RETURN_IF_FAILED(ActivateEndpointInternal(
                    parentInstanceId,
                    deviceInterfaceId.c_str(),
                    TRUE,
                    MidiFlowIn,
                    (ULONG)midi1InInterfaceProperties.size(),
                    0,
                    (DEVPROPERTY*)(midi1InInterfaceProperties.data()),
                    nullptr,
                    (SW_DEVICE_CREATE_INFO*)createInfo, 
                    nullptr));
            }
            else if (flow == MidiFlowOut)
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating MIDI 1.0 Output Endpoint for MidiFlowOut", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                RETURN_IF_FAILED(ActivateEndpointInternal(
                    parentInstanceId,
                    deviceInterfaceId.c_str(),
                    TRUE,
                    MidiFlowOut,
                    (ULONG)midi1OutInterfaceProperties.size(),
                    0,
                    (DEVPROPERTY*)(midi1OutInterfaceProperties.data()),
                    nullptr,
                    (SW_DEVICE_CREATE_INFO*)createInfo, 
                    nullptr));
            }
            else if (flow == MidiFlowIn)
            {
                TraceLoggingWrite(
                    MidiSrvTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_INFO,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Activating MIDI 1.0 Input Endpoint for MidiFlowIn", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                RETURN_IF_FAILED(ActivateEndpointInternal(
                    parentInstanceId,
                    deviceInterfaceId.c_str(),
                    TRUE,
                    MidiFlowIn,
                    (ULONG)midi1InInterfaceProperties.size(),
                    0,
                    (DEVPROPERTY*)(midi1InInterfaceProperties.data()),
                    nullptr,
                    (SW_DEVICE_CREATE_INFO*)createInfo, 
                    nullptr));
            }
        }

        // return the created device interface Id. This is needed for anything that will 
        // do a match in the Ids from Windows.Devices.Enumeration
        if (createdDeviceInterfaceId != nullptr)
        {
            wil::unique_cotaskmem_string tempString = wil::make_cotaskmem_string_nothrow(deviceInterfaceId.c_str());
            RETURN_IF_NULL_ALLOC(tempString.get());
            *createdDeviceInterfaceId = tempString.release();
        }

        cleanupOnFailure.release();
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::ActivateEndpointInternal
(
    PCWSTR parentInstanceId,
    PCWSTR associatedInterfaceId,
    BOOL midiOne,
    MidiFlow flow,
    ULONG intPropertyCount,
    ULONG devPropertyCount,
    const DEVPROPERTY *interfaceDevProperties,
    const DEVPROPERTY *deviceDevProperties,
    const SW_DEVICE_CREATE_INFO *createInfo,
    std::wstring *deviceInterfaceId
)
{
    RETURN_HR_IF_NULL(E_INVALIDARG, interfaceDevProperties);
    RETURN_HR_IF(E_INVALIDARG, intPropertyCount == 0);

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Enter", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(parentInstanceId, "parent instance id"),
        TraceLoggingWideString(associatedInterfaceId, "associated interface id"),
        TraceLoggingBool(midiOne, "midi 1"),
        TraceLoggingULong(intPropertyCount, "interface prop count"),
        TraceLoggingULong(devPropertyCount, "device prop count")
    );

    SW_DEVICE_CREATE_INFO internalCreateInfo = *createInfo;

    std::unique_ptr<MIDIPORT> midiPort = std::make_unique<MIDIPORT>();

    RETURN_IF_NULL_ALLOC(midiPort);

    std::vector<DEVPROPERTY> interfaceProperties{};

    // copy the incoming array into a vector so that we can add additional items.
    if (interfaceDevProperties != nullptr && intPropertyCount > 0)
    {
        std::vector<DEVPROPERTY> existingInterfaceProperties(interfaceDevProperties, interfaceDevProperties + intPropertyCount);

        // copy 'em over
        interfaceProperties = existingInterfaceProperties;
    }

    if (associatedInterfaceId != nullptr && wcslen(associatedInterfaceId) > 0)
    {
        // add any additional required items to the vector
        interfaceProperties.push_back({ {PKEY_MIDI_AssociatedUMP, DEVPROP_STORE_SYSTEM, nullptr},
                   DEVPROP_TYPE_STRING, static_cast<ULONG>((wcslen(associatedInterfaceId) + 1) * sizeof(WCHAR)), (PVOID)associatedInterfaceId });

        midiPort->AssociatedInterfaceId = associatedInterfaceId;
    }
    else
    {
        // necessary for cases where this was previously set and cached

        interfaceProperties.push_back({ {PKEY_MIDI_AssociatedUMP, DEVPROP_STORE_SYSTEM, nullptr}, DEVPROP_TYPE_EMPTY, 0, nullptr });
    }


    DEVPROP_BOOLEAN devPropTrue = DEVPROP_TRUE;

    interfaceProperties.push_back({ {DEVPKEY_Device_PresenceNotForDevice, DEVPROP_STORE_SYSTEM, nullptr},
        DEVPROP_TYPE_BOOLEAN, static_cast<ULONG>(sizeof(devPropTrue)), &devPropTrue });

    interfaceProperties.push_back({ {DEVPKEY_Device_NoConnectSound, DEVPROP_STORE_SYSTEM, nullptr},
        DEVPROP_TYPE_BOOLEAN, static_cast<ULONG>(sizeof(devPropTrue)), &devPropTrue });

    midiPort->InstanceId = internal::NormalizeDeviceInstanceIdWStringCopy(createInfo->pszInstanceId);
    midiPort->Flow = flow;
    midiPort->Enumerator = midiOne ? AUDIO_DEVICE_ENUMERATOR : MIDI_DEVICE_ENUMERATOR;
    midiPort->MidiOne = midiOne;

    internalCreateInfo.pszzCompatibleIds = nullptr;
    internalCreateInfo.pszzHardwareIds = nullptr;

    if (midiOne)
    {
        if (flow == MidiFlowOut)
        {
            //midiPort->InstanceId += L"_O";
            midiPort->InterfaceCategory = &DEVINTERFACE_MIDI_OUTPUT;
        }
        else if (flow == MidiFlowIn)
        {
            //midiPort->InstanceId += L"_I";
            midiPort->InterfaceCategory = &DEVINTERFACE_MIDI_INPUT;
        }
        else
        {
            RETURN_IF_FAILED(E_INVALIDARG);
        }
    }
    else
    {
        internalCreateInfo.pszzCompatibleIds = szzMidiDeviceCompatibleId;
        internalCreateInfo.pszzHardwareIds = szzMidiDeviceHardwareId;

        if (flow == MidiFlowOut)
        {
            midiPort->InterfaceCategory = &DEVINTERFACE_UNIVERSALMIDIPACKET_OUTPUT;
        }
        else if (flow == MidiFlowIn)
        {
            midiPort->InterfaceCategory = &DEVINTERFACE_UNIVERSALMIDIPACKET_INPUT;
        }
        else if (flow == MidiFlowBidirectional)
        {
            midiPort->InterfaceCategory = &DEVINTERFACE_UNIVERSALMIDIPACKET_BIDI;
        }
        else
        {
            RETURN_IF_FAILED(E_INVALIDARG);
        }
    }

    // lambdas can only be converted to a function pointer if they
    // don't do capture, so copy everything into the CREATECONTEXT
    // to share with the SwDeviceCreate callback.
    CREATECONTEXT creationContext;
    creationContext.MidiPort = midiPort.get();
    creationContext.InterfaceDevProperties = interfaceProperties.data();
    creationContext.IntPropertyCount = (ULONG)interfaceProperties.size();

    // create the devnode for the device
    midiPort->hr = SwDeviceCreate(
        midiPort->Enumerator.c_str(),
        parentInstanceId,
        &internalCreateInfo,
        devPropertyCount,
        devPropertyCount == 0 ? (DEVPROPERTY*)nullptr : deviceDevProperties,
        SwMidiPortCreateCallback,
        &creationContext,
        wil::out_param(midiPort->SwDevice));

    if (SUCCEEDED(midiPort->hr))
    {
        // wait 10 seconds for creation to complete
        creationContext.CreationCompleted.wait(10000);
    }
    else if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == midiPort->hr)
    {
        //TraceLoggingWrite(
        //    MidiSrvTelemetryProvider::Provider(),
        //    MIDI_TRACE_EVENT_INFO,
        //    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        //    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        //    TraceLoggingPointer(this, "this"),
        //    TraceLoggingWideString(L"Endpoint already exists."),
        //    TraceLoggingWideString(parentInstanceId, "parent"),
        //    TraceLoggingBool(midiOne)
        //);

        // if the devnode already exists, then we likely only need to create/activate the interface, a previous
        // call created the devnode. First, locate the matching SwDevice handle for the existing devnode.
        auto item = std::find_if(m_midiPorts.begin(), m_midiPorts.end(), [&](const std::unique_ptr<MIDIPORT>& Port)
        {
            // if this instance id already activated, then we cannot activate/create a second time,
            if (midiPort->InstanceId == Port->InstanceId &&
                midiPort->Enumerator == Port->Enumerator)
            {
                return true;
            }

            return false;
        });

        if (item != m_midiPorts.end())
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Activating previously created endpoint."),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingWideString(midiPort->InstanceId.c_str(), "new port"),
                TraceLoggingWideString(midiPort->Enumerator.c_str(), "new port enumerator"),
                TraceLoggingBool(midiOne)
            );

            midiPort->SwDevice = item->get()->SwDevice;
            SwMidiPortCreateCallback(item->get()->SwDevice.get(), S_OK, &creationContext, nullptr);
        }
        else
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Endpoint exists, but it is not in the MidiDeviceManager store. Is something else creating endpoints?"),
                TraceLoggingWideString(parentInstanceId, "parent"),
                TraceLoggingWideString(midiPort->InstanceId.c_str(), "new port"),
                TraceLoggingWideString(midiPort->Enumerator.c_str(), "new port enumerator"),
                TraceLoggingBool(midiOne)
            );
        }
    }
    else
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"SwDeviceCreate failed", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent"),
            TraceLoggingBool(midiOne)
        );
    }

    // confirm we were able to register the interface

    if (FAILED(midiPort->hr))
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"SwDeviceCreate returned a failed HR", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingHResult(midiPort->hr, MIDI_TRACE_EVENT_HRESULT_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent"),
            TraceLoggingBool(midiOne)
        );
    }
    RETURN_IF_FAILED(midiPort->hr);

    if (midiPort->SwDeviceState != SWDEVICESTATE::Created)
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"SwDeviceState != SWDEVICESTATE::Created", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(parentInstanceId, "parent"),
            TraceLoggingBool(midiOne)
        );
    }
    RETURN_HR_IF(E_FAIL, midiPort->SwDeviceState != SWDEVICESTATE::Created);

    if (midiOne)
    {
        // Assign the port number for this midi port
        RETURN_IF_FAILED(AssignPortNumber(midiPort->SwDevice.get(), midiPort->DeviceInterfaceId.get(), flow));
    }

    // Activate the SWD just created, it's created in the disabled state to allow for assigning
    // the port prior to activating it.
    RETURN_IF_FAILED(SwDeviceInterfaceSetState(
        midiPort->SwDevice.get(),
        midiPort->DeviceInterfaceId.get(),
        TRUE));

    if (deviceInterfaceId)
    {
        *deviceInterfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(midiPort->DeviceInterfaceId.get());

        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(deviceInterfaceId->c_str(), "created device interface id")
        );
    }

    // success, transfer the midiPort to the list
    m_midiPorts.push_back(std::move(midiPort));

    return S_OK;
}


_Use_decl_annotations_
HRESULT
CMidiDeviceManager::UpdateEndpointProperties
(
    PCWSTR deviceInterfaceId,
    ULONG intPropertyCount,
    const DEVPROPERTY* interfaceDevProperties
)
{
    RETURN_HR_IF_NULL(E_INVALIDARG, deviceInterfaceId);
    RETURN_HR_IF_NULL(E_INVALIDARG, interfaceDevProperties);
    RETURN_HR_IF(E_INVALIDARG, intPropertyCount == 0);

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(deviceInterfaceId, MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
        TraceLoggingULong(intPropertyCount, "property count")
    );

    auto requestedInterfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(deviceInterfaceId);

    // locate the MIDIPORT 
    auto item = std::find_if(m_midiPorts.begin(), m_midiPorts.end(), [&](const std::unique_ptr<MIDIPORT>& Port)
        {
            auto portInterfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(Port->DeviceInterfaceId.get());

            return (portInterfaceId == requestedInterfaceId);
        });

    if (item == m_midiPorts.end())
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Device not found. If the updates are from the config file, the device may not yet have been enumerated.", MIDI_TRACE_EVENT_MESSAGE_FIELD),
            TraceLoggingWideString(requestedInterfaceId.c_str(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
            TraceLoggingULong(intPropertyCount, "property count")
        );

        // device not found
        return E_NOTFOUND;
    }
    else
    {
        // Using the found handle, add/update properties
        auto deviceHandle = item->get()->SwDevice.get();

        auto propSetHR = SwDeviceInterfacePropertySet(
            deviceHandle,
            item->get()->DeviceInterfaceId.get(),
            intPropertyCount,
            (const DEVPROPERTY*)interfaceDevProperties
        );

        if (SUCCEEDED(propSetHR))
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Properties set", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingWideString(requestedInterfaceId.c_str(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
                TraceLoggingULong(intPropertyCount, "property count")
            );

            // update/refresh the port numbers for all affected midi1 ports
            for (auto const& MidiPort : m_midiPorts)
            {
                if (MidiPort->AssociatedInterfaceId == requestedInterfaceId &&
                    MidiPort->MidiOne)
                {
                    RETURN_IF_FAILED(AssignPortNumber(MidiPort->SwDevice.get(), MidiPort->DeviceInterfaceId.get(), MidiPort->Flow));
                }
            }

            return S_OK;
        }
        else
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Error setting properties", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingHResult(propSetHR, MIDI_TRACE_EVENT_HRESULT_FIELD),
                TraceLoggingWideString(requestedInterfaceId.c_str(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
                TraceLoggingULong(intPropertyCount, "property count")
            );

            return propSetHR;
        }
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::DeleteEndpointProperties
(
    PCWSTR deviceInterfaceId,
    ULONG intPropertyCount,
    const DEVPROPERTY* interfaceDevPropKeys
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(deviceInterfaceId, MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
        TraceLoggingULong(intPropertyCount, "property count")
    );

    // create list of dev properties from the keys

    // Call UpdateEndpointProperties

    auto keys = (const DEVPROPKEY*)interfaceDevPropKeys;

    std::vector<DEVPROPERTY> properties;

    for (ULONG i = 0; i < intPropertyCount; i++)
    {
        properties.push_back({{ keys[i], DEVPROP_STORE_SYSTEM, nullptr },
            DEVPROP_TYPE_EMPTY, 0, NULL });
    }

    return UpdateEndpointProperties(deviceInterfaceId, (ULONG)properties.size(), properties.data());
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::DeactivateEndpoint
(
    PCWSTR instanceId
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Exit success", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(instanceId, MIDI_TRACE_EVENT_DEVICE_INSTANCE_ID_FIELD)
    );

    RETURN_HR_IF_NULL(E_INVALIDARG, instanceId);

    auto cleanId = internal::NormalizeDeviceInstanceIdWStringCopy(instanceId);

    // there may be more than one SWD associated with this instance id, as we reuse
    // the instance id for the legacy SWD, it just has a different activator and InterfaceClass.
    do
    {
        // locate the MIDIPORT that identifies the swd
        // NOTE: This uses instanceId, not the Device Interface Id
        auto item = std::find_if(m_midiPorts.begin(), m_midiPorts.end(), [&](const std::unique_ptr<MIDIPORT>& Port)
        {
            if (cleanId == Port->InstanceId)
            {
                return true;
            }

            return false;
        });

        // exit if the item was not found. We're done.
        if (item == m_midiPorts.end())
        {
            break;
        }
        else
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Found instance id in ports list. Erasing", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingWideString(cleanId.c_str(), MIDI_TRACE_EVENT_DEVICE_INSTANCE_ID_FIELD),
                TraceLoggingWideString(item->get()->DeviceInterfaceId.get(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD)
            );

            // Erasing this item from the list will free the unique_ptr and also trigger a SwDeviceClose on the item->SwDevice,
            // which will deactivate the device, done.
            m_midiPorts.erase(item);
        }
    } while (TRUE);


    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Exit success", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(cleanId.c_str(), MIDI_TRACE_EVENT_DEVICE_INSTANCE_ID_FIELD)
    );

    return S_OK;
}

_Use_decl_annotations_
HRESULT
CMidiDeviceManager::RemoveEndpoint
(
    PCWSTR instanceId 
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(instanceId, MIDI_TRACE_EVENT_DEVICE_INSTANCE_ID_FIELD)
    );

    // first deactivate, to ensure it's removed from the tracking list
    LOG_IF_FAILED(DeactivateEndpoint(instanceId));

    // TODO: Locate the device with this instance id using windows.devices.enumeration,
    // and delete it.

    return S_OK;
}


_Use_decl_annotations_
HRESULT
CMidiDeviceManager::UpdateTransportConfiguration
(
    GUID transportId,
    LPCWSTR configurationJson,
    LPWSTR* response
)
{
    if (configurationJson != nullptr)
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_INFO,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingPointer(this, "this"),
            TraceLoggingGuid(transportId, "transport id"),
            TraceLoggingWideString(configurationJson, "config json")
        );

        if (auto search = m_midiTransportConfigurationManagers.find(transportId); search != m_midiTransportConfigurationManagers.end())
        {
            auto configManager = search->second;

            if (configManager)
            {
                return configManager->UpdateConfiguration(configurationJson, response);
            }
        }
        else
        {
            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Failed to find the referenced transport by its GUID", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingGuid(transportId, "transport id")
            );

        }
    }
    else
    {
        TraceLoggingWrite(
            MidiSrvTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Json is null", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );
    }


    // failed to find the transport or its related endpoint manager
    return E_FAIL;
}


HRESULT
CMidiDeviceManager::Shutdown()
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
    );

    for (auto const& endpointManager : m_midiEndpointManagers)
    {
        LOG_IF_FAILED(endpointManager.second->Shutdown());
    }

    for (auto const& configurationManager : m_midiTransportConfigurationManagers)
    {
        LOG_IF_FAILED(configurationManager.second->Shutdown());
    }

    m_midiEndpointManagers.clear();
    m_midiTransportConfigurationManagers.clear();

    m_midiPorts.clear();

    m_midiParents.clear();

    return S_OK;
}

#define MIDI1_OUTPUT_DEVICES \
    L"System.Devices.InterfaceClassGuid:=\"{6DC23320-AB33-4CE4-80D4-BBB3EBBF2814}\""

#define MIDI1_INPUT_DEVICES \
    L"System.Devices.InterfaceClassGuid:=\"{504BE32C-CCF6-4D2C-B73F-6F8B3747E22B}\""

typedef struct _PORT_INFO
{
    bool inUse {false};
    bool isEnabled {false};
    bool hasCustomPortNumber{false};
    UINT32 customPortNumber {0};
    std::wstring interfaceId;
} PORT_INFO;


_Use_decl_annotations_
HRESULT
CMidiDeviceManager::AssignPortNumber(
    HSWDEVICE SwDevice,
    PWSTR deviceInterfaceId,
    MidiFlow flow
)
{
    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Enter", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(deviceInterfaceId, MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD)
    );

    auto interfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(deviceInterfaceId);

    std::map<UINT32, PORT_INFO> portInfo;

    bool hasServiceAssignedPortNumber {false};
    UINT32 serviceAssignedPortNumber {0};
    bool hasCustomPortNumber {false};
    UINT32 customPortNumber {0};
    bool interfaceEnabled {false};

    winrt::hstring deviceSelector(flow == MidiFlowOut?MIDI1_OUTPUT_DEVICES:MIDI1_INPUT_DEVICES);
    wil::unique_event enumerationCompleted{wil::EventOptions::None};

    auto additionalProperties = winrt::single_threaded_vector<winrt::hstring>();
    additionalProperties.Append(winrt::to_hstring(STRING_PKEY_MIDI_ServiceAssignedPortNumber));
    additionalProperties.Append(winrt::to_hstring(STRING_PKEY_MIDI_CustomPortNumber));

    auto watcher = DeviceInformation::CreateWatcher(deviceSelector, additionalProperties);

    auto deviceAddedHandler = watcher.Added(winrt::auto_revoke, [&](DeviceWatcher watcher, DeviceInformation device)
    {
        bool servicePortNumValid {false};
        UINT32 servicePortNum {0};
        bool userPortNumValid {false};
        UINT32 userPortNum {0};

        auto processingInterface = internal::NormalizeEndpointInterfaceIdWStringCopy(device.Id().c_str());

        // all others with a service assigned port number goes into the portInfo structure for processing.
        auto prop = device.Properties().Lookup(winrt::to_hstring(STRING_PKEY_MIDI_ServiceAssignedPortNumber));
        if (prop)
        {
            servicePortNum = winrt::unbox_value<UINT32>(prop);
            //servicePortNumValid = true;
            servicePortNumValid = servicePortNum <= MAX_WINMM_PORT_NUMBER;        // Temporary gate because this value sometimes comes back as int32.max

            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Endpoint has service-assigned port number", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingWideString(device.Id().c_str(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
                TraceLoggingUInt32(servicePortNum, "port number"),
                TraceLoggingBool(servicePortNumValid, "port number valid")
            );

            if (!servicePortNumValid) servicePortNum = 0;


        }

        prop = device.Properties().Lookup(winrt::to_hstring(STRING_PKEY_MIDI_CustomPortNumber));
        if (prop)
        {
            userPortNum = winrt::unbox_value<UINT32>(prop);
            //userPortNumValid = true;

            userPortNumValid = userPortNum <= MAX_WINMM_PORT_NUMBER;        // Temporary gate because this value sometimes comes back as int32.max

            TraceLoggingWrite(
                MidiSrvTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_INFO,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Endpoint has user-assigned port number", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                TraceLoggingWideString(device.Id().c_str(), MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD),
                TraceLoggingUInt32(userPortNum, "port number"),
                TraceLoggingBool(userPortNumValid, "port number valid")
            );

            if (!userPortNumValid) userPortNum = 0;
        }

        // if this is the interface we are processing, do not add it to the list, we want to determine if
        // there are any others using the requested port(s).
        if (processingInterface == interfaceId)
        {
            interfaceEnabled = device.IsEnabled();
            hasCustomPortNumber = userPortNumValid;
            customPortNumber = userPortNum;
            hasServiceAssignedPortNumber = servicePortNumValid;
            serviceAssignedPortNumber = servicePortNum;
        }
        else if (servicePortNumValid)
        {
            // we track for any port usage at all for assigning a new port number, avoiding 
            // ports that are in use even if inactive to prevent future conflicts.
            // There may be inactive users on the same port as an active user, so prefer to track
            // the currently active user of a given port in case we need to move them.
            if (!portInfo[servicePortNum].inUse || (!portInfo[servicePortNum].isEnabled && device.IsEnabled()))
            {
                portInfo[servicePortNum].inUse = true;
                portInfo[servicePortNum].isEnabled = device.IsEnabled();
                portInfo[servicePortNum].interfaceId = processingInterface;
                portInfo[servicePortNum].hasCustomPortNumber = userPortNumValid;
                portInfo[servicePortNum].customPortNumber = userPortNum;
            }
        }
    });

    auto deviceStoppedHandler = watcher.Stopped(winrt::auto_revoke, [&](DeviceWatcher, winrt::Windows::Foundation::IInspectable)
    {
        enumerationCompleted.SetEvent();
        return S_OK;
    });

    auto enumerationCompletedHandler = watcher.EnumerationCompleted(winrt::auto_revoke, [&](DeviceWatcher , winrt::Windows::Foundation::IInspectable)
    {
        enumerationCompleted.SetEvent();
        return S_OK;
    });

    watcher.Start();
    enumerationCompleted.wait();
    watcher.Stop();
    enumerationCompleted.wait();

    deviceAddedHandler.revoke();
    deviceStoppedHandler.revoke();
    enumerationCompletedHandler.revoke();

    // if we have a service assigned port number, but that number is already in use
    // by either and active port, or an inactive port and we're currently active (meaning this port
    // was requested to relocate), move this port somewhere else.
    bool serviceAssignedPortInUse = (hasServiceAssignedPortNumber &&
                                        portInfo[serviceAssignedPortNumber].inUse && 
                                        (interfaceEnabled || portInfo[serviceAssignedPortNumber].isEnabled));

    // if this port is already enabled, but not on it's ideal user assigned endpoint,
    // then an update to our user assigned port is desired (but not guaranteed).
    bool configUpdateDesired = (interfaceEnabled && 
                                hasCustomPortNumber && 
                                hasServiceAssignedPortNumber && 
                                serviceAssignedPortNumber != customPortNumber);

    // if no one is using the user assigned port number, or there is someone but they 
    // can be moved to some other port, then our optimal port is available to us.
    // This logic ensures that we do not take a user assigned port number from an enabled port
    // that was also assigned this same port number, otherwise we would end up in a recursion
    // loop with each port reassigning the other.
    bool optimalPortAvailable = (hasCustomPortNumber &&
                                    (!portInfo[customPortNumber].inUse || 
                                    !portInfo[customPortNumber].isEnabled ||
                                    !portInfo[customPortNumber].hasCustomPortNumber ||
                                    portInfo[customPortNumber].customPortNumber != customPortNumber));

    bool interfaceDeactivated{false};
    auto restoreInterfaceStateOnExit = wil::scope_exit([&]()
    {
        // if this interface was deactivated for a change, restore
        // the state to active before returning.
        if (interfaceDeactivated)
        {
            LOG_IF_FAILED(SwDeviceInterfaceSetState(SwDevice, deviceInterfaceId, TRUE));
        }
    });

    // if either the assigned port is already in use, or we're trying to move
    // this endpoint to its preferred user specified port number, move it.
    if (serviceAssignedPortInUse || (configUpdateDesired && optimalPortAvailable))
    {
        if (interfaceEnabled)
        {
            // first, disable the interface if it's currently active, because
            // we're going to be changing the port number for it and it can't be
            // active when we do that.
            LOG_IF_FAILED(SwDeviceInterfaceSetState(SwDevice, deviceInterfaceId, FALSE));
            interfaceDeactivated = true;
        }

        // clear the existing service assigned port number for this endpoint so that a new port
        // number will be assigned
        std::vector<DEVPROPERTY> newProperties{};
        newProperties.push_back({{ PKEY_MIDI_ServiceAssignedPortNumber, DEVPROP_STORE_SYSTEM, nullptr },
            DEVPROP_TYPE_EMPTY, 0, NULL });
        RETURN_IF_FAILED(SwDeviceInterfacePropertySet(
            SwDevice,
            deviceInterfaceId,
            1,
            (const DEVPROPERTY*)newProperties.data()));

        hasServiceAssignedPortNumber = false;
    }

    // no service assigned port number on this endpoint, assign one, preferring the user
    // assigned port number, if it's available.
    if (!hasServiceAssignedPortNumber)
    {
        UINT32 firstAvailablePortNumber = 0;
        UINT32 assignedPortNumber = 0;
        std::vector<DEVPROPERTY> newProperties{};

        // first we figure out what the default new port number will be, assuming the first available,
        // this will be our fallback in the event of any errors or the user assigned port isn't available.
        for(firstAvailablePortNumber = 1; portInfo[firstAvailablePortNumber].inUse;firstAvailablePortNumber++);
        assignedPortNumber = firstAvailablePortNumber;

        auto cleanupOnFailure = wil::scope_exit([&]()
        {
            // in the event that something fails with reassigning the old port, we want to fall back to a safe
            // unused  port number.
            newProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_ServiceAssignedPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                                    DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(firstAvailablePortNumber)) });
            LOG_IF_FAILED(SwDeviceInterfacePropertySet(
                SwDevice,
                deviceInterfaceId,
                1,
                (const DEVPROPERTY*)newProperties.data()));
            newProperties.clear();
        });

        // if we previously determined that the optimal (user specified)
        // port number is available for this endpoint, we want to use it.
        if (optimalPortAvailable)
        {
            assignedPortNumber = customPortNumber;   
        }

        // we've decided what the new port will be for this endpoint, lock it in.
        newProperties.push_back(DEVPROPERTY{ {PKEY_MIDI_ServiceAssignedPortNumber, DEVPROP_STORE_SYSTEM, nullptr},
                                DEVPROP_TYPE_UINT32, (ULONG)(sizeof(UINT32)), (PVOID)(&(assignedPortNumber)) });
        RETURN_IF_FAILED(SwDeviceInterfacePropertySet(
            SwDevice,
            deviceInterfaceId,
            1,
            (const DEVPROPERTY*)newProperties.data()));
        newProperties.clear();

        // Following from the logic above, we need to determine if the user assigned port number that was 
        // assigned to this port was in use by an enabled port and needs to have a previous user reassigned.
        if (hasCustomPortNumber && 
            assignedPortNumber == customPortNumber &&
            portInfo[customPortNumber].inUse &&
            portInfo[customPortNumber].isEnabled)
        {
            // First, we need to locate the MIDIPORT for the current user of this port, so we have the SW handle,
            // the MIDIPORT should exist, since the port is active.
            // If the MIDIPORT does not exist, then we can not move the conflicting port, exit with failure and fall back to the
            // next available for the assigning port.
            auto requestedInterfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(portInfo[customPortNumber].interfaceId);
            auto item = std::find_if(m_midiPorts.begin(), m_midiPorts.end(), [&](const std::unique_ptr<MIDIPORT>& Port)
            {
                auto portInterfaceId = internal::NormalizeEndpointInterfaceIdWStringCopy(Port->DeviceInterfaceId.get());
                return (portInterfaceId == requestedInterfaceId);
            });

            RETURN_HR_IF(E_ABORT, item != m_midiPorts.end());
            
            // Recursive call, assign a new port number to this port.
            RETURN_IF_FAILED(AssignPortNumber(item->get()->SwDevice.get(), item->get()->DeviceInterfaceId.get(), flow));
        }

        cleanupOnFailure.release();
    }

    TraceLoggingWrite(
        MidiSrvTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this"),
        TraceLoggingWideString(L"Exit", MIDI_TRACE_EVENT_MESSAGE_FIELD),
        TraceLoggingWideString(deviceInterfaceId, MIDI_TRACE_EVENT_DEVICE_SWD_ID_FIELD)
    );


    return S_OK;
}

