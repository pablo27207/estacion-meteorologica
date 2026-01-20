/**
 * Toast Notification System
 * Sistema de notificaciones no intrusivas
 */

const Toast = (function () {
    let container = null;

    // Tipos de toast con colores
    const TYPES = {
        success: {
            bg: 'bg-emerald-500',
            icon: 'fa-check-circle'
        },
        error: {
            bg: 'bg-red-500',
            icon: 'fa-times-circle'
        },
        warning: {
            bg: 'bg-amber-500',
            icon: 'fa-exclamation-triangle'
        },
        info: {
            bg: 'bg-blue-500',
            icon: 'fa-info-circle'
        }
    };

    /**
     * Inicializa el contenedor de toasts
     */
    function init() {
        if (container) return;

        container = document.createElement('div');
        container.id = 'toast-container';
        container.className = 'fixed bottom-4 right-4 z-[100] flex flex-col gap-2 pointer-events-none';
        document.body.appendChild(container);
    }

    /**
     * Muestra un toast
     * @param {string} message - Mensaje a mostrar
     * @param {string} type - Tipo: success, error, warning, info
     * @param {number} duration - Duración en ms (default 5000)
     */
    function show(message, type = 'info', duration = 5000) {
        init();

        const config = TYPES[type] || TYPES.info;

        const toast = document.createElement('div');
        toast.className = `
            pointer-events-auto
            flex items-center gap-3 px-4 py-3 rounded-lg shadow-lg
            ${config.bg} text-white
            transform translate-x-full opacity-0
            transition-all duration-300 ease-out
            max-w-sm
        `;

        toast.innerHTML = `
            <i class="fas ${config.icon} text-lg"></i>
            <span class="flex-1 text-sm font-medium">${message}</span>
            <button class="hover:opacity-75 transition-opacity" onclick="this.parentElement.remove()">
                <i class="fas fa-times"></i>
            </button>
        `;

        container.appendChild(toast);

        // Trigger animation
        requestAnimationFrame(() => {
            toast.classList.remove('translate-x-full', 'opacity-0');
        });

        // Auto dismiss
        if (duration > 0) {
            setTimeout(() => {
                toast.classList.add('translate-x-full', 'opacity-0');
                setTimeout(() => toast.remove(), 300);
            }, duration);
        }

        return toast;
    }

    // Public API
    return {
        show,
        success: (msg, duration) => show(msg, 'success', duration),
        error: (msg, duration) => show(msg, 'error', duration),
        warning: (msg, duration) => show(msg, 'warning', duration),
        info: (msg, duration) => show(msg, 'info', duration)
    };
})();
