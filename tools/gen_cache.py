#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reuse the headers tools/build.py generates when their inputs are unchanged.

A nightly build renders the font, the 909 samples, and the built-in break bank
before it compiles. Those outputs follow the generators, the assets, the 909
sources, and the host compiler. They do not follow the rest of the firmware.
The cache key is a content hash of those inputs. A changed byte misses.
Set X0X_GEN_CACHE=0 to generate every time. Any other value is the cache directory.
"""
import hashlib
import os
import re
import shutil
import subprocess
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
HEADERS = ("felucca_font.h", "x0x_drum_samples.h", "x0x_drum_tables.h", "x0x_break_bank.h")
GENERATED = set(HEADERS)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
SEARCH = (SRC / "firmware" / "src", SRC / "firmware" / "src" / "dsp")
TOOLS = (
    "tools/gen_font.py",
    "tools/gen_drum_samples.py",
    "tools/gen_break_bank.py",
    "tools/gen_builtin_break.sh",
    "tools/gen_builtin_break.c",
    "tools/sampleio.py",
    "tools/break_pack.py",
)
ROOTS = (
    SRC / "tools" / "gen_builtin_break.c",
    SRC / "firmware" / "src" / "dsp" / "drum909.c",
)


def cache_root():
    """None disables the cache. An empty X0X_GEN_CACHE uses the default directory."""
    value = os.environ.get("X0X_GEN_CACHE")
    if value == "0":
        return None
    if value:
        return Path(value)
    return Path.home() / ".cache" / "x0x-gen"


def _add(hasher, label, data):
    hasher.update(label.encode())
    hasher.update(b"\0")
    hasher.update(data)
    hasher.update(b"\0")


def _add_file(hasher, path):
    path = Path(path)
    hasher.update(str(path).encode())
    hasher.update(b"\0")
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            hasher.update(chunk)
    hasher.update(b"\0")


def _sources():
    """Local headers the break-bank compiler reads, plus the generators."""
    found = []
    stack = [path for path in ROOTS if path.is_file()]
    while stack:
        path = stack.pop()
        if path in found or path.name in GENERATED or not path.is_file():
            continue
        found.append(path)
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for name in INCLUDE.findall(text):
            for base in (path.parent, *SEARCH):
                candidate = base / name
                if candidate.is_file() and candidate.name not in GENERATED:
                    stack.append(candidate)
                    break
    for rel in TOOLS:
        path = SRC / rel
        if path.is_file() and path not in found:
            found.append(path)
    return found


def _font_files():
    files = []
    dirs = [SRC / "assets" / "fonts"]
    dirs += [Path(p) for p in os.environ.get("X0X_FONT_DIRS", "").split(":") if p]
    for directory in dirs:
        if not directory.is_dir():
            continue
        for path in sorted(directory.iterdir()):
            if path.suffix.lower() in {".bdf", ".ttf", ".otf", ".ttc"} and path.is_file():
                files.append(path)
    return files


def _wavs(directory):
    directory = Path(directory)
    if not directory.is_dir():
        return []
    return sorted(path for path in directory.rglob("*") if path.is_file() and path.suffix.lower() == ".wav")


def _font_renderer_inputs():
    """Version the libraries which turn font bytes into compiled glyph headers.

    A source/assets hash alone cannot distinguish Pillow, FreeType or libraqm
    changes. If this runtime cannot be identified, bypass rather than reuse
    an incompatible generated-header cache entry.
    """
    try:
        import PIL
        from PIL import features
        raqm = bool(features.check("raqm"))
        return {
            "pillow": PIL.__version__,
            "freetype2": features.version_module("freetype2") or "unavailable",
            "raqm-enabled": "1" if raqm else "0",
            "raqm-version": (features.version_feature("raqm") or "unavailable") if raqm else "disabled",
        }
    except (ImportError, AttributeError, ValueError, RuntimeError) as exc:
        raise OSError("cannot identify Pillow font renderer for generated cache") from exc


def fingerprint():
    """Hash generate() inputs: compiler, assets, source, and font renderer."""
    hasher = hashlib.sha256()
    for label, value in sorted(_font_renderer_inputs().items()):
        _add(hasher, "font-renderer-" + label, str(value).encode())
    for key in ("X0X_FONTSET", "X0X_FONT_DIRS", "X0X_BREAK_BANK", "X0X_FACTORY_BREAKS", "CC"):
        _add(hasher, key, os.environ.get(key, "").encode())
    mode = "bank" if os.environ.get("X0X_BREAK_BANK") else (
        "factory" if os.environ.get("X0X_FACTORY_BREAKS") == "1" else "none")
    _add(hasher, "break-bank-mode", mode.encode())
    cc = os.environ.get("CC", "cc")
    resolved = shutil.which(cc) or cc
    _add(hasher, "cc-path", resolved.encode())
    version = subprocess.run([cc, "--version"], capture_output=True, text=True, timeout=30)
    if version.returncode != 0:
        raise OSError("host compiler did not report its version")
    _add(hasher, "cc-version", (version.stdout + version.stderr).encode())
    for path in _sources():
        _add_file(hasher, path)
    for path in _font_files():
        _add_file(hasher, path)
    for path in _wavs(SRC / "assets" / "909"):
        _add_file(hasher, path)
    bank = os.environ.get("X0X_BREAK_BANK")
    if bank:
        for path in sorted(p for p in Path(bank).rglob("*") if p.is_file()):
            _add_file(hasher, path)
    return hasher.hexdigest()


def _digest(directory):
    """Content digest. The directory path is not part of it, so a renamed entry still matches."""
    hasher = hashlib.sha256()
    for name in HEADERS:
        path = directory / name
        if not path.is_file() or path.stat().st_size == 0:
            return ""
        hasher.update(name.encode())
        hasher.update(b"\0")
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1 << 20), b""):
                hasher.update(chunk)
        hasher.update(b"\0")
    return hasher.hexdigest()


def _ready(directory):
    done = directory / "DONE"
    try:
        return done.is_file() and done.read_text(encoding="utf-8").strip() == _digest(directory)
    except OSError:
        return False


def restore(gen_dir):
    """Copy a complete cache entry into gen_dir. A miss or a damaged entry returns False."""
    root = cache_root()
    if root is None:
        return False
    try:
        source = root / fingerprint()
    except (OSError, subprocess.SubprocessError):
        return False
    if not _ready(source):
        return False
    gen_dir = Path(gen_dir)
    gen_dir.mkdir(parents=True, exist_ok=True)
    for name in HEADERS:
        shutil.copyfile(source / name, gen_dir / name)
    if _digest(gen_dir) != (source / "DONE").read_text(encoding="utf-8").strip():
        return False
    return True


def store(gen_dir):
    """Save gen_dir when it holds every header. Leave an existing good entry alone."""
    root = cache_root()
    if root is None:
        return
    gen_dir = Path(gen_dir)
    if any(not (gen_dir / name).is_file() for name in HEADERS):
        return
    try:
        key = fingerprint()
    except (OSError, subprocess.SubprocessError):
        return
    dest = root / key
    if _ready(dest):
        return
    root.mkdir(parents=True, exist_ok=True)
    temporary = root / f".{key}.{os.getpid()}.tmp"
    if temporary.exists():
        shutil.rmtree(temporary)
    temporary.mkdir()
    try:
        for name in HEADERS:
            shutil.copyfile(gen_dir / name, temporary / name)
        (temporary / "DONE").write_text(_digest(temporary) + "\n", encoding="utf-8")
        if dest.exists() and not _ready(dest):
            shutil.rmtree(dest)
        if _ready(dest):
            shutil.rmtree(temporary)
            return
        os.rename(temporary, dest)
    except Exception:
        shutil.rmtree(temporary, ignore_errors=True)
        if not _ready(dest):
            raise
