# Arduino libraries for environment.ino

`environment.ino` targets an **ESP8266** board and uses a DS18B20 temperature
sensor and an MQ7 gas sensor (humidity is simulated in the sketch). Here is
everything it needs to compile.

## 1. ESP8266 board support (provides ESP8266WiFi.h, ESP8266WebServer.h)

These two headers are **not** separate libraries — they come with the ESP8266
Arduino core. Install it once:

1. Arduino IDE → File → Preferences → "Additional Boards Manager URLs", add:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. Tools → Board → Boards Manager → search **esp8266** → install
   "esp8266 by ESP8266 Community".
3. Select your board under Tools → Board (e.g. "NodeMCU 1.0 (ESP-12E Module)").

## 2. External libraries (already downloaded here)

These two are bundled in this folder so you don't have to download them:

| Library | Version | Provides |
|---------|---------|----------|
| OneWire | 2.3.8 | `OneWire.h` — 1-Wire bus used by the DS18B20 |
| DallasTemperature | 4.0.6 | `DallasTemperature.h` — DS18B20 readings |

### Install them

**Option A — copy into your Arduino libraries folder**
Copy the `OneWire` and `DallasTemperature` folders into your sketchbook's
`libraries` directory:
- Windows: `Documents\Arduino\libraries\`
- macOS: `~/Documents/Arduino/libraries/`
- Linux: `~/Arduino/libraries/`

Restart the Arduino IDE afterwards.

**Option B — Arduino Library Manager (downloads fresh copies)**
Sketch → Include Library → Manage Libraries, then install:
- **OneWire** by Paul Stoffregen
- **DallasTemperature** by Miles Burton

## 3. Compile checklist

- Board: an ESP8266 (NodeMCU / Wemos D1, etc.)
- Libraries: OneWire + DallasTemperature installed (step 2)
- Core headers: ESP8266WiFi / ESP8266WebServer (come with step 1)

> Note: the project description mentions a DHT22 and an ESP32, but the actual
> `environment.ino` uses an ESP8266 with DS18B20 + MQ7 and simulates humidity,
> so no DHT library is required.
