/**
 * Authentication Middleware
 * Protects routes that require authentication
 */

/**
 * Middleware to require authentication
 * Blocks requests without valid session
 */
function requireAuth(req, res, next) {
    if (req.session && req.session.userId) {
        return next();
    }
    return res.status(401).json({ error: 'No autorizado. Inicie sesión.' });
}

/**
 * Middleware to require specific role
 * @param {string|string[]} roles - Required role(s)
 */
function requireRole(...roles) {
    return (req, res, next) => {
        if (!req.session || !req.session.userId) {
            return res.status(401).json({ error: 'No autorizado. Inicie sesión.' });
        }
        if (!roles.includes(req.session.userRole)) {
            return res.status(403).json({ error: 'Acceso denegado. Rol insuficiente.' });
        }
        return next();
    };
}

/**
 * Optional auth - attaches user info if logged in, but doesn't block
 */
function optionalAuth(req, res, next) {
    // Session info is already attached by express-session
    next();
}

module.exports = {
    requireAuth,
    requireRole,
    optionalAuth
};
