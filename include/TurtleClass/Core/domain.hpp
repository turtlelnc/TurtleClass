#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace turtleclass::core {

template <typename Tag> class StrongId {
public:
    StrongId() = default;
    explicit StrongId(std::string value);
    [[nodiscard]] const std::string& value() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    friend auto operator<=>(const StrongId&, const StrongId&) = default;
private:
    std::string value_;
};

struct StudentTag; struct DeviceTag; struct EventTag; struct EventGroupTag; struct RuleSetTag;
using StudentId = StrongId<StudentTag>;
using DeviceId = StrongId<DeviceTag>;
using EventId = StrongId<EventTag>;
using EventGroupId = StrongId<EventGroupTag>;
using RuleSetId = StrongId<RuleSetTag>;

struct LevelRule { std::string name; int points_to_next = 0; int badges_on_enter = 0; };
struct RuleSet { RuleSetId id; int version = 1; std::vector<LevelRule> levels; static RuleSet default_rules(); };
struct StudentState { int level_index = 0; int points_in_level = 0; int badges = 0; bool frozen = false; };

enum class EventType { PointsAdjusted, BadgeAdjusted, Compensation };
struct DomainEvent {
    EventId event_id;
    EventGroupId event_group_id;
    StudentId target_id;
    DeviceId device_id;
    std::int64_t device_local_sequence = 0;
    EventType event_type = EventType::PointsAdjusted;
    int points_delta = 0;
    int badge_delta = 0;
    std::optional<EventId> compensates_event_id;
};
struct EventGroup { EventGroupId group_id; std::vector<DomainEvent> events; };

class InMemoryEventStore {
public:
    bool append_group(const EventGroup& group);
    [[nodiscard]] const std::vector<DomainEvent>& events() const noexcept;
private:
    std::vector<DomainEvent> events_;
    std::set<EventId> event_ids_;
};

class StateProjector {
public:
    explicit StateProjector(RuleSet rules);
    [[nodiscard]] std::map<StudentId, StudentState> replay(const std::vector<DomainEvent>& events) const;
    [[nodiscard]] StudentState apply(const StudentState& before, const DomainEvent& event) const;
private:
    RuleSet rules_;
    void apply_points(StudentState& state, int delta) const;
};

class DomainService {
public:
    explicit DomainService(InMemoryEventStore& store);
    bool commit(const EventGroup& group);
    EventGroup make_compensation_group(EventGroupId group_id, const EventGroup& original, DeviceId device, std::int64_t sequence_start) const;
private:
    InMemoryEventStore& store_;
};

struct ConsistencyReport { bool ok = true; std::vector<std::string> errors; };
class ConsistencyChecker {
public:
    [[nodiscard]] ConsistencyReport check(const std::vector<DomainEvent>& events, const RuleSet& rules) const;
};

extern template class StrongId<StudentTag>;
extern template class StrongId<DeviceTag>;
extern template class StrongId<EventTag>;
extern template class StrongId<EventGroupTag>;
extern template class StrongId<RuleSetTag>;

} // namespace turtleclass::core
