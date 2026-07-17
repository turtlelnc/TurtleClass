#pragma once

#include <string>
#include <vector>

namespace winrt::TurtleClass::WinUI::implementation {

struct StudentDisplayState {
    std::wstring student_id;
    std::wstring display_name;
    int level_index = 0;
    int points_in_level = 0;
    int badges = 0;
};

struct QueueCommandResult {
    bool accepted = false;
    std::wstring message;
};

class StudentListViewModel {
public:
    void LoadDesignTimeStudents();
    [[nodiscard]] const std::vector<StudentDisplayState>& Students() const noexcept;
    [[nodiscard]] QueueCommandResult QueueTemporaryPoints(int points_delta);

private:
    std::vector<StudentDisplayState> students_;
};

} // namespace winrt::TurtleClass::WinUI::implementation
