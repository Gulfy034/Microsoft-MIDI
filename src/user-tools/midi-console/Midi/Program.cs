﻿using Spectre.Console.Cli;
using Spectre.Console;
using System.Runtime.Versioning;

using Microsoft.Devices.Midi2.ConsoleApp;
using Microsoft.Devices.Midi2.ConsoleApp.Resources;


var app = new CommandApp();

app.Configure(config =>
{
    config.SetApplicationName("midi");

    config.AddCommand<EnumEndpointsCommand>("enumerate-endpoints")
        .WithAlias("enum-endpoints")
        .WithAlias("list-endpoints")
        .WithDescription(Strings.CommandEnumerateEndpointsDescription)
        .WithExample("enum-endpoints", "--direction all")
        ;

    config.AddCommand<EnumLegacyEndpointsCommand>("enumerate-legacy-endpoints")
        .WithAlias("enum-legacy-endpoints")
        .WithAlias("list-legacy-endpoints")
        .WithDescription(Strings.CommandEnumerateLegacyEndpointsDescription)
        .WithExample("enum-legacy-endpoints", "--direction all")
        ;

    config.AddCommand<CheckHealthCommand>("check-health")
        .WithAlias("test")
        .WithDescription(Strings.CommandCheckHealthDescription)
        .WithExample("test", "--loopback")
        ;

    config.AddCommand<ServicePingCommand>("service-ping")
        .WithAlias("ping")
        .WithDescription(Strings.CommandServicePingDescription)
        .WithExample("service-ping", "--count 10 --timeout 5000 --details")
        ;


    config.AddCommand<SendMessageCommand>("send-message")
        .WithAlias("send-ump")
        .WithExample("send-message", "-word 0x405F3AB7 -word 0x12345789")
        .WithDescription(Strings.CommandSendMessageDescription)
        ;

    config.AddCommand<SendMessagesFileCommand>("send-message-file")
        .WithAlias("send-ump-file")
        .WithAlias("send-file")
        .WithExample("send-message-file", "%USERPROFILE%\\Documents\\messages.txt")
        .WithDescription(Strings.CommandSendMessagesFileDescription)
        ;

    config.AddCommand<MonitorEndpointCommand>("monitor-endpoint")
        .WithAlias("monitor")
        .WithAlias("listen")
        .WithExample("monitor-endpoint", "--instance-id \\\\?\\SWD#MIDISRV#MIDIU_DEFAULT_LOOPBACK_IN#{ae174174-6396-4dee-ac9e-1e9c6f403230}")
        .WithDescription(Strings.CommandMonitorEndpointDescription)
        ;

    config.AddCommand<DiagnosticsReportCommand>("diagnostics-report")
        .WithAlias("report")
        .WithDescription(Strings.CommandDiagnosticsReportDescription)
        .WithExample("diagnostics-report", "--output %USERPROFILE%\\Documents\\report.txt")
        ;
});

// app title
AnsiConsole.WriteLine();
AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatAppTitle(Strings.AppTitle));
AnsiConsole.WriteLine();

if (args.Length == 0)
{
    // show app description only when no arguments supplied

    AnsiConsole.MarkupLine(AnsiMarkupFormatter.FormatAppDescription(Strings.AppDescription));
    AnsiConsole.WriteLine();
}

return app.Run(args);
