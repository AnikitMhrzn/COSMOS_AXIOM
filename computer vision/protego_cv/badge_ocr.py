"""
Task 2 - Badge number recognition.

Crops the torso region of each detected person and reads the printed badge
number with EasyOCR, restricted to digits for accuracy.
"""
from __future__ import annotations
from typing import List, Optional, Tuple

import cv2
import numpy as np
import easyocr

import config
from helmet_detector import Person


class BadgeOCR:
    def __init__(self):
        # gpu=False keeps it portable for a laptop demo.
        self.reader = easyocr.Reader(config.OCR_LANGS, gpu=False, verbose=False)

    def _preprocess(self, crop):
        """Upscale + grayscale + adaptive threshold to help OCR on wrinkled paper."""
        if crop.size == 0:
            return crop
        scale = max(1, int(400 / max(1, crop.shape[0])))
        if scale > 1:
            crop = cv2.resize(crop, None, fx=scale, fy=scale,
                              interpolation=cv2.INTER_CUBIC)
        gray = cv2.cvtColor(crop, cv2.COLOR_BGR2GRAY)
        gray = cv2.bilateralFilter(gray, 7, 50, 50)
        return gray

    def read_number(self, img) -> Tuple[Optional[str], float]:
        """Return (digits, confidence) for the most confident digit run in img."""
        proc = self._preprocess(img)
        if proc.size == 0:
            return None, 0.0
        results = self.reader.readtext(proc, allowlist=config.OCR_ALLOWLIST,
                                       detail=1, paragraph=False)
        best_text, best_conf = None, 0.0
        for _box, text, conf in results:
            digits = "".join(ch for ch in text if ch.isdigit())
            if digits and conf >= config.OCR_MIN_CONF and conf > best_conf:
                best_text, best_conf = digits, float(conf)
        return best_text, best_conf

    def read_for_people(self, frame, people: List[Person]):
        for p in people:
            tx1, ty1, tx2, ty2 = p.torso_region
            crop = frame[max(0, ty1):ty2, max(0, tx1):tx2]
            text, conf = self.read_number(crop)
            p.badge, p.badge_conf = text, conf
