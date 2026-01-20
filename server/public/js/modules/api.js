import { api } from './utils.js';

export async function fetchDashboard() {
    const res = await fetch(api('/api/dashboard'));
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function fetchStationHistory(id, limit, period) {
    let url = `/api/data/${id}/history?limit=${limit}`;
    if (period) url += `&period=${period}`;
    const res = await fetch(api(url));
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function fetchStations() {
    const res = await fetch(api('/api/stations'));
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function fetchStationConfig(id) {
    const res = await fetch(api(`/api/stations/${id}`));
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function updateStationConfig(id, config) {
    const res = await fetch(api(`/api/stations/${id}/config`), {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    });
    // This allows caller to handle 400 etc by reading body
    return res;
}

export async function createStation(data) {
    const res = await fetch(api('/api/stations'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });
    return res;
}

export async function fetchAlerts() {
    const res = await fetch(api('/api/alerts'));
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function acknowledgeAlert(id) {
    const res = await fetch(api(`/api/alerts/${id}/acknowledge`), { method: 'POST' });
    if (!res.ok) throw new Error('Network response was not ok');
    return res;
}

export async function compareStations(ids, metric, period) {
    const res = await fetch(
        api(`/api/compare?stationIds=${ids.join(',')}&metric=${metric}&period=${period}`)
    );
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

// ============================================
// Authentication API
// ============================================

export async function getAuthStatus() {
    const res = await fetch(api('/api/auth/status'), { credentials: 'include' });
    if (!res.ok) throw new Error('Network response was not ok');
    return res.json();
}

export async function logout() {
    const res = await fetch(api('/api/auth/logout'), {
        method: 'POST',
        credentials: 'include'
    });
    if (!res.ok) throw new Error('Logout failed');
    return res.json();
}

export async function getCurrentUser() {
    const res = await fetch(api('/api/auth/me'), { credentials: 'include' });
    if (!res.ok) return null;
    return res.json();
}

