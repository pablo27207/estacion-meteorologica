/**
 * GIPIS Weather Station Server
 * Servidor central para gestión de múltiples estaciones meteorológicas
 */

require('dotenv').config();
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const rateLimit = require('express-rate-limit');
const path = require('path');

// Routes
const stationsRouter = require('./routes/stations');
const dataRouter = require('./routes/data');
const apiRouter = require('./routes/api');

// Database
const db = require('./models/database');

const app = express();
const PORT = process.env.PORT || 3000;

// Security middleware
app.use(helmet({
    contentSecurityPolicy: {
        directives: {
            defaultSrc: ["'self'"],
            scriptSrc: ["'self'", "cdn.jsdelivr.net", "cdnjs.cloudflare.com"],
            styleSrc: ["'self'", "'unsafe-inline'", "cdnjs.cloudflare.com", "fonts.googleapis.com"],
            fontSrc: ["'self'", "fonts.gstatic.com", "cdnjs.cloudflare.com"],
            imgSrc: ["'self'", "data:", "https:"],
            connectSrc: ["'self'", "cdn.jsdelivr.net"]
        }
    }
}));

// Rate limiting para API de ingesta (estaciones)
const ingestLimiter = rateLimit({
    windowMs: 1 * 60 * 1000, // 1 minuto
    max: 60, // máx 60 requests por minuto por IP
    message: { error: 'Demasiadas solicitudes, intente más tarde' }
});

// Rate limiting general
const generalLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutos
    max: 1000
});

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(morgan('combined'));
app.use(generalLimiter);

// Servir archivos estáticos
app.use(express.static(path.join(__dirname, '../public')));

// API Routes
app.use('/api/stations', stationsRouter);
app.use('/api/data', ingestLimiter, dataRouter);
app.use('/api', apiRouter);

// Health check
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        timestamp: new Date().toISOString(),
        uptime: process.uptime()
    });
});

// SPA fallback - servir index.html para rutas no encontradas
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, '../public/index.html'));
});

// Error handler
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({ error: 'Error interno del servidor' });
});

// Iniciar servidor
app.listen(PORT, () => {
    console.log(`
╔════════════════════════════════════════════════════════════╗
║           GIPIS Weather Station Server v1.0.0              ║
╠════════════════════════════════════════════════════════════╣
║  Servidor corriendo en: http://localhost:${PORT}              ║
║  Ambiente: ${process.env.NODE_ENV || 'development'}                                ║
╚════════════════════════════════════════════════════════════╝
    `);
});

module.exports = app;
