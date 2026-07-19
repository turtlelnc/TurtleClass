#include "MainWindow.hpp"
#include "resource.h"

#include <string>

namespace turtleclass::windows_desktop {
namespace {
constexpr wchar_t kWindowClassName[] = L"TurtleClassWindowsDesktopWindow";
}

bool MainWindow::create(HINSTANCE instance, int show_command) {
    view_model_.load_design_time_students();

    WNDCLASSW window_class{};
    window_class.lpfnWndProc = MainWindow::window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = kWindowClassName;
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&window_class);

    window_ = CreateWindowExW(
        0,
        kWindowClassName,
        L"TurtleClass",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        920,
        560,
        nullptr,
        nullptr,
        instance,
        this);

    if (window_ == nullptr) return false;
    ShowWindow(window_, show_command);
    UpdateWindow(window_);
    return true;
}

HWND MainWindow::handle() const noexcept { return window_; }

LRESULT CALLBACK MainWindow::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        auto* self = static_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->window_ = window;
    }

    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (self != nullptr) return self->handle_message(message, wparam, lparam);
    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT MainWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        create_child_controls();
        populate_students();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == IDC_QUEUE_BUTTON) queue_points_command();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window_, message, wparam, lparam);
    }
}

void MainWindow::create_child_controls() {
    CreateWindowW(L"STATIC", L"TurtleClass", WS_CHILD | WS_VISIBLE, 24, 18, 220, 32, window_, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Students", WS_CHILD | WS_VISIBLE, 24, 64, 160, 24, window_, nullptr, nullptr, nullptr);
    students_list_ = CreateWindowW(L"LISTBOX", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 24, 92, 360, 360, window_, reinterpret_cast<HMENU>(IDC_STUDENT_LIST), nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Quick points", WS_CHILD | WS_VISIBLE, 424, 64, 220, 24, window_, nullptr, nullptr, nullptr);
    points_edit_ = CreateWindowW(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 424, 96, 120, 28, window_, reinterpret_cast<HMENU>(IDC_POINTS_EDIT), nullptr, nullptr);
    queue_button_ = CreateWindowW(L"BUTTON", L"Queue offline event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 424, 140, 180, 32, window_, reinterpret_cast<HMENU>(IDC_QUEUE_BUTTON), nullptr, nullptr);
    status_text_ = CreateWindowW(L"STATIC", L"Offline queue ready. Events wait for sync protocol v1.", WS_CHILD | WS_VISIBLE, 424, 196, 430, 64, window_, reinterpret_cast<HMENU>(IDC_STATUS_TEXT), nullptr, nullptr);
}

void MainWindow::populate_students() {
    for (const auto& student : view_model_.students()) {
        const auto row = student.display_name + L"  |  L" + std::to_wstring(student.level_index + 1) + L"  |  " + std::to_wstring(student.points_in_level) + L" pts  |  " + std::to_wstring(student.badges) + L" badges";
        SendMessageW(students_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
}

void MainWindow::queue_points_command() {
    const auto result = view_model_.queue_temporary_points(static_cast<std::size_t>(selected_student_index()), requested_points_delta());
    set_status(result.message);
}

int MainWindow::selected_student_index() const {
    return static_cast<int>(SendMessageW(students_list_, LB_GETCURSEL, 0, 0));
}

int MainWindow::requested_points_delta() const {
    wchar_t buffer[32]{};
    GetWindowTextW(points_edit_, buffer, 32);
    return std::wcstol(buffer, nullptr, 10);
}

void MainWindow::set_status(const std::wstring& message) {
    SetWindowTextW(status_text_, message.c_str());
}

} // namespace turtleclass::windows_desktop
