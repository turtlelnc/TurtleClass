#include "TurtleClass/Security/security.hpp"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace turtleclass::security {

namespace {
std::int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Simple hash simulation for Phase 2 alpha (to be replaced with Argon2id)
// This is a placeholder - in production, use libsodium's argon2id implementation
std::string simple_hash(const std::string& input, const std::string& salt) {
    std::string combined = salt + input + salt;
    std::hash<std::string> hasher;
    auto hash_val = hasher(combined);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash_val;
    return "argon2id$placeholder$" + salt + "$" + ss.str();
}

bool verify_simple_hash(const std::string& input, const std::string& stored_hash) {
    // Parse stored hash format: argon2id$...$salt$hash
    auto pos1 = stored_hash.find('$');
    if (pos1 == std::string::npos) return false;
    auto pos2 = stored_hash.find('$', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    auto pos3 = stored_hash.find('$', pos2 + 1);
    if (pos3 == std::string::npos) return false;
    
    std::string salt = stored_hash.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string expected_hash = stored_hash.substr(pos3 + 1);
    
    std::string computed = simple_hash(input, salt);
    auto computed_hash_start = computed.rfind('$');
    if (computed_hash_start == std::string::npos) return false;
    
    return computed.substr(computed_hash_start + 1) == expected_hash;
}
} // namespace

// AccountManager implementation
bool AccountManager::create_class_account(const AccountId& account_id, const std::string& token) {
    if (account_id.empty() || token.empty()) return false;
    if (class_accounts_.contains(account_id)) return false;  // Already exists
    
    AccountCredentials creds;
    creds.account_id = account_id;
    creds.token_hash = hash_argon2id(token, "class_salt_v1");
    creds.created_at_unix = now_unix();
    creds.active = true;
    
    class_accounts_[account_id] = creds;
    return true;
}

bool AccountManager::verify_class_token(const AccountId& account_id, const std::string& token) const {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_argon2id(token, found->second.token_hash);
}

bool AccountManager::revoke_class_account(const AccountId& account_id) {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return false;
    found->second.active = false;
    return true;
}

std::optional<AccountCredentials> AccountManager::get_class_account(const AccountId& account_id) const {
    auto found = class_accounts_.find(account_id);
    if (found == class_accounts_.end()) return std::nullopt;
    return found->second;
}

bool AccountManager::create_admin_account(const AdminId& admin_id, const std::string& password) {
    if (admin_id.empty() || password.empty()) return false;
    if (admin_accounts_.contains(admin_id)) return false;  // Already exists
    
    AdminCredentials creds;
    creds.admin_id = admin_id;
    creds.password_hash = hash_argon2id(password, "admin_salt_v1");
    creds.created_at_unix = now_unix();
    creds.active = true;
    
    admin_accounts_[admin_id] = creds;
    return true;
}

bool AccountManager::verify_admin_password(const AdminId& admin_id, const std::string& password) const {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return false;
    if (!found->second.active) return false;
    
    return verify_argon2id(password, found->second.password_hash);
}

bool AccountManager::revoke_admin_account(const AdminId& admin_id) {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return false;
    found->second.active = false;
    return true;
}

std::optional<AdminCredentials> AccountManager::get_admin_account(const AdminId& admin_id) const {
    auto found = admin_accounts_.find(admin_id);
    if (found == admin_accounts_.end()) return std::nullopt;
    return found->second;
}

std::vector<AdminCredentials> AccountManager::all_admins() const {
    std::vector<AdminCredentials> out;
    out.reserve(admin_accounts_.size());
    for (const auto& [_, creds] : admin_accounts_) {
        out.push_back(creds);
    }
    return out;
}

std::string AccountManager::hash_argon2id(const std::string& input, const std::string& salt) const {
    return simple_hash(input, salt);
}

bool AccountManager::verify_argon2id(const std::string& input, const std::string& hash) const {
    return verify_simple_hash(input, hash);
}

// DeviceRegistry implementation
bool DeviceRegistry::register_device(DeviceId device_id, std::string display_name, std::string public_key_hex) {
    if (device_id.empty() || display_name.empty() || public_key_hex.empty()) return false;
    
    DeviceCredentials creds;
    creds.device_id = std::move(device_id);
    creds.display_name = std::move(display_name);
    creds.public_key_hex = std::move(public_key_hex);
    creds.registered_at_unix = now_unix();
    creds.active = true;
    
    devices_[creds.device_id] = creds;
    return true;
}

bool DeviceRegistry::update_device_key(const DeviceId& device_id, const std::string& new_public_key_hex) {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return false;
    if (new_public_key_hex.empty()) return false;
    
    found->second.public_key_hex = new_public_key_hex;
    return true;
}

bool DeviceRegistry::revoke_device(const DeviceId& device_id) {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return false;
    found->second.active = false;
    return true;
}

bool DeviceRegistry::is_active(const DeviceId& device_id) const {
    auto found = devices_.find(device_id);
    return found != devices_.end() && found->second.active;
}

std::optional<std::string> DeviceRegistry::get_public_key(const DeviceId& device_id) const {
    auto found = devices_.find(device_id);
    if (found == devices_.end()) return std::nullopt;
    return found->second.public_key_hex;
}

std::size_t DeviceRegistry::active_count() const {
    return static_cast<std::size_t>(
        std::count_if(devices_.begin(), devices_.end(), [](const auto& item) { return item.second.active; })
    );
}

std::vector<DeviceCredentials> DeviceRegistry::devices() const {
    std::vector<DeviceCredentials> out;
    out.reserve(devices_.size());
    for (const auto& [_, creds] : devices_) {
        out.push_back(creds);
    }
    return out;
}

// SignatureVerifier implementation
bool SignatureVerifier::verify_ed25519(
    const std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& signature,
    const std::vector<std::uint8_t>& public_key
) {
    // Placeholder implementation - to be replaced with libsodium
    // For Phase 2 alpha, we'll accept any non-empty signature as valid
    // This allows testing the flow before integrating actual crypto
    
    if (data.empty() || signature.empty() || public_key.empty()) {
        return false;
    }
    
    // TODO: Implement actual Ed25519 verification using libsodium
    // crypto_sign_verify_detached(signature.data(), data.data(), data.size(), public_key.data())
    
    // For now, return true to allow development flow
    return true;
}

std::vector<std::uint8_t> SignatureVerifier::hex_to_bytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    if (hex.size() % 2 != 0) return bytes;
    
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

std::string SignatureVerifier::bytes_to_hex(const std::vector<std::uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

} // namespace turtleclass::security
