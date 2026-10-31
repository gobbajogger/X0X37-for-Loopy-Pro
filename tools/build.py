#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Build X0X (a fork of Felucca): the app, the update loader and an installable .fwsc package.

  tools/build.py [--release X.Y[-suffix]] [--board fm1|smk37] [--model pro|elite|mke]

Outputs in build/: x0x.bin (app), loader/ota.bin (update loader, Felucca's, unchanged),
x0x.fwsc (package). See BUILDING.md for the toolchain and the SDK.

BOARD=smk37 (or --board smk37) builds for the M-Vave SMK-37 Pro: -DBOARD_SMK37=1 for the app and the
loader, identity SMK-37 Pro_900 (release SMK-37 Pro_9XXYYZZ), the SMK flash layout and app slot
(fm1pkg_make.py), the store owner family X0X (FELUCCA_DATA_FAMILY, FELUCCA_DATA_UNTAGGED_OK=0), and the
package build/x0x-smk37.fwsc (x0x-smk37-X.Y.fwsc for a release). The default stays the FM-1, unchanged.
MODEL=elite / mke (or --model) with smk37: the same build under the SMK-37 Elite's or the MKE-P37's model
name (identity SMK-37 Elite_900 / MKE-P37_900, the loader's and the update record's name too; -DSMK_MODEL),
so M-Vave's updater still takes that unit's factory package; build/x0x-smk37-elite.fwsc, -mke.fwsc.

The JieLi toolchain is Linux x86-64 only. JIELI_TOOLCHAIN points at it; on
macOS (or with JIELI_DOCKER=1) each tool runs in a linux/amd64 container.

X0X_ORIGINAL_SYNTH=1 is the default Chicago/Miami replacement, with the admitted
seven-type Chicago FX policy. Explicit0 retains only a reference build lane.
"""
import argparse
import hashlib
import os
import platform
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
FW = SRC / "firmware"
OUT = SRC / "build"
GEN = OUT / "gen"
LDR = OUT / "loader"
sys.path.insert(0, str(SRC / "tools"))
import fm1pkg_make  # noqa: E402
import gen_cache  # noqa: E402
import lz4blk  # noqa: E402
import rom_guard  # noqa: E402
import check_original_inputs  # noqa: E402

APP_XIP = 0x02000120                # app.bin offset 0 in the XIP map; the SPL jumps here
APP_SLOT = fm1pkg_make.APP_SLOT
APP_CODE_MAX = fm1pkg_make.APP_CODE_MAX   # SMK: the app code ends below the Bluetooth block (fm1_flash_map.h FL_BLE_LO)
LOADER_LOAD = 0x01C0A800
LOADER_NAME = b"usb_hid_ota.bin"    # the file name the SPL looks for
DOCKER_IMAGE = os.environ.get("JIELI_DOCKER_IMAGE", "debian:bookworm-slim")
CFLAGS = ["-Os", "-ffunction-sections", "-fdata-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]
# X0X: the AC79's single-precision FPU (the SDK's own flags); no fused multiply-add, so the host
# build (-ffp-contract=off) computes the same samples
FPU = ["-mcpu=r3", "-mfprev1", "-ffp-contract=off"]

def clang_std_inc():
    """Resolve JieLi standard headers in the compiler's execution environment."""
    include = "/opt/jieli/pi32v2/include" if use_docker() else str(toolchain() / "pi32v2" / "include")
    return ["-idirafter", include]
# the DSP, the sequencer and the audio engine: separate units at -O2
O2_UNITS = ["dsp/drum909.c", "dsp/drum808.c", "dsp/breaks.c", "dsp/fxbus.c", "dsp/master.c",
            "dsp/x0x_param.c", "seq/sequencer.c", "seq/tb3po.c", "seq/pattern.c", "seq/motion.c", "app/engine.c"]
# Explicit compilation superset. The app's backend calls, not forced export roots,
# decide which function/data sections survive the link. master_unmuted.c is an old
# component reference; the original graph uses master.c and its real lifecycle.
ORIGINAL_SYNTH_UNITS = [
    "experiments/original_synth/chicago_allocator.c",
    "experiments/original_synth/chicago_arp_control.c",
    "experiments/original_synth/chicago_control_owner.c",
    "experiments/original_synth/chicago_drive.c",
    "experiments/original_synth/chicago_dynamics.c",
    "experiments/original_synth/chicago_engine_output.c",
    "experiments/original_synth/chicago_full_mg.c",
    "experiments/original_synth/chicago_fx2_compact.c",
    "experiments/original_synth/chicago_fx2_wrapper.c",
    "experiments/original_synth/chicago_global_mg.c",
    "experiments/original_synth/chicago_global_stage.c",
    "experiments/original_synth/chicago_key_admission.c",
    "experiments/original_synth/chicago_modulation.c",
    "experiments/original_synth/chicago_mp4.c",
    "experiments/original_synth/chicago_note_context.c",
    "experiments/original_synth/chicago_output.c",
    "experiments/original_synth/chicago_packed.c",
    "experiments/original_synth/chicago_program.c",
    "experiments/original_synth/chicago_program_output.c",
    "experiments/original_synth/chicago_scheduler.c",
    "experiments/original_synth/chicago_selector.c",
    "experiments/original_synth/chicago_smallfx.c",
    "experiments/original_synth/chicago_tuning.c",
    "experiments/original_synth/chicago_voice.c",
    "experiments/original_synth/chicago_voice_owner.c",
    "experiments/original_synth/chicago_voice_snapshot.c",
    "experiments/original_synth/control_math.c",
    "experiments/original_synth/engine_events.c",
    "experiments/original_synth/fixed_fx.c",
    "experiments/original_synth/fused_reference.c",
    "experiments/original_synth/lossless_table.c",
    "experiments/original_synth/master.c",
    "experiments/original_synth/miami_control_owner.c",
    "experiments/original_synth/miami_fx_chain.c",
    "experiments/original_synth/miami_geometry.c",
    "experiments/original_synth/miami_ms20.c",
    "experiments/original_synth/miami_note_context.c",
    "experiments/original_synth/miami_output.c",
    "experiments/original_synth/miami_pitch.c",
    "experiments/original_synth/miami_program.c",
    "experiments/original_synth/miami_selected_voice.c",
    "experiments/original_synth/miami_startup.c",
    "experiments/original_synth/miami_voice_owner.c",
    "experiments/original_synth/miami_voice_snapshot.c",
    "experiments/original_synth/oscillator_motion.c",
    "experiments/original_synth/program_envelope.c",
    "experiments/original_synth/state_codec.c",
    "experiments/original_synth/voice_gain.c",
    "experiments/wobble/miami_decimator_adapter.c",
    "experiments/wobble/miami_decimator_component.c",
    "experiments/wobble/miami_fx_conversion.c",
]
ORIGINAL_SYNTH_BACKENDS = [
    "firmware/src/dsp/original_chicago_backend.c",
    "firmware/src/dsp/original_miami_backend.c",
    "firmware/src/app/original_runtime.c",
    "firmware/src/app/original_sound.c",
]
ORIGINAL_SYNTH_FLAGS = ["-fno-fast-math", "-ffp-contract=off", "-DNDEBUG",
                        "-DORIGINAL_USE_SOFT_FMA", "-DMIAMI_FX_NO_HOST_FENV",
                        "-DX0X_FAST_FMA"]   # device: mul+add for the soft FMA (docs/perf/engine-load-20261031.md)
LINE = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*\t(.*)$")

# SDK files of AC79NN_SDK_V1.2.1_2023-12-13 (the tested version)
SDK_SHA256 = {
    "uboot.boot": "4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4",
    "cfg_tool.bin": "276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b",
    "cfg/eq_cfg_hw.bin": "41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d",
}

PRODUCT = "FM-1_900"                # package identity; release builds are FM-1_9XXYYZZ (see main)
BOARD = "fm1"                       # fm1 (default) or smk37: BOARD env or --board
BOARD_PREFIX = {"fm1": "FM-1_9", "smk37": "SMK-37 Pro_9"}   # identity of a release build: prefix + XXYYZZ
BOARD_DEFS = {"fm1": [], "smk37": ["-DBOARD_SMK37=1"]}       # the FM-1 build gets no new flag (byte-identical)
# smk37: the keyboard's model name (MODEL env or --model); one hardware, three names. pro adds no flag
MODELS = {"pro": "SMK-37 Pro", "elite": "SMK-37 Elite", "mke": "MKE-P37"}
VERSION = None                      # numeric X.Y.Z passed to the firmware as X0X_VERSION
RELEASE_LABEL = None                # display only, e.g. 0.1.3_openbeta; not part of the package identity


def toolchain():
    tc = os.environ.get("JIELI_TOOLCHAIN")
    if not tc or not (Path(tc) / "pi32v2" / "bin" / "clang").exists():
        raise SystemExit("JIELI_TOOLCHAIN must point at the JieLi Linux toolchain "
                         "(the directory with pi32v2/ and common/; see tools/get_toolchain.sh)")
    return Path(tc).resolve()


def use_docker():
    native = platform.system() == "Linux" and platform.machine() in ("x86_64", "AMD64")
    return os.environ.get("JIELI_DOCKER", "0" if native else "1") == "1"


_BUILDER = None
_BUILDER_LOCK = __import__("threading").Lock()


def builder():
    """one long-lived toolchain container per source tree (starting a container per tool call cost
    most of the build); it stops by itself after an hour without a build"""
    global _BUILDER
    with _BUILDER_LOCK:            # the loader and the app build in parallel: start it once
        return _builder_start()


def _builder_start():
    global _BUILDER
    if _BUILDER:
        return _BUILDER
    name = "jieli-" + hashlib.sha1(str(SRC).encode()).hexdigest()[:10]
    st = subprocess.run(["docker", "inspect", "-f", "{{.State.Running}}", name], capture_output=True, text=True)
    if st.stdout.strip() != "true":
        subprocess.run(["docker", "rm", "-f", name], capture_output=True)
        r = subprocess.run(["docker", "run", "-d", "--rm", "--name", name, "--platform", "linux/amd64",
                            "-v", f"{SRC}:/work", "-v", f"{toolchain()}:/opt/jieli:ro", "-w", "/work",
                            DOCKER_IMAGE, "sh", "-c", "sleep 3600"], capture_output=True, text=True)
        if r.returncode:
            sys.stderr.write(r.stderr)
            raise SystemExit("build: cannot start the toolchain container")
    else:                           # an hour from now again
        subprocess.run(["docker", "exec", "-d", name, "sh", "-c", "sleep 3600 && kill 1"], capture_output=True)
    _BUILDER = name
    return name


def tc(tool, *args):
    """run a toolchain binary (pi32v2/bin/..., common/bin/...) with cwd SRC; paths relative to SRC"""
    rel = [str(Path(a).resolve().relative_to(SRC)) if isinstance(a, Path) else a for a in args]
    if tool == "cc":                # the toolchain's cc wrapper needs python3; call clang directly
        tool, rel = "pi32v2/bin/clang", ["-target", "pi32v2", *rel]
    if use_docker():
        cmd = ["docker", "exec", "-w", "/work", builder(), f"/opt/jieli/{tool}", *rel]
    else:
        cmd = [str(toolchain() / tool), *rel]
    r = subprocess.run(cmd, cwd=SRC, capture_output=True, text=True)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit(f"build: {tool} failed")
    if r.stderr.strip():
        sys.stderr.write(r.stderr)
    return r.stdout


def tc_all(*cmds):
    # at most 4 at once: a podman machine drops concurrent container connections (ssh EOF)
    with ThreadPoolExecutor(min(len(cmds), int(os.environ.get("X0X_JOBS", "4")))) as ex:
        return list(ex.map(lambda c: tc(*c), cmds))


def generate():
    """generated headers: the font (Felucca's), the 909's samples and tables, the break bank"""
    GEN.mkdir(parents=True, exist_ok=True)
    if gen_cache.restore(GEN):
        print("generate: cached font, drums, and break bank")
        return
    tools = SRC / "tools"
    cmds = [[tools / "gen_font.py", GEN / "felucca_font.h"],
            [tools / "gen_drum_samples.py", GEN / "x0x_drum_samples.h"]]
    procs = [subprocess.Popen([sys.executable, *map(str, c)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True) for c in cmds]
    failed = []
    for c, p in zip(cmds, procs):
        sys.stdout.write(p.communicate()[0])
        if p.returncode:
            failed.append(c[0].name)
    if failed:
        raise SystemExit(f"build: {', '.join(failed)} failed")
    # The default image has no built-in loops: that flash is code room. X0X_FACTORY_BREAKS=1
    # puts the designated stock loops in the image. X0X_BREAK_BANK=DIR uses another bank instead.
    import break_pack
    bank = os.environ.get("X0X_BREAK_BANK")
    factory = os.environ.get("X0X_FACTORY_BREAKS") == "1"
    if bank or factory:
        cmd = ([sys.executable, str(tools / "gen_break_bank.py"), bank, str(GEN / "x0x_break_bank.h")] if bank
               else [sys.executable, str(tools / "break_pack.py"), "--from-export",
                     str(SRC / "assets" / "factory-breaks"), "--header", str(GEN / "x0x_break_bank.h")])
        r = subprocess.run(cmd, capture_output=True, text=True)
        sys.stdout.write(r.stdout)
        if r.returncode:
            sys.stderr.write(r.stderr)
            raise SystemExit("build: break bank failed")
    else:
        break_pack.write_empty_header(GEN / "x0x_break_bank.h")
        print("break bank: none (factory loops are an optional install)")
    gen_cache.store(GEN)


# ---- update loader

def crc16(d, c=0):
    for b in d:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) if c & 0x8000 else c << 1
        c &= 0xFFFF
    return c


def ldr_head(dcrc, a, b, attr, name):
    body = struct.pack("<HIIBBH16s", dcrc, a, b, attr, 0, 0, name)
    return struct.pack("<H", crc16(body)) + body


def ldr_wrap(image):
    """ota.bin = outer JLFS head (32 B) + inner head (32 B, load address) +
    blocks [clen u32][dlen u32][LZ4 block], dlen 4096 except the last"""
    blocks = bytearray()
    for i in range(0, len(image), 4096):
        raw = image[i:i + 4096]
        c = lz4blk.compress(raw)
        if len(c) > 4096 or lz4blk.decompress(c) != raw:
            raise SystemExit(f"loader: block {i // 4096} does not compress below 4096 B")
        blocks += struct.pack("<II", len(c), len(raw)) + c
    inner = ldr_head(crc16(image), len(image), LOADER_LOAD, 0, LOADER_NAME)
    body = inner + bytes(blocks)
    return ldr_head(crc16(body), 0x20, len(body), 0x41, LOADER_NAME) + body


def build_loader():
    LDR.mkdir(parents=True, exist_ok=True)
    src = FW / "loader"
    flags = [*CFLAGS, *BOARD_DEFS[BOARD], "-Ifirmware/hal", "-Ifirmware/src", *clang_std_inc()]
    tc_all(("cc", "-c", src / "crt0_ldr.S", "-o", LDR / "crt0_ldr.o"),
           ("cc", *flags, "-c", src / "loader.c", "-o", LDR / "loader.o"))
    elf = LDR / "loader.elf"
    tc("pi32v2/bin/ld", "--gc-sections", "-e", "_start", "-T", src / "loader.ld",
       LDR / "crt0_ldr.o", LDR / "loader.o", "-o", elf)
    _, dis, hdr, syms = tc_all(("common/bin/objcopy", "-O", "binary", "-j", ".text", elf, LDR / "loader.bin"),
                               ("common/bin/objdump", "-d", elf),
                               ("common/bin/objdump", "-h", elf),
                               ("common/bin/objdump", "-t", elf))
    (LDR / "loader.dis").write_text(dis)
    for ln in hdr.splitlines():     # the loader rewrites the flash: nothing may run from XIP
        p = ln.split()
        if len(p) > 4 and p[1].startswith(".") and int(p[3], 16) >= 0x02000000 and int(p[2], 16):
            raise SystemExit(f"loader: section {p[1]} at {p[3]} is outside RAM")
    image = (LDR / "loader.bin").read_bytes()
    if not image or len(image) > 0x14000:
        raise SystemExit("loader: image empty or too big")
    ota = ldr_wrap(image)
    (LDR / "ota.bin").write_bytes(ota)
    bss = [int(ln.split()[0], 16) for ln in syms.splitlines() if ln.rstrip().endswith("_bss_end")]
    print(f"loader: image {len(image)} B at {LOADER_LOAD:#x}" + (f", bss end {bss[0]:#x}" if bss else ""))
    print(f"loader: ota.bin {len(ota)} B")
    return ota


# ---- app

def synth_build_config(environ=None):
    """Select one DSP implementation; an explicit legacy enable cannot mix them."""
    env = os.environ if environ is None else environ
    original = env.get("X0X_ORIGINAL_SYNTH", "1")
    if original != "1":
        raise SystemExit("X0X_ORIGINAL_SYNTH: only 1 is supported")
    if env.get("X0X_WOBBLE", "0") != "0":
        raise SystemExit("X0X_ORIGINAL_SYNTH=1 requires X0X_WOBBLE=0 or unset")
    if env.get("X0X_DEV_WOBBLE", "1") not in ("0", "1"):
        raise SystemExit("X0X_DEV_WOBBLE: 0 or 1 (original Miami rate-pad UI only)")
    return True, "0"


def require_original_synth_sources(root=None):
    """Fail before invoking the compiler if source or exact inputs are missing."""
    root = SRC if root is None else Path(root)
    required = [*ORIGINAL_SYNTH_UNITS, *ORIGINAL_SYNTH_BACKENDS,
                *(str(Path(u).with_suffix(".h")) for u in ORIGINAL_SYNTH_BACKENDS)]
    missing = [u for u in required if not (root / u).is_file()]
    if missing:
        raise SystemExit("X0X_ORIGINAL_SYNTH=1: missing replacement sources: " + ", ".join(missing))
    inputs = check_original_inputs.verify(
        root, expected_manifest_sha256=check_original_inputs.MANIFEST_SHA256)
    if not inputs["ok"]:
        raise SystemExit("X0X_ORIGINAL_SYNTH=1: invalid external inputs: " +
                         "; ".join(inputs["errors"]))


def app_compile_commands(flags, original_synth, wobble):
    """Return the exact app compile/link input plan without running tools."""
    if not original_synth:
        raise SystemExit("X0X_ORIGINAL_SYNTH: only 1 is supported")
    if original_synth:
        if wobble != "0":
            raise SystemExit("X0X_ORIGINAL_SYNTH=1 cannot compile legacy WBL")
        # x0x.c and engine.c use this flag to select the matching UI/factory and
        # backend code. The old factory tables are textual includes in x0x.c,
        # not standalone units that can be removed from this list.
        flags = [*flags, "-DX0X_ORIGINAL_SYNTH=1",
                 "-Iexperiments/original_synth", "-Iexperiments/wobble"]
    o2 = [f if f != "-Os" else "-O2" for f in flags]
    units = [("cc", "-c", FW / "crt0.S", "-o", OUT / "crt0.o"),
             ("cc", "-c", FW / "hal" / "fm1_vec.S", "-o", OUT / "fm1_vec.o"),
             ("cc", "-c", FW / "hal" / "fm1_isr.S", "-o", OUT / "fm1_isr.o"),
             ("cc", "-c", FW / "hal" / "fm1_cpu1.S", "-o", OUT / "fm1_cpu1.o"),
             ("cc", *[f if f != "-Os" else "-Oz" for f in flags], "-c", FW / "src" / "x0x.c", "-o", OUT / "x0x.o")]
    objs = [OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o", OUT / "fm1_cpu1.o", OUT / "x0x.o"]
    # Every admitted build is Chicago/Miami only. Keep the obsolete engine
    # units out of the compilation plan, not behind unreachable switches.
    for u in O2_UNITS:
        o = OUT / "o2" / (u.replace("/", "_")[:-2] + ".o")
        unit_flags = [*o2, *ORIGINAL_SYNTH_FLAGS] if u == "app/engine.c" else o2
        units.append(("cc", *unit_flags, "-c", FW / "src" / u, "-o", o))
        objs.append(o)
    if original_synth:
        original_flags = [*flags, *ORIGINAL_SYNTH_FLAGS]
        for u in (*ORIGINAL_SYNTH_UNITS, *ORIGINAL_SYNTH_BACKENDS):
            o = OUT / "original" / (Path(u).stem + ".o")
            units.append(("cc", *original_flags, "-c", Path(u), "-o", o))
            objs.append(o)
    return units, objs


def build_app():
    original_synth, wobble = synth_build_config()
    if original_synth:
        require_original_synth_sources()
    flags = [*CFLAGS, *FPU, *BOARD_DEFS[BOARD], "-Ifirmware/hal", "-Ifirmware/src", "-Ibuild/gen"]
    if VERSION and os.environ.get("X0X_DEBUG") == "1":
        raise SystemExit("X0X_DEBUG=1 (register peek / poke over USB) is for development builds only")
    for flag in ("X0X_CDC", "X0X_DEBUG", "X0X_DEV_PADCAL", "X0X_DEV_WOBBLE", "X0X_WOBBLE"):
        v = os.environ.get(flag)    # unset: the default in firmware headers / below
        if v in ("0", "1"):
            flags.append(f"-D{flag}={v}")
    # Sound-selector previews use tagged storage but deliberately leave the
    # separate per-note snapshot lifecycle disabled until it is qualified.
    bass_flags = ("X0X_BASS_PRESET_PREVIEW", "X0X_BASS_TAGGED_SOUND", "X0X_BASS_NOTE_SOUND")
    for flag in bass_flags:
        v = os.environ.get(flag)
        if v is not None:
            if v not in ("0", "1"):
                raise SystemExit(f"{flag}: 0 or 1")
            flags.append(f"-D{flag}={v}")
    if os.environ.get("X0X_BASS_NOTE_SOUND") == "1" and os.environ.get("X0X_BASS_TAGGED_SOUND") != "1":
        raise SystemExit("X0X_BASS_NOTE_SOUND requires X0X_BASS_TAGGED_SOUND=1")
    if os.environ.get("X0X_BASS_TAGGED_SOUND") == "1" and os.environ.get("X0X_BASS_PRESET_PREVIEW") != "1":
        raise SystemExit("X0X_BASS_TAGGED_SOUND requires X0X_BASS_PRESET_PREVIEW=1")
    # The original replacement never links Wobble DSP. X0X_DEV_WOBBLE=1
    # enables only Miami's physical Free Play rate-pad UI by default.
    if f"-DX0X_WOBBLE={wobble}" not in flags:
        flags.append(f"-DX0X_WOBBLE={wobble}")
    if not any(f.startswith("-DX0X_DEV_WOBBLE=") for f in flags):
        flags.append(f"-DX0X_DEV_WOBBLE={'1' if original_synth else wobble}")
    flags += clang_std_inc()
    flags.append(f'-DFELUCCA_ID="{PRODUCT}"')
    if BOARD == "smk37":            # SMK store owner (storage.c st_owner_check): X0X's own family, untagged data cleared
        fam = os.environ.get("FELUCCA_DATA_FAMILY", "X0X")
        if not re.fullmatch(r"[A-Z0-9]{1,7}", fam):
            raise SystemExit("FELUCCA_DATA_FAMILY: 1..7 of A-Z 0-9")
        untagged = os.environ.get("FELUCCA_DATA_UNTAGGED_OK", "0")
        if untagged not in ("0", "1"):
            raise SystemExit("FELUCCA_DATA_UNTAGGED_OK: 0 or 1")
        flags += [f'-DFELUCCA_DATA_FAMILY="{fam}"', f"-DFELUCCA_DATA_UNTAGGED_OK={untagged}"]
        baud = os.environ.get("X0X_SMK_LCD_BAUD")   # (bring-up: the LCD's SPI divider, lcd.c LCD_BAUD; default 4)
        if baud:
            if not re.fullmatch(r"[0-8]", baud):
                raise SystemExit("X0X_SMK_LCD_BAUD: 0..8 (SPI clock = 60 MHz / (n + 1))")
            flags.append(f"-DLCD_BAUD={baud}u")
    if VERSION:
        flags.append(f'-DX0X_VERSION="{VERSION}"')
    if RELEASE_LABEL:
        flags.append(f'-DX0X_RELEASE_LABEL="{RELEASE_LABEL}"')
    units, objs = app_compile_commands(flags, original_synth, wobble)
    for directory in {o.parent for o in objs}:
        directory.mkdir(parents=True, exist_ok=True)
    tc_all(*units)
    elf = OUT / "x0x.elf"
    # unreferenced functions (each in its own section: -ffunction-sections) are dropped: the DSP keeps
    # mono reference paths for the host tests that the firmware no longer calls
    ld = FW / "app.ld"
    if APP_CODE_MAX != 0x8DFBC:      # another board's app code area (SMK-37 Pro): the same script, its XIP length
        txt = ld.read_text()
        if txt.count("LENGTH = 0x8DFBC") != 1:
            raise SystemExit("app.ld: the XIP LENGTH line was not found")
        ld = OUT / "app.ld"
        ld.write_text(txt.replace("LENGTH = 0x8DFBC", f"LENGTH = {APP_CODE_MAX:#X}".replace("0X", "0x")))
    tc("pi32v2/bin/ld", "--gc-sections", "-e", "_start", "-T", ld, *objs, "-o", elf)
    for sect in ("text.bin", "data.bin", "ramtext.bin", "c1text.bin"):
        (OUT / sect).unlink(missing_ok=True)
    *_, syms, dis, rt = tc_all(("common/bin/objcopy", "-O", "binary", "-j", ".text", elf, OUT / "text.bin"),
                               ("common/bin/objcopy", "-O", "binary", "-j", ".data", elf, OUT / "data.bin"),
                               ("common/bin/objcopy", "-O", "binary", "-j", ".ram_text", elf, OUT / "ramtext.bin"),
                               ("common/bin/objcopy", "-O", "binary", "-j", ".c1_text", elf, OUT / "c1text.bin"),
                               ("common/bin/objdump", "-t", elf),
                               ("common/bin/objdump", "-d", elf),
                               ("common/bin/objdump", "-d", "-j", ".ram_text", elf))
    (OUT / "x0x.dis").write_text(dis)

    def symv(name):
        return int(re.search(r"^([0-9a-f]+) .*\s" + name + r"$", syms, re.M).group(1), 16)
    img = bytearray((OUT / "text.bin").read_bytes())
    # .ram_text, .c1_text (the second core's loop) and .data follow .text at their load addresses, in
    # that order; the start-up copies them by words
    for sect, lname in (("ramtext.bin", "_rt_load"), ("c1text.bin", "_c1_load"), ("data.bin", "_data_load")):
        load = symv(lname)
        if load % 4:
            raise SystemExit(f"{lname} {load:#x} is not word aligned")
        blob = (OUT / sect).read_bytes() if (OUT / sect).exists() else b""
        if blob:
            if load - APP_XIP < len(img):
                raise SystemExit(f"{lname} overlaps the image")
            img += b"\xff" * (load - APP_XIP - len(img))
            img += blob
    img += b"\xff" * (-len(img) % 4)
    # A 0x00 mark, then pad back to a word. The optional break pack sits after this
    # mark, in the 0xFF tail, and does not overwrite the image.
    img += b"\x00\xff\xff\xff"
    (OUT / "x0x.bin").write_bytes(img)
    return bytes(img), syms, dis, rt


def check(img, syms, dis, rt):
    errors, notes = [], []
    m = re.search(r"^([0-9a-f]+) .*\s_start$", syms, re.M)
    if not m or int(m.group(1), 16) != APP_XIP:
        errors.append(f"_start is not at {APP_XIP:#x}")
    if img[:4] != bytes.fromhex("04818000"):
        errors.append(f"image starts with {img[:4].hex()}, not the entry stub")
    rt_calls = [ln for ln in rt.splitlines() if re.search(r"\bcall\b", ln)]
    if rt_calls:                    # RAM code runs with the flash off: no calls into XIP
        errors.append(f".ram_text contains calls: {rt_calls[:3]}")
    else:
        notes.append(f".ram_text: {len([ln for ln in rt.splitlines() if LINE.match(ln)])} insns, no calls")
    errors.extend(rom_guard.references(dis))  # no ROM calls, loads or escaping ROM addresses
    if len(img) > APP_CODE_MAX:
        errors.append(f"image {len(img)} B exceeds the app code area ({APP_CODE_MAX} B)")
    else:
        notes.append(f"app code area {APP_CODE_MAX} B, {APP_CODE_MAX - len(img)} B free"
                     + (f" (Bluetooth block at app.bin {APP_CODE_MAX:#x})" if APP_CODE_MAX < APP_SLOT else ""))
    # X0X: the FPU is single precision; a soft-double routine in the image means a double crept in
    soft = sorted(set(re.findall(r"\s(__(?:add|sub|mul|div|neg|cmp|eq|ne|lt|le|gt|ge|un|extendsf|truncdf|fixdf|fixunsdf|floatsidf|floatunsidf|floatdidf)\w*df\w*)$", syms, re.M)))
    if soft:
        errors.append(f"soft-double routines linked: {soft[:6]} (a double in the code)")
    else:
        notes.append("no soft-double routines (single-precision FPU only)")

    def sym(name):
        mm = re.search(r"^([0-9a-f]+) .*\s" + name + r"$", syms, re.M)
        return int(mm.group(1), 16) if mm else 0
    bss = sym("_bss_end") - 0x01C08000
    pool = sym("_pool_end") - sym("_pool_start")
    notes.append(f"image {len(img)} B; RAM .data+.bss {bss} B of 98304; pool {pool} B of {0x54000}")
    if bss > 96 * 1024:
        errors.append("RAM region overflow")
    if 0x54000 - pool < 8192:                     # keep >= 8 KiB of the pool spare
        errors.append(f"pool headroom {0x54000 - pool} B < 8192 B")
    return errors, notes


# Every hardware register access lives in hal/. In src/ and loader/ (comments stripped):
#  - no volatile pointer cast, except of a C object's address (`*(volatile T *)&x`: a read-once
#    of a RAM flag shared with an ISR, e.g. usb.c ota_wire_send);
#  - no literal in a register or reserved window (core SFRs 0x10000-0x13FFF, SFC 0x40000-0x43FFF,
#    GPIO/IOMAP 0x50000-0x51FFF, CPU 0x1EE0000-0x1EEFFFF, RAM top 0x01C7F000- (boot info,
#    mailbox, vectors), XIP 0x02000000-0x020FFFFF);
#  - no inline asm, except the empty compiler barrier RING_PUBLISH() (emits no instruction);
#  - no HAL register macro (a hal/ #define that is, or expands to, a volatile access).
# Linker symbols (_bss_start[], _rt_load[], ...) are plain C objects and pass.
MMIO_LIT = re.compile(r"\b0x0*(1[0-3][0-9a-f]{3}|4[0-3][0-9a-f]{3}|5[01][0-9a-f]{3}|1ee[0-9a-f]{4}|"
                      r"1c7f[0-9a-f]{3}|20[0-9a-f]{5})u?l?\b", re.I)
MMIO_CAST = re.compile(r"\(\s*(?:const\s+)?volatile\b[^()]*\*\s*\)(?!\s*&)")
MMIO_ASM = re.compile(r"\b(?:__asm__|asm)\b(?!\s+volatile\s*\(\s*\"\"\s*:::\s*\"memory\"\s*\))")


def mmio_check():
    def strip(s):
        s = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), s, flags=re.S)
        return re.sub(r"//[^\n]*", "", s)
    defs = {}
    for h in sorted((FW / "hal").glob("*.h")):
        for m in re.finditer(r"^#define\s+(\w+)(?:\([^)]*\))?\s+(.*)$", strip(h.read_text()), re.M):
            defs[m.group(1)] = m.group(2)
    regs = {n for n, b in defs.items() if re.search(r"\bvolatile\b", b)}
    while True:                                   # macros built on register macros (FM1_WR_LIMIT_H -> FM1_X2)
        more = {n for n, b in defs.items() if n not in regs and set(re.findall(r"\w+", b)) & regs}
        if not more:
            break
        regs |= more
    errors = []
    for f in sorted([*(FW / "src").rglob("*.[ch]"), *(FW / "loader").glob("*.c")]):
        for no, ln in enumerate(strip(f.read_text()).splitlines(), 1):
            where = f"{f.relative_to(FW)}:{no}"
            for rx, what in ((MMIO_LIT, "register/window address"), (MMIO_CAST, "volatile pointer cast"),
                             (MMIO_ASM, "inline asm")):
                m = rx.search(ln)
                if m:
                    errors.append(f"{where}: {what} outside hal/ ({m.group(0).strip()}); add a hal/ helper")
            used = set(re.findall(r"\b[A-Z_][A-Z0-9_]*\b", ln)) & regs
            if used:
                errors.append(f"{where}: HAL register macro {sorted(used)[0]} outside hal/; use a hal/ helper")
    return errors


def release_identity(text):
    """Split a --release argument into machine identity and open-beta display.

    The package identity uses only X.Y.Z, each component 0..99, as model_9XXYYZZ.
    `_openbeta` is a display label. It is not accepted as a version argument and
    it is not encoded in the product name. A different -suffix keeps the older
    on-device version string and gets no open-beta label.
    """
    match = re.fullmatch(r"(\d{1,2})\.(\d{1,2})(?:\.(\d{1,2}))?(-[A-Za-z0-9]+)?", text or "")
    if not match:
        raise ValueError("use X.Y[.Z][-suffix], up to two digits each")
    major, minor, patch = int(match[1]), int(match[2]), int(match[3] or 0)
    numeric = "%d.%d.%d" % (major, minor, patch)
    digits = "%02d%02d%02d" % (major, minor, patch)
    channel = match.group(4)
    if channel in (None, "-openbeta"):
        return {"numeric": numeric, "digits": digits, "version_macro": numeric,
                "release_label": numeric + "_openbeta", "filename": numeric}
    shown = text.upper() if "BETA" in text.upper() else text.upper() + " BETA"
    return {"numeric": numeric, "digits": digits, "version_macro": shown,
            "release_label": None, "filename": text}


def main():
    global PRODUCT, VERSION, RELEASE_LABEL, BOARD, APP_SLOT, APP_CODE_MAX, OUT, LDR
    ap = argparse.ArgumentParser()
    ap.add_argument("--release", metavar="X.Y[.Z]", help="release build: identity FM-1_9XXYYZZ, version string X.Y[.Z]")
    ap.add_argument("--board", choices=sorted(BOARD_PREFIX), default=None,
                    help="target board: fm1 (default) or smk37 (default: $BOARD, else fm1)")
    ap.add_argument("--model", choices=sorted(MODELS), default=None,
                    help="smk37: the model name, pro (default), elite or mke (default: $MODEL, else pro)")
    ap.add_argument("--sdk", type=Path, help="JieLi AC79 SDK checkout (default: $AC79_SDK)")
    a = ap.parse_args()
    original_synth, _ = synth_build_config()
    if original_synth:
        require_original_synth_sources()
    # regression guard: hardware-relevant settings must match tools/locked_settings.json
    if subprocess.run([sys.executable, str(SRC / "tools" / "check_locked_settings.py")]).returncode:
        raise SystemExit("build.py: locked settings differ (see above); the build stops")
    BOARD = (a.board or os.environ.get("BOARD") or "fm1").lower().replace("-", "")
    if BOARD not in BOARD_PREFIX:
        raise SystemExit(f"BOARD={BOARD}: use fm1 or smk37")
    model = (a.model or (os.environ.get("MODEL") if BOARD == "smk37" else None) or "pro").lower()
    if model not in MODELS or (model != "pro" and BOARD != "smk37"):
        raise SystemExit(f"MODEL={model}: pro, elite or mke, with BOARD=smk37")
    fm1pkg_make.set_board(BOARD)
    APP_SLOT = fm1pkg_make.APP_SLOT
    APP_CODE_MAX = fm1pkg_make.APP_CODE_MAX
    stem = "x0x" if BOARD == "fm1" else "x0x-" + BOARD
    pkg_dir = OUT                   # the package always in build/; another board's objects, app and loader in
    if BOARD != "fm1":              # build/<board>/, so the FM-1 outputs (and the tests on them) stay as they are
        OUT = pkg_dir / BOARD
        LDR = OUT / "loader"
    PRODUCT = fm1pkg_make.BOARDS[BOARD][2]
    if model != "pro":              # another model name: identity, loader and update record (fm1_cc.h SMK_MODEL)
        BOARD_DEFS[BOARD] = [*BOARD_DEFS[BOARD], f'-DSMK_MODEL="{MODELS[model]}"']
        PRODUCT = PRODUCT.replace(MODELS["pro"], MODELS[model])
        BOARD_PREFIX[BOARD] = BOARD_PREFIX[BOARD].replace(MODELS["pro"], MODELS[model])
        stem += "-" + model
        OUT = OUT.parent / f"{BOARD}-{model}"     # its own objects: the Pro build's stay as they are
        LDR = OUT / "loader"
    name = f"{stem}.fwsc"
    RELEASE_LABEL = None
    if a.release:
        # identity FM-1_9 + major, minor, patch as two digits each (0.10 -> FM-1_9001000). FM-1 is the
        # model the update tools match on; the number is ours (X0X 0.3..0.9 were FM-1_9XY, one digit
        # each, which ran out at 0.9 and was also Felucca's own numbering). Digits only after the
        # underscore. The open-beta words stay in X0X_RELEASE_LABEL, not in this identity.
        try:
            ident = release_identity(a.release)
        except ValueError as exc:
            raise SystemExit(f"--release {a.release}: {exc}") from exc
        PRODUCT = BOARD_PREFIX[BOARD] + ident["digits"]
        VERSION = ident["version_macro"]
        RELEASE_LABEL = ident["release_label"]
        name = f"{stem}-{ident['filename']}.fwsc"
    fm1pkg_make.SDK = a.sdk
    for rel, sha in SDK_SHA256.items():          # fail early without the SDK
        if hashlib.sha256(fm1pkg_make.sdk_file(rel)).hexdigest() != sha:
            print(f"warning: SDK {rel} differs from AC79NN_SDK_V1.2.1; the package will not match the reference")
    OUT.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(2) as ex:
        gen, ldr = ex.submit(generate), ex.submit(build_loader)
        gen.result()
        ota = ldr.result()
    img, syms, dis, rt = build_app()
    errors, notes = check(img, syms, dis, rt)
    hal_err = mmio_check()
    errors += hal_err
    if not hal_err:
        notes.append("register access: hal/ only (src/, loader/ clean)")
    for n in notes:
        print("  ok   ", n)
    for e in errors:
        print("  FAIL ", e)
    if errors:
        raise SystemExit("build: checks failed")
    pkg = fm1pkg_make.ufw(fm1pkg_make.flash_image(img, fm1pkg_make.KEY), ota, PRODUCT)
    (pkg_dir / name).write_bytes(pkg)
    print(f"app      {OUT / 'x0x.bin'}  {len(img)} B")
    print(f"loader   {LDR / 'ota.bin'}  {len(ota)} B")
    print(f"package  {pkg_dir / name}  {len(pkg)} B, identity {PRODUCT}" + ("" if BOARD == "fm1" else f", board {BOARD}"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
