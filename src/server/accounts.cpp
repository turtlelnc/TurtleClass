#include "TurtleClass/Server/accounts.hpp"
#include <sodium.h>
#include <argon2.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <cstring>

namespace turtleclass::server {

namespace {
std::int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Generate a random salt (16 bytes)
std::string generate_salt() {
    std::vector<unsigned char> salt(16);
    randombytes_buf(salt.data(), salt.size());
    
    std::stringstream ss;
    for (auto byte : salt) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// Argon2id hash - production ready
std::string argon2id_hash(const std::string& password, const std::string& salt) {
    // Argon2id parameters for password hashing (OWASP recommendations)
    // Memory: 64 MB, Iterations: 3, Parallelism: 4
    const uint32_t memory_cost = 65536;  // 64 MB in KB
    const uint32_t time_cost = 3;
    const uint32_t parallelism = 4;
    const uint32_t hash_len = 32;  // 256-bit hash
    
    std::string hashed_output(hash_len, '\0');
    
    int result = argon2id_hash_raw(
        time_cost,
        memory_cost,
        parallelism,
        password.c_str(),
        password.size(),
        salt.c_str(),
        salt.size(),
        hashed_output.data(),
        hash_len
    );
    
    if (result != ARGON2_OK) {
        throw std::runtime_error("Argon2id hashing failed: " + std::string(argon2_error_message(result)));
    }
    
    // Convert hash to hex
    std::stringstream ss;
    for (unsigned char c : hashed_output) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
    }
    
    // Format: $argon2id$v=19$m=65536,t=3,p=4$salt$hash
    return "$argon2id$v=19$m=65536,t=3,p=4$" + salt + "$" + ss.str();
}

bool verify_argon2id_hash(const std::string& password, const std::string& stored_hash) {
    // Parse stored hash format: $argon2id$v=19$m=65536,t=3,p=4$salt$hash
    if (stored_hash.find("$argon2id$") != 0) {
        return false;
    }
    
    // Find salt and hash positions
    auto pos1 = stored_hash.find('$', 10);  // Skip "$argon2id$"
    if (pos1 == std::string::npos) return false;
    auto pos2 = stored_hash.find('$', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    auto pos3 = stored_hash.find('$', pos2 + 1);
    if (pos3 == std::string::npos) return false;
    
    // Extract salt (between second and third $)
    std::string salt = stored_hash.substr(pos2 + 1, pos3 - pos2 - 1);
    std::string expected_hash = stored_hash.substr(pos3 + 1);
    
    // Re-hash with same parameters
    try {
        std::string computed_hash_full = argon2id_hash(password, salt);
        // Extract just the hash part from the full format
        auto hash_pos = computed_hash_full.rfind('$');
        if (hash_pos == std::string::npos) return false;
        std::string computed_hash = computed_hash_full.substr(hash_pos + 1);
        
        return computed_hash == expected_hash;
    } catch (...) {
        return false;
    }
}
} // namespace

// AccountStore implementation
bool AccountStore::create_class_account(const AccountId& account_id, const std::string& token) {
    if (account_id.empty() || token.empty()) return false;
    if (class_accounts_.contains(account_id)) return false;
    
    std::string salt = generate_salt();
    std::string token_hash = argon2id_hash(token, salt);
    
    ClassAccount account;
    account.account_id = account_id;
    account.token_hash = token_hash;
    account.created_at_unix = now_unix();
    account.active = true;
    
    class_accounts_[account_id] = account;
    return true;
}

bool AccountStore::verify_class_token(const AccountId& account_id, const std::string& token) const {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_argon2id_hash(token, found->second.token_hash);
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
    
    std::string salt = generate_salt();
    std::string password_hash = argon2id_hash(password, salt);
    
    AdminAccount account;
    account.admin_id = admin_id;
    account.password_hash = password_hash;
    account.created_at_unix = now_unix();
    account.active = true;
    
    admin_accounts_[admin_id] = account;
    return true;
}

bool AccountStore::verify_admin_password(const AdminId& admin_id, const std::string& password) const {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_argon2id_hash(password, found->second.password_hash);
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
