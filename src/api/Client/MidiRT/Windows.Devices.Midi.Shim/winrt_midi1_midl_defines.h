// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once

// These are required for internal Microsoft build support and if set incorrectly
// will break any public compilation.

#define QUOTE(s) #s


#ifdef MICROSOFT_INTERNAL_BUILD

// internal build (using Razzle)

#define MIDI_ROOT_NAMESPACE_IDL Windows.Devices.Midi
#define MIDI_ROOT_NAMESPACE_CPP Windows::Devices::Midi
#define MIDI_CONCAT_NAMESPACE(r,n) QUOTE(r.n)
#define MIDI_MIDL_INTERFACE_NAME(n) MIDI_CONCAT_NAMESPACE(MIDI_ROOT_NAMESPACE_IDL, n)

#define MIDI1_SHIM_API_CONTRACT(n) contract(Windows.Foundation.UniversalApiContract, n) 

#define MIDI_IDL_IMPORT \
import "Windows.Foundation.idl"; \
import "Windows.Devices.Enumeration.idl"; \

#define MIDI_INTERFACE_UUID(u,v) uuid(u)

#else

// public build (using Visual Studio, msbuild, etc.)

#define MIDI_ROOT_NAMESPACE_IDL Microsoft.Windows.Devices.Midi
#define MIDI_ROOT_NAMESPACE_CPP Microsoft::Windows::Devices::Midi
#define MIDI_CONCAT_NAMESPACE(r,n) QUOTE(r.n)
#define MIDI_MIDL_INTERFACE_NAME(n) MIDI_CONCAT_NAMESPACE(MIDI_ROOT_NAMESPACE_IDL, n)


#define MIDI1_SHIM_API_CONTRACT(n) 
#define MIDI_IDL_IMPORT
#define MIDI_INTERFACE_UUID(u,v) uuid(u), version(v)

#endif

