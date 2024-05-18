---
layout: api_page
title: MidiMessage64
parent: Messages
grand_parent: Microsoft.Devices.Midi2
has_children: false
---

# MidiMessage64

`MidiMessage64` is used for some data messages and for MIDI 2.0 Channel Voice messages.

Includes all functions and properties in `IMidiUniversalPacket`, as well as:

| Property | Description |
| -------- | ----------- |
| `Word0` | First 32-bit MIDI word|
| `Word1` | Second 32-bit MIDI word |

| Function | Description |
| -------- | ----------- |
| `MidiMessage64()` | Default constructor |
| `MidiMessage64(timestamp, word0, word1)` | Construct a new message with a timestamp and all 32 bit MIDI words |

## IDL

[MidiMessage64 IDL](https://github.com/microsoft/MIDI/blob/main/src/app-sdk/winrt-core/MidiMessage64.idl)

