#!/usr/bin/env python3
"""Prove fourteen focused checks reject real production mutations.

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
        'else if ((smc.locked ? smc.lock_view : smc.view) == V_SEQ)',
        'else if (smc.view == V_SEQ)',
    ),
    "filter-shows-false-solo": (
        # Strip 8 row 2 now resolves before the SMC_VIEW table (its V_PART entry is unreachable).
        'part_is_303(smc.part) ? "GEN" : "PART";',
        'part_is_303(smc.part) ? "GEN" : "S";',
    ),
    "static-knob-arc": (
        '(float)smc_scaled(k) / 127.0f', '0.5f',
    ),
    "cryptic-footer-returned": (
        '    cv_commit_popup_knobs();',
        '    cv_text(5, 69, &FONT_T, "> R << >> [< >] ^ v < >", C_GRAY);\n    cv_commit_popup_knobs();',
    ),
    "selection-follows-touch": (
        "if (n == selected) cv_rect(x + 1, 0, 28, 13, UI_N_SELECT);",
        "if (n == smc_ui.strip) cv_rect(x + 1, 0, 28, 13, UI_N_SELECT);",
    ),
    "visible-kit-requires-part": (
        "kit >= 0 ? smc.sel[kit] - smc_drum_track(kit, 0) :",
        "kit >= 0 ? (smc.part == kit ? smc.sel[kit] - smc_drum_track(kit, 0) : -1) :",
    ),
    "main-mix-ignores-channel": (
        "(smc_page_now() == 3 ? 7 : part_position(smc.mixsel))",
        "(smc_page_now() == 3 ? 7 : part_position(smc.part))",
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
        "        smc_show(n, 0, r);",   # #737 peek: the fader shows through smc_show
        '        smc_show(n, 0, r); say("SMC OLD BAR", 0);',
    ),
}
OUT = Path(os.environ.get("SMC_UI_NEGATIVE_OUT", ROOT / "build/host/smc-ui-negative"))
OUT.mkdir(parents=True, exist_ok=True)
# Reuse the real app/framebuffer harness, but isolate each fault from unrelated
# legacy geometry assertions. The unmodified control MUST pass every check.
DRIVER = r'''
#define SMC_UI_MAIN legacy_ui_main
#include "@HARNESS@"
#undef SMC_UI_MAIN
int main(void)
{
    const char *which=getenv("SMC_NEGATIVE_CASE");
    int n=which?atoi(which):-1;
    if(n<3) {
        test_drum_titles();
        /* Deliberately diverge live SMC view too: captured view must win. */
        setup(V_SEQ,PART_808);CHECK(smc_lock_toggle());smc.view=V_MIX;
        knob(0,1);ui_frame();check_drum_titles(PART_808,0,1);
    } else if(n<6) test_action_captions_and_knobs();
    else if(n<9) {
        setup(V_MIX,PART_303A);smc.mixsel=PART_BRK;
        if(n==7){smc_page_set(1);smc.sel[0]=2;}
        knob(0,1);ui_frame();
        CHECK(fb[MAIN_Y*240+2*30+2]==swap16(UI_N_SELECT));
        CHECK(fb[MAIN_Y*240+2]==swap16(C_BLACK));
    } else {
        setup(V_HOME,PART_303A);fader(0,0);CHECK(smc_ui_active());
        if(n==9){expire();fader(0,0);CHECK(!smc_ui_active());}
        if(n==10)expire();
        if(n==11){smc_ui_cancel();CHECK(!smc_ui.on);}
        if(n==12){clip_forces=0;smc_ui_cancel();CHECK(clip_forces>0);}
        if(n==13)CHECK(ui.msg_until==0);
    }
    return 0;
}
'''
driver = OUT / "focused.c"
driver.write_text(DRIVER.replace("@HARNESS@", str(ROOT / "tests/host/smc_ui_test.c")))

def run_case(source, output, case):
    env = dict(os.environ, SMC_UI_SOURCE_FILE=str(source), SMC_UI_OUT=str(output),
               SMC_UI_TEST_FILE=str(driver), SMC_NEGATIVE_CASE=str(case))
    return subprocess.run(["sh", "tests/host/smc_ui_test.sh"], cwd=ROOT, env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

for name, (old, _) in MUTANTS.items():
    if text.count(old) != 1:
        raise RuntimeError(f"{name}: expected one production anchor, found {text.count(old)}")

# Compile the clean control once, then exercise the same case for each mutant.
control = run_case(SOURCE, OUT / "control", 0)
(OUT / "control.log").write_text(control.stdout)
if control.returncode:
    raise RuntimeError("unmodified control failed; mutations cannot count as proof\n" + control.stdout)
for case, (name, (old, new)) in enumerate(MUTANTS.items()):
    env = dict(os.environ, SMC_NEGATIVE_CASE=str(case))
    control = subprocess.run([str(OUT / "control/test")], cwd=ROOT, env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if control.returncode:
        raise RuntimeError(f"{name}: unmodified control failed\n{control.stdout}")
    with tempfile.TemporaryDirectory(prefix="smc-ui-negative-") as tmp:
        candidate = Path(tmp) / "smc_mixer.c"
        candidate.write_text(text.replace(old, new, 1))
        result = run_case(candidate, OUT / name, case)
        (OUT / f"{name}.log").write_text(result.stdout)
        failures = [line for line in result.stdout.splitlines() if line.startswith("FAIL ")]
        if result.returncode == 0 or not failures:
            raise RuntimeError(f"{name}: expected a runtime assertion failure, exit={result.returncode}\n{result.stdout}")
        print(f"PASS negative {name}: {failures[0]}", flush=True)
print(f"smc_ui_negative: PASS ({len(MUTANTS)} clean controls and runtime faults; compile failures excluded)")
