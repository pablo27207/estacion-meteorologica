/**
 * Firmware OTA API Routes
 * 
 * Endpoints for firmware version check and download
 */

const express = require('express');
const router = express.Router();
const path = require('path');
const fs = require('fs');

// Firmware directory
const FIRMWARE_DIR = path.join(__dirname, '../../firmware');
const VERSION_FILE = path.join(FIRMWARE_DIR, 'version.json');

// API Key validation middleware
const validateApiKey = (req, res, next) => {
    const apiKey = req.headers['x-api-key'];
    const validKey = process.env.STATION_API_KEY || '99d6d311131e329f5c2eff3fe3eb012453e07f66cc299ef58fcaedc6cc7ba30a';
    
    if (!apiKey || apiKey !== validKey) {
        return res.status(401).json({ error: 'Invalid API key' });
    }
    next();
};

/**
 * GET /api/firmware/version
 * Returns current firmware version info
 */
router.get('/version', validateApiKey, (req, res) => {
    try {
        if (!fs.existsSync(VERSION_FILE)) {
            return res.status(404).json({ error: 'No firmware available' });
        }
        
        const versionData = JSON.parse(fs.readFileSync(VERSION_FILE, 'utf8'));
        
        // Return version info (without sensitive data)
        res.json({
            version: versionData.version,
            description: versionData.description,
            date: versionData.date,
            size: versionData.size,
            minVersion: versionData.minVersion || '0.0.0'
        });
    } catch (err) {
        console.error('[Firmware] Error reading version:', err);
        res.status(500).json({ error: 'Error reading firmware version' });
    }
});

/**
 * GET /api/firmware/download
 * Downloads the firmware binary
 */
router.get('/download', validateApiKey, (req, res) => {
    try {
        if (!fs.existsSync(VERSION_FILE)) {
            return res.status(404).json({ error: 'No firmware available' });
        }
        
        const versionData = JSON.parse(fs.readFileSync(VERSION_FILE, 'utf8'));
        const firmwarePath = path.join(FIRMWARE_DIR, versionData.filename);
        
        if (!fs.existsSync(firmwarePath)) {
            return res.status(404).json({ error: 'Firmware file not found' });
        }
        
        // Set headers for binary download
        const stats = fs.statSync(firmwarePath);
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Length', stats.size);
        res.setHeader('Content-Disposition', `attachment; filename="${versionData.filename}"`);
        res.setHeader('X-Firmware-Version', versionData.version);
        
        // Stream the file
        const stream = fs.createReadStream(firmwarePath);
        stream.pipe(res);
        
        console.log(`[Firmware] Download started: ${versionData.filename} (${stats.size} bytes)`);
    } catch (err) {
        console.error('[Firmware] Error downloading:', err);
        res.status(500).json({ error: 'Error downloading firmware' });
    }
});

/**
 * GET /api/firmware/info
 * Public endpoint - returns basic info without auth (for web dashboard)
 */
router.get('/info', (req, res) => {
    try {
        if (!fs.existsSync(VERSION_FILE)) {
            return res.json({ available: false });
        }
        
        const versionData = JSON.parse(fs.readFileSync(VERSION_FILE, 'utf8'));
        res.json({
            available: true,
            version: versionData.version,
            date: versionData.date,
            description: versionData.description
        });
    } catch (err) {
        res.json({ available: false });
    }
});

module.exports = router;
