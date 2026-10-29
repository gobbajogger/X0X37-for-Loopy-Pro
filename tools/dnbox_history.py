#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the DnBOX firmware history from the *existing* catalog and hosted bytes.

This is read-only. It never creates a version, changes a package or decides flash eligibility.
"""
from __future__ import annotations

import hashlib
import html
import re
from pathlib import Path
from urllib.parse import quote


def safe_local_name(entry: dict) -> str | None:
    address = entry.get("pkg", "")
    if not isinstance(address, str) or not address.startswith("../firmware/"):
        return None
    name = address.removeprefix("../firmware/")
    if not name or name in (".", "..") or "/" in name or "\\" in name or name.startswith("."):
        return None
    return name


def verified_package(artifact: Path, expected: str, size: int) -> bool:
    if not re.fullmatch(r"[0-9a-f]{64}", str(expected).lower()):
        return False
    if not isinstance(size, int) or isinstance(size, bool) or size <= 0 or size > 16 * 1024 * 1024:
        return False
    if not artifact.is_file() or artifact.is_symlink() or artifact.stat().st_size != size:
        return False
    return hashlib.sha256(artifact.read_bytes()).hexdigest() == expected.lower()


def render_history(out: Path, catalog: dict, style: str = "") -> str:
    lines = []
    versions = catalog.get("versions", []) if isinstance(catalog, dict) else []
    for version in versions if isinstance(versions, list) else []:
        if not isinstance(version, dict):
            continue
        title = html.escape(str(version.get("version", "Unlabelled")))
        label = html.escape(str(version.get("label", "")))
        rows = []
        models = version.get("models", {})
        for model, package in (models.items() if isinstance(models, dict) else []):
            if not isinstance(package, dict):
                continue
            name = safe_local_name(package)
            digest = str(package.get("sha256", "")).lower()
            size = package.get("size")
            valid = bool(name and verified_package(out / "firmware" / name, digest, size))
            model_label = html.escape(str(model))
            sha = html.escape(digest if re.fullmatch(r"[0-9a-f]{64}", digest) else "unverified")
            if valid:
                link = f'<a href="../firmware/{quote(name)}" download>Package bytes verified</a>'
            else:
                link = '<span>Package unavailable or checksum mismatch; not offered for download</span>'
            rows.append(f'<li><strong>{model_label}</strong>: {link} '
                        f'<small>SHA-256: <code>{sha}</code></small></li>')
        if not rows:
            rows = ['<li>No packages are recorded for this version.</li>']
        lines.append(f'<section class="dnbox-workspace"><h2>Version {title}</h2><p>{label}</p>'
                     f'<ul>{"".join(rows)}</ul></section>')
    if not lines:
        lines.append('<p>No verified firmware versions are recorded in this site snapshot. '
                     'You may inspect a local .fwsc on the Install page; no published package is inferred.</p>')
    return ("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>DnBOX · Firmware history</title><style>" + style + "</style></head>"
            "<body><main><nav class=\"dnbox-site-nav\" aria-label=\"DnBOX\">"
            "<a class=\"dnbox-brand\" href=\"../\">DnBOX</a>"
            "<a href=\"../install/\">Install</a><a href=\"../breaks/\">Samples</a>"
            "<a href=\"../projects/\">Projects</a><a href=\"../manual/\">Manual</a>"
            "<a href=\"../new/\">What's new</a></nav>"
            "<h1>Firmware history</h1>"
            "<p>Only packages whose exact hosted bytes match the catalog size and SHA-256 have a "
            "download link. A checksum match does not grant device suitability or permission to flash; "
            "the installer applies its separate model and publication checks.</p>"
            + "".join(lines) + "</main></body></html>\n")


def write_history(out: Path, catalog: dict, css_path: Path) -> None:
    dest = out / "history"
    dest.mkdir(parents=True, exist_ok=True)
    css = css_path.read_text(encoding="utf-8")
    (dest / "index.html").write_text(render_history(out, catalog, css), encoding="utf-8")