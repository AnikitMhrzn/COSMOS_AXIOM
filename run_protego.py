"""
Protego - unified launcher.

ONE command starts everything and gives you a single dashboard:

    python run_protego.py --esp http://192.168.1.50

  * Computer-vision unit  (camera + helmet/badge detection)   -> internal port 8000
  * Environment unit      (ESP8266 temp/humidity/CO monitor)   -> internal port 8050
  * Unified dashboard      with Vision / Environment tabs        -> http://localhost:8080

Options:
    --esp URL     ESP address (e.g. http://192.168.1.50). Omit to skip the env unit.
    --cam N       camera index (default 0)
    --no-cam      skip the computer-vision unit
    --port N      unified dashboard port (default 8080)

Press Ctrl-C once to stop everything.

This launches your existing servers as-is (run_dashboard_cam.py and
env_server.py); nothing in them is modified.
"""
import argparse
import subprocess
import sys
import time
import signal
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parent

def _find_dir(filename, fallback):
    """Locate the folder containing `filename` under ROOT (handles renames/spaces)."""
    here = ROOT / fallback
    if (here / filename).exists():
        return here
    matches = list(ROOT.glob(f"**/{filename}"))
    if matches:
        return matches[0].parent
    return here

CV_DIR = _find_dir("run_dashboard_cam.py", Path("computer-vision") / "protego_cv")
ENV_DIR = _find_dir("env_server.py", Path("environment_dashboard"))

CV_PORT = 8000
ENV_PORT = 8050

procs = []


def launch(cmd, cwd, name):
    print(f"[launch] {name}: {' '.join(str(c) for c in cmd)}")
    p = subprocess.Popen(cmd, cwd=str(cwd))
    procs.append((name, p))
    return p


UNIFIED_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>Protego - Unified Dashboard</title>
<style>
  :root{{--bg:#0d1117;--panel:#161b22;--line:#272e3a;--text:#e6edf3;--muted:#8b949e;--accent:#388bfd;}}
  *{{box-sizing:border-box}}
  html,body{{margin:0;height:100%;font-family:-apple-system,Segoe UI,Roboto,Arial,sans-serif;background:var(--bg);color:var(--text)}}
  header{{display:flex;align-items:center;gap:22px;padding:14px 22px;border-bottom:1px solid var(--line);background:var(--panel)}}
  h1{{font-size:18px;margin:0}} h1 span{{color:var(--accent)}}
  .tabs{{display:flex;gap:8px}}
  .tab{{padding:8px 18px;border-radius:8px;border:1px solid var(--line);background:transparent;color:var(--muted);
        font-size:14px;font-weight:600;cursor:pointer}}
  .tab.active{{background:var(--accent);color:#fff;border-color:var(--accent)}}
  .wrap{{position:absolute;top:61px;left:0;right:0;bottom:0}}
  .wrap>div{{height:100%}}
  iframe{{width:100%;height:100%;border:0;background:var(--bg);display:block}}
  .hidden{{display:none}}
  .disabled{{display:flex;align-items:center;justify-content:center;height:100%;color:var(--muted);font-size:15px}}
</style>
</head>
<body>
  <header>
    <h1><span>Protego</span> - Safety Compliance Console</h1>
    <div class="tabs">
      <button class="tab active" data-t="vision">Computer Vision</button>
      <button class="tab" data-t="env">Environment</button>
    </div>
  </header>
  <div class="wrap">
    <div id="vision-pane">{vision_pane}</div>
    <div id="env-pane" class="hidden">{env_pane}</div>
  </div>
<script>
  const tabs=document.querySelectorAll('.tab');
  function activate(sel){{
    tabs.forEach(x=>x.classList.toggle('active', x.dataset.t===sel));
    document.getElementById('vision-pane').classList.toggle('hidden', sel!=='vision');
    document.getElementById('env-pane').classList.toggle('hidden', sel!=='env');
  }}
  tabs.forEach(t=>t.onclick=()=>activate(t.dataset.t));
  const want=(location.hash||'').replace('#','');
  if(want==='env'||want==='vision') activate(want);
</script>
</body>
</html>"""


def make_page(cv_on, env_on, host):
    def frame(port):
        return f'<iframe src="http://{host}:{port}/"></iframe>'
    vision = frame(CV_PORT) if cv_on else '<div class="disabled">Computer-vision unit not started (--no-cam).</div>'
    env = frame(ENV_PORT) if env_on else '<div class="disabled">Environment unit not started (pass --esp http://your-esp-ip).</div>'
    return UNIFIED_HTML.format(vision_pane=vision, env_pane=env)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--esp", default="", help="ESP address, e.g. http://192.168.1.50")
    ap.add_argument("--cam", type=int, default=0)
    ap.add_argument("--no-cam", action="store_true")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--infer-every", type=float, default=0.2)
    args = ap.parse_args()

    cv_on = not args.no_cam
    env_on = bool(args.esp)

    if cv_on:
        launch([sys.executable, "run_dashboard_cam.py", "--cam", str(args.cam),
                "--port", str(CV_PORT), "--infer-every", str(args.infer_every)],
               CV_DIR, "computer-vision")
    if env_on:
        launch([sys.executable, "env_server.py", "--esp", args.esp, "--port", str(ENV_PORT)],
               ENV_DIR, "environment")
    if not cv_on and not env_on:
        print("Nothing to run: don't use --no-cam without --esp.")
        return

    page = make_page(cv_on, env_on, "localhost").encode()
    landing_file = ROOT / "landing.html"
    landing = landing_file.read_text(encoding="utf-8").encode() if landing_file.exists() else page

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *a): pass
        def _html(self, body):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        def do_GET(self):
            path = self.path.split("?")[0]
            if path in ("/", "/index.html"):
                self._html(landing)                       # cosmic landing page
            elif path.startswith("/dashboard"):
                self._html(page)                          # unified tabbed dashboards
            elif path.startswith("/vendor/"):
                rel = path[len("/vendor/"):]
                fp = (ROOT / "web" / "vendor" / rel).resolve()
                base = (ROOT / "web" / "vendor").resolve()
                if base in fp.parents and fp.is_file():
                    body = fp.read_bytes()
                    ctype = "application/javascript" if rel.endswith(".js") else "application/octet-stream"
                    self.send_response(200)
                    self.send_header("Content-Type", ctype)
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers(); self.wfile.write(body)
                else:
                    self.send_response(404); self.end_headers()
            else:
                self.send_response(404); self.end_headers()

    srv = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"\n[protego] LANDING PAGE   -> http://localhost:{args.port}")
    print(f"[protego] DASHBOARDS     -> http://localhost:{args.port}/dashboard\n")
    print("          (Ctrl-C to stop everything)\n")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\n[protego] shutting down...")
    finally:
        for name, p in procs:
            p.terminate()
        for name, p in procs:
            try:
                p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                p.kill()
        print("[protego] stopped.")


if __name__ == "__main__":
    main()
