#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reference data for tests/host/drum909_adpcm_test.c (the 909 cymbals as IMA ADPCM).

  tests/host/cymbal_ref.py OUTDIR [AB_DIR]
  tests/host/cymbal_ref.py --wav OUTDIR     (OUTDIR/hh.wav, ride.wav, crash.wav: the decoded ADPCM as
                                             16-bit WAV, for 9W9's sample folder in run_drum909.sh)

For each cymbal (hh, ride, crash), writes OUTDIR/<c>.ref: the expected decoded samples, int16 LE.
- With AB_DIR (the approved A/B study, default ../adpcm_ab): the ADPCM in build/gen/x0x_drum_samples.h
  must be byte-identical to AB_DIR/<c>.adpcm, and the reference is AB_DIR/<c>_2_adpcm44k.wav (decoded
  there by a verbatim copy of the firmware decoder).
- Without AB_DIR: the reference is this script's own IMA decode (sampleio.py's tables) of the header's
  ADPCM.
Also writes OUTDIR/<c>.pcm: the original int16 PCM (assets/909, as gen_drum_samples.py reads it), for the
SNR check."""
import re
import struct
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SRC / "tools"))
import sampleio as sio  # noqa: E402
import gen_drum_samples as gds  # noqa: E402

CYM = [("hh", "hh.wav"), ("ride", "ride.wav"), ("crash", "crash.wav")]


def decode(b, n):
    pred, idx, out = 0, 0, []
    for i in range(n):
        code = (b[i >> 1] >> (4 if i & 1 else 0)) & 15
        step = sio.IMA_STEP[idx]
        vd = step >> 3
        if code & 4:
            vd += step
        if code & 2:
            vd += step >> 1
        if code & 1:
            vd += step >> 2
        pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
        idx = max(0, min(88, idx + sio.IMA_IDX[code & 7]))
        out.append(pred)
    return out


def wav_pcm(path):
    d = path.read_bytes()
    ch = sio.wav_chunks(d)
    o, n = ch[b"data"]
    return list(struct.unpack_from("<%dh" % (n // 2), d, o))


def header_adpcm(c):
    hdr = (SRC / "build" / "gen" / "x0x_drum_samples.h").read_text()
    m = re.search(r"x0x_smp_%s\[(\d+)\] = \{(.*?)\};" % c, hdr, re.S)
    n = int(re.search(r"X0X_SMP_%s_LEN (\d+)u" % c.upper(), hdr).group(1))
    return bytes(int(v) for v in re.findall(r"\d+", m.group(2))), n


def write_wavs(out):
    out.mkdir(parents=True, exist_ok=True)
    for c, fn in CYM:
        data, n = header_adpcm(c)
        pcm = struct.pack("<%dh" % n, *decode(data, n))
        (out / fn).write_bytes(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " +
                               struct.pack("<IHHIIHH", 16, 1, 1, 44100, 88200, 2, 16) + b"data" +
                               struct.pack("<I", len(pcm)) + pcm)


def main():
    if sys.argv[1] == "--wav":
        write_wavs(Path(sys.argv[2]))
        return
    out = Path(sys.argv[1])
    ab = Path(sys.argv[2]) if len(sys.argv) > 2 else SRC.parent / "adpcm_ab"
    use_ab = (ab / "hh.adpcm").is_file()
    out.mkdir(parents=True, exist_ok=True)
    for c, fn in CYM:
        data, n = header_adpcm(c)
        if use_ab:
            if data != (ab / f"{c}.adpcm").read_bytes():
                sys.exit(f"{c}: the firmware ADPCM is not the A/B study's {ab / (c + '.adpcm')}")
            ref = wav_pcm(ab / f"{c}_2_adpcm44k.wav")
        else:
            ref = decode(data, n)
        if len(ref) != n:
            sys.exit(f"{c}: reference has {len(ref)} samples, header says {n}")
        (out / f"{c}.ref").write_bytes(struct.pack("<%dh" % n, *ref))
        pcm, _ = gds.read_wav(gds.ASSETS / fn)
        (out / f"{c}.pcm").write_bytes(struct.pack("<%dh" % n, *pcm))
    print(f"cymbal references: {'A/B study ' + str(ab) if use_ab else 'own decoder (no A/B study found)'}")


if __name__ == "__main__":
    main()
