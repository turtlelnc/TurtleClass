#include "MainWindow.hpp"
#include "resource.h"

#include <string>

namespace turtleclass::windows_desktop {
namespace {
constexpr wchar_t kWindowClassName[] = L"TurtleClassWindowsDesktopWindow";
}

bool MainWindow::create(HINSTANCE instance, int show_command) {
    // Set up login state change callback
    auth_service_.set_login_state_changed_callback([this](bool logged_in) {
        update_ui_for_login_state();
    });
    
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
        update_ui_for_login_state();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == IDC_QUEUE_BUTTON) queue_points_command();
        else if (LOWORD(wparam) == IDC_LOGIN_BUTTON) show_login_dialog();
        else if (LOWORD(wparam) == IDC_LOGOUT_BUTTON) on_logout();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window_, message, wparam, lparam);
    }
}

void MainWindow::create_child_controls() {
    // Title
    CreateWindowW(L"STATIC", L"TurtleClass", WS_CHILD | WS_VISIBLE, 24, 18, 220, 32, window_, nullptr, nullptr, nullptr);
    
    // Login section
    CreateWindowW(L"STATIC", L"Login", WS_CHILD | WS_VISIBLE, 24, 64, 160, 24, window_, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Class Code:", WS_CHILD | WS_VISIBLE, 24, 92, 80, 24, window_, nullptr, nullptr, nullptr);
    class_code_edit_ = CreateWindowW(L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 104, 92, 120, 28, window_, reinterpret_cast<HMENU>(IDC_CLASS_CODE_EDIT), nullptr, nullptr);
    
    CreateWindowW(L"STATIC", L"Student ID:", WS_CHILD | WS_VISIBLE, 24, 124, 80, 24, window_, nullptr, nullptr, nullptr);
    student_id_edit_ = CreateWindowW(L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 104, 124, 120, 28, window_, reinterpret_cast<HMENU>(IDC_STUDENT_ID_EDIT), nullptr, nullptr);
    
    CreateWindowW(L"STATIC", L"Password:", WS_CHILD | WS_VISIBLE, 24, 156, 80, 24, window_, nullptr, nullptr, nullptr);
    password_edit_ = CreateWindowW(L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 104, 156, 120, 28, window_, reinterpret_cast<HMENU>(IDC_PASSWORD_EDIT), nullptr, nullptr);
    
    login_button_ = CreateWindowW(L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 24, 196, 100, 32, window_, reinterpret_cast<HMENU>(IDC_LOGIN_BUTTON), nullptr, nullptr);
    logout_button_ = CreateWindowW(L"BUTTON", L"Logout", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 136, 196, 100, 32, window_, reinterpret_cast<HMENU>(IDC_LOGOUT_BUTTON), nullptr, nullptr);
    
    // Student list section
    CreateWindowW(L"STATIC", L"Students", WS_CHILD | WS_VISIBLE, 424, 64, 160, 24, window_, nullptr, nullptr, nullptr);
    students_list_ = CreateWindowW(L"LISTBOX", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 424, 92, 360, 360, window_, reinterpret_cast<HMENU>(IDC_STUDENT_LIST), nullptr, nullptr);

    // Quick points section
    CreateWindowW(L"STATIC", L"Quick points", WS_CHILD | WS_VISIBLE, 424, 464, 220, 24, window_, nullptr, nullptr, nullptr);
    points_edit_ = CreateWindowW(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 424, 496, 120, 28, window_, reinterpret_cast<HMENU>(IDC_POINTS_EDIT), nullptr, nullptr);
    queue_button_ = CreateWindowW(L"BUTTON", L"Queue offline event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 424, 540, 180, 32, window_, reinterpret_cast<HMENU>(IDC_QUEUE_BUTTON), nullptr, nullptr);
    status_text_ = CreateWindowW(L"STATIC", L"Not logged in. Please login to access class features.", WS_CHILD | WS_VISIBLE, 424, 584, 430, 64, window_, reinterpret_cast<HMENU>(IDC_STATUS_TEXT), nullptr, nullptr);
}

void MainWindow::populate_students() {
    if (!auth_service_.is_logged_in()) {
        SendMessageW(students_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Please login to see students"));
        return;
    }
    
    for (const auto& student : view_model_.students()) {
        const auto row = student.display_name + L"  |  L" + std::to_wstring(student.level_index + 1) + L"  |  " + std::to_wstring(student.points_in_level) + L" pts  |  " + std::to_wstring(student.badges) + L" badges";
        SendMessageW(students_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
}

void MainWindow::queue_points_command() {
    if (!auth_service_.is_logged_in()) {
        set_status(L"Must be logged in to queue commands.");
        return;
    }
    
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

void MainWindow::show_login_dialog() {
    // In a real implementation, this would show a modal dialog
    // For now, we'll just read from the inline edit controls and attempt login
    
    wchar_t class_code[64]{};
    wchar_t student_id[64]{};
    wchar_t password[64]{};
    
    GetWindowTextW(class_code_edit_, class_code, 64);
    GetWindowTextW(student_id_edit_, student_id, 64);
    GetWindowTextW(password_edit_, password, 64);
    
    LoginCredentials credentials{class_code, student_id, password};
    auto result = auth_service_.login(credentials);
    
    if (result.success) {
        current_login_ = std::move(result);
        on_login_success();
    } else {
        set_status(result.message);
    }
}

void MainWindow::on_login_success() {
    set_status(L"Login successful! Syncing student data...");
    
    // Clear the student list and repopulate
    SendMessageW(students_list_, LB_RESETCONTENT, 0, 0);
    populate_students();
    
    update_ui_for_login_state();
}

void MainWindow::on_logout() {
    auth_service_.logout();
    current_login_.reset();
    
    // Clear the student list
    SendMessageW(students_list_, LB_RESETCONTENT, 0, 0);
    SendMessageW(students_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Logged out. Please login again."));
    
    update_ui_for_login_state();
    set_status(L"Logged out successfully.");
}

void MainWindow::update_ui_for_login_state() {
    bool logged_in = auth_service_.is_logged_in();
    
    EnableWindow(login_button_, !logged_in);
    EnableWindow(logout_button_, logged_in);
    EnableWindow(class_code_edit_, !logged_in);
    EnableWindow(student_id_edit_, !logged_in);
    EnableWindow(password_edit_, !logged_in);
    EnableWindow(queue_button_, logged_in);
    EnableWindow(students_list_, logged_in);
    EnableWindow(points_edit_, logged_in);
    
    if (logged_in) {
        SetWindowTextW(status_text_, L"Connected. Events queued locally will sync when online.");
    } else {
        SetWindowTextW(status_text_, L"Not logged in. Please login to access class features.");
    }
}

} // namespace turtleclass::windows_desktop
