/**
 * General API Routes
 * Endpoints adicionales y alertas
 */

const express = require('express');
const router = express.Router();
const { stations, readings, alerts, db } = require('../models/database');

/**
 * GET /api/dashboard
 * Datos agregados para el dashboard principal
 */
router.get('/dashboard', (req, res) => {
    try {
        const allStations = stations.getAll.all();

        // Enriquecer con última lectura
        const enriched = allStations.map(station => {
            const lastReading = readings.getLatest.get(station.id);
            const activeAlerts = alerts.getActive.all(station.id);

            // Calcular estado de conexión
            let connectionStatus = 'offline';
            if (station.last_seen) {
                const lastSeenDate = new Date(station.last_seen);
                const now = new Date();
                const diffMs = now - lastSeenDate;
                const expectedInterval = station.config_interval || 600000;

                if (diffMs < expectedInterval * 2) {
                    connectionStatus = 'online';
                } else if (diffMs < expectedInterval * 5) {
                    connectionStatus = 'delayed';
                }
            }

            return {
                id: station.id,
                name: station.name,
                location: {
                    lat: station.location_lat,
                    lng: station.location_lng
                },
                connectionStatus,
                lastSeen: station.last_seen,
                lastReading: lastReading ? {
                    timestamp: lastReading.timestamp,
                    tempAir: lastReading.temp_air,
                    humAir: lastReading.hum_air,
                    tempSoil: lastReading.temp_soil,
                    vwcSoil: lastReading.vwc_soil,
                    rssi: lastReading.rssi,
                    snr: lastReading.snr
                } : null,
                alertCount: activeAlerts.length
            };
        });

        // Estadísticas globales
        const globalStats = db.prepare(`
            SELECT
                COUNT(DISTINCT station_id) as active_stations,
                COUNT(*) as total_readings_today,
                (SELECT COUNT(*) FROM alerts WHERE acknowledged = 0) as active_alerts
            FROM readings
            WHERE timestamp >= datetime('now', '-24 hours')
        `).get();

        res.json({
            stations: enriched,
            global: globalStats,
            serverTime: new Date().toISOString()
        });
    } catch (error) {
        console.error('Error en dashboard:', error);
        res.status(500).json({ error: 'Error al obtener datos del dashboard' });
    }
});

/**
 * GET /api/alerts
 * Listar todas las alertas activas
 */
router.get('/alerts', (req, res) => {
    try {
        const { stationId, acknowledged = 'false' } = req.query;

        let query = `
            SELECT a.*, s.name as station_name
            FROM alerts a
            JOIN stations s ON a.station_id = s.id
            WHERE 1=1
        `;
        const params = [];

        if (stationId) {
            query += ' AND a.station_id = ?';
            params.push(stationId);
        }

        if (acknowledged === 'false') {
            query += ' AND a.acknowledged = 0';
        }

        query += ' ORDER BY a.created_at DESC LIMIT 100';

        const alertList = db.prepare(query).all(...params);
        res.json(alertList);
    } catch (error) {
        console.error('Error al obtener alertas:', error);
        res.status(500).json({ error: 'Error al obtener alertas' });
    }
});

/**
 * POST /api/alerts/:id/acknowledge
 * Marcar alerta como reconocida
 */
router.post('/alerts/:id/acknowledge', (req, res) => {
    try {
        alerts.acknowledge.run(req.params.id);
        res.json({ success: true });
    } catch (error) {
        console.error('Error al reconocer alerta:', error);
        res.status(500).json({ error: 'Error al reconocer alerta' });
    }
});

/**
 * GET /api/compare
 * Comparar datos entre estaciones
 */
router.get('/compare', (req, res) => {
    try {
        const { stationIds, metric = 'temp_air', period = '24h' } = req.query;

        if (!stationIds) {
            return res.status(400).json({ error: 'stationIds es requerido' });
        }

        const ids = stationIds.split(',');
        const periodMap = {
            '1h': '-1 hours',
            '24h': '-24 hours',
            '7d': '-7 days'
        };

        const sqlPeriod = periodMap[period] || periodMap['24h'];

        const result = ids.map(stationId => {
            const data = db.prepare(`
                SELECT timestamp, ${metric} as value
                FROM readings
                WHERE station_id = ? AND timestamp >= datetime('now', ?)
                ORDER BY timestamp ASC
            `).all(stationId.trim(), sqlPeriod);

            const station = stations.getById.get(stationId.trim());

            return {
                stationId: stationId.trim(),
                stationName: station?.name || stationId,
                data
            };
        });

        res.json(result);
    } catch (error) {
        console.error('Error al comparar:', error);
        res.status(500).json({ error: 'Error al comparar estaciones' });
    }
});

/**
 * DELETE /api/data/cleanup
 * Limpiar datos antiguos
 */
router.delete('/data/cleanup', (req, res) => {
    try {
        const { olderThan = '90' } = req.query; // días

        const result = readings.deleteOld.run(`-${olderThan} days`);

        res.json({
            success: true,
            deletedRows: result.changes
        });
    } catch (error) {
        console.error('Error en cleanup:', error);
        res.status(500).json({ error: 'Error al limpiar datos' });
    }
});

module.exports = router;
