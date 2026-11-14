"""Bounded exhaustive scheduler independent of model implementation.

Enumerates all order-preserving interleavings of two 4-event client scripts
and one 2-event environment script. It checks accepted music against a
separate effect ledger after EVERY event, including hidden-write attacks.
"""
from __future__ import annotations

import copy
import hashlib
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from model import Server


def sha(data):
    return hashlib.sha256(data).hexdigest()


def schedules(left=(4, 4, 2), prefix=()):
    if not any(left):
        yield prefix
    else:
        for actor in range(3):
            if left[actor]:
                rest = list(left)
                rest[actor] -= 1
                yield from schedules(tuple(rest), prefix + (actor,))


def run(schedule):
    s = Server(b"BASE")
    clients = [dict(owner="A-secret", request="A-request", data=b"AAAA", ticket=None),
               dict(owner="B-secret", request="B-request", data=b"BBBB", ticket=None)]
    pos = [0, 0, 0]
    expected_data, expected_rev = b"BASE", 0
    accepted = set()
    assertions = 0
    for actor in schedule:
        event = pos[actor]
        pos[actor] += 1
        if actor == 2:
            if event == 0:
                out = s.local_edit(b"LOCL")
                assert out["code"] == "OK", (schedule, pos, out)
                expected_data, expected_rev = b"LOCL", expected_rev + 1
            else:
                s.advance(15)
        else:
            c = clients[actor]
            owner, request, data = c["owner"], c["request"], c["data"]
            if event == 0:
                out = s.begin(owner, request, s.epoch)
                if out["code"] == "OK":
                    c["ticket"] = out
            elif event == 1:
                if c["ticket"] is None:
                    out = s.begin(owner, request, s.epoch)
                    if out["code"] == "OK":
                        c["ticket"] = out
                r = c["ticket"]
                if r:
                    t, g, rev = r["token"], r["generation"], r["revision"]
                    out = s.stage(owner, t, g, rev, len(data), sha(data))
                    if out["code"] == "OK":
                        assert s.write(owner, t, g, 0, data)["code"] == "OK"
                        assert s.validate(owner, t, g)["code"] == "OK"
            elif event == 2:
                r = c["ticket"]
                if r:
                    out = s.commit(owner, r["token"], r["generation"], r["revision"], sha(data))
                    if out["code"] == "COMMITTED" and actor not in accepted:
                        accepted.add(actor)
                        expected_data, expected_rev = data, expected_rev + 1
            else:
                s.cancel(owner, request, s.epoch)
        assert (s.live, s.revision) == (expected_data, expected_rev), (schedule, pos, s.live, s.revision, expected_data, expected_rev)
        assertions += 1
        if s.active is not None:
            t = s.active
            before = copy.deepcopy(s.transactions[t])
            for attack in [lambda: s.abort("FOREIGN-secret", t, 0),
                           lambda: s.write("FOREIGN-secret", t, 0, 0, b"LOSS"),
                           lambda: s.commit("FOREIGN-secret", t, 0, 0, sha(b"LOSS"))]:
                out = attack()
                assert out["code"] == "FOREIGN", (schedule, pos, out)
                assert s.transactions[t] == before, (schedule, pos, "foreign stage/lease mutation")
                assert (s.live, s.revision) == (expected_data, expected_rev)
                assertions += 3
    for actor, c in enumerate(clients):
        r = c["ticket"]
        if r:
            # Delayed duplicate commit and abort can report a previous COMMIT,
            # but may never apply a second mutation or another owner's intent.
            s.commit(c["owner"], r["token"], r["generation"], r["revision"], sha(c["data"]))
            s.abort(c["owner"], r["token"], r["generation"])
            assert (s.live, s.revision) == (expected_data, expected_rev)
            assertions += 1
    return assertions


if __name__ == "__main__":
    traces, assertions = 0, 0
    for schedule in schedules():
        assertions += run(schedule)
        traces += 1
    assert traces == 3150
    print(f"PASS: {traces} complete interleavings; {assertions} ledger/authorization assertions; no hidden writes")
