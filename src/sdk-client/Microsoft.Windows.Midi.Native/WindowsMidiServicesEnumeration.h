// ------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the GitHub project root for license information.
// ------------------------------------------------------------------------------------------------

// ====================================================================
// PRE-RELEASE VERSION. BREAKING CHANGES LIKELY. NOT FOR PRODUCTION USE.
// For more information, please see https://github.com/microsoft/midi
// ====================================================================

#pragma once

#include <Windows.h>

#ifdef WINDOWSMIDISERVICES_EXPORTS
#define WINDOWSMIDISERVICES_API __declspec(dllexport)
#else
#define WINDOWSMIDISERVICES_API __declspec(dllimport)
#endif

#include "WindowsMidiServicesUmp.h"

// ----------------------------------------------------------------------------
// Enumeration
// ----------------------------------------------------------------------------

namespace Microsoft::Windows::Midi::Enumeration //::inline v0_1_0_pre
{
	// GUIDs are used for IDs. If porting this to another platform, consider
	// usig something like CrossGuid, taking an API dependency on boost, or
	// simply redefining as necessary
	// https://github.com/graeme-hill/crossguid

		// examples: USB, BLE, RTP
	struct WINDOWSMIDISERVICES_API MidiTransportInformation
	{
	private:
		struct implMidiTransportInformation;
		implMidiTransportInformation* _pimpl;


		MidiTransportInformation(const MidiTransportInformation& info);// don't copy
	public:
		MidiTransportInformation(MidiObjectId id);
		~MidiTransportInformation();
		const MidiObjectId getId();						// Unique Id of the type of transport. Referenced by the device. Created by plugin and retained across reboots
		const wchar_t* getName();						// Name, like BLE, RTP, USB etc.
		const wchar_t* getLongName();					// Longer name like Bluetooth Low Energy MIDI 1.0
		const wchar_t* getIconFileName();				// Name, without path, of the image used to represent this type of transport
		const bool getSupportsRuntimeDeviceCreation();	// true if this supports creating virtual devices/streams

		friend class MidiEnumerator;
	};



	// MIDI Device
	// ----------------------------------
	// MIDI Devices are similar to, but not identical to, devices as described
	// in the MIDI CI and MIDI 2.0 specifications. The device, in this case
	// is whatever is connected to the PC. That may be a USB or BLE-connected
	// device, a virtual device with streams, a network device, etc.
	// In the protocol specs, a device is anything connected to MIDI, which
	// includes any of the 256 possible things connected via groups/channels
	// on a single transport
	struct WINDOWSMIDISERVICES_API MidiDeviceInformation
	{
	private:
		struct implMidiDeviceInformation;
		implMidiDeviceInformation* _pimpl;


		MidiDeviceInformation(const MidiDeviceInformation& info);	// don't copy
	public:
		MidiDeviceInformation(MidiObjectId id, MidiObjectId transportId);
		~MidiDeviceInformation();
		MidiObjectId getId();						// Unique Id of the device. Used in most MIDI messaging
		MidiObjectId getTransportId();				// Uinque Id of the transport used by the device. For displaying appropriate name/icons
		const wchar_t* getName();					// Device name. May have been changed by the user through config tools
		const wchar_t* getDeviceSuppliedName();		// Device name as supplied by the device plug-in or driver
		const char8_t* getSerial();					// If there's a unique serial number for the device, we track it here.
		const wchar_t* getIconFileName();			// Name, without path, of the image used to represent this specific device
		const wchar_t* getDescription();			// user-supplied long text description

		const bool getIsRuntimeCreated();						// true if this was created at runtime
		const uint32_t getOwningProcessIdIfRuntimeCreated();	// owning process ID.

		friend class MidiEnumerator;
	};

	enum WINDOWSMIDISERVICES_API MidiStreamType
	{
		MidiStreamTypeOutput = 0,
		MidiStreamTypeInput = 1,
		MidiStreamTypeBidirectional = 2			// either a negotiated port pair, or a true bidirectional stream
	};

	// MIDI Stream
	// ----------------------------------
	// A stream is what UMPs are sent to and received from. Similar to a 
	// Port in MIDI 1.0
	// 
	// For USB-connected single devices, a stream is often 1:1
	// with the device but for devices which provide other streams (like a
	// synth with multiple DIN MIDI ports, or other USB or network ports), 
	// device:stream relationship is a 1:1 to 1:many relationship
	// In addition to true bi-directional streams (like network, USB, etc.)
	// Endpoints also encapsulate any negotiated bi-directional communications,
	// involving pairing of discrete ports, for purposes of MIDI CI. 
	struct WINDOWSMIDISERVICES_API MidiStreamInformation final
	{
	private:
		struct implMidiStreamInformation;
		implMidiStreamInformation* _pimpl;


		MidiStreamInformation(const MidiStreamInformation& info);	// don't copy
	public:
		MidiStreamInformation(MidiObjectId id, MidiObjectId parentDeviceId, MidiStreamType streamType);
		~MidiStreamInformation();
		const MidiObjectId getId();					// Unique Id of the stream. Used in most MIDI messaging
		const MidiObjectId getParentDeviceId();		// Unique Id of the parent device which owns this stream.
		const MidiStreamType getStreamType();		// Type of stream. Mostly used to differentiate unidirectional (like DIN) from bidirectional streams
		const wchar_t* getName();					// Name of this stream. May have been changed by the user through config tools.
		const wchar_t* getDeviceSuppliedName();		// Endpoint name as supplied by the device plug-in or driver
		const wchar_t* getIconFileName();			// Name, without path, of the image used to represent this specific endpoint
		const wchar_t* getDescription();			// Text description of the stream.

		// TODO: Expose appropriate MIDI CI information per group/channel as negotiated by service
		// For example, bandwidth, protocol, etc.
		// Note that entire API is UMP, so translation to/from byte stream happens
		// either in the driver (example: USB) or in the device/transport plugin

		friend class MidiEnumerator;

	};

	// ----------------------------------------------------------------------------
	// Enumeration change callbacks / delegates
	// ----------------------------------------
	// In previous versions of Windows MIDI APIs, like WinMM or WinRT MIDI, devices
	// and ports could be added or removed, but generally did not otherwise change. 
	// In this version, and in MIDI 2.0 in general, devices, streams, and more
	// can change properties at any time. Those changes may be due to MIDI CI
	// notifications, programmatic virtual ports, or user action in settings apps. 
	// 
	// We encourage developers to track when devices/streams are added or removed, 
	// or when properties of those devices/streams/etc change. 
	// 
	// The API objects themselves will be automatically updated as a result of 
	// these change notifications.
	// ----------------------------------------------------------------------------

	typedef WINDOWSMIDISERVICES_API void (*MidiTransportAddedCallback)(
		const MidiObjectId transportId);

	typedef WINDOWSMIDISERVICES_API void(*MidiTransportRemovedCallback)(
		const MidiObjectId transportId);

	typedef WINDOWSMIDISERVICES_API void(*MidiTransportChangedCallback)(
		const MidiObjectId transportId);


	typedef WINDOWSMIDISERVICES_API void(*MidiDeviceAddedCallback)(
		const MidiObjectId deviceId);

	typedef WINDOWSMIDISERVICES_API void(*MidiDeviceRemovedCallback)(
		const MidiObjectId deviceId);

	typedef WINDOWSMIDISERVICES_API void(*MidiDeviceChangedCallback)(
		const MidiObjectId deviceId);


	typedef WINDOWSMIDISERVICES_API void(*MidiStreamAddedCallback)(
		const MidiObjectId deviceId,
		const MidiObjectId streamId);

	typedef WINDOWSMIDISERVICES_API void(*MidiStreamRemovedCallback)(
		const MidiObjectId deviceId,
		const MidiObjectId streamId);

	typedef WINDOWSMIDISERVICES_API void(*MidiStreamChangedCallback)(
		const MidiObjectId deviceId,
		const MidiObjectId streamId);




	class WINDOWSMIDISERVICES_API MidiTransportInformationCollection final
	{
	private:
		struct implMidiTransportInformationCollection;
		implMidiTransportInformationCollection* _pimpl;

		MidiTransportInformationCollection();
	public:
		~MidiTransportInformationCollection();

		// TODO. Implement C++ iterator-like pattern here without exposting std::

		friend class MidiEnumerator;
	};

	class WINDOWSMIDISERVICES_API MidiDeviceInformationCollection final
	{
	private:
		struct implMidiDeviceInformationCollection;
		implMidiDeviceInformationCollection* _pimpl;

		MidiDeviceInformationCollection();
	public:
		~MidiDeviceInformationCollection();

		// TODO. Implement C++ iterator-like pattern here without exposting std::

		friend class MidiEnumerator;
	};

	class WINDOWSMIDISERVICES_API MidiStreamInformationCollection final
	{
	private:
		struct implMidiStreamInformationCollection;
		implMidiStreamInformationCollection* _pimpl;

		MidiStreamInformationCollection();
	public:
		~MidiStreamInformationCollection();

		// TODO. Implement C++ iterator-like pattern here without exposting std::

		friend class MidiEnumerator;
	};


	enum WINDOWSMIDISERVICES_API MidiEnumeratorCreateResultErrorDetail
	{
		MidiEnumeratorCreateErrorCommunication = 999,
		MidiEnumeratorCreateErrorOther = 1000
	};

	struct WINDOWSMIDISERVICES_API MidiEnumeratorCreateResult
	{
		bool Success;
		MidiEnumeratorCreateResultErrorDetail ErrorDetail;	// Additional error information
		MidiEnumerator* Enumerator;
	};



	// Enumerator class. Responsible for exposing information about every device
	// and stream known to the system. Service-side, the first time enumeration 
	// happens it causes MIDI CI calls to be made to negotiate properties of 
	// connected devices and stream
	class WINDOWSMIDISERVICES_API MidiEnumerator final
	{
	private:
		struct implMidiEnumerator;
		implMidiEnumerator* _pimpl;

		MidiEnumerator(const MidiEnumerator& info);	// don't copy
		MidiEnumerator();
	public:
		~MidiEnumerator();

		// creates the enumerator and returns when enumeration has been completed
		// if enumeration is skipped (good if you want to create a bunch of virtual
		// devices up-front), call Load when you are ready to enumerate
		static MidiEnumeratorCreateResult Create(bool skipEnumeration);

		// loads all the tree info. Call this if you skipped enumeration in the factory method.
		// TODO: Eval having this return a struct result like other calls? Really just need to know if it succeeded and what errors (if any)
		bool Load();

		// these return copies of the objects rather than pointers into the tree, to
		// help eliminate potential memory leaks or information changing while you
		// have the references.
		const bool GetTransportInformationFromId(
			MidiObjectId transportId,
			MidiTransportInformation& info);

		const bool GetDeviceInformationFromId(
			MidiObjectId deviceId,
			MidiDeviceInformation& info);

		const bool GetStreamInformationFromId(
			MidiObjectId deviceId,
			MidiObjectId streamId,
			MidiStreamInformation& info);

		// These return copies instead of pointers because the underlying objects
		// could be destroyed by the time they are accessed by the API's client
		// Recommended only for presenting a list to the user or similar. Do not 
		// hold on to these objects as they will not receive change notifications

		const MidiTransportInformationCollection GetStaticTransportList();

		const MidiDeviceInformationCollection GetStaticDeviceList();
		const MidiDeviceInformationCollection GetStaticDeviceListByName(wchar_t* name);
		const MidiDeviceInformationCollection GetStaticDeviceListByDeviceSuppliedName(wchar_t* deviceSuppliedDeviceName);

		const MidiStreamInformationCollection GetStaticStreamList();
		const MidiStreamInformationCollection GetStaticStreamList(MidiObjectId deviceId);
		const MidiStreamInformationCollection GetStaticStreamListByName(wchar_t* name);
		const MidiStreamInformationCollection GetStaticStreamListByDeviceSuppliedName(wchar_t* deviceSuppliedStreamName);


		void SubscribeToTransportChangeNotifications(
			const MidiTransportAddedCallback& addedCallback,
			const MidiTransportRemovedCallback& removedCallback,
			const MidiTransportChangedCallback& changedCallback);

		void SubscribeToDeviceChangeNotifications(
			const MidiDeviceAddedCallback& addedCallback,
			const MidiDeviceRemovedCallback& removedCallback,
			const MidiDeviceChangedCallback& changedCallback);

		void SubscribeToEndpointChangeNotifications(
			const MidiStreamAddedCallback& addedCallback,
			const MidiStreamRemovedCallback& removedCallback,
			const MidiStreamChangedCallback& changedCallback);


	};
}

