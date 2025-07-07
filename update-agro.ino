#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* defaultSsid = "Agrotech-1";
const char* defaultPassword = "12345678";

String currentSsid;
String currentPassword;

WebServer server(80);

const int relayPin = 5;

// Waktu
unsigned long timeBaseMillis = 0;
int zoneHour = 12;
int zoneMinute = 0;
int zoneSecond = 0;

unsigned long lastCheck = 0;
bool relayShouldBeOn = false;

// Mode relay
bool isManualMode = false;
bool manualRelayState = false;

void getCurrentTime(int &h, int &m, int &s) {
  unsigned long elapsed = millis() - timeBaseMillis;
  int totalSeconds = zoneHour * 3600 + zoneMinute * 60 + zoneSecond + elapsed / 1000;
  totalSeconds %= (24 * 3600);

  h = totalSeconds / 3600;
  m = (totalSeconds % 3600) / 60;
  s = (totalSeconds % 60);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
<title>Agrotech Panel</title>
<style>
*, *::before, *::after {
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', sans-serif;
    background: linear-gradient(135deg, #d8f3dc, #b7e4c7, #a5d8ff);
    margin: 0;
    padding: 0;
    color: #111;
}
header {
    background: #2d6a4f;
    color: white;
    padding: 20px;
    text-align: center;
    font-size: 26px;
    font-weight: bold;
}
.container {
    max-width: 500px;
    margin: 20px auto;
    background: #f5f0c9; /* krem sedikit lebih gelap */
    padding: 25px;
    border-radius: 15px;
    box-shadow: 0 4px 15px rgba(0,0,0,0.1);
}
h2 {
    font-size: 22px;
    margin-top: 25px;
    margin-bottom: 15px;
    color: #1b4332; /* Hijau gelap alami */
    background: linear-gradient(90deg, #95d5b2, #74c69d);
    padding: 10px 15px;
    border-radius: 10px;
    text-shadow: 0 1px 1px rgba(0,0,0,0.15); /* Membuat teks lebih hidup */
}
button {
    background: linear-gradient(90deg, #38b000, #0081a7);
    color: white;
    border: none;
    padding: 14px 20px;
    font-size: 18px;
    cursor: pointer;
    border-radius: 10px;
    margin-top: 10px;
    width: 100%;
    transition: background 0.3s, transform 0.1s;
}
button:hover {
    background: linear-gradient(90deg, #0081a7, #38b000);
    transform: scale(1.02);
}
.reset {
    background: linear-gradient(90deg, #d00000, #f48c06);
}
.reset:hover {
    background: linear-gradient(90deg, #9d0208, #d00000);
}
input {
    width: 100%;
    padding: 14px;
    margin-top: 8px;
    font-size: 18px;
    border: 1px solid #ccc;
    border-radius: 10px;
    background: #fff;
}
input:focus {
    outline: none;
    border: 1px solid #38b000;
}
ul {
    list-style: none;
    padding: 0;
}
li {
    background: #e9edc9;
    margin: 8px 0;
    padding: 12px 15px;
    border-left: 6px solid #38b000;
    border-radius: 8px;
    font-size: 18px;
}
label.switch {
    display: inline-block;
    width: 60px;
    height: 34px;
    position: relative;
}
label.switch input {
    display: none;
}
label.switch .slider {
    position: absolute;
    cursor: pointer;
    top: 0; left: 0; right: 0; bottom: 0;
    background: #ccc;
    transition: 0.4s;
    border-radius: 34px;
}
label.switch .slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background: white;
    transition: 0.4s;
    border-radius: 50%;
}
label.switch input:checked + .slider {
    background: #38b000;
}
label.switch input:checked + .slider:before {
    transform: translateX(26px);
}
hr {
    border: none;
    border-top: 1px solid #ccc;
    margin: 20px 0;
}
p {
    font-size: 18px;
}
</style>
</head>
<body>
<header>🌿 Agrotech Panel</header>
<div class="container">
    <h2>🔌 Manual Relay</h2>
    <label class="switch">
        <input type="checkbox" id="relaySwitch" onchange="toggleRelay()">
        <span class="slider"></span>
    </label>
    <button onclick="setAuto()">Mode Otomatis</button>
    <p id="relayStatus"></p>

    <hr>

    <h2>🕒 Waktu Sekarang</h2>
    <input type="time" id="timeInput" value="12:00">
    <button onclick="setTime()">Set Waktu</button>
    <p id="currentTime"></p>

    <hr>

    <h2>⏰ Tambah / Edit Waktu ON/OFF</h2>
    <input type="hidden" id="alarmId">
    <input type="text" id="label" placeholder="Label Alarm " required>
    <input type="time" id="start" value="13:00" required>
    <input type="time" id="end" value="14:00" required>
    <button onclick="addAlarm()">Simpan Alarm</button>

    <hr>

    <h2>📋 Daftar Waktu On/Off</h2>
    <ul id="alarmList"></ul>

    <hr>

    <h2>📶 Pengaturan WiFi</h2>
    <input type="text" id="wifiSsid" min="3" placeholder="WiFi SSID">
    <input type="text" id="wifiPass" min="8" placeholder="WiFi Password">
    <button onclick="saveWifiConfig()">Simpan WiFi</button>
    <button class="reset" onclick="resetDevice()">Reset Semua Data</button>
</div>
</body>
<script>
function toggleRelay() {
  let sw = document.getElementById('relaySwitch');
  let statusText = document.getElementById('relayStatus');
  if (sw.checked) {
    fetch('/on').then(() => loadRelayStatus());
  } else {
    fetch('/off').then(() => loadRelayStatus());
  }
}

function setAuto() {
  fetch('/auto').then(() => {
    alert("Relay pindah ke AUTO");
    loadRelayStatus();
  });
}

function setTime() {
  let t = document.getElementById('timeInput').value;
  fetch('/setTime', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify({time: t})
  }).then(() => alert('Waktu di-set!'));
}

function addAlarm() {
  let alarm = {
    id: document.getElementById('alarmId').value,
    label: document.getElementById('label').value,
    start: document.getElementById('start').value,
    end: document.getElementById('end').value
  };
  fetch('/saveAlarm', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify(alarm)
  }).then(() => {
    loadAlarms();
    clearForm();
  });
}

function editAlarm(id) {
  fetch('/getAlarms').then(r => r.json()).then(list => {
    let alarm = list.find(a => a.id === id);
    if (alarm) {
      document.getElementById('alarmId').value = alarm.id;
      document.getElementById('label').value = alarm.label;
      document.getElementById('start').value = alarm.start;
      document.getElementById('end').value = alarm.end;
    }
  });
}

function deleteAlarm(id) {
  if (confirm("Yakin hapus alarm?")) {
    fetch('/deleteAlarm', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({id})
    }).then(() => loadAlarms());
  }
}

function loadAlarms() {
  fetch('/getAlarms').then(r => r.json()).then(list => {
    let html = "";
    list.forEach(a => {
      html += `
        <li>
          <strong>${a.label}</strong><br>${a.start} - ${a.end}<br>
          <button onclick="editAlarm('${a.id}')">Edit</button>
          <button onclick="deleteAlarm('${a.id}')">Hapus</button>
        </li>
      `;
    });
    document.getElementById('alarmList').innerHTML = html;
  });
}

function loadTime() {
  fetch('/getTime').then(r => r.json()).then(t => {
    document.getElementById('currentTime').innerText = "Jam saat ini: " + t.time;
  });
}

function loadRelayStatus() {
  fetch('/getRelayStatus')
    .then(r => r.json())
    .then(data => {
      const sw = document.getElementById('relaySwitch');
      const statusText = document.getElementById('relayStatus');
      if (data.mode === "manual") {
        sw.checked = data.status === "on";
        statusText.textContent = "Relay " + (data.status === "on" ? "ON (Manual)" : "OFF (Manual)");
      } else {
        sw.checked = false;
        statusText.textContent = "AUTO MODE aktif. Relay " + (data.status === "on" ? "ON" : "OFF");
      }
    });
}

function loadWifiConfig() {
  fetch('/getWifiConfig')
    .then(r => r.json())
    .then(cfg => {
      document.getElementById('wifiSsid').value = cfg.ssid || "";
      document.getElementById('wifiPass').value = cfg.password || "";
    });
}

function saveWifiConfig() {
  let data = {
    ssid: document.getElementById('wifiSsid').value,
    password: document.getElementById('wifiPass').value
  };
  fetch('/saveWifiConfig', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify(data)
  }).then(() => alert("WiFi config disimpan!"));
}

function resetDevice() {
  if (confirm("Yakin reset semua data?")) {
    fetch('/reset', { method: 'POST' }).then(() => {
      alert("Device akan restart!");
      location.reload();
    });
  }
}

function clearForm() {
  document.getElementById('alarmId').value = "";
  document.getElementById('label').value = "";
  document.getElementById('start').value = "13:00";
  document.getElementById('end').value = "14:00";
}

// Mulai interval realtime
setInterval(() => {
  loadTime();
  loadRelayStatus();
  loadAlarms(); // jika ingin alarm juga selalu update di semua perangkat
}, 2000);

// Inisialisasi awal saat halaman dimuat
loadAlarms();
loadTime();
loadRelayStatus();
loadWifiConfig();
</script>


</html>
  )rawliteral";

  server.send(200, "text/html", html);
}
void handleGetRelayStatus() {
  DynamicJsonDocument doc(128);
  doc["mode"] = isManualMode ? "manual" : "auto";
  doc["status"] = relayShouldBeOn ? "on" : "off";
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleOn() {
  isManualMode = true;
  manualRelayState = true;
  relayShouldBeOn = true;
  digitalWrite(relayPin, LOW);
  server.send(200, "application/json", "{\"status\":\"Relay ON (Manual)\"}");
}

void handleOff() {
  isManualMode = true;
  manualRelayState = false;
  relayShouldBeOn = false;
  digitalWrite(relayPin, HIGH);
  server.send(200, "application/json", "{\"status\":\"Relay OFF (Manual)\"}");
}

void handleAuto() {
  isManualMode = false;
  server.send(200, "application/json", "{\"status\":\"Relay AUTO mode aktif\"}");
}

void handleSetTime() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    String timeStr = doc["time"];
    zoneHour = timeStr.substring(0, 2).toInt();
    zoneMinute = timeStr.substring(3, 5).toInt();
    zoneSecond = 0;
    timeBaseMillis = millis();

    File f = LittleFS.open("/zonetime.txt", "w");
    f.println(timeStr);
    f.close();

    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
}

void handleGetTime() {
  int h, m, s;
  getCurrentTime(h, m, s);
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  DynamicJsonDocument doc(128);
  doc["time"] = buf;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveAlarm() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    String id = doc["id"] | String((uint32_t)esp_random(), HEX);

    DynamicJsonDocument alarmsDoc(4096);
    JsonArray arr;
    if (LittleFS.exists("/alarms.json")) {
      File f = LittleFS.open("/alarms.json", "r");
      deserializeJson(alarmsDoc, f);
      f.close();
      arr = alarmsDoc.as<JsonArray>();
    } else {
      arr = alarmsDoc.to<JsonArray>();
    }

    bool updated = false;
    for (JsonObject a : arr) {
      if (a["id"].as<String>() == id) {
        a["label"] = doc["label"];
        a["start"] = doc["start"];
        a["end"] = doc["end"];
        updated = true;
        break;
      }
    }

    if (!updated) {
      JsonObject newAlarm = arr.createNestedObject();
      newAlarm["id"] = id;
      newAlarm["label"] = doc["label"];
      newAlarm["start"] = doc["start"];
      newAlarm["end"] = doc["end"];
    }

    File f = LittleFS.open("/alarms.json", "w");
    serializeJson(arr, f);
    f.close();

    DynamicJsonDocument response(256);
    response["id"] = id;
    String json;
    serializeJson(response, json);
    server.send(200, "application/json", json);
  }
}

void handleDeleteAlarm() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    String idToDelete = doc["id"];

    DynamicJsonDocument alarmsDoc(4096);
    JsonArray newArr = alarmsDoc.to<JsonArray>();

    if (LittleFS.exists("/alarms.json")) {
      File f = LittleFS.open("/alarms.json", "r");
      deserializeJson(alarmsDoc, f);
      f.close();

      for (JsonObject alarm : alarmsDoc.as<JsonArray>()) {
        if (alarm["id"].as<String>() != idToDelete) {
          JsonObject obj = newArr.createNestedObject();
          obj["id"] = alarm["id"];
          obj["label"] = alarm["label"];
          obj["start"] = alarm["start"];
          obj["end"] = alarm["end"];
        }
      }
    }

    File f = LittleFS.open("/alarms.json", "w");
    serializeJson(newArr, f);
    f.close();

    server.send(200, "application/json", "{\"status\":\"Alarm deleted\"}");
  }
}

void handleGetAlarms() {
  if (LittleFS.exists("/alarms.json")) {
    File f = LittleFS.open("/alarms.json", "r");
    String data = f.readString();
    f.close();
    server.send(200, "application/json", data);
  } else {
    server.send(200, "application/json", "[]");
  }
}

void handleSaveWifiConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    currentSsid = doc["ssid"].as<String>();
    currentPassword = doc["password"].as<String>();

    File f = LittleFS.open("/wifi.json", "w");
    serializeJson(doc, f);
    f.close();

    server.send(200, "application/json", "{\"status\":\"WiFi saved\"}");
  }
}

void handleGetWifiConfig() {
  DynamicJsonDocument doc(256);

  if (LittleFS.exists("/wifi.json")) {
    File f = LittleFS.open("/wifi.json", "r");
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
      doc["ssid"] = defaultSsid;
      doc["password"] = defaultPassword;
    }
  } else {
    doc["ssid"] = defaultSsid;
    doc["password"] = defaultPassword;
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleReset() {
  LittleFS.format();
  server.send(200, "application/json", "{\"status\":\"reset done\"}");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    while (true);
  }

  // Load WiFi config
  DynamicJsonDocument doc(256);
  if (LittleFS.exists("/wifi.json")) {
    File f = LittleFS.open("/wifi.json", "r");
    deserializeJson(doc, f);
    f.close();
    currentSsid = doc["ssid"].as<String>();
    currentPassword = doc["password"].as<String>();
  } else {
    currentSsid = defaultSsid;
    currentPassword = defaultPassword;
  }

  WiFi.softAP(currentSsid.c_str(), currentPassword.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.println("ESP32 AP IP Address: " + IP.toString());

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/auto", handleAuto);
  server.on("/setTime", HTTP_POST, handleSetTime);
  server.on("/getTime", HTTP_GET, handleGetTime);
  server.on("/saveAlarm", HTTP_POST, handleSaveAlarm);
  server.on("/getAlarms", HTTP_GET, handleGetAlarms);
  server.on("/deleteAlarm", HTTP_POST, handleDeleteAlarm);
  server.on("/saveWifiConfig", HTTP_POST, handleSaveWifiConfig);
  server.on("/getWifiConfig", HTTP_GET, handleGetWifiConfig);
  server.on("/getRelayStatus", HTTP_GET, handleGetRelayStatus);
  server.on("/reset", HTTP_POST, handleReset);

  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    checkAlarms();
  }
}

void checkAlarms() {
  if (isManualMode) {
    digitalWrite(relayPin, manualRelayState ? LOW : HIGH);
    return;
  }

  int h, m, s;
  getCurrentTime(h, m, s);
  int nowMin = h * 60 + m;

  bool shouldBeOn = false;

  if (LittleFS.exists("/alarms.json")) {
    File f = LittleFS.open("/alarms.json", "r");
    String data = f.readString();
    f.close();

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, data);
    if (!err) {
      for (JsonObject alarm : doc.as<JsonArray>()) {
        int startMin = alarm["start"].as<String>().substring(0, 2).toInt() * 60
                       + alarm["start"].as<String>().substring(3, 5).toInt();
        int endMin = alarm["end"].as<String>().substring(0, 2).toInt() * 60
                                          + alarm["end"].as<String>().substring(3, 5).toInt();

        bool active;
        if (startMin <= endMin) {
          active = nowMin >= startMin && nowMin < endMin;
        } else {
          // Jika alarm melewati tengah malam (contoh: 23:00 - 02:00)
          active = nowMin >= startMin || nowMin < endMin;
        }

        if (active) {
          shouldBeOn = true;
          break; // Jika salah satu alarm aktif, langsung ON
        }
      }
    }
  }

  if (shouldBeOn != relayShouldBeOn) {
    relayShouldBeOn = shouldBeOn;
    digitalWrite(relayPin, relayShouldBeOn ? LOW : HIGH);
    Serial.printf("Relay changed to %s\n", relayShouldBeOn ? "ON" : "OFF");
  }
}
