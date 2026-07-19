using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.Threading.Tasks;

namespace TurtleClassWinUI;

public sealed partial class MainWindow : Window
{
    private readonly AuthenticationService _authService;
    private readonly SyncService _syncService;

    public MainWindow()
    {
        this.InitializeComponent();
        
        _authService = new AuthenticationService();
        _syncService = new SyncService(_authService);
        
        // Subscribe to auth state changes
        _authService.LoginStateChanged += OnLoginStateChanged;
        
        // Show login page by default
        ContentFrame.Navigate(typeof(LoginPage));
    }

    private void OnLoginStateChanged(object? sender, bool isLoggedIn)
    {
        if (isLoggedIn)
        {
            // Navigate to main dashboard after successful login
            ContentFrame.Navigate(typeof(MainDashboardPage));
        }
        else
        {
            // Return to login page
            ContentFrame.Navigate(typeof(LoginPage));
        }
    }

    private void InitializeComponent()
    {
        // WinUI 3 window setup
        this.Title = "TurtleClass";
        
        var frame = new Frame();
        ContentFrame = frame;
        this.Content = frame;
    }

    public Frame ContentFrame { get; private set; } = null!;
}
