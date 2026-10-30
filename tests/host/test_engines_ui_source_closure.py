#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-identity and build-routing gate; not an audio or hardware test.

The optimized DSP imports must remain exact Git blobs from the previously
verified branches. UI is from the reviewed DnBOX material branch. Compiling
the actual UI and exercising the device are separate mandatory checks.
"""
from pathlib import Path
import importlib.util
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PINS = """experiments/original_synth/chicago_engine_output.c 6df70adcf59ef9b588f08847ad6edc745d60e4a7
experiments/original_synth/chicago_engine_output.h 73bd4ede5060175a4abe9c7d6c19a06537217c87
experiments/original_synth/chicago_global_mg.c dce859f14344566e3fb8534460cb0e2acd83a0db
experiments/original_synth/chicago_global_mg.h 7187380510ad8dc08ec5df938ec1a145d3bcf6c9
experiments/original_synth/chicago_modulation.c 073e7af2e4cdc8765e508f30335603f590bcb154
experiments/original_synth/chicago_output.c c0adcc1f09b33093185a4d640c00c41388cea653
experiments/original_synth/chicago_output.h ea7ebf37f899cda4b9072d9b7ca1edffe9742024
experiments/original_synth/chicago_scheduler.c 869e3e3f5dc2efeb28998534718de965d2d719bc
experiments/original_synth/chicago_scheduler.h 3861153a4c8bd1b825a43cf1b492cb5b136c010d
experiments/original_synth/chicago_voice.c 39758fe0eaaca5323917e7bdc3350df99b74ae31
experiments/original_synth/chicago_voice.h 45b19fe7d5ab87226f454ba25292a6b031aaef04
experiments/original_synth/chicago_voice_owner.c 508042070418624fe00801cc5d95cfd7cca418ae
experiments/original_synth/chicago_voice_owner.h 4589844e133b8afc2b9cc142b375861dc9420096
experiments/original_synth/oscillator_motion.c 1f99fa130c7831f63592f154a83838f7bfd8640e
firmware/src/dsp/original_chicago_backend.c e9a53e604311b23a11fa773433f4fe0f12910d32
experiments/original_synth/chicago_chorus_span.h fa122964123593df78c074d4d12aca05fe47df3a
experiments/original_synth/chicago_clock_span.h 7786c93147ff5669c18845fa2432818f49fdac7e
experiments/original_synth/miami_fx_chain.c 651688c8c52ad1a0360c043459cf77d8fc19ca7d
experiments/original_synth/miami_fx_chain.h 4b6f74a6d84d2089497f8401720ae494483aaadc
experiments/original_synth/miami_ms20.c 263341344016770755c1c99fc1cebbd86480c58d
experiments/original_synth/miami_output.c 8adea53e2d6a19c450e38bea7923a6dd9ac21ef0
experiments/original_synth/miami_output.h 46cc11e8e5810679ca910fac780dfbc2b9550808
experiments/original_synth/miami_selected_voice.c acccca765db63a9179bd2b825c3cc58048300ae6
experiments/original_synth/miami_selected_voice.h dce65323f8be66d50e04c5ea279397e44de58cd6
experiments/original_synth/miami_voice_owner.c d729f21aba68145e4b53b0ffaa67db27e4cf26dc
experiments/original_synth/miami_voice_owner.h 416570cdd98b1942d89481700dec3a6d98e9b687
firmware/src/dsp/original_miami_backend.c caa3b2159a7bf48c1cd74dd324994a1d42f0c218
firmware/src/dsp/original_miami_backend.h 12be6db74340f2feb5a2a3d19d5a9e35172e3cd4
experiments/original_synth/miami_idle_clock.h efe52f8599bbd9b7c698f809a294732de484db93
experiments/original_synth/miami_interpolation.h 6258b044484a575d976109bd0a863274cccd041f
experiments/original_synth/miami_zero.h 62eaf66539ad3a36c99c3805a129dab74ee2e1fb
firmware/src/app/ui.c 2e51cc1ffa6026c98eeaba76774b041982f5e7ac
firmware/src/app/main_fm1.c 3aa31a25c0628fdfd7e3fbc5855a8cb55d1d55f3
firmware/src/app/boot_screen.h ac75a56515f29e882d7410c01c3193cc591103e2
firmware/src/app/ui_original_controls.h 9448c6db47424c3e8c78b13961db191623634cd8
experiments/original_synth/chicago_selector.c 678e5046475a51fc5c499b226d66e9d55af1ded8"""

def fail(message):
    raise SystemExit("FAIL engines-ui: " + message)

def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()

def main():
    for line in PINS.splitlines():
        path, expected = line.split()
        file = ROOT / path
        if not file.is_file():
            fail("missing " + path)
        actual = git("hash-object", path)
        if actual != expected:
            fail(path + ": " + actual + " != pinned " + expected)
    spec = importlib.util.spec_from_file_location("build_plan", ROOT / "tools/build.py")
    if not spec or not spec.loader:
        fail("cannot load tools/build.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    original, wobble = module.synth_build_config(
        {"X0X_ORIGINAL_SYNTH": "1", "X0X_WOBBLE": "0", "X0X_DEV_WOBBLE": "0"})
    if not original or wobble != "0":
        fail("original Chicago/Miami profile not enabled")
    if "experiments/original_synth/chicago_selector.c" not in module.ORIGINAL_SYNTH_UNITS:
        fail("Chicago selector source excluded from build")
    commands, _ = module.app_compile_commands(["-Os"], True, "0")
    plan = "\n".join(" ".join(map(str, command)) for command in commands)
    if "chicago_selector.c" not in plan or "original_chicago_backend.c" not in plan or "original_miami_backend.c" not in plan:
        fail("original synth compile closure missing")
    if "dsp/bass303.c" in plan or "experiments/wobble/wobble.c" in plan:
        fail("legacy synth object reintroduced to original synth build")
    ui_text = (ROOT / "firmware/src/app/ui.c").read_text()
    for required in (
        "R_BENGINE", "R_BPRESET", "R_BSTATUS", "engine_bass_select_preset(",
        'add_page("SOUND"', "control_glyph(", "draw_knobs(",
        "draw_header(", "draw_home(", "draw_fx(", "draw_mix(", "draw_song(",
        "draw_break(", "draw_drum_mix(", "draw_list(", "draw_ask(",
    ):
        if required not in ui_text:
            fail("missing production UI routing or renderer " + required)
    engine = (ROOT / "firmware/src/app/engine.h").read_text()
    if "#define X0X_BASS_PRESET_PREVIEW 1" not in engine or "#define X0X_BASS_TAGGED_SOUND 1" not in engine:
        fail("native original synth selector preset profile is disabled")
    # The legacy web/EDITOR_PROTOCOL.md G_ENGSEL / PRESET / RELOAD messages
    # are not implemented by firmware/src/app/editor.c. Local panel control
    # runs through R_BENGINE/R_BPRESET, not an invented SysEx migration.
    changed = git("diff", "--name-only", "cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21", "HEAD").splitlines()
    forbidden = (".fwsc", ".img", ".component", ".vst3")
    for path in changed:
        if path.endswith(forbidden) or path.endswith("_private.h") or "/private/" in path:
            fail("forbidden file in integration diff: " + path)
    print(f"PASS engines-ui source: {len(PINS.splitlines())} exact file blobs; Chicago/Miami build and UI selector/glyph routing present")
    print("Not an audio, rendered UI, Callgrind, target-build or device PASS.")

if __name__ == "__main__":
    main()
