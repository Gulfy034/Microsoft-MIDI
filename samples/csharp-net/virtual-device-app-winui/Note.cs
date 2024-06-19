﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Devices.Midi2;
using midi2 = Windows.Devices.Midi2;

namespace MidiSample.AppToAppMidi
{

    public class Note
    {
        public midi2.MidiEndpointConnection Connection { get; set; }
        public byte NoteNumber { get; set; }

        public byte GroupIndex { get; set; }

        public byte ChannelIndex { get; set; }

        public void NoteOn()
        {
            System.Diagnostics.Debug.Write("Note On");

            UInt16 index = NoteNumber;
            index <<= 8;

            UInt16 velocity = 0xFFFF;

            UInt32 word1 = (UInt32)velocity << 16;

            if (MidiEndpointConnection.SendMessageSucceeded(Connection.SendSingleMessagePacket(
                midi2.MidiMessageBuilder.BuildMidi2ChannelVoiceMessage(
                    0,
                    new MidiGroup(GroupIndex),
                    midi2.Midi2ChannelVoiceMessageStatus.NoteOn,
                    new MidiChannel(ChannelIndex),
                    index,
                    word1))))
            {
                System.Diagnostics.Debug.WriteLine(" - sent");
            }
        }
        public void NoteOff()
        {
            System.Diagnostics.Debug.Write("Note Off");

            UInt16 index = NoteNumber;
            index <<= 8;

            if (MidiEndpointConnection.SendMessageSucceeded(Connection.SendSingleMessagePacket(
                midi2.MidiMessageBuilder.BuildMidi2ChannelVoiceMessage(
                    0,
                    new MidiGroup(GroupIndex),
                    midi2.Midi2ChannelVoiceMessageStatus.NoteOff,
                    new MidiChannel(ChannelIndex),
                    index,
                    0))))
            {
                System.Diagnostics.Debug.WriteLine(" - sent");
            }
        }
    }


}
