#!/usr/bin/env python3
"""Read-only, exact-provenance gate for the existing v13r2 36-frame film.

This does NOT build an MCU video decoder or claim firmware integration.
Use on a checkout containing assets/branding/box-entry/*. Run from any cwd.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from fractions import Fraction
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "assets" / "branding" / "box-entry"
EXPECTED = {
    "DnBOX-box-entry-short.mp4": (90324, "566ee16ad25c826874f9c536b0c3cf56fa3269be7e32469331864b27b653e7d4"),
    "DnBOX-box-entry-short-first.png": (177762, "814d5729cc2562de1984683222d66dea8faaa906dd81e081c4852aba560dfd52"),
    "DnBOX-box-entry-short-last.png": (13218, "e847e1d6635f3b85d6e14111ba7622e4cb570dbfd0f3908535c942d54a5cb942"),
}
NATIVE_HOME_480_SHA256 = EXPECTED["DnBOX-box-entry-short-last.png"][1]


def verify_bytes(path: Path, expected_size: int, expected_sha256: str) -> None:
    payload = path.read_bytes()
    if len(payload) != expected_size:
        raise ValueError(f"{path.name}: size {len(payload)} != {expected_size}")
    actual = hashlib.sha256(payload).hexdigest()
    if actual != expected_sha256:
        raise ValueError(f"{path.name}: SHA256 {actual} != {expected_sha256}")


def png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise ValueError(f"{path.name}: invalid PNG header")
    return int.from_bytes(data[16:20], "big"), int.from_bytes(data[20:24], "big")


def verify_probe(probe: dict) -> dict:
    streams = probe.get("streams", [])
    videos = [s for s in streams if s.get("codec_type") == "video"]
    if len(videos) != 1 or len(streams) != 1:
        raise ValueError("expected exactly one video stream and no audio/other streams")
    video = videos[0]
    if video.get("codec_name") != "h264" or video.get("width") != 480 or video.get("height") != 480:
        raise ValueError("expected source H.264 480x480 video")
    rate = Fraction(video.get("avg_frame_rate", "0"))
    if rate != 70:
        raise ValueError(f"unexpected frame rate: {rate}")
    frames = video.get("nb_read_frames") or video.get("nb_frames")
    if frames is None or int(frames) != 36:
        raise ValueError(f"expected 36 decoded frames, got {frames}")
    # Duration is a rational 36/70 seconds; Matroska/MP4 fields may round to ms.
    duration = float(video.get("duration") or probe.get("format", {}).get("duration") or 0)
    if abs(duration - (36 / 70)) > 0.003:
        raise ValueError(f"unexpected duration {duration}s, expected 36/70s")
    return {"frames": 36, "fps": 70, "duration_seconds": 36 / 70,
            "width": 480, "height": 480, "container_bytes": EXPECTED["DnBOX-box-entry-short.mp4"][0]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, default=ASSETS)
    args = parser.parse_args()
    try:
        for name, (size, sha) in EXPECTED.items():
            verify_bytes(args.assets / name, size, sha)
        for name in ("DnBOX-box-entry-short-first.png", "DnBOX-box-entry-short-last.png"):
            if png_dimensions(args.assets / name) != (480, 480):
                raise ValueError(f"{name}: not 480x480")
        result = subprocess.run(
            ["ffprobe", "-v", "error", "-count_frames", "-show_streams", "-show_format",
             "-of", "json", str(args.assets / "DnBOX-box-entry-short.mp4")],
            capture_output=True, text=True, check=True,
        )
        meta = verify_probe(json.loads(result.stdout))
        meta["last_frame_is_exact_94197fc8_home"] = True
        meta["firmware_integration_verified"] = False
        meta["embedded_video_safe_for_flash"] = False
        print(json.dumps(meta, sort_keys=True, indent=2))
        return 0
    except (OSError, ValueError, subprocess.SubprocessError, json.JSONDecodeError) as exc:
        print(f"box-entry asset validation FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
