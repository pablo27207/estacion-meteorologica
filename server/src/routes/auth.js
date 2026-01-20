/**
 * Authentication Routes
 * Handles user registration, login, logout
 */

const express = require('express');
const bcrypt = require('bcryptjs');
const router = express.Router();

const db = require('../models/database');
const { requireAuth } = require('../middleware/auth');

// Password hashing rounds
const SALT_ROUNDS = 10;

/**
 * POST /api/auth/register
 * Register a new user
 */
router.post('/register', async (req, res) => {
    try {
        const { username, email, password } = req.body;

        // Validation
        if (!username || !email || !password) {
            return res.status(400).json({ error: 'Todos los campos son requeridos' });
        }

        if (password.length < 6) {
            return res.status(400).json({ error: 'La contraseña debe tener al menos 6 caracteres' });
        }

        // Check if user exists
        const existingEmail = db.users.getByEmail.get(email);
        if (existingEmail) {
            return res.status(400).json({ error: 'El email ya está registrado' });
        }

        const existingUsername = db.users.getByUsername.get(username);
        if (existingUsername) {
            return res.status(400).json({ error: 'El nombre de usuario ya existe' });
        }

        // Hash password
        const passwordHash = await bcrypt.hash(password, SALT_ROUNDS);

        // Create user (default role: viewer)
        const result = db.users.create.run(username, email, passwordHash, 'viewer');

        // Auto-login after registration
        const newUser = db.users.getById.get(result.lastInsertRowid);
        req.session.userId = newUser.id;
        req.session.userRole = newUser.role;
        req.session.username = newUser.username;

        res.status(201).json({
            message: 'Usuario registrado exitosamente',
            user: {
                id: newUser.id,
                username: newUser.username,
                email: newUser.email,
                role: newUser.role
            }
        });
    } catch (error) {
        console.error('Registration error:', error);
        res.status(500).json({ error: 'Error al registrar usuario' });
    }
});

/**
 * POST /api/auth/login
 * Authenticate user and create session
 */
router.post('/login', async (req, res) => {
    try {
        const { email, password } = req.body;

        if (!email || !password) {
            return res.status(400).json({ error: 'Email y contraseña son requeridos' });
        }

        // Find user by email
        const user = db.users.getByEmail.get(email);
        if (!user) {
            return res.status(401).json({ error: 'Credenciales inválidas' });
        }

        // Verify password
        const isValid = await bcrypt.compare(password, user.password_hash);
        if (!isValid) {
            return res.status(401).json({ error: 'Credenciales inválidas' });
        }

        // Update last login
        db.users.updateLastLogin.run(user.id);

        // Create session
        req.session.userId = user.id;
        req.session.userRole = user.role;
        req.session.username = user.username;

        res.json({
            message: 'Sesión iniciada',
            user: {
                id: user.id,
                username: user.username,
                email: user.email,
                role: user.role
            }
        });
    } catch (error) {
        console.error('Login error:', error);
        res.status(500).json({ error: 'Error al iniciar sesión' });
    }
});

/**
 * POST /api/auth/logout
 * Destroy session
 */
router.post('/logout', (req, res) => {
    req.session.destroy((err) => {
        if (err) {
            return res.status(500).json({ error: 'Error al cerrar sesión' });
        }
        res.clearCookie('connect.sid');
        res.json({ message: 'Sesión cerrada' });
    });
});

/**
 * GET /api/auth/me
 * Get current user info
 */
router.get('/me', requireAuth, (req, res) => {
    const user = db.users.getById.get(req.session.userId);
    if (!user) {
        return res.status(404).json({ error: 'Usuario no encontrado' });
    }
    res.json({
        id: user.id,
        username: user.username,
        email: user.email,
        role: user.role,
        created_at: user.created_at,
        last_login: user.last_login
    });
});

/**
 * GET /api/auth/status
 * Check if user is authenticated (no auth required)
 */
router.get('/status', (req, res) => {
    if (req.session && req.session.userId) {
        res.json({
            authenticated: true,
            user: {
                id: req.session.userId,
                username: req.session.username,
                role: req.session.userRole
            }
        });
    } else {
        res.json({ authenticated: false });
    }
});

module.exports = router;
