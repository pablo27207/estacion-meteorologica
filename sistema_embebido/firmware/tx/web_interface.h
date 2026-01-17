#ifndef TX_WEB_INTERFACE_H
#define TX_WEB_INTERFACE_H

#include <Arduino.h>

// ============================================
// TX Status Page (simple, no dashboard needed)
// ============================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TX Estación GIPIS</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
</head>
<body class="bg-slate-100 min-h-screen p-4">
    <div class="max-w-2xl mx-auto">
        <div class="bg-white rounded-xl shadow-md p-6 mb-6">
            <div class="flex items-center gap-3 mb-4">
                <i class="fas fa-satellite-dish text-blue-600 text-2xl"></i>
                <h1 class="text-xl font-bold text-slate-700">TX Estación GIPIS</h1>
            </div>
            <div id="status" class="text-sm text-slate-600">Cargando...</div>
        </div>

        <!-- Sensor Data -->
        <div class="bg-white rounded-xl shadow-md p-6 mb-6">
            <h2 class="font-bold text-slate-700 mb-4"><i class="fas fa-thermometer-half text-blue-500 mr-2"></i>Sensores</h2>
            <div class="grid grid-cols-2 gap-4">
                <div class="bg-slate-50 p-3 rounded">
                    <div class="text-xs text-slate-500">Temp Aire</div>
                    <div class="text-xl font-bold" id="tempAire">--</div>
                </div>
                <div class="bg-slate-50 p-3 rounded">
                    <div class="text-xs text-slate-500">Hum Aire</div>
                    <div class="text-xl font-bold" id="humAire">--</div>
                </div>
                <div class="bg-slate-50 p-3 rounded">
                    <div class="text-xs text-slate-500">Temp Suelo</div>
                    <div class="text-xl font-bold" id="tempSuelo">--</div>
                </div>
                <div class="bg-slate-50 p-3 rounded">
                    <div class="text-xs text-slate-500">VWC Suelo</div>
                    <div class="text-xl font-bold" id="vwcSuelo">--</div>
                </div>
            </div>
        </div>

        <!-- Server Config -->
        <div class="bg-white rounded-xl shadow-md p-6">
            <h2 class="font-bold text-slate-700 mb-4"><i class="fas fa-cloud text-green-500 mr-2"></i>Servidor Remoto</h2>
            <div class="space-y-3">
                <div>
                    <label class="text-xs text-slate-500">URL</label>
                    <input type="text" id="serverUrl" class="w-full border rounded px-3 py-2 text-sm">
                </div>
                <div>
                    <label class="text-xs text-slate-500">API Key</label>
                    <input type="text" id="apiKey" class="w-full border rounded px-3 py-2 text-sm">
                </div>
                <div class="flex items-center gap-2">
                    <input type="checkbox" id="serverEnabled" class="w-4 h-4">
                    <label class="text-sm">Envío habilitado</label>
                </div>
                <button onclick="saveServer()" class="w-full bg-green-600 text-white py-2 rounded font-bold">
                    Guardar
                </button>
                <div id="saveStatus" class="text-sm text-center"></div>
            </div>
        </div>
    </div>

    <script>
        function loadData() {
            fetch('/data').then(r => r.json()).then(d => {
                document.getElementById('tempAire').innerText = d.tempAire.toFixed(1) + '°C';
                document.getElementById('humAire').innerText = d.humAire.toFixed(1) + '%';
                document.getElementById('tempSuelo').innerText = d.tempSuelo.toFixed(1) + '°C';
                document.getElementById('vwcSuelo').innerText = d.vwcSuelo.toFixed(1) + '%';
                document.getElementById('status').innerText = 'WiFi: ' + d.wifiRSSI + ' dBm | Server: ' + (d.serverOk ? 'OK' : 'Error');
            }).catch(e => {
                document.getElementById('status').innerText = 'Error de conexión';
            });
        }

        function loadServerConfig() {
            fetch('/get_server').then(r => r.json()).then(d => {
                document.getElementById('serverUrl').value = d.url || '';
                document.getElementById('apiKey').value = d.apiKey || '';
                document.getElementById('serverEnabled').checked = d.enabled;
            });
        }

        function saveServer() {
            const url = document.getElementById('serverUrl').value;
            const key = document.getElementById('apiKey').value;
            const enabled = document.getElementById('serverEnabled').checked ? '1' : '0';
            
            fetch('/save_server?url=' + encodeURIComponent(url) + '&apiKey=' + encodeURIComponent(key) + '&enabled=' + enabled)
                .then(r => r.text())
                .then(t => {
                    document.getElementById('saveStatus').innerText = 'Guardado!';
                    document.getElementById('saveStatus').className = 'text-sm text-center text-green-600';
                })
                .catch(e => {
                    document.getElementById('saveStatus').innerText = 'Error';
                    document.getElementById('saveStatus').className = 'text-sm text-center text-red-500';
                });
        }

        loadServerConfig();
        loadData();
        setInterval(loadData, 5000);
    </script>
</body>
</html>
)rawliteral";

// ============================================
// WiFi Setup Portal (same as RX)
// ============================================
const char setup_wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configurar WiFi - GIPIS TX</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
</head>
<body class="bg-slate-100 flex items-center justify-center h-screen px-4">
    <div class="bg-white p-8 rounded-xl shadow-lg max-w-md w-full">
        <div class="text-center mb-6">
            <i class="fas fa-wifi text-4xl text-blue-600 mb-2"></i>
            <h1 class="text-2xl font-bold text-slate-700">Configurar WiFi</h1>
            <p class="text-slate-500 text-sm">TX Estación Meteorológica GIPIS</p>
        </div>
        
        <div id="scanResults" class="mb-4 max-h-40 overflow-y-auto border border-slate-200 rounded hidden"></div>

        <button onclick="scanWifi()" class="w-full bg-slate-200 text-slate-700 py-2 rounded mb-4 hover:bg-slate-300 transition">
            <i class="fas fa-sync-alt"></i> Escanear Redes
        </button>

        <div class="space-y-4">
            <div>
                <label class="block text-sm font-bold text-slate-600">Nombre de Red (SSID)</label>
                <input type="text" id="ssid" class="w-full border border-slate-300 rounded px-3 py-2 mt-1">
            </div>
            <div>
                <label class="block text-sm font-bold text-slate-600">Contraseña</label>
                <input type="password" id="pass" class="w-full border border-slate-300 rounded px-3 py-2 mt-1">
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
            list.innerHTML = '<div class="p-2 text-center text-slate-500">Escaneando...</div>';
            list.classList.remove('hidden');
            
            fetch('/start_scan').then(() => {
                setTimeout(pollResults, 1000);
            });
        }

        function pollResults() {
            fetch('/scan_results').then(r => r.json()).then(data => {
                if(data.status === "running") {
                    setTimeout(pollResults, 1000);
                } else {
                    renderList(data);
                }
            });
        }
        
        function renderList(data) {
            const list = document.getElementById('scanResults');
            list.innerHTML = '';
            if(data.length === 0) {
                list.innerHTML = '<div class="p-2 text-center text-slate-500">Sin redes</div>';
                return;
            }
            data.forEach(net => {
                const item = document.createElement('div');
                item.className = 'p-2 hover:bg-blue-50 cursor-pointer border-b flex justify-between';
                item.innerHTML = `<span>${net.ssid}</span><span class="text-xs bg-slate-200 px-2 rounded">${net.rssi}dB</span>`;
                item.onclick = () => { document.getElementById('ssid').value = net.ssid; document.getElementById('pass').focus(); };
                list.appendChild(item);
            });
        }

        function save() {
            const s = document.getElementById('ssid').value;
            const p = document.getElementById('pass').value;
            const st = document.getElementById('status');
            
            if(!s) { st.innerText = "Falta SSID"; st.className = "text-red-500"; return; }
            
            st.innerText = "Guardando...";
            st.className = "text-blue-500";
            
            fetch('/save_wifi?ssid=' + encodeURIComponent(s) + '&pass=' + encodeURIComponent(p))
                .then(r => r.text())
                .then(t => {
                    st.innerText = "Guardado! Reiniciando...";
                    st.className = "text-green-600 font-bold";
                });
        }
    </script>
</body>
</html>
)rawliteral";

#endif
