﻿using Microsoft.Midi.Settings.Contracts.Services;
using Microsoft.Midi.Settings.ViewModels;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.Midi.Settings.Views;

public sealed partial class WinMMMidi1DevicesPage : Page
{
    private ILoggingService _loggingService;

    public WinMMMidi1DevicesViewModel ViewModel
    {
        get;
    }

    public WinMMMidi1DevicesPage()
    {
        _loggingService = App.GetService<ILoggingService>();

        ViewModel = App.GetService<WinMMMidi1DevicesViewModel>();
        InitializeComponent();
    }
}