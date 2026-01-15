#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Estación Meteorológica GIPIS</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600&display=swap" rel="stylesheet">
    <style>
        body { font-family: 'Inter', sans-serif; }
        .tab-btn.active { border-bottom: 2px solid #2563eb; color: #2563eb; }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
    </style>
</head>
<body class="bg-slate-100 text-slate-800">
    <nav class="bg-white shadow-sm mb-6 sticky top-0 z-50">
        <div class="container mx-auto px-4">
            <div class="flex justify-between items-center h-16">
                <div class="flex items-center space-x-2">
                    <i class="fas fa-satellite-dish text-blue-600 text-xl"></i>
                    <h1 class="text-xl font-bold text-slate-700">Estación GIPIS</h1>
                </div>
                <div class="flex items-center space-x-2 text-sm">
                    <span id="status-dot" class="h-3 w-3 rounded-full bg-red-500"></span>
                    <span id="last-update" class="text-slate-500 hidden md:inline">Esperando...</span>
                </div>
            </div>
            <!-- Tabs -->
            <div class="flex space-x-8">
                <button onclick="switchTab('dashboard')" id="tab-dashboard" class="tab-btn active pb-3 text-sm font-semibold text-slate-500 hover:text-slate-700 transition">
                    <i class="fas fa-chart-line mr-1"></i> Monitor
                </button>
                <button onclick="switchTab('config')" id="tab-config" class="tab-btn pb-3 text-sm font-semibold text-slate-500 hover:text-slate-700 transition">
                    <i class="fas fa-cogs mr-1"></i> Configuración
                </button>
            </div>
        </div>
    </nav>

    <div class="container mx-auto px-4 pb-10">
        
        <!-- DASHBOARD TAB -->
        <div id="view-dashboard" class="tab-content active">
            <!-- Cards Grid -->
            <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
                <div class="bg-white rounded-xl shadow-md p-6 border-l-4 border-blue-500">
                    <div class="flex justify-between items-start">
                        <div>
                            <p class="text-xs font-semibold text-slate-400 uppercase tracking-wider">Temp. Aire</p>
                            <h2 class="text-3xl font-bold text-slate-700 mt-1"><span id="tempAire">--</span><span class="text-lg text-slate-400 font-normal">°C</span></h2>
                        </div>
                        <div class="p-3 bg-blue-50 rounded-lg text-blue-600"><i class="fas fa-thermometer-half text-xl"></i></div>
                    </div>
                </div>
                <div class="bg-white rounded-xl shadow-md p-6 border-l-4 border-cyan-500">
                    <div class="flex justify-between items-start">
                        <div>
                            <p class="text-xs font-semibold text-slate-400 uppercase tracking-wider">Hum. Aire</p>
                            <h2 class="text-3xl font-bold text-slate-700 mt-1"><span id="humAire">--</span><span class="text-lg text-slate-400 font-normal">%</span></h2>
                        </div>
                        <div class="p-3 bg-cyan-50 rounded-lg text-cyan-600"><i class="fas fa-wind text-xl"></i></div>
                    </div>
                </div>
                <div class="bg-white rounded-xl shadow-md p-6 border-l-4 border-amber-500">
                    <div class="flex justify-between items-start">
                        <div>
                            <p class="text-xs font-semibold text-slate-400 uppercase tracking-wider">Temp. Suelo</p>
                            <h2 class="text-3xl font-bold text-slate-700 mt-1"><span id="tempSuelo">--</span><span class="text-lg text-slate-400 font-normal">°C</span></h2>
                        </div>
                        <div class="p-3 bg-amber-50 rounded-lg text-amber-600"><i class="fas fa-temperature-low text-xl"></i></div>
                    </div>
                </div>
                <div class="bg-white rounded-xl shadow-md p-6 border-l-4 border-emerald-500">
                    <div class="flex justify-between items-start">
                        <div>
                            <p class="text-xs font-semibold text-slate-400 uppercase tracking-wider">Hum. Suelo</p>
                            <h2 class="text-3xl font-bold text-slate-700 mt-1"><span id="vwcSuelo">--</span><span class="text-lg text-slate-400 font-normal">%</span></h2>
                        </div>
                        <div class="p-3 bg-emerald-50 rounded-lg text-emerald-600"><i class="fas fa-seedling text-xl"></i></div>
                    </div>
            </div>

            <!-- Charts Grid -->
            <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
                <div class="bg-white p-4 rounded-xl shadow-md"><h3 class="text-slate-600 font-semibold mb-2">Temperatura Aire (°C)</h3><canvas id="cTempAir"></canvas></div>
                <div class="bg-white p-4 rounded-xl shadow-md"><h3 class="text-slate-600 font-semibold mb-2">Humedad Aire (%)</h3><canvas id="cHumAir"></canvas></div>
                <div class="bg-white p-4 rounded-xl shadow-md"><h3 class="text-slate-600 font-semibold mb-2">Temperatura Suelo (°C)</h3><canvas id="cTempGnd"></canvas></div>
                <div class="bg-white p-4 rounded-xl shadow-md"><h3 class="text-slate-600 font-semibold mb-2">VWC Suelo (%)</h3><canvas id="cVwcGnd"></canvas></div>
            </div>
        </div>

        <!-- CONFIG TAB -->
        <div id="view-config" class="tab-content">
            
            <!-- Signal Quality -->
            <div class="bg-white p-6 rounded-xl shadow-md mb-8">
                <h3 class="text-lg font-bold text-slate-700 mb-4 flex items-center gap-2"><i class="fas fa-signal text-violet-500"></i> Calidad de Señal LoRa</h3>
                <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-2 gap-6">
                    <!-- RSSI -->
                    <div class="bg-slate-50 p-4 rounded-lg relative group border border-slate-200">
                        <div class="flex justify-between items-center mb-1">
                            <span class="text-sm font-bold text-slate-500">RSSI (Potencia)</span>
                            <i class="fas fa-question-circle text-slate-300 cursor-help"></i>
                        </div>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-64 z-10 shadow-lg">
                            Indicador de fuerza de señal recibida.<br>
                            > -100 dBm: Excelente/Buena.<br>
                            < -120 dBm: Débil/Límite.
                        </div>
                        <div class="text-2xl font-mono font-bold text-slate-700" id="rssiVal">-- dBm</div>
                        <div class="w-full bg-slate-200 rounded-full h-2.5 mt-2">
                          <div id="rssiBar" class="bg-slate-300 h-2.5 rounded-full transition-all duration-500" style="width: 0%"></div>
                        </div>
                    </div>
                    
                    <!-- SNR -->
                    <div class="bg-slate-50 p-4 rounded-lg relative group border border-slate-200">
                        <div class="flex justify-between items-center mb-1">
                            <span class="text-sm font-bold text-slate-500">SNR (Ruido)</span>
                            <i class="fas fa-question-circle text-slate-300 cursor-help"></i>
                        </div>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-64 z-10 shadow-lg">
                            Relación Señal/Ruido.<br>
                            > 0 dB: Señal muy limpia.<br>
                            < -10 dB: Mucho ruido/interferencia.
                        </div>
                        <div class="text-2xl font-mono font-bold text-slate-700" id="snrVal">-- dB</div>
                        <div class="w-full bg-slate-200 rounded-full h-2.5 mt-2">
                          <div id="snrBar" class="bg-slate-300 h-2.5 rounded-full transition-all duration-500" style="width: 0%"></div>
                        </div>
                    </div>

                    <!-- Reliability -->
                    <div class="bg-slate-50 p-4 rounded-lg relative group border border-slate-200">
                         <div class="flex justify-between items-center mb-1">
                            <span class="text-sm font-bold text-slate-500">Estabilidad</span>
                            <i class="fas fa-question-circle text-slate-300 cursor-help"></i>
                        </div>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-64 z-10 shadow-lg">
                            Paquetes Recibidos Exitosamente.<br>
                            Si es bajo (< 90%), prueba aumentar SF o bajar BW.
                        </div>
                        <div class="text-2xl font-mono font-bold text-slate-700" id="relVal">-- %</div>
                        <div class="w-full bg-slate-200 rounded-full h-2.5 mt-2">
                          <div id="relBar" class="bg-slate-300 h-2.5 rounded-full transition-all duration-500" style="width: 0%"></div>
                        </div>
                    </div>

                    <!-- Freq Error -->
                     <div class="bg-slate-50 p-4 rounded-lg relative group border border-slate-200">
                         <div class="flex justify-between items-center mb-1">
                            <span class="text-sm font-bold text-slate-500">Desvío Frec.</span>
                            <i class="fas fa-question-circle text-slate-300 cursor-help"></i>
                        </div>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-64 z-10 shadow-lg">
                            Desviación de frecuencia del módulo.<br>
                            Ideal: < 1000 Hz. Si es muy alto, podría haber problemas de hardware.
                        </div>
                        <div class="text-2xl font-mono font-bold text-slate-700" id="freqVal">-- Hz</div>
                        <div class="text-xs text-slate-400 mt-2">Hardware Drift</div>
                    </div>
                </div>
            </div>

            <!-- Remote Config -->
            <div class="bg-white p-6 rounded-xl shadow-md border border-slate-200">
                <h3 class="text-lg font-bold text-slate-700 mb-4 flex items-center gap-2"><i class="fas fa-sliders-h text-blue-500"></i> Parámetros de Transmisión (RX & TX)</h3>
                <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
                    <div class="relative group">
                        <label class="block text-xs font-bold text-slate-500 uppercase flex items-center gap-1 mb-1">
                            Intervalo (min) <i class="fas fa-info-circle text-gray-300"></i>
                        </label>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-48 z-10">
                            Frecuencia de envío de datos.
                        </div>
                        <input type="number" id="intVal" class="w-full border border-slate-300 rounded px-3 py-2 focus:ring-2 focus:ring-blue-500 outline-none" step="0.5" min="0.1">
                    </div>
                    
                    <div class="relative group">
                        <label class="block text-xs font-bold text-slate-500 uppercase flex items-center gap-1 mb-1">
                            Spreading Factor <i class="fas fa-info-circle text-gray-300"></i>
                        </label>
                         <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-56 z-10">
                            SF7: Rápido, menor alcance.<br>SF12: Lento, máximo alcance.
                        </div>
                        <select id="sfVal" class="w-full border border-slate-300 rounded px-3 py-2 bg-white focus:ring-2 focus:ring-blue-500 outline-none">
                            <option value="7">SF7 (Rápido)</option>
                            <option value="8">SF8</option>
                            <option value="9">SF9 (Balance)</option>
                            <option value="10">SF10</option>
                            <option value="11">SF11</option>
                            <option value="12">SF12 (Largo Alcance)</option>
                        </select>
                    </div>
                    
                    <div class="relative group">
                        <label class="block text-xs font-bold text-slate-500 uppercase flex items-center gap-1 mb-1">
                            Bandwidth (kHz) <i class="fas fa-info-circle text-gray-300"></i>
                        </label>
                        <div class="absolute bottom-full mb-2 hidden group-hover:block bg-gray-800 text-white text-xs p-2 rounded w-56 z-10">
                            125 kHz: Estándar, mayor alcance.<br>
                            500 kHz: Más rápido, menor alcance.
                        </div>
                        <select id="bwVal" class="w-full border border-slate-300 rounded px-3 py-2 bg-white focus:ring-2 focus:ring-blue-500 outline-none">
                            <option value="125">125 kHz</option>
                            <option value="250">250 kHz</option>
                            <option value="500">500 kHz</option>
                        </select>
                    </div>
                </div>
                
                <div class="mt-8 flex justify-between items-center bg-slate-50 p-4 rounded-lg">
                     <div id="cmdStatus" class="text-sm text-slate-500 font-mono">Esperando cambios...</div>
                     <button onclick="saveConfig()" class="bg-blue-600 text-white px-6 py-2 rounded font-semibold hover:bg-blue-700 transition shadow-sm flex items-center gap-2">
                        <i class="fas fa-save"></i> Guardar Cambios
                     </button>
                </div>
            </div>

            <!-- Remote Server Config -->
            <div class="bg-white p-6 rounded-xl shadow-md border border-slate-200 mt-8">
                <h3 class="text-lg font-bold text-slate-700 mb-4 flex items-center gap-2"><i class="fas fa-cloud-upload-alt text-green-500"></i> Servidor Remoto</h3>
                <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
                    <div>
                        <label class="block text-xs font-bold text-slate-500 uppercase mb-1">URL del Servidor</label>
                        <input type="text" id="serverUrl" placeholder="https://gipis.unp.edu.ar/weather" class="w-full border border-slate-300 rounded px-3 py-2 focus:ring-2 focus:ring-green-500 outline-none">
                    </div>
                    <div>
                        <label class="block text-xs font-bold text-slate-500 uppercase mb-1">API Key</label>
                        <input type="text" id="serverApiKey" placeholder="clave-secreta-estacion" class="w-full border border-slate-300 rounded px-3 py-2 focus:ring-2 focus:ring-green-500 outline-none">
                    </div>
                </div>
                <div class="mt-4 flex items-center gap-2">
                    <input type="checkbox" id="serverEnabled" class="w-5 h-5 text-green-600 rounded focus:ring-green-500">
                    <label for="serverEnabled" class="text-sm font-semibold text-slate-600">Habilitar envío de datos al servidor</label>
                </div>
                <div class="mt-6 flex justify-between items-center bg-slate-50 p-4 rounded-lg">
                     <div id="serverStatus" class="text-sm text-slate-500 font-mono">Esperando...</div>
                     <button onclick="saveServer()" class="bg-green-600 text-white px-6 py-2 rounded font-semibold hover:bg-green-700 transition shadow-sm flex items-center gap-2">
                        <i class="fas fa-cloud"></i> Guardar Servidor
                     </button>
                </div>
            </div>
        </div>
    </div>

    <script>
        // --- TABS LOGIC ---
        function switchTab(tab) {
            document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
            document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
            
            document.getElementById('view-' + tab).classList.add('active');
            document.getElementById('tab-' + tab).classList.add('active');
        }

        // --- CHART & DATA LOGIC ---
        const MAX_POINTS = 50;
        const commonOpts = { responsive: true, scales: { x: { display: false } }, elements: { point: { radius: 0, hitRadius: 10 } }, plugins: { legend: { display: false } } };

        function createChart(id, label, colorHex) {
            return new Chart(document.getElementById(id).getContext('2d'), {
                type: 'line',
                data: { labels: [], datasets: [{ label: label, data: [], borderColor: colorHex, backgroundColor: colorHex + '20', fill: true, tension: 0.4 }] },
                options: commonOpts
            });
        }

        const chTA = createChart('cTempAir', 'Temp Aire', '#3b82f6'); 
        const chHA = createChart('cHumAir',  'Hum Aire',  '#06b6d4'); 
        const chTG = createChart('cTempGnd', 'Temp Suelo','#f59e0b'); 
        const chVG = createChart('cVwcGnd',  'VWC Suelo', '#10b981'); 

        function addData(chart, label, val) {
            chart.data.labels.push(label);
            chart.data.datasets[0].data.push(val);
            if(chart.data.labels.length > MAX_POINTS) { chart.data.labels.shift(); chart.data.datasets[0].data.shift(); }
            chart.update();
        }

        let lastPacketId = -1;
        let initDone = false;
        
        setInterval(() => {
            fetch("/data").then(r => r.json()).then(d => {
                // Sync UI once
                if(!initDone && d.packetId > 0) {
                    document.getElementById("intVal").value = d.interval;
                    document.getElementById("sfVal").value = d.sf;
                    document.getElementById("bwVal").value = d.bw;
                    initDone = true;
                }

                // Update Data
                document.getElementById("tempAire").innerText = d.tempAire.toFixed(1);
                document.getElementById("humAire").innerText = d.humAire.toFixed(1);
                document.getElementById("tempSuelo").innerText = d.tempSuelo.toFixed(1);
                document.getElementById("vwcSuelo").innerText = d.vwcSuelo.toFixed(1);

                // Update Log Info
                if(d.logSize !== undefined) {
                    document.getElementById("logSize").innerText = (d.logSize / 1024).toFixed(1);
                    document.getElementById("logEntries").innerText = d.logEntries || 0;
                }

                // Update Signal (Config Tab)
                if(d.rssi) {
                    // 1. RSSI
                    const rssiVal = d.rssi;
                    document.getElementById("rssiVal").innerText = rssiVal.toFixed(0) + " dBm";
                    let rssiPct = Math.max(0, Math.min(100, (rssiVal + 130) * (100/90))); 
                    const rBar = document.getElementById("rssiBar");
                    rBar.style.width = rssiPct + "%";
                    rBar.className = "h-2.5 rounded-full transition-all duration-500 ";
                    if(rssiVal > -90) rBar.classList.add("bg-emerald-500");
                    else if(rssiVal > -110) rBar.classList.add("bg-yellow-500");
                    else rBar.classList.add("bg-red-500");
                    
                    // 2. SNR
                    const snrVal = d.snr;
                    document.getElementById("snrVal").innerText = snrVal.toFixed(1) + " dB";
                    let snrPct = Math.max(0, Math.min(100, (snrVal + 20) * (100/30)));
                    const sBar = document.getElementById("snrBar");
                    sBar.style.width = snrPct + "%";
                    sBar.className = "h-2.5 rounded-full transition-all duration-500 ";
                    if(snrVal > 0) sBar.classList.add("bg-emerald-500");
                    else if(snrVal > -10) sBar.classList.add("bg-yellow-500");
                    else sBar.classList.add("bg-red-500");
                    
                    // 3. Reliability
                    const relVal = d.reliability || 100;
                    document.getElementById("relVal").innerText = relVal.toFixed(1) + " %";
                    const relBar = document.getElementById("relBar");
                    relBar.style.width = relVal + "%";
                    relBar.className = "h-2.5 rounded-full transition-all duration-500 ";
                    if(relVal > 95) relBar.classList.add("bg-emerald-500");
                    else if(relVal > 80) relBar.classList.add("bg-yellow-500");
                    else relBar.classList.add("bg-red-500");

                    // 4. Freq Error
                    const freqErr = d.freqErr || 0;
                    document.getElementById("freqVal").innerText = freqErr.toFixed(0) + " Hz";
                }
                
                if(d.packetId > 0 && d.packetId != lastPacketId) {
                    lastPacketId = d.packetId;
                    const now = new Date();
                    const timeStr = now.toLocaleTimeString();
                    document.getElementById("last-update").innerText = "Act. " + timeStr;
                    document.getElementById("status-dot").className = "h-3 w-3 rounded-full bg-green-500 animate-pulse";
                    addData(chTA, timeStr, d.tempAire); addData(chHA, timeStr, d.humAire); addData(chTG, timeStr, d.tempSuelo); addData(chVG, timeStr, d.vwcSuelo);
                    
                    const st = document.getElementById("cmdStatus");
                    if(st.innerText.includes("Encolado")) {
                        st.innerText = "Configuración aplicada.";
                        st.className = "text-sm text-green-600 font-mono font-bold";
                    }
                }
            }).catch(e => { document.getElementById("status-dot").className = "h-3 w-3 rounded-full bg-red-500"; });
        }, 2000);

        function saveConfig() {
            if(!confirm("¿Estás seguro? Esto cambiará la configuración de radio.")) return;

            const intVal = document.getElementById("intVal").value;
            const sfVal = document.getElementById("sfVal").value;
            const bwVal = document.getElementById("bwVal").value;
            
            const statusEl = document.getElementById("cmdStatus");
            statusEl.innerText = "Encolando...";
            statusEl.className = "text-sm text-blue-500 font-mono";
            
            fetch(`/set_remote?int=${intVal}&sf=${sfVal}&bw=${bwVal}`)
                .then(r => r.text())
                .then(t => {
                    statusEl.innerText = t;
                    if(t.includes("Encolado")) statusEl.className = "text-sm text-green-600 font-mono font-bold";
                    else statusEl.className = "text-sm text-red-500 font-mono";
                })
                .catch(e => {
                    statusEl.innerText = "Error de conexión";
                    statusEl.className = "text-sm text-red-500 font-mono";
                });
        }

        function clearLog() {
            if(!confirm("¿Borrar todo el historial de datos?")) return;
            fetch('/clear_log')
                .then(r => r.text())
                .then(t => {
                    document.getElementById("logSize").innerText = "0";
                    document.getElementById("logEntries").innerText = "0";
                    alert("Historial borrado");
                })
                .catch(e => alert("Error al borrar"));
        }

        // --- SERVER CONFIG ---
        function loadServerConfig() {
            fetch('/get_server')
                .then(r => r.json())
                .then(d => {
                    document.getElementById("serverUrl").value = d.url || "";
                    document.getElementById("serverApiKey").value = d.apiKey || "";
                    document.getElementById("serverEnabled").checked = d.enabled || false;
                    document.getElementById("serverStatus").innerText = d.enabled ? "Habilitado" : "Deshabilitado";
                })
                .catch(e => {
                    document.getElementById("serverStatus").innerText = "Error al cargar";
                });
        }

        function saveServer() {
            const url = document.getElementById("serverUrl").value;
            const apiKey = document.getElementById("serverApiKey").value;
            const enabled = document.getElementById("serverEnabled").checked ? "1" : "0";
            
            const st = document.getElementById("serverStatus");
            st.innerText = "Guardando...";
            st.className = "text-sm text-blue-500 font-mono";
            
            fetch(`/save_server?url=${encodeURIComponent(url)}&apiKey=${encodeURIComponent(apiKey)}&enabled=${enabled}`)
                .then(r => r.text())
                .then(t => {
                    st.innerText = t;
                    st.className = "text-sm text-green-600 font-mono font-bold";
                })
                .catch(e => {
                    st.innerText = "Error de conexión";
                    st.className = "text-sm text-red-500 font-mono";
                });
        }

        // Load server config on page load
        loadServerConfig();
    </script>
</body>
</html>
)rawliteral";

const char setup_wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configurar WiFi - GIPIS</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <style>body { font-family: sans-serif; }</style>
</head>
<body class="bg-slate-100 flex items-center justify-center h-screen px-4">
    <div class="bg-white p-8 rounded-xl shadow-lg max-w-md w-full">
        <div class="text-center mb-6">
            <i class="fas fa-wifi text-4xl text-blue-600 mb-2"></i>
            <h1 class="text-2xl font-bold text-slate-700">Configurar WiFi</h1>
            <p class="text-slate-500 text-sm">Estación Meteorológica GIPIS</p>
        </div>
        
        <!-- Scan List -->
        <div id="scanResults" class="mb-4 max-h-40 overflow-y-auto border border-slate-200 rounded hidden">
            <!-- Populated via JS -->
        </div>

        <button onclick="scanWifi()" class="w-full bg-slate-200 text-slate-700 py-2 rounded mb-4 hover:bg-slate-300 transition">
            <i class="fas fa-sync-alt"></i> Escanear Redes
        </button>

        <div class="space-y-4">
            <div>
                <label class="block text-sm font-bold text-slate-600">Nombre de Red (SSID)</label>
                <input type="text" id="ssid" class="w-full border border-slate-300 rounded px-3 py-2 mt-1 focus:ring-2 focus:ring-blue-500 focus:outline-none">
            </div>
            <div>
                <label class="block text-sm font-bold text-slate-600">Contraseña</label>
                <input type="password" id="pass" class="w-full border border-slate-300 rounded px-3 py-2 mt-1 focus:ring-2 focus:ring-blue-500 focus:outline-none">
            </div>
            <button onclick="save()" class="w-full bg-blue-600 text-white py-3 rounded font-bold hover:bg-blue-700 transition">
                Guardar y Reiniciar
            </button>
        </div>
        <div id="status" class="mt-4 text-center text-sm font-mono h-5"></div>
    </div>

    <script>
        function scanWifi() {
            const list = document.getElementById('scanResults');
            list.innerHTML = '<div class="p-2 text-center text-slate-500">Iniciando escaneo...</div>';
            list.classList.remove('hidden');
            
            // 1. Trigger Scan
            fetch('/start_scan')
                .then(r => {
                    if(r.ok) {
                        list.innerHTML = '<div class="p-2 text-center text-slate-500"><i class="fas fa-spinner fa-spin"></i> Escaneando...</div>';
                        pollResults();
                    } else {
                        list.innerHTML = '<div class="p-2 text-center text-red-500">Error al iniciar</div>';
                    }
                })
                .catch(e => {
                     list.innerHTML = '<div class="p-2 text-center text-red-500">Error de conexión</div>';
                });
        }

        function pollResults() {
            const list = document.getElementById('scanResults');
            let attempts = 0;
            const maxAttempts = 10; // 10s timeout
            
            const interval = setInterval(() => {
                attempts++;
                fetch('/scan_results').then(r => r.json()).then(data => {
                    // Check if data is status object or array
                    if(data.status === "running") {
                        // Keep waiting
                        if(attempts > maxAttempts) {
                            clearInterval(interval);
                            list.innerHTML = '<div class="p-2 text-center text-red-500">Tiempo de espera agotado</div>';
                        }
                    } else {
                        // It's the array!
                        clearInterval(interval);
                        renderList(data);
                    }
                }).catch(e => {
                    // Ignore errors during poll (maybe busy)
                });
            }, 1000);
        }
        
        function renderList(data) {
             const list = document.getElementById('scanResults');
             list.innerHTML = '';
             if(data.length === 0) {
                 list.innerHTML = '<div class="p-2 text-center text-slate-500">Sin redes encontradas</div>';
                 return;
             }
             data.forEach(net => {
                const item = document.createElement('div');
                item.className = 'p-2 hover:bg-blue-50 cursor-pointer border-b last:border-0 flex justify-between items-center';
                item.innerHTML = `<span>${net.ssid}</span> <span class="text-xs bg-slate-200 px-2 rounded">${net.rssi}dB</span>`;
                item.onclick = () => { document.getElementById('ssid').value = net.ssid; document.getElementById('pass').focus(); };
                list.appendChild(item);
            });
        }

        function save() {
            const s = document.getElementById('ssid').value;
            const p = document.getElementById('pass').value;
            const st = document.getElementById('status');
            
            if(!s) { st.innerText = "Falta SSID"; st.className = "mt-4 text-center text-sm font-mono text-red-500"; return; }
            
            st.innerText = "Guardando...";
            st.className = "mt-4 text-center text-sm font-mono text-blue-500";
            
            fetch(`/save_wifi?ssid=${encodeURIComponent(s)}&pass=${encodeURIComponent(p)}`)
                .then(r => r.text())
                .then(t => {
                    st.innerText = "Guardado! Reiniciando...";
                    st.className = "mt-4 text-center text-sm font-mono text-green-600 font-bold";
                })
                .catch(e => {
                    st.innerText = "Error al guardar";
                    st.className = "mt-4 text-center text-sm font-mono text-red-500";
                });
        }
    </script>
</body>
</html>
)rawliteral";

#endif
