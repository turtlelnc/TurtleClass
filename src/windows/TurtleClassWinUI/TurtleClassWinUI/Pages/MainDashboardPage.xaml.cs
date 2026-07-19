using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.Collections.ObjectModel;

namespace TurtleClassWinUI;

public sealed partial class MainDashboardPage : Page
{
    private readonly DashboardViewModel _viewModel;

    public MainDashboardPage()
    {
        this.InitializeComponent();
        _viewModel = new DashboardViewModel();
        this.DataContext = _viewModel;
    }

    private void InitializeComponent()
    {
        var grid = new Grid();
        grid.Padding = new Thickness(20);
        
        // Tab control for different views
        var tabView = new TabView();
        
        // Students tab
        var studentsTab = new TabViewItem 
        { 
            Header = "学生列表",
            Content = CreateStudentsView()
        };
        
        // Quick Actions tab
        var actionsTab = new TabViewItem
        {
            Header = "快速操作",
            Content = CreateQuickActionsView()
        };
        
        // Sync status tab
        var syncTab = new TabViewItem
        {
            Header = "同步状态",
            Content = CreateSyncStatusView()
        };
        
        tabView.TabItems.Add(studentsTab);
        tabView.TabItems.Add(actionsTab);
        tabView.TabItems.Add(syncTab);
        
        grid.Children.Add(tabView);
        this.Content = grid;
    }

    private UIElement CreateStudentsView()
    {
        var scrollViewer = new ScrollViewer();
        var stackPanel = new StackPanel { Spacing = 12 };

        // Search box
        var searchBox = new TextBox 
        { 
            PlaceholderText = "搜索学生...",
            Margin = new Thickness(0, 0, 0, 16)
        };
        searchBox.TextChanged += (s, e) => _viewModel.SearchQuery = searchBox.Text;
        stackPanel.Children.Add(searchBox);

        // Students list
        var listView = new ListView();
        
        var studentsBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.Students)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        listView.SetBinding(ListView.ItemsSourceProperty, studentsBinding);
        
        listView.SelectionChanged += (s, e) =>
        {
            if (listView.SelectedItem is StudentViewModel student)
            {
                _viewModel.SelectedStudent = student;
            }
        };

        stackPanel.Children.Add(listView);
        scrollViewer.Content = stackPanel;
        return scrollViewer;
    }

    private UIElement CreateQuickActionsView()
    {
        var scrollViewer = new ScrollViewer();
        var stackPanel = new StackPanel { Spacing = 16, MaxWidth = 500 };

        // Selected student info
        var studentInfoStack = new StackPanel { Spacing = 8 };
        
        var studentNameText = new TextBlock { FontSize = 20, FontWeight = Microsoft.UI.Text.FontWeights.SemiBold };
        var nameBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.SelectedStudentName)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        studentNameText.SetBinding(TextBlock.TextProperty, nameBinding);
        
        var studentPointsText = new TextBlock { FontSize = 16 };
        var pointsBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.SelectedStudentPoints)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        studentPointsText.SetBinding(TextBlock.TextProperty, pointsBinding);
        
        studentInfoStack.Children.Add(studentNameText);
        studentInfoStack.Children.Add(studentPointsText);
        stackPanel.Children.Add(studentInfoStack);

        // Points adjustment
        var pointsStack = new StackPanel { Spacing = 12, Margin = new Thickness(0, 20, 0, 0) };
        
        var pointsLabel = new TextBlock { Text = "调整分数", FontSize = 16 };
        var pointsInput = new NumberBox 
        { 
            Minimum = -100, 
            Maximum = 100,
            SpinButtonPlacementMode = Microsoft.UI.Xaml.Controls.NumberBoxSpinButtonPlacementMode.Inline,
            Width = 200,
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Left
        };
        
        var reasonBox = new TextBox 
        { 
            PlaceholderText = "原因（可选）",
            Width = 300,
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Left
        };
        
        var addPointsButton = new Button 
        { 
            Content = "+ 加分", 
            Width = 150,
            Margin = new Thickness(0, 10, 0, 0)
        };
        addPointsButton.Click += async (s, e) => 
        {
            var points = (int)pointsInput.Value;
            await _viewModel.AddPointsCommand.ExecuteAsync(new PointsAdjustment(points, reasonBox.Text));
        };
        
        var subtractPointsButton = new Button 
        { 
            Content = "- 减分", 
            Width = 150,
            Margin = new Thickness(10, 10, 0, 0)
        };
        subtractPointsButton.Click += async (s, e) => 
        {
            var points = -(int)Math.Abs(pointsInput.Value);
            await _viewModel.AddPointsCommand.ExecuteAsync(new PointsAdjustment(points, reasonBox.Text));
        };

        pointsStack.Children.Add(pointsLabel);
        pointsStack.Children.Add(pointsInput);
        pointsStack.Children.Add(reasonBox);
        
        var buttonsStack = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 10 };
        buttonsStack.Children.Add(addPointsButton);
        buttonsStack.Children.Add(subtractPointsButton);
        pointsStack.Children.Add(buttonsStack);
        
        stackPanel.Children.Add(pointsStack);

        scrollViewer.Content = stackPanel;
        return scrollViewer;
    }

    private UIElement CreateSyncStatusView()
    {
        var scrollViewer = new ScrollViewer();
        var stackPanel = new StackPanel { Spacing = 16 };

        // Sync status indicator
        var statusCircle = new Border
        {
            Width = 24,
            Height = 24,
            CornerRadius = new CornerRadius(12),
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Left
        };
        
        var statusColorBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.SyncStatusColor)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay,
            Converter = new StringToBrushConverter()
        };
        statusCircle.SetBinding(Border.BackgroundProperty, statusColorBinding);
        
        var statusText = new TextBlock { FontSize = 16 };
        var statusBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.SyncStatusText)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        statusText.SetBinding(TextBlock.TextProperty, statusBinding);
        
        var statusStack = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12 };
        statusStack.Children.Add(statusCircle);
        statusStack.Children.Add(statusText);
        stackPanel.Children.Add(statusStack);

        // Last sync time
        var lastSyncText = new TextBlock();
        var lastSyncBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.LastSyncTimeText)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        lastSyncText.SetBinding(TextBlock.TextProperty, lastSyncBinding);
        stackPanel.Children.Add(lastSyncText);

        // Manual sync button
        var syncButton = new Button 
        { 
            Content = "立即同步", 
            Width = 150,
            HorizontalAlignment = Microsoft.UI.Xaml.HorizontalAlignment.Left,
            Margin = new Thickness(0, 10, 0, 0)
        };
        syncButton.Click += async (s, e) => await _viewModel.SyncCommand.ExecuteAsync(null);
        stackPanel.Children.Add(syncButton);

        // Conflict warning
        var conflictWarning = new InfoBar
        {
            Title = "检测到冲突",
            Message = "请管理员解决数据冲突",
            Severity = Microsoft.UI.Xaml.Controls.InfoBarSeverity.Warning,
            IsOpen = false
        };
        
        var conflictBinding = new Microsoft.UI.Xaml.Data.Binding
        {
            Source = _viewModel,
            Path = new Microsoft.UI.Xaml.Data.PropertyPath(nameof(DashboardViewModel.HasConflicts)),
            Mode = Microsoft.UI.Xaml.Data.BindingMode.OneWay
        };
        conflictWarning.SetBinding(InfoBar.IsOpenProperty, conflictBinding);
        
        stackPanel.Children.Add(conflictWarning);

        scrollViewer.Content = stackPanel;
        return scrollViewer;
    }
}

public partial class DashboardViewModel : ObservableObject
{
    private readonly SyncService _syncService;

    [ObservableProperty]
    private ObservableCollection<StudentViewModel> _students = new();

    [ObservableProperty]
    private StudentViewModel? _selectedStudent;

    [ObservableProperty]
    private string _searchQuery = string.Empty;

    [ObservableProperty]
    private string _syncStatusText = "未同步";

    [ObservableProperty]
    private string _syncStatusColor = "Gray";

    [ObservableProperty]
    private string _lastSyncTimeText = "从未同步";

    [ObservableProperty]
    private bool _hasConflicts;

    public string SelectedStudentName => SelectedStudent?.Name ?? "未选择学生";
    public string SelectedStudentPoints => SelectedStudent?.Points.ToString("0") ?? "0";

    public DashboardViewModel()
    {
        _syncService = new SyncService(new AuthenticationService());
        _syncService.SyncStateChanged += OnSyncStateChanged;
        
        LoadStudents();
    }

    private void OnSyncStateChanged(object? sender, SyncState state)
    {
        if (state.IsSyncing)
        {
            SyncStatusText = "正在同步...";
            SyncStatusColor = "Orange";
        }
        else if (state.HasConflicts)
        {
            SyncStatusText = "存在冲突";
            SyncStatusColor = "Red";
            HasConflicts = true;
        }
        else if (state.LastSyncTime != default)
        {
            SyncStatusText = "已同步";
            SyncStatusColor = "Green";
            LastSyncTimeText = $"上次同步：{state.LastSyncTime.ToLocalTime():yyyy-MM-dd HH:mm:ss}";
        }
    }

    [RelayCommand]
    private async System.Threading.Tasks.Task AddPoints(PointsAdjustment adjustment)
    {
        if (SelectedStudent == null) return;

        var evt = new TurtleClassEvent
        {
            EventType = "points_adjustment",
            StudentId = SelectedStudent.StudentId,
            Points = adjustment.Points,
            Reason = adjustment.Reason,
            Timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
        };

        await _syncService.EnqueueEventAsync(evt);
        
        // Update local view
        SelectedStudent.Points += adjustment.Points;
        OnPropertyChanged(nameof(SelectedStudentPoints));
    }

    [RelayCommand]
    private async System.Threading.Tasks.Task Sync()
    {
        await _syncService.SyncAsync();
    }

    private void LoadStudents()
    {
        // Load students from server or local cache
        // For demo, add some sample data
        Students.Add(new StudentViewModel("001", "张三", 100));
        Students.Add(new StudentViewModel("002", "李四", 95));
        Students.Add(new StudentViewModel("003", "王五", 88));
    }
}

public class StudentViewModel
{
    public string StudentId { get; }
    public string Name { get; }
    public int Points { get; set; }

    public StudentViewModel(string studentId, string name, int points)
    {
        StudentId = studentId;
        Name = name;
        Points = points;
    }
}

public record PointsAdjustment(int Points, string Reason);

public class StringToBrushConverter : Microsoft.UI.Xaml.Data.IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (value is string colorName)
        {
            return new Microsoft.UI.Xaml.Media.SolidColorBrush(
                (Microsoft.UI.Color)Microsoft.UI.Xaml.Markup.XamlBindingHelper.ConvertValue(
                    typeof(Microsoft.UI.Color), colorName));
        }
        return new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Gray);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
        throw new NotImplementedException();
    }
}
