#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect real X0X37 .fwsc identity and produce non-release build labels.

A package version exists only if it is encoded in the actual product identity
(model_9XXYYZZ). The older model_900 identity is *unversioned*. A timestamp,
Git SHA or archive filename is never a firmware version.

This tool inspects package header/file CRCs and the update loader; it does not
validate source/build inputs, certify DEV CHECKED, authorize installation,
or establish physical-device proof.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

from fm1_install import LOADER_MARK, deinterleave, package_damage, product_of

MODELS = {
    "pro": "SMK-37 Pro",
    "elite": "SMK-37 Elite",
    "mke": "MKE-P37",
    "fm1": "FM-1",
}
VERSION = re.compile(r"(0|[1-9][0-9]?)\.(0|[1-9][0-9]?)\.(0|[1-9][0-9]?)\Z")
COMMIT = re.compile(r"[0-9a-f]{40}\Z")
DISPLAY_SUFFIX = "_openbeta"


def canonical_version(value):
    """Return canonical X.Y.Z (each component 0..99) or refuse it.

    A channel display such as 0.1.3_openbeta is not a machine version.
    """
    if isinstance(value, str) and value.endswith(DISPLAY_SUFFIX):
        raise ValueError("channel display is not a firmware version")
    if not isinstance(value, str) or not VERSION.fullmatch(value):
        raise ValueError("firmware version must be canonical X.Y.Z (components 0..99)")
    return value


def release_display(version):
    """Readable open-beta label. The suffix is not part of the package identity."""
    return canonical_version(version) + DISPLAY_SUFFIX


def version_of_product(product, model):
    """Find actual numeric identity, not the build's name, time or revision."""
    if model not in MODELS:
        raise ValueError("unsupported model: " + str(model))
    if not isinstance(product, str):
        raise ValueError("firmware product must be text")
    prefix = MODELS[model] + "_"
    if not product.startswith(prefix):
        raise ValueError("firmware model does not match expected " + MODELS[model])
    value = product[len(prefix):]
    if value == "900" or (model == "fm1" and re.fullmatch(r"9\d{2}", value)):
        return None  # Historic identity has no unambiguous numeric version.
    m = re.fullmatch(r"9(\d{2})(\d{2})(\d{2})", value)
    if m is None:
        raise ValueError("unsupported package identity: " + repr(product))
    return ".".join(str(int(part)) for part in m.groups())


def inspect_package(path, model, source_commit, expected_version=None):
    """Return factual package metadata; never infer a release or device PASS."""
    if not isinstance(source_commit, str) or not COMMIT.fullmatch(source_commit):
        raise ValueError("source_commit must be a full 40-character lowercase Git SHA")
    if expected_version is not None:
        canonical_version(expected_version)
    raw = Path(path).read_bytes()
    product = product_of(raw)  # refuses truncated marker section
    actual_version = version_of_product(product, model)
    if expected_version is not None and actual_version != expected_version:
        raise ValueError("expected firmware v" + expected_version + ", package contains "
                         + ("v" + actual_version if actual_version else "no numeric version"))
    if LOADER_MARK not in raw:
        raise ValueError("package lacks the X0X update-loader marker")
    logical, _ = deinterleave(raw)
    damage = package_damage(logical)
    if damage:
        raise ValueError("firmware package integrity error: " + damage)
    package_sha = hashlib.sha256(raw).hexdigest()
    label = "v" + actual_version if actual_version is not None else "Unversioned legacy DEV"
    build_id = "g" + source_commit[:12] + "-h" + package_sha[:12]
    return {
        "firmware_version": actual_version,
        "release_display": None if actual_version is None else release_display(actual_version),
        "version_label": label,
        "build_id": build_id,
        "display_label": "X0X37 " + label + " · " + MODELS[model] + " · DEV · DEVICE NOT TESTED",
        "model": model,
        "product": product,
        "source_commit": source_commit,
        "package_sha256": package_sha,
        "package_bytes": len(raw),
        "status": "package-inspected",
        "device_tested": False,
        "installable": False,
    }


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("package", type=Path, help="actual .fwsc package file")
    p.add_argument("--model", required=True, choices=sorted(MODELS))
    p.add_argument("--source-commit", required=True, help="exact 40-character Git source commit")
    p.add_argument("--expected-version", help="fail if actual product identity disagrees")
    p.add_argument("--out", type=Path, help="write JSON record here instead of stdout")
    a = p.parse_args(argv)
    try:
        result = inspect_package(a.package, a.model, a.source_commit, a.expected_version)
    except (ValueError, OSError) as exc:
        print("REFUSED: " + str(exc), file=sys.stderr)
        return 3
    content = json.dumps(result, indent=2) + "\n"
    if a.out:
        a.out.write_text(content, encoding="utf-8")
    else:
        sys.stdout.write(content)
    return 0


if __name__ == "__main__":
    sys.exit(main())
