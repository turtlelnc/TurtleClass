#include "TurtleClass/Server/server.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace turtleclass::core;
using namespace turtleclass::server;

#define REQUIRE_SERVER(expr) do { if (!(expr)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " expected " #expr "\n"; return EXIT_FAILURE; } } while (false)

static DomainEvent server_points(std::string id, std::int64_t sequence, std::string device, int points) {
    return {EventId{id}, ClassId{"class1"}, EventGroupId{"group-" + id}, StudentId{"student1"}, DeviceId{device}, sequence, EventType::PointsAdjusted, points, 0, 1, "", std::nullopt};
}

int run_server_tests() {
    const auto root = std::filesystem::temp_directory_path() / "turtleclass-server-tests";
    std::filesystem::remove_all(root);

    ServerBackend server{ClassId{"class1"}, FileEventLog{root}};
    server.load_from_disk();
    REQUIRE_SERVER(server.health().ok);
    REQUIRE_SERVER(server.health().confirmed_events == 0);

    EventGroup first{EventGroupId{"group-e1"}, ClassId{"class1"}, {server_points("e1", 1, "device-a", 5)}};
    auto uploaded = server.upload(first);
    REQUIRE_SERVER(uploaded.accepted);
    REQUIRE_SERVER(uploaded.first_server_sequence == 1);
    REQUIRE_SERVER(uploaded.last_server_sequence == 1);
    REQUIRE_SERVER(server.download_after(0).size() == 1);

    auto duplicate = server.upload(first);
    REQUIRE_SERVER(duplicate.accepted);
    REQUIRE_SERVER(duplicate.duplicate);
    REQUIRE_SERVER(server.download_after(0).size() == 1);

    EventGroup out_of_order{EventGroupId{"group-e2"}, ClassId{"class1"}, {server_points("e2", 1, "device-a", 2)}};
    REQUIRE_SERVER(!server.upload(out_of_order).accepted);

    EventGroup second{EventGroupId{"group-e3"}, ClassId{"class1"}, {server_points("e3", 1, "device-b", 3)}};
    REQUIRE_SERVER(server.upload(second).accepted);
    REQUIRE_SERVER(server.download_after(1).size() == 1);

    server.set_maintenance_mode(true);
    EventGroup blocked{EventGroupId{"group-e4"}, ClassId{"class1"}, {server_points("e4", 2, "device-b", 1)}};
    REQUIRE_SERVER(!server.upload(blocked).accepted);
    server.set_maintenance_mode(false);

    server.create_backup();
    const auto export_path = root / "exports" / "events.tsv";
    server.export_events(export_path);
    REQUIRE_SERVER(std::filesystem::exists(export_path));

    ServerBackend restored{ClassId{"class1"}, FileEventLog{root}};
    restored.load_from_disk();
    REQUIRE_SERVER(restored.health().confirmed_events == 2);
    REQUIRE_SERVER(restored.download_after(0).size() == 2);

    std::filesystem::remove_all(root);
    std::cout << "All TurtleClass server tests passed\n";
    return EXIT_SUCCESS;
}
