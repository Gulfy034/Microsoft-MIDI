// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>

#include <hstring.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.Streams.h>

// this mDNS/DNS-SD API is quite a bit easier to use than the win32 version
#include <winrt/windows.networking.servicediscovery.dnssd.h>
#include <winrt/windows.networking.sockets.h>
#include <winrt/windows.networking.h>

namespace json = ::winrt::Windows::Data::Json;


#include <assert.h>
#include <devioctl.h>
#include <wrl\implements.h>
#include <wrl\module.h>
#include <wrl\event.h>
//#include <ks.h>
//#include <ksmedia.h>
#include <avrt.h>
#include <wil\com.h>
#include <wil\resource.h>
#include <wil\result_macros.h>
#include <wil\tracelogging.h>
#include <ppltasks.h>

#include <SDKDDKVer.h>

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlcoll.h>
#include <atlsync.h>

#include <winmeta.h>
#include <TraceLoggingProvider.h>

#include "SWDevice.h"
#include <initguid.h>
#include "setupapi.h"
//#include "Devpkey.h"

#include "strsafe.h"
#include "wstring_util.h"
#include "hstring_util.h"

#undef GetObject
#include <winrt/Windows.Data.Json.h>
namespace json = ::winrt::Windows::Data::Json;


// AbstractionUtilities
#include "wstring_util.h"
namespace internal = ::WindowsMidiServicesInternal;

#include "MidiDefs.h"
#include "WindowsMidiServices.h"
#include "WindowsMidiServices_i.c"

#include "json_defs.h"
#include "json_helpers.h"
#include "swd_helpers.h"
#include "resource_util.h"

#include "MidiXProc.h"

#include "Midi2NetworkMidiTransport_i.c"
#include "Midi2NetworkMidiTransport.h"

#include "dllmain.h"

class CMidi2NetworkMidiEndpointManager;
class CMidi2NetworkMidiConfigurationManager;
class MidiNetworkAdvertiser;
class MidiNetworkHostSession;
class MidiNetworkClientSession;

#include "transport_defs.h"

#include "MidiNetworkEndpointDefinition.h"

#include "MidiNetworkMessages.h"
#include "MidiNetworkMessageProcessor.h"

#include "MidiNetworkClient.h"
#include "MidiNetworkClientSession.h"

#include "MidiNetworkHost.h"
#include "MidiNetworkHostSession.h"

#include "MidiNetworkAdvertiser.h"

#include "TransportState.h"

#include "Midi2.NetworkMidiTransport.h"
#include "Midi2.NetworkMidiBiDi.h"
#include "Midi2.NetworkMidiEndpointManager.h"
#include "Midi2.NetworkMidiConfigurationManager.h"
#include "Midi2.NetworkMidiPluginMetadataProvider.h"
