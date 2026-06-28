"""
All-in-one demo: one command runs the webcam, the Protego pipeline, and the
dashboard with a live annotated video stream.

    python run_dashboard_cam.py            # camera 0, http://localhost:8000
    python run_dashboard_cam.py --cam 1 --port 8000 --infer-every 0.25

Open http://localhost:8000 - live camera with green/red boxes on top, compliance
cards below. Press Ctrl-C to stop.
"""
import argparse
import threading
import time
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import cv2

from pipeline import Pipeline

_LOCK = threading.Lock()
_RAW = None
_PEOPLE = []
_LATEST_JPEG = None
_STATE = {}
_LAST_INFER_TS = 0.0
_RUNNING = True

DASHBOARD_HTML = (Path(__file__).parent / "dashboard.html").read_text(encoding="utf-8")


def capture_loop(cam_index):
    """Fast loop: grab frames, draw cached detections, encode for streaming."""
    global _RAW, _LATEST_JPEG, _RUNNING
    cap = cv2.VideoCapture(cam_index)
    try:
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    except Exception:
        pass
    if not cap.isOpened():
        print(f"[x] could not open camera {cam_index}")
        _RUNNING = False
        return
    print("[camera] capturing...")
    while _RUNNING:
        ok, frame = cap.read()
        if not ok:
            time.sleep(0.03)
            continue
        with _LOCK:
            _RAW = frame.copy()
            people = list(_PEOPLE)
        Pipeline.draw(frame, people)
        ok, buf = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 70])
        if ok:
            with _LOCK:
                _LATEST_JPEG = buf.tobytes()
    cap.release()


def infer_loop(infer_every):
    """Slow loop: run YOLO + OCR on the latest raw frame, update detections."""
    global _PEOPLE, _LAST_INFER_TS
    pipe = Pipeline()
    while _RUNNING:
        with _LOCK:
            frame = None if _RAW is None else _RAW.copy()
        if frame is None:
            time.sleep(0.05)
            continue
        people, records = pipe.process(frame)
        now = time.time()
        with _LOCK:
            _PEOPLE = people
            _LAST_INFER_TS = now
            for r in records:
                _STATE[str(r.worker)] = {
                    "helmet": r.helmet, "helmet_conf": r.helmet_conf,
                    "badge_conf": r.badge_conf, "last_seen": now,
                }
        time.sleep(max(0.0, infer_every))


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
            self._send(200, DASHBOARD_HTML, "text/html; charset=utf-8")
        elif self.path == "/api/state":
            now = time.time()
            with _LOCK:
                workers = [
                    {"worker": w, **v, "stale": (now - v["last_seen"]) > 10}
                    for w, v in sorted(_STATE.items())
                ]
                last = _LAST_INFER_TS
            self._send(200, json.dumps({"now": now, "last_payload_ts": last,
                                        "workers": workers, "has_stream": True}),
                       "application/json")
        elif self.path.startswith("/snapshot"):
            with _LOCK:
                jpg = _LATEST_JPEG
            if jpg is None:
                self._send(503, b"", "image/jpeg")
            else:
                self._send(200, jpg, "image/jpeg")
        elif self.path.startswith("/stream"):
            self._stream_mjpeg()
        else:
            self._send(404, json.dumps({"error": "not found"}), "application/json")

    def _stream_mjpeg(self):
        try:
            self.send_response(200)
            self.send_header("Content-Type",
                             "multipart/x-mixed-replace; boundary=frame")
            self.send_header("Cache-Control", "no-cache, private")
            self.send_header("Pragma", "no-cache")
            self.end_headers()
        except (BrokenPipeError, ConnectionResetError):
            return
        try:
            while _RUNNING:
                with _LOCK:
                    jpg = _LATEST_JPEG
                if jpg is None:
                    time.sleep(0.05)
                    continue
                self.wfile.write(b"--frame\r\n"
                                 b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(jpg)}\r\n\r\n".encode())
                self.wfile.write(jpg)
                self.wfile.write(b"\r\n")
                self.wfile.flush()
                time.sleep(0.033)
        except (BrokenPipeError, ConnectionResetError):
            pass


def main():
    global _RUNNING
    ap = argparse.ArgumentParser()
    ap.add_argument("--cam", type=int, default=0)
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--infer-every", type=float, default=0.2,
                    help="seconds to wait between inference passes")
    args = ap.parse_args()

    threading.Thread(target=capture_loop, args=(args.cam,), daemon=True).start()
    threading.Thread(target=infer_loop, args=(args.infer_every,), daemon=True).start()

    srv = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"[dashboard] open http://localhost:{args.port}  (Ctrl-C to stop)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n[stopping]")
    finally:
        _RUNNING = False


if __name__ == "__main__":
    main()
