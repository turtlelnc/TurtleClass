using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Threading.Tasks;

namespace TurtleClassWinUI;

public sealed partial class LoginPage : Page
{
    private readonly LoginViewModel _viewModel;

    public LoginPage()
    {
        this.InitializeComponent();
        _viewModel = new LoginViewModel();
        this.DataContext = _viewModel;
    }

    private void InitializeComponent()
    {
        var grid = new Grid();
        grid.Padding = new Thickness(40);
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto }); // Title
        grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) }); // Form
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto }); // Status

        // Title
        var titleText = new TextBlock
        {
            Text = "TurtleClass 登录",
            FontSize = 32,
            FontWeight = Microsoft.UI.Text.FontWeights.SemiBold,
            Margin = new Thickness(0, 0, 0, 40),
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Center
        };
        Grid.SetRow(titleText, 0);
        grid.Children.Add(titleText);

        // Form container
        var formStack = new StackPanel { Spacing = 16, MaxWidth = 400 };
        Grid.SetRow(formStack, 1);

        // Class Code
        var classCodeLabel = new TextBlock { Text = "班级代码", Margin = new Thickness(0, 0, 0, 8) };
        var classCodeBox = new PasswordBox 
        { 
            PlaceholderText = "请输入班级代码"
        };
        classCodeBox.PasswordChanged += (s, e) => _viewModel.ClassCode = classCodeBox.Password;
        formStack.Children.Add(classCodeLabel);
        formStack.Children.Add(classCodeBox);

        // Student ID
        var studentIdLabel = new TextBlock { Text = "学号", Margin = new Thickness(0, 0, 0, 8) };
        var studentIdBox = new TextBox { PlaceholderText = "请输入学号" };
        studentIdBox.TextChanged += (s, e) => _viewModel.StudentId = studentIdBox.Text;
        formStack.Children.Add(studentIdLabel);
        formStack.Children.Add(studentIdBox);

        // Password
        var passwordLabel = new TextBlock { Text = "密码", Margin = new Thickness(0, 0, 0, 8) };
        var passwordBox = new PasswordBox { PlaceholderText = "请输入密码" };
        passwordBox.PasswordChanged += (s, e) => _viewModel.Password = passwordBox.Password;
        formStack.Children.Add(passwordLabel);
        formStack.Children.Add(passwordBox);

        // Login button
        var loginButton = new Button 
        { 
            Content = "登录", 
            Width = double.NaN, 
            Height = 44,
            Margin = new Thickness(0, 20, 0, 0)
        };
        loginButton.Click += async (s, e) => await _viewModel.LoginCommand.ExecuteAsync(null);
        loginButton.IsEnabled = false;
        
        var loginBinding = new Binding
        {
            Source = _viewModel,
            Path = new PropertyPath(nameof(LoginViewModel.CanLogin)),
            Mode = BindingMode.OneWay
        };
        loginButton.SetBinding(Button.IsEnabledProperty, loginBinding);
        
        formStack.Children.Add(loginButton);

        // Status message
        var statusText = new TextBlock 
        { 
            Margin = new Thickness(0, 20, 0, 0),
            TextWrapping = TextWrapping.Wrap,
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Center
        };
        
        var statusBinding = new Binding
        {
            Source = _viewModel,
            Path = new PropertyPath(nameof(LoginViewModel.StatusMessage)),
            Mode = BindingMode.OneWay
        };
        statusText.SetBinding(TextBlock.TextProperty, statusBinding);
        
        Grid.SetRow(statusText, 2);
        grid.Children.Add(statusText);

        grid.Children.Add(formStack);
        this.Content = grid;
    }
}

public partial class LoginViewModel : ObservableObject
{
    private readonly AuthenticationService _authService;

    [ObservableProperty]
    private string _classCode = string.Empty;

    [ObservableProperty]
    private string _studentId = string.Empty;

    [ObservableProperty]
    private string _password = string.Empty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    [ObservableProperty]
    private bool _canLogin;

    [ObservableProperty]
    private bool _isLoading;

    public LoginViewModel()
    {
        _authService = new AuthenticationService();
        
        // Validate inputs to enable login button
        PropertyChanged += (s, e) =>
        {
            CanLogin = !string.IsNullOrEmpty(ClassCode) && 
                      !string.IsNullOrEmpty(StudentId) && 
                      !string.IsNullOrEmpty(Password) &&
                      !IsLoading;
        };
    }

    [RelayCommand]
    private async Task Login()
    {
        try
        {
            IsLoading = true;
            StatusMessage = "正在登录...";

            var success = await _authService.LoginAsync(ClassCode, StudentId, Password);

            if (success)
            {
                StatusMessage = "登录成功！";
                // AuthService will trigger navigation via event
            }
            else
            {
                StatusMessage = "登录失败：班级代码、学号或密码错误";
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"登录出错：{ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }
}
