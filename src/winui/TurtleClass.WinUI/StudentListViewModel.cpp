#include "pch.h"
#include "StudentListViewModel.hpp"

namespace winrt::TurtleClass::WinUI::implementation {

void StudentListViewModel::LoadDesignTimeStudents() {
    students_ = {
        {L"student-001", L"Student 001", 0, 0, 0},
        {L"student-002", L"Student 002", 0, 0, 0},
    };
}

const std::vector<StudentDisplayState>& StudentListViewModel::Students() const noexcept { return students_; }

QueueCommandResult StudentListViewModel::QueueTemporaryPoints(int points_delta) {
    if (students_.empty()) return {false, L"No selected student yet."};
    return {true, L"Command intent queued locally; Core/Sync integration will create the immutable event."};
}

} // namespace winrt::TurtleClass::WinUI::implementation
