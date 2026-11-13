#!/usr/bin/env python3
"""One Elite first-pass config. Requires FIRST_PASS_VALIDATED. Uses the
labelled sampler fm1-live, never the shared emu360-target binary."""
import json, os, subprocess, sys, time, hashlib, shutil
from pathlib import Path

ROOT = Path("/workspace/btr-dropout-repro")
EMU = ROOT / "emu-sampler/fm1-live"
EMU_SHA = "a147b8512fb4bd29170abc957f816a6978feccc2ddfc36bee80df599fb748500"
STOCK = Path("/workspace/emu360-target/release/fm1-live")
STOCK_SHA = "f3e65ccfa39874168b99494055f9a755342c41bc769257883030d15aef7b54f5"
FWSC = Path("/workspace/build-1214/artifacts/elite/DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc")
FWSC_SHA = "95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6"
ELF = Path("/workspace/build-1214/artifacts/elite/x0x.elf")
ELF_SHA = "720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea"
LAYOUT = ROOT / "emu-sampler/half_layout.txt"

def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def main():
    if len(sys.argv) < 2:
        print("usage: run_core_config.py CONFIG")
        return 2
    cfg = sys.argv[1]
    val = (ROOT / "FIRST_PASS_VALIDATED").read_text() if (ROOT / "FIRST_PASS_VALIDATED").exists() else ""
    if "FIRST_PASS_VALIDATED=1" not in val:
        print("REFUSED: FIRST_PASS_VALIDATED is not set")
        return 4
    if cfg == "loo-no-break":
        print("loo-no-break: NOT_RUN (BREAK cannot play on this package; not a 5-min duplicate)")
        return 0
    keys = ROOT / "keys" / f"{cfg}.keys"
    usb = ROOT / "keys" / f"{cfg}.usb"
    states = json.loads((ROOT / "keys" / f"{cfg}.states.json").read_text())
    if not keys.exists() or not usb.exists():
        print(f"missing keys for {cfg}")
        return 2
    # safety: never overwrite stock
    if sha256(STOCK) != STOCK_SHA:
        print("REFUSED: stock fm1-live hash changed")
        return 5
    if sha256(EMU) != EMU_SHA:
        print("REFUSED: sampler fm1-live hash is not the labelled one")
        return 5
    if sha256(FWSC) != FWSC_SHA or sha256(ELF) != ELF_SHA:
        print("REFUSED: Elite fwsc/ELF hash mismatch")
        return 5
    out = ROOT / "logs" / "core" / cfg
    if out.exists():
        shutil.rmtree(out)
    (out / "work").mkdir(parents=True)
    env = os.environ.copy()
    env["FM1_STRICT"] = "1"
    env["FM1_BENCH_KEYS"] = "@" + str(keys.resolve())
    env["FM1_USB_SEND"] = "@" + str(usb.resolve())
    env["FM1_DUMPRAM"] = str(out / "ram.bin")
    env["FM1_HALF_CSV"] = str(out / "half.csv")
    env["FM1_HALF_LAYOUT"] = str(LAYOUT)
    env["FM1_CORE_LOAD"] = "01c00e94-01c010f4"
    env["FM1_CORE_LOAD_CSV"] = str(out / "core1.csv")
    meta = {
        "config": cfg,
        "emu": str(EMU),
        "emu_sha256": EMU_SHA,
        "stock_sha256": STOCK_SHA,
        "stock": "UNTOUCHED",
        "elf_sha256": ELF_SHA,
        "fwsc_sha256": FWSC_SHA,
        "source": "94197fc839b00aff47b98ca02f3c6396e4a6eabf",
        "mhz": 360,
        "FM1_STRICT": 1,
        "BREAK": "NOT_RUN",
        "owner_scene": "partial (BREAK not playable)",
        "states": states,
        "layout": str(LAYOUT),
        "branch": "btr/half-sampler 48d20ab",
    }
    (out / "META.json").write_text(json.dumps(meta, indent=2) + "\n")
    cmd = [str(EMU), "--fwsc", str(FWSC), "--mhz", "360", "--usb",
           "--shot", str(states["shot_ms"]), "--lcd-only"]
    log = out / "emu.log"
    print(f"START {cfg} shot={states['shot_ms']} ms ({states['shot_ms']/60000:.2f} min emu) -> {out}", flush=True)
    t0 = time.perf_counter()
    with log.open("w") as f:
        f.write(json.dumps(meta) + "\n")
        f.flush()
        r = subprocess.run(cmd, cwd=out, env=env, stdout=f, stderr=subprocess.STDOUT)
    wall = time.perf_counter() - t0
    (out / "wall.txt").write_text(f"wall_s={wall:.2f}\nexit={r.returncode}\n")
    print(f"DONE {cfg} exit={r.returncode} wall={wall:.1f}s", flush=True)
    # LCD
    ppm = out / "work/lcd.ppm"
    if ppm.exists():
        from PIL import Image
        img = Image.open(ppm)
        img.save(out / "lcd.png")
        img.resize((img.width * 2, img.height * 2), Image.Resampling.NEAREST).save(out / "lcd-2x.png")
    if (out / "half.csv").exists():
        n = sum(1 for _ in open(out / "half.csv")) - 1
        print(f"half.csv rows={n}", flush=True)
    return r.returncode

if __name__ == "__main__":
    raise SystemExit(main())
