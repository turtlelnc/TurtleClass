#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include "core/event.hpp"
#include "crypto/signer.hpp"
#include "server/database.hpp"

using namespace turtleclass;

// Simulate two devices syncing with each other via a central server
class SyncSimulator {
public:
    SyncSimulator(const std::string& db_path) {
        if (!db_.initialize(db_path)) {
            std::cerr << "Failed to initialize database" << std::endl;
            exit(1);
        }

        // Create a test class
        std::string class_code = "TEST001";
        auto class_opt = db_.getClassByCode(class_code);
        if (!class_opt) {
            std::string salt = generateSalt();
            std::string hash = simpleHash("admin123", salt);
            db_.createClass(class_code, "Test Class", hash, salt);
            class_id_ = 1;
            std::cout << "Created test class: " << class_code << std::endl;
        } else {
            class_id_ = class_opt->id;
            std::cout << "Using existing class: " << class_code << std::endl;
        }

        // Register two devices
        registerDevice("device_A", "windows");
        registerDevice("device_B", "cli");
    }

    void runSimulation() {
        std::cout << "\n=== Starting Two-Device Sync Simulation ===\n" << std::endl;

        // Device A generates some events
        std::cout << "Device A generating events..." << std::endl;
        auto events_a = generateEvents("device_A", 5);
        for (const auto& event : events_a) {
            uploadEvent(event, "device_A");
        }

        // Device B generates some events
        std::cout << "\nDevice B generating events..." << std::endl;
        auto events_b = generateEvents("device_B", 3);
        for (const auto& event : events_b) {
            uploadEvent(event, "device_B");
        }

        // Simulate network delay
        std::cout << "\nSimulating network delay..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Device A syncs (downloads events from Device B)
        std::cout << "\nDevice A syncing..." << std::endl;
        int last_seq_a = db_.getCurrentSequence(class_id_);
        auto new_events_a = db_.getEventsAfter(class_id_, last_seq_a - 3); // Simulate partial sync
        std::cout << "Device A received " << new_events_a.size() << " new events" << std::endl;

        // Device B syncs (downloads events from Device A)
        std::cout << "\nDevice B syncing..." << std::endl;
        int last_seq_b = db_.getCurrentSequence(class_id_);
        auto new_events_b = db_.getEventsAfter(class_id_, last_seq_b - 8);
        std::cout << "Device B received " << new_events_b.size() << " new events" << std::endl;

        // Check for conflicts
        std::cout << "\nChecking for conflicts..." << std::endl;
        checkConflicts();

        // Create snapshot
        std::cout << "\nCreating snapshot..." << std::endl;
        createSnapshot();

        // Verify snapshot
        verifySnapshot();

        std::cout << "\n=== Simulation Complete ===" << std::endl;
    }

private:
    server::Database db_;
    int class_id_;
    std::mt19937 rng_{std::random_device{}()};

    std::string generateSalt() {
        static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string salt;
        for (int i = 0; i < 16; ++i) {
            salt += alphanum[rng_ % (sizeof(alphanum) - 1)];
        }
        return salt;
    }

    std::string simpleHash(const std::string& input, const std::string& salt) {
        // Simple hash for simulation (not cryptographically secure)
        unsigned long hash = 5381;
        std::string combined = salt + input;
        for (char c : combined) {
            hash = ((hash << 5) + hash) + c;
        }
        return std::to_string(hash);
    }

    void registerDevice(const std::string& device_id, const std::string& device_type) {
        crypto::Signer signer;
        auto keypair = signer.generateKeyPair();
        
        if (db_.registerDevice(device_id, device_id + " device", device_type, class_id_, keypair.public_key)) {
            std::cout << "Registered device: " << device_id << " (" << device_type << ")" << std::endl;
        } else {
            std::cout << "Device already exists: " << device_id << std::endl;
        }
    }

    core::Event generateEvent(const std::string& device_id) {
        static int event_counter = 0;
        event_counter++;

        core::Event event;
        event.event_id = "evt_" + device_id + "_" + std::to_string(event_counter);
        event.class_id = class_id_;
        event.device_id = device_id;
        event.event_type = (event_counter % 2 == 0) ? "add_points" : "remove_points";
        
        // Create event data
        event.event_data = R"({"student_id": "S)" + std::to_string((event_counter % 10) + 1) + 
                           R"(", "points": )" + std::to_string((event_counter % 5) + 1) + "}";
        
        event.timestamp = getCurrentTimestamp();
        event.sequence_number = db_.getNextSequence(class_id_);

        // Sign the event
        crypto::Signer signer;
        event.signature = signer.sign(event.getPayload(), getDevicePrivateKey(device_id));

        return event;
    }

    std::vector<core::Event> generateEvents(const std::string& device_id, int count) {
        std::vector<core::Event> events;
        for (int i = 0; i < count; ++i) {
            events.push_back(generateEvent(device_id));
        }
        return events;
    }

    bool uploadEvent(const core::Event& event, const std::string& device_id) {
        // Verify signature
        crypto::Signer signer;
        auto device = db_.getDevice(device_id);
        if (!device) {
            std::cerr << "Device not found: " << device_id << std::endl;
            return false;
        }

        bool valid = signer.verify(event.getPayload(), event.signature, device->public_key);
        if (!valid) {
            std::cerr << "Invalid signature for event: " << event.event_id << std::endl;
            return false;
        }

        // Insert into database
        if (db_.insertEvent(event.event_id, event.class_id, event.device_id,
                            event.event_type, event.event_data, event.signature,
                            event.timestamp, event.sequence_number)) {
            std::cout << "  Uploaded event: " << event.event_id << " (seq: " << event.sequence_number << ")" << std::endl;
            db_.updateDeviceLastSeen(device_id);
            return true;
        }

        std::cerr << "  Failed to upload event: " << event.event_id << std::endl;
        return false;
    }

    void checkConflicts() {
        // Simulate conflict detection (sequence gaps, duplicate events, etc.)
        int current_seq = db_.getCurrentSequence(class_id_);
        std::cout << "Current sequence number: " << current_seq << std::endl;

        // Check for pending conflicts
        auto conflicts = db_.getPendingConflicts(class_id_);
        if (conflicts.empty()) {
            std::cout << "No pending conflicts detected" << std::endl;
        } else {
            std::cout << "Found " << conflicts.size() << " pending conflicts:" << std::endl;
            for (const auto& conflict : conflicts) {
                std::cout << "  - " << conflict.conflict_id << " (" << conflict.conflict_type << ")" << std::endl;
            }
        }
    }

    void createSnapshot() {
        // Get all events and create a snapshot
        auto events = db_.getEventsAfter(class_id_, -1); // Get all events
        
        // Simple snapshot: JSON array of all events
        std::string snapshot_data = "[";
        bool first = true;
        for (const auto& event : events) {
            if (!first) snapshot_data += ",";
            snapshot_data += R"({"event_id": ")" + event.event_id + R"(", "type": ")" + 
                            event.event_type + R"(", "data": )" + event.event_data + "}";
            first = false;
        }
        snapshot_data += "]";

        // Calculate checksum (simple hash for simulation)
        std::string checksum = simpleHash(snapshot_data, "snapshot_salt");

        int version = events.size(); // Use event count as version
        if (db_.createSnapshot(class_id_, version, snapshot_data, checksum)) {
            std::cout << "Created snapshot v" << version << " with " << events.size() << " events" << std::endl;
        } else {
            std::cerr << "Failed to create snapshot" << std::endl;
        }
    }

    void verifySnapshot() {
        auto snapshot = db_.getLatestSnapshot(class_id_);
        if (!snapshot) {
            std::cerr << "No snapshot found" << std::endl;
            return;
        }

        std::cout << "Latest snapshot: " << snapshot->snapshot_id << std::endl;
        std::cout << "  Version: " << snapshot->snapshot_version << std::endl;
        std::cout << "  Valid: " << (snapshot->is_valid ? "Yes" : "No") << std::endl;
        std::cout << "  Checksum: " << snapshot->checksum << std::endl;

        // Verify checksum
        std::string expected_checksum = simpleHash(snapshot->snapshot_data, "snapshot_salt");
        if (expected_checksum != snapshot->checksum) {
            std::cerr << "Snapshot checksum mismatch! Marking as invalid." << std::endl;
            db_.markSnapshotInvalid(snapshot->snapshot_id);
        } else {
            std::cout << "Snapshot checksum verified OK" << std::endl;
        }
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t_now));
        return std::string(buffer);
    }

    std::string getDevicePrivateKey(const std::string& device_id) {
        // In real implementation, this would retrieve encrypted private key
        // For simulation, we use a deterministic key per device
        return "simulated_key_" + device_id;
    }
};

int main(int argc, char* argv[]) {
    std::string db_path = "turtleclass_test.db";
    
    if (argc > 1) {
        db_path = argv[1];
    }

    std::cout << "TurtleClass CLI Sync Simulator" << std::endl;
    std::cout << "Database: " << db_path << std::endl;
    std::cout << "==============================\n" << std::endl;

    SyncSimulator simulator(db_path);
    simulator.runSimulation();

    return 0;
}
