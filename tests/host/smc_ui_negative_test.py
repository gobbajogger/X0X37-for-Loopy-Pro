#!/usr/bin/env python3
"""Prove the full UI regression detects runtime failures in fourteen real paths.

Each mutation changes one production operation in a private scratch source;
compiler failures do not count as an expected negative result. No device I/O.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "firmware/src/app/smc_mixer.c"
text = SOURCE.read_text()
MUTANTS = {
    "drum-seq-numbered": (
        'put_s(name, engine_voice_name(kit ? T_808 : T_909, track));',
        'put_i(name, track + 1);',
    ),
    "drum-seq-wrong-kit": (
        'put_s(name, engine_voice_name(kit ? T_808 : T_909, track));',
        'put_s(name, engine_voice_name(T_909, track));',
    ),
    "drum-seq-ignores-captured-view": (
        'else if ((smc.locked ? smc.lock_view : ui.view) == V_SEQ)',
        'else if (ui.view == V_SEQ)',
    ),
    "filter-shows-false-solo": (
        # Strip 8 row 2 now resolves before the SMC_VIEW table (its V_PART entry is unreachable).
        'is_303() ? "GEN" : "PART";', 'is_303() ? "GEN" : "S";',
    ),
    "static-knob-arc": (
        '(float)smc_scaled(k) / 127.0f', '0.5f',
    ),
    "cryptic-footer-returned": (
        '    cv_commit(3, 0, KNOB_Y);',
        '    cv_text(5, 69, &FONT_T, "> R << >> [< >] ^ v < >", C_GRAY);\n    cv_commit(3, 0, KNOB_Y);',
    ),
    "selection-follows-touch": (
        "if (n == selected) cv_rect(x + 1, 0, 28, 13, C_WHITE);",
        "if (n == smc_ui.strip) cv_rect(x + 1, 0, 28, 13, C_WHITE);",
    ),
    "visible-kit-requires-part": (
        "kit >= 0 ? ui.sel[kit] - smc_drum_track(kit, 0) :",
        "kit >= 0 ? (ui.part == kit ? ui.sel[kit] - smc_drum_track(kit, 0) : -1) :",
    ),
    "main-mix-ignores-channel": (
        "(smc_page_now() == 3 ? 7 : part_position(ui.mixsel))",
        "(smc_page_now() == 3 ? 7 : part_position(ui.part))",
    ),
    "duplicate-opens-overlay": (
        "    if (moved) {",
        "    if (1) { /* negative: duplicates reopen */",
    ),
    "timeout-disabled": (
        "!smc_ui_edge_alive(plat_ms(), smc_ui.until,\n                smc_held(&smc_states[peer]))",
        "0 /* negative control: no inactivity timeout */",
    ),
    "cancel-disabled": (
        "    smc_ui.on = 0;",
        "    smc_ui.on = 1; /* negative control: cannot dismiss */",
    ),
    "clip-redraw-disabled": (
        "{ extern void smk_clip_lcd_force(void); smk_clip_lcd_force(); }",
        "{ /* negative control: retain clip painter cache */ }",
    ),
    "yellow-notice-returned": (
        "    smc_ui_begin(n, knob);",
        '    smc_ui_begin(n, knob); say("SMC OLD BAR", 0);',
    ),
}
OUT = Path(os.environ.get("SMC_UI_NEGATIVE_OUT", ROOT / "build/host/smc-ui-negative"))
OUT.mkdir(parents=True, exist_ok=True)
for name, (old, new) in MUTANTS.items():
    if text.count(old) != 1:
        raise RuntimeError(f"{name}: expected one production anchor, found {text.count(old)}")
    with tempfile.TemporaryDirectory(prefix="smc-ui-negative-") as tmp:
        candidate = Path(tmp) / "smc_mixer.c"
        candidate.write_text(text.replace(old, new, 1))
        env = dict(os.environ, SMC_UI_SOURCE_FILE=str(candidate), SMC_UI_OUT=str(OUT / name))
        result = subprocess.run(["sh", "tests/host/smc_ui_test.sh"], cwd=ROOT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (OUT / f"{name}.log").write_text(result.stdout)
        if result.returncode == 0 or "FAIL tests/host/smc_ui_test.c:" not in result.stdout:
            raise RuntimeError(f"{name}: expected a runtime assertion failure, exit={result.returncode}\n{result.stdout}")
        failure = next(line for line in result.stdout.splitlines() if line.startswith("FAIL tests/host/smc_ui_test.c:"))
        print(f"PASS negative {name}: {failure}")
print(f"smc_ui_negative: PASS ({len(MUTANTS)} runtime failures detected; compile failures excluded)")
