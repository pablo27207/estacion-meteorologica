/**
 * GIPIS Weather Station - Frontend Application
 */

// State
let state = {
    stations: [],
    selectedStation: null,
    charts: {},
    refreshInterval: null
};

// Chart colors
const COLORS = [
    '#3b82f6', '#06b6d4', '#f59e0b', '#10b981',
    '#8b5cf6', '#ec4899', '#f43f5e', '#14b8a6'
];

/**
 * API helper - construye URLs relativas correctamente
 * Funciona tanto con subdominio como con path-based routing
 */
function api(path) {
    // Remover / inicial si existe para hacer la ruta relativa
    const cleanPath = path.startsWith('/') ? path.slice(1) : path;
    return cleanPath;
}

// Initialize on load
document.addEventListener('DOMContentLoaded', () => {
    initCharts();
    loadDashboard();
    startAutoRefresh();

    // Form handlers
    document.getElementById('add-station-form').addEventListener('submit', handleAddStation);

    // Station selector change handler
    document.getElementById('station-selector').addEventListener('change', function () {
        selectStation(this.value);
    });

    // Event delegation for all data-action elements
    document.body.addEventListener('click', function (e) {
        const target = e.target.closest('[data-action]');
        if (!target) return;

        const action = target.dataset.action;

        switch (action) {
            case 'showView':
                showView(target.dataset.view);
                break;
            case 'toggleSidebar':
                toggleSidebar();
                break;
            case 'refreshData':
                refreshData();
                break;
            case 'showAddStationModal':
                showAddStationModal();
                break;
            case 'closeModal':
                closeModal(target.dataset.modal);
                break;
            case 'loadComparison':
                loadComparison();
                break;
            case 'refreshAlerts':
                refreshAlerts();
                break;
            case 'copyApiKey':
                copyApiKey();
                break;
            case 'saveStationConfig':
                saveStationConfig(target.dataset.stationId);
                break;
            case 'acknowledgeAlert':
                acknowledgeAlert(target.dataset.alertId);
                break;
            case 'selectStationSettings':
                selectStation(target.dataset.stationId);
                showView('settings');
                break;
        }
    });
});

/**
 * Initialize Chart.js charts
 */
function initCharts() {
    const commonOpts = {
        responsive: true,
        maintainAspectRatio: true,
        scales: {
            x: { display: true, grid: { display: false } },
            y: { display: true, grid: { color: '#f1f5f9' } }
        },
        elements: { point: { radius: 2, hitRadius: 10 }, line: { tension: 0.3 } },
        plugins: { legend: { display: false } }
    };

    const chartConfigs = [
        { id: 'chart-temp-air', label: 'Temp Aire', color: '#3b82f6' },
        { id: 'chart-hum-air', label: 'Hum Aire', color: '#06b6d4' },
        { id: 'chart-temp-soil', label: 'Temp Suelo', color: '#f59e0b' },
        { id: 'chart-vwc-soil', label: 'VWC Suelo', color: '#10b981' }
    ];

    chartConfigs.forEach(cfg => {
        const ctx = document.getElementById(cfg.id);
        if (ctx) {
            state.charts[cfg.id] = new Chart(ctx.getContext('2d'), {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: cfg.label,
                        data: [],
                        borderColor: cfg.color,
                        backgroundColor: cfg.color + '20',
                        fill: true
                    }]
                },
                options: commonOpts
            });
        }
    });

    // Compare chart
    const compareCtx = document.getElementById('chart-compare');
    if (compareCtx) {
        state.charts['compare'] = new Chart(compareCtx.getContext('2d'), {
            type: 'line',
            data: { labels: [], datasets: [] },
            options: {
                ...commonOpts,
                plugins: { legend: { display: true, position: 'top' } }
            }
        });
    }
}

/**
 * Load dashboard data
 */
async function loadDashboard() {
    try {
        const response = await fetch(api('/api/dashboard'));
        const data = await response.json();

        state.stations = data.stations;

        // Update connection status
        updateConnectionStatus(true);

        // Update station selector
        updateStationSelector();

        // Update global stats (from first station or selected)
        const targetStation = state.selectedStation
            ? data.stations.find(s => s.id === state.selectedStation)
            : data.stations[0];

        if (targetStation?.lastReading) {
            updateValueCard('val-temp-air', targetStation.lastReading.tempAir);
            updateValueCard('val-hum-air', targetStation.lastReading.humAir);
            updateValueCard('val-temp-soil', targetStation.lastReading.tempSoil);
            updateValueCard('val-vwc-soil', targetStation.lastReading.vwcSoil);
        }

        // Update stations grid
        renderStationsGrid(data.stations);

        // Update alert badge
        const totalAlerts = data.stations.reduce((sum, s) => sum + s.alertCount, 0);
        updateAlertBadge(totalAlerts);

        // Update last update text
        document.getElementById('last-update-text').textContent =
            `Última actualización: ${new Date().toLocaleTimeString()}`;

        // Load charts for selected station
        if (state.selectedStation) {
            loadStationCharts(state.selectedStation);
        } else if (data.stations.length > 0) {
            loadStationCharts(data.stations[0].id);
        }

    } catch (error) {
        console.error('Error loading dashboard:', error);
        updateConnectionStatus(false);
    }
}

/**
 * Update value cards with animation
 */
function updateValueCard(elementId, value) {
    const el = document.getElementById(elementId);
    if (el && value !== null && value !== undefined) {
        el.textContent = value.toFixed(1);
    }
}

/**
 * Update connection status indicator
 */
function updateConnectionStatus(connected) {
    const indicator = document.getElementById('connection-indicator');
    const text = document.getElementById('connection-text');
    const mobile = document.getElementById('mobile-status');

    if (connected) {
        indicator.className = 'w-3 h-3 rounded-full bg-green-500 animate-pulse';
        text.textContent = 'Conectado';
        mobile.className = 'w-3 h-3 rounded-full bg-green-500';
    } else {
        indicator.className = 'w-3 h-3 rounded-full bg-red-500';
        text.textContent = 'Sin conexión';
        mobile.className = 'w-3 h-3 rounded-full bg-red-500';
    }
}

/**
 * Update station selector dropdown
 */
function updateStationSelector() {
    const selector = document.getElementById('station-selector');
    const compareSelect = document.getElementById('compare-stations');

    // Clear and rebuild
    selector.innerHTML = '<option value="">Todas las estaciones</option>';

    state.stations.forEach(station => {
        selector.innerHTML += `<option value="${station.id}">${station.name}</option>`;
        if (compareSelect) {
            compareSelect.innerHTML += `<option value="${station.id}">${station.name}</option>`;
        }
    });

    if (state.selectedStation) {
        selector.value = state.selectedStation;
    }
}

/**
 * Render stations grid cards
 */
function renderStationsGrid(stations) {
    const grid = document.getElementById('stations-grid');
    grid.innerHTML = '';

    stations.forEach(station => {
        const statusColor = {
            'online': 'bg-green-500',
            'delayed': 'bg-yellow-500',
            'offline': 'bg-red-500'
        }[station.connectionStatus] || 'bg-gray-500';

        const card = document.createElement('div');
        card.className = 'bg-white rounded-xl shadow-sm p-5 hover:shadow-md transition cursor-pointer';
        card.onclick = () => selectStation(station.id);

        card.innerHTML = `
            <div class="flex items-center justify-between mb-4">
                <div class="flex items-center gap-3">
                    <div class="w-10 h-10 bg-slate-100 rounded-lg flex items-center justify-center">
                        <i class="fas fa-broadcast-tower text-slate-600"></i>
                    </div>
                    <div>
                        <h3 class="font-semibold text-slate-800">${station.name}</h3>
                        <p class="text-xs text-slate-400">${station.id}</p>
                    </div>
                </div>
                <span class="w-3 h-3 rounded-full ${statusColor}"></span>
            </div>

            ${station.lastReading ? `
                <div class="grid grid-cols-2 gap-3 text-sm">
                    <div class="bg-slate-50 rounded-lg p-2">
                        <span class="text-slate-400 text-xs">Temp</span>
                        <p class="font-semibold">${station.lastReading.tempAir?.toFixed(1) ?? '--'}°C</p>
                    </div>
                    <div class="bg-slate-50 rounded-lg p-2">
                        <span class="text-slate-400 text-xs">Hum</span>
                        <p class="font-semibold">${station.lastReading.humAir?.toFixed(1) ?? '--'}%</p>
                    </div>
                    <div class="bg-slate-50 rounded-lg p-2">
                        <span class="text-slate-400 text-xs">Suelo</span>
                        <p class="font-semibold">${station.lastReading.vwcSoil?.toFixed(1) ?? '--'}%</p>
                    </div>
                    <div class="bg-slate-50 rounded-lg p-2">
                        <span class="text-slate-400 text-xs">RSSI</span>
                        <p class="font-semibold">${station.lastReading.rssi?.toFixed(0) ?? '--'} dBm</p>
                    </div>
                </div>
            ` : `
                <p class="text-slate-400 text-sm text-center py-4">Sin datos recientes</p>
            `}

            ${station.alertCount > 0 ? `
                <div class="mt-3 flex items-center gap-2 text-amber-600 text-sm">
                    <i class="fas fa-exclamation-triangle"></i>
                    <span>${station.alertCount} alerta(s) activa(s)</span>
                </div>
            ` : ''}
        `;

        grid.appendChild(card);
    });
}

/**
 * Select a station
 */
function selectStation(stationId) {
    state.selectedStation = stationId || null;
    document.getElementById('station-selector').value = stationId || '';

    // Update dashboard values for selected station
    const station = state.stations.find(s => s.id === stationId);
    if (station?.lastReading) {
        updateValueCard('val-temp-air', station.lastReading.tempAir);
        updateValueCard('val-hum-air', station.lastReading.humAir);
        updateValueCard('val-temp-soil', station.lastReading.tempSoil);
        updateValueCard('val-vwc-soil', station.lastReading.vwcSoil);
    }

    if (stationId) {
        loadStationCharts(stationId);
        loadStationConfig(stationId);
    }
}

/**
 * Load charts for a specific station
 */
async function loadStationCharts(stationId) {
    try {
        const response = await fetch(api(`/api/data/${stationId}/history?limit=100`));
        const data = await response.json();

        // Reverse to get chronological order
        data.reverse();

        const labels = data.map(r => new Date(r.timestamp).toLocaleTimeString());

        // Update each chart
        updateChart('chart-temp-air', labels, data.map(r => r.temp_air));
        updateChart('chart-hum-air', labels, data.map(r => r.hum_air));
        updateChart('chart-temp-soil', labels, data.map(r => r.temp_soil));
        updateChart('chart-vwc-soil', labels, data.map(r => r.vwc_soil));

    } catch (error) {
        console.error('Error loading charts:', error);
    }
}

/**
 * Update a chart with new data
 */
function updateChart(chartId, labels, data) {
    const chart = state.charts[chartId];
    if (chart) {
        chart.data.labels = labels;
        chart.data.datasets[0].data = data;
        chart.update();
    }
}

/**
 * Load station configuration
 */
async function loadStationConfig(stationId) {
    try {
        const response = await fetch(api(`/api/stations/${stationId}`));
        const station = await response.json();

        const form = document.getElementById('station-config-form');
        form.innerHTML = `
            <div class="space-y-4">
                <div>
                    <label class="block text-sm font-semibold text-slate-600 mb-1">Nombre</label>
                    <input type="text" id="cfg-name" value="${station.name}" class="w-full border rounded-lg px-3 py-2">
                </div>
                <div class="grid grid-cols-3 gap-4">
                    <div>
                        <label class="block text-sm font-semibold text-slate-600 mb-1">Spreading Factor</label>
                        <select id="cfg-sf" class="w-full border rounded-lg px-3 py-2">
                            ${[7, 8, 9, 10, 11, 12].map(sf =>
            `<option value="${sf}" ${station.config_sf === sf ? 'selected' : ''}>SF${sf}</option>`
        ).join('')}
                        </select>
                    </div>
                    <div>
                        <label class="block text-sm font-semibold text-slate-600 mb-1">Bandwidth</label>
                        <select id="cfg-bw" class="w-full border rounded-lg px-3 py-2">
                            ${[125, 250, 500].map(bw =>
            `<option value="${bw}" ${station.config_bw === bw ? 'selected' : ''}>${bw} kHz</option>`
        ).join('')}
                        </select>
                    </div>
                    <div>
                        <label class="block text-sm font-semibold text-slate-600 mb-1">Intervalo (min)</label>
                        <input type="number" id="cfg-interval" value="${(station.config_interval / 60000).toFixed(1)}"
                            step="0.5" min="0.1" class="w-full border rounded-lg px-3 py-2">
                    </div>
                </div>
                <div class="flex gap-3 pt-4">
                    <button data-action="saveStationConfig" data-station-id="${stationId}"
                        class="bg-blue-600 text-white px-6 py-2 rounded-lg hover:bg-blue-700 transition">
                        Guardar Configuración
                    </button>
                    <a href="api/data/${stationId}/export" download
                        class="bg-slate-200 text-slate-700 px-6 py-2 rounded-lg hover:bg-slate-300 transition">
                        <i class="fas fa-download mr-2"></i>Exportar CSV
                    </a>
                </div>
            </div>
        `;
    } catch (error) {
        console.error('Error loading config:', error);
    }
}

/**
 * Save station configuration
 */
async function saveStationConfig(stationId) {
    const sf = parseInt(document.getElementById('cfg-sf').value);
    const bw = parseFloat(document.getElementById('cfg-bw').value);
    const interval = parseFloat(document.getElementById('cfg-interval').value) * 60000;

    try {
        const response = await fetch(api(`/api/stations/${stationId}/config`), {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ sf, bw, interval })
        });

        if (response.ok) {
            alert('Configuración guardada. Se aplicará en la próxima transmisión.');
        } else {
            const error = await response.json();
            alert('Error: ' + error.error);
        }
    } catch (error) {
        alert('Error de conexión');
    }
}

/**
 * Update alert badge
 */
function updateAlertBadge(count) {
    const badge = document.getElementById('alert-badge');
    if (count > 0) {
        badge.textContent = count;
        badge.classList.remove('hidden');
    } else {
        badge.classList.add('hidden');
    }
}

/**
 * Load alerts view
 */
async function refreshAlerts() {
    try {
        const response = await fetch(api('/api/alerts'));
        const alerts = await response.json();

        const list = document.getElementById('alerts-list');
        list.innerHTML = '';

        if (alerts.length === 0) {
            list.innerHTML = '<p class="text-slate-500 text-center py-8">No hay alertas activas</p>';
            return;
        }

        alerts.forEach(alert => {
            const typeIcons = {
                'TEMP_HIGH': 'fa-temperature-high text-red-500',
                'FROST_WARNING': 'fa-snowflake text-blue-500',
                'SOIL_DRY': 'fa-tint-slash text-amber-500',
                'SIGNAL_WEAK': 'fa-signal text-purple-500'
            };

            const div = document.createElement('div');
            div.className = 'bg-white rounded-lg shadow-sm p-4 flex items-center justify-between';
            div.innerHTML = `
                <div class="flex items-center gap-4">
                    <div class="w-10 h-10 rounded-lg bg-slate-100 flex items-center justify-center">
                        <i class="fas ${typeIcons[alert.type] || 'fa-exclamation text-gray-500'}"></i>
                    </div>
                    <div>
                        <p class="font-semibold text-slate-800">${alert.message}</p>
                        <p class="text-sm text-slate-400">${alert.station_name} - ${new Date(alert.created_at).toLocaleString()}</p>
                    </div>
                </div>
                <button data-action="acknowledgeAlert" data-alert-id="${alert.id}" class="text-slate-400 hover:text-slate-600">
                    <i class="fas fa-check"></i>
                </button>
            `;
            list.appendChild(div);
        });

    } catch (error) {
        console.error('Error loading alerts:', error);
    }
}

/**
 * Acknowledge an alert
 */
async function acknowledgeAlert(alertId) {
    try {
        await fetch(api(`/api/alerts/${alertId}/acknowledge`), { method: 'POST' });
        refreshAlerts();
        loadDashboard();
    } catch (error) {
        console.error('Error acknowledging alert:', error);
    }
}

/**
 * Load comparison chart
 */
async function loadComparison() {
    const stationSelect = document.getElementById('compare-stations');
    const selected = Array.from(stationSelect.selectedOptions).map(o => o.value);

    if (selected.length === 0) {
        alert('Seleccione al menos una estación');
        return;
    }

    const metric = document.getElementById('compare-metric').value;
    const period = document.getElementById('compare-period').value;

    try {
        const response = await fetch(
            api(`/api/compare?stationIds=${selected.join(',')}&metric=${metric}&period=${period}`)
        );
        const data = await response.json();

        const chart = state.charts['compare'];

        // Find all unique timestamps
        const allTimestamps = [...new Set(data.flatMap(s => s.data.map(d => d.timestamp)))].sort();
        const labels = allTimestamps.map(t => new Date(t).toLocaleTimeString());

        // Create datasets
        const datasets = data.map((station, i) => {
            const values = allTimestamps.map(t => {
                const point = station.data.find(d => d.timestamp === t);
                return point ? point.value : null;
            });

            return {
                label: station.stationName,
                data: values,
                borderColor: COLORS[i % COLORS.length],
                backgroundColor: COLORS[i % COLORS.length] + '20',
                fill: false,
                tension: 0.3
            };
        });

        chart.data.labels = labels;
        chart.data.datasets = datasets;
        chart.update();

    } catch (error) {
        console.error('Error loading comparison:', error);
    }
}

/**
 * Handle add station form
 */
async function handleAddStation(e) {
    e.preventDefault();

    const formData = new FormData(e.target);
    const data = Object.fromEntries(formData);

    try {
        const response = await fetch(api('/api/stations'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });

        const result = await response.json();

        if (response.ok) {
            document.getElementById('new-api-key').textContent = result.apiKey;
            document.getElementById('api-key-result').classList.remove('hidden');
            e.target.reset();
            loadDashboard();
        } else {
            alert('Error: ' + result.error);
        }
    } catch (error) {
        alert('Error de conexión');
    }
}

/**
 * Copy API key to clipboard
 */
function copyApiKey() {
    const key = document.getElementById('new-api-key').textContent;
    navigator.clipboard.writeText(key).then(() => {
        alert('API Key copiada al portapapeles');
    });
}

/**
 * View switching
 */
function showView(viewName) {
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));

    document.getElementById('view-' + viewName).classList.remove('hidden');
    document.querySelector(`[onclick="showView('${viewName}')"]`)?.classList.add('active');

    // Load view-specific data
    if (viewName === 'alerts') refreshAlerts();
    if (viewName === 'stations') loadStationsList();
}

/**
 * Load detailed stations list
 */
async function loadStationsList() {
    try {
        const response = await fetch(api('/api/stations'));
        const stations = await response.json();

        const list = document.getElementById('stations-list');
        list.innerHTML = '';

        stations.forEach(station => {
            const div = document.createElement('div');
            div.className = 'bg-white rounded-xl shadow-sm p-6';
            div.innerHTML = `
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-4">
                        <div class="w-12 h-12 bg-blue-100 rounded-lg flex items-center justify-center">
                            <i class="fas fa-broadcast-tower text-blue-600 text-xl"></i>
                        </div>
                        <div>
                            <h3 class="font-semibold text-lg text-slate-800">${station.name}</h3>
                            <p class="text-sm text-slate-400">${station.id} | ${station.total_readings || 0} lecturas</p>
                        </div>
                    </div>
                    <div class="flex items-center gap-3">
                        <button data-action="selectStationSettings" data-station-id="${station.id}"
                            class="bg-slate-100 text-slate-600 px-4 py-2 rounded-lg hover:bg-slate-200 transition">
                            <i class="fas fa-cog"></i>
                        </button>
                        <a href="api/data/${station.id}/export" download
                            class="bg-blue-100 text-blue-600 px-4 py-2 rounded-lg hover:bg-blue-200 transition">
                            <i class="fas fa-download"></i>
                        </a>
                    </div>
                </div>
                ${station.description ? `<p class="mt-3 text-slate-500">${station.description}</p>` : ''}
            `;
            list.appendChild(div);
        });

    } catch (error) {
        console.error('Error loading stations:', error);
    }
}

/**
 * Modal helpers
 */
function showAddStationModal() {
    document.getElementById('modal-add-station').classList.remove('hidden');
    document.getElementById('modal-add-station').classList.add('flex');
    document.getElementById('api-key-result').classList.add('hidden');
}

function closeModal(modalId) {
    document.getElementById(modalId).classList.add('hidden');
    document.getElementById(modalId).classList.remove('flex');
}

/**
 * Sidebar toggle (mobile)
 */
function toggleSidebar() {
    const sidebar = document.getElementById('sidebar');
    sidebar.classList.toggle('-translate-x-full');
}

/**
 * Refresh data
 */
function refreshData() {
    loadDashboard();
}

/**
 * Auto-refresh
 */
function startAutoRefresh() {
    state.refreshInterval = setInterval(loadDashboard, 30000); // 30 seconds
}
