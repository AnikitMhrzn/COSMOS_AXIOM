#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ---- WiFi Credentials ----
const char* WIFI_SSID     = "SoftGirl";
const char* WIFI_PASSWORD = "AHaa4456";

// ---- Discord Webhook ----
const char* DISCORD_WEBHOOK = "https://discord.com/api/webhooks/1517821505139114024/QChbf_NhGMEoW_zg48o1vd3SQjdYt_KKxgXTS8lPxkacw50Tz7cJyAMmazXsub66V-6R";

// ---- Pins ----
const int SDA_PIN      = 21;
const int SCL_PIN      = 22;
const int BUZZER_PIN   = 13;
const int PANIC_BTN    = 4;    // wire a button between GPIO4 (D4) and GND
const int BOOT_BTN     = 0;    // built-in BOOT button on ESP32

const int MPU_ADDR = 0x68;

int16_t accelX_raw, accelY_raw, accelZ_raw;
int16_t gyroX_raw, gyroY_raw, gyroZ_raw;
int16_t temperature;

float gyroX_offset = 0, gyroY_offset = 0, gyroZ_offset = 0;
float accelX_offset = 0, accelY_offset = 0, accelZ_offset = 0;

float angleX = 0, angleY = 0, angleZ = 0;
unsigned long lastTime;

// ---- Fall detection state ----
bool fallSuspected  = false;
bool fallSettling   = false;
bool fallLocked     = false;
unsigned long fallSettleStart = 0;
unsigned long fallLockStart   = 0;

// ---- Thresholds ----
float FREEFALL_THRESHOLD      = 0.85;
float IMPACT_THRESHOLD        = 1.1;
unsigned long SETTLE_TIME_MS  = 1000;
unsigned long WAIT_TIME_MS    = 2000;
float MOVEMENT_GYRO_THRESHOLD = 25.0;

// ---- WiFi ----
bool connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 60) {
      Serial.println("\nWiFi failed. Running without network.");
      return false;
    }
  }
  Serial.println();
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

// ---- Discord ----
void sendDiscordAlert(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - skipping Discord alert.");
    return;
  }

  HTTPClient http;
  http.begin(DISCORD_WEBHOOK);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  message.replace("\"", "\\\"");
  String payload = "{\"content\":\"" + message + "\",\"username\":\"Fall Detector\"}";

  int httpCode = http.POST(payload);
  if (httpCode == 204) {
    Serial.println("Discord alert sent!");
  } else {
    Serial.print("Discord failed. HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();
}

// ---- Buzzer with cancel support ----
// Returns true if cancelled by BOOT button (only after minBeepsBeforeCancel beeps)
bool buzzWithCancel(int times, int onMs, int offMs, int minBeepsBeforeCancel) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    delay(offMs);

    // Only allow cancel after minBeepsBeforeCancel beeps have completed
    if (i + 1 >= minBeepsBeforeCancel) {
      if (digitalRead(BOOT_BTN) == LOW) {
        Serial.println("BOOT button pressed - alert CANCELLED by user!");
        digitalWrite(BUZZER_PIN, LOW);
        return true; // cancelled
      }
    }
  }
  return false; // not cancelled
}

// ---- MPU6050 ----
void readRaw() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  accelX_raw = Wire.read() << 8 | Wire.read();
  accelY_raw = Wire.read() << 8 | Wire.read();
  accelZ_raw = Wire.read() << 8 | Wire.read();
  temperature = Wire.read() << 8 | Wire.read();
  gyroX_raw  = Wire.read() << 8 | Wire.read();
  gyroY_raw  = Wire.read() << 8 | Wire.read();
  gyroZ_raw  = Wire.read() << 8 | Wire.read();
}

void calibrate() {
  Serial.println("Calibrating... keep the sensor still and level.");

  long sumAX = 0, sumAY = 0, sumAZ = 0;
  long sumGX = 0, sumGY = 0, sumGZ = 0;
  const int samples = 1000;

  for (int i = 0; i < samples; i++) {
    readRaw();
    sumAX += accelX_raw; sumAY += accelY_raw; sumAZ += accelZ_raw;
    sumGX += gyroX_raw;  sumGY += gyroY_raw;  sumGZ += gyroZ_raw;
    delay(2);
  }

  gyroX_offset = sumGX / (float)samples;
  gyroY_offset = sumGY / (float)samples;
  gyroZ_offset = sumGZ / (float)samples;
  accelX_offset = sumAX / (float)samples;
  accelY_offset = sumAY / (float)samples;
  accelZ_offset = (sumAZ / (float)samples) - 16384.0;

  Serial.println("Calibration done.");
}

float wrap360(float angle) {
  angle = fmod(angle, 360.0);
  if (angle < 0) angle += 360.0;
  return angle;
}

void printThresholds() {
  Serial.println("---- Current Thresholds ----");
  Serial.print("FREEFALL_THRESHOLD: ");      Serial.println(FREEFALL_THRESHOLD);
  Serial.print("IMPACT_THRESHOLD: ");        Serial.println(IMPACT_THRESHOLD);
  Serial.print("SETTLE_TIME_MS: ");          Serial.println(SETTLE_TIME_MS);
  Serial.print("WAIT_TIME_MS: ");            Serial.println(WAIT_TIME_MS);
  Serial.print("MOVEMENT_GYRO_THRESHOLD: "); Serial.println(MOVEMENT_GYRO_THRESHOLD);
  Serial.println("-----------------------------");
}

void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd == "test")        { buzz(3, 300, 200); return; }
    if (cmd == "testdiscord") { sendDiscordAlert("Test alert - Fall detector online!"); return; }
    if (cmd == "panic")       { sendDiscordAlert("PANIC! Manual alert triggered via serial."); return; }
    if (cmd == "show")        { printThresholds(); return; }

    int eqIndex = cmd.indexOf('=');
    if (eqIndex == -1) {
      Serial.println("Commands: freefall=, impact=, settletime=, waittime=, movement=, test, testdiscord, panic, show");
      return;
    }

    String key = cmd.substring(0, eqIndex);
    float val  = cmd.substring(eqIndex + 1).toFloat();
    key.toLowerCase();

    if      (key == "freefall")   { FREEFALL_THRESHOLD = val; }
    else if (key == "impact")     { IMPACT_THRESHOLD = val; }
    else if (key == "settletime") { SETTLE_TIME_MS = (unsigned long)val; }
    else if (key == "waittime")   { WAIT_TIME_MS = (unsigned long)val; }
    else if (key == "movement")   { MOVEMENT_GYRO_THRESHOLD = val; }
    else { Serial.println("Unknown command."); return; }

    Serial.print(key); Serial.print(" set to "); Serial.println(val);
  }
}

// redefine plain buzz for non-cancel uses
void buzz(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    delay(offMs);
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(PANIC_BTN, INPUT_PULLUP);  // press = LOW
  pinMode(BOOT_BTN,  INPUT_PULLUP);  // press = LOW (built-in)

  Serial.begin(115200);
  delay(100);
  Serial.println("MPU6050 Fall Detector (ESP32 + Discord + Panic Button)");
  Serial.println("Commands: freefall=, impact=, settletime=, waittime=, movement=, test, testdiscord, panic, show");

  connectWiFi();
  calibrate();
  printThresholds();
  sendDiscordAlert("Fall detector is online! Monitoring started.");

  lastTime = millis();
}

void loop() {
  handleSerialCommands();

  // ---- Panic button check (any time, always works) ----
  if (digitalRead(PANIC_BTN) == LOW) {
    delay(50); // debounce
    if (digitalRead(PANIC_BTN) == LOW) {
      Serial.println("PANIC BUTTON PRESSED! Sending alert...");
      buzz(3, 200, 100); // short buzz to confirm press
      sendDiscordAlert("PANIC BUTTON PRESSED! User needs help immediately!");
      delay(2000); // prevent double-send
    }
  }

  readRaw();

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float aX = (accelX_raw - accelX_offset) / 16384.0;
  float aY = (accelY_raw - accelY_offset) / 16384.0;
  float aZ = (accelZ_raw - accelZ_offset) / 16384.0;

  float gX = (gyroX_raw - gyroX_offset) / 131.0;
  float gY = (gyroY_raw - gyroY_offset) / 131.0;
  float gZ = (gyroZ_raw - gyroZ_offset) / 131.0;

  angleX = wrap360(angleX + gX * dt);
  angleY = wrap360(angleY + gY * dt);
  angleZ = wrap360(angleZ + gZ * dt);

  float totalAccel = sqrt(aX * aX + aY * aY + aZ * aZ);
  float gyroMag    = sqrt(gX * gX + gY * gY + gZ * gZ);

  // ---- Settling period ----
  if (fallSettling) {
    if (now - fallSettleStart >= SETTLE_TIME_MS) {
      Serial.println("Settle complete - watching for movement or timeout...");
      fallSettling = false;
      fallLocked   = true;
      fallLockStart = now;
    } else {
      Serial.print("Settling... ");
      Serial.println((SETTLE_TIME_MS - (now - fallSettleStart)) / 1000.0, 2);
    }
    delay(50);
    return;
  }

  // ---- Locked: watching for movement or timeout ----
  if (fallLocked) {
    if (gyroMag > MOVEMENT_GYRO_THRESHOLD) {
      Serial.println("Movement detected - person OK! Cancelling alert.");
      fallLocked = false;
      delay(50);
      return;
    }

    if (now - fallLockStart >= WAIT_TIME_MS) {
      Serial.println("FALL CONFIRMED - no movement! Alerting!");

      // Reconnect WiFi if needed before alerting
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int w = 0;
        while (WiFi.status() != WL_CONNECTED && w < 20) {
          delay(500); Serial.print("."); w++;
        }
        Serial.println();
      }

      // Buzz 10 times, allow BOOT cancel only after 2 beeps
      bool cancelled = buzzWithCancel(10, 300, 200, 2);

      if (cancelled) {
        Serial.println("False alarm cancelled by user. No Discord alert sent.");
        sendDiscordAlert("False alarm - user cancelled the fall alert.");
      } else {
        sendDiscordAlert("FALL DETECTED! No movement after impact. Please check immediately!");
      }

      fallLocked = false;
    } else {
      Serial.print("Locked - waiting... ");
      Serial.print((WAIT_TIME_MS - (now - fallLockStart)) / 1000.0, 1);
      Serial.print("s left | gyroMag:"); Serial.println(gyroMag, 1);
    }

    delay(50);
    return;
  }

  // ---- Normal detection ----
  if (!fallSuspected) {
    if (totalAccel < FREEFALL_THRESHOLD) {
      fallSuspected = true;
      Serial.println("Dip detected, watching for impact...");
    }
  } else {
    if (totalAccel > IMPACT_THRESHOLD) {
      Serial.println("Impact detected! Settling...");
      fallSettleStart = now;
      fallSuspected   = false;
      fallSettling    = true;
    }
  }

  Serial.print("Accel(g) total:"); Serial.print(totalAccel, 2);
  Serial.print(" | gyroMag:"); Serial.print(gyroMag, 1);
  Serial.print(" | State: ");
  Serial.println(fallSuspected ? "DIP_DETECTED" : "NORMAL");

  delay(50);
}