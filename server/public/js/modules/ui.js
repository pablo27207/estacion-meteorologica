import { state } from './state.js';
import { formatTimeFull } from './utils.js';

function triggerStationSelection(stationId) {
    document.dispatchEvent(new CustomEvent('station-selected', { detail: { id: stationId } }));
}

export function updateValueCard(elementId, value) {
    const el = document.getElementById(elementId);
    if (el && value !== null && value !== undefined) {
        el.textContent = value.toFixed(1);
    }
}

export function updateConnectionStatus(connected) {
    const indicator = document.getElementById('connection-indicator');
    const text = document.getElementById('connection-text');
    const mobile = document.getElementById('mobile-status');

    if (connected) {
        indicator.className = 'w-3 h-3 rounded-full bg-green-500 animate-pulse';
        text.textContent = 'Servidor: online';
        mobile.className = 'w-3 h-3 rounded-full bg-green-500';
    } else {
        indicator.className = 'w-3 h-3 rounded-full bg-red-500';
        text.textContent = 'Servidor: sin conexión';
        mobile.className = 'w-3 h-3 rounded-full bg-red-500';
    }
}

// Selector logic removed

// Stations Grid logic removed

export function updateAlertBadge(count) {
    const badge = document.getElementById('alert-badge');
    if (count > 0) {
        badge.textContent = count;
        badge.classList.remove('hidden');
    } else {
        badge.classList.add('hidden');
    }
}

export function renderAlerts(alerts) {
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
}

export function updateLastUpdateText() {
    const el = document.getElementById('last-update-text');
    if (el) el.textContent = `Última actualización: ${formatTimeFull(new Date())}`;
}

export function showView(viewName) {
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));

    document.getElementById('view-' + viewName).classList.remove('hidden');

    // Find nav links that trigger this view
    const navLink = document.querySelector(`[data-action="showView"][data-view="${viewName}"]`);
    if (navLink) navLink.classList.add('active');
    // Also support onclick legacy
    const onclickLink = document.querySelector(`[onclick="showView('${viewName}')"]`);
    if (onclickLink) onclickLink.classList.add('active');
}

export function toggleSidebar() {
    const sidebar = document.getElementById('sidebar');
    sidebar.classList.toggle('-translate-x-full');
}

export function toggleSidebarCollapse() {
    const sidebar = document.getElementById('sidebar');
    const main = document.getElementById('main-content');

    sidebar.classList.toggle('collapsed');
    main.classList.toggle('sidebar-collapsed');

    localStorage.setItem('sidebarCollapsed', sidebar.classList.contains('collapsed') ? 'true' : 'false');
}

export function restoreSidebarState() {
    if (localStorage.getItem('sidebarCollapsed') === 'true') {
        const sidebar = document.getElementById('sidebar');
        const main = document.getElementById('main-content');
        sidebar.classList.add('collapsed');
        main.classList.add('sidebar-collapsed');
    }
}

export function showAddStationModal() {
    document.getElementById('modal-add-station').classList.remove('hidden');
    document.getElementById('modal-add-station').classList.add('flex');
    document.getElementById('api-key-result').classList.add('hidden');
}

export function closeModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.add('hidden');
        modal.classList.remove('flex');
    }
}

export function showApiKeyResult(apiKey) {
    document.getElementById('new-api-key').textContent = apiKey;
    document.getElementById('api-key-result').classList.remove('hidden');
}

export function renderStationConfigForm(station) {
    const form = document.getElementById('station-config-form');
    form.innerHTML = `
        <div class="space-y-4">
            <div>
                <label class="block text-sm font-semibold text-slate-600 mb-1">Nombre</label>
                <input type="text" id="cfg-name" value="${station.name}" class="w-full border rounded-lg px-3 py-2">
            </div>
            <div class="grid grid-cols-3 gap-4">
                <div>
                    <label class="block text-sm font-semibold text-slate-600 mb-1" title="Factor de dispersión LoRa. Mayor SF = mayor alcance pero menor velocidad de transmisión.">
                        Spreading Factor
                        <i class="fas fa-info-circle text-slate-400 ml-1 cursor-help" title="SF7: Corto alcance, alta velocidad. SF12: Largo alcance, baja velocidad."></i>
                    </label>
                    <select id="cfg-sf" class="w-full border rounded-lg px-3 py-2">
                        ${[7, 8, 9, 10, 11, 12].map(sf =>
        `<option value="${sf}" ${station.config_sf === sf ? 'selected' : ''}>SF${sf}</option>`
    ).join('')}
                    </select>
                </div>
                <div>
                    <label class="block text-sm font-semibold text-slate-600 mb-1" title="Ancho de banda de la señal LoRa.">
                        Bandwidth
                        <i class="fas fa-info-circle text-slate-400 ml-1 cursor-help" title="125 kHz: Mayor sensibilidad. 500 kHz: Mayor velocidad pero menor alcance."></i>
                    </label>
                    <select id="cfg-bw" class="w-full border rounded-lg px-3 py-2">
                        ${[125, 250, 500].map(bw =>
        `<option value="${bw}" ${station.config_bw === bw ? 'selected' : ''}>${bw} kHz</option>`
    ).join('')}
                    </select>
                </div>
                <div>
                    <label class="block text-sm font-semibold text-slate-600 mb-1" title="Tiempo entre envíos de datos en minutos.">
                        Intervalo (min)
                        <i class="fas fa-info-circle text-slate-400 ml-1 cursor-help" title="Cada cuántos minutos la estación envía datos. Menor intervalo = más datos pero mayor consumo de batería."></i>
                    </label>
                    <input type="number" id="cfg-interval" value="${(station.config_interval / 60000).toFixed(1)}"
                        step="0.5" min="0.1" class="w-full border rounded-lg px-3 py-2">
                </div>
            </div>
            <div class="flex gap-3 pt-4">
                <button data-action="saveStationConfig" data-station-id="${station.id}"
                    class="bg-blue-600 text-white px-6 py-2 rounded-lg hover:bg-blue-700 transition">
                    Guardar Configuración
                </button>
            </div>
        </div>
    `;
}
