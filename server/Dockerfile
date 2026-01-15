# ============================================
# GIPIS Weather Station Server
# Multi-stage build para imagen optimizada
# ============================================

# Stage 1: Dependencies
FROM node:20-alpine AS deps
WORKDIR /app

# Instalar dependencias de compilación para better-sqlite3
RUN apk add --no-cache python3 make g++

COPY package.json package-lock.json* ./
RUN npm ci --only=production

# Stage 2: Production
FROM node:20-alpine AS runner
WORKDIR /app

# Crear usuario no-root por seguridad
RUN addgroup --system --gid 1001 nodejs && \
    adduser --system --uid 1001 appuser

# Copiar dependencias compiladas
COPY --from=deps /app/node_modules ./node_modules
COPY . .

# Crear directorios necesarios con permisos correctos
RUN mkdir -p /app/data && \
    chown -R appuser:nodejs /app/data

# Variables de entorno por defecto
ENV NODE_ENV=production
ENV PORT=3000

# Cambiar a usuario no-root
USER appuser

EXPOSE 3000

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:3000/health || exit 1

CMD ["node", "src/server.js"]
