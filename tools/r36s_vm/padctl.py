"""Send physical R36S button labels to the local VM preview and capture evidence."""

import argparse
import json
from pathlib import Path
import time
import uuid

BUTTONS = {"UP": 0, "DOWN": 1, "LEFT": 2, "RIGHT": 3, "A": 4, "B": 5, "X": 6, "START": 7,
           "SELECT": 9, "L1": 10, "R1": 11, "Y": 12, "L2": 13, "R2": 14, "FN": 15}


def atomic_json(path, value):
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value), encoding="utf-8")
    temporary.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("buttons", nargs="*", type=str.upper, choices=BUTTONS)
    parser.add_argument("--hold", type=int, default=180)
    parser.add_argument("--settle", type=int, default=550)
    parser.add_argument("--session", type=Path, required=True)
    args = parser.parse_args()
    if not (30 <= args.hold <= 4000 and 0 <= args.settle <= 5000):
        parser.error("Use a hold of 30–4000 ms and a settle of 0–5000 ms")
    if not (args.session / "ready.json").is_file():
        parser.error("Wait for the preview to receive a frame")
    request = args.session / f"{uuid.uuid4().hex}.request.json"
    atomic_json(request, {"buttons": list(dict.fromkeys(args.buttons)), "hold": args.hold if args.buttons else 0,
                          "settle": args.settle})
    answer = request.with_name(request.name.replace(".request.", ".result."))
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        if answer.is_file():
            result = json.loads(answer.read_text(encoding="utf-8"))
            print(json.dumps(result, ensure_ascii=False))
            return 0 if result["ok"] else 1
        time.sleep(0.05)
    raise SystemExit("The preview did not answer the controller command")


if __name__ == "__main__":
    raise SystemExit(main())
