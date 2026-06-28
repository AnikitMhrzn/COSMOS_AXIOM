"""
Lightweight live dashboard for the Protego compliance feed. Pure standard
library — no Flask needed.

Run it in its own terminal:

    python dashboard_server.py            # serves on http://localhost:8000

Then set DASHBOARD_URL = "http://localhost:8000/api/compliance" in config.py and
run run_webcam.py (or run_images.py). The pipeline POSTs records here; open
http://localhost:8000 in a browser to watch compliance update in real time.

Endpoints:
    GET  /                 -> the dashboard page
    POST /api/compliance   -> receive a records payload from the pipeline
    GET  /api/state        -> current worker state (polled by the page)
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import time

STATE = {}          # worker -> {helmet, helmet_conf, badge_conf, last_seen}
LAST_PAYLOAD_TS = 0.0
DASHBOARD_HTML = (Path(__file__).parent / "dashboard.html").read_text(encoding="utf-8")


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):  # quieten the console
        pass

    def _send(self, code, body, ctype="application/json"):
        data = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/index"):
            self._send(200, DASHBOARD_HTML, "text/html; charset=utf-8")
        elif self.path == "/api/state":
            now = time.time()
            workers = [
                {"worker": w, **v, "stale": (now - v["last_seen"]) > 10}
                for w, v in sorted(STATE.items())
            ]
            self._send(200, json.dumps({"now": now, "last_payload_ts": LAST_PAYLOAD_TS,
                                        "workers": workers}))
        else:
            self._send(404, json.dumps({"error": "not found"}))

    def do_POST(self):
        global LAST_PAYLOAD_TS
        if self.path != "/api/compliance":
            self._send(404, json.dumps({"error": "not found"}))
            return
        length = int(self.headers.get("Content-Length", 0))
        try:
            payload = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError:
            self._send(400, json.dumps({"error": "bad json"}))
            return
        ts = payload.get("timestamp", time.time())
        LAST_PAYLOAD_TS = ts
        for w in payload.get("workers", []):
            wid = str(w.get("worker", "??"))
            STATE[wid] = {
                "helmet": bool(w.get("helmet", False)),
                "helmet_conf": w.get("helmet_conf", 0),
                "badge_conf": w.get("badge_conf", 0),
                "last_seen": ts,
            }
        self._send(200, json.dumps({"ok": True, "count": len(STATE)}))


def main(port=8000):
    srv = ThreadingHTTPServer(("0.0.0.0", port), Handler)
    print(f"[dashboard] http://localhost:{port}  (Ctrl-C to stop)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n[dashboard] stopped")


if __name__ == "__main__":
    import sys
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 8000)
