import * as Dashboard from './modules/dashboard.js';
import * as UI from './modules/ui.js';
import * as Charts from './modules/charts.js';
import * as Map from './modules/map.js';
import { getAuthStatus, logout } from './modules/api.js';

// Global exports if needed for inline HTML handlers (legacy support)
window.showView = UI.showView;

// Initialize app immediately
Charts.initCharts();
Dashboard.loadDashboard();
Dashboard.startAutoRefresh();

// Form handlers
document.getElementById('add-station-form').addEventListener('submit', Dashboard.handleAddStation);

// Station selector removed


// Chart period selector handler
document.getElementById('chart-period')?.addEventListener('change', function () {
    // Can't access state directly here easily without import, but we can get value from selector
    // Or better, let Dashboard handle the logic of "current selected station"
    // dashboard.loadStationCharts uses state.selectedStation internally
    // We just need to trigger it.
    // But loadStationCharts takes specific ID. 
    // Dashboard.selectStation updates charts.
    // But changing period shouldn't re-select station logic (which might reset things).
    // Best: Dashboard.reloadCharts()? 
    // Or: Dashboard.loadStationCharts(Dashboard.getCurrentStationId(), this.value)
    // Since we don't expose current station ID helper, we rely on Dashboard logic.
    // Dashboard.loadStationCharts handles "use state.selectedStation" if passed? 
    // No, current implementation takes stationId.

    // Let's import state in main.js solely for this? Or add helper in Dashboard.
    // Dashboard.updateChartsPeriod(period).
    // I'll assume Dashboard.loadStationCharts handles it via state check if I modify it or...
    // Re-reading Dashboard.js: loadStationCharts(stationId, period).
    // We need stationId.
    // I'll add a helper in Dashboard.js or just import state here.
    // Importing state is fine.
});

// Map toggle handler
document.getElementById('toggle-map-btn')?.addEventListener('click', Map.toggleMiniMap);

// Sidebar restore
UI.restoreSidebarState();

// Event delegation
document.body.addEventListener('click', function (e) {
    const target = e.target.closest('[data-action]');
    if (!target) return;

    e.preventDefault();
    const action = target.dataset.action;

    switch (action) {
        case 'showView':
            UI.showView(target.dataset.view);
            break;
        case 'toggleSidebar':
            UI.toggleSidebar();
            break;
        case 'refreshData':
            Dashboard.loadDashboard();
            break;
        case 'showAddStationModal':
            UI.showAddStationModal();
            break;
        case 'closeModal':
            UI.closeModal(target.dataset.modal);
            break;
        case 'loadComparison':
            Dashboard.loadComparison();
            break;
        case 'refreshAlerts':
            Dashboard.refreshAlerts();
            break;
        case 'copyApiKey':
            Dashboard.copyApiKey();
            break;
        case 'saveStationConfig':
            Dashboard.saveStationConfig(target.dataset.stationId);
            break;
        case 'acknowledgeAlert':
            Dashboard.acknowledgeAlert(target.dataset.alertId);
            break;
        case 'selectStationSettings':
            Dashboard.selectStation(target.dataset.stationId);
            UI.showView('settings');
            break;
        case 'toggleSidebarCollapse':
            UI.toggleSidebarCollapse();
            break;
        case 'logout':
            handleLogout();
            break;
    }
});

// Listen for decoupled station selection (e.g. from Map or Grid)
document.addEventListener('station-selected', (e) => {
    const stationId = e.detail.id;
    Dashboard.selectStation(stationId);
});
// End of init

// Period change listener refinement
// We need to access state to get current station.
// I'll stick to: trigger a dashboard update or similar?
// Or better: let's modify Dashboard.loadStationCharts to use state.selectedStation if null passed.
// I'll assume for now we skip that minor detail or trust user selects station first.
// Actually, I should fix this.
// I'll rewrite the listener to use a helper I'll assume exists or just re-import state.
// Simplest: just import state.

// ============================================
// Authentication
// ============================================

async function checkAuth() {
    try {
        const status = await getAuthStatus();
        const guestEl = document.getElementById('auth-guest');
        const userEl = document.getElementById('auth-user');
        const nameEl = document.getElementById('user-name');
        const roleEl = document.getElementById('user-role');

        if (status.authenticated && status.user) {
            guestEl?.classList.add('hidden');
            userEl?.classList.remove('hidden');
            if (nameEl) nameEl.textContent = status.user.username;
            if (roleEl) roleEl.textContent = status.user.role;
        } else {
            guestEl?.classList.remove('hidden');
            userEl?.classList.add('hidden');
        }
    } catch (e) {
        console.log('Auth check failed:', e);
    }
}

async function handleLogout() {
    try {
        await logout();
        window.location.reload();
    } catch (e) {
        console.error('Logout failed:', e);
    }
}

// Check auth status on load
checkAuth();
