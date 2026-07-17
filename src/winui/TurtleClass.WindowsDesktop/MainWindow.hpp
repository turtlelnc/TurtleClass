#pragma once

#include "StudentListViewModel.hpp"

#include <windows.h>

namespace turtleclass::windows_desktop {

class MainWindow {
public:
    bool create(HINSTANCE instance, int show_command);
    [[nodiscard]] HWND handle() const noexcept;

private:
    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);

    void create_child_controls();
    void populate_students();
    void queue_points_command();
    [[nodiscard]] int selected_student_index() const;
    [[nodiscard]] int requested_points_delta() const;
    void set_status(const std::wstring& message);

    HWND window_ = nullptr;
    HWND students_list_ = nullptr;
    HWND points_edit_ = nullptr;
    HWND queue_button_ = nullptr;
    HWND status_text_ = nullptr;
    StudentListViewModel view_model_;
};

} // namespace turtleclass::windows_desktop
