"""
Task 1 - Helmet detection.

Detects people with YOLOv8 (COCO) and decides, per person, whether a helmet
is present on the head. Two backends:

  * PPE model backend  (config.USE_PPE_MODEL = True)
        Runs a dedicated hard-hat YOLO model and matches helmet boxes to the
        head region of each detected person.

  * Heuristic backend  (default, no extra weights)
        Looks for helmet-coloured pixels inside the top region of each person
        box. Fast, dependency-free, good enough for a demo. Tune the HSV range
        in config.py to match your real helmets.
"""
from __future__ import annotations
from dataclasses import dataclass, field
from typing import List, Optional

import cv2
import numpy as np
from ultralytics import YOLO

import config


@dataclass
class Person:
    box: tuple          # (x1, y1, x2, y2) in pixels
    conf: float
    helmet: bool = False
    helmet_conf: float = 0.0
    badge: Optional[str] = None
    badge_conf: float = 0.0

    @property
    def head_region(self):
        x1, y1, x2, y2 = self.box
        return (x1, y1, x2, int(y1 + (y2 - y1) * config.HEAD_REGION_FRAC))

    @property
    def torso_region(self):
        x1, y1, x2, y2 = self.box
        h = y2 - y1
        return (x1, int(y1 + h * config.TORSO_TOP_FRAC),
                x2, int(y1 + h * config.TORSO_BOTTOM_FRAC))

    @property
    def label(self):
        wid = f"Worker #{self.badge}" if self.badge else "Worker #??"
        return f"{wid} - helmet: {'yes' if self.helmet else 'MISSING'}"


def _iou(a, b) -> float:
    ax1, ay1, ax2, ay2 = a
    bx1, by1, bx2, by2 = b
    ix1, iy1 = max(ax1, bx1), max(ay1, by1)
    ix2, iy2 = min(ax2, bx2), min(ay2, by2)
    iw, ih = max(0, ix2 - ix1), max(0, iy2 - iy1)
    inter = iw * ih
    if inter == 0:
        return 0.0
    area_a = (ax2 - ax1) * (ay2 - ay1)
    return inter / float(area_a) if area_a else 0.0


class HelmetDetector:
    def __init__(self):
        self.person_model = YOLO(config.PERSON_MODEL)
        self.ppe_model = None
        if config.USE_PPE_MODEL:
            self.ppe_model = YOLO(config.PPE_MODEL)

    # -- person detection ----------------------------------------------------
    def detect_people(self, frame) -> List[Person]:
        res = self.person_model(frame, classes=[0], conf=config.PERSON_CONF,
                                verbose=False)[0]
        people = []
        for b in res.boxes:
            x1, y1, x2, y2 = map(int, b.xyxy[0].tolist())
            people.append(Person(box=(x1, y1, x2, y2), conf=float(b.conf[0])))
        return people

    # -- helmet decision -----------------------------------------------------
    def check_helmets(self, frame, people: List[Person]):
        if self.ppe_model is not None:
            self._check_with_ppe_model(frame, people)
        else:
            for p in people:
                self._check_heuristic(frame, p)

    def _check_with_ppe_model(self, frame, people: List[Person]):
        res = self.ppe_model(frame, conf=0.30, verbose=False)[0]
        names = res.names
        helmet_boxes, head_boxes = [], []
        for b in res.boxes:
            cls = names[int(b.cls[0])].lower()
            box = tuple(map(int, b.xyxy[0].tolist()))
            if cls in config.PPE_HELMET_CLASSES:
                helmet_boxes.append((box, float(b.conf[0])))
            elif cls in config.PPE_NOHELMET_CLASSES:
                head_boxes.append((box, float(b.conf[0])))
        for p in people:
            best = 0.0
            for box, conf in helmet_boxes:
                if _iou(box, p.head_region) > config.HELMET_IOU:
                    best = max(best, conf)
            p.helmet = best > 0
            p.helmet_conf = best

    def _check_heuristic(self, frame, p: Person):
        hx1, hy1, hx2, hy2 = p.head_region
        crop = frame[max(0, hy1):hy2, max(0, hx1):hx2]
        if crop.size == 0:
            p.helmet, p.helmet_conf = False, 0.0
            return
        hsv = cv2.cvtColor(crop, cv2.COLOR_BGR2HSV)
        mask = cv2.inRange(hsv, np.array(config.HELMET_HSV_LOWER),
                           np.array(config.HELMET_HSV_UPPER))
        frac = float(mask.mean()) / 255.0
        p.helmet = frac >= config.HELMET_MIN_FRAC
        p.helmet_conf = round(min(1.0, frac / config.HELMET_MIN_FRAC), 2)
