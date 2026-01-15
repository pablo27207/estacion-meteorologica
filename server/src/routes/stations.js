/**
 * Stations API Routes
 * CRUD de estaciones meteorológicas
 */

const express = require('express');
const router = express.Router();
const crypto = require('crypto');
const { stations, readings } = require('../models/database');

// Generar ID único para estación
function generateStationId() {
    return 'EST-' + crypto.randomBytes(4).toString('hex').toUpperCase();
}

// Generar API key para estación
function generateApiKey() {
    return crypto.randomBytes(32).toString('hex');
}

/**
 * GET /api/stations
 * Listar todas las estaciones activas
 */
router.get('/', (req, res) => {
    try {
        const allStations = stations.getAll.all();
        res.json(allStations);
    } catch (error) {
        console.error('Error al obtener estaciones:', error);
        res.status(500).json({ error: 'Error al obtener estaciones' });
    }
});

/**
 * GET /api/stations/:id
 * Obtener detalle de una estación
 */
router.get('/:id', (req, res) => {
    try {
        const station = stations.getById.get(req.params.id);
        if (!station) {
            return res.status(404).json({ error: 'Estación no encontrada' });
        }

        // Obtener última lectura
        const lastReading = readings.getLatest.get(req.params.id);

        // Obtener estadísticas del día
        const todayStats = readings.getStats.get(req.params.id, '-24 hours');

        res.json({
            ...station,
            lastReading,
            stats: {
                today: todayStats
            }
        });
    } catch (error) {
        console.error('Error al obtener estación:', error);
        res.status(500).json({ error: 'Error al obtener estación' });
    }
});

/**
 * POST /api/stations
 * Crear nueva estación
 */
router.post('/', (req, res) => {
    try {
        const { name, description, location_lat, location_lng, altitude } = req.body;

        if (!name) {
            return res.status(400).json({ error: 'Nombre es requerido' });
        }

        const id = generateStationId();
        const apiKey = generateApiKey();

        stations.create.run(
            id,
            name,
            description || null,
            location_lat || null,
            location_lng || null,
            altitude || null,
            apiKey
        );

        res.status(201).json({
            id,
            name,
            apiKey,
            message: 'Estación creada exitosamente. Guarde el API key, no se mostrará de nuevo.'
        });
    } catch (error) {
        console.error('Error al crear estación:', error);
        res.status(500).json({ error: 'Error al crear estación' });
    }
});

/**
 * PUT /api/stations/:id
 * Actualizar estación
 */
router.put('/:id', (req, res) => {
    try {
        const { name, description, location_lat, location_lng, altitude } = req.body;

        const station = stations.getById.get(req.params.id);
        if (!station) {
            return res.status(404).json({ error: 'Estación no encontrada' });
        }

        stations.update.run(
            name || station.name,
            description !== undefined ? description : station.description,
            location_lat !== undefined ? location_lat : station.location_lat,
            location_lng !== undefined ? location_lng : station.location_lng,
            altitude !== undefined ? altitude : station.altitude,
            req.params.id
        );

        res.json({ message: 'Estación actualizada' });
    } catch (error) {
        console.error('Error al actualizar estación:', error);
        res.status(500).json({ error: 'Error al actualizar estación' });
    }
});

/**
 * PUT /api/stations/:id/config
 * Actualizar configuración LoRa de la estación
 */
router.put('/:id/config', (req, res) => {
    try {
        const { sf, bw, interval } = req.body;

        const station = stations.getById.get(req.params.id);
        if (!station) {
            return res.status(404).json({ error: 'Estación no encontrada' });
        }

        // Validar parámetros
        if (sf && (sf < 7 || sf > 12)) {
            return res.status(400).json({ error: 'SF debe estar entre 7 y 12' });
        }
        if (bw && ![125, 250, 500].includes(bw)) {
            return res.status(400).json({ error: 'BW debe ser 125, 250 o 500' });
        }
        if (interval && interval < 10000) {
            return res.status(400).json({ error: 'Intervalo mínimo es 10000ms' });
        }

        stations.updateConfig.run(
            sf || station.config_sf,
            bw || station.config_bw,
            interval || station.config_interval,
            req.params.id
        );

        res.json({
            message: 'Configuración actualizada',
            pendingConfig: {
                sf: sf || station.config_sf,
                bw: bw || station.config_bw,
                interval: interval || station.config_interval
            }
        });
    } catch (error) {
        console.error('Error al actualizar config:', error);
        res.status(500).json({ error: 'Error al actualizar configuración' });
    }
});

/**
 * DELETE /api/stations/:id
 * Desactivar estación (soft delete)
 */
router.delete('/:id', (req, res) => {
    try {
        const station = stations.getById.get(req.params.id);
        if (!station) {
            return res.status(404).json({ error: 'Estación no encontrada' });
        }

        stations.delete.run(req.params.id);
        res.json({ message: 'Estación desactivada' });
    } catch (error) {
        console.error('Error al eliminar estación:', error);
        res.status(500).json({ error: 'Error al eliminar estación' });
    }
});

/**
 * POST /api/stations/:id/regenerate-key
 * Regenerar API key de una estación
 */
router.post('/:id/regenerate-key', (req, res) => {
    try {
        const station = stations.getById.get(req.params.id);
        if (!station) {
            return res.status(404).json({ error: 'Estación no encontrada' });
        }

        const newApiKey = generateApiKey();
        const updateKey = require('../models/database').db.prepare(
            'UPDATE stations SET api_key = ? WHERE id = ?'
        );
        updateKey.run(newApiKey, req.params.id);

        res.json({
            apiKey: newApiKey,
            message: 'API key regenerada. Actualice la configuración del gateway.'
        });
    } catch (error) {
        console.error('Error al regenerar key:', error);
        res.status(500).json({ error: 'Error al regenerar API key' });
    }
});

module.exports = router;
