import { state } from './state.js';
import { COLORS } from './config.js';

/* global Chart */

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

export function initCharts() {
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

    // Signal charts
    const signalConfigs = [
        { id: 'chart-rssi', label: 'RSSI (dBm)', color: '#3b82f6' },
        { id: 'chart-snr', label: 'SNR (dB)', color: '#8b5cf6' }
    ];

    signalConfigs.forEach(cfg => {
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
                        fill: true,
                        tension: 0.1
                    }]
                },
                options: {
                    ...commonOpts,
                    scales: {
                        x: { display: true, grid: { display: false } },
                        y: {
                            display: true,
                            grid: { color: '#f1f5f9' },
                            title: { display: true, text: cfg.label }
                        }
                    }
                }
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

export function updateChart(chartId, labels, data) {
    const chart = state.charts[chartId];
    if (chart) {
        chart.data.labels = labels;
        chart.data.datasets[0].data = data;
        chart.update();
    }
}
