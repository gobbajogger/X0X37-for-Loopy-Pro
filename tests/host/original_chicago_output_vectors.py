#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent unbounded-integer/rational binary32 output/wrapper oracle.
No original plugin execution, host libm or production C calls.
"""
import hashlib
import importlib.util
import json
import random
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("rational", ROOT / "miami_fx_conversion_vectors.py")
o = importlib.util.module_from_spec(spec)
spec.loader.exec_module(o)
manifest = Path(__import__("sys").argv[1])
raw = manifest.read_bytes()
assert hashlib.sha256(raw).hexdigest() == "bb0fcb9314a99135ea0c662af5dedd476021e9747a2489d8ce821b407393e9f6"
m = json.loads(raw)
D = m["functional_tables"]["Drive"]["values_int16"]
G = m["functional_tables"]["Gain"]["values_int16"]
I = lambda v: (v + 2**31) % 2**32 - 2**31
low = lambda v: (v + 32768) % 65536 - 32768


def mul(a, b):
    result = o.rounded(o.value(a) * o.value(b))
    return ((a ^ b) & 0x80000000) if not result & 0x7fffffff else result


def add(a, b):
    result = o.rounded(o.value(a) + o.value(b))
    return 0x80000000 if not result and a == b == 0x80000000 else result


def sub(a, b):
    return add(a, b ^ 0x80000000)


def div(a, b):
    result = o.rounded(o.value(a) / o.value(b))
    return ((a ^ b) & 0x80000000) if not result & 0x7fffffff else result


def mix(old, newer, weight):
    return max(-32767, min(32767, (I(newer * weight) >> 15) + (I(old * (32767-weight)) >> 15)))


class Drive:
    def __init__(self):
        self.td, self.tg = 16254, 20801
        self.sd = self.sg = self.st = 0
        self.pre = [0, 0]
        self.y = [0, 0]
        self.active, self.ed, self.eg = 0, 63, 100

    def reset(self):
        self.sd, self.sg, self.st = I(self.td << 15), I(self.tg << 15), 0
        self.pre, self.y = [0, 0], [0, 0]

    def edit(self, p, v):
        if p == 2:
            self.ed, self.td = v, D[v]
        else:
            self.eg, self.tg = v, 1279 + (I(G[v] * 31489) >> 15)

    def frame(self, left, right):
        k = 32767 if self.active else 102
        smooth = lambda a, b: I(I(a - I((a >> 15) * k)) + I(k * b))
        self.sd, self.sg, self.st = smooth(self.sd, self.td), smooth(self.sg, self.tg), smooth(self.st, max(19660-self.td, 0))
        d, g, threshold = self.sd >> 15, self.sg >> 15, self.st >> 15
        feedback_gain = I(d * 23919) >> 15
        out = []
        for c, x in enumerate([left, right]):
            a = I(I(self.y[c] << 1) - I(self.y[c] << 15))
            feedback = I(I((a >> 15) << 1) - I(a & 0xffff8000)) >> 15
            v = max(-32767, min(32767, I(I(self.pre[c] + x) + (I(feedback * feedback_gain) >> 15))))
            z = max(abs(v)-threshold, 0)
            sign = 26213 if v > 0 else -26213 if v else 0
            y = I(v - (I(((I(z*z) & 0xffffffff) >> 15) * sign) >> 15))
            self.pre[c], self.y[c] = I(d*x) >> 13, y
            out.append(low((I(y*g) & 0xffffffff) >> 15))
        return out

    def fields(self):
        return [self.td, self.tg, self.sd, self.sg, self.st, *self.pre, *self.y, self.active, self.ed, self.eg]


class Wrapper:
    def __init__(self, d, g, on):
        self.drive = Drive()
        self.ed, self.eg = d, g
        self.current, self.pending, self.dirty = 1, on, 1
        self.target = self.accumulator = self.progress = 0
        self.activity = -1
        self.initial, self.selected, self.transition = 1, 0, 0
        self.events, self.cursor = [], 0
        self.chunks = []

    def edit(self, p, v):
        if p == 1:
            self.pending, self.dirty = v, 1
        elif p in [2, 3]:
            if p == 2:
                self.ed = v
            else:
                self.eg = v
            if self.selected:
                self.drive.edit(p, v)

    def reset(self):
        self.transition, self.current = 0, self.pending
        self.target = 32767 if self.current else 0
        self.accumulator = self.target << 15
        self.drive.reset()

    def invoke(self, samples):
        self.chunks.append(len(samples))
        out = []
        for i, sample in enumerate(samples):
            while self.cursor < len(self.events) and self.events[self.cursor][0] <= i:
                if self.events[self.cursor][1] == 4:
                    self.drive.reset()
                self.cursor += 1
            out.append(self.drive.frame(*sample))
        return out

    def block(self, dry):
        if self.dirty:
            self.current, self.dirty = self.pending, 0
            self.target = 32767 if self.current else 0
        if self.initial:
            self.progress, self.transition = 32767, 1
            self.selected, self.initial, self.activity = 1, 0, 0
            self.drive.reset()
            self.drive.edit(2, self.ed)
            self.drive.edit(3, self.eg)
            self.drive.active = 1
        wet, position = [], 0
        while position < len(dry):
            if self.transition:
                chunk = min(len(dry)-position, 743*(max(self.progress // 743, 1)+1))
                processed = self.invoke(dry[position:position+chunk])
                for i in range(chunk):
                    old_weight = ((self.progress*self.progress) & 0xffffffff) >> 15
                    wet.append([mix(dry[position+i][c], processed[i][c], 32767-old_weight) for c in range(2)])
                    self.progress = max(self.progress-743, 0)
                if not self.progress:
                    self.transition = 0
            else:
                chunk = len(dry)-position
                wet.extend(self.invoke(dry[position:]) if self.selected else dry[position:])
            position += chunk
        out = []
        for old, new in zip(dry, wet):
            g = self.accumulator >> 15
            if g != self.target:
                self.accumulator = I(self.accumulator + (self.target-g)*102)
                g = self.accumulator >> 15
            out.append([mix(old[c], new[c], g) for c in range(2)])
        if self.activity >= 0:
            self.activity += len(dry)
            if self.activity >= 44:
                self.activity, self.drive.active = -1, 0
        return out

    def fields(self):
        return self.drive.fields() + [self.target, self.accumulator, self.progress, self.activity, self.ed, self.eg, self.current, self.pending, self.dirty, self.initial, self.selected, self.transition, len(self.events), self.cursor]


class Output:
    def __init__(self, d, g, on):
        self.wrapper = Wrapper(d, g, on)
        self.current = self.target = self.step = 0
        self.committed, self.pending = 1, 0
        self.countdown, self.process, self.enable = [-1, -1], [1, 1], [1, 1]
        self.fx2calls = self.fx2frames = self.fx2resets = 0
        self.fx2history = [3, -5]

    def fx2reset(self):
        self.fx2resets += 1
        self.fx2history = [3, -5]

    def fx2block(self, samples):
        self.fx2calls += 1
        self.fx2frames += len(samples)
        out = []
        for l, r in samples:
            out.append([low((l*3 >> 2) + self.fx2history[0]), low((r*5 >> 2) + self.fx2history[1])])
            self.fx2history = [l, r]
        return out

    def block(self, inputs, volume, fz):
        if not inputs:
            return []
        samples = [[o.chain(x, fz)[3] for x in pair] for pair in inputs]
        for slot in range(2):
            if not self.enable[slot]:
                continue
            if self.countdown[slot] > 0:
                self.countdown[slot] -= len(samples)
                if self.countdown[slot] <= 0:
                    self.countdown[slot] = -1
                    if slot:
                        self.fx2reset()
                    else:
                        self.wrapper.reset()
            if self.process[slot]:
                samples = self.fx2block(samples) if slot else self.wrapper.block(samples)
        target = 0 if self.pending else mul(volume, 0x40800000)
        count_bits = o.rounded(F(len(samples)))
        if self.committed == self.pending:
            step = div(sub(target, self.current), count_bits)
        elif self.pending:
            step = div(self.current ^ 0x80000000, count_bits)
        else:
            self.current, step = target, 0
        self.committed, self.target, self.step = self.pending, target, step
        out = []
        for sample in samples:
            self.current = add(self.current, step)
            out.append([o.cleanup(mul(o.back(n), self.current)) for n in sample])
        self.current = target
        return out

    def fields(self):
        return self.wrapper.fields() + [self.current, self.target, self.step, self.committed, self.pending, *self.countdown, *self.process, *self.enable, self.fx2calls, self.fx2frames, self.fx2resets, *self.fx2history]


def main():
    rng = random.Random(33435)
    count = 0
    state_rows = 0
    path = Path(__import__("sys").argv[2])
    with path.open("w") as f:
        s = None

        def state():
            nonlocal state_rows
            fields = s.fields()
            f.write("W " + str(len(fields)) + " " + " ".join(map(str, fields)) + "\n")
            state_rows += 1

        def init(d=100, g=110, on=1):
            nonlocal s
            f.write(f"I {d} {g} {on}\n")
            s = Output(d, g, on)
            state()

        def edit(p, v):
            f.write(f"P {p} {v}\n")
            s.wrapper.edit(p, v)
            state()

        def events(values):
            f.write("E " + str(len(values)) + " " + " ".join(str(x) for pair in values for x in pair) + "\n")
            s.wrapper.events, s.wrapper.cursor = values, 0
            state()

        def block(n, volume=0x3e4e7073, fz=0, zero=False):
            nonlocal count
            if zero:
                inputs = [[0, 0] for _ in range(n)]
            else:
                choices = [0, 0x80000000, 1, 0x80000001, 0x3f000000, 0xbf000001, 0x3f7fffff, 0xbf800000, 0x7f800000, 0xff800000, 0x7fa01234, 0xffc01234, 0x3eaaaaab]
                inputs = [[rng.choice(choices) if rng.randrange(3) else rng.getrandbits(32) for _ in range(2)] for _ in range(n)]
            result = s.block(inputs, volume, fz)
            f.write(f"B {n} {volume:08x} {fz}\n")
            for pair, expected in zip(inputs, result):
                f.write("S " + " ".join(f"{x:08x}" for x in pair+expected) + "\n")
            count += n
            state()

        def direct(typ):
            f.write(f"D {typ}\n")
            if typ == 5:
                s.wrapper.drive.reset()
            state()

        def gate(slot, enabled):
            f.write(f"G {slot} {enabled}\n")
            s.enable[slot] = enabled
            state()

        def suspend(slot, flag, fs=0x472c4400):
            f.write(f"H {slot} {flag} {fs:08x}\n")
            raw = div(mul(fs, 0x41a00000), 0x40a00000)
            s.countdown[slot] = I(min(0xffffffff, max(0, int(o.value(raw)))) & 0xfffffff0) if flag else -1
            s.process[slot] = int(not flag)
            state()

        init()
        block(0, 0x7fa12345, 9)  # Entire outer no-op, even cold state.
        events([(0, 5), (0, 4), (11, 4), (4000, 4)])
        block(20)
        edit(2, 29)  # Ordinary edits do not restart fast timer.
        block(31)
        assert s.wrapper.activity == -1 and s.wrapper.drive.active == 0
        direct(4)
        direct(5)
        block(1)
        edit(1, 0)
        block(64)
        f.write("R\n");s.wrapper.reset();state()
        edit(1, 1)
        block(44)

        init()
        block(40)
        assert s.wrapper.progress == 3047
        events([(200, 7), (3800, 4)])
        block(4096)
        assert s.wrapper.chunks[-2:] == [3715, 381] and s.wrapper.cursor == 1
        block(4096)
        assert s.wrapper.cursor == 2

        init()
        events([(0, 4)])
        f.write("Z\n");s.wrapper.block([]);state()
        assert s.wrapper.cursor == 0 and s.wrapper.drive.active == 1
        block(1)
        f.write("R\n");s.wrapper.reset();state()
        assert s.wrapper.activity == 1 and s.wrapper.drive.active == 1
        block(43)
        assert s.wrapper.activity == -1

        # Reach the real44100 countdown crossing; disable FX2 so this tests
        # paused Drive history/reset ownership without substituting an FX body.
        gate(1, 0)
        suspend(0, 1)
        assert s.countdown[0] == 176400
        for _ in range(43):
            block(4096, zero=True)
        block(271, zero=True)
        assert s.countdown[0] == 1
        block(1, zero=True)
        assert s.countdown[0] == -1 and not s.process[0]
        suspend(0, 0)
        gate(1, 1)
        block(64)
        # Disabled outer slot freezes both processing and countdown.
        suspend(1, 1, 0x45fa0000)  # Fs8000, independently selected metadata.
        gate(1, 0)
        block(4096)
        assert s.countdown[1] == 32000
        gate(1, 1)
        for _ in range(7):
            block(4096)
        block(3328)
        assert s.countdown[1] == -1 and s.fx2resets == 1
        suspend(1, 0)
        for byte in [1, 127, 0, 0, 1, 0]:
            f.write(f"M {byte}\n");s.pending = byte;state()
            block(17, 0x3f800000)

        # All integer Drive controls and both switches, changing volume per
        # block. These fixtures exercise routing, not original FX2 sound.
        for i in range(128):
            edit(2, i);edit(3, 127-i)
            if i % 11 == 0:
                edit(1, int(i % 22 == 0))
            if i % 13 == 0:
                events([(0, 4), (0, 5), (3, 7), (7, 4)])
            block([1, 17, 43, 44, 45, 64, 128, 512][i % 8], o.rounded(F(i, 127)), i & 1)
    result = {"scope": "Independent rational binary32 and unbounded-integer oracle. Synthetic FX2 callback proves routing only; no missing original FX2 body is claimed.", "frames": count, "state_rows": state_rows, "fixture_bytes": path.stat().st_size, "fixture_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "special_transition_chunks": [3715, 381], "state_field_count": len(s.fields())}
    path.with_suffix(".json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
