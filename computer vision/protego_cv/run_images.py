"""
Offline demo / test: run the pipeline on still images instead of a webcam.
Useful for validating on the sample photos.

    python run_images.py                       # runs on ../person-with-number
    python run_images.py path/to/folder        # any folder of images
    python run_images.py path/to/image.jpeg    # a single image

Annotated copies are written to ./runs/ and a JSON summary is printed.
"""
import sys
import json
from pathlib import Path

import cv2

import config
from pipeline import Pipeline
from dashboard import DashboardSender

EXTS = {".jpg", ".jpeg", ".png", ".bmp"}


def gather(target: Path):
    if target.is_file():
        return [target]
    return sorted(p for p in target.iterdir() if p.suffix.lower() in EXTS)


def main():
    arg = sys.argv[1] if len(sys.argv) > 1 else str(config.SAMPLES_DIR / "person-with-number")
    target = Path(arg)
    images = gather(target)
    if not images:
        raise SystemExit(f"No images found at {target}")

    pipe = Pipeline()
    dash = DashboardSender()
    summary = []

    for img_path in images:
        frame = cv2.imread(str(img_path))
        if frame is None:
            print(f"skip (unreadable): {img_path.name}")
            continue
        people, records = pipe.process(frame)
        pipe.draw(frame, people)
        out = config.OUTPUT_DIR / f"annotated_{img_path.stem}.jpg"
        cv2.imwrite(str(out), frame)
        dash.maybe_push(records, force=True)

        lines = [r.line() for r in records] or ["(no people detected)"]
        print(f"\n=== {img_path.name} ===")
        for ln in lines:
            print("  " + ln)
        summary.append({
            "image": img_path.name,
            "workers": [
                {"worker": r.worker, "helmet": r.helmet,
                 "helmet_conf": r.helmet_conf, "badge_conf": r.badge_conf}
                for r in records
            ],
        })

    (config.OUTPUT_DIR / "summary.json").write_text(json.dumps(summary, indent=2))
    print(f"\nAnnotated images + summary.json written to {config.OUTPUT_DIR}")


if __name__ == "__main__":
    main()
