#include "TurtleClass/Core/domain.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace turtleclass::core;

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " expected " #expr "\n"; return EXIT_FAILURE; } } while (false)

static DomainEvent points(std::string id, int delta, std::string student = "s1") {
    return {EventId{id}, ClassId{"class1"}, EventGroupId{"g" + id}, StudentId{student}, DeviceId{"d1"}, 1, EventType::PointsAdjusted, delta, 0, 1, "", std::nullopt};
}
static StudentState project(const RuleSet& rules, std::vector<DomainEvent> events) { return StateProjector{rules}.replay(events).at(StudentId{"s1"}); }

int run_server_tests();

int main() {
    auto rules = RuleSet::default_rules();
    REQUIRE(rules.levels.size() == 8);
    auto s = project(rules, {points("e1", 15)});
    REQUIRE(s.level_index == 1 && s.points_in_level == 5 && s.badges == 1);
    s = project(rules, {points("e2", 10), points("e3", -3)});
    REQUIRE(s.level_index == 0 && s.points_in_level == 7 && s.badges == 0);
    s = project(rules, {points("e4", 65)});
    REQUIRE(s.level_index == 3 && s.points_in_level == 5 && s.badges == 4);
    s = project(rules, {points("e5", 65), points("e6", -50)});
    REQUIRE(s.level_index == 1 && s.points_in_level == 5 && s.badges == 1);
    s = project(rules, {points("e7", -5)});
    REQUIRE(s.level_index == 0 && s.points_in_level == -5);
    s = project(rules, {points("e8", -5), points("e9", 8)});
    REQUIRE(s.level_index == 0 && s.points_in_level == 3);
    s = project(rules, {points("e10", 10), DomainEvent{EventId{"b1"}, ClassId{"class1"}, EventGroupId{"gb1"}, StudentId{"s1"}, DeviceId{"d1"}, 2, EventType::BadgeAdjusted, 0, -2, 1, "", std::nullopt}});
    REQUIRE(s.badges == -1);

    RuleSet changed = rules; changed.levels[0].points_to_next = 5;
    s = project(changed, {points("e11", 15)});
    REQUIRE(s.level_index == 1 && s.points_in_level == 10);
    RuleSet added = rules; added.levels.back().points_to_next = 80; added.levels.push_back({"Level 9", 0, 5});
    REQUIRE(project(added, {points("e12", 1000)}).level_index == 8);
    RuleSet removed = rules; removed.levels.pop_back();
    REQUIRE(project(removed, {points("e13", 1000)}).level_index == 6);

    InMemoryEventStore store;
    DomainService service{store};
    EventGroup group{EventGroupId{"grp"}, ClassId{"class1"}, {DomainEvent{EventId{"a"}, ClassId{"class1"}, EventGroupId{"grp"}, StudentId{"s1"}, DeviceId{"d1"}, 1, EventType::PointsAdjusted, 10, 0, 1, "", std::nullopt}, DomainEvent{EventId{"b"}, ClassId{"class1"}, EventGroupId{"grp"}, StudentId{"s1"}, DeviceId{"d1"}, 2, EventType::BadgeAdjusted, 0, 3, 1, "", std::nullopt}}};
    REQUIRE(service.commit(group));
    REQUIRE(!service.commit(group));
    auto undo = service.make_compensation_group(EventGroupId{"undo"}, group, DeviceId{"d1"}, 3);
    REQUIRE(service.commit(undo));
    auto states = StateProjector{rules}.replay(store.events());
    REQUIRE(states.at(StudentId{"s1"}).level_index == 0 && states.at(StudentId{"s1"}).points_in_level == 0 && states.at(StudentId{"s1"}).badges == 0);
    auto redo = service.make_compensation_group(EventGroupId{"redo"}, undo, DeviceId{"d1"}, 5);
    REQUIRE(service.commit(redo));
    states = StateProjector{rules}.replay(store.events());
    REQUIRE(states.at(StudentId{"s1"}).level_index == 1 && states.at(StudentId{"s1"}).badges == 4);

    EventGroup bad{EventGroupId{"bad"}, ClassId{"class1"}, {DomainEvent{EventId{"c"}, ClassId{"class1"}, EventGroupId{"bad"}, StudentId{"s1"}, DeviceId{"d1"}, 1, EventType::PointsAdjusted, 1, 0, 1, "", std::nullopt}, DomainEvent{EventId{"c"}, ClassId{"class1"}, EventGroupId{"bad"}, StudentId{"s1"}, DeviceId{"d1"}, 2, EventType::PointsAdjusted, 1, 0, 1, "", std::nullopt}}};
    auto before = store.events().size();
    REQUIRE(!service.commit(bad));
    REQUIRE(store.events().size() == before);
    REQUIRE(ConsistencyChecker{}.check(store.events(), rules).ok);
    auto dup = store.events(); dup.push_back(dup.front());
    REQUIRE(!ConsistencyChecker{}.check(dup, rules).ok);
    RuleSet invalid = rules; invalid.levels[0].points_to_next = 0;
    REQUIRE(!ConsistencyChecker{}.check(store.events(), invalid).ok);
    EventGroup wrong_class{EventGroupId{"wrong"}, ClassId{"class1"}, {DomainEvent{EventId{"wrong-event"}, ClassId{"class2"}, EventGroupId{"wrong"}, StudentId{"s1"}, DeviceId{"d1"}, 1, EventType::PointsAdjusted, 1, 0, 1, "", std::nullopt}}};
    REQUIRE(!service.commit(wrong_class));
    std::cout << "All TurtleClass core tests passed\n";
    return run_server_tests();
}
