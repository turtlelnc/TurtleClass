using Microsoft.UI.Xaml;

namespace TurtleClassWinUI;

public class App : Application
{
    private Window? m_window;

    public App()
    {
        this.InitializeComponent();
    }

    protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
    {
        m_window = new MainWindow();
        m_window.Activate();
    }

    private void InitializeComponent()
    {
        // WinUI 3 initialization
    }
}
