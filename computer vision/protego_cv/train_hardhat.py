"""
(Optional) Train your own hard-hat detector with YOLOv8 once you have a labelled
dataset. You do NOT need this for the demo — the heuristic backend works out of
the box. Use this only when you want production-grade, colour-independent helmet
detection.

Getting a dataset (fastest paths):
  * Roboflow Universe -> search "hard hat" / "PPE" -> export in "YOLOv8" format.
    You get a folder with data.yaml + train/valid/test images & labels.
  * Or label your own images with the Roboflow / CVAT / LabelImg tools.

Then:
    pip install ultralytics
    python train_hardhat.py --data /path/to/data.yaml --epochs 50

The best weights land in runs/detect/train*/weights/best.pt — copy that to
models/hardhat.pt and set USE_PPE_MODEL = True in config.py.
"""
import argparse
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", required=True, help="path to dataset data.yaml")
    ap.add_argument("--epochs", type=int, default=50)
    ap.add_argument("--imgsz", type=int, default=640)
    ap.add_argument("--base", default="yolov8n.pt", help="base weights to fine-tune")
    args = ap.parse_args()

    from ultralytics import YOLO
    model = YOLO(args.base)
    results = model.train(data=args.data, epochs=args.epochs, imgsz=args.imgsz)
    best = Path(results.save_dir) / "weights" / "best.pt"
    print(f"\n[done] best weights: {best}")
    print("Copy it:  python get_hardhat_model.py --src", best)


if __name__ == "__main__":
    main()
