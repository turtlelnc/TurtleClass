#pragma once

#include "MainWindow.g.h"
#include "StudentListViewModel.hpp"

namespace winrt::TurtleClass::WinUI::implementation {
struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
    void QueuePointsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
private:
    StudentListViewModel view_model_;
};
}
