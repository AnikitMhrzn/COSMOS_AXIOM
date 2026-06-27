#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "SoftGirl";      // ← Change this
const char* password = "AHaa4456";  // ← Change this

// ==================== PIN CONFIGURATION ====================
#define ONE_WIRE_BUS D5        // DS18B20 data pin (REAL sensor)
#define DHT_PIN D4             // DHT22 data pin (SIMULATED - broken)
#define MQ7_PIN A0             // MQ7 analog pin (SIMULATED - broken)

// ==================== DS18B20 SETUP (REAL) ====================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ==================== WEB SERVER ====================
ESP8266WebServer server(80);

// ==================== SIMULATION VARIABLES ====================
float simulatedTemp = 0;
float simulatedHumidity = 0;
int simulatedCO = 0;
unsigned long lastSimUpdate = 0;
unsigned long lastPrint = 0;
unsigned long lastWiFiCheck = 0;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  ESP8266 Multi-Sensor + WiFi Test");
  Serial.println("========================================\n");

  // Initialize real DS18B20
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  Serial.print("DS18B20 sensors found: ");
  Serial.println(deviceCount);
  if (deviceCount == 0) {
    Serial.println("WARNING: No DS18B20 detected! Check wiring.");
  }

  // Seed random
  randomSeed(analogRead(A0) + millis());
  updateSimulatedData();

  // ==================== CONNECT TO WIFI ====================
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WIFI CONNECTED SUCCESSFULLY!");
    Serial.print("📶 Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("🌐 IP Address: http://");
    Serial.println(WiFi.localIP());
    Serial.print("🔧 MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("📡 Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("🚪 Gateway: ");
    Serial.println(WiFi.gatewayIP());
  } else {
    Serial.println("❌ WIFI CONNECTION FAILED!");
    Serial.println("   Check SSID and Password.");
    Serial.println("   Continuing without WiFi...");
  }

  // Start web server
  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("\n🌐 Web server started on port 80");
  Serial.println("========================================\n");
}

// ==================== SIMULATION FUNCTIONS ====================
void updateSimulatedData() {
  if (millis() - lastSimUpdate < 2000) return;
  lastSimUpdate = millis();

  float tempChange = (random(-10, 11) / 10.0);
  simulatedTemp += tempChange;
  simulatedTemp = constrain(simulatedTemp, 20.0, 30.0);
  if (simulatedTemp == 20.0 || simulatedTemp == 30.0) {
    simulatedTemp += (simulatedTemp == 20.0 ? 0.5 : -0.5);
  }

  float humidityBase = 55.0 - (simulatedTemp - 25.0) * 2.0;
  simulatedHumidity = humidityBase + random(-30, 31) / 10.0;
  simulatedHumidity = constrain(simulatedHumidity, 40.0, 70.0);

  int coChange = random(-5, 6);
  simulatedCO += coChange;
  simulatedCO = constrain(simulatedCO, 10, 150);
  if (random(0, 100) < 5) {
    simulatedCO += random(20, 50);
    simulatedCO = constrain(simulatedCO, 10, 150);
  }
}

float readRealTemperature() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if (tempC == -127.0 || tempC == 85.0) return NAN;
  return tempC;
}

// ==================== WEB SERVER HANDLERS ====================
void handleRoot() {
  float realTemp = readRealTemperature();
  updateSimulatedData();

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 Sensor Dashboard</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: #eaeaea; min-height: 100vh; padding: 20px;
    }
    .container { max-width: 900px; margin: 0 auto; }
    h1 { text-align: center; margin-bottom: 10px; font-size: 2em; }
    .status-bar {
      background: rgba(255,255,255,0.05); border-radius: 12px;
      padding: 15px; margin-bottom: 25px; display: flex;
      justify-content: space-around; flex-wrap: wrap; gap: 10px;
    }
    .status-item { text-align: center; }
    .status-label { font-size: 0.75em; color: #888; text-transform: uppercase; }
    .status-value { font-size: 1.1em; font-weight: bold; color: #4ecca3; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 20px; }
    .card {
      background: rgba(255,255,255,0.07); border-radius: 16px;
      padding: 25px; backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.1);
      transition: transform 0.2s;
    }
    .card:hover { transform: translateY(-3px); }
    .card-header { display: flex; align-items: center; gap: 12px; margin-bottom: 15px; }
    .icon { font-size: 2em; }
    .card-title { font-size: 1.1em; color: #aaa; }
    .badge {
      font-size: 0.65em; padding: 3px 8px; border-radius: 10px;
      font-weight: bold; margin-left: auto;
    }
    .badge-real { background: #2ecc71; color: #fff; }
    .badge-sim { background: #e74c3c; color: #fff; }
    .value-big { font-size: 3em; font-weight: 300; margin: 10px 0; }
    .value-unit { font-size: 0.4em; color: #888; }
    .sub-value { font-size: 1.2em; color: #bbb; margin-top: 8px; }
    .progress-bar {
      width: 100%; height: 6px; background: rgba(255,255,255,0.1);
      border-radius: 3px; margin-top: 15px; overflow: hidden;
    }
    .progress-fill { height: 100%; border-radius: 3px; transition: width 1s; }
    .temp-fill { background: linear-gradient(90deg, #3498db, #e74c3c); }
    .hum-fill { background: linear-gradient(90deg, #3498db, #2ecc71); }
    .co-fill { background: linear-gradient(90deg, #2ecc71, #f39c12, #e74c3c); }
    .footer { text-align: center; margin-top: 30px; color: #666; font-size: 0.85em; }
    .refresh { text-align: center; margin-top: 15px; }
    .refresh a {
      display: inline-block; padding: 10px 25px;
      background: #4ecca3; color: #1a1a2e;
      text-decoration: none; border-radius: 25px;
      font-weight: bold; transition: opacity 0.2s;
    }
    .refresh a:hover { opacity: 0.8; }
    @media (max-width: 500px) {
      .value-big { font-size: 2.2em; }
      h1 { font-size: 1.5em; }
    }
  </style>
  <meta http-equiv="refresh" content="5">
</head>
<body>
  <div class="container">
    <h1>🌡️ Sensor Dashboard</h1>
    <div class="status-bar">
      <div class="status-item">
        <div class="status-label">WiFi Status</div>
        <div class="status-value">)rawliteral" + String(WiFi.status() == WL_CONNECTED ? "🟢 Connected" : "🔴 Disconnected") + R"rawliteral(</div>
      </div>
      <div class="status-item">
        <div class="status-label">IP Address</div>
        <div class="status-value">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</div>
      </div>
      <div class="status-item">
        <div class="status-label">Signal</div>
        <div class="status-value">)rawliteral" + String(WiFi.RSSI()) + R"rawliteral( dBm</div>
      </div>
      <div class="status-item">
        <div class="status-label">Uptime</div>
        <div class="status-value">)rawliteral" + String(millis()/1000) + R"rawliteral( s</div>
      </div>
    </div>
    
    <div class="grid">
      <!-- DS18B20 REAL -->
      <div class="card">
        <div class="card-header">
          <span class="icon">🌡️</span>
          <span class="card-title">DS18B20 Temperature</span>
          <span class="badge badge-real">REAL</span>
        </div>
        <div class="value-big">)rawliteral" + String(isnan(realTemp) ? 0 : realTemp, 2) + R"rawliteral(<span class="value-unit">°C</span></div>
        <div class="sub-value">)rawliteral" + String(isnan(realTemp) ? 0 : realTemp * 1.8 + 32, 1) + R"rawliteral( °F</div>
        <div class="progress-bar"><div class="progress-fill temp-fill" style="width: )rawliteral" + String(isnan(realTemp) ? 0 : constrain(map(realTemp, -10, 50, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
      </div>
      
      <!-- DHT22 SIMULATED -->
      <div class="card">
        <div class="card-header">
          <span class="icon">💧</span>
          <span class="card-title">DHT22 Environment</span>
          <span class="badge badge-sim">SIM</span>
        </div>
        <div class="value-big">)rawliteral" + String(simulatedTemp, 1) + R"rawliteral(<span class="value-unit">°C</span></div>
        <div class="sub-value">💧 Humidity: )rawliteral" + String(simulatedHumidity, 1) + R"rawliteral( %</div>
        <div class="progress-bar"><div class="progress-fill hum-fill" style="width: )rawliteral" + String(constrain(map(simulatedHumidity, 0, 100, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
      </div>
      
      <!-- MQ7 SIMULATED -->
      <div class="card">
        <div class="card-header">
          <span class="icon">☠️</span>
          <span class="card-title">MQ7 CO Sensor</span>
          <span class="badge badge-sim">SIM</span>
        </div>
        <div class="value-big">)rawliteral" + String(simulatedCO) + R"rawliteral(<span class="value-unit">ppm</span></div>
        <div class="sub-value">Raw ADC: )rawliteral" + String(map(simulatedCO, 0, 200, 0, 1023)) + R"rawliteral( / 1023</div>
        <div class="progress-bar"><div class="progress-fill co-fill" style="width: )rawliteral" + String(constrain(map(simulatedCO, 0, 200, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
      </div>
    </div>
    
    <div class="refresh">
      <a href="/">🔄 Refresh Now</a>
    </div>
    
    <div class="footer">
      ESP8266 Sensor Monitor | Auto-refresh every 5s | )rawliteral" + String(WiFi.status() == WL_CONNECTED ? "WiFi Connected" : "WiFi Offline") + R"rawliteral(
    </div>
  </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleAPI() {
  float realTemp = readRealTemperature();
  updateSimulatedData();
  
  String json = "{";
  json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"uptime\":" + String(millis()/1000) + ",";
  json += "\"ds18b20\":{\"real\":true,\"temperature\":" + String(isnan(realTemp) ? 0 : realTemp, 2) + ",\"status\":\"" + String(isnan(realTemp) ? "error" : "ok") + "\"},";
  json += "\"dht22\":{\"real\":false,\"temperature\":" + String(simulatedTemp, 1) + ",\"humidity\":" + String(simulatedHumidity, 1) + "},";
  json += "\"mq7\":{\"real\":false,\"co_ppm\":" + String(simulatedCO) + ",\"raw_adc\":" + String(map(simulatedCO, 0, 200, 0, 1023)) + "}";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// ==================== MAIN LOOP ====================
void loop() {
  server.handleClient();
  updateSimulatedData();

  // WiFi reconnection check every 30 seconds
  if (millis() - lastWiFiCheck > 30000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Attempting reconnect...");
      WiFi.reconnect();
    }
  }

  // Serial print every 2 seconds
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    float realTemp = readRealTemperature();

    Serial.println("----------------------------------------");
    Serial.println("📊 SENSOR READINGS");
    Serial.println("----------------------------------------");

    // WiFi Status
    Serial.print("📡 WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected | IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    } else {
      Serial.println("DISCONNECTED");
    }

    // REAL DS18B20
    Serial.print("🌡️  DS18B20 (REAL)  → Temperature: ");
    if (isnan(realTemp)) {
      Serial.println("ERROR - Check wiring!");
    } else {
      Serial.print(realTemp, 2);
      Serial.println(" °C");
    }

    // SIMULATED DHT22
    Serial.print("💧 DHT22  (SIM)     → Temperature: ");
    Serial.print(simulatedTemp, 1);
    Serial.print(" °C  |  Humidity: ");
    Serial.print(simulatedHumidity, 1);
    Serial.println(" %");

    // SIMULATED MQ7
    Serial.print("☠️  MQ7    (SIM)     → CO Level: ");
    Serial.print(simulatedCO);
    Serial.print(" ppm  |  Raw ADC: ");
    Serial.println(map(simulatedCO, 0, 200, 0, 1023));

    Serial.println("----------------------------------------");
    Serial.print("🌐 Web Dashboard: http://");
    Serial.println(WiFi.localIP());
    Serial.println("----------------------------------------\n");
  }

  delay(10);
}