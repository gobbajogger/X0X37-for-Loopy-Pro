#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Upload break loops to the X0X BREAK part: the device's user slots, packed with as many loops as
fit (each loop is one zone). On the device they follow the built-in loops in LOOP A / LOOP B as BR1,
BR2, ... (the firmware numbers the loops of every slot straight through, in order).

  tools/upload_breaks.py [--bars] [--rate 44100|22050] [--trim] [--slots 1,2] [--name BR] FILE.wav ...
  tools/upload_breaks.py --dry-run OUTDIR [--nslots N] [--slot-kb K[,K...]] ...   (no device)
  tools/upload_breaks.py --bbgen [BBGEN_SAMPLES_DIR] ...    (BB Gen's classic breaks, one bar each)

  --bars     cut one bar from each file (bar count guessed from its length: the one that puts
             its tempo nearest 120 BPM inside 80-180); without it, files are taken whole
  --rate     44100 (default: full quality) or 22050 (opt-in: twice the loops in a slot, duller)
  --oldschool  medium grit (2x); same as --oldschool-grit medium
  --oldschool-grit light|medium|heavy|max|FACTOR  crunch amount (off by default)
  --trim     cut the silence at both ends of each file first (sampleio.trim_silence: -60 dBFS,
             1 ms kept before the sound, 5 ms fade at the end). Off by default: a loop's length
             is its tempo, and trimming its ends changes it
  --slots    which slots to fill (default: all the device has, from SMP_INFO)
  --bbgen    use tools/import_breaks.py's selection from BB Gen's sample folder (implies --bars)
  --dry-run  build the slot images (OUTDIR/slotN.hdr / .bin) and print the plan; no device. The
             layout is --slot-kb, one size per slot (default 80,120: the SMK-37's), or --nslots
             slots of one --slot-kb size (80 if not given)

The slot count and the size of each slot come from the device (SMP_INFO). Files are packed in the order given,
filling slot 1, then 2, and so on. A loop that does not fit in the remaining slots is reported and
skipped. Uses Felucca's encoder (tools/sampleio.py) and upload protocol (tools/fm1_sample_upload.py,
tools/x0x_slots.py)."""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sampleio as sio  # noqa: E402
import import_breaks as ib  # noqa: E402

ROOT_BASE = 36                                  # zone i gets root 36 + i: sampleio sorts zones by root


RATES = (44100, 22050)
DRY_SLOT_KB = [80, 120]                         # --dry-run's default layout: the SMK-37's slots (KiB)


def cut(path, bars, rate=44100, trim=False, grit_factor=1.0):
    sr, x = sio.read_any_wav(path)
    if trim:
        x = sio.trim_silence(x, sr)
    if bars:
        n, _ = ib.bars_of(len(x) / sr)
        x = x[: int(round(len(x) / n))]
    y = sio.resample(x, sr, rate)
    pk = max(1e-9, max(abs(v) for v in y))
    s = sio.to_int16([v * (0.95 / pk) for v in y])
    if grit_factor > 1:
        return sio.apply_oldschool(s, rate, grit_factor)
    return s, rate


def pack(loops, nslots, cap=sio.SLOT_MAX_DATA):
    """loops: [(label, int16 samples)] -> slots: [[(label, samples)]] (one list per slot, empty ones
    included), skipped labels. cap: ADPCM bytes a slot, one value or one per slot. The loops go in
    order: into the current slot if they fit, else into the next slot that can take them. A loop larger
    than every slot left is skipped; a later, smaller one can still go into the current slot."""
    caps = list(cap) if isinstance(cap, (list, tuple)) else [cap] * nslots
    slots, cur, skipped = [[] for _ in range(nslots)], 0, []
    used = 0
    for label, s, _playback in loops:
        need = (len(s) + 1) // 2
        fit = [j for j in range(cur, nslots)
               if (used if j == cur else 0) + need <= caps[j] and len(slots[j]) < sio.SLOT_ZONES]
        if not fit:
            skipped.append(label)
            continue
        if fit[0] != cur:
            cur, used = fit[0], 0
        slots[cur].append((label, s, _playback))
        used += need
    while slots and not slots[-1]:
        slots.pop()
    return slots, skipped


def main():
    a = sys.argv[1:]
    if not a or a[0] in ("-h", "--help"):
        sys.exit(__doc__)
    bars = dry = slot_ids = None
    rate, trim, grit_factor, nslots, slot_kb, name, files = 44100, False, 1.0, None, None, "BR", []
    while a:
        o = a.pop(0)
        if o == "--bars":
            bars = True
        elif o == "--dry-run":
            dry = Path(a.pop(0))
        elif o == "--rate":
            rate = int(a.pop(0))
            if rate not in RATES:
                sys.exit("--rate: 44100 or 22050")
        elif o == "--trim":
            trim = True
        elif o == "--oldschool":
            grit_factor = sio.OLDSCHOOL_FACTOR
        elif o == "--oldschool-grit":
            if not a:
                sys.exit("--oldschool-grit needs light, medium, heavy, max, or a number")
            try:
                grit_factor = sio.parse_oldschool_grit(a.pop(0))
            except ValueError as e:
                sys.exit(str(e))
        elif o == "--nslots":
            nslots = int(a.pop(0))
        elif o == "--slot-kb":
            slot_kb = [int(v) for v in a.pop(0).split(",")]
        elif o == "--slots":
            slot_ids = [int(v) for v in a.pop(0).split(",")]
        elif o == "--name":
            name = a.pop(0)
        elif o == "--bbgen":
            src = Path(a.pop(0)) if a and not a[0].startswith("--") else Path(__file__).resolve().parents[2] / "schwung-breakbeat" / "samples"
            files += [(lbl, src / fn) for fn, lbl in ib.BANK + ib.MORE]
            bars = True
        else:
            files.append((Path(o).stem.upper()[:6], Path(o)))
    link = None
    if dry:
        kb = slot_kb or ([80] * nslots if nslots else DRY_SLOT_KB)
        sizes = [v * 1024 for v in (kb * nslots if nslots and len(kb) == 1 else kb)]
    else:                                       # the device's layout: SMP_INFO
        import fm1_sample_upload as up
        import x0x_slots as xs
        link = up.Link()
        lay = xs.layout(link)
        sizes = [sl["size"] for sl in lay["slots"]]
    nslots = len(sizes)
    if slot_ids is None:
        slot_ids = list(range(1, nslots + 1))
    if any(not 1 <= k <= nslots for k in slot_ids):
        sys.exit(f"--slots: the device has slots 1..{nslots}")
    caps = [sizes[k - 1] - sio.SLOT_DATA_OFF for k in slot_ids]
    loops = [(lbl, *cut(p, bars, rate, trim, grit_factor)) for lbl, p in files]
    slots, skipped = pack(loops, len(slot_ids), caps)
    snames = [f"{name[:2]}{slot_ids[i]}" for i in range(len(slots))]   # the device shows 3 chars + .zone
    print(f"{len(slot_ids)} slot(s) of {'/'.join(str(sizes[k - 1] // 1024) for k in slot_ids)} KiB, {rate} Hz")
    for i, sl in enumerate(slots):
        if not sl:
            continue
        names = ", ".join(f"{snames[i]}.{z + 1}={lbl}" if len(sl) > 1 else f"{snames[i]}={lbl}" for z, (lbl, _, __) in enumerate(sl))
        kib = sum((len(s) + 1) // 2 for _, s, __ in sl) / 1024
        print(f"USR{slot_ids[i]}: {len(sl)} loop(s), {kib:.1f} KiB: {names}")
    if skipped:
        print("did not fit (skipped):", ", ".join(skipped))
    images = [sio.user_slot(snames[i], [(s, ROOT_BASE + z, None, None) for z, (_, s, _pr) in enumerate(sl)],
                            rate=sl[0][2], slot_size=sizes[slot_ids[i] - 1]) if sl else None
              for i, sl in enumerate(slots)]
    if dry:
        dry.mkdir(parents=True, exist_ok=True)
        for i, im in enumerate(images):
            if im:
                (dry / f"slot{slot_ids[i]}.hdr").write_bytes(im[0])
                (dry / f"slot{slot_ids[i]}.bin").write_bytes(im[1])
        print(f"dry run: images in {dry}")
        return
    for i, im in enumerate(images):
        if not im:
            continue
        hdr, data = im
        slot = slot_ids[i] - 1
        t0 = time.time()
        img = hdr.ljust(sio.SLOT_DATA_OFF, b"\xff") + data
        try:
            xs.write_slot(link, slot, img, progress=lambda d, n, k=slot: print(f"\r  USR{k + 1}: {d} / {n}", end="", flush=True))
            print(f"\n  {time.time() - t0:.1f} s, ok")
        except xs.SlotError as e:
            sys.exit(f"\n  USR{slot + 1}: {e}")


if __name__ == "__main__":
    main()
