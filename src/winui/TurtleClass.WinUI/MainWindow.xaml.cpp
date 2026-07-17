#include "pch.h"
#include "MainWindow.xaml.h"

namespace winrt::TurtleClass::WinUI::implementation {
MainWindow::MainWindow() {
    InitializeComponent();
    view_model_.LoadDesignTimeStudents();
}

void MainWindow::QueuePointsButton_Click(winrt::Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
    const auto result = view_model_.QueueTemporaryPoints(static_cast<int>(PointsDeltaBox().Value()));
    SyncStatusBar().Title(result.accepted ? L"Queued locally" : L"Not queued");
    SyncStatusBar().Message(result.message);
}
}
