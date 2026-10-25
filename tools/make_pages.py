#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the X0X site for GitHub Pages
(https://gobbajogger.github.io/X0X37-for-Loopy-Pro/):

  index.html                  landing: install, breaks, projects, manual, source
  install/index.html          web installer with version + model pickers (Web MIDI)
  breaks/index.html           sample importer for break loops
  projects/index.html         project save/load/reset and portable files (JS modules beside it)
  manual/, new/               from docs/MANUAL.md and docs/NEW.md
  firmware/*.fwsc             packages for every published version/model kept on the site
  versions.json               catalog the installer loads (also inlined into install/)

Legacy (single package):
  tools/make_pages.py build/x0x-0.1-beta.fwsc 0.1-beta OUT_DIR

Multi-model SMK release (keeps older versions when --merge points at an existing site):
  tools/make_pages.py --version 0.2 --out OUT_DIR \\
      --pro build/x0x-smk37-0.2.fwsc \\
      --elite build/x0x-smk37-elite-0.2.fwsc \\
      --mke build/x0x-smk37-mke-0.2.fwsc \\
      [--merge EXISTING_SITE]

Hostable shell (no packages yet; drop-.fwsc + Releases API still work):
  tools/make_pages.py --shell --out OUT_DIR [--merge EXISTING_SITE]
"""
from __future__ import annotations

import argparse
import hashlib
import html
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
from make_site import product_of, strip_module  # noqa: E402

REPO = "https://github.com/gobbajogger/X0X37-for-Loopy-Pro"
PAGES = "https://gobbajogger.github.io/X0X37-for-Loopy-Pro"
MODEL_FILE = {
    "pro": "x0x37-{ver}.fwsc",
    "elite": "x0x37-{ver}-elite.fwsc",
    "mke": "x0x37-{ver}-mke.fwsc",
    "fm1": "x0x-{ver}.fwsc",
}


def installer_model(product: str) -> str:
    """Return the exact supported target model for a release package identity."""
    if re.fullmatch(r"FM-1_9(?:\d{2}|\d{6})", product) and product != "FM-1_900":
        return "FM-1"
    m = re.fullmatch(r"(SMK-37 Pro|SMK-37 Elite|MKE-P37)_9\d{6}", product)
    if m:
        return m.group(1)
    raise SystemExit(f"unsupported installer identity {product!r}: use a release build for FM-1 or SMK-37")


def model_key(product: str) -> str:
    name = installer_model(product)
    return {"FM-1": "fm1", "SMK-37 Pro": "pro", "SMK-37 Elite": "elite", "MKE-P37": "mke"}[name]


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_version(version: str) -> str:
    return re.sub(r"[^A-Za-z0-9.-]", "-", version)


def pkg_entry(path: Path, version: str, key: str | None = None) -> tuple[str, dict]:
    raw = path.read_bytes()
    product = product_of(raw)
    try:
        installer_model(product)
    except SystemExit as e:
        raise SystemExit(f"{path}: identity {product!r}: make a release build") from e
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{path}: no update loader in it")
    key = key or model_key(product)
    name = MODEL_FILE[key].format(ver=safe_version(version))
    return key, {
        "product": product,
        "pkg": f"../firmware/{name}",
        "file": name,
        "sha256": sha256_file(path),
        "size": len(raw),
        "src": path,
    }


LANDING = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X0X for the SMK-37 · Loopy Pro</title>
<meta name="description" content="Groovebox firmware for the M-VAVE SMK-37 (and FM-1): install any version in the browser, import your own break samples.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow:wght@400;600&family=Barlow+Semi+Condensed:wght@600;700&display=swap">
<style>
:root { --ground: #f4f5f6; --ink: #1c1f23; --muted: #5b626a; --rule: #d9dde1; --accent: #c8700a; --card: #fff; color-scheme: light; }
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) { --ground: #131518; --ink: #e6e8eb; --muted: #9aa1a9; --rule: #2d3137; --accent: #f0a043; --card: #1a1d21; color-scheme: dark; }
}
:root[data-theme="dark"] { --ground: #131518; --ink: #e6e8eb; --muted: #9aa1a9; --rule: #2d3137; --accent: #f0a043; --card: #1a1d21; color-scheme: dark; }
body { background: var(--ground); color: var(--ink); margin: 0; padding-inline: 16px; padding-block: 48px 64px;
       font: 400 17px/1.6 "Barlow", "Helvetica Neue", Arial, sans-serif; }
main { max-width: 40rem; margin: 0 auto; }
h1 { font: 700 2.6rem/1.1 "Barlow Semi Condensed", "Barlow", sans-serif; margin: 0 0 .4rem; }
.lede { color: var(--muted); margin: 0 0 1.6rem; max-width: 34rem; }
.status { border: 1px solid var(--rule); background: var(--card); border-radius: 8px; padding: 12px 16px; margin: 0 0 2rem; }
.ways { display: grid; gap: 12px; margin: 0 0 2rem; }
.ways a { display: block; text-decoration: none; color: var(--ink); background: var(--card); border: 1px solid var(--rule);
          border-radius: 10px; padding: 14px 18px; }
.ways a:hover, .ways a:focus-visible { border-color: var(--accent); outline: none; }
.ways .card { background: var(--card); border: 1px solid var(--rule); border-radius: 10px; padding: 14px 18px; }
.ways .card a { color: var(--accent); text-decoration: none; border: 0; padding: 0; background: none; display: inline; }
.ways .card a:hover, .ways .card a:focus-visible { text-decoration: underline; }
.ways .card a.new { font: 400 .95rem "Barlow", sans-serif; margin-left: .5rem; }
.ways strong { font: 600 1.25rem "Barlow Semi Condensed", "Barlow", sans-serif; color: var(--accent); display: block; }
.ways span { color: var(--muted); font-size: .95rem; }
p.small { color: var(--muted); font-size: .9rem; }
a { color: var(--accent); }
</style>
</head>
<body>
<main>
<h1>X0X</h1>
<p class="lede">Groovebox firmware for the M-VAVE SMK-37 (Pro, Elite, MKE-P37) and the FM-1.
A 909, an 808, two 303s with TB-3PO, and a breakbeat player — with an APC40-style clip launcher
for Loopy Pro on the SMK-37.</p>
<div class="status">Install any published version from Chrome or Edge over USB. The sample importer
puts your own loops into the BREAK slots (optional oldschool grit, with preview before you write).
Installing is at your own risk; packages keep the keyboard's model name so M-UPGRADE still works.</div>
<nav class="ways" aria-label="Get X0X">
  <div class="card"><strong><a href="install/">Install__VERSION_LABEL__</a> <a class="new" href="new/">What's new?</a></strong><span>Upgrade, downgrade, or switch: pick a published version and model, or drop any .fwsc (stock / other custom). Nothing to install on the computer.</span></div>
  <a href="breaks/"><strong>Sample importer</strong><span>Drop in loops and send them to the BREAK part. Off by default: optional oldschool grit (light to max) and listen-before-send preview. Chrome or Edge.</span></a>
  <a href="projects/"><strong>Projects</strong><span>Save, load and reset on the keyboard, or export a project with its original samples.</span></a>
  <a href="manual/"><strong>Manual</strong><span>Getting started, then everything else — including the SMK-37 map.</span></a>
  <a href="__REPO__/releases"><strong>All releases</strong><span>Firmware files for the command line: <code>python3 tools/fm1_install.py …</code></span></a>
  <a href="__REPO__"><strong>Source</strong><span>GitHub, GPL-3.0. Built on Felucca by Hügelton Instruments.</span></a>
</nav>
<p class="small">SMK-37 control map: see <a href="__REPO__/blob/main/SMK37.md">SMK37.md</a>.
To go back to the official firmware, use M-VAVE's M-UPGRADE from
<a href="https://www.m-vave.com/download">m-vave.com/download</a>, or drop the stock .fwsc on the install page.
M-VAVE, FM-1 and SMK-37 are trademarks of their owners. Loopy Pro is a product of A Tasty Pixel.</p>
</main>
</body>
</html>
"""


def load_catalog(path: Path | None) -> dict:
    if not path or not path.is_file():
        return {"repo": REPO, "pages": PAGES, "latest": None, "versions": []}
    data = json.loads(path.read_text(encoding="utf-8"))
    data.setdefault("repo", REPO)
    data.setdefault("pages", PAGES)
    data.setdefault("versions", [])
    return data


def merge_version(catalog: dict, version: str, models: dict) -> dict:
    """Insert or replace one version entry; keep other versions' packages."""
    versions = [v for v in catalog.get("versions", []) if v.get("version") != version]
    entry = {"version": version, "models": {}}
    for key, meta in models.items():
        entry["models"][key] = {
            "product": meta["product"],
            "pkg": f"../firmware/{meta['file']}" if not str(meta["pkg"]).startswith("http") else meta["pkg"],
            "sha256": meta["sha256"],
            "size": meta["size"],
        }
    versions.append(entry)
    versions.sort(key=lambda v: str(v["version"]), reverse=True)
    catalog = dict(catalog)
    catalog["versions"] = versions
    catalog["latest"] = version
    catalog["repo"] = REPO
    catalog["pages"] = PAGES
    return catalog


def write_site(out: Path, catalog: dict, copy_pkgs: dict[str, Path], merge_from: Path | None, banner=None):
    """Build OUT from catalog; copy_pkgs maps firmware filename -> source path for this publish."""
    if out.exists():
        shutil.rmtree(out)
    for d in ("install", "manual", "firmware", "breaks", "projects", "new"):
        (out / d).mkdir(parents=True)

    # Carry forward every firmware file still referenced, plus new packages.
    keep = {m["pkg"].split("/")[-1] for v in catalog["versions"] for m in v.get("models", {}).values()
            if not str(m.get("pkg", "")).startswith("http")}
    if merge_from:
        src_fw = merge_from / "firmware"
        if src_fw.is_dir():
            for f in src_fw.glob("*.fwsc"):
                if f.name in keep:
                    shutil.copy2(f, out / "firmware" / f.name)
    for name, src in copy_pkgs.items():
        shutil.copy2(src, out / "firmware" / name)

    # Installer: inline libraries + catalog (paths relative to install/)
    page = (SRC / "web" / "x0x_installer.html").read_text(encoding="utf-8")
    lib = "\n".join(strip_module((SRC / "web" / f).read_text(encoding="utf-8"))
                    for f in ("fm1pkg.js", "fm1ota.js", "x0x_breaks.js", "boot_screen.js"))
    cat_json = json.dumps(catalog, separators=(",", ":"))
    for mark, repl in (("/*LIB*/", lib), ("/*CATALOG*/", cat_json)):
        if page.count(mark) != 1:
            raise SystemExit(f"x0x_installer.html must contain {mark} once")
        page = page.replace(mark, repl)
    (out / "install" / "index.html").write_text(page, encoding="utf-8")
    # Browser-relative catalog (install page also embeds it)
    browser_cat = json.loads(cat_json)
    for v in browser_cat["versions"]:
        for m in v.get("models", {}).values():
            if m["pkg"].startswith("../firmware/"):
                m["pkg"] = m["pkg"].replace("../firmware/", "firmware/", 1)
    (out / "versions.json").write_text(json.dumps(browser_cat, indent=2) + "\n", encoding="utf-8")

    # Breaks (sample importer)
    breaks = (SRC / "web" / "x0x_breaks.html").read_text(encoding="utf-8")
    breaks = breaks.replace("https://github.com/charlesvestal/fm1-x0x", REPO)
    if breaks.count("/*LIB*/") != 1:
        raise SystemExit("x0x_breaks.html must contain /*LIB*/ once")
    lib = strip_module((SRC / "web" / "x0x_breaks.js").read_text(encoding="utf-8"))
    (out / "breaks" / "index.html").write_text(breaks.replace("/*LIB*/", lib), encoding="utf-8")
    # The project page keeps its testable modules intact; it also runs from web/ on localhost.
    shutil.copy(SRC / "web" / "x0x_projects.html", out / "projects" / "index.html")
    for module in ("x0x_projects.js", "x0x_breaks.js"):
        shutil.copy(SRC / "web" / module, out / "projects" / module)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "MANUAL.md"),
                    str(out / "manual" / "index.html")], check=True, capture_output=True)
    shutil.copy(SRC / "docs" / "PROJECT_FILES.md", out / "manual" / "PROJECT_FILES.md")
    shutil.copytree(SRC / "docs" / "img", out / "manual" / "img")
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "NEW.md"),
                    str(out / "new" / "index.html")], check=True, capture_output=True)
    if (SRC / "docs" / "img" / "new").is_dir():
        shutil.copytree(SRC / "docs" / "img" / "new", out / "new" / "img" / "new")

    latest = catalog.get("latest") or (catalog["versions"][0]["version"] if catalog["versions"] else "")
    ver_label = f" v{html.escape(str(latest))}" if latest else ""
    landing = (LANDING.replace("__VERSION_LABEL__", ver_label)
                      .replace("__REPO__", REPO))
    if banner:
        landing = landing.replace("<div class=\"status\">",
                                  "<div class=\"status\">" + html.escape(banner) + " ", 1)
    (out / "index.html").write_text(landing, encoding="utf-8")
    (out / ".nojekyll").write_text("")
    models = ", ".join(sorted({k for v in catalog["versions"] for k in v.get("models", {})}))
    print(f"site: {out}: {len(catalog['versions'])} version(s), models [{models}], "
          f"firmware files {len(list((out / 'firmware').glob('*.fwsc')))}")


def main_legacy(pkg: Path, version: str, out: Path, merge: Path | None):
    key, meta = pkg_entry(pkg, version)
    catalog = load_catalog((merge / "versions.json") if merge else None)
    catalog = merge_version(catalog, version, {key: meta})
    write_site(out, catalog, {meta["file"]: meta["src"]}, merge)


def main_shell(out: Path, merge: Path | None):
    """Hostable site with installer + sample importer; catalog may be empty until a release."""
    catalog = load_catalog((merge / "versions.json") if merge else None)
    catalog["repo"] = REPO
    catalog["pages"] = PAGES
    write_site(out, catalog, {}, merge)


def main_multi(args):
    version = args.version
    models = {}
    for key, path in (("pro", args.pro), ("elite", args.elite), ("mke", args.mke), ("fm1", args.fm1)):
        if not path:
            continue
        k, meta = pkg_entry(Path(path), version, key)
        if k != key:
            raise SystemExit(f"{path}: identity is {meta['product']} (expected model {key})")
        models[key] = meta
    if not models:
        raise SystemExit("pass at least one of --pro / --elite / --mke / --fm1")
    merge = Path(args.merge) if args.merge else None
    catalog = load_catalog((merge / "versions.json") if merge else None)
    catalog = merge_version(catalog, version, models)
    write_site(Path(args.out), catalog, {m["file"]: m["src"] for m in models.values()}, merge)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and not argv[0].startswith("-") and len(argv) == 3:
        main_legacy(Path(argv[0]), argv[1], Path(argv[2]), None)
        return
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--shell", action="store_true",
                    help="build hostable site without new packages (drop-.fwsc + Releases still work)")
    ap.add_argument("--version", help="release version label, e.g. 0.2 or 1.0.6")
    ap.add_argument("--out", required=True, help="output site directory")
    ap.add_argument("--pro", help="SMK-37 Pro .fwsc")
    ap.add_argument("--elite", help="SMK-37 Elite .fwsc")
    ap.add_argument("--mke", help="MKE-P37 .fwsc")
    ap.add_argument("--fm1", help="FM-1 .fwsc")
    ap.add_argument("--merge", help="existing site dir whose firmware/ and versions.json to keep")
    args = ap.parse_args(argv)
    merge = Path(args.merge) if args.merge else None
    if args.shell:
        main_shell(Path(args.out), merge)
        return
    if not args.version:
        raise SystemExit("--version is required unless --shell")
    main_multi(args)


if __name__ == "__main__":
    try:
        main()
    except BrokenPipeError:
        pass
