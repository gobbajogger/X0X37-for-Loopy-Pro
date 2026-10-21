#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the X0X site for GitHub Pages (https://charlesvestal.github.io/fm1-x0x/):

  index.html                  what X0X is, and the three ways in: install, manual, source
  install/index.html          the web installer (web/x0x_installer.html, with fm1pkg.js, fm1ota.js,
                              x0x_breaks.js (the slots kept over an install) and the package's
                              metadata inlined; Chrome or Edge, Web MIDI)
  breaks/index.html           your own break loops onto the FM-1 (web/x0x_breaks.html with
                              x0x_breaks.js inlined; Chrome or Edge, Web MIDI)
  projects/index.html         project save/load/reset and portable files, with its two JS modules
  manual/index.html, img/     the manual (tools/manual_html.py, from docs/MANUAL.md)
  new/index.html, img/new/    what's new in each version (the same, from docs/NEW.md)
  firmware/x0x-VERSION.fwsc   the package the installer writes; also the download

  tools/make_pages.py build/x0x-0.1-beta.fwsc 0.1-beta OUT_DIR

The package must be a release build (./build.sh --release X.Y-beta): the installer accepts the matching FM-1, SMK-37 Pro,
SMK-37 Elite or MKE-P37 identity. For SMK builds, FM-1-only stock recovery controls are hidden."""
import html
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
from make_site import product_of, strip_module  # noqa: E402  (Felucca's: the package format)

REPO = "https://github.com/charlesvestal/fm1-x0x"
STOCK_URL = "https://yms-file-store.oss-cn-hongkong.aliyuncs.com/software/firmware/FM-1.fwsc"


def installer_model(product):
    """Return the exact supported target model for a release package identity."""
    if re.fullmatch(r"FM-1_9(?:\d{2}|\d{6})", product) and product != "FM-1_900":
        return "FM-1"
    m = re.fullmatch(r"(SMK-37 Pro|SMK-37 Elite|MKE-P37)_9\d{6}", product)
    if m:
        return m.group(1)
    raise SystemExit(f"unsupported installer identity {product!r}: use a release build for FM-1 or SMK-37")


LANDING = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X0X for the FM-1</title>
<meta name="description" content="Groovebox firmware for the M-VAVE FM-1: a 909, an 808, two 303s and a breakbeat player.">
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
.video { position: relative; aspect-ratio: 16 / 9; max-width: 100%; margin: 1.5rem 0; border-radius: 6px; overflow: hidden; background: #000; }
.video iframe { position: absolute; inset: 0; width: 100%; height: 100%; border: 0; }
a { color: var(--accent); }
</style>
</head>
<body>
<main>
<h1>X0X</h1>
<p class="lede">Groovebox firmware for the M-VAVE FM-1. A 909, an 808, two 303s with the TB-3PO
line generator, and a breakbeat player, all running at once, with patterns, a song mode and
recorded knob moves.</p>
<div class="video"><iframe src="https://www.youtube-nocookie.com/embed/cZLYrZaLDUk" title="X0X running on the FM-1"
  allow="accelerometer; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen loading="lazy"></iframe></div>
<div class="status">X0X installs and uninstalls the way Felucca does, and the installer can put M-VAVE's own firmware back. Installing is
at your own risk.</div>
<nav class="ways" aria-label="Get X0X">
  <div class="card"><strong><a href="install/">Install v__VERSION__</a> <a class="new" href="new/">What's new?</a></strong><span>From Chrome or Edge, with the FM-1 connected by USB. Nothing to install on the computer.</span></div>
  <a href="manual/"><strong>Manual</strong><span>Getting started in ten steps, then everything else.</span></a>
  <a href="breaks/"><strong>Your own breaks</strong><span>Drop in loops and send them to the BREAK part, from Chrome or Edge.</span></a>
  <a href="projects/"><strong>Projects</strong><span>Save, load and reset on the keyboard, or export a project with its original samples.</span></a>
  <a href="firmware/__PKG__"><strong>Download __PKG__</strong><span>For the command-line installer: <code>python3 tools/fm1_install.py __PKG__</code></span></a>
  <a href="__REPO__"><strong>Source</strong><span>GitHub, GPL-3.0. Built on Felucca by Hügelton Instruments.</span></a>
</nav>
<p class="small">To go back to the official firmware, use M-VAVE's updater, M-UPGRADE, from
<a href="https://www.m-vave.com/download">m-vave.com/download</a>. M-VAVE and FM-1 are trademarks of their owners.</p>
</main>
</body>
</html>
"""


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    try:
        model = installer_model(product)
    except SystemExit as e:
        raise SystemExit(f"{pkg}: identity {product!r}: make a release build (./build.sh --release X.Y-beta)") from e
    is_fm1 = model == "FM-1"
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it")
    name = f"x0x-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    if out.exists():
        shutil.rmtree(out)
    for d in ("install", "manual", "firmware", "breaks", "projects", "new"):
        (out / d).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    # the installer
    page = (SRC / "web" / "x0x_installer.html").read_text(encoding="utf-8")
    if not is_fm1:
        # This page's rescue package is specifically FM-1 V15. Rename user-facing device
        # text for SMK builds, restore the literal FM-1 stock URL, and hide the FM-1-only
        # recovery/stock controls through meta.stock in the generated page.
        page = page.replace("FM-1", model)
        page = page.replace(f"https://yms-file-store.oss-cn-hongkong.aliyuncs.com/software/firmware/{model}.fwsc", STOCK_URL)
    lib = "\n".join(strip_module((SRC / "web" / f).read_text(encoding="utf-8"))
                    for f in ("fm1pkg.js", "fm1ota.js", "x0x_breaks.js"))   # x0x_breaks.js: the slots kept over an install
    meta = json.dumps({"version": version, "product": product, "pkg": "../firmware/" + name, "stock": is_fm1})
    for mark in ("/*LIB*/", "/*META*/"):
        if page.count(mark) != 1:
            raise SystemExit(f"x0x_installer.html must contain {mark} once")
    (out / "install" / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    # the break loop uploader
    page = (SRC / "web" / "x0x_breaks.html").read_text(encoding="utf-8")
    if page.count("/*LIB*/") != 1:
        raise SystemExit("x0x_breaks.html must contain /*LIB*/ once")
    lib = strip_module((SRC / "web" / "x0x_breaks.js").read_text(encoding="utf-8"))
    (out / "breaks" / "index.html").write_text(page.replace("/*LIB*/", lib), encoding="utf-8")
    # The project page keeps its testable modules intact; it also runs from web/ on localhost.
    shutil.copy(SRC / "web" / "x0x_projects.html", out / "projects" / "index.html")
    for module in ("x0x_projects.js", "x0x_breaks.js"):
        shutil.copy(SRC / "web" / module, out / "projects" / module)
    # the manual (a complete page; its images beside it)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "MANUAL.md"),
                    str(out / "manual" / "index.html")], check=True, capture_output=True)
    shutil.copytree(SRC / "docs" / "img", out / "manual" / "img")
    # what's new (the same converter; its pictures beside it)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "NEW.md"),
                    str(out / "new" / "index.html")], check=True, capture_output=True)
    shutil.copytree(SRC / "docs" / "img" / "new", out / "new" / "img" / "new")
    # the landing page
    landing = LANDING
    if not is_fm1:
        landing = landing.replace("FM-1", model)
    (out / "index.html").write_text(landing.replace("__VERSION__", html.escape(version)).replace("__PKG__", name)
                                    .replace("__REPO__", REPO), encoding="utf-8")
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html, install/ ({product}), manual/, firmware/{name} ({len(raw)} B)")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
