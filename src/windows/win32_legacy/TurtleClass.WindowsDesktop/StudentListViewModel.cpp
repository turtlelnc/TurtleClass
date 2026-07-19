#include "StudentListViewModel.hpp"

namespace turtleclass::windows_desktop {

void StudentListViewModel::load_design_time_students() {
    students_ = {
        {L"student-001", L"Student 001", 0, 0, 0},
        {L"student-002", L"Student 002", 0, 0, 0},
        {L"student-003", L"Student 003", 0, 0, 0},
    };
}

const std::vector<StudentDisplayState>& StudentListViewModel::students() const noexcept { return students_; }

QueueCommandResult StudentListViewModel::queue_temporary_points(std::size_t selected_index, int) const {
    if (selected_index >= students_.size()) return {false, L"Select a student before queueing a command."};
    return {true, L"Command intent queued locally; Core/Sync integration will create the immutable event."};
}

} // namespace turtleclass::windows_desktop
