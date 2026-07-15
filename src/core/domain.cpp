#include "TurtleClass/Core/domain.hpp"

#include <algorithm>
#include <stdexcept>

namespace turtleclass::core {

template <typename Tag>
StrongId<Tag>::StrongId(std::string value) : value_(std::move(value)) {}
template <typename Tag> const std::string& StrongId<Tag>::value() const noexcept { return value_; }
template <typename Tag> bool StrongId<Tag>::empty() const noexcept { return value_.empty(); }
template class StrongId<StudentTag>; template class StrongId<DeviceTag>; template class StrongId<EventTag>; template class StrongId<EventGroupTag>; template class StrongId<RuleSetTag>;

RuleSet RuleSet::default_rules() {
    return {RuleSetId{"default"}, 1, {{"Level 1", 10, 0}, {"Level 2", 20, 1}, {"Level 3", 30, 1}, {"Level 4", 40, 2}, {"Level 5", 50, 2}, {"Level 6", 60, 3}, {"Level 7", 70, 3}, {"Level 8", 0, 4}}};
}

bool InMemoryEventStore::append_group(const EventGroup& group) {
    if (group.events.empty() || group.group_id.empty()) return false;
    std::set<EventId> incoming;
    for (const auto& event : group.events) {
        if (event.event_id.empty() || event.target_id.empty() || event.device_id.empty() || event.event_group_id.empty() || event.event_group_id != group.group_id) return false;
        if (event_ids_.contains(event.event_id) || !incoming.insert(event.event_id).second) return false;
    }
    for (const auto& event : group.events) { event_ids_.insert(event.event_id); events_.push_back(event); }
    return true;
}
const std::vector<DomainEvent>& InMemoryEventStore::events() const noexcept { return events_; }

StateProjector::StateProjector(RuleSet rules) : rules_(std::move(rules)) { if (rules_.levels.empty()) throw std::invalid_argument("rules require at least one level"); }
std::map<StudentId, StudentState> StateProjector::replay(const std::vector<DomainEvent>& events) const {
    std::map<StudentId, StudentState> states;
    for (const auto& event : events) states[event.target_id] = apply(states[event.target_id], event);
    return states;
}
StudentState StateProjector::apply(const StudentState& before, const DomainEvent& event) const {
    auto next = before;
    if (event.event_type == EventType::PointsAdjusted || event.event_type == EventType::Compensation) apply_points(next, event.points_delta);
    next.badges += event.badge_delta;
    return next;
}
void StateProjector::apply_points(StudentState& state, int delta) const {
    state.points_in_level += delta;
    while (state.level_index + 1 < static_cast<int>(rules_.levels.size()) && rules_.levels[state.level_index].points_to_next > 0 && state.points_in_level >= rules_.levels[state.level_index].points_to_next) {
        state.points_in_level -= rules_.levels[state.level_index].points_to_next;
        ++state.level_index;
        state.badges += rules_.levels[state.level_index].badges_on_enter;
    }
    while (state.level_index > 0 && state.points_in_level < 0) {
        state.badges -= rules_.levels[state.level_index].badges_on_enter;
        --state.level_index;
        state.points_in_level += rules_.levels[state.level_index].points_to_next;
    }
}

DomainService::DomainService(InMemoryEventStore& store) : store_(store) {}
bool DomainService::commit(const EventGroup& group) { return store_.append_group(group); }
EventGroup DomainService::make_compensation_group(EventGroupId group_id, const EventGroup& original, DeviceId device, std::int64_t sequence_start) const {
    EventGroup result{group_id, {}};
    for (auto it = original.events.rbegin(); it != original.events.rend(); ++it) {
        DomainEvent event = *it;
        event.event_id = EventId{group_id.value() + ":undo:" + event.event_id.value()};
        event.event_group_id = group_id;
        event.device_id = device;
        event.device_local_sequence = sequence_start++;
        event.event_type = EventType::Compensation;
        event.points_delta = -event.points_delta;
        event.badge_delta = -event.badge_delta;
        event.compensates_event_id = it->event_id;
        result.events.push_back(event);
    }
    return result;
}

ConsistencyReport ConsistencyChecker::check(const std::vector<DomainEvent>& events, const RuleSet& rules) const {
    ConsistencyReport report;
    std::set<EventId> ids;
    for (const auto& event : events) {
        if (event.event_id.empty() || !ids.insert(event.event_id).second) { report.ok = false; report.errors.push_back("duplicate or empty event id"); }
        if (event.target_id.empty()) { report.ok = false; report.errors.push_back("empty target id"); }
    }
    try { (void)StateProjector{rules}.replay(events); } catch (const std::exception& ex) { report.ok = false; report.errors.push_back(ex.what()); }
    return report;
}

} // namespace turtleclass::core
