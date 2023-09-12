﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Devices.Midi2.ConsoleApp
{
    // TODO: Add theme support here to allow for no colors, or colors which work on a lighter console.
    // theme should be saved in a prefs file
    // individual themes could be json files or, to keep things simple, just resources here
    // valid color codes https://spectreconsole.net/appendix/colors

    internal class AnsiMarkupFormatter
    {
        public static string FormatAppTitle(string title)
        {
            return "[deepskyblue1]" + title + "[/]";
        }

        public static string FormatAppDescription(string description)
        {
            return "[deepskyblue2]" + description + "[/]";
        }


        public static string FormatError(string error)
        {
            return "[red]" + error + "[/]";
        }
        public static string FormatTimestamp(UInt64 timestamp)
        {
            return "[olive]" + timestamp.ToString() + "[/]";
        }

        public static string FormatDeviceInstanceId(string id)
        {
            return "[olive]" + id.Trim() + "[/]";
        }

        public static string FormatEndpointName(string name)
        {
            return "[steelblue1_1]" + name.Trim() + "[/]";
        }

        public static string FormatGeneralNumber(UInt64 i)
        {
            return "[olive]" + i.ToString() + "[/]";
        }

        public static string FormatGeneralNumber(double d)
        {
            return "[olive]" + d.ToString() + "[/]";
        }


        public static string FormatMidiWords(params UInt32[] words)
        {
            string output = string.Empty;

            string[] colors = { "[deepskyblue1]", "[deepskyblue2]", "[deepskyblue3]", "[deepskyblue4]" };

            for (int i = 0; i < words.Length; i++)
            {
                output += string.Format(colors[i]+"{0:X8}[/] ", words[i]);

            }

            return output.Trim();
        }


    }
}
