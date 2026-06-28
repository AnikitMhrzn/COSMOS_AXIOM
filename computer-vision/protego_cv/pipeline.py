"""
Combined pipeline: helmet detection + badge OCR -> one compliance record per
worker. Also handles drawing overlays for the demo.
"""
from __future__ import annotations
from dataclasses import dataclass, asdict
from typing import List
import time

import cv2

import config
from helmet_detector import HelmetDetector, Person
from badge_ocr import BadgeOCR


@dataclass
class ComplianceRecord:
    worker: str
    helmet: bool
    helmet_conf: float
    badge_conf: float
    box: tuple
    ts: float

    def line(self):
        wid = self.worker if self.worker != "??" else "??"
        status = "helmet: yes" if self.helmet else "MISSING helmet"
        return f"Worker #{wid} - {status}"


class Pipeline:
    def __init__(self):
        print("[Protego] loading models...")
        self.detector = HelmetDetector()
        self.ocr = BadgeOCR()
        print("[Protego] ready.")

    def process(self, frame) -> List[ComplianceRecord]:
        people = self.detector.detect_people(frame)
        self.detector.check_helmets(frame, people)
        self.ocr.read_for_people(frame, people)
        now = time.time()
        records = []
        for p in people:
            records.append(ComplianceRecord(
                worker=p.badge or "??",
                helmet=p.helmet,
                helmet_conf=round(p.helmet_conf, 2),
                badge_conf=round(p.badge_conf, 2),
                box=p.box,
                ts=now,
            ))
        return people, records

    # -- visualization -------------------------------------------------------
    @staticmethod
    def draw(frame, people: List[Person]):
        GREEN = (0, 180, 0)
        RED = (0, 0, 255)
        GREY = (180, 180, 180)
        font = cv2.FONT_HERSHEY_SIMPLEX
        for p in people:
            x1, y1, x2, y2 = p.box
            ok = p.helmet
            box_color = GREEN if ok else RED
            cv2.rectangle(frame, (x1, y1), (x2, y2), box_color, 2)

            # --- label drawn in two parts with independent colours ---
            # Worker ID: GREEN once a badge number is detected, grey otherwise.
            id_detected = bool(p.badge)
            id_text = f"Worker #{p.badge} " if id_detected else "Worker #?? "
            id_color = GREEN if id_detected else GREY
            status_text = "- helmet: yes" if ok else "- MISSING helmet"
            status_color = GREEN if ok else RED

            ty = max(20, y1 - 8)
            cv2.putText(frame, id_text, (x1, ty), font, 0.6, id_color, 2)
            (id_w, _), _ = cv2.getTextSize(id_text, font, 0.6, 2)
            cv2.putText(frame, status_text, (x1 + id_w, ty), font, 0.6, status_color, 2)

            # torso box (where the badge is read)
            tx1, ty1, tx2, ty2 = p.torso_region
            cv2.rectangle(frame, (tx1, ty1), (tx2, ty2), (200, 200, 0), 1)
        return frame
