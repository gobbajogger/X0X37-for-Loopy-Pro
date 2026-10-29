#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline stock export refusal tests and independent firmware/browser comparisons.

Pass --bank and --receipt for the private approved bank. Those bytes are never
committed. A source-extracted engine/plat_slot probe uses the real slot scanner,
pack parser and IMA decoder with simulated flash. It does not test target timing,
DSP rendering or hardware. The browser module independently serializes each FSMP
image, reads the archive and compares every decoded sample with the C decoder.
"""
import argparse
import io
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import stock_slots as stock  # noqa: E402

INPUT_BANK = INPUT_RECEIPT = None
EVIDENCE = None
CC = "cc"


def fixture_parts():
    return [{"name": name, "nsamples": n, "bars": bars,
             "adpcm": bytes(((i * 37 + k * 79) & 255) for i in range((n + 1) // 2))}
            for k, (name, n, bars) in enumerate(stock.STOCK)]


def source_function(text, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*?\)\s*\{", text, re.M)
    if not match:
        raise AssertionError(f"actual source function missing: {name}")
    depth = 0
    for i in range(text.index("{", match.start()), len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[match.start():i + 1]
    raise AssertionError(f"unterminated actual source function: {name}")


def mutate_u32(raw, offset, value):
    out = bytearray(raw)
    struct.pack_into("<I", out, offset, value)
    return bytes(out)


def repack(files):
    out = io.BytesIO()
    with zipfile.ZipFile(out, "w") as archive:
        for name, raw in files.items():
            archive.writestr(name, raw)
    return out.getvalue()


class StockTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parts = stock.approved_bank(INPUT_BANK) if INPUT_BANK else fixture_parts()
        cls.bank = INPUT_BANK or stock.break_pack.encode_pack(cls.parts)
        cls.images = [stock.encode_slot(parts, f"STOCK{k + 1}", stock.layout()[k][1])
                      for k, parts in enumerate(stock.stock_layout(cls.parts))]
        cls.bundle = (stock.build_distribution(INPUT_BANK, INPUT_RECEIPT, "elite", stock.PROFILE)
                      if INPUT_BANK else None)

    def need_private_input(self):
        if self.bundle is None:
            self.skipTest("approved private bank/receipt not provided; use --bank and --receipt")

    def test_full_byte_lengths_and_slot_bounds(self):
        self.assertEqual([len(p["adpcm"]) for p in self.parts], [35295, 52914, 42863])
        self.assertEqual([len(b) for b in self.images], [35807, 96289])
        self.assertEqual([stock.layout()[k][1] - len(b) for k, b in enumerate(self.images)], [46113, 26591])
        self.assertEqual(self.images[0][512:], self.parts[0]["adpcm"])
        self.assertEqual(self.images[1][512:], self.parts[1]["adpcm"] + self.parts[2]["adpcm"])
        for k, image in enumerate(self.images):
            got = stock.validate_slot(image, stock.layout()[k][1])
            self.assertEqual([p["adpcm"] for p in got], [p["adpcm"] for p in stock.stock_layout(self.parts)[k]])
        with self.assertRaises(stock.StockError):
            stock.encode_slot(self.parts, "ALL", stock.layout()[1][1])
        with self.assertRaises(stock.StockError):
            stock.encode_slot(self.parts[1:], "TOOBIG", stock.layout()[0][1])
        for part in self.parts:
            for _, size in stock.layout():
                self.assertLessEqual(len(stock.encode_slot([part], part["name"], size)), size)

    def test_encoder_and_audio_processing_are_never_called(self):
        names = ("ima_encode", "to_int16", "resample", "read_any_wav", "trim_silence", "apply_oldschool")
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in names:
                stack.enter_context(mock.patch.object(stock.sampleio, name, side_effect=AssertionError("audio mutation")))
            for k, group in enumerate(stock.stock_layout(self.parts)):
                self.assertEqual(stock.encode_slot(group, f"STOCK{k + 1}", stock.layout()[k][1]), self.images[k])
            if INPUT_BANK:
                self.assertEqual(stock.build_distribution(INPUT_BANK, INPUT_RECEIPT, "elite", stock.PROFILE), self.bundle)

    def test_odd_nibble_tail_is_kept(self):
        part = {"name": "ODD", "nsamples": 65, "bars": 1, "adpcm": bytes(32) + b"\xf1"}
        image = stock.encode_slot([part], "ODD", stock.layout()[0][1])
        self.assertEqual(image[-1], 0xf1)
        self.assertEqual(stock.validate_slot(image, stock.layout()[0][1])[0]["nsamples"], 65)

    def test_malformed_pack_refusals(self):
        raw = stock.break_pack.encode_pack([{"name": "ODD", "nsamples": 65, "bars": 1, "adpcm": bytes(33)}])
        cases = [raw[:19], raw[:-1], raw + b"\xff", b"BADMAGIC" + raw[8:], mutate_u32(raw, 12, 12345)]
        for off, value in ((8, 2), (10, 0), (10, 9), (32, 3), (20, 255), (33, 1)):
            bad = bytearray(raw)
            bad[off] = value
            if off >= 20:
                struct.pack_into("<I", bad, 16, zlib.crc32(bad[20:]))
            cases.append(bytes(bad))
        cases += [mutate_u32(raw, 28, 63), mutate_u32(raw, 36, 1), raw[:20] + b"ABCDEFGH" + raw[28:]]
        for bad in cases:
            with self.subTest(sha=stock.sha256(bad)):
                with self.assertRaises(stock.StockError):
                    stock.parse_bank(bad)
        bad = bytearray(raw)
        bad[-1] ^= 1
        with self.assertRaises(stock.StockError):
            stock.parse_bank(bad)

    def test_malformed_slot_refusals(self):
        raw, size = self.images[1], stock.layout()[1][1]
        cases = [raw[:479], raw[:511], raw[:-1], raw + b"\xff"]
        for off, value in ((0, 0), (4, 2), (6, 0), (6, 17), (7, 1), (8, 255), (24, 1),
                           (56, 89), (54, 1), (57, 4), (59, 1), (88, 1), (480, 0)):
            bad = bytearray(raw)
            bad[off] = value
            cases.append(bytes(bad))
        for off, value in ((16, 0xffffffff), (32, 1), (36, 0xffffffff), (40, 1), (44, 0),
                           (48, 0), (48, 32768), (60, len(raw)), (64, 0)):
            cases.append(mutate_u32(raw, off, value))
        bad = bytearray(raw)
        bad[-1] ^= 1
        cases.append(bytes(bad))
        for bad in cases:
            with self.subTest(sha=stock.sha256(bad)):
                with self.assertRaises(stock.StockError):
                    stock.validate_slot(bad, size)
        with self.assertRaises(stock.StockError):
            stock.validate_slot(raw, stock.layout()[0][1])

    def test_bad_input_guards(self):
        with self.assertRaises(stock.StockError):
            stock.encode_slot([], "EMPTY", stock.layout()[0][1])
        with self.assertRaises(stock.StockError):
            stock.encode_slot([self.parts[0]] * 17, "MANY", stock.layout()[1][1])
        for delta in ({"nsamples": 64}, {"bars": 8}, {"nsamples": -1}):
            with self.assertRaises(stock.StockError):
                stock.encode_slot([{**self.parts[0], **delta}], "BAD", stock.layout()[0][1])
        for model, profile in (("fm1", stock.PROFILE), ("elite", "legacy"), ("SMK-37 Pro", stock.PROFILE)):
            with self.assertRaises(stock.StockError):
                stock.build_distribution(self.bank, b"{}", model, profile)
        with mock.patch.dict(stock.package.SLOTMAP, {"smk37": [(0xC2000, 0x14000)]}):
            with self.assertRaises(stock.StockError):
                stock.layout()
        for bad in (self.bank[:-1], self.bank + b"\xff", b"\x00" + self.bank[1:]):
            with self.assertRaises(stock.StockError):
                stock.approved_bank(bad)

    def test_approved_pin_and_extraction_receipt(self):
        self.need_private_input()
        self.assertEqual(stock.sha256(INPUT_BANK), stock.BANK_SHA256)
        self.assertEqual(stock.validate_receipt(INPUT_RECEIPT)["bank_bytes"], len(INPUT_BANK))
        bad = bytearray(INPUT_BANK)
        bad[-1] ^= 1
        struct.pack_into("<I", bad, 16, zlib.crc32(bad[20:]))
        self.assertIsNotNone(stock.parse_bank(bad))
        with self.assertRaises(stock.StockError):
            stock.approved_bank(bad)
        for receipt in (b"not json", b"[]", b"{}", stock.json_bytes({"bank_bytes": stock.BANK_BYTES, "bank_sha256": "0" * 64})):
            with self.assertRaises(stock.StockError):
                stock.build_distribution(INPUT_BANK, receipt, "elite", stock.PROFILE)

    def test_reproducibility_and_per_model_archives(self):
        self.need_private_input()
        self.assertEqual(self.bundle, stock.build_distribution(INPUT_BANK, INPUT_RECEIPT, "elite", stock.PROFILE))
        for model in stock.MODELS:
            bundle = stock.build_distribution(INPUT_BANK, INPUT_RECEIPT, model, stock.PROFILE)
            manifest, files = stock.inspect_distribution(bundle, model, stock.PROFILE)
            meta, images = stock.slots.read_archive(files[manifest["archive"]])
            self.assertEqual(images, self.images)
            self.assertEqual((meta["model"], meta["profile"]), (model, stock.PROFILE))
            self.assertEqual([z["name"] for s in manifest["slots"] for z in s["zones"]], [p[0] for p in stock.STOCK])
            self.assertEqual([z["menu_index"] for s in manifest["slots"] for z in s["zones"]], [0, 1, 2])
            with self.assertRaises(stock.StockError):
                stock.inspect_distribution(bundle, "pro" if model == "elite" else "elite", stock.PROFILE)

    def test_distribution_damage_and_forged_metadata(self):
        self.need_private_input()
        _, files = stock.inspect_distribution(self.bundle, "elite", stock.PROFILE)
        cases = [self.bundle[:-22], b"bad zip", bytes(stock.MAX_ZIP_BYTES + 1)]
        for path in ("loops/CRAMEN.adpcm", "slots/slot2.fsmp", "provenance/stock-extraction.json"):
            edited = dict(files)
            edited[path] = bytes([files[path][0] ^ 1]) + files[path][1:]
            cases.append(repack(edited))
        for update in ({"model": "pro"}, {"profile": "legacy"}, {"slots": []}, {"files": []}, {"source": {}}):
            manifest = json.loads(files["manifest.json"])
            manifest.update(update)
            cases.append(repack({**files, "manifest.json": stock.json_bytes(manifest)}))
        for name in ("../escape", "/absolute", "unexpected.bin"):
            cases.append(repack({**files, name: b"extra"}))
        manifest = json.loads(files["manifest.json"])
        archive = manifest["archive"]
        edited = {**files, archive: files[archive][:-1] + bytes([files[archive][-1] ^ 1])}
        manifest["files"][archive] = {"bytes": len(edited[archive]), "sha256": stock.sha256(edited[archive])}
        edited["manifest.json"] = stock.json_bytes(manifest)
        cases.append(repack(edited))
        for bad in cases:
            with self.subTest(sha=stock.sha256(bad)):
                with self.assertRaises(stock.StockError):
                    stock.inspect_distribution(bad, "elite", stock.PROFILE)

    def test_inspect_only_explicit_replacement(self):
        self.need_private_input()
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in ("req", "layout", "read_slot", "write_slot", "restore", "restore_mapped"):
                stack.enter_context(mock.patch.object(stock.slots, name, side_effect=AssertionError("device access")))
            plan = stock.replacement_plan(self.bundle, "elite", stock.PROFILE, [1, 2])
            self.assertFalse(plan["executed"])
            self.assertIsNone(plan["write_command"])
            self.assertEqual(plan["mode"], "INSPECT_ONLY")
            self.assertTrue(all(a["prior"]["state"] == "unknown" for a in plan["actions"]))
            one = stock.replacement_plan(self.bundle, "elite", stock.PROFILE, [2], "CRAMEN")
            self.assertEqual(one["untouched_slots"], [1])
            self.assertEqual(one["actions"][0]["image_bytes"], 35807)
            _, files = stock.inspect_distribution(self.bundle, "elite", stock.PROFILE)
            baseline = files[f"stock-elite-{stock.PROFILE}.x0xslots"]
            before = stock.sha256(baseline)
            plan = stock.replacement_plan(self.bundle, "elite", stock.PROFILE, [1, 2], current_backup=baseline)
            self.assertEqual(stock.sha256(baseline), before)
            self.assertEqual(plan["actions"][1]["prior"]["header"]["nz"], 2)
            for selection, loop in (([], None), ([1], None), ([1, 1], None), ([3], "CRAMEN"),
                                    ([1, 2], "CRAMEN"), ([1], "UNKNOWN"), ([True], "CRAMEN")):
                with self.assertRaises(stock.StockError):
                    stock.replacement_plan(self.bundle, "elite", stock.PROFILE, selection, loop)
            with self.assertRaises(stock.StockError):
                stock.replacement_plan(self.bundle, "elite", stock.PROFILE, [1, 2], current_backup=baseline[:-1])
        self.assertNotIn("mido", sys.modules)
        self.assertNotIn("fm1_sample_upload", sys.modules)

    def test_exclusive_output_preserves_existing_files(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "user-samples.bin"
            path.write_bytes(b"keep user data")
            with self.assertRaises(FileExistsError):
                stock.publish_exclusive(path, b"new bytes")
            self.assertEqual(path.read_bytes(), b"keep user data")
            link = Path(directory) / "linked.bin"
            link.symlink_to(path)
            with self.assertRaises(FileExistsError):
                stock.publish_exclusive(link, b"new bytes")
            self.assertEqual(path.read_bytes(), b"keep user data")
            new = Path(directory) / "new.zip"
            stock.publish_exclusive(new, b"exact output")
            self.assertEqual(new.read_bytes(), b"exact output")
            self.assertEqual(sorted(p.name for p in Path(directory).iterdir()), ["linked.bin", "new.zip", "user-samples.bin"])

    def test_actual_firmware_and_independent_browser(self):
        engine = (ROOT / "firmware/src/app/engine.c").read_text()
        platform = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
        functions = "\n\n".join(source_function(platform, name) for name in ("plat_slot_zones", "plat_slot"))
        functions += "\n\n" + "\n\n".join(source_function(engine, name) for name in (
            "loop_bars", "load_pack", "builtin_count", "brk_builtin"))
        globals_start = engine.index("#define NLOOPS_MAX")
        globals_end = engine.index("X0X_COLD __attribute__((noinline)) static void scan_loops", globals_start)
        functions += "\n\n" + engine[globals_start:globals_end]
        functions += "\n\n" + "\n\n".join(source_function(engine, name) for name in (
            "scan_loops", "engine_brk_nslots", "engine_brk_slot_names", "engine_brk_loops"))
        with tempfile.TemporaryDirectory(prefix="x0x-stock-test-") as directory:
            work = Path(directory)
            source = PROBE_PRELUDE + functions + PROBE_MAIN
            (work / "probe.c").write_text(source)
            (work / "bank.bin").write_bytes(self.bank)
            for k, image in enumerate(self.images, 1):
                (work / f"slot{k}.fsmp").write_bytes(image)
            archive = stock.stock_archive(self.images, "elite", stock.PROFILE)
            (work / "stock.x0xslots").write_bytes(archive)
            spec = [{"name": part["name"], "n": part["nsamples"], "file": f"{part['name']}.adpcm"} for part in self.parts]
            (work / "spec.json").write_text(json.dumps(spec))
            for part in self.parts:
                (work / f"{part['name']}.adpcm").write_bytes(part["adpcm"])
            cmd = [CC, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                   "-fsanitize=address,undefined,bounds", "-fno-sanitize-recover=all", "-I", str(ROOT),
                   str(work / "probe.c"), "-o", str(work / "probe")]
            built = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            # This managed environment denies LeakSanitizer's /proc task scan.
            # The probe uses static buffers; address/undefined/bounds stay on.
            runtime_env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
            ran = subprocess.run([str(work / "probe"), str(work)], capture_output=True, text=True, env=runtime_env)
            self.assertEqual(ran.returncode, 0, ran.stdout + ran.stderr)
            script = NODE_PROBE.replace("MODULE_URL", json.dumps((ROOT / "web/x0x_breaks.js").as_uri()))
            (work / "probe.mjs").write_text(script)
            js = subprocess.run(["node", str(work / "probe.mjs"), str(work)], capture_output=True, text=True)
            self.assertEqual(js.returncode, 0, js.stdout + js.stderr)
            print(ran.stdout.strip(), flush=True)
            print(js.stdout.strip(), flush=True)
            if EVIDENCE:
                EVIDENCE.mkdir(parents=True, exist_ok=True)
                (EVIDENCE / "firmware-probe.c").write_text(source)
                (EVIDENCE / "firmware-probe.log").write_text(ran.stdout + ran.stderr)
                (EVIDENCE / "browser-probe.log").write_text(js.stdout + js.stderr)
                paths = ["firmware/src/app/engine.c", "firmware/src/app/plat_fm1.c", "firmware/src/app/slots.c",
                         "firmware/src/app/brk_pack.h", "firmware/src/dsp/ima_adpcm.h", "firmware/src/dsp/breaks.h",
                         "firmware/src/seq/pattern.h", "firmware/src/app/plat.h", "web/x0x_breaks.js",
                         "tools/stock_slots.py", "tools/sampleio.py", "tools/x0x_slots.py", "tools/break_pack.py",
                         "tools/fm1pkg_make.py", "tests/stock_slots_test.py"]
                receipt = {"scope": "offline exact stock data and source-path slot admission/selection",
                           "bank_sha256": stock.sha256(self.bank), "bank_bytes": len(self.bank),
                           "private_approved_input": INPUT_BANK is not None,
                           "compiler": subprocess.check_output([CC, "--version"], text=True).splitlines()[0],
                           "compile_command": cmd, "compile_exit": built.returncode, "firmware_probe_exit": ran.returncode,
                           "sanitizers": "address,undefined,bounds; leak scan disabled because /proc/task is unavailable",
                           "browser_probe_exit": js.returncode, "decoded_samples_compared": sum(p["nsamples"] for p in self.parts),
                           "projection_sha256": stock.sha256(source.encode()),
                           "source_sha256": {path: stock.sha256((ROOT / path).read_bytes()) for path in paths},
                           "limits": ["simulated flash; no target execution or device access", "selection calls capture loops at breaks_set_loop; no DSP render/timing claim"]}
                (EVIDENCE / "source-path-receipt.json").write_bytes(stock.json_bytes(receipt))


PROBE_PRELUDE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BOARD_SMK37 1
#define X0X_COLD
#define X0X_NBREAKS 0
#define X0X_IMA_TABLES_DEFINE
#include "firmware/src/app/plat.h"
#include "firmware/src/dsp/breaks.h"
#include "firmware/src/seq/sequencer.h"
#include "firmware/src/dsp/ima_adpcm.h"
#include "firmware/src/app/brk_pack.h"
static uint32_t flash_words[2][0x1E000 / 4];
#define SMP_USER_XIP(k) ((const uint8_t *)flash_words[k])
#include "firmware/src/app/slots.c"
_Static_assert(sizeof(smp_zone_t) == 28, "zone ABI");
_Static_assert(sizeof(smp_user_hdr_t) == 480, "header ABI");
_Static_assert(SMP_USER_DATA == 512 && SMP_USER_SLOTS == PLAT_NSLOTS && PLAT_NSLOTS == 2, "slot ABI");
_Static_assert(SMP_USER_OFF(0) == 0xC2000 && SMP_USER_SIZE(0) == 0x14000, "slot 1 map");
_Static_assert(SMP_USER_OFF(1) == 0xD6000 && SMP_USER_SIZE(1) == 0x1E000, "slot 2 map");
static uint8_t test_app[0x86EE0];
#define _start test_app
static brk_pack_loop_t pack_loops[BRK_PACK_MAX];
static int pack_n = -1;
static breaks_t brk;
static brk_loop_t selected[2];
static struct { pattern_t pat[1]; uint8_t ppat[NTRACKS]; } seq;
void breaks_flush(breaks_t *b) { (void)b; }
void breaks_set_loop(breaks_t *b, int w, const brk_loop_t *l) { (void)b; selected[w] = *l; }
'''

PROBE_MAIN = r'''
static size_t load(const char *dir, const char *name, uint8_t *dst, size_t cap) {
    char path[2048]; assert(snprintf(path, sizeof path, "%s/%s", dir, name) > 0);
    FILE *f = fopen(path, "rb"); assert(f); size_t n = fread(dst, 1, cap, f);
    assert(n && fgetc(f) == EOF && !ferror(f)); assert(fclose(f) == 0); return n;
}
int main(int argc, char **argv) {
    assert(argc == 2); const char *dir = argv[1];
    uint8_t bank[150000]; size_t bank_size = load(dir, "bank.bin", bank, sizeof bank);
    brk_pack_loop_t original[BRK_PACK_MAX];
    assert(brk_pack_parse(bank, (uint32_t)bank_size, original, BRK_PACK_MAX) == 3);
    memset(test_app, 0xff, sizeof test_app); memset(test_app, 0xa5, 512);
    assert(load_pack() == 0 && builtin_count() == 0);
    memcpy(test_app + 512, "X0XBRKPK", 8); assert(load_pack() == 0);
    memcpy(test_app + 1024, bank, bank_size); assert(load_pack() == 3);
    test_app[1024 + bank_size] = 0; assert(load_pack() == 0); /* valid interior pack isn't a tail pack */
    test_app[1024 + bank_size] = 0xff; test_app[1024 + 16] ^= 1; assert(load_pack() == 0);
    memset(test_app, 0xff, sizeof test_app); memset(test_app, 0xa5, 512); assert(load_pack() == 0);

    memset(flash_words, 0xff, sizeof flash_words); smp_user_scan(0); smp_user_scan(1);
    scan_loops(); assert(engine_brk_nslots() == 0); engine_brk_loops();
    assert(!selected[0].adpcm && !selected[1].adpcm); /* no-stock boot selects empty loops */
    size_t sizes[2];
    sizes[0] = load(dir, "slot1.fsmp", (uint8_t *)flash_words[0], SMP_USER_SIZE(0));
    sizes[1] = load(dir, "slot2.fsmp", (uint8_t *)flash_words[1], SMP_USER_SIZE(1));
    for (int k = 0; k < 2; k++) { smp_user_scan(k); assert(usr_nz[k] == k + 1); }
    scan_loops(); assert(engine_brk_nslots() == 3);
    const char *const *names = engine_brk_slot_names();
    assert(!strcmp(names[0], "BR1") && !strcmp(names[1], "BR2") && !strcmp(names[2], "BR3"));
    assert(loop_slot[0] == 0 && loop_zone[0] == 0);
    assert(loop_slot[1] == 1 && loop_zone[1] == 0);
    assert(loop_slot[2] == 1 && loop_zone[2] == 1);
    uint32_t decoded = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t n = 0, rate = 0; char name[9];
        const uint8_t *raw = plat_slot(loop_slot[i], loop_zone[i], &n, &rate, name);
        assert(raw && n == original[i].n && rate == 44100 && loop_bars(n, rate) == original[i].bars);
        assert(!memcmp(raw, original[i].adpcm, original[i].nbytes));
        char path[2048]; assert(snprintf(path, sizeof path, "%s/%s.pcm", dir, original[i].name) > 0);
        FILE *f = fopen(path, "wb"); assert(f);
        int32_t p = 0, index = 0, reference_p = 0, reference_index = 0;
        for (uint32_t s = 0; s < n; s++) {
            int32_t v = ima_decode(raw, s, &p, &index);
            assert(v == ima_decode(original[i].adpcm, s, &reference_p, &reference_index));
            uint16_t u = (uint16_t)(int16_t)v;
            assert(fputc(u & 255, f) != EOF && fputc(u >> 8, f) != EOF); decoded++;
        }
        assert(fclose(f) == 0);
        for (int w = 0; w < 2; w++) {
            seq.pat[0].brk.slot_a = seq.pat[0].brk.slot_b = (uint8_t)i;
            engine_brk_loops();
            assert(selected[w].adpcm == raw && selected[w].nsamples == n && selected[w].rate == rate);
            assert(selected[w].bars == 1 && selected[w].slices == 8);
        }
    }
    seq.pat[0].brk.slot_a = 255; seq.pat[0].brk.slot_b = 255;
    engine_brk_loops(); assert(!selected[0].adpcm && !selected[1].adpcm);
    assert(!plat_slot(-1, 0, &(uint32_t){0}, &(uint32_t){0}, (char[9]){0}));
    assert(!plat_slot(0, 1, &(uint32_t){0}, &(uint32_t){0}, (char[9]){0}));
    smp_user_hdr_t *h = (smp_user_hdr_t *)flash_words[1];
    uint32_t saved = h->data_len; h->data_len = SMP_USER_SIZE(1); smp_user_scan(1); assert(!usr_nz[1]); h->data_len = saved;
    saved = h->zone[1].off; h->zone[1].off = h->data_len; smp_user_scan(1); assert(!usr_nz[1]); h->zone[1].off = saved;
    saved = h->zone[1].n; h->zone[1].n = UINT32_MAX; smp_user_scan(1); assert(!usr_nz[1]); h->zone[1].n = saved;
    saved = h->zone[1].rate; h->zone[1].rate = 0; smp_user_scan(1); assert(!usr_nz[1]); h->zone[1].rate = saved;
    h->zone[1].idx = 89; smp_user_scan(1); assert(!usr_nz[1]); h->zone[1].idx = 0;
    h->nz = 17; smp_user_scan(1); assert(!usr_nz[1]); h->nz = 2;
    ((uint8_t *)flash_words[1])[sizes[1] - 1] ^= 1; smp_user_scan(1); assert(!usr_nz[1]);
    scan_loops(); assert(engine_brk_nslots() == 1); seq.pat[0].brk.slot_a = 2; engine_brk_loops(); assert(!selected[0].adpcm);
    assert(decoded == 262141);
    printf("PASS firmware source: 2 slots, 3 zones/BR names, all A/B choices, %u exact decoded samples, empty/invalid fallback and corrupt slot/pack refusal; ASan/UBSan\n", decoded);
    return 0;
}
'''

NODE_PROBE = r'''
import fs from "node:fs";
import assert from "node:assert/strict";
import crypto from "node:crypto";
import {userSlotRaw, readArchive, imaDecode} from MODULE_URL;
const dir = process.argv[2], read = n => new Uint8Array(fs.readFileSync(`${dir}/${n}`));
const spec = JSON.parse(fs.readFileSync(`${dir}/spec.json`));
const parts = spec.map(p => ({adpcm: read(p.file), n: p.n, rate:44100}));
const groups = [[parts[0]], [parts[1], parts[2]]];
const actual = [read("slot1.fsmp"), read("slot2.fsmp")];
groups.forEach((group, i) => {
  const result = userSlotRaw(`STOCK${i+1}`, group, i ? 0x1E000 : 0x14000);
  const packed = new Uint8Array(512 + result.data.length); packed.fill(255);
  packed.set(result.hdr); packed.set(result.data, 512);
  assert.deepEqual(packed, actual[i]);
});
const archive = readArchive(read("stock.x0xslots"));
assert.deepEqual(archive.images, actual);
let samples = 0;
for (let i=0; i<parts.length; i++) {
  const decoded = imaDecode(parts[i].adpcm, parts[i].n), pcm = read(`${spec[i].name}.pcm`);
  const view = new DataView(pcm.buffer, pcm.byteOffset, pcm.byteLength);
  assert.equal(pcm.length, 2*parts[i].n);
  for(let j=0; j<decoded.length; j++) assert.equal(decoded[j], view.getInt16(2*j, true));
  samples += decoded.length;
  console.log(`${spec[i].name}: samples=${decoded.length} PCM_SHA256=${crypto.createHash("sha256").update(pcm).digest("hex")}`);
}
console.log(`PASS independent browser: both raw FSMP serializers match, native archive parses, ${samples} decoded samples match production C`);
'''


def main():
    global INPUT_BANK, INPUT_RECEIPT, EVIDENCE, CC
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bank", type=Path)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--evidence-dir", type=Path)
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    if bool(args.bank) != bool(args.receipt):
        parser.error("--bank and --receipt must be supplied together")
    if args.bank:
        INPUT_BANK, INPUT_RECEIPT = args.bank.read_bytes(), args.receipt.read_bytes()
    EVIDENCE, CC = args.evidence_dir, args.cc
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(StockTests))
    if EVIDENCE:
        EVIDENCE.mkdir(parents=True, exist_ok=True)
        (EVIDENCE / "test-result.json").write_bytes(stock.json_bytes({"tests": result.testsRun,
            "failures": len(result.failures), "errors": len(result.errors), "skipped": len(result.skipped),
            "result": "PASS" if result.wasSuccessful() and not result.skipped else "FAIL_OR_INCOMPLETE",
            "approved_stock_input": INPUT_BANK is not None}))
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
