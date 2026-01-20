import { state } from './state.js';

/* global L */

// Internal helper to trigger selection
function triggerStationSelection(stationId) {
    document.dispatchEvent(new CustomEvent('station-selected', { detail: { id: stationId } }));
}

/**
 * Initialize Dashboard Map
 */
export function initDashboardMap() {
    const mapContainer = document.getElementById('dashboard-map');
    if (!mapContainer) return;

    if (!state.dashboardMap) {
        // Initial center: Comodoro Rivadavia
        let center = [-45.86, -67.46];
        let zoom = 9;

        state.dashboardMap = L.map('dashboard-map', {
            zoomControl: false // We can add it elsewhere or keep default
        }).setView(center, zoom);

        L.control.zoom({ position: 'topright' }).addTo(state.dashboardMap);

        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; OpenStreetMap'
        }).addTo(state.dashboardMap);
    }

    // Ensure size is correct
    setTimeout(() => {
        state.dashboardMap.invalidateSize();
        updateDashboardMapMarkers();
    }, 100);
}

export function updateDashboardMapMarkers() {
    if (!state.dashboardMap) return;

    // Clear existing markers
    if (state.dashboardMapMarkers) {
        Object.values(state.dashboardMapMarkers).forEach(m => m.remove());
    }
    state.dashboardMapMarkers = {};

    const markers = [];

    // Add markers for each station with coordinates
    state.stations.forEach(station => {
        // Correct nested property access
        const lat = station.location?.lat || station.location_lat || station.lat;
        const lng = station.location?.lng || station.location_lng || station.lng;

        if (lat && lng) {
            const isSelected = station.id === state.selectedStation;

            // Marker color based on status
            const statusColor = {
                'online': '#22c55e',
                'delayed': '#eab308',
                'offline': '#ef4444'
            }[station.connectionStatus] || '#9ca3af';

            const size = isSelected ? 36 : 24;
            const anchor = size / 2;

            const icon = L.divIcon({
                className: 'custom-marker',
                html: `<div style="
                    width:${size}px;
                    height:${size}px;
                    background:${statusColor};
                    border:3px solid ${isSelected ? '#3b82f6' : 'white'};
                    border-radius:50%;
                    box-shadow:0 4px 6px rgba(0,0,0,0.3);
                    display:flex;align-items:center;justify-content:center;
                    font-size:10px;font-weight:bold;color:white;
                    cursor:pointer;
                    transition: all 0.2s ease;
                "></div>`,
                iconSize: [size, size],
                iconAnchor: [anchor, anchor]
            });

            // Tooltip content
            const tooltipContent = `
                <div class="text-center">
                    <strong class="block text-slate-800">${station.name}</strong>
                    <span class="text-xs text-slate-500">${station.id}</span>
                </div>
            `;

            const marker = L.marker([lat, lng], { icon })
                .addTo(state.dashboardMap)
                .bindTooltip(tooltipContent, {
                    permanent: false,
                    direction: 'top',
                    offset: [0, -15],
                    className: 'px-2 py-1 bg-white border border-slate-200 shadow-sm rounded text-xs'
                });

            marker.on('click', () => {
                triggerStationSelection(station.id);
            });

            state.dashboardMapMarkers[station.id] = marker;
            markers.push(marker);
        }
    });

    // Auto-zoom (Fit Bounds) logic
    if (markers.length > 0) {
        const group = L.featureGroup(markers);
        // Pad 0.2 means 20% padding around markers
        state.dashboardMap.fitBounds(group.getBounds().pad(0.2));
    }

    updateStationControl();
}

// Custom Control for Selected Station
let stationControl = null;

function updateStationControl() {
    if (!state.dashboardMap) return;

    if (!stationControl) {
        stationControl = L.control({ position: 'topleft' });
        stationControl.onAdd = function () {
            const div = L.DomUtil.create('div', 'leaflet-bar leaflet-control leaflet-control-custom');
            div.style.backgroundColor = 'white';
            div.style.padding = '5px 10px';
            div.style.border = '2px solid rgba(0,0,0,0.2)';
            div.style.borderRadius = '4px';
            div.style.fontSize = '14px';
            div.style.fontWeight = 'bold';
            div.style.color = '#333';
            div.style.boxShadow = '0 1px 5px rgba(0,0,0,0.4)';
            div.innerHTML = 'Cargando...';
            return div;
        };
        stationControl.addTo(state.dashboardMap);
    }

    const container = stationControl.getContainer();
    if (state.selectedStation) {
        const station = state.stations.find(s => s.id === state.selectedStation);
        if (station) {
            container.innerHTML = `<i class="fas fa-map-marker-alt text-blue-500 mr-2"></i> ${station.name}`;
            container.style.display = 'block';
        } else {
            container.innerHTML = 'Estación desconocida';
        }
    } else {
        container.innerHTML = 'Todas las estaciones';
    }
}

/**
 * Mini-map functions (kept if still needed elsewhere, or remove if obsolete)
 * User removed sidebar selector which had "Ver en mapa".
 * The Dashboard IS the map now.
 * We can probably remove mini-map logic if no view uses it anymore.
 * But to be safe, I'll allow it to stay or comment it out.
 * The request implies replacing the main interaction with the map.
 * I'll just keeping initDashboardMap as the main export.
 */
