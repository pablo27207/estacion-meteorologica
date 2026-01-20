GIPIS Weather Station - Roadmap del Proyecto detallado
Estado Actual: ✅ FASE 1 COMPLETADA
📅 Plan de Proyecto Detallado
✅ FASE 1: Fixes Críticos y UX Base
El objetivo de esta fase fue estabilizar la aplicación y mejorar la experiencia de usuario básica.

 Sidebar Colapsable: Funcionalidad completa de ocultar/mostrar menú con persistencia en localStorage para recordar la preferencia del usuario.
 Corrección de Timezone: Implementación de utilidades (
utils.js
) para formatear fechas correctamente en la zona horaria local.
 Selector de Estación: Integrado como control nativo de Leaflet en el mapa.
 Rediseño Dashboard:
Map-First Layout: El mapa es ahora el contenedor principal a pantalla completa.
Glassmorphism: Panel de estadísticas flotante con efecto de desenfoque.
Polished UI: Bordes redondeados y espaciado corregido para controles de zoom.
🚧 FASE 2: Arquitectura y Modularización
Mejoras en la estructura del código y expansión de capacidades funcionales.

 Refactorización Frontend: Migración completa a módulos ES6 (import/export) para mejor mantenibilidad.
 Simulación Multi-Estación: Backend capaz de simular datos para múltiples estaciones geográficas (Puerto, Aero, etc.).
 Sistema de Autenticación:
Backend: Rutas de API /api/auth/login y /api/auth/register con hashing de contraseñas (bcrypt).
Frontend: Pantallas de Login y Registro con validación visual.
Seguridad: Manejo de sesiones o Tokens JWT.
 Métricas Calculadas:
Implementación de fórmulas meteorológicas en JS:
Heat Index (Sensación térmica por calor).
Wind Chill (Sensación térmica por frío).
Dew Point (Punto de rocío).
Visualización de estos datos en tarjetas adicionales.
 Alertas Configurables:
Sistema para definir umbrales personalizados (ej: "Avisar si Temp > 30°C").
Notificaciones visuales en el dashboard cuando se rompen los umbrales.
 Comparador Flexible:
Interfaz para seleccionar dinámicamente qué sensores graficar.
Soporte para multieje Y (ej: Temperatura vs Humedad en el mismo gráfico).
🔮 FASE 3: Mejoras Visuales
 Dark Mode: Implementación de variables CSS o Tailwind dark mode para alternar temas.
 Loading States: Reemplazar pantallas en blanco con "Skeletons" o Spinners animados durante la carga de datos.
📡 FASE 4: Tiempo Real y Conectividad
 WebSockets/SSE: Migrar del "polling" (pedir datos cada 10s) a una conexión persistente para actualizaciones instantáneas.
 Estadísticas de Señal: Visualización gráfica del RSSI y SNR para enlaces LoRa y WiFi.
🚀 FASE 5: Features Avanzados
 Exportación Avanzada: Generación de reportes CSV/Excel por rangos de fecha personalizados.
 Historial de Alertas: Tabla de logs de eventos pasados con filtros de búsqueda.
 PWA (Progressive Web App): Configuración de manifest y Service Workers para instalar la app en móviles y permitir funcionamiento básico offline.
🛠️ FASE 6: Técnico
 Manejo de Errores: Interfaz amigable (Empty States, Error Boundaries) cuando falla la conexión.
 Validaciones JS: Feedback inmediato en formularios (email inválido, password débil).
 Optimización: Compresión de assets y lazy loading de componentes pesados.