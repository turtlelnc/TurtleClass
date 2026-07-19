#pragma once

#include "TurtleClass/Core/domain.hpp"

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <cstdint>

namespace turtleclass::server {

using turtleclass::core::AccountId;
using turtleclass::core::AdminId;
using turtleclass::core::DeviceId;

// Class account credentials
struct ClassAccount {
    AccountId account_id;
    std::string token_hash;  // Argon2id hash (placeholder in Phase 2 alpha)
    std::int64_t created_at_unix = 0;
    bool active = true;
};

// Admin account credentials
struct AdminAccount {
    AdminId admin_id;
    std::string password_hash;  // Argon2id hash (placeholder in Phase 2 alpha)
    std::int64_t created_at_unix = 0;
    bool active = true;
};

// Device entry with Ed25519 public key
struct DeviceEntry {
    DeviceId device_id;
    std::string display_name;
    std::string public_key_hex;  // Ed25519 public key
    std::int64_t registered_at_unix = 0;
    bool active = true;
};

// Account store for class and admin accounts
class AccountStore {
public:
    // Class account operations
    bool create_class_account(const AccountId& account_id, const std::string& token);
    bool verify_class_token(const AccountId& account_id, const std::string& token) const;
    bool revoke_class_account(const AccountId& account_id);
    [[nodiscard]] std::optional<ClassAccount> get_class_account(const AccountId& account_id) const;
    
    // Admin account operations
    bool create_admin_account(const AdminId& admin_id, const std::string& password);
    bool verify_admin_password(const AdminId& admin_id, const std::string& password) const;
    bool revoke_admin_account(const AdminId& admin_id);
    [[nodiscard]] std::optional<AdminAccount> get_admin_account(const AdminId& admin_id) const;
    [[nodiscard]] std::vector<AdminAccount> all_admins() const;

private:
    std::map<AccountId, ClassAccount> class_accounts_;
    std::map<AdminId, AdminAccount> admin_accounts_;
};

// Persistent device table with Ed25519 credentials
class DeviceTable {
public:
    // Registration with public key
    bool register_device(DeviceId device_id, std::string display_name, std::string public_key_hex);
    bool update_device_key(const DeviceId& device_id, const std::string& new_public_key_hex);
    bool revoke_device(const DeviceId& device_id);
    
    // Verification
    [[nodiscard]] bool is_active(const DeviceId& device_id) const;
    [[nodiscard]] std::optional<std::string> get_public_key(const DeviceId& device_id) const;
    
    // Listing
    [[nodiscard]] std::size_t active_count() const;
    [[nodiscard]] std::vector<DeviceEntry> devices() const;

private:
    std::map<DeviceId, DeviceEntry> devices_;
};

} // namespace turtleclass::server
