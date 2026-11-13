#!/usr/bin/env python3
"""Build a read-only release-status catalog JSON and a static HTML page.

The catalog is generated from the release records only. It does not publish,
promote, install, or write to a device. An Elite report never implies Pro.
Withdrawn builds stay visible. Hardware qualification is never inferred from
an upload or from emulation.
"""

from __future__ import annotations

import argparse
import html
import json
import sys
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from evaluate_promotion import evaluate_record  # noqa: E402
from validate_release_status import (  # noqa: E402
    SHA256,
    UsageError,
    canonical_json,
    default_paths,
    load_records,
    models_of,
    record_id_of,
    sha256_text,
)

CATALOG_SCHEMA = 1
TOOL_PATH = "release-status/tools/generate_catalog.py"

DISCLAIMERS = [
    "This catalog is a read-only report from release-status records.",
    "It is not a hardware qualification and it does not promote a build.",
    "An Elite report never implies a Pro report.",
    "Emulation, an upload, and a catalog publication are not hardware qualification.",
    "A withdrawn build is shown and must not be offered for install.",
]


def _history(record: dict) -> list[dict]:
    return [h for h in (record.get("history") or []) if isinstance(h, dict)]


def _event_view(event: dict) -> dict[str, Any]:
    return {
        "sequence": event.get("sequence"),
        "time": event.get("time"),
        "time_precision": event.get("time_precision"),
        "order": event.get("order"),
        "kind": event.get("kind"),
        "source": event.get("source"),
        "quoted": bool(event.get("quoted") is True),
        "confidence": event.get("confidence"),
        "verified": bool(event.get("verified") is True),
        "is_current": bool(event.get("is_current") is True),
        "text": event.get("text"),
        "events": list(event.get("events") or []),
        "device": event.get("device"),
        "model": event.get("model"),
        "model_confidence": event.get("model_confidence"),
        "build": event.get("build"),
        "build_confidence": event.get("build_confidence"),
        "installed_hash": event.get("installed_hash"),
        "installed_hash_confidence": event.get("installed_hash_confidence"),
        "scope": event.get("scope"),
        "notes": list(event.get("notes") or []),
        "ref": event.get("ref"),
    }


def _model_view(name: str, model: dict, record: dict) -> dict[str, Any]:
    tested = False
    for h in _history(record):
        raw = str(h.get("model") or "").lower()
        if raw == name.lower() or name.lower() in raw:
            if h.get("kind") not in {"publish", "withdrawal", "withdraw"}:
                tested = True
    return {
        "model": name,
        "product": model.get("product"),
        "image_sha256": model.get("image_sha256"),
        "image_size": model.get("image_size"),
        "package_sha256": model.get("package_sha256"),
        "package_url": model.get("package_url"),
        "controller_sha256": model.get("controller_sha256"),
        "controller_size": model.get("controller_size"),
        "controller_strategy": model.get("controller_strategy"),
        "controller_check": model.get("controller_check"),
        "crc32": model.get("crc32"),
        "owner_tested": tested,
        "install_verified": False,
        "owner_hardware_approval": False,
        "note": (
            f"The {name} package is recorded by exact SHA-256. "
            "An Elite report never implies Pro."
        ),
    }


def _elf_view(record: dict) -> dict[str, Any] | None:
    elf = record.get("elf_provenance")
    if not isinstance(elf, dict):
        return None
    # Hashes only. Never copy bytes-like fields.
    out: dict[str, Any] = {"published": bool(elf.get("published") is True)}
    for key in ("elite_sha256", "pro_sha256", "toolchain", "note", "kept"):
        if key in elf:
            out[key] = elf[key]
    # Per-model hash maps used in the seed.
    for key, value in elf.items():
        if "sha" in key.lower() or key in ("models", "files", "elite", "pro"):
            if key not in out:
                out[key] = value
    out["bytes_included"] = False
    return out


def record_view(record: dict) -> dict[str, Any]:
    models = models_of(record)
    evaluation = evaluate_record(record)
    # Strip any temptation to claim hardware qualification.
    evaluation = dict(evaluation)
    evaluation["hardware_qualified"] = False
    evaluation["promotion_performed"] = False
    evaluation["automatic_promotion"] = "not-performed"

    model_views = {name: _model_view(name, model, record) for name, model in models.items()}
    # Attach per-model gate status from the evaluator (still not a promotion).
    for name, view in model_views.items():
        for g in evaluation.get("hardware_gates_by_model", {}).get(name, []):
            if g["gate"] == f"install-verified:{name}":
                view["install_verified_gate"] = g["status"]
                view["install_verified"] = False  # never claim verified unless status is pass AND owner says so
                # If the evidence gate is pass, still report it as evidence-pass, not a qualification stamp.
                view["install_verified_evidence"] = g["status"]
            if g["gate"] == f"owner-hardware-approval:{name}":
                view["owner_hardware_approval_gate"] = g["status"]
                view["owner_hardware_approval"] = False
                view["owner_hardware_approval_evidence"] = g["status"]

    events = [_event_view(h) for h in _history(record)]
    withdrawn = bool((record.get("states") or {}).get("withdrawn")) or record.get("channel") == "withdrawn"

    return {
        "version": record.get("version"),
        "catalog_id": record.get("catalog_id"),
        "channel": record.get("channel"),
        "source_sha": record.get("source_sha"),
        "source_tree": record.get("source_tree"),
        "source_branch": record.get("source_branch"),
        "source_pr": record.get("source_pr"),
        "published_pages_url": record.get("published_pages_url"),
        "states": dict(record.get("states") or {}),
        "identity_confidence": dict(record.get("identity_confidence") or {}),
        "models": model_views,
        "owner_reported_events": events,
        "withdrawn": withdrawn,
        "withdraw_ref": record.get("withdraw_ref"),
        "withdraw_reason": record.get("withdraw_reason"),
        "withdraw_time": record.get("withdraw_time"),
        "replaced_by": record.get("replaced_by"),
        "notes": list(record.get("notes") or []),
        "elf_provenance": _elf_view(record),
        "elite_never_implies_pro": True,
        "hardware_qualified": False,
        "promotion_performed": False,
        "gates": evaluation,
    }


def record_sort_key(view: dict) -> tuple:
    # Current offered builds first, withdrawn last. Then version, then id.
    withdrawn = 1 if view.get("withdrawn") else 0
    channel_rank = {
        "stable-MAIN": 0,
        "nightly-hardware-test": 1,
        "withdrawn": 2,
    }.get(view.get("channel") or "", 3)
    return (withdrawn, channel_rank, view.get("version") or "", view.get("catalog_id") or "")


def build_catalog(records: list[tuple[str, dict]]) -> dict[str, Any]:
    views = [record_view(rec) for _name, rec in records]
    views.sort(key=record_sort_key)
    payload = {
        "schema": CATALOG_SCHEMA,
        "tool": TOOL_PATH,
        "role": "read-only-catalog",
        "automatic_promotion": "not-performed",
        "promotion_performed": False,
        "hardware_qualified": False,
        "elite_never_implies_pro": True,
        "disclaimers": list(DISCLAIMERS),
        "records": views,
    }
    # Content hash of the records section for integrity, after a canonical dump
    # that excludes this field.
    payload["content_sha256"] = sha256_text(canonical_json({
        "schema": payload["schema"],
        "records": payload["records"],
        "disclaimers": payload["disclaimers"],
    }))
    return payload


def _esc(value: Any) -> str:
    if value is None:
        return ""
    return html.escape(str(value), quote=True)


def _state_cells(states: dict) -> str:
    order = [
        "built",
        "gate-passed",
        "published-nightly",
        "installation-reported",
        "boot-restart-reported",
        "install-verified",
        "owner-hardware-approval",
        "withdrawn",
    ]
    parts = []
    for key in order:
        if key not in states:
            continue
        val = states[key]
        cls = "yes" if val else "no"
        # withdrawn=true is a warning, not a success.
        if key == "withdrawn" and val:
            cls = "withdrawn"
        if key in ("install-verified", "owner-hardware-approval") and val:
            cls = "yes"
        parts.append(
            f'<tr><th scope="row">{_esc(key)}</th><td class="{cls}">{"true" if val else "false"}</td></tr>'
        )
    for key, val in states.items():
        if key in order:
            continue
        parts.append(
            f'<tr><th scope="row">{_esc(key)}</th><td>{"true" if val else "false"}</td></tr>'
        )
    return "\n".join(parts)


def _gate_rows(evaluation: dict) -> str:
    rows = []
    for g in evaluation.get("gates") or []:
        status = g.get("status") or ""
        model = g.get("model")
        label = g.get("gate") + (f" ({model})" if model else "")
        rows.append(
            "<tr>"
            f'<td class="gate-{_esc(status)}">{_esc(status)}</td>'
            f"<td>{_esc(label)}</td>"
            f"<td>{_esc(g.get('reason'))}</td>"
            "</tr>"
        )
    return "\n".join(rows)


def _event_block(event: dict) -> str:
    quoted = "quoted owner words" if event.get("quoted") else "not a quotation"
    current = "current" if event.get("is_current") else (event.get("order") or "")
    meta = [
        f"sequence { _esc(event.get('sequence')) }",
        _esc(current),
        _esc(event.get("kind")),
        _esc(quoted),
        "verified" if event.get("verified") else "not verified",
    ]
    time = event.get("time") or "time not recorded"
    if event.get("time_precision") and event.get("time_precision") != "exact":
        time = f"{time} ({event.get('time_precision')})"
    model = event.get("model")
    model_line = ""
    if model:
        model_line = (
            f"<p><strong>Model:</strong> {_esc(model)} "
            f"(confidence {_esc(event.get('model_confidence'))}). "
            "An Elite report never implies Pro.</p>"
        )
    elif event.get("kind") not in {"publish", "withdrawal"}:
        model_line = "<p><strong>Model:</strong> not stated.</p>"
    hash_line = ""
    if event.get("kind") not in {"publish", "withdrawal"}:
        ih = event.get("installed_hash")
        hash_line = (
            f"<p><strong>Installed hash:</strong> "
            f"{_esc(ih) if ih else 'absent'} "
            f"(confidence {_esc(event.get('installed_hash_confidence') or 'absent')}).</p>"
        )
    notes = "".join(f"<li>{_esc(n)}</li>" for n in (event.get("notes") or []))
    notes_html = f"<ul>{notes}</ul>" if notes else ""
    ref = event.get("ref")
    ref_html = f'<p>Ref: <a href="{_esc(ref)}">{_esc(ref)}</a></p>' if ref else ""
    return f"""
<article class="event">
  <h4>{_esc(event.get('kind') or 'event')} · {_esc(time)}</h4>
  <p class="meta">{' · '.join(m for m in meta if m.strip())}</p>
  <p><strong>Source:</strong> {_esc(event.get('source'))}</p>
  <p>{_esc(event.get('text'))}</p>
  {model_line}
  {hash_line}
  {ref_html}
  {notes_html}
</article>
"""


def _model_card(view: dict) -> str:
    name = view.get("model")
    tested = "owner-reported test is present" if view.get("owner_tested") else "no owner-reported test for this model"
    ev = view.get("install_verified_evidence") or "missing"
    ap = view.get("owner_hardware_approval_evidence") or "missing"
    return f"""
<article class="model" id="model-{_esc(name)}">
  <h4>SMK-37 {_esc((name or '').capitalize())}</h4>
  <p class="tag">{_esc(tested)}. An Elite report never implies Pro.</p>
  <p>Product: <code>{_esc(view.get('product'))}</code></p>
  <dl>
    <dt>Image SHA-256</dt><dd><code>{_esc(view.get('image_sha256'))}</code></dd>
    <dt>Image size</dt><dd>{_esc(view.get('image_size'))} bytes</dd>
    <dt>Package SHA-256</dt><dd><code>{_esc(view.get('package_sha256'))}</code></dd>
    <dt>Package URL</dt><dd><a href="{_esc(view.get('package_url'))}">{_esc(view.get('package_url'))}</a></dd>
    <dt>Controller SHA-256</dt><dd><code>{_esc(view.get('controller_sha256'))}</code></dd>
    <dt>Controller check</dt><dd>{_esc(view.get('controller_check'))} ({_esc(view.get('controller_strategy'))}, {_esc(view.get('controller_size'))} bytes)</dd>
    <dt>Install-verified evidence</dt><dd>{_esc(ev)} (this is not a qualification stamp)</dd>
    <dt>Owner-hardware-approval evidence</dt><dd>{_esc(ap)} (this is not a qualification stamp)</dd>
  </dl>
</article>
"""


def _record_section(view: dict) -> str:
    withdrawn = view.get("withdrawn")
    cls = "record withdrawn-record" if withdrawn else "record"
    title = f"DnBOX {view.get('version')} · {view.get('catalog_id')}"
    banner = ""
    if withdrawn:
        banner = (
            '<p class="banner withdrawn-banner"><strong>Withdrawn.</strong> '
            f"{_esc(view.get('withdraw_reason'))} "
            f'Ref: <a href="{_esc(view.get("withdraw_ref"))}">{_esc(view.get("withdraw_ref"))}</a>. '
            "Do not offer this exact build for install.</p>"
        )
    conf = view.get("identity_confidence") or {}
    conf_html = (
        f"<p>Identity confidence: model <strong>{_esc(conf.get('model'))}</strong>, "
        f"build <strong>{_esc(conf.get('build'))}</strong>, "
        f"installed hash <strong>{_esc(conf.get('installed_hash'))}</strong>.</p>"
    )
    if conf.get("model_note"):
        conf_html += f"<p>{_esc(conf.get('model_note'))}</p>"
    if conf.get("build_note"):
        conf_html += f"<p>{_esc(conf.get('build_note'))}</p>"
    if conf.get("installed_hash_note"):
        conf_html += f"<p>{_esc(conf.get('installed_hash_note'))}</p>"
    models_html = "".join(_model_card(m) for m in (view.get("models") or {}).values())
    events_html = "".join(_event_block(e) for e in (view.get("owner_reported_events") or []))
    if not events_html:
        events_html = "<p>No owner-reported events are recorded.</p>"
    notes = "".join(f"<li>{_esc(n)}</li>" for n in (view.get("notes") or []))
    notes_html = f"<ul>{notes}</ul>" if notes else ""
    gates = view.get("gates") or {}
    ready = gates.get("owner_required_promotion_ready")
    return f"""
<section class="{cls}" id="{_esc(view.get('catalog_id'))}">
  <h2>{_esc(title)}</h2>
  {banner}
  <p>Channel: <strong>{_esc(view.get('channel'))}</strong>. Source SHA: <code>{_esc(view.get('source_sha'))}</code>. Source tree: <code>{_esc(view.get('source_tree'))}</code>.</p>
  <p>Elite never implies Pro. This record is not a hardware qualification. This page does not promote this build.</p>
  {conf_html}
  <h3>States</h3>
  <table class="states">
    <thead><tr><th>Flag</th><th>Value</th></tr></thead>
    <tbody>
    {_state_cells(view.get('states') or {})}
    </tbody>
  </table>
  <h3>Exact model artifacts</h3>
  <div class="models">
  {models_html}
  </div>
  <h3>Owner-reported events (with sources)</h3>
  {events_html}
  <h3>Owner-required gates (report only)</h3>
  <p>{_esc(gates.get('channel_note'))}</p>
  <p>Automatic promotion: <strong>not-performed</strong>. Owner-required promotion ready: <strong>{_esc(ready)}</strong>. {_esc(gates.get('ready_reason'))}</p>
  <table class="gates">
    <thead><tr><th>Status</th><th>Gate</th><th>Reason</th></tr></thead>
    <tbody>
    {_gate_rows(gates)}
    </tbody>
  </table>
  <h3>Record notes</h3>
  {notes_html}
</section>
"""


def render_html(catalog: dict) -> str:
    sections = "".join(_record_section(v) for v in (catalog.get("records") or []))
    disclaimers = "".join(f"<li>{_esc(d)}</li>" for d in (catalog.get("disclaimers") or DISCLAIMERS))
    content_sha = catalog.get("content_sha256") or ""
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>DnBOX release status</title>
  <style>
    :root {{
      --bg: #f7f4ef;
      --ink: #1b1b18;
      --muted: #4a4a43;
      --card: #fffdf8;
      --line: #d4cfc4;
      --withdrawn: #8b1e1e;
      --withdrawn-bg: #f8e8e8;
      --pass: #215c2a;
      --fail: #8b1e1e;
      --missing: #6a4d00;
      --yes: #215c2a;
      --no: #5c5c56;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      font-family: "Source Serif 4", "Iowan Old Style", "Palatino Linotype", Palatino, serif;
      background: var(--bg);
      color: var(--ink);
      line-height: 1.45;
    }}
    header, main, footer {{
      max-width: 68rem;
      margin: 0 auto;
      padding: 1.25rem 1.25rem;
    }}
    header {{
      padding-top: 2rem;
    }}
    h1 {{ font-size: 2rem; margin: 0 0 0.5rem; }}
    h2 {{ font-size: 1.45rem; margin-top: 0; }}
    h3 {{ font-size: 1.15rem; }}
    h4 {{ font-size: 1.05rem; margin-bottom: 0.3rem; }}
    .lead, .meta, footer {{ color: var(--muted); }}
    .disclaimers {{
      background: var(--card);
      border: 1px solid var(--line);
      padding: 0.75rem 1rem;
    }}
    .record {{
      background: var(--card);
      border: 1px solid var(--line);
      padding: 1.2rem 1.2rem 0.4rem;
      margin: 1.5rem 0;
    }}
    .withdrawn-record {{
      border-color: var(--withdrawn);
      background: var(--withdrawn-bg);
    }}
    .withdrawn-banner, .banner {{
      border: 2px solid var(--withdrawn);
      padding: 0.6rem 0.8rem;
      background: #fff;
    }}
    .models {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(22rem, 1fr));
      gap: 1rem;
    }}
    .model, .event {{
      border: 1px solid var(--line);
      padding: 0.8rem 1rem;
      background: #fff;
      margin-bottom: 0.8rem;
    }}
    table {{
      border-collapse: collapse;
      width: 100%;
      background: #fff;
      margin-bottom: 1rem;
    }}
    th, td {{
      text-align: left;
      vertical-align: top;
      border: 1px solid var(--line);
      padding: 0.35rem 0.5rem;
    }}
    code {{
      font-family: "IBM Plex Mono", "Source Code Pro", ui-monospace, monospace;
      font-size: 0.82em;
      word-break: break-all;
    }}
    td.yes, .gate-pass {{ color: var(--yes); font-weight: 600; }}
    td.no {{ color: var(--no); }}
    td.withdrawn, .gate-fail {{ color: var(--fail); font-weight: 600; }}
    .gate-missing {{ color: var(--missing); font-weight: 600; }}
    .tag {{ font-style: italic; color: var(--muted); }}
    dl {{ display: grid; grid-template-columns: 12rem 1fr; gap: 0.2rem 0.8rem; }}
    dt {{ font-weight: 600; }}
    dd {{ margin: 0; }}
    a {{ color: #123a73; }}
    nav ul {{ padding-left: 1.2rem; }}
  </style>
</head>
<body>
  <header>
    <h1>DnBOX release status</h1>
    <p class="lead">A read-only status page generated from release-status records. It is not a live installer and it does not promote a build.</p>
    <ul class="disclaimers">
      {disclaimers}
    </ul>
    <nav>
      <p>Records in this catalog:</p>
      <ul>
        {''.join(
            f'<li><a href="#{_esc(v.get("catalog_id"))}">DnBOX {_esc(v.get("version"))} · {_esc(v.get("catalog_id"))}'
            + (' (withdrawn)' if v.get('withdrawn') else '')
            + f' · {_esc(v.get("channel"))}</a></li>'
            for v in (catalog.get('records') or [])
        )}
      </ul>
    </nav>
  </header>
  <main>
    {sections}
  </main>
  <footer>
    <p>Generated by {_esc(catalog.get('tool') or TOOL_PATH)}. Content SHA-256: <code>{_esc(content_sha)}</code>.</p>
    <p>This page is not published by the generator. Publication needs a later owner decision.</p>
  </footer>
</body>
</html>
"""


def write_catalog(catalog: dict, json_path: Path, html_path: Path) -> None:
    json_path.parent.mkdir(parents=True, exist_ok=True)
    html_path.parent.mkdir(parents=True, exist_ok=True)
    # Stable pretty JSON: sort keys at the top level only would scramble
    # meaningful field order. Keep insertion order and a trailing newline.
    json_path.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n")
    html_path.write_text(render_html(catalog))


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    paths = default_paths()
    root = paths["records"].parent
    p = argparse.ArgumentParser(
        description="Generate a read-only release-status catalog and HTML page. Does not publish."
    )
    p.add_argument("--records", type=Path, default=paths["records"], help="records directory")
    p.add_argument(
        "--out-json",
        type=Path,
        default=root / "catalog" / "catalog.json",
        help="catalog JSON output path",
    )
    p.add_argument(
        "--out-html",
        type=Path,
        default=root / "catalog" / "index.html",
        help="status page HTML output path",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="print the catalog JSON to stdout and do not write files",
    )
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    try:
        args = parse_args(argv)
        if not args.records.is_dir():
            raise UsageError(f"records directory not found: {args.records}")
        records = load_records(args.records)
        if not records:
            raise UsageError(f"no records found in {args.records}")
        catalog = build_catalog(records)
        if args.dry_run:
            sys.stdout.write(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n")
            return 0
        write_catalog(catalog, args.out_json, args.out_html)
        sys.stdout.write(
            f"Wrote {args.out_json} and {args.out_html}\n"
            f"content_sha256={catalog['content_sha256']}\n"
            "This tool did not publish and did not promote.\n"
        )
        return 0
    except UsageError as exc:
        sys.stderr.write(f"USAGE: {exc}\n")
        return 2


if __name__ == "__main__":
    sys.exit(main())
