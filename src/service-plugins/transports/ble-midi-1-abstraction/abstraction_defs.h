// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================

#pragma once

#define ABSTRACTION_LAYER_GUID __uuidof(Midi2BluetoothMidiAbstraction);


// the IDs here aren't the full Ids, just the values we start with
// The full Id comes back from the swdevicecreate callback

#define TRANSPORT_MNEMONIC L"BLE1"
#define MIDI_BLE_INSTANCE_ID_PREFIX L"MIDIU_BLE1_"

#define TRANSPORT_PARENT_ID L"MIDIU_BLE1_TRANSPORT"

// TODO: Names should be moved to .rc for localization
#define TRANSPORT_PARENT_DEVICE_NAME L"MIDI 1.0 Bluetooth Devices"


#define LOOPBACK_PARENT_ROOT L"HTREE\\ROOT\\0"

#define TRANSPORT_ENUMERATOR L"MIDISRV"
