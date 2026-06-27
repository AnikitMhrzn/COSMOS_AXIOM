# Protego — PPE & Identity Compliance (Computer Vision Module)

Monitors a work area through a laptop webcam and, for every person in frame,
answers two questions:

1. **Are they wearing a helmet?** (Task 1 — helmet detection)
2. **Who are they?** — reads the printed badge number on their vest
   (Task 2 — badge OCR, digits only)

The two answers are merged into one record per worker, e.g.
`Worker #913 — helmet: yes` or `Worker #022 — MISSING helmet`, and pushed to the
Protego dashboard every few seconds.

## How it works

```
webcam frame
   │
   ├─ YOLOv8 (COCO) ──────────► person boxes
   │                                │
   │                ┌───────────────┴───────────────┐
   │                ▼                                ▼
   │        head region                       torso region
   │        helmet check                       EasyOCR (digits)
   │        (Task 1)                            (Task 2)
   │                └───────────────┬───────────────┘
   ▼                                ▼
 overlay  ◄──────────  ComplianceRecord per worker  ──────────►  dashboard
```

- **Person detection:** stock `yolov8n.pt` (auto-downloads on first run, no
  training needed).
- **Helmet decision (default):** a fast colour/heuristic check on the head
  region — zero extra model weights, ideal for a demo. Tune `HELMET_HSV_*` in
  `config.py` to your helmet colour. For production accuracy, drop a trained
  hard-hat YOLO model into `models/hardhat.pt` and set `USE_PPE_MODEL = True`.
- **Badge OCR:** EasyOCR restricted to `0–9`, run on the torso crop only.

> Why no custom-trained model? 13 photos is plenty to *test* but far too few to
> *train* a reliable detector. Using a pretrained YOLOv8 + heuristic/OCR gets a
> working demo immediately. The code is structured so you can swap in a trained
> PPE model later by changing two lines in `config.py`.

## Install

```bash
cd protego_cv
pip install -r requirements.txt
```

First run downloads `yolov8n.pt` (~6 MB) and the EasyOCR English model
automatically.

## Run

Live webcam demo:
```bash
python run_webcam.py            # press q to quit
python run_webcam.py --cam 1    # different camera
```

Test on still images (e.g. your sample photos):
```bash
python run_images.py                          # ../person-with-number by default
python run_images.py ../badge                 # any folder
python run_images.py ../person-with-number/IMG_0074.jpeg   # one image
```
Annotated images and a `summary.json` are written to `runs/`.

## Files

| File | Purpose |
|------|---------|
| `config.py` | All tunable settings (thresholds, model paths, dashboard URL). |
| `helmet_detector.py` | Task 1 — person detection + helmet decision. |
| `badge_ocr.py` | Task 2 — EasyOCR digit reading on the torso crop. |
| `pipeline.py` | Merges both into one `ComplianceRecord` per worker + overlays. |
| `dashboard.py` | Pushes records to the Protego dashboard (prints if no URL set). |
| `run_webcam.py` | Live webcam entry point. |
| `run_images.py` | Offline/image entry point for testing. |

## Live dashboard

A self-contained dashboard ships with the project (no Flask, pure stdlib).

```bash
# terminal 1 — start the dashboard
python dashboard_server.py            # http://localhost:8000
```
Then set `DASHBOARD_URL = "http://localhost:8000/api/compliance"` in `config.py`
and run the pipeline in another terminal:
```bash
# terminal 2 — feed it
python run_webcam.py
```
Open <http://localhost:8000> to watch worker cards flip green (helmet OK) / red
(missing helmet) in real time, with live/idle status and counts.

Records are POSTed as JSON:
```json
{"timestamp": 1750000000.0,
 "workers": [{"worker": "913", "helmet": true, "helmet_conf": 0.9, "badge_conf": 0.74}]}
```
With no `DASHBOARD_URL` set, the same payload is printed to the console instead.

## Upgrading to a trained hard-hat model

The default colour heuristic is demo-grade. For colour-independent, production
accuracy, plug in a trained YOLOv8 PPE model:

```bash
# you already have a .pt:
python get_hardhat_model.py --src /path/to/model.pt
# or download one:
python get_hardhat_model.py --url https://example.com/hardhat.pt
# inspect class names:
python get_hardhat_model.py --check
```
Then set `USE_PPE_MODEL = True` in `config.py` (and make `PPE_HELMET_CLASSES`
match your model's class names). To train your own from a Roboflow/CVAT dataset,
see `train_hardhat.py`.

## Tuning notes (from testing on the sample photos)

- The badge close-up (`913`) reads correctly once the crop is binarised; EasyOCR
  handles the wrinkled-paper bib better than plain OCR.
- Read the number from a **tight** region — a wide full-body crop pulls in faces
  and hands and confuses OCR. The pipeline already crops the torso slice
  (`TORSO_TOP_FRAC` / `TORSO_BOTTOM_FRAC`) for this reason.
- For real hard hats, set `HELMET_HSV_LOWER/UPPER` to your helmet colour, or
  switch to a trained PPE model for colour-independent detection.
```
```
