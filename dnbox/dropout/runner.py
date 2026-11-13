#!/usr/bin/env python3
"""Run the pinned DnBOX ELF/package with an emulator-only sample and observer.

No firmware build or device transport. All output goes to a new directory.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from fixture import FIRMWARE_SOURCE, RUNNER_SOURCE, FACTORY_PACK_SHA256, FLASH_BYTES, make_fixture, sha256

PINS = {
    "elite": {"elf": "720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea",
              "fwsc": "95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6"},
    "pro": {"elf": "7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535",
            "fwsc": "ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8"},
}
REQUIRED = dict(x0x_dbg=52, audio_xruns=4, eng_prof=32, eng_prof_frames=4,
                eng_job_cost=20, eng_plan=1, eng_c1_mask=1, eng_stage=4,
                eng_mastered=4, eng_do_master=1, eng_job_arg=4, brk_silent_now=4,
                usr_nz=3, nloops=4, pack_n=4, kbuf=1024, dry_l=1024, dry_r=1024)
RAM_BASE, RAM_BYTES = 0x01C00000, 1 << 20


def verified_bytes(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    if sha256(data) != expected:
        raise ValueError(f"{path}: SHA256 mismatch; expected {expected}, got {sha256(data)}")
    return data


def inventory(inputs: dict) -> dict:
    report = {"status": "INPUTS_PRESENT", "missing": [], "mismatched": [], "inputs": {}}
    for name, (path, expected) in inputs.items():
        item = {"path": str(path), "expected_sha256": expected}
        if not path.is_file():
            report["missing"].append(name)
        else:
            item["sha256"] = sha256(path.read_bytes())
            if expected and item["sha256"] != expected:
                report["mismatched"].append(name)
        report["inputs"][name] = item
    if report["missing"] or report["mismatched"]:
        report["status"] = "BLOCKED_INPUTS"
    return report


def elf_symbols(data: bytes) -> dict[str, tuple[int, int]]:
    """Return unique canonical ELF32 LE object names as (address, size).

    LTO suffixes are removed. Ambiguous local names are omitted, so a consumer
    requiring one must reject the missing entry rather than guess an address.
    """
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected an ELF32 little-endian symbol-bearing file")
    header = struct.unpack_from("<HHIIIIIHHHHHH", data, 16)
    shoff, shsize, count = header[5], header[10], header[11]
    if shsize != 40 or count == 0 or shoff + shsize * count > len(data):
        raise ValueError("truncated or unsupported ELF section table")
    sections = [struct.unpack_from("<10I", data, shoff + i * shsize) for i in range(count)]
    symbols = {}
    for section in sections:
        if section[1] != 2:
            continue
        offset, size, link, stride = section[4], section[5], section[6], section[9]
        if stride != 16 or size % stride or offset + size > len(data) or link >= count:
            raise ValueError("invalid ELF symbol table")
        strings = sections[link]
        if strings[1] != 3 or strings[4] + strings[5] > len(data):
            raise ValueError("truncated ELF symbol string table")
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(offset, offset + size, stride):
            name_at, address, width, info, _, section_index = struct.unpack_from("<IIIBBH", data, at)
            if not section_index or info & 15 != 1:
                continue
            if name_at >= len(names) or b"\0" not in names[name_at:]:
                raise ValueError("invalid ELF symbol name")
            name = names[name_at:names.index(0, name_at)].decode("ascii", "strict")
            canonical = name.split(".lto_priv.", 1)[0]
            symbols.setdefault(canonical, set()).add((address, width))
    return {name: next(iter(matches)) for name, matches in symbols.items() if len(matches) == 1}


def observer_map(data: bytes) -> str:
    """Validate the pinned profile against actual ELF symbols, never guessed addresses."""
    symbols = elf_symbols(data)
    output, ranges = [], []
    for wanted, expected in REQUIRED.items():
        if wanted not in symbols:
            raise ValueError(f"{wanted}: missing or ambiguous ELF object symbol; cannot guess its address")
        address, size = symbols[wanted]
        if size != expected:
            raise ValueError(f"{wanted}: ELF size {size} differs from pinned profile size {expected}")
        if address < RAM_BASE or address + size > RAM_BASE + RAM_BYTES or (size % 4 == 0 and address % 4):
            raise ValueError(f"{wanted}: range is outside RAM or misaligned")
        if any(address < end and start < address + size for start, end in ranges):
            raise ValueError(f"{wanted}: overlapping ELF watch ranges")
        ranges.append((address, address + size))
        output.append(f"{wanted} 0x{address:08x} {size}\n")
    return "".join(output)


def run_environment(source: dict | None = None) -> dict:
    env = {key: value for key, value in (os.environ if source is None else source).items()
           if not key.startswith("FM1_")}
    env.update(FM1_STRICT="1", FM1_BENCH_KEYS="")
    return env


def _read_scene(path: Path, expected_sha256: str | None = None) -> tuple[dict, dict[str, bytes]]:
    contents = {"scene": path.read_bytes()}
    if expected_sha256 and sha256(contents["scene"]) != expected_sha256:
        raise ValueError(f"{path}: SHA256 mismatch after inventory")
    data = json.loads(contents["scene"])
    if (not isinstance(data, dict) or data.get("schema") != 1 or data.get("firmware_source") != FIRMWARE_SOURCE
            or data.get("kind") != "representative" or data.get("owner_scene_exact") is not False):
        raise ValueError("scene must explicitly identify this source and representative/non-exact status")
    start, end, shot = (data.get(name) for name in ("start_ms", "end_ms", "shot_ms"))
    if (any(type(x) is not int for x in (start, end, shot))
            or start < 0 or end - start < 300000 or shot < end + 100):
        raise ValueError("scene needs at least five measured minutes and a post-window capture tail")
    if not isinstance(data.get("intended"), dict) or not data.get("owner_unknown"):
        raise ValueError("scene must record chosen intentions and unknown owner settings")
    for key in ("keys", "usb"):
        name = Path(data[key])
        resolved = (path.resolve().parent / name).resolve()
        if name.is_absolute() or not resolved.is_relative_to(path.resolve().parent):
            raise ValueError("scene inputs must remain inside the scene directory")
        contents[key] = resolved.read_bytes()
        text = contents[key].decode("utf-8").strip()
        if not text:
            raise ValueError(f"empty {key} fixture")
        if key == "keys" and any(re.search(r":m[0-9a-fA-F]", token) for token in text.split(",")):
            raise ValueError("SMK synth MIDI must use USB, not the unsupported TRS script route")
        if key == "usb":
            for token in text.split(";"):
                match = re.fullmatch(r"([0-9]+):([0-9a-fA-F]{6})", token.strip())
                if not match or int(match[1]) > shot:
                    raise ValueError("USB scene needs one complete three-byte MIDI message per timed item")
                message = bytes.fromhex(match[2])
                if message[0] not in (0x81, 0x82, 0x91, 0x92) or any(x > 127 for x in message[1:]):
                    raise ValueError("this fixture permits only held-note events for synth channels2/3")
        data[key + "_path"] = str(resolved)
        data[key + "_sha256"] = sha256(contents[key])
    data["scene_sha256"] = sha256(contents["scene"])
    return data, contents


def load_scene(path: Path) -> dict:
    """Validate and hash each scene file from exactly the same byte read."""
    return _read_scene(path)[0]


def capture_execution_inputs(directory: Path, sources: dict, mapping: str) -> tuple[dict, dict]:
    """Capture verified inputs in a new private directory and return scene/manifest.

    sources maps names to (path, expected_sha256), including emulator/fwsc/scene
    and optionally elf/pack. The expected emulator/scene hashes come from the
    initial inventory; firmware/package/pack hashes retain their pinned values.
    """
    directory = directory.resolve()
    directory.mkdir(mode=0o700)
    names = dict(emulator="fm1-live", fwsc="firmware.fwsc", elf="x0x.elf",
                 pack="factory-break-pack.bin", scene="scene.source.json",
                 keys="scene.keys", usb="scene.usb", observer_map="observer.map")
    if not {"emulator", "fwsc", "scene"}.issubset(sources) or set(sources) - {"emulator", "fwsc", "elf", "pack", "scene"}:
        raise ValueError("execution capture needs emulator/fwsc/scene and only recognized input roles")
    if any(not expected for _, expected in sources.values()):
        raise ValueError("every captured source needs an expected SHA256")
    scene, contents = _read_scene(sources["scene"][0], sources["scene"][1])
    originals = {name: str(path.resolve()) for name, (path, _) in sources.items()}
    originals.update(keys=scene["keys_path"], usb=scene["usb_path"])
    for name, (path, expected) in sources.items():
        if name != "scene":
            contents[name] = verified_bytes(path, expected)
    contents["observer_map"] = mapping.encode("utf-8")
    manifest = {}
    for name, raw in contents.items():
        path = directory / names[name]
        with path.open("xb") as stream:
            stream.write(raw)
        digest = sha256(raw)
        verified_bytes(path, digest)
        path.chmod(0o500 if name == "emulator" else 0o400)
        manifest[name] = {"path": str(path), "sha256": digest, "bytes": len(raw),
                          "source_path": originals.get(name)}
    for key in ("keys", "usb"):
        scene[key + "_path"] = manifest[key]["path"]
    receipt = directory / "manifest.json"
    receipt.write_text(json.dumps(manifest, indent=2) + "\n")
    receipt.chmod(0o400)
    return scene, manifest


def _execution_inputs(emulator: Path, fwsc: Path, env: dict, manifest: dict | None) -> tuple[dict, dict]:
    checked = {}
    for name, item in (manifest or {}).items():
        raw = verified_bytes(Path(item["path"]), item["sha256"])
        checked[name] = {"path": item["path"], "sha256": sha256(raw), "bytes": len(raw)}
    paths = {"emulator": emulator, "fwsc": fwsc}
    for name, variable in (("keys", "FM1_BENCH_KEYS"), ("usb", "FM1_USB_SEND")):
        setting = env.get(variable, "")
        if setting.startswith("@"):
            paths[name] = Path(setting[1:])
        elif setting and manifest is not None:
            raise ValueError(f"{variable}: execution must use the captured script file")
    if env.get("FM1_DNBOX_MAP"):
        paths["observer_map"] = Path(env["FM1_DNBOX_MAP"])
    actual = {}
    for name, path in paths.items():
        path = path.resolve()
        if manifest is not None:
            if name not in checked or path != Path(checked[name]["path"]).resolve():
                raise ValueError(f"{name}: execution path does not match captured input")
            actual[name] = checked[name]
        else:
            raw = path.read_bytes()
            actual[name] = {"path": str(path), "sha256": sha256(raw), "bytes": len(raw)}
    return actual, checked


def _flash_state(path: Path) -> dict:
    try:
        with path.open("rb") as stream:
            before = os.fstat(stream.fileno())
            raw = stream.read()
            after = os.fstat(stream.fileno())
    except FileNotFoundError:
        return {"path": str(path), "state": "ABSENT"}
    current = path.stat()
    if ((before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns)
            or (after.st_dev, after.st_ino) != (current.st_dev, current.st_ino)
            or len(raw) != after.st_size):
        raise RuntimeError(f"flash changed during readback: {path}")
    return {"path": str(path), "state": "PRESENT", "device": after.st_dev, "inode": after.st_ino,
            "bytes": len(raw), "sha256": sha256(raw)}


def execute(emulator: Path, fwsc: Path, directory: Path, flash: Path, shot_ms: int, env: dict, usb: bool,
            *, input_manifest: dict | None = None, initial_flash_sha256: str | None = None) -> dict:
    directory.mkdir()
    (directory / "work").mkdir()
    command = [str(emulator), "--fwsc", str(fwsc), "--mhz", "360", "--shot", str(shot_ms), "--flash", str(flash)]
    if usb:
        command.append("--usb")
    record = {"argv": command, "returncode": None, "status": "PREPARING",
              "fm1_environment": {key: value for key, value in env.items() if key.startswith("FM1_")}}
    try:
        record["inputs"], record["captured_inputs"] = _execution_inputs(emulator, fwsc, env, input_manifest)
        before = _flash_state(flash)
        record["initial_flash"] = before
        if before["state"] == "PRESENT" and before["bytes"] != FLASH_BYTES:
            raise RuntimeError(f"initial flash size is not {FLASH_BYTES}: {flash}")
        if initial_flash_sha256 and before.get("sha256") != initial_flash_sha256:
            raise ValueError(f"initial flash SHA256 mismatch: {flash}")
        record["status"] = "RUNNING"
        (directory / "command.json").write_text(json.dumps(record, indent=2) + "\n")
        with (directory / "stdout.log").open("wb") as out, (directory / "stderr.log").open("wb") as err:
            result = subprocess.run(command, env=env, cwd=directory, stdout=out, stderr=err, check=False)
        record["returncode"] = result.returncode
        stderr = (directory / "stderr.log").read_bytes()
        if result.returncode or b"halt:" in stderr:
            raise RuntimeError(f"emulator failed or halted; inspect {directory}")
        after = _flash_state(flash)
        replaced = (before["state"] == "PRESENT" and after["state"] == "PRESENT"
                    and (before["device"], before["inode"]) != (after["device"], after["inode"]))
        record["flash_save"] = {"observed_file": after, "atomically_replaced": replaced,
                                "stderr_save_error": any(line.startswith(str(flash).encode() + b": ") for line in stderr.splitlines())}
        if after["state"] != "PRESENT":
            raise RuntimeError(f"final flash was not created: {flash}")
        if before["state"] == "PRESENT" and not replaced:
            raise RuntimeError(f"final flash was not atomically replaced: {flash}")
        if after["bytes"] != FLASH_BYTES:
            raise RuntimeError(f"final flash size is not {FLASH_BYTES}: {flash}")
        record["final_flash"] = dict(after, atomically_replaced=replaced, created=before["state"] == "ABSENT")
        record["status"] = "COMPLETED"
        return record
    except (OSError, ValueError, RuntimeError) as error:
        record["status"], record["error"] = "FAILED", str(error)
        raise
    finally:
        (directory / "command.json").write_text(json.dumps(record, indent=2) + "\n")


def observation_complete(result: dict) -> bool:
    """Evidence completeness, deliberately independent of the deadline outcome.

    This is not a firmware/hardware PASS. A measured dropout is a complete
    diagnostic result when the intended representative scene and its evidence
    are present. Missing source isolation or observer differences are not.
    """
    summary = result.get("analysis", {})
    brk = summary.get("break_evidence", {})
    state = result.get("scene_state", {})
    activity = result.get("808_activity", {})
    pcm = result.get("pcm", {})
    isolated = result.get("isolated_break", {}).get("state", {})
    return (summary.get("status") == "OBSERVED"
            and brk.get("loaded") == "OBSERVED" and brk.get("mix_contribution") == "OBSERVED"
            and brk.get("invalid_mix_halves") == 0
            and state.get("required_state_matches_at_all_observed_boundaries") is True
            and activity.get("status") == "INACTIVE_OBSERVED"
            and activity.get("all_observed_stretches_inactive") is True
            and activity.get("mix_guard_excluded_at_all_observed_stretches") is True
            and activity.get("coverage_complete") is True
            and pcm.get("status") == "OBSERVED" and pcm.get("numeric_output") == "NONZERO"
            and bool(summary.get("trace_sha256")) and pcm.get("trace_sha256") == summary["trace_sha256"]
            and isolated.get("status") == "NUMERICAL_BREAK_OUTPUT_OBSERVED"
            and all(result.get(name) is True for name in
                    ("observer_pcm_bit_exact", "observer_ram_bit_exact", "observer_flash_bit_exact")))


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--model", choices=PINS, default="elite")
    p.add_argument("--emulator", type=Path, default=Path("target/release/fm1-live"))
    p.add_argument("--elf", type=Path)
    p.add_argument("--fwsc", type=Path)
    p.add_argument("--pack", type=Path, default=Path("/workspace/build-1214/base-94197fc8/assets/factory-breaks/factory-break-pack.bin"))
    p.add_argument("--scene", type=Path, default=Path(__file__).with_name("representative") / "scene.json")
    p.add_argument("--out", type=Path, required=True)
    mode = p.add_mutually_exclusive_group()
    mode.add_argument("--execute", action="store_true", help="run the declared five-minute window plus a short isolated BREAK witness")
    mode.add_argument("--smoke", action="store_true", help="run an explicit nine-second diagnostic window; never substitutes for the declared five minutes")
    a = p.parse_args()
    a.elf = (a.elf or Path(f"/workspace/build-1214/artifacts/{a.model}/x0x.elf")).resolve()
    model_name = "Elite" if a.model == "elite" else "Pro"
    a.fwsc = (a.fwsc or Path(f"/workspace/build-1214/artifacts/{a.model}/DnBOX-0.0.26-SMK37-{model_name}-94197fc8.fwsc")).resolve()
    a.emulator, a.out = a.emulator.resolve(), a.out.resolve()
    report = inventory({"elf": (a.elf, PINS[a.model]["elf"]), "fwsc": (a.fwsc, PINS[a.model]["fwsc"]),
                        "pack": (a.pack, FACTORY_PACK_SHA256), "emulator": (a.emulator, None), "scene": (a.scene, None)})
    report.update(firmware_source=FIRMWARE_SOURCE, runner_source=RUNNER_SOURCE,
                  owner_scene_exact=False, model=a.model, scene_result="NOT_RUN")
    try:
        a.out.mkdir(parents=True, exist_ok=False)
        (a.out / "inputs.json").write_text(json.dumps(report, indent=2) + "\n")
        if report["status"] != "INPUTS_PRESENT":
            print(json.dumps(report, indent=2))
            return 2
        from scene import scene_map, check_window
        from activity import activity_map, check_808_inactive
        from pcm import analyze_pcm
        from isolation import break_only_keys, check_break_only
        elf = verified_bytes(a.elf, PINS[a.model]["elf"])
        symbols = elf_symbols(elf)
        mapping = observer_map(elf) + scene_map(symbols, sha256(elf)) + activity_map(symbols, sha256(elf))
        (a.out / "observer.map").write_text(mapping)
        sources = {name: (Path(item["path"]), item["sha256"]) for name, item in report["inputs"].items()}
        scene, captured = capture_execution_inputs(a.out / "execution-inputs", sources, mapping)
        # Reject an unsupported isolation recipe before any potentially long
        # firmware execution, and derive from the same captured bytes.
        isolated_keys = break_only_keys(verified_bytes(Path(captured["keys"]["path"]), captured["keys"]["sha256"]))
        (a.out / "scene.json").write_text(json.dumps(scene, indent=2) + "\n")
        if not (a.execute or a.smoke):
            print(json.dumps({"status": "PREFLIGHT_ONLY", "output": str(a.out), "scene_result": "NOT_RUN"}))
            return 0
        # A fresh package-created flash avoids inherited project/sample state.
        # --flash is written by the existing shot path even without guest input.
        emulator, fwsc = (Path(captured[name]["path"]) for name in ("emulator", "fwsc"))
        base_flash = a.out / "base.flash"
        execute(emulator, fwsc, a.out / "bootstrap", base_flash, 2000, run_environment(), False, input_manifest=captured)
        if not base_flash.is_file():
            raise RuntimeError("emulator did not produce the required base flash")
        initial, fixture = make_fixture(base_flash.read_bytes(), verified_bytes(Path(captured["pack"]["path"]), FACTORY_PACK_SHA256), "CRAMEN", 1)
        (a.out / "fixture.flash").write_bytes(initial)
        (a.out / "fixture.json").write_text(json.dumps(fixture, indent=2) + "\n")
        start = scene["start_ms"]
        end = start + 9000 if a.smoke else scene["end_ms"]
        shot = end + 1000 if a.smoke else scene["shot_ms"]
        window_args = dict(observed_start_ms=start, observed_end_ms=end) if a.smoke else {}
        pcm_hashes, ram_hashes, runs = {}, {}, {}
        for name in ("control", "observed"):
            run_dir = a.out / name
            flash = a.out / (name + ".flash")
            flash.write_bytes(initial)
            env = run_environment()
            env.update(FM1_BENCH_KEYS="@" + scene["keys_path"], FM1_USB_SEND="@" + scene["usb_path"],
                       FM1_DNBOX_PCM=str(run_dir / "audio.raw"), FM1_DUMPRAM=str(run_dir / "ram.bin"))
            if name == "observed":
                env.update(FM1_DNBOX_MAP=captured["observer_map"]["path"], FM1_DNBOX_TRACE=str(run_dir / "trace.jsonl"))
            runs[name] = execute(emulator, fwsc, run_dir, flash, shot, env, True,
                                 input_manifest=captured, initial_flash_sha256=sha256(initial))
            pcm = run_dir / "audio.raw"
            if not pcm.is_file() or pcm.stat().st_size == 0:
                raise RuntimeError("observer-capable emulator did not produce PCM")
            pcm_hashes[name] = sha256(pcm.read_bytes())
            ram = (run_dir / "ram.bin").read_bytes()
            if len(ram) != RAM_BYTES:
                raise RuntimeError("emulator did not produce the complete final RAM capture")
            ram_hashes[name] = sha256(ram)
        from trace import analyze_trace
        summary = analyze_trace(a.out / "observed" / "trace.jsonl", start, end, slot=1, out_dir=a.out / "analysis")
        scene_state = check_window(a.out / "analysis" / "per_half.csv", scene, summary, **window_args)
        activity = check_808_inactive(a.out / "analysis" / "stage_events.jsonl", summary)
        pcm = analyze_pcm(a.out / "observed" / "audio.raw", a.out / "observed" / "trace.jsonl", start, end)

        # A separate fresh-flash execution establishes numerical BREAK output.
        # It removes the four 909 toggles and sends no synth MIDI. The source
        # recipe hash is required by break_only_keys; arbitrary scripts cannot
        # silently be relabeled as an isolated source.
        isolated_path = a.out / "break-only.keys"
        isolated_path.write_bytes(isolated_keys)
        isolated_path.chmod(0o400)
        isolated_inputs = dict(captured)
        isolated_inputs["keys"] = {"path": str(isolated_path), "sha256": sha256(isolated_keys),
                                   "bytes": len(isolated_keys), "source_path": captured["keys"]["path"]}
        run_dir, flash = a.out / "break-only", a.out / "break-only.flash"
        flash.write_bytes(initial)
        env = run_environment()
        env.update(FM1_BENCH_KEYS="@" + str(isolated_path),
                   FM1_DNBOX_MAP=captured["observer_map"]["path"], FM1_DNBOX_TRACE=str(run_dir / "trace.jsonl"),
                   FM1_DNBOX_PCM=str(run_dir / "audio.raw"), FM1_DUMPRAM=str(run_dir / "ram.bin"))
        runs["break-only"] = execute(emulator, fwsc, run_dir, flash, start + 10000, env, True,
                                     input_manifest=isolated_inputs, initial_flash_sha256=sha256(initial))
        isolated_summary = analyze_trace(run_dir / "trace.jsonl", start, start + 9000, slot=1, out_dir=a.out / "break-analysis")
        isolated_pcm = analyze_pcm(run_dir / "audio.raw", run_dir / "trace.jsonl", start, start + 9000)
        isolated_state = check_break_only(a.out / "break-analysis" / "per_half.csv", scene, isolated_summary, isolated_pcm)
        isolated_receipt = {"status": isolated_state["status"], "state": isolated_state,
                            "analysis": isolated_summary, "pcm": isolated_pcm,
                            "input_contract": {"fresh_flash_sha256": sha256(initial),
                                "full_keys_sha256": captured["keys"]["sha256"],
                                "derived_keys_sha256": sha256(isolated_keys), "usb_midi_messages": 0}}
        result = {"status": "INCOMPLETE", "owner_scene_exact": False,
                  "duration_scope": "SHORT_DIAGNOSTIC" if a.smoke else "DECLARED_FIVE_MINUTE_WINDOW",
                  "observation_window": {"start_ms": start, "end_ms": end},
                  "pcm_sha256": pcm_hashes, "final_ram_sha256": ram_hashes,
                  "observer_pcm_bit_exact": pcm_hashes["control"] == pcm_hashes["observed"],
                  "observer_ram_bit_exact": ram_hashes["control"] == ram_hashes["observed"],
                  "observer_flash_bit_exact": runs["control"]["final_flash"]["sha256"] == runs["observed"]["final_flash"]["sha256"],
                  "control_final_flash_sha256": runs["control"]["final_flash"]["sha256"],
                  "observed_final_flash_sha256": runs["observed"]["final_flash"]["sha256"],
                  "analysis": summary, "pcm": pcm, "scene_state": scene_state,
                  "808_activity": activity, "isolated_break": isolated_receipt,
                  "human_audibility": "UNVERIFIED", "owner_scene_qualification": "NOT_CLAIMED"}
        if observation_complete(result):
            result["status"] = "OBSERVATION_COMPLETE"
        (a.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps({"status": result["status"], "output": str(a.out),
                          "duration_scope": result["duration_scope"], "deadline": summary["deadline"],
                          "owner_scene_exact": False}, indent=2))
        # A completed diagnosis may contain deadline misses. Exit0 means its
        # evidence was collected, not that a deadline/hardware gate passed.
        return 0 if result["status"] == "OBSERVATION_COMPLETE" else 2
    except (OSError, ValueError, KeyError, RuntimeError, struct.error) as e:
        p.exit(2, f"runner: {e}\n")


if __name__ == "__main__":
    raise SystemExit(main())
