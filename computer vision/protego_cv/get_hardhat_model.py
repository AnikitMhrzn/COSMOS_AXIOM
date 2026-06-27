"""
Helper to set up a trained hard-hat / PPE detection model so helmet detection
becomes colour-independent and far more reliable than the demo heuristic.

There is no single "official" hard-hat weight file shipped with this project,
so this script supports the common ways to obtain one. Pick whichever fits.

OPTION A — you already have a trained .pt model
    Just copy it to models/hardhat.pt and set USE_PPE_MODEL = True in config.py.
        python get_hardhat_model.py --src /path/to/your_model.pt

OPTION B — download from a direct URL (e.g. your own release, a Roboflow export)
        python get_hardhat_model.py --url https://example.com/hardhat.pt

OPTION C — Roboflow Universe (many free hard-hat datasets/models)
    1. pip install roboflow
    2. Grab the snippet from the model's "Deploy" tab, or train a YOLOv8 model
       from a hard-hat dataset (see train_hardhat.py).

After any option, verify with:
        python get_hardhat_model.py --check

The class names your model uses must be reflected in config.PPE_HELMET_CLASSES
/ PPE_NOHELMET_CLASSES (lower-cased). The checker prints them for you.
"""
import argparse
import shutil
import sys
from pathlib import Path

import config


def _check():
    p = Path(config.PPE_MODEL)
    if not p.exists():
        print(f"[x] no model at {p}")
        return
    try:
        from ultralytics import YOLO
        m = YOLO(str(p))
        print(f"[ok] loaded {p}")
        print("     classes:", m.names)
        print("     -> make sure config.PPE_HELMET_CLASSES / PPE_NOHELMET_CLASSES")
        print("        match the relevant lower-cased names above, then set")
        print("        USE_PPE_MODEL = True in config.py")
    except Exception as e:
        print(f"[x] could not load model: {e}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", help="copy a local .pt into models/hardhat.pt")
    ap.add_argument("--url", help="download a .pt into models/hardhat.pt")
    ap.add_argument("--check", action="store_true", help="load + inspect the model")
    args = ap.parse_args()

    dest = Path(config.PPE_MODEL)
    dest.parent.mkdir(exist_ok=True)

    if args.src:
        shutil.copy(args.src, dest)
        print(f"[ok] copied to {dest}")
    elif args.url:
        import urllib.request
        print(f"downloading {args.url} ...")
        urllib.request.urlretrieve(args.url, dest)
        print(f"[ok] saved to {dest}")
    elif args.check:
        _check()
        return
    else:
        print(__doc__)
        return

    _check()


if __name__ == "__main__":
    main()
