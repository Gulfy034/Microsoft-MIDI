﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of Windows MIDI Services and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================



//using Microsoft.Windows.Devices.Midi2.Initialization;
using Microsoft.Windows.Devices.Midi2.Messages;

namespace Microsoft.Midi.ConsoleApp
{
    internal class EndpointSendMessagesFileCommand : Command<EndpointSendMessagesFileCommand.Settings>
    {
        public sealed class Settings : SendMessageCommandSettings
        {
            [LocalizedDescription("ParameterSendMessagesFileCommandFile")]
            [CommandArgument(1, "<Input File>")]
            public string? InputFile { get; set; }

            [EnumLocalizedDescription("ParameterSendMessagesFileFieldDelimiter", typeof(ParseFieldDelimiter))]
            [CommandOption("-d|--delimiter")]
            [DefaultValue(ParseFieldDelimiter.Auto)]
            public ParseFieldDelimiter FieldDelimiter { get; set; }

            [LocalizedDescription("ParameterSendMessagesFileVerbose")]
            [CommandOption("-v|--verbose")]
            [DefaultValue(false)]
            public bool Verbose { get; set; }

            [LocalizedDescription("ParameterSendMessagesFileReplaceGroup")]
            [CommandOption("-g|--new-group-index")]
            public int? NewGroupIndex { get; set; }

            //Settings()
            //{
            //    InputFile = String.Empty;
            //}
        }

        public override ValidationResult Validate(CommandContext context, Settings settings)
        {
            if (settings.InputFile == null)
            {
                // TODO: Localize
                return ValidationResult.Error($"File not specified.");
            }

            if (settings.InputFile != null && !System.IO.File.Exists(settings.InputFile))
            {
                // TODO: Localize
                return ValidationResult.Error($"File not found {settings.InputFile}.");
            }

            if (settings.NewGroupIndex.HasValue)
            {
                byte newGroup = (byte)settings.NewGroupIndex.GetValueOrDefault(0);

                if (!MidiGroup.IsValidIndex(newGroup))
                {
                    return ValidationResult.Error(Strings.ValidationErrorInvalidGroup);
                }
            }

            return base.Validate(context, settings);
        }

        private bool ValidateMessage(UInt32[]? words)
        {
            if (words != null && words.Length > 0 && words.Length <= 4)
            {
                // allowed behavior is to cast the packet type to the word count
                return (bool)((int)MidiMessageHelper.GetPacketTypeFromMessageFirstWord(words[0]) == words.Length);
            }
            else
            {
                return false;
            }
        }

        public override int Execute(CommandContext context, Settings settings)
        {
            //if (!MidiServicesInitializer.EnsureServiceAvailable())
            //{
            //    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatError("MIDI Service is not available."));
            //    return (int)MidiConsoleReturnCode.ErrorServiceNotAvailable;
            //}


            string endpointId = string.Empty;

            if (!string.IsNullOrEmpty(settings.EndpointDeviceId))
            {
                endpointId = settings.EndpointDeviceId.Trim();
            }
            else
            {
                endpointId = UmpEndpointPicker.PickEndpoint();
            }

            if (!string.IsNullOrEmpty(endpointId))
            {
                // TODO: Update loc strings
                string endpointName = EndpointUtility.GetEndpointNameFromEndpointInterfaceId(endpointId);

                AnsiConsole.Markup(Strings.SendMessageSendingThroughEndpointLabel);
                AnsiConsole.MarkupLine(" " + AnsiMarkupFormatter.FormatEndpointName(endpointName));
                AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatFullEndpointInterfaceId(endpointId));
                AnsiConsole.WriteLine();
                AnsiConsole.MarkupLine("Only error lines will be displayed when sending messages.");
                AnsiConsole.WriteLine();


                using var session = MidiSession.Create($"{Strings.AppShortName} - {Strings.SendMessageSessionNameSuffix}");
                if (session == null)
                {
                    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatError(Strings.ErrorUnableToCreateSession));
                    return (int)MidiConsoleReturnCode.ErrorCreatingSession;
                }

                var connection = session.CreateEndpointConnection(endpointId);
                if (connection == null)
                {
                    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatError(Strings.ErrorUnableToCreateEndpointConnection));

                    return (int)MidiConsoleReturnCode.ErrorCreatingEndpointConnection;
                }

                bool openSuccess = openSuccess = connection.Open(); ;
                if (!openSuccess)
                {
                    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatError(Strings.ErrorUnableToOpenEndpoint));

                    return (int)MidiConsoleReturnCode.ErrorOpeningEndpointConnection;
                }

                // TODO: Consider creating a message sender thread worker object that is shared
                // between this and the send-message command

                // if not verbose, just show a status spinner
                //AnsiConsole.Progress()
                //    .Start(ctx =>
                //    {
                //        //if (settings.DelayBetweenMessages == 0 && (settings.Count * (settings.Words!.Length + 2)) > bufferWarningThreshold)
                //        //{
                //        //    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatWarning(Strings.SendMessageFloodWarning));
                //        //    AnsiConsole.WriteLine();
                //        //}

                //        var sendTask = ctx.AddTask("[white]Sending messages[/]");
                //        sendTask.MaxValue = settings.Count;
                //        sendTask.Value = 0;

                //        messageSenderThread.Start();


                //        AnsiConsole.MarkupLine(Strings.SendMessagePressEscapeToStopSendingMessage);
                //        AnsiConsole.WriteLine();
                //        while (stillSending)
                //        {
                //            // check for input

                //            if (Console.KeyAvailable)
                //            {
                //                var keyInfo = Console.ReadKey(true);

                //                if (keyInfo.Key == ConsoleKey.Escape)
                //                {
                //                    stillSending = false;

                //                    // wake up the threads so they terminate
                //                    m_messageDispatcherThreadWakeup.Set();

                //                    AnsiConsole.WriteLine();
                //                    AnsiConsole.MarkupLine("🛑 " + Strings.SendMessageEscapePressedMessage);
                //                }

                //            }

                //            sendTask.Value = messagesSent;
                //            ctx.Refresh();

                //            if (stillSending) Thread.Sleep(100);
                //        }

                //        sendTask.Value = messagesSent;

                //    });


                // if verbose, spin up a live table
                var table = new Table();

                UInt32 countSkippedLines = 0;
                UInt32 countFailedLines = 0;
                UInt32 countMessagesSent = 0;

                AnsiConsole.Live(table)
                    .Start(ctx =>
                    {
                        // TODO: Localize these
                        table.AddColumn("Line");
                        table.AddColumn("Timestamp");          // a file with a timestamp isn't useful, really. But we could support an offset like +value
                        table.AddColumn("Sent Data");
                        table.AddColumn("Message Type");
                        table.AddColumn("Specific Type");

                        ctx.Refresh();
                        //AnsiConsole.WriteLine("Created table");

                        // get starting timestamp for any offset
                        var startingTimestamp = MidiClock.Now;

                        // open our data file

                        var fileStream = System.IO.File.OpenText(settings.InputFile!);

                        char delimiter = (char)0;

                        switch (settings.FieldDelimiter)
                        {
                            case ParseFieldDelimiter.Auto:
                                // we'll evaluate on each line
                                break;
                            case ParseFieldDelimiter.Space:
                                delimiter = ' ';
                                break;
                            case ParseFieldDelimiter.Comma:
                                delimiter = ',';
                                break;
                            case ParseFieldDelimiter.Pipe:
                                delimiter = '|';
                                break;
                            case ParseFieldDelimiter.Tab:
                                delimiter = '\t';
                                break;
                        }

                        bool changeGroup = settings.NewGroupIndex.HasValue;
                        var newGroup = new MidiGroup((byte)settings.NewGroupIndex.GetValueOrDefault(0));


                        if (fileStream != null)
                        {
                            uint lineNumber = 0;        // if someone has a file with more than uint.MaxValue / 4.3 billion lines, we'll overflow :)

                            string? line = string.Empty;

                            while (!fileStream.EndOfStream && line != null)
                            {
                                line = fileStream.ReadLine();

                                if (line == null)
                                {
                                    countSkippedLines++;
                                    continue;
                                }

                                lineNumber++;

                                // skip over comments and white space
                                if (LineIsIgnorable(line))
                                {
                                    countSkippedLines++;
                                    continue;
                                }

                                // if we're using Auto for the delimiter, each line is evaluated in case the file is mixed
                                // if you don't want to take this hit, specify the delimiter on the command line
                                if (settings.FieldDelimiter == ParseFieldDelimiter.Auto)
                                    delimiter = IdentifyFieldDelimiter(line);

                                UInt32[]? words;

                                // ignore files with timestamps for this first version

                                if (ParseNextDataLine(line, delimiter, (int)settings.WordDataFormat, out words))
                                {
                                    if (words != null && ValidateMessage(words))
                                    {
                                        var timestamp = MidiClock.Now;

                                        if (changeGroup)
                                        {
                                            if (MidiMessageHelper.MessageTypeHasGroupField(MidiMessageHelper.GetMessageTypeFromMessageFirstWord(words[0])))
                                            {
                                                words[0] = MidiMessageHelper.ReplaceGroupInMessageFirstWord(words[0], newGroup);
                                            }
                                        }

                                        // send the message
                                        if (MidiEndpointConnection.SendMessageSucceeded(connection.SendSingleMessageWordArray(timestamp, 0, (byte)words.Count(), words)))
                                        {
                                            countMessagesSent++;
                                        }
                                        else
                                        {
                                            countFailedLines++;
                                        }

                                        
                                        string detailedMessageType = MidiMessageHelper.GetMessageDisplayNameFromFirstWord(words[0]);

#if false
                                        // display the sent data
                                        table.AddRow(
                                            AnsiMarkupFormatter.FormatGeneralNumber(lineNumber),
                                            AnsiMarkupFormatter.FormatTimestamp(timestamp),
                                            AnsiMarkupFormatter.FormatMidiWords(words),
                                            AnsiMarkupFormatter.FormatMessageType(MidiMessageUtility.GetMessageTypeFromMessageFirstWord(words[0])),
                                            AnsiMarkupFormatter.FormatDetailedMessageType(MidiMessageUtility.GetMessageFriendlyNameFromFirstWord(words[0]))
                                            );

                                        ctx.Refresh();
#endif


                                    }
                                    else
                                    {
                                        countFailedLines++;

                                        // invalid UMP
                                        table.AddRow(
                                            AnsiMarkupFormatter.FormatGeneralNumber(lineNumber),
                                            "",
                                            AnsiMarkupFormatter.FormatError("Line does not contain a valid UMP") + "\n\"" + line + "\"",
                                            ""
                                            );

                                        ctx.Refresh();
                                    }

                                    if (settings.DelayBetweenMessages > 0)
                                    {
                                        Thread.Sleep(settings.DelayBetweenMessages);
                                    }
                                }
                                else
                                {
                                    // report line number and that it is an error
                                    table.AddRow(
                                        AnsiMarkupFormatter.FormatGeneralNumber(lineNumber),
                                        "",
                                        AnsiMarkupFormatter.FormatError("Unable to parse MIDI words from line") + "\n\"" + line + "\"",
                                        ""
                                        );

                                    ctx.Refresh();
                                    Thread.Sleep(0);
                                }
                            }
                        }
                        else
                        {
                            // file stream is null
                            AnsiConsole.WriteLine(AnsiMarkupFormatter.FormatError("Unable to open file. File stream is null"));
                        }
                    });


                if (countMessagesSent > 0) 
                {
                    AnsiConsole.WriteLine($"{countMessagesSent.ToString("N0")} message(s) sent.");
                }

                if (countSkippedLines > 0)
                {
                    AnsiConsole.WriteLine($"{countSkippedLines.ToString("N0")} lines skipped (empty or comments).");
                }

                if (countFailedLines > 0)
                {
                    AnsiConsole.WriteLine(AnsiMarkupFormatter.FormatError($"{countFailedLines.ToString("N0")} lines with errors."));
                }


                if (session != null)
                    session.Dispose();

            }


            return (int)MidiConsoleReturnCode.Success;
        }


        // true if the line is a comment or is white space
        private bool LineIsIgnorable(string inputLine)
        {
            // empty line
            if (string.IsNullOrEmpty(inputLine))
                return true;

            // comment
            if (inputLine.Trim().StartsWith("#"))
                return true;

            return false;
        }

        private char IdentifyFieldDelimiter(string line)
        {
            line = line.Trim();

            if (line.Contains(","))
                return ',';

            if (line.Contains("|"))
                return '|';

            if (line.Contains("\t"))
                return '\t';

            // some people use ", " between words instead of just ",". We need to be
            // sure to allow for that and related so we check this last
            if (line.Contains(" "))
                return ' ';

            return (char)0;
        }

        // returns true if it could parse the line, false if not.
        //private bool ParseNextDataLine(string inputLine, char delimiter, out bool timestampIsOffset, out UInt64 timestamp, out UInt32[] words)
        //{

        //}

        private bool ParseNextDataLine(string inputLine, char delimiter, int fromBase, out UInt32[]? words)
        {
            try
            {
                // in the case of Auto, no delimiter can be found if the line just has one entry
                if (delimiter == (char)0)
                {
                    words = new UInt32[1];

                    words[0] = ParseMidiWord(inputLine.Trim(), fromBase); 

                    return true;
                }
                else
                {
                    var strings = inputLine.Split(delimiter);

                    if (strings == null)
                    {
                        words = null;
                        return false;
                    }

                    words = new UInt32[strings.Length];

                    for (int i = 0; i < strings.Length; i++)
                    {
                        // TODO: Use the word data format to convert to base 10

                        words[i] = ParseMidiWord(strings[i].Trim(), fromBase);
                    }

                    return true;
                }
            }
            catch (Exception) 
            {
                //AnsiConsole.WriteException(ex);

                words = null;
                return false;
            }
        }

        private UInt32 ParseMidiWord(string midiWord, int fromBase)
        {
            // support an "h" suffix for hex
            if (fromBase == 16 && midiWord.ToLower().EndsWith("h"))
            {
                midiWord = midiWord.Substring(0, midiWord.Length - 1);
            }
            else if (fromBase == 10 && midiWord.ToLower().EndsWith("d"))
            {
                midiWord = midiWord.Substring(0, midiWord.Length - 1);
            }
            else if (fromBase == 2 && midiWord.ToLower().EndsWith("b"))
            {
                midiWord = midiWord.Substring(0, midiWord.Length - 1);
            }


            return Convert.ToUInt32(midiWord, fromBase);
        }



    }
}
