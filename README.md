# GIPIS - Estación Meteorológica

Sistema de monitoreo meteorológico distribuido para agricultura de precisión. Utiliza comunicación LoRa para transmisión de datos desde sensores en campo hacia un gateway WiFi, con servidor web centralizado para gestión de múltiples estaciones.

## Arquitectura

```
┌─────────────────┐         LoRa 433MHz         ┌─────────────────┐
│   Nodo TX       │ ─────────────────────────▶  │   Nodo RX       │
│   (Campo)       │                             │   (Gateway)     │
│                 │                             │                 │
│ • Sensores      │                             │ • WiFi AP/STA   │
│ • SD Card       │                             │ • Dashboard     │
│ • OLED Display  │                             │ • HTTP Client   │
│ • Solar Power   │                             │                 │
└─────────────────┘                             └────────┬────────┘
                                                         │
                                                    HTTP/JSON
                                                         │
                                                         ▼
                                                ┌─────────────────┐
                                                │   Servidor Web  │
                                                │                 │
                                                │ • Multi-station │
                                                │ • Históricos    │
                                                │ • Alertas       │
                                                │ • API REST      │
                                                └─────────────────┘
```

## Estructura del Proyecto

```
estacion-meteorologica/
├── firmware/                    # Código ESP32 (PlatformIO)
│   ├── shared/                  # Configuración compartida
│   │   └── config.h            # Estructuras y constantes
│   ├── tx/                      # Nodo transmisor
│   │   ├── main.cpp            # Loop principal TX
│   │   ├── sensors.cpp/h       # Lectura de sensores
│   │   ├── lora.cpp/h          # Transmisión LoRa
│   │   ├── sd_logger.cpp/h     # Logging SD card
│   │   ├── display.cpp/h       # Interfaz OLED
│   │   └── button.cpp/h        # Control de botón
│   └── rx/                      # Nodo receptor
│       ├── main.cpp            # Loop principal RX
│       ├── lora.cpp/h          # Recepción LoRa
│       ├── wifi_manager.cpp/h  # WiFi + servidor local
│       ├── server_client.cpp/h # Cliente HTTP a servidor
│       ├── data_logger.cpp/h   # Logging LittleFS
│       ├── display.cpp/h       # Interfaz OLED
│       └── web_interface.h     # HTML embebido (legacy)
│
├── server/                      # Servidor web centralizado
│   ├── src/
│   │   ├── server.js           # Entry point Express
│   │   ├── routes/             # API endpoints
│   │   │   ├── stations.js     # CRUD estaciones
│   │   │   ├── data.js         # Ingesta y consulta
│   │   │   └── api.js          # Dashboard, alertas
│   │   ├── models/
│   │   │   └── database.js     # SQLite + queries
│   │   └── scripts/
│   │       └── init-db.js      # Inicialización DB
│   ├── public/                  # Frontend estático
│   │   ├── index.html
│   │   ├── css/app.css
│   │   └── js/app.js
│   ├── data/                    # Base de datos SQLite
│   ├── package.json
│   └── .env.example
│
├── docs/                        # Documentación
├── platformio.ini               # Configuración PlatformIO
└── README.md
```

## Hardware

### Board Recomendado
- **Heltec WiFi LoRa 32 V3/V4** (ESP32-S3 + SX1262)
- Alternativos: TTGO T-Beam, LilyGO T3-S3

### Sensores (Nodo TX)

| Sensor | Parámetro | Interfaz | Pin |
|--------|-----------|----------|-----|
| DHT22 | Temp/Hum aire | 1-Wire | GPIO 6 |
| DS18B20 | Temp suelo | OneWire | GPIO 7 |
| EC-5 | Humedad suelo (VWC) | Analógico | GPIO 5 |

### Pinout Heltec V3

```
LoRa SX1262:
  NSS  → GPIO 8
  DIO1 → GPIO 14
  RST  → GPIO 12
  BUSY → GPIO 13
  SCLK → GPIO 9
  MISO → GPIO 11
  MOSI → GPIO 10

OLED SSD1306:
  SDA → GPIO 17
  SCL → GPIO 18
  RST → GPIO 21

SD Card (TX):
  MOSI → GPIO 47
  MISO → GPIO 33
  SCK  → GPIO 48
  CS   → GPIO 26

User Button: GPIO 0
VEXT Control: GPIO 36
```

## Instalación

### 1. Firmware (PlatformIO)

```bash
# Clonar repositorio
git clone <repo-url>
cd estacion-meteorologica

# Compilar nodo TX
pio run -e tx_node

# Compilar nodo RX
pio run -e rx_gateway

# Subir a dispositivo
pio run -t upload -e tx_node
pio run -t upload -e rx_gateway
```

### 2. Servidor Web

```bash
cd server

# Instalar dependencias
npm install

# Copiar configuración
cp .env.example .env

# Inicializar base de datos
npm run init-db

# Iniciar servidor
npm start

# Desarrollo con hot-reload
npm run dev
```

El servidor estará disponible en `http://localhost:3000`

## Configuración

### Nodo TX
La configuración se realiza mediante el botón físico y la pantalla OLED:
- **Short press**: Cambiar pantalla
- **Long press** (en Config): Editar parámetro
- Parámetros: SF, BW, Intervalo de medición, Intervalo TX

### Nodo RX
1. Al primer inicio, crea punto de acceso WiFi: `GIPIS-Estacion`
2. Conectarse y acceder a `192.168.4.1`
3. Escanear y seleccionar red WiFi
4. Una vez conectado, acceder al dashboard local

### Servidor Web
1. Crear nueva estación desde el dashboard
2. Copiar el API Key generado
3. Configurar el RX para enviar datos al servidor

## API REST

### Endpoints Principales

```
# Estaciones
GET    /api/stations              # Listar estaciones
POST   /api/stations              # Crear estación
GET    /api/stations/:id          # Detalle estación
PUT    /api/stations/:id          # Actualizar estación
PUT    /api/stations/:id/config   # Configurar LoRa
DELETE /api/stations/:id          # Desactivar estación

# Datos
POST   /api/data/ingest           # Recibir datos (requiere X-API-Key)
GET    /api/data/:id/latest       # Última lectura
GET    /api/data/:id/history      # Histórico
GET    /api/data/:id/stats        # Estadísticas
GET    /api/data/:id/export       # Exportar CSV

# Dashboard
GET    /api/dashboard             # Vista general
GET    /api/alerts                # Alertas activas
POST   /api/alerts/:id/acknowledge
GET    /api/compare               # Comparar estaciones
```

### Ejemplo: Enviar datos desde Gateway

```bash
curl -X POST http://servidor:3000/api/data/ingest \
  -H "Content-Type: application/json" \
  -H "X-API-Key: tu-api-key" \
  -d '{
    "packetId": 123,
    "tempAire": 25.5,
    "humAire": 65.0,
    "tempSuelo": 20.1,
    "vwcSuelo": 35.2,
    "rssi": -95,
    "snr": 8.5
  }'
```

## Protocolo LoRa

### Configuración por Defecto
- **Frecuencia**: 433.0 MHz
- **Spreading Factor**: 9 (configurable 7-12)
- **Bandwidth**: 125 kHz (configurable 125/250/500)
- **Coding Rate**: 4/7
- **Sync Word**: 0x12
- **Potencia TX**: 22 dBm

### Estructura de Paquetes

**MeteorDataPacket** (TX → RX):
```cpp
struct MeteorDataPacket {
    float tempAire;           // °C
    float humAire;            // %
    float tempSuelo;          // °C
    float vwcSuelo;           // % VWC
    uint32_t rawSuelo;        // ADC raw
    unsigned long packetId;   // Contador
    uint32_t interval;        // Estado
};
```

**ConfigPacket** (RX → TX):
```cpp
struct ConfigPacket {
    uint32_t magic;           // 0xCAFEBABE
    uint32_t interval;        // ms
    uint8_t sf;               // 7-12
    float bw;                 // kHz
};
```

## Funcionalidades del Dashboard

- **Monitor en tiempo real**: Visualización de datos de todas las estaciones
- **Gráficos históricos**: Temperatura, humedad, VWC (24h)
- **Gestión de estaciones**: Crear, editar, configurar
- **Sistema de alertas**: Heladas, suelo seco, señal débil
- **Comparación**: Múltiples estaciones en un gráfico
- **Exportación**: Descarga CSV de históricos
- **Configuración remota**: Cambiar parámetros LoRa desde la web

## Alertas Automáticas

| Tipo | Condición | Umbral |
|------|-----------|--------|
| TEMP_HIGH | Temperatura aire alta | > 40°C |
| FROST_WARNING | Riesgo de helada | < -5°C |
| SOIL_DRY | Suelo seco | < 15% VWC |
| SIGNAL_WEAK | Señal débil | < -120 dBm |

## Desarrollo

### Requisitos
- PlatformIO CLI o IDE
- Node.js >= 18
- ESP32 con LoRa (Heltec V3/V4)

### Estructura de Base de Datos

```sql
-- Estaciones
stations (id, name, description, location, api_key, config_*)

-- Lecturas
readings (station_id, timestamp, temp_*, hum_*, rssi, snr)

-- Alertas
alerts (station_id, type, message, value, threshold, acknowledged)
```

## Roadmap

- [ ] Soporte para sensores adicionales (PAR, pluviómetro)
- [ ] Notificaciones push/email
- [ ] Integración con servicios externos (ThingSpeak, InfluxDB)
- [ ] App móvil
- [ ] Modo mesh LoRa para mayor cobertura
- [ ] Dashboard con mapas

## Licencia

MIT License - Ver archivo LICENSE

## Autores

Proyecto GIPIS - Grupo de Investigación en Producción e Industria Sustentable
