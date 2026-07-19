-- TurtleClass Server Database Schema
-- Version: 1.0 (Phase 2)

-- Class accounts table
CREATE TABLE IF NOT EXISTS classes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_code TEXT UNIQUE NOT NULL,
    class_name TEXT NOT NULL,
    admin_password_hash TEXT NOT NULL,
    admin_salt TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Students table
CREATE TABLE IF NOT EXISTS students (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,
    student_id TEXT NOT NULL,
    student_name TEXT NOT NULL,
    password_hash TEXT,
    password_salt TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (class_id) REFERENCES classes(id),
    UNIQUE(class_id, student_id)
);

-- Devices table (persistent device credentials)
CREATE TABLE IF NOT EXISTS devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT UNIQUE NOT NULL,
    device_name TEXT,
    device_type TEXT NOT NULL, -- 'windows', 'cli', 'server'
    class_id INTEGER,
    public_key TEXT NOT NULL, -- Ed25519 public key (hex encoded)
    private_key_encrypted TEXT, -- Encrypted private key (optional for servers)
    status TEXT DEFAULT 'active', -- 'active', 'revoked', 'suspended'
    last_seen DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    revoked_at DATETIME,
    FOREIGN KEY (class_id) REFERENCES classes(id)
);

-- Device revocation list
CREATE TABLE IF NOT EXISTS device_revocations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    revocation_reason TEXT,
    revoked_by TEXT, -- admin or system
    revoked_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (device_id) REFERENCES devices(device_id)
);

-- Events log (JSON Lines stored as text for flexibility)
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id TEXT UNIQUE NOT NULL,
    class_id INTEGER NOT NULL,
    device_id TEXT NOT NULL,
    event_type TEXT NOT NULL,
    event_data TEXT NOT NULL, -- JSON payload
    signature TEXT NOT NULL, -- Ed25519 signature (hex encoded)
    timestamp DATETIME NOT NULL,
    sequence_number INTEGER NOT NULL,
    processed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (class_id) REFERENCES classes(id),
    FOREIGN KEY (device_id) REFERENCES devices(device_id),
    UNIQUE(class_id, sequence_number)
);

-- Sequence counters per class
CREATE TABLE IF NOT EXISTS sequence_counters (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,
    current_sequence INTEGER DEFAULT 0,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (class_id) REFERENCES classes(id),
    UNIQUE(class_id)
);

-- Snapshots table
CREATE TABLE IF NOT EXISTS snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    snapshot_id TEXT UNIQUE NOT NULL,
    class_id INTEGER NOT NULL,
    snapshot_version INTEGER NOT NULL,
    snapshot_data TEXT NOT NULL, -- Compressed JSON state
    checksum TEXT NOT NULL, -- SHA-256 checksum
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_valid BOOLEAN DEFAULT TRUE,
    FOREIGN KEY (class_id) REFERENCES classes(id)
);

-- Conflicts table
CREATE TABLE IF NOT EXISTS conflicts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    conflict_id TEXT UNIQUE NOT NULL,
    class_id INTEGER NOT NULL,
    conflict_type TEXT NOT NULL, -- 'sequence', 'signature', 'data'
    conflicting_events TEXT NOT NULL, -- JSON array of event_ids
    resolution_status TEXT DEFAULT 'pending', -- 'pending', 'resolved', 'frozen'
    resolution_data TEXT, -- JSON resolution details
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    resolved_at DATETIME,
    resolved_by TEXT,
    FOREIGN KEY (class_id) REFERENCES classes(id)
);

-- Audit log for admin actions
CREATE TABLE IF NOT EXISTS audit_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action_type TEXT NOT NULL,
    actor_id TEXT, -- admin or device_id
    target_type TEXT, -- 'student', 'device', 'event', 'class'
    target_id TEXT,
    action_data TEXT, -- JSON details
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    ip_address TEXT
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_events_class_sequence ON events(class_id, sequence_number);
CREATE INDEX IF NOT EXISTS idx_events_device ON events(device_id);
CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp);
CREATE INDEX IF NOT EXISTS idx_students_class ON students(class_id);
CREATE INDEX IF NOT EXISTS idx_devices_class ON devices(class_id);
CREATE INDEX IF NOT EXISTS idx_snapshots_class ON snapshots(class_id);
CREATE INDEX IF NOT EXISTS idx_conflicts_class ON conflicts(class_id);
CREATE INDEX IF NOT EXISTS idx_audit_log_timestamp ON audit_log(timestamp);
