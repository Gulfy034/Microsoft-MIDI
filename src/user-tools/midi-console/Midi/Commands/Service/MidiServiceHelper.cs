﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of Windows MIDI Services and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================

using System.ServiceProcess;

namespace Microsoft.Midi.ConsoleApp
{
    internal class MidiServiceHelper
    {
        private const int MAX_TRIES = 50;
        private const int SLEEP_MS_PER_ATTEMPT = 250;

        public static string GetServiceName()
        {
            return "MidiSrv";
        }

        public static ServiceController GetServiceController()
        {
            return new System.ServiceProcess.ServiceController(GetServiceName());
        }

        public static bool ServiceIsStopped(ServiceController serviceController) 
        {
            return serviceController.Status == ServiceControllerStatus.Stopped || serviceController.Status == ServiceControllerStatus.StopPending;
        }

        public static bool ServiceIsRunning(ServiceController serviceController)
        {
            return serviceController.Status == ServiceControllerStatus.Running || serviceController.Status == ServiceControllerStatus.StartPending;
        }

        public static bool ServiceIsReallyRunning(ServiceController serviceController)
        {
            return serviceController.Status == ServiceControllerStatus.Running;
        }

        public static bool StopServiceWithConsoleStatusUpdate(ServiceController controller)
        {
            controller.Stop();

            int i = 0;
            while (i < MAX_TRIES && controller.Status != ServiceControllerStatus.Stopped)
            {
                Thread.Sleep(SLEEP_MS_PER_ATTEMPT);
                AnsiConsole.Write(".");
                i++;
                controller.Refresh();
            }
            AnsiConsole.WriteLine();

            return controller.Status == ServiceControllerStatus.Stopped;
        }

        public static bool StartServiceWithConsoleStatusUpdate(ServiceController controller)
        {
            controller.Start();

            int i = 0;
            while (i < MAX_TRIES && controller.Status != System.ServiceProcess.ServiceControllerStatus.Running)
            {
                Thread.Sleep(SLEEP_MS_PER_ATTEMPT);
                controller.Refresh();

                AnsiConsole.Write(".");
                i++;
            }
            AnsiConsole.WriteLine();

            return controller.Status == ServiceControllerStatus.Running;
        }

    }
}
