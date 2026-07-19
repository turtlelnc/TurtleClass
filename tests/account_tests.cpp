#include "TurtleClass/Server/accounts.hpp"

#include <cstdlib>
#include <iostream>

using namespace turtleclass::server;

#define REQUIRE_ACCOUNT(expr) do { if (!(expr)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " expected " #expr "\n"; return EXIT_FAILURE; } } while (false)

int run_account_tests() {
    AccountStore store;
    
    // Test class account creation
    REQUIRE_ACCOUNT(store.create_class_account(AccountId{"class-001"}, "token123"));
    REQUIRE_ACCOUNT(!store.create_class_account(AccountId{"class-001"}, "token456"));  // Duplicate
    
    // Test class account verification
    REQUIRE_ACCOUNT(store.verify_class_token(AccountId{"class-001"}, "token123"));
    REQUIRE_ACCOUNT(!store.verify_class_token(AccountId{"class-001"}, "wrongtoken"));
    REQUIRE_ACCOUNT(!store.verify_class_token(AccountId{"nonexistent"}, "token123"));
    
    // Test admin account creation
    REQUIRE_ACCOUNT(store.create_admin_account(AdminId{"admin-tom"}, "adminpass123"));
    REQUIRE_ACCOUNT(!store.create_admin_account(AdminId{"admin-tom"}, "anotherpass"));  // Duplicate
    
    // Test admin password verification
    REQUIRE_ACCOUNT(store.verify_admin_password(AdminId{"admin-tom"}, "adminpass123"));
    REQUIRE_ACCOUNT(!store.verify_admin_password(AdminId{"admin-tom"}, "wrongpass"));
    REQUIRE_ACCOUNT(!store.verify_admin_password(AdminId{"nonexistent"}, "pass"));
    
    // Test account revocation
    REQUIRE_ACCOUNT(store.revoke_class_account(AccountId{"class-001"}));
    REQUIRE_ACCOUNT(!store.verify_class_token(AccountId{"class-001"}, "token123"));  // Revoked
    
    REQUIRE_ACCOUNT(store.revoke_admin_account(AdminId{"admin-tom"}));
    REQUIRE_ACCOUNT(!store.verify_admin_password(AdminId{"admin-tom"}, "adminpass123"));  // Revoked
    
    // Test device table
    DeviceTable devices;
    
    // Test device registration
    REQUIRE_ACCOUNT(devices.register_device(DeviceId{"device-001"}, "Classroom PC", "ed25519pubkey001"));
    REQUIRE_ACCOUNT(devices.register_device(DeviceId{"device-002"}, "Teacher PC", "ed25519pubkey002"));
    REQUIRE_ACCOUNT(!devices.register_device(DeviceId{"device-001"}, "Another PC", "anotherkey"));  // Duplicate
    
    // Test device verification
    REQUIRE_ACCOUNT(devices.is_active(DeviceId{"device-001"}));
    REQUIRE_ACCOUNT(!devices.is_active(DeviceId{"nonexistent"}));
    
    // Test public key retrieval
    auto key = devices.get_public_key(DeviceId{"device-001"});
    REQUIRE_ACCOUNT(key.has_value());
    REQUIRE_ACCOUNT(key.value() == "ed25519pubkey001");
    
    // Test device revocation
    REQUIRE_ACCOUNT(devices.revoke_device(DeviceId{"device-001"}));
    REQUIRE_ACCOUNT(!devices.is_active(DeviceId{"device-001"}));
    
    // Test active count
    REQUIRE_ACCOUNT(devices.active_count() == 1);  // Only device-002 is active
    
    // Test device key update
    REQUIRE_ACCOUNT(devices.update_device_key(DeviceId{"device-002"}, "newpubkey002"));
    auto new_key = devices.get_public_key(DeviceId{"device-002"});
    REQUIRE_ACCOUNT(new_key.has_value());
    REQUIRE_ACCOUNT(new_key.value() == "newpubkey002");
    
    // Test listing all devices
    auto all_devices = devices.devices();
    REQUIRE_ACCOUNT(all_devices.size() == 2);
    
    std::cout << "All TurtleClass account tests passed\n";
    return EXIT_SUCCESS;
}

int main() {
    return run_account_tests();
}
