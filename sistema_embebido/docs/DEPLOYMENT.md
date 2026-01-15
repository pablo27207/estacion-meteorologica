# Guía de Despliegue - GIPIS Weather Station Server

## Requisitos Previos

- Docker y Docker Compose instalados
- Traefik configurado con red `traefik-public`
- Certificado SSL configurado en Traefik (wildcard o específico)
- Subdominio DNS apuntando al servidor

## Arquitectura de Despliegue

```
┌─────────────────────────────────────────────────────────────┐
│                        SERVIDOR                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   Internet                                                  │
│      │                                                      │
│      ▼                                                      │
│   ┌─────────┐                                               │
│   │ Traefik │ :80/:443                                      │
│   │ (proxy) │                                               │
│   └────┬────┘                                               │
│        │                                                    │
│        │ traefik-public network                             │
│        │                                                    │
│   ┌────┴────────────────────┐                               │
│   │                         │                               │
│   ▼                         ▼                               │
│ ┌──────────┐          ┌──────────────┐                      │
│ │ gipis-web│          │ gipis-weather│                      │
│ │ (Flask)  │          │ (Node.js)    │                      │
│ │ :5000    │          │ :3000        │                      │
│ └──────────┘          └──────────────┘                      │
│                              │                              │
│                              ▼                              │
│                       ┌────────────┐                        │
│                       │ SQLite DB  │                        │
│                       │ (volume)   │                        │
│                       └────────────┘                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Instalación Paso a Paso

### 1. Clonar el Repositorio

```bash
# En tu servidor
cd /opt  # o donde tengas tus proyectos
git clone <repo-url> estacion-meteorologica
cd estacion-meteorologica/server
```

### 2. Configurar Variables de Entorno

```bash
# Copiar archivo de ejemplo
cp .env.example .env

# Editar configuración
nano .env
```

Ajustar el dominio:
```env
WEATHER_DOMAIN=weather.gipis.unp.edu.ar
```

### 3. Verificar Red de Traefik

```bash
# La red debe existir (creada por tu compose principal)
docker network ls | grep traefik-public

# Si no existe, créala manualmente:
docker network create traefik-public
```

### 4. Configurar Certificado SSL (si es necesario)

Si usas certificados manuales, asegúrate de que Traefik tenga acceso al certificado para el nuevo subdominio.

**Opción A: Certificado Wildcard**
Si ya tienes `*.gipis.unp.edu.ar`, no necesitas hacer nada.

**Opción B: Certificado Específico**
Agregar al archivo `traefik-tls.yml`:

```yaml
tls:
  certificates:
    - certFile: /certs/gipis.unp.edu.ar.crt
      keyFile: /certs/gipis.unp.edu.ar.key
    # Agregar:
    - certFile: /certs/weather.gipis.unp.edu.ar.crt
      keyFile: /certs/weather.gipis.unp.edu.ar.key
```

### 5. Construir e Iniciar

```bash
# Construir imagen
docker compose build

# Iniciar en segundo plano
docker compose up -d

# Ver logs
docker compose logs -f
```

### 6. Inicializar Base de Datos

```bash
# Crear estación de demo (opcional)
docker compose exec weather-server npm run init-db
```

### 7. Verificar Funcionamiento

```bash
# Health check interno
docker compose exec weather-server wget -qO- http://localhost:3000/health

# Desde el navegador
# https://weather.gipis.unp.edu.ar
```

## Configuración DNS

Agregar registro A o CNAME en tu DNS:

```
weather.gipis.unp.edu.ar  →  IP_DEL_SERVIDOR
# o
weather.gipis.unp.edu.ar  →  CNAME → gipis.unp.edu.ar
```

## Comandos Útiles

```bash
# Ver estado
docker compose ps

# Ver logs en tiempo real
docker compose logs -f weather-server

# Reiniciar servicio
docker compose restart

# Detener
docker compose down

# Actualizar (rebuild)
git pull
docker compose build
docker compose up -d

# Acceder al contenedor
docker compose exec weather-server sh

# Backup de base de datos
docker compose exec weather-server cat /app/data/weather.db > backup.db
```

## Gestión de Datos

### Backup

```bash
# Copiar base de datos
docker cp gipis-weather:/app/data/weather.db ./backup-$(date +%Y%m%d).db
```

### Restaurar

```bash
# Detener servicio
docker compose stop

# Copiar backup
docker cp backup.db gipis-weather:/app/data/weather.db

# Reiniciar
docker compose start
```

### Limpieza de Datos Antiguos

La API tiene un endpoint para limpieza:

```bash
# Eliminar datos de más de 90 días
curl -X DELETE "https://weather.gipis.unp.edu.ar/api/data/cleanup?olderThan=90"
```

## Monitoreo

### Health Check

El contenedor incluye health check automático. Ver estado:

```bash
docker inspect gipis-weather --format='{{.State.Health.Status}}'
```

### Métricas

Endpoint de salud:
```bash
curl https://weather.gipis.unp.edu.ar/health
```

Respuesta:
```json
{
  "status": "ok",
  "timestamp": "2025-01-15T12:00:00.000Z",
  "uptime": 3600
}
```

## Troubleshooting

### El contenedor no inicia

```bash
# Ver logs de error
docker compose logs weather-server

# Errores comunes:
# - Puerto ya en uso → cambiar PORT en .env
# - Red no existe → crear traefik-public
```

### Error de conexión a Traefik

```bash
# Verificar que esté en la red correcta
docker network inspect traefik-public | grep gipis-weather

# Verificar labels
docker inspect gipis-weather --format='{{json .Config.Labels}}' | jq
```

### Error de permisos en base de datos

```bash
# Verificar permisos del volumen
docker compose exec weather-server ls -la /app/data

# Recrear volumen si es necesario
docker compose down -v
docker compose up -d
```

### Certificado SSL no funciona

```bash
# Verificar configuración de Traefik
docker logs traefik 2>&1 | grep -i "weather"

# Verificar que el dominio resuelve
nslookup weather.gipis.unp.edu.ar
```

## Actualización

```bash
cd /opt/estacion-meteorologica

# Obtener cambios
git pull origin main

# Reconstruir
docker compose build --no-cache

# Reiniciar con nueva imagen
docker compose up -d

# Verificar
docker compose logs -f
```

## Configuración de Estaciones

### Crear Nueva Estación

Desde el dashboard web o via API:

```bash
curl -X POST https://weather.gipis.unp.edu.ar/api/stations \
  -H "Content-Type: application/json" \
  -d '{"name": "Estación Campo Norte", "description": "Parcela experimental"}'
```

Respuesta:
```json
{
  "id": "EST-A1B2C3D4",
  "apiKey": "abc123...xyz789",
  "message": "Guarde el API key"
}
```

### Configurar Gateway ESP32

En el nodo RX, configurar:
- URL del servidor: `https://weather.gipis.unp.edu.ar`
- API Key: el generado al crear la estación

## Seguridad

- El servidor usa Helmet.js para headers de seguridad
- Rate limiting configurado (100 req/min por IP)
- API keys únicas por estación
- Sin acceso directo a base de datos desde exterior
- Comunicación HTTPS obligatoria via Traefik
