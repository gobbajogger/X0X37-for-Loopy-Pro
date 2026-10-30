#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local host check for Stage-7 installer dual-digest evidence (FP-001).

Nightly prepare:candidate / validateCatalog keep using qualification.installer_sha256
as the source composite from installerFingerprint(). That value is NOT the sha256 of
the served install/index.html. Stage evidence must record both digests by name and
must not claim a single "Installer FP" equals both.

This check fails when:
- required dual fields are missing or malformed
- evidence still uses a conflated single pin for both meanings
- the two named digests are identical (copy-paste conflation)
- a fetched (or provided) install HTML digest ≠ served_install_html_sha256

It does not change package, controller, or firmware bytes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

HEX64 = re.compile(r"^[0-9a-f]{64}$")
SOURCE_KEYS = ("installer_source_fingerprint",)
SERVED_KEYS = ("served_install_html_sha256",)
# Legacy / conflated names that must not stand alone as the dual record.
CONFLATED_KEYS = (
    "installer_fp",
    "Installer FP",
    "installer_fingerprint",
)
DEFAULT_INSTALL_URL = (
    "https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/"
)


class DigestCheckError(ValueError):
    """Evidence digests are missing, conflated, or do not match fetched HTML."""


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_evidence(path: Path) -> dict:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise DigestCheckError(f"unreadable evidence JSON: {error}") from error
    if not isinstance(raw, dict):
        raise DigestCheckError("evidence must be a JSON object")
    return raw


def _hex_field(evidence: dict, keys: tuple[str, ...], label: str) -> str:
    present = [k for k in keys if k in evidence]
    if len(present) != 1:
        raise DigestCheckError(
            f"evidence must define exactly one {label} field "
            f"({', '.join(keys)}); found {present or 'none'}"
        )
    value = evidence[present[0]]
    if not isinstance(value, str) or not HEX64.fullmatch(value):
        raise DigestCheckError(f"{present[0]} must be a lowercase 64-hex SHA-256")
    return value


def detect_conflation(evidence: dict) -> None:
    """Reject single-pin / legacy shapes that treat both digests as one value."""
    # qualification.installer_sha256 alone is the nightly source pin — allowed only
    # when dual fields are also present (alias documented, not a substitute).
    has_source = any(k in evidence for k in SOURCE_KEYS)
    has_served = any(k in evidence for k in SERVED_KEYS)
    for key in CONFLATED_KEYS:
        if key in evidence and not (has_source and has_served):
            raise DigestCheckError(
                f"conflated field {key!r} without separate "
                "installer_source_fingerprint and served_install_html_sha256"
            )
    if "installer_sha256" in evidence and not (has_source and has_served):
        raise DigestCheckError(
            "installer_sha256 alone conflates the nightly source fingerprint with "
            "served HTML; record installer_source_fingerprint and "
            "served_install_html_sha256 separately "
            "(installer_sha256 may remain as an alias of the source fingerprint)"
        )
    # Explicit claim that one pin equals both artifacts.
    claim = evidence.get("installer_fp_equals_both") or evidence.get(
        "single_installer_fp_for_source_and_html"
    )
    if claim is True:
        raise DigestCheckError(
            "evidence claims a single Installer FP for source fingerprint and "
            "served HTML"
        )
    label = evidence.get("installer_fp_label")
    if isinstance(label, str) and label.strip().lower() in {
        "installer fp",
        "installer_fp",
        "both",
        "source_and_html",
    }:
        if evidence.get("installer_source_fingerprint") == evidence.get(
            "served_install_html_sha256"
        ):
            raise DigestCheckError(
                "Installer FP label used while source and served digests are equal"
            )


def fetch_install_html(url: str, opener=None) -> bytes:
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "x0x37-check-installer-digests/1"},
        method="GET",
    )
    open_url = opener or urllib.request.urlopen
    try:
        with open_url(req, timeout=60) as resp:
            status = getattr(resp, "status", None) or resp.getcode()
            if status != 200:
                raise DigestCheckError(f"install URL HTTP {status}")
            data = resp.read()
    except DigestCheckError:
        raise
    except (urllib.error.URLError, TimeoutError, OSError) as error:
        raise DigestCheckError(f"install URL fetch failed: {error}") from error
    if not data:
        raise DigestCheckError("install URL returned empty body")
    return data


def check_evidence(
    evidence: dict,
    *,
    fetch_url: str | None = None,
    html_bytes: bytes | None = None,
    expect_source: str | None = None,
) -> dict:
    """Validate dual-digest evidence. Returns a result dict on success."""
    detect_conflation(evidence)
    source = _hex_field(evidence, SOURCE_KEYS, "source fingerprint")
    served = _hex_field(evidence, SERVED_KEYS, "served install HTML")
    if source == served:
        raise DigestCheckError(
            "installer_source_fingerprint must not equal served_install_html_sha256 "
            "(conflation / copy-paste of one pin into both fields)"
        )
    # Nightly alias: if present, must match the source fingerprint only.
    alias = evidence.get("installer_sha256")
    if alias is not None:
        if not isinstance(alias, str) or not HEX64.fullmatch(alias):
            raise DigestCheckError(
                "installer_sha256 alias must be a lowercase 64-hex SHA-256"
            )
        if alias != source:
            raise DigestCheckError(
                "installer_sha256 alias must equal installer_source_fingerprint "
                "(prepare:candidate / validateCatalog source-fingerprint semantics)"
            )
    if expect_source is not None:
        if not HEX64.fullmatch(expect_source):
            raise DigestCheckError("expect_source must be lowercase 64-hex")
        if source != expect_source:
            raise DigestCheckError(
                "installer_source_fingerprint does not match expect_source"
            )

    fetched = None
    if html_bytes is not None:
        fetched = sha256_bytes(html_bytes)
    elif fetch_url:
        fetched = sha256_bytes(fetch_install_html(fetch_url))
    if fetched is not None and fetched != served:
        raise DigestCheckError(
            "served_install_html_sha256 does not match fetched install HTML "
            f"(recorded={served} fetched={fetched})"
        )

    return {
        "ok": True,
        "installer_source_fingerprint": source,
        "served_install_html_sha256": served,
        "fetched_install_html_sha256": fetched,
        "install_url": fetch_url,
    }


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Check Stage evidence records installer_source_fingerprint and "
            "served_install_html_sha256 separately; optionally verify served HTML."
        )
    )
    parser.add_argument(
        "evidence",
        type=Path,
        help="JSON evidence with dual digests (see evidence/f07-stage2/digests.json)",
    )
    parser.add_argument(
        "--fetch",
        nargs="?",
        const=DEFAULT_INSTALL_URL,
        default=None,
        metavar="URL",
        help=(
            "Fetch install HTML and require sha256 == served_install_html_sha256 "
            f"(default URL {DEFAULT_INSTALL_URL})"
        ),
    )
    parser.add_argument(
        "--html-file",
        type=Path,
        help="Local install/index.html bytes instead of --fetch",
    )
    parser.add_argument(
        "--expect-source",
        help="Optional expected installer_source_fingerprint (qualification pin)",
    )
    parser.add_argument(
        "--json-out",
        type=Path,
        help="Write the result object as JSON",
    )
    args = parser.parse_args(argv)

    try:
        evidence = load_evidence(args.evidence)
        html_bytes = None
        if args.html_file is not None:
            html_bytes = args.html_file.read_bytes()
        result = check_evidence(
            evidence,
            fetch_url=args.fetch,
            html_bytes=html_bytes,
            expect_source=args.expect_source,
        )
    except DigestCheckError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1

    line = (
        f"PASS dual-digest source={result['installer_source_fingerprint']} "
        f"served_html={result['served_install_html_sha256']}"
    )
    if result["fetched_install_html_sha256"]:
        line += f" fetched={result['fetched_install_html_sha256']}"
    print(line)
    if args.json_out is not None:
        args.json_out.write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
