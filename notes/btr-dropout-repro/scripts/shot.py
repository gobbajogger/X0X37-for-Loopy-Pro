#!/usr/bin/env python3
"""Run a short fm1-live --shot, write LCD under OUT, never use leftover cwd."""
import argparse, os, subprocess, shutil, time
from pathlib import Path

EMU = "/workspace/emu360-target/release/fm1-live"
FWSC = "/workspace/build-1214/artifacts/elite/DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--keys", required=True)
    ap.add_argument("--shot", type=int, required=True)
    ap.add_argument("--usb", action="store_true")
    ap.add_argument("--usb-send")
    ap.add_argument("--record", action="store_true")
    args = ap.parse_args()
    out = Path(args.out).resolve()
    if out.exists():
        shutil.rmtree(out)
    work = out / "work"
    work.mkdir(parents=True)
    env = os.environ.copy()
    env["FM1_STRICT"] = "1"
    env["FM1_BENCH_KEYS"] = args.keys if args.keys.startswith(("@",)) or ":" in args.keys else "@" + str(Path(args.keys).resolve())
    if args.keys.startswith("@") or ":" in args.keys:
        if not args.keys.startswith("@") and ":" in args.keys and not Path(args.keys).exists():
            env["FM1_BENCH_KEYS"] = args.keys
        elif args.keys.startswith("@"):
            env["FM1_BENCH_KEYS"] = args.keys
        else:
            env["FM1_BENCH_KEYS"] = "@" + str(Path(args.keys).resolve())
    env["FM1_DUMPRAM"] = str(out / "ram.bin")
    if args.usb_send:
        env["FM1_USB_SEND"] = args.usb_send
    cmd = [EMU, "--fwsc", FWSC, "--mhz", "360", "--shot", str(args.shot), "--lcd-only"]
    if args.usb or args.usb_send:
        cmd.append("--usb")
    else:
        cmd.extend(["--midi", "none"])
    if args.record:
        rec = out / "frames"
        rec.mkdir()
        cmd.extend(["--record", str(rec)])
    log = out / "emu.log"
    t0 = time.perf_counter()
    with log.open("w") as f:
        r = subprocess.run(cmd, cwd=out, env=env, stdout=f, stderr=subprocess.STDOUT)
    wall = time.perf_counter() - t0
    ppm = work / "lcd.ppm"
    # --record may not write final lcd.ppm the same way; copy last frame if needed
    if not ppm.exists() and args.record:
        frames = sorted((out / "frames").glob("*.ppm"))
        if frames:
            shutil.copy(frames[-1], ppm)
    if ppm.exists():
        from PIL import Image
        img = Image.open(ppm)
        img.save(out / "lcd.png")
        img.resize((img.width * 2, img.height * 2), Image.Resampling.NEAREST).save(out / "lcd-2x.png")
    print(f"exit={r.returncode} shot={args.shot} wall={wall:.2f}s lcd={ppm.exists()} out={out}")
    print(log.read_text()[-1200:])
    return r.returncode

if __name__ == "__main__":
    raise SystemExit(main())
