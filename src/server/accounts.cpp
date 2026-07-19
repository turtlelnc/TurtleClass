#include "TurtleClass/Server/accounts.hpp"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace turtleclass::server {

namespace {
std::int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Simple hash simulation for Phase 2 alpha (to be replaced with Argon2id)
std::string simple_hash(const std::string& input, const std::string& salt) {
    std::string combined = salt + input + salt;
    std::hash<std::string> hasher;
    auto hash_val = hasher(combined);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash_val;
    return "argon2id$placeholder$" + salt + "$" + ss.str();
}

bool verify_simple_hash(const std::string& input, const std::string& stored_hash) {
    // Parse stored hash format: argon2id$placeholder$salt$hash
    auto pos1 = stored_hash.find('$');
    if (pos1 == std::string::npos) return false;
    auto pos2 = stored_hash.find('$', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    auto pos3 = stored_hash.find('$', pos2 + 1);
    if (pos3 == std::string::npos) return false;
    
    // Salt is between second and third $
    std::string salt = stored_hash.substr(pos2 + 1, pos3 - pos2 - 1);
    std::string expected_hash = stored_hash.substr(pos3 + 1);
    
    // Recompute hash with same salt using identical logic to simple_hash
    std::string combined = salt + input + salt;
    std::hash<std::string> hasher;
    auto hash_val = hasher(combined);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash_val;
    std::string computed_hash = ss.str();
    
    return computed_hash == expected_hash;
}
} // namespace

// AccountStore implementation
bool AccountStore::create_class_account(const AccountId& account_id, const std::string& token) {
    if (account_id.empty() || token.empty()) return false;
    if (class_accounts_.contains(account_id)) return false;
    
    ClassAccount account;
    account.account_id = account_id;
    account.token_hash = simple_hash(token, "class_salt_v1");
    account.created_at_unix = now_unix();
    account.active = true;
    
    class_accounts_[account_id] = account;
    return true;
}

bool AccountStore::verify_class_token(const AccountId& account_id, const std::string& token) const {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_simple_hash(token, found->second.token_hash);
}

bool AccountStore::revoke_class_account(const AccountId& account_id) {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return false;
    found->second.active = false;
    return true;
}

std::optional<ClassAccount> AccountStore::get_class_account(const AccountId& account_id) const {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return std::nullopt;
    return found->second;
}

bool AccountStore::create_admin_account(const AdminId& admin_id, const std::string& password) {
    if (admin_id.empty() || password.empty()) return false;
    if (admin_accounts_.contains(admin_id)) return false;
    
    AdminAccount account;
    account.admin_id = admin_id;
    account.password_hash = simple_hash(password, "admin_salt_v1");
    account.created_at_unix = now_unix();
    account.active = true;
    
    admin_accounts_[admin_id] = account;
    return true;
}

bool AccountStore::verify_admin_password(const AdminId& admin_id, const std::string& password) const {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_simple_hash(password, found->second.password_hash);
}

bool AccountStore::revoke_admin_account(const AdminId& admin_id) {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return false;
    found->second.active = false;
    return true;
}

std::optional<AdminAccount> AccountStore::get_admin_account(const AdminId& admin_id) const {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return std::nullopt;
    return found->second;
}

std::vector<AdminAccount> AccountStore::all_admins() const {
    std::vector<AdminAccount> out;
    out.reserve(admin_accounts_.size());
    for (const auto& [_, account] : admin_accounts_) {
        out.push_back(account);
    }
    return out;
}

// DeviceTable implementation
bool DeviceTable::register_device(DeviceId device_id, std::string display_name, std::string public_key_hex) {
    if (device_id.empty() || display_name.empty() || public_key_hex.empty()) return false;
    // Do not allow duplicate device registration
    if (devices_.contains(device_id)) return false;
    
    DeviceEntry entry;
    entry.device_id = std::move(device_id);
    entry.display_name = std::move(display_name);
    entry.public_key_hex = std::move(public_key_hex);
    entry.registered_at_unix = now_unix();
    entry.active = true;
    
    devices_[entry.device_id] = entry;
    return true;
}

bool DeviceTable::update_device_key(const DeviceId& device_id, const std::string& new_public_key_hex) {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return false;
    if (new_public_key_hex.empty()) return false;
    
    found->second.public_key_hex = new_public_key_hex;
    return true;
}

bool DeviceTable::revoke_device(const DeviceId& device_id) {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return false;
    found->second.active = false;
    return true;
}

bool DeviceTable::is_active(const DeviceId& device_id) const {
    auto found = devices_.find(device_id);
    return found != devices_.end() && found->second.active;
}

std::optional<std::string> DeviceTable::get_public_key(const DeviceId& device_id) const {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return std::nullopt;
    return found->second.public_key_hex;
}

std::size_t DeviceTable::active_count() const {
    return static_cast<std::size_t>(
        std::count_if(devices_.begin(), devices_.end(), [](const auto& item) { return item.second.active; })
    );
}

std::vector<DeviceEntry> DeviceTable::devices() const {
    std::vector<DeviceEntry> out;
    out.reserve(devices_.size());
    for (const auto& [_, entry] : devices_) {
        out.push_back(entry);
    }
    return out;
}

} // namespace turtleclass::server
