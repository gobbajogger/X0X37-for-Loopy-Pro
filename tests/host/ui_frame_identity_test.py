#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Box-only base/candidate UI identity and timing. Never invoked in Codex.

Reuses the production SMC fixture/build composition. Only scratch copies gain
observers. SHA-256 is computed as the child streams pixels/audio through a pipe.
"""
import argparse
import hashlib
import json
import itertools
import os
from pathlib import Path
import re
import struct
import subprocess
import threading

ROOT = Path(__file__).resolve().parents[2]
BASE = "ee89337b36c62002908e6729b02ad87864fb7c52"
BASE_BLOB = "1d14710acfa6a06bd5fc4c730cdb8d27203ae09b"
UI = ROOT / "firmware/src/app/ui.c"
FIXTURE = ROOT / "tests/host/smc_ui_test.c"
HOST = ROOT / "host/x0x_host.c"
DECL = "static void frame_pixels(unsigned,uint32_t,uint32_t,uint32_t,uint32_t,const uint16_t *);\n"


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"source anchor must occur once: {old[:100]!r}")
    return text.replace(old, new, 1)


def absolute_includes(text, source):
    def include(match):
        path = source.parent / match[1]
        return f'#include "{path.resolve()}"' if path.is_file() else match[0]
    return re.sub(r'#include "([^"]+)"', include, text)


def prepare(ui, dest):
    dest.mkdir(parents=True, exist_ok=False)
    # Benchmark the actual decay statements from this input, without renderer
    # cost masking C3. This is a generated probe, never production source.
    start = ui.index("    for (i = 0; i < NPARTS; i++)", ui.index("void ui_frame(void)"))
    end = ui.index("#if BOARD_SMK37", start)
    (dest/"decay.inc").write_text("static void frame_decay_probe(void) { uint32_t i;\n"+ui[start:end]+"}\n")
    # Observe the rendered canvas before commit can pack it in-place.
    for name in ("cv_commit", "cv_commit_rows"):
        pattern = rf"(static void {name}\([^\n]+\)\n\{{)"
        ui, count = re.subn(pattern, r"\1\n    frame_pixels('D',0,0,cv_w,cv_h,cv_px);", ui)
        if count != 1:
            raise ValueError(f"missing {name} observer anchor")
    # Also record cv_px after each top-level draw, including a packed prefix.
    prefix, frame = ui.split("void ui_frame(void)", 1)
    frame = re.sub(r"(draw_(?:header|main|knobs)\(\);)",
                   r"\1 frame_pixels('E',0,0,cv_w,cv_h,cv_px);", frame)
    ui_path = dest / "ui.c"
    ui_path.write_text(DECL + absolute_includes(prefix + "void ui_frame(void)" + frame, UI))
    host = HOST.read_text()
    host = replace_once(host,
        "static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)\n{",
        "static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)\n{\n"
        "    frame_pixels('B',x,y,w,h,px);")
    host = replace_once(host, '#include "../firmware/src/app/ui.c"', f'#include "{ui_path}"')
    # Wall-clock-derived profiler values are not renderer inputs for identity.
    # Force the existing host timing source to deterministic zero in capture;
    # use the real clock for the separate UI_FRAME_BENCH process.
    host = replace_once(host, "static double now_ns(void)\n{",
        "static double now_ns(void)\n{\n    if (!getenv(\"UI_FRAME_BENCH\")) return 0;")
    host = replace_once(host, "static uint64_t host_ns(void)\n{",
        "static uint64_t host_ns(void)\n{\n    if (!getenv(\"UI_FRAME_BENCH\")) return 0;")
    host_path = dest / "host.c"
    host_path.write_text("#include <stdint.h>\n" + DECL + absolute_includes(host, HOST))
    fixture = replace_once(FIXTURE.read_text(), '#define X0X_DEV_PADCAL 1', '#define X0X_DEV_PADCAL 0')
    fixture = replace_once(fixture, '#include "../../host/x0x_host.c"', f'#include "{host_path}"')
    fixture_path = dest / "fixture.c"
    fixture_path.write_text(absolute_includes(fixture, FIXTURE))
    return fixture_path


def exact_read(stream, size):
    data = bytearray()
    while len(data) < size:
        chunk = stream.read(size-len(data))
        if not chunk:
            raise ValueError("truncated trace event")
        data.extend(chunk)
    return data


def consume(fd, output, result):
    counts, digests = {}, {}
    try:
        with os.fdopen(fd, "rb") as stream, output.open("w") as log:
            while True:
                head = stream.read(28)
                if not head:
                    break
                if len(head) != 28:
                    head += exact_read(stream, 28-len(head))
                kind, frame, x, y, w, h, size = struct.unpack("<7I", head)
                kind = chr(kind)
                if size > 240*240*4:
                    raise ValueError("invalid trace payload length")
                data = exact_read(stream, size)
                if kind == "B" and (not w or not h or x+w > 240 or y+h > 240):
                    result.setdefault("invariant_error", "blit outside panel")
                counts[kind] = counts.get(kind, 0)+1
                digests.setdefault(kind, hashlib.sha256()).update(data)
                record = dict(kind=kind, frame=frame, x=x, y=y, w=w, h=h,
                              bytes=size, sha256=hashlib.sha256(data).hexdigest())
                log.write(json.dumps(record, separators=(",", ":"))+"\n")
        result.update(counts=counts, sha256={k: v.hexdigest() for k, v in digests.items()})
    except Exception as error:
        result["reader_error"] = str(error)


def run_capture(name, ui, out, frames, short=False):
    dest = out / name
    fixture = prepare(ui, dest)
    read_fd, write_fd = os.pipe()
    result = {}
    reader = threading.Thread(target=consume, args=(read_fd, dest/"capture.events.jsonl", result))
    reader.start()
    env = os.environ.copy()
    # Refuse inherited fixture/compiler substitutions that could hide production.
    for key in ("SMC_UI_SOURCE_FILE", "X0X_RAW", "SMC_UI_SEND_IMPLEMENTED", "UI_FRAME_BENCH", "UI_FRAME_SHORT"):
        env.pop(key, None)
    env.update(SMC_UI_OUT=str(dest), SMC_UI_TEST_FILE=str(ROOT/"tests/host/ui_frame_identity_test.c"),
               X0X_HOST_DEFS=f'-DUI_FRAME_FIXTURE=\"{fixture}\" -DUI_FRAME_DECAY_FILE=\"{dest / "decay.inc"}\"',
               UI_FRAME_TRACE_FD=str(write_fd), UI_FRAME_FRAMES=str(frames))
    if short:
        env["UI_FRAME_SHORT"] = "1"
    try:
        with (dest/"capture.build.log").open("w") as build_log:
            process = subprocess.Popen(["sh", "tests/host/smc_ui_test.sh"], cwd=ROOT,
                env=env, pass_fds=(write_fd,), stdout=build_log, stderr=subprocess.STDOUT)
        os.close(write_fd)
        write_fd = None
        code = process.wait()
    finally:
        # Popen errors must not leave a reader waiting for an open writer.
        if write_fd is not None:
            os.close(write_fd)
        reader.join()
    result["exit_code"] = code
    (dest/"capture.summary.json").write_text(json.dumps(result, indent=2)+"\n")
    return dest, result


def require_capture(result):
    if result["exit_code"] or result.get("reader_error") or result.get("invariant_error"):
        raise ValueError(f"capture failed: {result}")
    missing = set("BDEFACVHPSRQ") - set(result["counts"])
    if missing:
        raise ValueError(f"missing event kinds: {missing}")


def compare(a, b):
    with (a/"capture.events.jsonl").open() as left, (b/"capture.events.jsonl").open() as right:
        for number, pair in enumerate(itertools.zip_longest(left, right), 1):
            if pair[0] != pair[1]:
                raise ValueError(f"identity mismatch at event {number}: {a.name} / {b.name}: {pair}")


def benchmark_pair(baseline, candidate):
    env = os.environ.copy()
    env["UI_FRAME_BENCH"] = "1"
    # A fresh process, tracing disabled; retain independent repeated samples.
    with (baseline/"capture.timing.jsonl").open("w") as before, (candidate/"capture.timing.jsonl").open("w") as after:
        for _ in range(5):
            for dest, output in ((baseline,before), (candidate,after)):
                subprocess.run([str(dest/"test")], cwd=ROOT, env=env, stdout=output, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--base-ui", type=Path)
    parser.add_argument("--frames", type=int, default=4096)
    args = parser.parse_args()
    if args.frames < 4096:
        parser.error("at least 4096 frames per palette are required")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    base_bytes = args.base_ui.read_bytes() if args.base_ui else subprocess.check_output(
        ["git", "show", f"{BASE}:firmware/src/app/ui.c"], cwd=ROOT)
    blob = hashlib.sha1(b"blob "+str(len(base_bytes)).encode()+b"\0"+base_bytes).hexdigest()
    if blob != BASE_BLOB:
        raise ValueError(f"incorrect base ui.c blob: {blob}")
    base = base_bytes.decode()
    baseline, result = run_capture("base", base, out, args.frames)
    require_capture(result)
    repeat, result = run_capture("base-repeat", base, out, args.frames)
    require_capture(result)
    compare(baseline, repeat)
    wrong = replace_once(base, "lcd_blit(x + c0 * 8u, y + r0, w, h, cv_px);",
                         "lcd_blit(x + c0 * 8u + 1u, y + r0, w, h, cv_px);")
    _, result = run_capture("wrong-blit", wrong, out, args.frames, short=True)
    if result.get("invariant_error") != "blit outside panel" or result.get("reader_error"):
        raise ValueError("wrong blit did not fail the panel-coordinate assertion")
    wrong = replace_once(base, "*h = (uint8_t)(*h - (*h >> 3) - (*h ? 1 : 0));",
                         "*h = (uint8_t)(*h - (*h >> 3));")
    bad, result = run_capture("wrong-decay", wrong, out, args.frames, short=True)
    if not result["exit_code"] or "eng_hit[kit][voice] == value" not in (bad/"capture.build.log").read_text():
        raise ValueError("wrong decay did not fail the real UI decay assertion")
    candidate, result = run_capture("candidate", UI.read_text(), out, args.frames)
    require_capture(result)
    compare(baseline, candidate)
    benchmark_pair(baseline, candidate)
    print(f"PASS: repeatable base, two detected mutations, identical ordered events. Evidence: {out}")
    print("Timing samples captured; Orchestrator must judge gain and native RAM/code budgets.")


if __name__ == "__main__":
    main()
