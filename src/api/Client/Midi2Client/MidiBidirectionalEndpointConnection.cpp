// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#include "pch.h"
#include "MidiBidirectionalEndpointConnection.h"
#include "MidiBidirectionalEndpointConnection.g.cpp"




namespace winrt::Windows::Devices::Midi2::implementation
{
    
    _Use_decl_annotations_
    bool MidiBidirectionalEndpointConnection::InternalInitialize(
        winrt::com_ptr<IMidiAbstraction> serviceAbstraction,
        winrt::hstring const endpointInstanceId,
        winrt::hstring const deviceId, 
        midi2::MidiBidirectionalEndpointOpenOptions options
    )
    {
        try
        {
            m_id = endpointInstanceId;
            m_inputDeviceId = deviceId;
            m_outputDeviceId = deviceId;

            WINRT_ASSERT(!DeviceId().empty());
            WINRT_ASSERT(serviceAbstraction != nullptr);

            m_serviceAbstraction = serviceAbstraction;

            // TODO: Read any settings we need for this endpoint


            // TODO: Add any automatic handlers if the options allow for it


            return true;
        }
        catch (winrt::hresult_error const& ex)
        {
            internal::LogHresultError(__FUNCTION__, L" hresult exception initializing endpoint.", ex);

            return false;
        }
    }


    _Use_decl_annotations_
    bool MidiBidirectionalEndpointConnection::Open()
    {
        if (!IsOpen())
        {
            // Activate the endpoint for this device. Will fail if the device is not a BiDi device
            // we use m_inputAbstraction here and simply provide a copy of it to m_outputAbstraction so that
            // we don't have to duplicate all that code
            if (!ActivateMidiStream(m_serviceAbstraction, __uuidof(IMidiBiDi), (void**)&m_inputAbstraction))
            {
                internal::LogGeneralError(__FUNCTION__, L"Could not activate MIDI Stream");

                return false;
            }

            if (m_inputAbstraction != nullptr)
            {
                try
                {
                    DWORD mmcssTaskId{};  // TODO: Does this need to be session-wide? Probably, but it can be modified by the endpoint init, so maybe should be endpoint-local

                    winrt::check_hresult(m_inputAbstraction->Initialize(
                        (LPCWSTR)(DeviceId().c_str()),
                        &mmcssTaskId,
                        (IMidiCallback*)(this)
                    ));

                    // provide a copy to the output logic
                    m_outputAbstraction = m_inputAbstraction;

                    IsOpen(true);
                    OutputIsOpen(true);
                    InputIsOpen(true);

                    CallOnConnectionOpenedOnPlugins();

                    return true;
                }
                catch (winrt::hresult_error const& ex)
                {
                    internal::LogHresultError(__FUNCTION__, L" hresult exception initializing endpoint interface. Service may be unavailable.", ex);

                    m_inputAbstraction = nullptr;
                    m_outputAbstraction = nullptr;

                    return false;
                }
            }
            else
            {
                internal::LogGeneralError(__FUNCTION__, L" Endpoint interface is nullptr");

                return false;
            }
        }
        else
        {
            // already open. Just return true here. Not fatal in any way, so no error
            return true;
        }
    }




    // TODO: Move this logic to the base classes

    _Use_decl_annotations_
    MidiBidirectionalEndpointConnection::~MidiBidirectionalEndpointConnection()
    {
        if (m_inputAbstraction != nullptr)
        {
            m_inputAbstraction->Cleanup();
            m_inputAbstraction = nullptr;
        }

        m_outputAbstraction = nullptr;

        IsOpen(false);
        InputIsOpen(false);
        OutputIsOpen(false);

        // TODO: any event cleanup?
    }

}
