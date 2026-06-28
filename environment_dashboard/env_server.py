"""
Protego - Environment Monitoring Dashboard server.

Polls the ESP8266 environment unit's JSON API (http://<esp-ip>/api), checks each
reading against safety thresholds, keeps a short history, and serves a live
dashboard. Pure standard library - no pip install needed.

    python env_server.py --esp http://192.168.1.50 --port 8050
    # then open http://localhost:8050

The --esp value is the ESP's address shown in its serial monitor
("IP Address: http://192.168.x.x"). /api is appended automatically.

Why a proxy server instead of the browser hitting the ESP directly?
The ESP's API sends no CORS headers, so a browser on a different origin can't
read it. This server fetches server-side (no CORS limit), adds threshold logic,
history and alerting, and serves one clean page.
"""
import argparse
import json
import threading
import time
import urllib.request
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

# ---- safety thresholds (edit to your site's limits) ------------------------
THRESHOLDS = {
    # key: (warn, danger, unit, direction)  direction "high" = bad when above
    "co":          {"warn": 50,  "danger": 100, "unit": "ppm", "label": "CO Gas"},
    "temperature": {"warn": 35,  "danger": 40,  "unit": "°C", "label": "Temperature"},
    "humidity":    {"warn": 70,  "danger": 85,  "unit": "%",  "label": "Humidity"},
}

POLL_EVERY = 3.0          # seconds between ESP polls
HISTORY_LEN = 120         # ~6 min of history at 3s

_LOCK = threading.Lock()
_STATE = {
    "online": False, "last_ok": 0.0, "error": "",
    "co": None, "temperature": None, "humidity": None,
    "wifi_connected": None, "rssi": None, "ip": "", "uptime": None,
}
_HISTORY = {"co": deque(maxlen=HISTORY_LEN),
            "temperature": deque(maxlen=HISTORY_LEN),
            "humidity": deque(maxlen=HISTORY_LEN),
            "t": deque(maxlen=HISTORY_LEN)}
_RUNNING = True
_ESP_API = ""

DASH_HTML = (Path(__file__).parent / "env_dashboard.html").read_text(encoding="utf-8")


def level_for(key, value):
    """Return 'ok' | 'warn' | 'danger' for a reading."""
    if value is None:
        return "unknown"
    t = THRESHOLDS[key]
    if value >= t["danger"]:
        return "danger"
    if value >= t["warn"]:
        return "warn"
    return "ok"


def poll_loop():
    global _STATE
    while _RUNNING:
        try:
            with urllib.request.urlopen(_ESP_API, timeout=4) as r:
                data = json.loads(r.read().decode())
            co = data.get("co", {}).get("ppm")
            temp = data.get("temperature", {}).get("value")
            hum = data.get("humidity", {}).get("value")
            now = time.time()
            with _LOCK:
                _STATE.update({
                    "online": True, "last_ok": now, "error": "",
                    "co": co, "temperature": temp, "humidity": hum,
                    "wifi_connected": data.get("wifi_connected"),
                    "rssi": data.get("rssi"), "ip": data.get("ip", ""),
                    "uptime": data.get("uptime"),
                    "raw_adc": data.get("co", {}).get("raw_adc"),
                })
                _HISTORY["co"].append(co)
                _HISTORY["temperature"].append(temp)
                _HISTORY["humidity"].append(hum)
                _HISTORY["t"].append(now)
        except Exception as e:
            with _LOCK:
                _STATE["online"] = False
                _STATE["error"] = str(e)
        time.sleep(POLL_EVERY)


def build_state():
    with _LOCK:
        s = dict(_STATE)
        hist = {k: list(v) for k, v in _HISTORY.items()}
    readings = {}
    alerts = []
    for key in ("co", "temperature", "humidity"):
        val = s.get(key)
        lvl = level_for(key, val)
        t = THRESHOLDS[key]
        readings[key] = {"value": val, "level": lvl, "unit": t["unit"],
                         "label": t["label"], "warn": t["warn"], "danger": t["danger"]}
        if lvl in ("warn", "danger"):
            alerts.append({"key": key, "label": t["label"], "level": lvl,
                           "value": val, "unit": t["unit"],
                           "limit": t["danger"] if lvl == "danger" else t["warn"]})
    # stale if no successful read in 3 poll cycles
    s["stale"] = (time.time() - s["last_ok"]) > POLL_EVERY * 3 if s["last_ok"] else True
    return {"now": time.time(), "device": s, "readings": readings,
            "alerts": alerts, "history": hist, "poll_every": POLL_EVERY}


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _send(self, code, body, ctype):
        data = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        if data:
            self.wfile.write(data)

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/index"):
            self._send(200, DASH_HTML, "text/html; charset=utf-8")
        elif self.path.startswith("/api/state"):
            self._send(200, json.dumps(build_state()), "application/json")
        else:
            self._send(404, json.dumps({"error": "not found"}), "application/json")


def main():
    global _ESP_API, _RUNNING
    ap = argparse.ArgumentParser()
    ap.add_argument("--esp", required=True,
                    help="ESP address, e.g. http://192.168.1.50 (or with /api)")
    ap.add_argument("--port", type=int, default=8050)
    args = ap.parse_args()

    esp = args.esp.rstrip("/")
    _ESP_API = esp if esp.endswith("/api") else esp + "/api"
    print(f"[env] polling {_ESP_API} every {POLL_EVERY}s")

    threading.Thread(target=poll_loop, daemon=True).start()
    srv = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"[env] dashboard at http://localhost:{args.port}  (Ctrl-C to stop)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n[env] stopped")
    finally:
        _RUNNING = False


if __name__ == "__main__":
    main()
