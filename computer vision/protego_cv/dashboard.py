"""
Sends compliance records to the Protego dashboard. If no DASHBOARD_URL is set
in config.py, it just prints the payload (handy for the demo).
"""
from __future__ import annotations
from dataclasses import asdict
from typing import List
import json
import time

import config


class DashboardSender:
    def __init__(self):
        self._last_push = 0.0
        self._session = None
        if config.DASHBOARD_URL:
            import requests
            self._session = requests.Session()

    def maybe_push(self, records, force=False):
        now = time.time()
        if not force and (now - self._last_push) < config.DASHBOARD_PUSH_EVERY:
            return
        self._last_push = now
        payload = {
            "timestamp": now,
            "workers": [
                {"worker": r.worker, "helmet": r.helmet,
                 "helmet_conf": r.helmet_conf, "badge_conf": r.badge_conf}
                for r in records
            ],
        }
        if self._session:
            try:
                self._session.post(config.DASHBOARD_URL, json=payload, timeout=3)
            except Exception as e:  # don't crash the camera loop on a network blip
                print(f"[dashboard] push failed: {e}")
        else:
            print("[dashboard] " + json.dumps(payload))
