#!/usr/bin/env python3
"""Extract pinned, actual Synth One scheduler bodies into a C++ oracle.

Only Objective-C container plumbing, class qualification, output buffers, and
the explicitly excluded global-FX suffix are adapted. All source identities,
extracted-body identities and exact-count substitutions are recorded.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

UPSTREAM = "6466a3715c96b7ecf1dd255a218cbd571408a314"
CONTRACT_SHA = "8125bf7e84651291620c6a64d30a49d0db12fecc8c8635a06eaf1c195cce326c"
RECEIPT_SHA = "bcc78cf484067a5863ae041efca070a1e74c124e42c6c5ead7196331b362e823"
PINS = {
    "S1DSPKernel+toggleKeys.mm": (6142, "dbffc5a69ae0ee4da88b67291d50b52386a8947d", "f03340bf1f8a2cc4401db47b4fb55b5b73c1f1d6adf0a9e0bbd65274f6b01b05"),
    "S1DSPKernel+startStopNotes.mm": (2743, "ad445754be0a4041bab70a12a1341ef67bb5da20", "f9b779fccaec8cae75b20fe54965e512d023b4ba52b3422a6f5f8ee0164a1725"),
    "S1DSPKernel+reset.mm": (1135, "0672082715066745750541804009745821725389", "304a59e8a03da909b9b9c781fa9f566834514ef9d4324a144f2c37f00d87c643"),
    "S1DSPKernel+process.mm": (13463, "4299be29a55fd89d5f264bce28bfe155307c3ca6", "3fa142e20a4e51869b796743a61e58e04c34e73f81ca6736f5c456a81cadce53"),
}
METHODS = [
    ("S1DSPKernel+toggleKeys.mm", "turnOnKey(int noteNumber, int velocity)"),
    ("S1DSPKernel+toggleKeys.mm", "turnOnKey(int noteNumber, int velocity, float frequency)"),
    ("S1DSPKernel+toggleKeys.mm", "turnOffKey(int noteNumber)"),
    ("S1DSPKernel+startStopNotes.mm", "startNote(int noteNumber, int velocity)"),
    ("S1DSPKernel+startStopNotes.mm", "startNote(int noteNumber, int velocity, float frequency)"),
    ("S1DSPKernel+startStopNotes.mm", "stopNote(int noteNumber)"),
    ("S1DSPKernel+startStopNotes.mm", "stopAllNotes()"),
    ("S1DSPKernel+reset.mm", "resetDSP()"),
    ("S1DSPKernel+reset.mm", "reset()"),
    ("S1DSPKernel+reset.mm", "resetSequencer()"),
]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read_verified(path: Path, pin: tuple[int, str, str]) -> str:
    raw = path.read_bytes()
    size, blob, digest = pin
    require(len(raw) == size, f"source byte length mismatch: {path}")
    require(git_blob(raw) == blob, f"source Git blob mismatch: {path}")
    require(sha(raw) == digest, f"source SHA-256 mismatch: {path}")
    return raw.decode("utf-8")


def method(text: str, suffix: str) -> tuple[str, int, int]:
    signature = "void S1DSPKernel::" + suffix + " {"
    require(text.count(signature) == 1, f"signature occurrence mismatch: {suffix}")
    start = text.index(signature)
    opening = start + len(signature) - 1
    depth = 0
    state = "code"
    i = opening
    while i < len(text):
        ch = text[i]
        nxt = text[i:i + 2]
        if state == "line":
            if ch == "\n":
                state = "code"
        elif state == "block":
            if nxt == "*/":
                state = "code"
                i += 1
        elif state in ("string", "char"):
            if ch == "\\":
                i += 1
            elif ch == ('"' if state == "string" else "'"):
                state = "code"
        elif nxt == "//":
            state = "line"
            i += 1
        elif nxt == "/*":
            state = "block"
            i += 1
        elif ch == '"':
            state = "string"
        elif ch == "'":
            state = "char"
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1], text.count("\n", 0, start) + 1, text.count("\n", 0, i) + 1
        i += 1
    raise SystemExit(f"unclosed method: {suffix}")


class Adapter:
    def __init__(self, text: str):
        self.text = text
        self.edits: list[dict] = []

    def replace(self, old: str, new: str, expected: int, reason: str) -> None:
        actual = self.text.count(old)
        require(actual == expected, f"adapter occurrence mismatch ({reason}): expected {expected}, got {actual}")
        self.text = self.text.replace(old, new)
        self.edits.append({"reason": reason, "expected_occurrences": expected, "from": old, "to": new})


def objective_c(a: Adapter, name: str) -> None:
    if name in ("startNote(int noteNumber, int velocity, float frequency)", "stopNote(int noteNumber)"):
        a.replace("NSValue* value = heldNoteNumbers[i];\n        [value getValue:&existingNote];", "existingNote = heldNoteNumbers[i];", 1, "read fixed held-record value instead of NSValue copy")
    if name == "startNote(int noteNumber, int velocity, float frequency)":
        a.replace("[heldNoteNumbers removeObjectAtIndex:i];", "heldNoteNumbers.remove(i);", 1, "remove held record by index")
        a.replace("NSValue *value = [NSValue valueWithBytes:&note objCType:@encode(NoteNumber)];\n    [heldNoteNumbers insertObject:value atIndex:0];", "heldNoteNumbers.insert(note, 0);", 1, "copy NoteNumber into held-record head")
    if name == "stopNote(int noteNumber)":
        a.replace("NSInteger index = -1;", "int index = -1;", 1, "portable signed held-record index")
        a.replace("[heldNoteNumbers removeObjectAtIndex:index];", "heldNoteNumbers.remove(index);", 1, "remove held record by index")
    if name in ("stopAllNotes()", "resetDSP()"):
        a.replace("[heldNoteNumbers removeAllObjects];", "heldNoteNumbers.clear();", 1, "clear fixed held-record list")
    if name in ("startNote(int noteNumber, int velocity, float frequency)", "stopAllNotes()", "resetDSP()"):
        a.replace("[heldNoteNumbersAE updateWithContentsOfArray:heldNoteNumbers];", "heldNoteNumbersAE = heldNoteNumbers;", 1, "sequential snapshot copy preserving held-record iteration order")
    if name == "stopNote(int noteNumber)":
        a.replace("[heldNoteNumbersAE updateWithContentsOfArray: heldNoteNumbers];", "heldNoteNumbersAE = heldNoteNumbers;", 1, "sequential snapshot copy preserving held-record iteration order")


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--kernel-dir", type=Path, required=True)
    p.add_argument("--contract-dir", type=Path, help="Optional private input-recovery receipt cross-check; upstream pins are always enforced")
    p.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parent)
    args = p.parse_args()
    if args.contract_dir is not None:
        require(sha((args.contract_dir / "SOURCE-CONTRACT.md").read_bytes()) == CONTRACT_SHA, "contract SHA mismatch")
        receipt_bytes = (args.contract_dir / "SOURCE-HASHES.json").read_bytes()
        require(sha(receipt_bytes) == RECEIPT_SHA, "source receipt SHA mismatch")
        receipt = json.loads(receipt_bytes)
        for name, pin in PINS.items():
            rows = [r for r in receipt["input_files"] if r["path"] == "AudioKitSynthOne/DSP/Kernel/" + name]
            require(len(rows) == 1, "source receipt binding missing or ambiguous: " + name)
            r = rows[0]
            require((r["bytes"], r["git_blob_sha"], r["sha256"]) == pin and r["commit"] == UPSTREAM, "source receipt pin mismatch: " + name)
    source = {name: read_verified(args.kernel_dir / name, pin) for name, pin in PINS.items()}
    fragments = []
    extraction = []
    for file, name in METHODS:
        original, first, last = method(source[file], name)
        a = Adapter(original)
        a.replace("void S1DSPKernel::", "void Oracle::", 1, "bind actual body to oracle class")
        objective_c(a, name)
        require("NSValue" not in a.text and "NSInteger" not in a.text and "[heldNote" not in a.text, "Objective-C residue: " + name)
        fragments.append(f"// Actual upstream {file}:{first}-{last}\n" + a.text)
        extraction.append({"path": file, "signature": name, "lines": [first, last], "extracted_sha256": sha(original.encode()), "adapted_sha256": sha(a.text.encode()), "substitutions": a.edits})

    file = "S1DSPKernel+process.mm"
    original, first, last = method(source[file], "process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset)")
    marker = "        // MONO: NoteState render output \"synthOut\" is mono"
    require(original.count(marker) == 1, "pre-FX cutoff occurrence mismatch")
    cut = original.index(marker)
    prefix = original[:cut]
    a = Adapter(prefix)
    a.replace("void S1DSPKernel::process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset) {", "void Oracle::process(unsigned frameCount, unsigned bufferOffset, float* outputL, float* outputR) {", 1, "portable output-buffer ABI")
    a.replace("float* outL = (float*)outBufferListPtr->mBuffers[0].mData + bufferOffset;", "float* outL = outputL + bufferOffset;", 1, "pass left base output pointer explicitly")
    a.replace("float* outR = (float*)outBufferListPtr->mBuffers[1].mData + bufferOffset;", "float* outR = outputR + bufferOffset;", 1, "pass right base output pointer explicitly")
    a.replace("for (AUAudioFrameCount frameIndex = 0;", "for (unsigned frameIndex = 0;", 1, "portable frame count type")
    a.replace("sp_phasor_compute(sp, lfo1Phasor, nil, &lfo1);", "sp_phasor_compute(sp, lfo1Phasor, nullptr, &lfo1);", 1, "portable null pointer")
    a.replace("sp_phasor_compute(sp, lfo2Phasor, nil, &lfo2);", "sp_phasor_compute(sp, lfo2Phasor, nullptr, &lfo2);", 1, "portable null pointer")
    a.replace("    ///MARK: RENDER LOOP: Render one audio frame", "    trace(0, 0); // comparison-only observer after actual block prologue\n\n    ///MARK: RENDER LOOP: Render one audio frame", 1, "comparison-only observer at block boundary")
    a.text += "        trace(1, frameIndex); // comparison-only observer after actual note prefix\n    }\n}"
    fragments.append(f"// Actual upstream {file}:{first}-{first + prefix.count(chr(10)) - 1}; global FX suffix deliberately excluded\n" + a.text)
    extraction.append({"path": file, "signature": "process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset)", "whole_method_lines": [first, last], "whole_method_sha256": sha(original.encode()), "prefix_lines": [first, first + prefix.count("\n") - 1], "extracted_sha256": sha(prefix.encode()), "adapted_sha256": sha(a.text.encode()), "substitutions": a.edits, "appended": "comparison-only frame observer and two lexical closing braces"})
    header = '''// Generated by generate.py from pinned actual Synth One scheduler bodies.
// MIT: Aurelius Prochazka / AudioKit. See upstream notices in source closure.
#pragma once
#include "../scheduler.hpp"

namespace s1scheduler {
class Oracle : public State {
public:
    void turnOnKey(int noteNumber, int velocity);
    void turnOnKey(int noteNumber, int velocity, float frequency);
    void turnOffKey(int noteNumber);
    void startNote(int noteNumber, int velocity);
    void startNote(int noteNumber, int velocity, float frequency);
    void stopNote(int noteNumber);
    void stopAllNotes();
    void resetDSP();
    void reset();
    void resetSequencer();
    void process(unsigned frameCount, unsigned bufferOffset, float* outputL, float* outputR);
};
} // namespace s1scheduler
'''
    cpp = '''// Generated by generate.py; actual pinned bodies with recorded adapters only.
// MIT: Aurelius Prochazka / AudioKit. See upstream notices in source closure.
#include "oracle.hpp"
#include <cmath>

namespace s1scheduler {
// Fixed direct-event adapter domain, matching original MIDI/polyphony limits.
static constexpr int S1_NUM_MIDI_NOTES = 128;
static constexpr int S1_MAX_POLYPHONY = 6;
static constexpr float S1_RELEASE_AMPLITUDE_THRESHOLD = 0.01f;
using AEArrayToken = Held*;
static AEArrayToken AEArrayGetToken(Held& records) { return &records; }
static void* AEArrayGetItem(AEArrayToken token, unsigned index) {
    return &(*token)[index];
}
// Direct-note/pre-FX oracle excludes all four global delay states. Only their
// reset calls are represented by this explicit no-op; no tail fidelity claim.
static void sp_vdelay_reset(sp_data*, void*) {}

''' + "\n\n".join(fragments) + "\n\n} // namespace s1scheduler\n"
    manifest = {
        "schema": 1,
        "kind": "actual-upstream-direct-scheduler-extraction",
        "upstream": {"repository": "AudioKit/AudioKitSynthOne", "commit": UPSTREAM},
        "contract_sha256": CONTRACT_SHA,
        "source_receipt_sha256": RECEIPT_SHA,
        "inputs": [{"path": "AudioKitSynthOne/DSP/Kernel/" + name, "bytes": pin[0], "git_blob_sha": pin[1], "sha256": pin[2]} for name, pin in PINS.items()],
        "extractions": extraction,
        "generated": [{"path": "oracle.hpp", "sha256": sha(header.encode())}, {"path": "oracle.cpp", "sha256": sha(cpp.encode())}],
        "preconditions": ["Sequential direct-event calls only; no cross-thread AEArray timing claim", "Arp disabled and original sequencer.reset(true) leaves no pending notes before processing", "Caller enforces held capacity of 128 distinct valid MIDI pitches", "Source-compatible State supplies actual per-note DSP, shared tables/LFOs/PRNG, independent actual parameter ports and no-op notification methods"],
        "exclusions": ["Global FX suffix after eligible-note render", "Four variable-delay reset functions are explicit pre-FX no-ops", "AEManagedValue and Objective-C cross-thread snapshot timing", "Arpeggiator/pending-event execution", "AU event slicing, sustain/app wrapper and MIDI-origin policy", "CPU/resource/native/device or complete engine acceptance"],
        "local_adapters": ["AEArray token is the address of the independently copied Held snapshot; item returns the corresponding NoteNumber address", "Direct-event domain constants are MIDI 128, six poly slots and source 0.01f release threshold", "Namespaced sp_vdelay_reset(sp_data*,void*) is the documented pre-FX no-op"],
        "notes": ["No scheduler/allocation/ADSR/frequency/LFO arithmetic rewritten", "Source body comments preserved", "Traces observe state only and must not mutate it", "State is caller-owned common adaptation; its initialization and helper closure require separate verification"],
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "oracle.hpp").write_text(header)
    (args.output_dir / "oracle.cpp").write_text(cpp)
    (args.output_dir / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"methods": len(extraction), "generated": manifest["generated"]}, indent=2))


if __name__ == "__main__":
    main()
