---
layout: api_page
title: MidiSessionConnectionInformation
parent: Midi2.Diagnostics
has_children: false
---

# MidiSessionConnectionInformation

This class represents an open connection in a Windows MIDI Services session. This is an informational class only for reporting system-wide connection usage. 

## Properties

| Property | Description |
|---|---|
| `EndpointDeviceId` | The endpoint device id for the connection |
| `InstanceCount` | The number of instances of this connection which are open in the parent session |
| `EarliestConnectionTime` | The date and time the first instance of the connection was opened |

## IDL

[MidiSessionConnectionInformation IDL](https://github.com/microsoft/MIDI/blob/main/src/app-sdk/winrt-diagnostics/MidiSessionConnectionInformation.idl)

