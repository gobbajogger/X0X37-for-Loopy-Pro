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


def version_sort_key(version: str) -> tuple:
    """Sort numeric versions naturally: 0.0.10 follows 0.0.9 in release history."""
    return tuple((1, int(part)) if part.isdecimal() else (0, part.casefold())
                 for part in re.split(r"(\d+)", str(version)))


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
    previous = next((v for v in catalog.get("versions", []) if v.get("version") == version), {})
    entry = {**previous, "version": version, "models": dict(previous.get("models", {}))}
    for key, meta in models.items():
        old = entry["models"].get(key)
        if old and any(old.get(field) != meta.get(field) for field in ("sha256", "size", "product")):
            raise SystemExit(f"version {version}/{key} already identifies different package bytes; assign a new version")
        entry["models"][key] = {
            "product": meta["product"],
            "pkg": f"../firmware/{meta['file']}" if not str(meta["pkg"]).startswith("http") else meta["pkg"],
            "sha256": meta["sha256"],
            "size": meta["size"],
        }
    versions.append(entry)
    versions.sort(key=lambda v: version_sort_key(v["version"]), reverse=True)
    catalog = dict(catalog)
    catalog["versions"] = versions
    old_latest = catalog.get("latest")
    catalog["latest"] = (old_latest if old_latest and
                         any(v["version"] == old_latest for v in versions) and
                         version_sort_key(old_latest) >= version_sort_key(version)
                         else version)
    catalog["repo"] = REPO
    catalog["pages"] = PAGES
    return catalog


def write_site(out: Path, catalog: dict, copy_pkgs: dict[str, Path], merge_from: Path | None, banner=None):
    """Build OUT from catalog; copy_pkgs maps firmware filename -> source path for this publish."""
    if out.resolve() == SRC.resolve() or out.resolve() in SRC.resolve().parents:
        raise SystemExit("site output cannot replace the source worktree or its parents")
    if merge_from and (out.resolve() == merge_from.resolve() or out.resolve() in merge_from.resolve().parents):
        raise SystemExit("site output must not contain the retained site")
    if any(out.resolve() == src.resolve() or out.resolve() in src.resolve().parents for src in copy_pkgs.values()):
        raise SystemExit("site output must not contain input firmware packages")
    # Validate retention before deleting/replacing an output. Missing historical bytes
    # must fail, rather than publish a catalog whose older packages disappeared.
    for version in catalog.get("versions", []):
        for pkg in version.get("models", {}).values():
            path = str(pkg.get("pkg", ""))
            if path.startswith("http"):
                continue
            name = path.split("/")[-1]
            source = copy_pkgs.get(name) or (merge_from / "firmware" / name if merge_from else None)
            if not name.endswith(".fwsc") or not source or not source.is_file():
                raise SystemExit(f"retained firmware is missing: {name}")
            if pkg.get("sha256") and sha256_file(source) != pkg["sha256"]:
                raise SystemExit(f"retained firmware digest mismatch: {name}")
            if pkg.get("size") and source.stat().st_size != pkg["size"]:
                raise SystemExit(f"retained firmware size mismatch: {name}")
    if out.exists():
        shutil.rmtree(out)
    for d in ("install", "manual", "firmware", "breaks", "projects", "new", "history"):
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
                    for f in ("fm1pkg.js", "fm1ota.js", "x0x_breaks.js", "factory_break_pack.js", "boot_screen.js", "device_session.js", "workspace.js"))
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
    lib = "\n".join(strip_module((SRC / "web" / name).read_text(encoding="utf-8")) for name in ("x0x_breaks.js", "device_session.js"))
    (out / "breaks" / "index.html").write_text(breaks.replace("/*LIB*/", lib), encoding="utf-8")
    # The project page keeps its testable modules intact; it also runs from web/ on localhost.
    shutil.copy(SRC / "web" / "x0x_projects.html", out / "projects" / "index.html")
    for module in ("x0x_projects.js", "x0x_breaks.js", "device_session.js"):
        shutil.copy(SRC / "web" / module, out / "projects" / module)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "MANUAL.md"),
                    str(out / "manual" / "legacy.html")], check=True, capture_output=True)
    shutil.copy(SRC / "docs" / "PROJECT_FILES.md", out / "manual" / "PROJECT_FILES.md")
    shutil.copytree(SRC / "docs" / "img", out / "manual" / "img")
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "NEW.md"),
                    str(out / "new" / "legacy.html")], check=True, capture_output=True)
    if (SRC / "docs" / "img" / "new").is_dir():
        shutil.copytree(SRC / "docs" / "img" / "new", out / "new" / "img" / "new")

    shutil.copy(SRC / "web" / "site.css", out / "site.css")
    shutil.copytree(SRC / "web" / "assets", out / "assets")
    for section, source in (("manual", "MANUAL.md"), ("new", "NEW.md")):
        subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "web" / source),
                        str(out / section / "index.html")], check=True, capture_output=True)
    # Real documentation and history share the workspace typography/navigation.
    nav = '<nav class="site-nav" aria-label="DnBOX"><a href="../">DnBOX</a><a href="../manual/">Manual</a><a href="../new/">What’s new</a><a href="../history/">History</a></nav>'
    for section in ("manual", "new"):
        for doc in (out / section).glob("*.html"):
            content = doc.read_text(encoding="utf-8").replace("</head>", '<link rel="stylesheet" href="../site.css"></head>')
            content = content.replace("<body>", "<body>" + nav)
            doc.write_text(content, encoding="utf-8")
    entries = []
    for v in catalog.get("versions", []):
        models = v.get("models", {})
        rows = []
        for model, pkg in models.items():
            rows.append('<li><strong>' + html.escape(str(model)) + '</strong>: ' + html.escape(str(pkg.get("product", "Unknown product"))) +
                        '<br>Package: <code>' + html.escape(str(pkg.get("pkg", "No package"))) + '</code>' +
                        '<br>SHA-256: <code>' + html.escape(str(pkg.get("sha256", "Not recorded"))) + '</code>' +
                        '<br>Bytes: ' + html.escape(str(pkg.get("size", "Not recorded"))) + '</li>')
        entries.append('<section><h2>' + html.escape(str(v.get("version", "Unversioned history entry"))) + '</h2><p>' +
                       html.escape(str(v.get("label") or v.get("status") or "Catalog entry; installation requires independent publisher admission")) +
                       '</p><p>Source: <code>' + html.escape(str(v.get("source_commit", "Not recorded"))) + '</code></p><ul>' + ''.join(rows) + '</ul></section>')
    history = '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>DnBOX · History</title><link rel="stylesheet" href="../site.css"></head><body><main>' + nav + '<h1>Firmware history</h1><p>Retained catalog records. Only packages admitted by the installer’s independent publisher checks are installable there. A history record does not grant that approval. No package numbers or checksums are invented.</p>' + (''.join(entries) or '<p>This generated site has no package catalog yet. Use a verified local firmware file. No earlier package history was supplied to this build.</p>') + '</main></body></html>'
    (out / "history" / "index.html").write_text(history, encoding="utf-8")
    # The root is the same install/data workspace, with paths rebased once.
    landing = page.replace('../site.css', 'site.css').replace('href="../"', 'href="./"')
    for section in ("manual", "new", "history", "breaks", "projects"):
        landing = landing.replace(f'../{section}/', f'{section}/')
    landing = landing.replace('../firmware/', 'firmware/')
    if banner:
        landing = landing.replace('<div id="installview">', '<div id="installview"><p class="browser-note">' + html.escape(banner) + '</p>', 1)
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
