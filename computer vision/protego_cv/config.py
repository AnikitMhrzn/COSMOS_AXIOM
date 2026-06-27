"""
Central configuration for the Protego PPE & Identity Compliance module.
Edit these values instead of hard-coding things in the pipeline.
"""
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent
MODELS_DIR = PROJECT_ROOT / "models"
SAMPLES_DIR = PROJECT_ROOT.parent          # the "computer vision" folder with sample subfolders
OUTPUT_DIR = PROJECT_ROOT / "runs"
MODELS_DIR.mkdir(exist_ok=True)
OUTPUT_DIR.mkdir(exist_ok=True)

# ---------------------------------------------------------------------------
# Models
# ---------------------------------------------------------------------------
# Person detector: stock YOLOv8 trained on COCO (class 0 = person).
# Auto-downloads on first run.
PERSON_MODEL = "yolov8n.pt"

# Helmet / hard-hat detector.
# By default we use a lightweight heuristic on the head region (no extra weights
# needed -> fast for a demo). If you drop a trained PPE YOLO model here and set
# USE_PPE_MODEL = True, the pipeline will use it instead.
USE_PPE_MODEL = False
PPE_MODEL = str(MODELS_DIR / "hardhat.pt")
# Class names your PPE model uses for a worn helmet (lower-cased substring match)
PPE_HELMET_CLASSES = {"helmet", "hardhat", "hard-hat", "hard hat"}
PPE_NOHELMET_CLASSES = {"no-helmet", "no helmet", "head", "no-hardhat", "nohardhat"}

# ---------------------------------------------------------------------------
# Detection thresholds
# ---------------------------------------------------------------------------
PERSON_CONF = 0.40        # min confidence to accept a person
HELMET_IOU = 0.05         # min overlap of helmet box with person's head region
HEAD_REGION_FRAC = 0.32   # top fraction of a person box treated as the "head" zone

# Heuristic helmet detector (used when USE_PPE_MODEL is False).
# Hard hats here are bright yellow; tune the HSV range for your real helmets.
HELMET_HSV_LOWER = (15, 80, 80)    # H,S,V  (yellow-orange)
HELMET_HSV_UPPER = (45, 255, 255)
HELMET_MIN_FRAC = 0.06    # min fraction of head-zone pixels matching helmet color

# ---------------------------------------------------------------------------
# OCR (badge number)
# ---------------------------------------------------------------------------
OCR_LANGS = ["en"]
OCR_ALLOWLIST = "0123456789"   # digits only
OCR_MIN_CONF = 0.30
# Torso crop relative to a person box: take this vertical slice for the badge.
TORSO_TOP_FRAC = 0.30
TORSO_BOTTOM_FRAC = 0.75

# ---------------------------------------------------------------------------
# Dashboard
# ---------------------------------------------------------------------------
DASHBOARD_URL = "http://localhost:8000/api/compliance"   # local dashboard; set "" to just print
DASHBOARD_PUSH_EVERY = 3.0  # seconds between pushes
