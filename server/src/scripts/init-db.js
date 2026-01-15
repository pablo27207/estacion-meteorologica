/**
 * Database initialization script
 * Creates demo station for testing
 */

const crypto = require('crypto');
const { db } = require('../models/database');

console.log('Initializing database...');

// Create demo station
const demoId = 'EST-DEMO0001';
const demoApiKey = crypto.randomBytes(32).toString('hex');

try {
    const existing = db.prepare('SELECT id FROM stations WHERE id = ?').get(demoId);

    if (!existing) {
        db.prepare(`
            INSERT INTO stations (id, name, description, location_lat, location_lng, altitude, api_key)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        `).run(
            demoId,
            'Estación Demo',
            'Estación de demostración para pruebas',
            -34.6037,  // Buenos Aires
            -58.3816,
            25,
            demoApiKey
        );

        console.log('\n✅ Demo station created:');
        console.log(`   ID: ${demoId}`);
        console.log(`   API Key: ${demoApiKey}`);
        console.log('\n   Use this API key in your ESP32 gateway configuration.\n');
    } else {
        const station = db.prepare('SELECT api_key FROM stations WHERE id = ?').get(demoId);
        console.log('\n⚠️  Demo station already exists:');
        console.log(`   ID: ${demoId}`);
        console.log(`   API Key: ${station.api_key}\n`);
    }

    // Insert some sample data for demo
    const sampleData = db.prepare(`
        INSERT INTO readings (station_id, packet_id, temp_air, hum_air, temp_soil, vwc_soil, rssi, snr)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    `);

    // Generate 24 hours of sample data
    console.log('Generating sample data...');
    for (let i = 0; i < 24; i++) {
        const temp = 20 + Math.sin(i / 4) * 8 + (Math.random() - 0.5) * 2;
        const hum = 60 + Math.cos(i / 6) * 20 + (Math.random() - 0.5) * 5;
        const tempSoil = 18 + Math.sin(i / 8) * 3 + (Math.random() - 0.5);
        const vwc = 35 + Math.cos(i / 12) * 15 + (Math.random() - 0.5) * 3;

        sampleData.run(
            demoId,
            i + 1,
            temp.toFixed(1),
            hum.toFixed(1),
            tempSoil.toFixed(1),
            vwc.toFixed(1),
            -90 + Math.random() * 20,
            5 + Math.random() * 5
        );
    }

    console.log('✅ Sample data generated (24 readings)\n');
    console.log('Database initialization complete!\n');

} catch (error) {
    console.error('Error initializing database:', error);
    process.exit(1);
}
