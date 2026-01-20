/**
 * Database Model - SQLite con better-sqlite3
 * Gestión de estaciones y datos meteorológicos
 */

const Database = require('better-sqlite3');
const path = require('path');
const fs = require('fs');

// Asegurar que existe el directorio de datos
const dataDir = path.join(__dirname, '../../data');
if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
}

const db = new Database(path.join(dataDir, 'weather.db'));

// Habilitar WAL mode para mejor rendimiento concurrente
db.pragma('journal_mode = WAL');

// Crear tablas si no existen
db.exec(`
    -- Tabla de estaciones
    CREATE TABLE IF NOT EXISTS stations (
        id TEXT PRIMARY KEY,
        name TEXT NOT NULL,
        description TEXT,
        location_lat REAL,
        location_lng REAL,
        altitude REAL,
        api_key TEXT UNIQUE NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        last_seen TEXT,
        is_active INTEGER DEFAULT 1,
        config_sf INTEGER DEFAULT 9,
        config_bw REAL DEFAULT 125.0,
        config_interval INTEGER DEFAULT 600000
    );

    -- Tabla de datos meteorológicos
    CREATE TABLE IF NOT EXISTS readings (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        station_id TEXT NOT NULL,
        timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
        packet_id INTEGER,
        temp_air REAL,
        hum_air REAL,
        temp_soil REAL,
        vwc_soil REAL,
        pressure REAL,
        par REAL,
        solar_radiation REAL,
        precipitation REAL,
        rssi REAL,
        snr REAL,
        freq_error REAL,
        battery_voltage REAL,
        FOREIGN KEY (station_id) REFERENCES stations(id)
    );

    -- Tabla de alertas
    CREATE TABLE IF NOT EXISTS alerts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        station_id TEXT NOT NULL,
        type TEXT NOT NULL,
        message TEXT NOT NULL,
        value REAL,
        threshold REAL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        acknowledged INTEGER DEFAULT 0,
        FOREIGN KEY (station_id) REFERENCES stations(id)
    );

    -- Tabla de usuarios
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        email TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        role TEXT DEFAULT 'viewer',
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        last_login TEXT
    );

    -- Índices para optimizar queries
    CREATE INDEX IF NOT EXISTS idx_readings_station_time
        ON readings(station_id, timestamp DESC);

    CREATE INDEX IF NOT EXISTS idx_readings_timestamp
        ON readings(timestamp DESC);

    CREATE INDEX IF NOT EXISTS idx_alerts_station
        ON alerts(station_id, acknowledged);

    CREATE INDEX IF NOT EXISTS idx_users_email
        ON users(email);
`);

// Funciones de estaciones
const stationQueries = {
    getAll: db.prepare(`
        SELECT s.*,
            (SELECT COUNT(*) FROM readings WHERE station_id = s.id) as total_readings,
            (SELECT MAX(timestamp) FROM readings WHERE station_id = s.id) as last_reading
        FROM stations s
        WHERE is_active = 1
        ORDER BY name
    `),

    getById: db.prepare(`SELECT * FROM stations WHERE id = ?`),

    getByApiKey: db.prepare(`SELECT * FROM stations WHERE api_key = ?`),

    create: db.prepare(`
        INSERT INTO stations (id, name, description, location_lat, location_lng, altitude, api_key)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    `),

    update: db.prepare(`
        UPDATE stations
        SET name = ?, description = ?, location_lat = ?, location_lng = ?, altitude = ?
        WHERE id = ?
    `),

    updateLastSeen: db.prepare(`
        UPDATE stations SET last_seen = CURRENT_TIMESTAMP WHERE id = ?
    `),

    updateConfig: db.prepare(`
        UPDATE stations SET config_sf = ?, config_bw = ?, config_interval = ? WHERE id = ?
    `),

    delete: db.prepare(`UPDATE stations SET is_active = 0 WHERE id = ?`)
};

// Funciones de lecturas
const readingQueries = {
    insert: db.prepare(`
        INSERT INTO readings (
            station_id, packet_id, temp_air, hum_air, temp_soil, vwc_soil,
            pressure, par, solar_radiation, precipitation,
            rssi, snr, freq_error, battery_voltage
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `),

    getLatest: db.prepare(`
        SELECT * FROM readings
        WHERE station_id = ?
        ORDER BY timestamp DESC
        LIMIT 1
    `),

    getByStation: db.prepare(`
        SELECT * FROM readings
        WHERE station_id = ?
        ORDER BY timestamp DESC
        LIMIT ?
    `),

    getByStationRange: db.prepare(`
        SELECT * FROM readings
        WHERE station_id = ? AND timestamp BETWEEN ? AND ?
        ORDER BY timestamp ASC
    `),

    getStats: db.prepare(`
        SELECT
            COUNT(*) as count,
            AVG(temp_air) as avg_temp_air,
            MIN(temp_air) as min_temp_air,
            MAX(temp_air) as max_temp_air,
            AVG(hum_air) as avg_hum_air,
            AVG(temp_soil) as avg_temp_soil,
            AVG(vwc_soil) as avg_vwc_soil,
            SUM(precipitation) as total_precipitation
        FROM readings
        WHERE station_id = ? AND timestamp >= datetime('now', ?)
    `),

    deleteOld: db.prepare(`
        DELETE FROM readings
        WHERE timestamp < datetime('now', ?)
    `)
};

// Funciones de alertas
const alertQueries = {
    create: db.prepare(`
        INSERT INTO alerts (station_id, type, message, value, threshold)
        VALUES (?, ?, ?, ?, ?)
    `),

    getActive: db.prepare(`
        SELECT * FROM alerts
        WHERE station_id = ? AND acknowledged = 0
        ORDER BY created_at DESC
    `),

    acknowledge: db.prepare(`
        UPDATE alerts SET acknowledged = 1 WHERE id = ?
    `)
};

// Funciones de usuarios
const userQueries = {
    create: db.prepare(`
        INSERT INTO users (username, email, password_hash, role)
        VALUES (?, ?, ?, ?)
    `),

    getById: db.prepare(`SELECT id, username, email, role, created_at, last_login FROM users WHERE id = ?`),

    getByEmail: db.prepare(`SELECT * FROM users WHERE email = ?`),

    getByUsername: db.prepare(`SELECT * FROM users WHERE username = ?`),

    updateLastLogin: db.prepare(`UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?`),

    getAll: db.prepare(`SELECT id, username, email, role, created_at, last_login FROM users ORDER BY created_at DESC`)
};

module.exports = {
    db,
    stations: stationQueries,
    readings: readingQueries,
    alerts: alertQueries,
    users: userQueries
};
