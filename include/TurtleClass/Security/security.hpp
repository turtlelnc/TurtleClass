#pragma once

#include "TurtleClass/Core/domain.hpp"

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace turtleclass::security {

using turtleclass::core::DeviceId;
using turtleclass::core::AccountId;
using turtleclass::core::AdminId;

// Account credentials
struct AccountCredentials {
    AccountId account_id;
    std::string token_hash;  // Argon2id hash
    std::int64_t created_at_unix = 0;
    bool active = true;
};

struct AdminCredentials {
    AdminId admin_id;
    std::string password_hash;  // Argon2id hash
    std::int64_t created_at_unix = 0;
    bool active = true;
};

// Device credentials (Ed25519)
struct DeviceCredentials {
    DeviceId device_id;
    std::string display_name;
    std::string public_key_hex;  // Ed25519 public key
    std::int64_t registered_at_unix = 0;
    bool active = true;
};

// Account manager for class and admin accounts
class AccountManager {
public:
    // Class account operations
    bool create_class_account(const AccountId& account_id, const std::string& token);
    bool verify_class_token(const AccountId& account_id, const std::string& token) const;
    bool revoke_class_account(const AccountId& account_id);
    [[nodiscard]] std::optional<AccountCredentials> get_class_account(const AccountId& account_id) const;
    
    // Admin account operations
    bool create_admin_account(const AdminId& admin_id, const std::string& password);
    bool verify_admin_password(const AdminId& admin_id, const std::string& password) const;
    bool revoke_admin_account(const AdminId& admin_id);
    [[nodiscard]] std::optional<AdminCredentials> get_admin_account(const AdminId& admin_id) const;
    [[nodiscard]] std::vector<AdminCredentials> all_admins() const;

private:
    std::map<AccountId, AccountCredentials> class_accounts_;
    std::map<AdminId, AdminCredentials> admin_accounts_;
    
    [[nodiscard]] std::string hash_argon2id(const std::string& input, const std::string& salt) const;
    [[nodiscard]] bool verify_argon2id(const std::string& input, const std::string& hash) const;
};

// Device registry with Ed25519 credentials
class DeviceRegistry {
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
    [[nodiscard]] std::vector<DeviceCredentials> devices() const;

private:
    std::map<DeviceId, DeviceCredentials> devices_;
};

// Signature verification utilities (Ed25519)
class SignatureVerifier {
public:
    // Verify Ed25519 signature
    // Returns true if signature is valid for the given data and public key
    static bool verify_ed25519(
        const std::vector<std::uint8_t>& data,
        const std::vector<std::uint8_t>& signature,
        const std::vector<std::uint8_t>& public_key
    );
    
    // Hex utilities
    static std::vector<std::uint8_t> hex_to_bytes(const std::string& hex);
    static std::string bytes_to_hex(const std::vector<std::uint8_t>& bytes);
};

} // namespace turtleclass::security
