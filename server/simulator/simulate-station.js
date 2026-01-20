/**
 * Weather Station Simulator (Multi-Station Support)
 * Simula múltiples estaciones meteorológicas enviando datos al servidor
 */

const API_URL = process.env.API_URL || 'http://localhost:3000'; // Default to localhost for local dev if not link
// Nota: en docker-compose se usa http://weather-server:3000

const INTERVAL_MS = parseInt(process.env.INTERVAL_MS) || 60000; // 1 minuto por defecto

// Configuración de estaciones a simular
const STATIONS_CONFIG = [
    {
        name: 'UNPSJB',
        lat: -45.825770, // Modificado a pedido
        lng: -67.463514,
        alt: 10,
        description: 'Estación de desarrollo base'
    },
    {
        name: 'Puerto Comodoro',
        lat: -45.862186,
        lng: -67.463342,
        alt: 5,
        description: 'Estación zona portuaria'
    },
    {
        name: 'Aeropuerto CR',
        lat: -45.789840,
        lng: -67.468697,
        alt: 46,
        description: 'Estación aeroportuaria'
    },
    {
        name: 'Muelle Caleta Córdova',
        lat: -45.749207,
        lng: -67.368660,
        alt: 3,
        description: 'Muelle pesquero artesanal'
    },
    {
        name: 'Rada Tilly',
        lat: -45.915561,
        lng: -67.545051,
        alt: 15,
        description: 'Villa balnearia'
    }
];

class StationSimulator {
    constructor(config) {
        this.config = config;
        this.apiKey = null;
        this.state = {
            tempAire: 15 + (Math.random() - 0.5) * 5, // Variación inicial
            humAire: 60 + (Math.random() - 0.5) * 20,
            tempSuelo: 12 + (Math.random() - 0.5) * 3,
            vwcSuelo: 30 + (Math.random() - 0.5) * 10,
            pressure: 1013.25,
            par: 0,
            solarRadiation: 0,
            precipitation: 0,
            rssi: -85 + (Math.random() - 0.5) * 10,
            snr: 8,
            freqError: 0,
            batteryVoltage: 3.7 + Math.random() * 0.4,
            packetId: 0
        };
    }

    vary(value, min, max, maxChange) {
        const change = (Math.random() - 0.5) * 2 * maxChange;
        return Math.max(min, Math.min(max, value + change));
    }

    getHourFactor() {
        const hour = new Date().getHours();
        if (hour >= 6 && hour <= 18) {
            return Math.sin((hour - 6) * Math.PI / 12);
        }
        return 0;
    }

    updateState() {
        const hourFactor = this.getHourFactor();
        const isDay = hourFactor > 0;

        // Add some noise based on station location (pseudo-random but stable behavior)
        const locationNoise = (this.config.lat + this.config.lng) % 1;

        // Temp Aire
        let baseTemp = 10 + hourFactor * 15 + locationNoise * 2;
        // Puerto/costa: menor amplitud térmica (simulado)
        if (this.config.alt < 10) baseTemp *= 0.95;

        this.state.tempAire = this.vary(this.state.tempAire, -5, 35, 0.5);
        this.state.tempAire = this.state.tempAire * 0.7 + baseTemp * 0.3;

        // Humedad
        this.state.humAire = this.vary(this.state.humAire, 20, 100, 2);
        if (this.state.tempAire > 25) this.state.humAire = Math.max(20, this.state.humAire - 1);

        // Temp Suelo
        this.state.tempSuelo = this.vary(this.state.tempSuelo, 0, 30, 0.2);
        this.state.tempSuelo = this.state.tempSuelo * 0.9 + this.state.tempAire * 0.6 * 0.1;

        // Humedad Suelo
        this.state.vwcSuelo = this.vary(this.state.vwcSuelo, 5, 85, 0.5);
        if (isDay && this.state.solarRadiation > 400) this.state.vwcSuelo -= 0.1;

        // Presión
        this.state.pressure = this.vary(this.state.pressure, 980, 1040, 0.5);

        // Radiación
        if (isDay) {
            this.state.par = this.vary(this.state.par, 0, 2000, 50);
            this.state.par = this.state.par * 0.5 + hourFactor * 1500 * 0.5;
            this.state.solarRadiation = this.state.par * 0.5;
        } else {
            this.state.par = 0;
            this.state.solarRadiation = 0;
        }

        // Precipitación
        if (Math.random() < 0.02) {
            this.state.precipitation = Math.random() * 5;
            this.state.humAire = Math.min(100, this.state.humAire + 10);
        } else {
            this.state.precipitation = 0;
        }

        // LoRa signal
        this.state.rssi = this.vary(this.state.rssi, -120, -50, 2);

        // Batería
        if (isDay && this.state.solarRadiation > 200) {
            this.state.batteryVoltage = Math.min(4.2, this.state.batteryVoltage + 0.005);
        } else {
            this.state.batteryVoltage = Math.max(3.0, this.state.batteryVoltage - 0.001);
        }

        this.state.packetId++;
    }

    roundState() {
        const s = this.state;
        return {
            packetId: s.packetId,
            tempAire: Math.round(s.tempAire * 10) / 10,
            humAire: Math.round(s.humAire * 10) / 10,
            tempSuelo: Math.round(s.tempSuelo * 10) / 10,
            vwcSuelo: Math.round(s.vwcSuelo * 10) / 10,
            pressure: Math.round(s.pressure * 10) / 10,
            par: Math.round(s.par),
            solarRadiation: Math.round(s.solarRadiation),
            precipitation: Math.round(s.precipitation * 10) / 10,
            rssi: Math.round(s.rssi),
            snr: Math.round(s.snr * 10) / 10,
            freqError: Math.round(s.freqError),
            batteryVoltage: Math.round(s.batteryVoltage * 100) / 100
        };
    }

    async ensureStation() {
        // En un entorno real, el simulador guardaría su API key en disco si se reinicia.
        // Aquí intentamos obtenerla o crearla. 
        // Si el nombre es único, el servidor debería devolver error o nosotros manejar la persistencia.
        // Para simplificar: intentamos "crear". Si el servidor maneja duplicados devolviendo la existente, genial.
        // Si no, tendremos que manejar el error "Station name already exists" buscando su ID... 
        // (El backend actual no tiene endpoint de búsqueda por nombre, asumiremos creación idempotent o fallo).

        // Revisando server.js/routes/stations.js (no visible aquí pero asumido comportamiento estándar)
        // Intentaremos crear.

        console.log(`Verificando estación: ${this.config.name}...`);

        try {
            const response = await fetch(`${API_URL}/api/stations`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: this.config.name,
                    description: this.config.description,
                    location_lat: this.config.lat,
                    location_lng: this.config.lng,
                    altitude: this.config.alt
                })
            });

            if (response.ok) {
                const result = await response.json();
                this.apiKey = result.apiKey;
                console.log(`✓ Estación lista: ${this.config.name} (ID: ${result.id})`);
            } else {
                // Si falla (ej: ya existe), necesitamos una forma de obtener la API key.
                // Como no tenemos DB access acá, y asumimos que es entorno DEV,
                // Imprimimos advertencia.
                // En un setup ideal, usaríamos una API KEY maestra o seeds.
                // Workaround: El backend debería devolver la API key si ya existe para simplificar dev?
                // O el simulador debería persistir las keys localmente en un json.
                console.warn(`! No se pudo crear (¿ya existe?): ${this.config.name}. Status: ${response.status}`);
                // Si no tenemos API key, no podemos enviar datos. 
                // Intentaremos continuar solo si logramos obtener key.
                const txt = await response.text();
                // Si el mensaje contiene la key (poco seguro pero útil en dev) o si ya la tenemos.
                // Por ahora, si falla, esta estación no transmitirá en esta ejecución a menos que
                // implementemos persistencia de keys en 'simulator/keys.json'.
            }
        } catch (error) {
            console.error(`✗ Error conectando para ${this.config.name}:`, error.message);
        }
    }

    async sendData() {
        if (!this.apiKey) return;

        this.updateState();
        const data = this.roundState();

        try {
            const response = await fetch(`${API_URL}/api/data/ingest`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-API-Key': this.apiKey
                },
                body: JSON.stringify(data)
            });
            if (response.ok) {
                // Silencioso para no saturar logs
            } else {
                console.error(`Error enviando ${this.config.name}: ${response.status}`);
            }
        } catch (error) {
            console.error(`Error red ${this.config.name}: ${error.message}`);
        }
    }
}

// Persistencia simple de keys en memoria o archivo si fuera necesario.
// Mejor estrategia para este script de dev: 
// 1. Obtener lista de estaciones del server.
// 2. Si existen por nombre, usar su ID/Key (si el server lo expone).
// Como el server no expone API Key por seguridad en lista pública...
// Modificaremos la estrategia:
// El server devuelve la API Key SOLO al crear.
// Guardaremos las keys en un archivo local `keys.json` para reutilizarlas entre reinicios del script.

const fs = require('fs');
const path = require('path');
const KEYS_FILE = path.join(__dirname, 'keys.json');

function loadKeys() {
    try {
        if (fs.existsSync(KEYS_FILE)) {
            return JSON.parse(fs.readFileSync(KEYS_FILE, 'utf8'));
        }
    } catch (e) {
        console.error("Error leyendo keys.json", e);
    }
    return {};
}

function saveKeys(keys) {
    try {
        fs.writeFileSync(KEYS_FILE, JSON.stringify(keys, null, 2));
    } catch (e) {
        console.error("Error guardando keys.json", e);
    }
}

async function waitForServer(maxRetries = 30) {
    console.log(`Esperando servidor en ${API_URL}...`);
    for (let i = 0; i < maxRetries; i++) {
        try {
            const res = await fetch(`${API_URL}/health`);
            if (res.ok) return true;
        } catch (e) { }
        await new Promise(r => setTimeout(r, 2000));
    }
    throw new Error('Timeout esperando servidor');
}

async function main() {
    console.log('=== Multi-Station Simulator ===');
    await waitForServer();

    const storedKeys = loadKeys();
    const simulators = [];

    for (const cfg of STATIONS_CONFIG) {
        const sim = new StationSimulator(cfg);

        // Recuperar key conocida o crear estación
        if (storedKeys[cfg.name]) {
            sim.apiKey = storedKeys[cfg.name];
            console.log(`Recuperada key para ${cfg.name}`);
        } else {
            await sim.ensureStation();
            if (sim.apiKey) {
                storedKeys[cfg.name] = sim.apiKey;
                saveKeys(storedKeys);
            }
        }

        if (sim.apiKey) {
            simulators.push(sim);
            // Enviar primer dato
            await sim.sendData();
        }
    }

    console.log(`\nSimulando ${simulators.length} estaciones activas.`);
    console.log(`Intervalo: ${INTERVAL_MS / 1000}s`);

    setInterval(() => {
        console.log(`[${new Date().toLocaleTimeString()}] Enviando datos broadcast...`);
        simulators.forEach(sim => sim.sendData());
    }, INTERVAL_MS);
}

main().catch(console.error);
