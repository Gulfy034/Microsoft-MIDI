// ------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the GitHub project root for license information.
// ------------------------------------------------------------------------------------------------

#define WINDOWSMIDISERVICES_EXPORTS

#include "WindowsMidiServicesUtility.h"
#include "WindowsMidiServicesMessages.h"

namespace Microsoft::Windows::Midi::Messages
{

	const uint8_t Midi2ChannelVoiceMessage::getOpcode()
	{
		return Utility::MostSignificantNibble(Byte[1]);
	}

	const uint8_t Midi2ChannelVoiceMessage::getChannel()
	{
		return Utility::LeastSignificantNibble(Byte[1]);
	}

	void Midi2ChannelVoiceMessage::setChannel(const uint8_t value)
	{
		Utility::SetLeastSignificantNibble(Byte[1], value);
	}


}









