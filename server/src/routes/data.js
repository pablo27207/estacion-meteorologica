/**
 * Data Ingestion API Routes
 * Endpoint para recibir datos de las estaciones
 */

const express = require('express');
const router = express.Router();
const { stations, readings, alerts } = require('../models/database');

// Umbrales de alerta por defecto
const ALERT_THRESHOLDS = {
    temp_air_high: 40,
    temp_air_low: -5,
    hum_air_high: 95,
    vwc_soil_low: 15,
    vwc_soil_high: 80,
    rssi_low: -120
};

/**
 * POST /api/data/ingest
 * Endpoint principal para recibir datos de las estaciones
 * Headers requeridos: X-API-Key
 */
router.post('/ingest', (req, res) => {
    try {
        // Verificar API key
        const apiKey = req.headers['x-api-key'];
        if (!apiKey) {
            return res.status(401).json({ error: 'API key requerida' });
        }

        // Buscar estación por API key
        const station = stations.getByApiKey.get(apiKey);
        if (!station) {
            return res.status(401).json({ error: 'API key inválida' });
        }

        // Extraer datos del body
        const {
            packetId,
            tempAire, humAire, tempSuelo, vwcSuelo,
            pressure, par, solarRadiation, precipitation,
            rssi, snr, freqError, batteryVoltage
        } = req.body;

        // Insertar lectura
        readings.insert.run(
            station.id,
            packetId || null,
            tempAire ?? null,
            humAire ?? null,
            tempSuelo ?? null,
            vwcSuelo ?? null,
            pressure ?? null,
            par ?? null,
            solarRadiation ?? null,
            precipitation ?? null,
            rssi ?? null,
            snr ?? null,
            freqError ?? null,
            batteryVoltage ?? null
        );

        // Actualizar last_seen de la estación
        stations.updateLastSeen.run(station.id);

        // Verificar alertas
        checkAndCreateAlerts(station.id, req.body);

        // Responder con configuración pendiente (si hay)
        res.json({
            success: true,
            timestamp: new Date().toISOString(),
            config: {
                sf: station.config_sf,
                bw: station.config_bw,
                interval: station.config_interval
            }
        });

    } catch (error) {
        console.error('Error en ingesta:', error);
        res.status(500).json({ error: 'Error al procesar datos' });
    }
});

/**
 * Verificar y crear alertas según umbrales
 */
function checkAndCreateAlerts(stationId, data) {
    const { tempAire, humAire, vwcSuelo, rssi } = data;

    // Temperatura aire alta
    if (tempAire !== undefined && tempAire > ALERT_THRESHOLDS.temp_air_high) {
        alerts.create.run(
            stationId,
            'TEMP_HIGH',
            `Temperatura del aire muy alta: ${tempAire}°C`,
            tempAire,
            ALERT_THRESHOLDS.temp_air_high
        );
    }

    // Temperatura aire baja (helada)
    if (tempAire !== undefined && tempAire < ALERT_THRESHOLDS.temp_air_low) {
        alerts.create.run(
            stationId,
            'FROST_WARNING',
            `Alerta de helada: ${tempAire}°C`,
            tempAire,
            ALERT_THRESHOLDS.temp_air_low
        );
    }

    // Humedad del suelo baja
    if (vwcSuelo !== undefined && vwcSuelo < ALERT_THRESHOLDS.vwc_soil_low) {
        alerts.create.run(
            stationId,
            'SOIL_DRY',
            `Suelo seco, considere riego: ${vwcSuelo}% VWC`,
            vwcSuelo,
            ALERT_THRESHOLDS.vwc_soil_low
        );
    }

    // Señal débil
    if (rssi !== undefined && rssi < ALERT_THRESHOLDS.rssi_low) {
        alerts.create.run(
            stationId,
            'SIGNAL_WEAK',
            `Señal LoRa débil: ${rssi} dBm`,
            rssi,
            ALERT_THRESHOLDS.rssi_low
        );
    }
}

/**
 * GET /api/data/:stationId/latest
 * Obtener última lectura de una estación
 */
router.get('/:stationId/latest', (req, res) => {
    try {
        const reading = readings.getLatest.get(req.params.stationId);
        if (!reading) {
            return res.status(404).json({ error: 'Sin datos disponibles' });
        }
        res.json(reading);
    } catch (error) {
        console.error('Error al obtener datos:', error);
        res.status(500).json({ error: 'Error al obtener datos' });
    }
});

/**
 * GET /api/data/:stationId/history
 * Obtener historial de lecturas
 * Query params: limit (default 100), from, to
 */
router.get('/:stationId/history', (req, res) => {
    try {
        const { limit = 100, from, to } = req.query;

        let data;
        if (from && to) {
            data = readings.getByStationRange.all(req.params.stationId, from, to);
        } else {
            data = readings.getByStation.all(req.params.stationId, parseInt(limit));
        }

        res.json(data);
    } catch (error) {
        console.error('Error al obtener historial:', error);
        res.status(500).json({ error: 'Error al obtener historial' });
    }
});

/**
 * GET /api/data/:stationId/stats
 * Obtener estadísticas agregadas
 * Query params: period (1h, 24h, 7d, 30d)
 */
router.get('/:stationId/stats', (req, res) => {
    try {
        const { period = '24h' } = req.query;

        const periodMap = {
            '1h': '-1 hours',
            '24h': '-24 hours',
            '7d': '-7 days',
            '30d': '-30 days'
        };

        const sqlPeriod = periodMap[period] || periodMap['24h'];
        const stats = readings.getStats.get(req.params.stationId, sqlPeriod);

        res.json({
            period,
            ...stats
        });
    } catch (error) {
        console.error('Error al obtener stats:', error);
        res.status(500).json({ error: 'Error al obtener estadísticas' });
    }
});

/**
 * GET /api/data/:stationId/export
 * Exportar datos en CSV
 */
router.get('/:stationId/export', (req, res) => {
    try {
        const { from, to, limit = 10000 } = req.query;

        let data;
        if (from && to) {
            data = readings.getByStationRange.all(req.params.stationId, from, to);
        } else {
            data = readings.getByStation.all(req.params.stationId, parseInt(limit));
        }

        // Generar CSV
        const headers = [
            'timestamp', 'packet_id', 'temp_air', 'hum_air',
            'temp_soil', 'vwc_soil', 'pressure', 'par',
            'solar_radiation', 'precipitation', 'rssi', 'snr'
        ];

        let csv = headers.join(',') + '\n';
        data.forEach(row => {
            csv += headers.map(h => row[h] ?? '').join(',') + '\n';
        });

        res.setHeader('Content-Type', 'text/csv');
        res.setHeader('Content-Disposition', `attachment; filename=station_${req.params.stationId}_data.csv`);
        res.send(csv);
    } catch (error) {
        console.error('Error al exportar:', error);
        res.status(500).json({ error: 'Error al exportar datos' });
    }
});

module.exports = router;
