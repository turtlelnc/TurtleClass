#include "TurtleClass/Core/domain.hpp"

#include <stdexcept>

namespace turtleclass::core {

template <typename Tag>
StrongId<Tag>::StrongId(std::string value) : value_(std::move(value)) {}
template <typename Tag> const std::string& StrongId<Tag>::value() const noexcept { return value_; }
template <typename Tag> bool StrongId<Tag>::empty() const noexcept { return value_.empty(); }
template class StrongId<ClassTag>; template class StrongId<StudentTag>; template class StrongId<DeviceTag>; template class StrongId<EventTag>; template class StrongId<EventGroupTag>; template class StrongId<RuleSetTag>; template class StrongId<AccountTag>; template class StrongId<AdminTag>;

RuleSet RuleSet::default_rules() {
    return {RuleSetId{"default"}, 1, {{"Level 1", 10, 0}, {"Level 2", 20, 1}, {"Level 3", 30, 1}, {"Level 4", 40, 2}, {"Level 5", 50, 2}, {"Level 6", 60, 3}, {"Level 7", 70, 3}, {"Level 8", 0, 4}}};
}

std::vector<std::string> RuleSet::validate() const {
    std::vector<std::string> errors;
    if (id.empty()) errors.push_back("rule set id is required");
    if (version <= 0) errors.push_back("rule version must be positive");
    if (levels.empty()) errors.push_back("at least one level is required");
    for (std::size_t i = 0; i < levels.size(); ++i) {
        if (levels[i].name.empty()) errors.push_back("level name is required");
        if (levels[i].points_to_next < 0) errors.push_back("level threshold cannot be negative");
        if (i + 1 < levels.size() && levels[i].points_to_next == 0) errors.push_back("non-final level threshold must be positive");
    }
    return errors;
}

AppendResult InMemoryEventStore::append_group(const EventGroup& group) {
    AppendResult result;
    if (group.group_id.empty()) result.errors.push_back("group id is required");
    if (group.class_id.empty()) result.errors.push_back("class id is required");
    if (group.events.empty()) result.errors.push_back("event group cannot be empty");

    std::set<EventId> incoming;
    for (const auto& event : group.events) {
        if (event.event_id.empty()) result.errors.push_back("event id is required");
        if (event.class_id.empty() || event.class_id != group.class_id) result.errors.push_back("event class id must match group class id");
        if (event.target_id.empty()) result.errors.push_back("target student id is required");
        if (event.device_id.empty()) result.errors.push_back("device id is required");
        if (event.event_group_id.empty() || event.event_group_id != group.group_id) result.errors.push_back("event group id must match group id");
        if (event.device_local_sequence <= 0) result.errors.push_back("device local sequence must be positive");
        if (event.rule_version <= 0) result.errors.push_back("rule version must be positive");
        if (!event.event_id.empty() && event_ids_.contains(event.event_id)) result.errors.push_back("duplicate committed event id");
        if (!event.event_id.empty() && !incoming.insert(event.event_id).second) result.errors.push_back("duplicate event id inside group");
    }
    if (!result.errors.empty()) return result;
    for (const auto& event : group.events) { event_ids_.insert(event.event_id); events_.push_back(event); }
    result.ok = true;
    return result;
}
const std::vector<DomainEvent>& InMemoryEventStore::events() const noexcept { return events_; }

StateProjector::StateProjector(RuleSet rules) : rules_(std::move(rules)) {
    auto errors = rules_.validate();
    if (!errors.empty()) throw std::invalid_argument(errors.front());
}
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
    while (state.level_index + 1 < static_cast<int>(rules_.levels.size()) && state.points_in_level >= rules_.levels[state.level_index].points_to_next) {
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
AppendResult DomainService::commit(const EventGroup& group) { return store_.append_group(group); }
EventGroup DomainService::make_compensation_group(EventGroupId group_id, const EventGroup& original, DeviceId device, std::int64_t sequence_start) const {
    EventGroup result{group_id, original.class_id, {}};
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
    for (const auto& error : rules.validate()) { report.ok = false; report.errors.push_back(error); }
    std::set<EventId> ids;
    for (const auto& event : events) {
        if (event.event_id.empty() || !ids.insert(event.event_id).second) { report.ok = false; report.errors.push_back("duplicate or empty event id"); }
        if (event.class_id.empty()) { report.ok = false; report.errors.push_back("empty class id"); }
        if (event.target_id.empty()) { report.ok = false; report.errors.push_back("empty target id"); }
        if (event.device_id.empty()) { report.ok = false; report.errors.push_back("empty device id"); }
        if (event.device_local_sequence <= 0) { report.ok = false; report.errors.push_back("invalid device local sequence"); }
    }
    try { (void)StateProjector{rules}.replay(events); } catch (const std::exception& ex) { report.ok = false; report.errors.push_back(ex.what()); }
    return report;
}

} // namespace turtleclass::core
