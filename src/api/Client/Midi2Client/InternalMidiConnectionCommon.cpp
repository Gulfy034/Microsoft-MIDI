// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================


#include <pch.h>

#include "InternalMidiConnectionCommon.h"

namespace Windows::Devices::Midi2::Internal
{
    //_Use_decl_annotations_
    //void InternalMidiConnectionCommon::SetInputConnectionOnPlugins(midi2::IMidiInputConnection inputConnection)
    //{
    //    try
    //    {
    //        for (const auto& plugin : m_messageProcessingPlugins)
    //        {
    //            auto listener = plugin.try_as<midi2::IMidiEndpointMessageListener>();

    //            if (listener != nullptr)
    //            {
    //                listener.InputConnection(inputConnection);
    //            }
    //        }
    //    }
    //    catch (...)
    //    {
    //    }
    //}

    //_Use_decl_annotations_
    //void InternalMidiConnectionCommon::SetOutputConnectionOnPlugins(midi2::IMidiOutputConnection outputConnection)
    //{
    //    try
    //    {
    //        for (const auto& plugin : m_messageProcessingPlugins)
    //        {
    //            auto responder = plugin.try_as<midi2::IMidiEndpointMessageResponder>();

    //            if (responder != nullptr)
    //            {
    //                responder.OutputConnection(outputConnection);
    //            }
    //        }
    //    }
    //    catch (...)
    //    {
    //    }
    //}

    //void InternalMidiConnectionCommon::InitializePlugins()
    //{
    //    for (const auto& plugin : m_messageProcessingPlugins)
    //    {
    //        try
    //        {
    //            plugin.Initialize();
    //        }
    //        catch (...)
    //        {
    //        }
    //    }
    //}

    //void InternalMidiConnectionCommon::CallOnConnectionOpenedOnPlugins()
    //{
    //    for (const auto& plugin : m_messageProcessingPlugins)
    //    {
    //        try
    //        {
    //            plugin.OnEndpointConnectionOpened();
    //        }
    //        catch (...)
    //        {
    //        }
    //    }
    //}

    //void InternalMidiConnectionCommon::CleanupPlugins()
    //{
    //    for (const auto& plugin : m_messageProcessingPlugins)
    //    {
    //        try
    //        {
    //            plugin.Cleanup();
    //        }
    //        catch (...)
    //        {
    //            // eat any exceptions
    //        }
    //    }
    //}

    _Use_decl_annotations_
    bool InternalMidiConnectionCommon::ActivateMidiStream(
        winrt::com_ptr<IMidiAbstraction> serviceAbstraction,
        const IID& iid,
        void** iface) noexcept
    {
        try
        {
            winrt::check_hresult(serviceAbstraction->Activate(iid, iface));

            return true;
        }
        catch (winrt::hresult_error const& ex)
        {
            internal::LogHresultError(__FUNCTION__, L"hresult exception activating stream. Service may be unavailable", ex);

            return false;
        }
    }
}