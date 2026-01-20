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
const session = require('express-session');
const SQLiteStore = require('connect-sqlite3')(session);

// Routes
const stationsRouter = require('./routes/stations');
const dataRouter = require('./routes/data');
const apiRouter = require('./routes/api');
const authRouter = require('./routes/auth');

// Database
const db = require('./models/database');

const app = express();
const PORT = process.env.PORT || 3000;

// Security middleware
app.use(helmet({
    contentSecurityPolicy: {
        directives: {
            defaultSrc: ["'self'"],
            scriptSrc: ["'self'", "'unsafe-inline'", "cdn.tailwindcss.com", "cdn.jsdelivr.net", "cdnjs.cloudflare.com", "unpkg.com"],
            styleSrc: ["'self'", "'unsafe-inline'", "cdnjs.cloudflare.com", "fonts.googleapis.com", "unpkg.com"],
            fontSrc: ["'self'", "fonts.gstatic.com", "cdnjs.cloudflare.com"],
            imgSrc: ["'self'", "data:", "https:", "*.tile.openstreetmap.org"],
            connectSrc: ["'self'", "cdn.jsdelivr.net", "*.tile.openstreetmap.org", "unpkg.com"]
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
app.use(cors({
    origin: true,
    credentials: true
}));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(morgan('combined'));
app.use(generalLimiter);

// Session middleware
app.use(session({
    store: new SQLiteStore({
        db: 'sessions.db',
        dir: path.join(__dirname, '../data')
    }),
    secret: process.env.SESSION_SECRET || 'gipis-weather-secret-dev',
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: process.env.NODE_ENV === 'production',
        httpOnly: true,
        maxAge: 24 * 60 * 60 * 1000 // 24 hours
    }
}));

// Base path para routing (vacío en dev, '/weather' en producción)
const BASE_PATH = process.env.BASE_PATH || '';

// Servir archivos estáticos (excluyendo index.html para manejarlo dinámicamente)
app.use(express.static(path.join(__dirname, '../public'), {
    index: false  // No servir index.html automáticamente
}));

// API Routes
app.use('/api/auth', authRouter);
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

// SPA fallback - servir index.html con BASE_PATH inyectado
const fs = require('fs');
const indexHtmlPath = path.join(__dirname, '../public/index.html');

app.get('*', (req, res) => {
    // Leer el HTML y reemplazar el base href dinámicamente
    let html = fs.readFileSync(indexHtmlPath, 'utf8');

    // Determinar el base href correcto
    // En dev: "/" | En producción: "/weather/"
    const baseHref = BASE_PATH ? `${BASE_PATH}/` : '/';
    html = html.replace(/<base href="[^"]*">/, `<base href="${baseHref}">`);

    res.type('html').send(html);
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
