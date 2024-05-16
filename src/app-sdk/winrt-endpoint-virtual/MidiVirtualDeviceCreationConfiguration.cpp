// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

#include "pch.h"
#include "MidiVirtualDeviceCreationConfiguration.h"
#include "MidiVirtualDeviceCreationConfiguration.g.cpp"

namespace winrt::Microsoft::Devices::Midi2::Endpoints::Virtual::implementation
{
    MidiVirtualDeviceCreationConfiguration::MidiVirtualDeviceCreationConfiguration(
        _In_ winrt::hstring name,
        _In_ winrt::hstring description,
        _In_ winrt::hstring manufacturer,
        midi2::MidiDeclaredEndpointInfo declaredEndpointInfo
    )
    {
        m_endpointName = name;
        m_description = description;
        m_manufacturer = manufacturer;
        m_declaredEndpointInfo = declaredEndpointInfo;
    }

    MidiVirtualDeviceCreationConfiguration::MidiVirtualDeviceCreationConfiguration(
        _In_ winrt::hstring name,
        _In_ winrt::hstring description,
        _In_ winrt::hstring manufacturer,
        midi2::MidiDeclaredEndpointInfo declaredEndpointInfo,
        midi2::MidiDeclaredDeviceIdentity declaredDeviceIdentity
        ) : MidiVirtualDeviceCreationConfiguration(name, description, manufacturer, declaredEndpointInfo)
    {
        m_declaredDeviceIdentity = declaredDeviceIdentity;
    }

    MidiVirtualDeviceCreationConfiguration::MidiVirtualDeviceCreationConfiguration(
        _In_ winrt::hstring name,
        _In_ winrt::hstring description,
        _In_ winrt::hstring manufacturer,
        midi2::MidiDeclaredEndpointInfo declaredEndpointInfo,
        midi2::MidiDeclaredDeviceIdentity declaredDeviceIdentity,
        midi2::MidiEndpointUserSuppliedInfo userSuppliedInfo
    ) : MidiVirtualDeviceCreationConfiguration(name, description, manufacturer, declaredEndpointInfo, declaredDeviceIdentity)
    {
        m_userSuppliedInfo = userSuppliedInfo;
    }



    // this is the format we're sending up
    // 
    //"endpointTransportPluginSettings":
    //{
    //    "{8FEAAD91-70E1-4A19-997A-377720A719C1}":
    //    {
    //       "create":
    //       [
    //           {
    //              ... properties ...
    //           }
    //       ]
    //    }
    //}
    //
    winrt::hstring MidiVirtualDeviceCreationConfiguration::GetConfigurationJson() const noexcept
    {
        // create the json for creating the endpoint

        json::JsonObject endpointDefinitionObject;

        json::JsonArray virtualDevicesCreationArray;
        json::JsonObject abstractionObject;
        json::JsonObject topLevelTransportPluginSettingsObject;
        json::JsonObject outerWrapperObject;

        // Create association guid
        auto associationGuid = foundation::GuidHelper::CreateNewGuid();

        // set all of our properties.

        internal::JsonSetGuidProperty(
            endpointDefinitionObject,
            MIDI_CONFIG_JSON_ENDPOINT_VIRTUAL_DEVICE_ASSOCIATION_ID_PROPERTY_KEY,
            associationGuid);

        endpointDefinitionObject.SetNamedValue(
            MIDI_CONFIG_JSON_ENDPOINT_COMMON_UNIQUE_ID_PROPERTY,
            json::JsonValue::CreateStringValue(m_declaredEndpointInfo.ProductInstanceId));

        endpointDefinitionObject.SetNamedValue(
            MIDI_CONFIG_JSON_ENDPOINT_COMMON_NAME_PROPERTY,
            json::JsonValue::CreateStringValue(m_endpointName));

        endpointDefinitionObject.SetNamedValue(
            MIDI_CONFIG_JSON_ENDPOINT_COMMON_DESCRIPTION_PROPERTY,
            json::JsonValue::CreateStringValue(m_description));

        endpointDefinitionObject.SetNamedValue(
            MIDI_CONFIG_JSON_ENDPOINT_COMMON_MANUFACTURER_PROPERTY,
            json::JsonValue::CreateStringValue(m_manufacturer));

        // TODO: Other props that have to be set at the service level and not in-protocol

        virtualDevicesCreationArray.Append(endpointDefinitionObject);

        // create the abstraction object with the child creation node

        abstractionObject.SetNamedValue(
            MIDI_CONFIG_JSON_ENDPOINT_COMMON_CREATE_KEY,
            virtualDevicesCreationArray);

        // create the main node with the abstraction id property as key to the array

        topLevelTransportPluginSettingsObject.SetNamedValue(
            internal::GuidToString(virt::MidiVirtualDeviceManager::AbstractionId()),
            abstractionObject);

        // wrap it all up so the json is valid

        outerWrapperObject.SetNamedValue(
            MIDI_CONFIG_JSON_TRANSPORT_PLUGIN_SETTINGS_OBJECT,
            topLevelTransportPluginSettingsObject);


        return outerWrapperObject.Stringify();
    }



}