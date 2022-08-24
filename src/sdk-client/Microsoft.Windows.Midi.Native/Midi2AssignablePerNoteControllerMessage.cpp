// ------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the GitHub project root for license information.
// ------------------------------------------------------------------------------------------------

#define WINDOWSMIDISERVICES_EXPORTS

#include "WindowsMidiServicesUtility.h"
#include "WindowsMidiServicesMessages.h"

namespace Microsoft::Windows::Midi::Messages
{

	const uint8_t Midi2AssignablePerNoteControllerMessage::getNoteNumber()
	{
		return 0;
	}

	void Midi2AssignablePerNoteControllerMessage::setNoteNumber(const uint8_t value)
	{
	}

	const uint8_t Midi2AssignablePerNoteControllerMessage::getIndex()
	{
		return 0;
	}

	void Midi2AssignablePerNoteControllerMessage::setIndex(const uint8_t value)
	{
	}

	const uint32_t Midi2AssignablePerNoteControllerMessage::getData()
	{
		return 0;
	}

	void Midi2AssignablePerNoteControllerMessage::setData(const uint32_t value)
	{
	}

	Midi2AssignablePerNoteControllerMessage Midi2AssignablePerNoteControllerMessage::FromValues(const uint8_t group, const uint8_t channel, const uint8_t noteNumber, const uint8_t index, const uint32_t data)
	{
		Midi2AssignablePerNoteControllerMessage msg;
		return msg;

	}


}