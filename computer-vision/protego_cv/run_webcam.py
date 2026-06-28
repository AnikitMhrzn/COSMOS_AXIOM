"""
Live demo: continuously capture the laptop webcam, run the compliance pipeline,
overlay results, and push records to the dashboard every few seconds.

    python run_webcam.py            # default camera 0
    python run_webcam.py --cam 1    # pick another camera

Press 'q' to quit.
"""
import argparse
import cv2

from pipeline import Pipeline
from dashboard import DashboardSender


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cam", type=int, default=0)
    args = ap.parse_args()

    pipe = Pipeline()
    dash = DashboardSender()
    cap = cv2.VideoCapture(args.cam)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.cam}")

    print("[Protego] running. Press 'q' to quit.")
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        people, records = pipe.process(frame)
        dash.maybe_push(records)
        pipe.draw(frame, people)
        cv2.imshow("Protego PPE & Identity Compliance", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
