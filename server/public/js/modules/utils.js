import { TIMEZONE, LOCALE } from './config.js';

export function parseUTCTimestamp(timestamp) {
    if (!timestamp) return new Date();
    if (timestamp instanceof Date) return timestamp;
    if (typeof timestamp === 'string' && !timestamp.includes('Z') && !timestamp.includes('+')) {
        return new Date(timestamp.replace(' ', 'T') + 'Z');
    }
    return new Date(timestamp);
}

export function formatTime(date) {
    return parseUTCTimestamp(date).toLocaleTimeString(LOCALE, {
        timeZone: TIMEZONE,
        hour: '2-digit',
        minute: '2-digit'
    });
}

export function formatDateTime(date) {
    const d = parseUTCTimestamp(date);
    return d.toLocaleDateString(LOCALE, { timeZone: TIMEZONE, day: '2-digit', month: '2-digit' }) +
        ' ' + d.toLocaleTimeString(LOCALE, { timeZone: TIMEZONE, hour: '2-digit', minute: '2-digit' });
}

export function formatTimeFull(date) {
    return parseUTCTimestamp(date).toLocaleTimeString(LOCALE, { timeZone: TIMEZONE });
}

export function api(path) {
    const cleanPath = path.startsWith('/') ? path.slice(1) : path;
    return cleanPath;
}
