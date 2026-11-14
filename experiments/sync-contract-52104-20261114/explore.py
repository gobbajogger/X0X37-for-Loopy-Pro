"""Enumerate all 12,870 interleavings of two eight-event client programs.

This is bounded exploration, not an unbounded formal verification claim. A
separate local-writer event is inserted at each of three selected schedule cuts.
"""
from itertools import combinations
import json
from model import Server, digest


def invariant(s):
    assert s.revision == len(s.journal)
    previous = b"old!"
    for i, event in enumerate(s.journal):
        assert event["before"] == previous
        assert event["base_revision"] == i
        assert event["revision"] == i + 1
        assert event["validated"] is True
        if event["actor"] not in ("LOCAL", "LEGACY"):
            tx = s.transactions[event["token"]]
            assert event["actor"] == tx.owner
            assert tx.base_revision == event["base_revision"]
            assert tx.outcome == "COMMITTED"
            assert digest(event["after"]) == tx.result_digest
        previous = event["after"]
    assert s.live == previous
    active = [t.token for t in s.transactions.values() if not t.outcome]
    assert active == ([] if s.active is None else [s.active])
    assert not (s.active and s.legacy_active)


def progress(s, client):
    owner, request, payload = client["owner"], client["request"], client["payload"]
    if client["token"] is None:
        reply = s.begin(owner, request, s.epoch)
        if reply["code"] == "OK":
            client["token"], client["base"] = reply["token"], reply["revision"]
            client["step"] = 1
        return
    token, base, step = client["token"], client["base"], client["step"]
    if step == 1:
        result = s.read(owner, token, 0, 0, 4)
        if result["code"] == "OK":
            client["snapshot"] = result["data"]
    elif step == 2:
        s.stage(owner, token, 0, base, 4, digest(payload))
    elif step == 3:
        s.write(owner, token, 0, 0, payload)
    elif step == 4:
        s.validate(owner, token, 0)
    elif step == 5:
        result = s.commit(owner, token, 0, base, digest(payload))
        if result["code"] == "COMMITTED":
            client["committed"] = True
    elif step == 6:
        result = s.abort(owner, token, 0)
        if client.get("committed"):
            assert result["code"] == "COMMITTED"  # Cancellation cannot undo.
    else:
        s.status(owner, token)
    client["step"] += 1


def explore():
    schedules = events = committed = conflicts = 0
    for a_positions in combinations(range(16), 8):
        a_positions = set(a_positions)
        for local_cut in (0, 8, 16):
            s = Server(b"old!")
            clients = [dict(owner="A", request="req-A", payload=b"A123", token=None),
                       dict(owner="B", request="req-B", payload=b"B234", token=None)]
            for index in range(17):
                if index == local_cut:
                    s.local_edit(b"LOC!")
                    invariant(s)
                    events += 1
                if index == 16:
                    break
                before, rev = s.live, s.revision
                actor = 0 if index in a_positions else 1
                progress(s, clients[actor])
                invariant(s)
                # Delayed/foreign cancellation and legacy ABORT cannot disturb
                # any currently active transaction owned by the other client.
                if s.active:
                    tx = s.transactions[s.active]
                    foreign = "B" if tx.owner == "A" else "A"
                    held = s.active
                    live = s.live
                    deadline = tx.idle_deadline
                    assert s.abort(foreign, held, 0)["code"] == "FOREIGN"
                    assert s.commit(foreign, held, 0, tx.base_revision, digest(b"evil"))["code"] == "FOREIGN"
                    assert s.legacy("ABORT")["code"] == "BUSY"
                    assert (s.active, s.live, tx.idle_deadline) == (held, live, deadline)
                    invariant(s)
                    events += 3
                if s.live != before:
                    assert s.revision == rev + 1
                    assert s.journal[-1]["actor"] == clients[actor]["owner"]
                events += 1
            schedules += 1
            committed += sum(c.get("committed", False) for c in clients)
            conflicts += sum(t.manifest is not None and t.base_revision != s.revision
                             and t.outcome != "COMMITTED" for t in s.transactions.values())
    result = dict(schedules=schedules, checked_events=events, observed_commits=committed,
                  final_uncommitted_stale_transactions=conflicts,
                  invariant_failures=0, clients=2, progress_events_per_client=8,
                  local_write_cuts=[0, 8, 16], evidence="synthetic semantic model only")
    print(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    explore()
