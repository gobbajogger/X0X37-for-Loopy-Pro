#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Load WAV files into a Felucca user sample slot (USR1..USR3) over USB-MIDI.

  fm1_sample_upload.py info
  fm1_sample_upload.py load SLOT NAME file.wav[:ROOT[:LO-HI]] ...   (SLOT 1..3)
  fm1_sample_upload.py --no-trim load|build ...                     (keep the silence at the ends)
  fm1_sample_upload.py erase SLOT
  fm1_sample_upload.py build NAME OUT_PREFIX file.wav[...] ...      (no device: writes OUT_PREFIX.hdr / .bin)

Each file becomes one zone: mono, 22050 Hz, IMA ADPCM (sampleio.py, the same encoder
as the built-in sets and the web editor). ROOT is a MIDI note (default: from the file
name, C4 = 60, else 60); without LO-HI the zones split the keyboard between their roots.
These are one-shots: the silence at both ends is cut first (sampleio.trim_silence: -60 dBFS, 1 ms
kept before the sound, 5 ms fade at the end); --no-trim keeps it. For loops use upload_breaks.py.
A slot holds 80 KiB (about 7 s at 22050 Hz). Protocol: web/EDITOR_PROTOCOL.md, cmds 11..15.
Read-only native MIDI info/backup and offline slot-image building remain supported.
Offline build never replaces existing .hdr/.bin outputs; use a private destination folder.
Native MIDI sample writes/erase/restore are intentionally disabled: the RtMidi
backend cannot authenticate a physical unit or guarantee atomic output delivery.
Matching MIDI names and successful read-only probes are not sufficient.
Needs mido (and a backend such as python-rtmidi) for read-only device commands.
"""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sampleio as sio  # noqa: E402

HDR = [0x7D, 0x46, 0x4C]
RC = {1: "size", 2: "header", 3: "data CRC", 4: "flash", 5: "zones"}


def pack7(b):
    """groups of up to 7 bytes, each preceded by their top bits"""
    out = []
    for i in range(0, len(b), 7):
        g = b[i:i + 7]
        out.append(sum(((x >> 7) & 1) << j for j, x in enumerate(g)))
        out += [x & 0x7F for x in g]
    return out


class Link:
    def __init__(self):
        try:
            import mido
        except ImportError:
            sys.exit("needs mido: pip install mido python-rtmidi")
        self.mido = mido
        outs = [n for n in mido.get_output_names() if "Felucca" in n or "X0X" in n]
        ins = [n for n in mido.get_input_names() if "Felucca" in n or "X0X" in n]
        if not outs or not ins:
            sys.exit("no MIDI port named Felucca: connect the FM-1 (running Felucca) by USB")
        # Never use the first input and first output independently. Even their
        # shared display name is NOT a hardware-unique physical identity; this
        # guard only prevents already-visible ambiguity and crossed MIDI ports.
        if len(outs) != 1 or len(ins) != 1 or outs[0] != ins[0]:
            sys.exit("ambiguous Felucca/X0X MIDI input/output ports: disconnect other "
                     "keyboards and use one exact input/output name; no sample write was started")
        self.o = mido.open_output(outs[0])
        try:
            self.i = mido.open_input(ins[0])
        except BaseException:
            self.o.close()
            raise
        try:
            for _ in range(3):  # the first frame after opening may be lost
                try:
                    self.req(1, [], 1.0)
                    break
                except TimeoutError:
                    continue
            else:
                sys.exit("selected sample MIDI device did not answer the initial query; "
                         "refusing erase/restore/write")
        except BaseException:
            self.i.close()
            self.o.close()
            raise

    def req(self, cmd, args, timeout=3.0):
        # RtMidi endpoint names and an SMP_INFO response cannot authenticate a
        # physical unit, nor make the outgoing send atomic against a USB swap.
        # Until the production backend provides an authenticated per-unit
        # transaction, this native link is READ ONLY. This protects the sample
        # upload, break upload and x0x_slots restore consumers of this Link.
        if type(cmd) is not int or cmd not in (1, 15, 16):
            sys.exit("native USB-MIDI sample writes are unsupported: no authenticated "
                     "physical-device binding or atomic output delivery; no erase/write "
                     "packet sent. Use offline build/show/extract, or await a separately "
                     "qualified hardware-bound installer")
        for _ in self.i.iter_pending():
            pass
        self.o.send(self.mido.Message("sysex", data=HDR + [cmd] + list(args)))
        t0 = time.time()
        while time.time() - t0 < timeout:
            for m in self.i.iter_pending():
                if m.type == "sysex" and list(m.data[:4]) == HDR + [cmd]:
                    return list(m.data[4:])
            time.sleep(0.002)
        raise TimeoutError(f"no reply to cmd {cmd}")


def write_slot_pair(prefix, hdr, data):
    """Create both offline slot outputs without overwriting another recording.

    Bind all operations to one no-follow destination directory. Any partial
    pair is removed only while its exact newly-created inode remains at the
    original name. If both descriptor identity queries fail after creation,
    the unidentified output is left for owner inspection rather than blindly
    deleted. Like extraction, use a private destination directory when
    another local process might change its contents concurrently.
    """
    import os
    from pathlib import Path

    dest = Path(prefix)
    if not dest.name or not hasattr(os, "O_NOFOLLOW") or not hasattr(os, "O_DIRECTORY"):
        raise OSError("safe slot build requires POSIX no-follow directory handles")
    directory = os.open(dest.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    names = ((dest.name + ".hdr", hdr), (dest.name + ".bin", data))
    created = []
    try:
        for name, _ in names:
            try:
                os.stat(name, dir_fd=directory, follow_symlinks=False)
            except FileNotFoundError:
                pass
            else:
                raise FileExistsError(f"slot build refuses existing output: {dest.parent / name}")
        try:
            for name, payload in names:
                fd = os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                             0o600, dir_fd=directory)
                try:
                    try:
                        identity = os.fstat(fd)
                    except BaseException:
                        # Creation succeeded. Query the original open descriptor,
                        # never a name another process could already have swapped.
                        try:
                            identity = os.stat(fd)
                        except BaseException:
                            raise OSError(
                                f"slot output identity unavailable after creating "
                                f"{dest.parent / name}. Manual recovery required: "
                                "automatic deletion is unsafe without inode proof. "
                                "Inspect this file in the private output directory, "
                                "remove it only if verified as this attempt's partial "
                                "output, and retry. Existing files were not overwritten."
                            )
                    created.append((name, identity.st_dev, identity.st_ino))
                    file = os.fdopen(fd, "wb")
                    fd = None  # fdopen owns it from here
                    with file:
                        if file.write(payload) != len(payload):
                            raise OSError(f"short slot build write: {name}")
                        file.flush()
                        os.fsync(file.fileno())
                finally:
                    if fd is not None:
                        os.close(fd)
            for name, dev, ino in created:
                actual = os.stat(name, dir_fd=directory, follow_symlinks=False)
                if (actual.st_dev, actual.st_ino) != (dev, ino):
                    raise OSError(f"slot build output replaced during writing: {name}")
            bound = os.fstat(directory)
            current = os.stat(dest.parent, follow_symlinks=False)
            if (bound.st_dev, bound.st_ino) != (current.st_dev, current.st_ino):
                raise OSError("slot build output directory changed while writing")
            os.fsync(directory)
        except BaseException:
            for name, dev, ino in created:
                try:
                    actual = os.stat(name, dir_fd=directory, follow_symlinks=False)
                    if (actual.st_dev, actual.st_ino) == (dev, ino):
                        os.unlink(name, dir_fd=directory)
                except OSError:
                    pass  # do not guess whether another owner's file replaced it
            raise
    finally:
        os.close(directory)


def build(name, specs, trim=True):
    """specs: "file.wav[:ROOT[:LO-HI]]" -> (header, ADPCM data) for SMP_END / SMP_WRITE. trim: cut the
    silence at both ends (one-shots)"""
    zones = []
    for spec in specs:
        parts = spec.split(":")
        path = Path(parts[0])
        root = int(parts[1]) if len(parts) > 1 and parts[1] else sio.note_from_name(path.stem)
        lo, hi = (int(v) for v in parts[2].split("-")) if len(parts) > 2 else (None, None)
        sr, x = sio.read_any_wav(path)
        if trim:
            x = sio.trim_silence(x, sr)
        x = sio.resample(x, sr, sio.SLOT_RATE)
        if not x:
            raise ValueError(f"{path}: no audio")
        zones.append((sio.to_int16(x), root, lo, hi))
    return sio.user_slot(name, zones)


def main():
    trim = "--no-trim" not in sys.argv
    if not trim:
        sys.argv.remove("--no-trim")
    if len(sys.argv) < 2 or sys.argv[1] not in ("info", "load", "erase", "build"):
        sys.exit(__doc__)
    cmd = sys.argv[1]
    if cmd in ("load", "erase"):
        sys.exit("native MIDI sample load/erase disabled: no authenticated physical-device "
                 "identity or atomic output delivery. No MIDI port was opened or write sent. "
                 "Use build offline or info read-only.")
    if cmd in ("load", "build"):
        if len(sys.argv) < 5:
            sys.exit(__doc__)
        try:
            hdr, data = build(sys.argv[3] if cmd == "load" else sys.argv[2], sys.argv[4:], trim)
        except (OSError, ValueError) as e:
            sys.exit(f"cannot build the slot: {e}")
        if cmd == "build":
            try:
                write_slot_pair(sys.argv[3], hdr, data)
            except OSError as e:
                sys.exit(f"cannot safely save the slot: {e}")
            print(f"{sys.argv[2]}: {len(data)} B ADPCM, {hdr[6]} zones")
            return
    if cmd != "info":
        if len(sys.argv) < 3 or sys.argv[2] not in ("1", "2", "3"):
            sys.exit("SLOT is 1, 2 or 3")
        slot = int(sys.argv[2]) - 1
    link = Link()
    if cmd == "info":
        r = link.req(15, [])
        n, i = r[0], 2
        for k in range(n):
            nz = r[i]
            j = r.index(0, i + 1)
            print(f"USR{k + 1}: {'empty' if not nz else f'{nz} zones, {bytes(r[i + 1:j]).decode()}, {r[j + 1]} KiB'}")
            i = j + 2
        return
    if cmd == "erase":
        print("erase:", "ok" if link.req(14, [slot], 10)[1] == 0 else "FAILED")
        return
    print(f"USR{slot + 1} {sys.argv[3]}: {len(data)} B ADPCM, {hdr[6]} zones")
    if link.req(11, [slot])[1]:
        sys.exit("begin failed")
    t0 = time.time()
    for off in range(0, len(data), 256):
        a = sio.SLOT_DATA_OFF + off
        r = link.req(12, [slot, a & 0x7F, (a >> 7) & 0x7F, (a >> 14) & 0x7F] + pack7(data[off:off + 256]))
        if r[4]:
            sys.exit(f"write at {a:#x} failed ({r[4]})")
        print(f"\r  {min(off + 256, len(data))} / {len(data)}", end="", flush=True)
    r = link.req(13, [slot] + pack7(hdr), 5)
    print(f"\n  {time.time() - t0:.1f} s, end:", "ok" if r[1] == 0 else f"FAILED ({r[1]}, {RC.get(r[1], '?')})")


if __name__ == "__main__":
    try:
        main()
    except TimeoutError as e:
        sys.exit(f"{e}: is Felucca running, and no other app using its MIDI port?")
