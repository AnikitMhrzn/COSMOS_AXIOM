#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "SoftGirl";      // ← Change this
const char* password = "AHaa4456";  // ← Change this

// ==================== PIN CONFIGURATION ====================
#define ONE_WIRE_BUS D5        // DS18B20 data pin (REAL sensor)
#define MQ7_PIN A0             // MQ7 analog pin (REAL sensor)

// ==================== DS18B20 SETUP ====================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ==================== WEB SERVER ====================
ESP8266WebServer server(80);

// ==================== MQ7 CALIBRATION ====================
// MQ7 outputs an analog voltage proportional to CO concentration, but the
// relationship is non-linear and depends on the sensor's load resistor and
// a clean-air baseline reading. The mapping below is a simple linear
// approximation good enough for relative trends (rising/falling CO), not
// certified ppm accuracy.
//
// To calibrate properly:
//   1. Let the sensor run in clean air for 24-48h (burn-in period).
//   2. Note the steady ADC reading in clean air -> that's your RAW_CLEAN_AIR.
//   3. Use a reference CO source (or accept approximate scaling) to map
//      higher readings to ppm using the sensor's datasheet curve.
#define MQ7_RAW_MIN 0      // ADC value in clean air (tune this)
#define MQ7_RAW_MAX 1023   // ADC value at sensor's max range
#define MQ7_PPM_MIN 0      // ppm at RAW_MIN
#define MQ7_PPM_MAX 1000   // ppm at RAW_MAX (check your MQ7 datasheet)

// ==================== SIMULATION VARIABLES (humidity only) ====================
float simulatedHumidity = 0;
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

  // Seed random (humidity simulation only)
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

// ==================== SIMULATION FUNCTIONS (humidity only) ====================
void updateSimulatedData() {
  if (millis() - lastSimUpdate < 2000) return;
  lastSimUpdate = millis();

  // Humidity drifts gently between 40-70%
  float humidityBase = 55.0;
  simulatedHumidity += random(-30, 31) / 10.0;
  simulatedHumidity = constrain(simulatedHumidity, 40.0, 70.0);
}

// ==================== REAL SENSOR READING FUNCTIONS ====================
float readRealTemperature() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if (tempC == -127.0 || tempC == 85.0) return NAN;
  return tempC;
}

int readRealCO() {
  int rawADC = analogRead(MQ7_PIN);
  int ppm = map(rawADC, MQ7_RAW_MIN, MQ7_RAW_MAX, MQ7_PPM_MIN, MQ7_PPM_MAX);
  ppm = constrain(ppm, MQ7_PPM_MIN, MQ7_PPM_MAX);
  return ppm;
}

// ==================== WEB SERVER HANDLERS ====================
void handleRoot() {
  float realTemp = readRealTemperature();
  int rawADC = analogRead(MQ7_PIN);
  int coPpm = readRealCO();
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
      <!-- DS18B20 TEMPERATURE -->
      <div class="card">
        <div class="card-header">
          <span class="icon">🌡️</span>
          <span class="card-title">Temperature</span>
        </div>
        <div class="value-big">)rawliteral" + String(isnan(realTemp) ? 0 : realTemp, 2) + R"rawliteral(<span class="value-unit">°C</span></div>
        <div class="sub-value">)rawliteral" + String(isnan(realTemp) ? 0 : realTemp * 1.8 + 32, 1) + R"rawliteral( °F</div>
        <div class="progress-bar"><div class="progress-fill temp-fill" style="width: )rawliteral" + String(isnan(realTemp) ? 0 : constrain(map(realTemp, -10, 50, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
      </div>
      
      <!-- HUMIDITY -->
      <div class="card">
        <div class="card-header">
          <span class="icon">💧</span>
          <span class="card-title">Humidity</span>
        </div>
        <div class="value-big">)rawliteral" + String(simulatedHumidity, 1) + R"rawliteral(<span class="value-unit">%</span></div>
        <div class="progress-bar"><div class="progress-fill hum-fill" style="width: )rawliteral" + String(constrain(map(simulatedHumidity, 0, 100, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
      </div>
      
      <!-- MQ7 CO SENSOR -->
      <div class="card">
        <div class="card-header">
          <span class="icon">☠️</span>
          <span class="card-title">CO Sensor</span>
        </div>
        <div class="value-big">)rawliteral" + String(coPpm) + R"rawliteral(<span class="value-unit">ppm</span></div>
        <div class="sub-value">Raw ADC: )rawliteral" + String(rawADC) + R"rawliteral( / 1023</div>
        <div class="progress-bar"><div class="progress-fill co-fill" style="width: )rawliteral" + String(constrain(map(coPpm, MQ7_PPM_MIN, MQ7_PPM_MAX, 0, 100), 0, 100)) + R"rawliteral(%"></div></div>
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
  int rawADC = analogRead(MQ7_PIN);
  int coPpm = readRealCO();
  updateSimulatedData();

  String json = "{";
  json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"uptime\":" + String(millis()/1000) + ",";
  json += "\"temperature\":{\"value\":" + String(isnan(realTemp) ? 0 : realTemp, 2) + ",\"status\":\"" + String(isnan(realTemp) ? "error" : "ok") + "\"},";
  json += "\"humidity\":{\"value\":" + String(simulatedHumidity, 1) + "},";
  json += "\"co\":{\"ppm\":" + String(coPpm) + ",\"raw_adc\":" + String(rawADC) + "}";
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
    int rawADC = analogRead(MQ7_PIN);
    int coPpm = readRealCO();

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

    // DS18B20 Temperature
    Serial.print("🌡️  Temperature → ");
    if (isnan(realTemp)) {
      Serial.println("ERROR - Check wiring!");
    } else {
      Serial.print(realTemp, 2);
      Serial.println(" °C");
    }

    // Humidity (simulated)
    Serial.print("💧 Humidity → ");
    Serial.print(simulatedHumidity, 1);
    Serial.println(" %");

    // MQ7 CO
    Serial.print("☠️  CO Level → ");
    Serial.print(coPpm);
    Serial.print(" ppm  |  Raw ADC: ");
    Serial.println(rawADC);

    Serial.println("----------------------------------------");
    Serial.print("🌐 Web Dashboard: http://");
    Serial.println(WiFi.localIP());
    Serial.println("----------------------------------------\n");
  }

  delay(10);
}