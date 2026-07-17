#pragma once

#include <string>
#include <vector>

namespace turtleclass::windows_desktop {

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
    void load_design_time_students();
    [[nodiscard]] const std::vector<StudentDisplayState>& students() const noexcept;
    [[nodiscard]] QueueCommandResult queue_temporary_points(std::size_t selected_index, int points_delta) const;

private:
    std::vector<StudentDisplayState> students_;
};

} // namespace turtleclass::windows_desktop
