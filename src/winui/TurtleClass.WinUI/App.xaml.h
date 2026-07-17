#pragma once

#include "App.g.h"

namespace winrt::TurtleClass::WinUI::implementation {
struct App : AppT<App> {
    App();
    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);
private:
    winrt::Microsoft::UI::Xaml::Window window_{nullptr};
};
}
