# Protego — Environment Monitoring Dashboard

A live dashboard for the ESP8266 environment unit (`environment.ino`). It reads
the device's JSON API, checks temperature, humidity and CO gas against safety
thresholds, shows live cards with history sparklines, and raises alerts when a
reading crosses a limit.

## How it works

```
ESP8266 (environment.ino)  --/api JSON-->  env_server.py  --HTML/JSON-->  browser
   DS18B20 + MQ7 + humidity                 polls + thresholds            cards + alerts
```

`env_server.py` polls the ESP every 3 seconds (server-side, so there are no
browser CORS problems), evaluates thresholds, keeps ~6 minutes of history, and
serves `env_dashboard.html` plus a `/api/state` JSON endpoint the page polls.

> Your sketch is unchanged. This dashboard only *reads* the API your ESP already
> exposes at `http://<esp-ip>/api`.

## Run

1. Note the ESP's IP from its serial monitor (`IP Address: http://192.168.x.x`).
2. Start the dashboard (no pip install needed — pure standard library):

   ```bash
   cd environment_dashboard
   python env_server.py --esp http://192.168.1.50 --port 8050
   ```
3. Open <http://localhost:8050>.

`--esp` accepts the address with or without `/api`. Your laptop must be on the
same Wi-Fi network as the ESP.

## Safety thresholds

Defaults (edit `THRESHOLDS` at the top of `env_server.py` for your site):

| Reading | Warning | Danger |
|---------|---------|--------|
| CO gas | ≥ 50 ppm | ≥ 100 ppm |
| Temperature | ≥ 35 °C | ≥ 40 °C |
| Humidity | ≥ 70 % | ≥ 85 % |

A card turns amber at the warning level and red at the danger level, and a
matching alert banner appears at the top. If the ESP can't be reached, the
dashboard shows an "ESP unreachable" alert.

## Files

| File | Purpose |
|------|---------|
| `env_server.py` | Polls the ESP, applies thresholds, serves the dashboard. |
| `env_dashboard.html` | The dashboard page: device status, sensor cards, alerts, sparklines. |

## Notes

- The sketch targets an **ESP8266** (not ESP32) and simulates humidity in
  firmware; temperature (DS18B20) and CO (MQ7) are real sensors.
- Arduino libraries for the sketch are in `../arduino_libraries/` with an
  `INSTALL.md` guide.
