"""Independent hostile contract tests; standard library only, no device I/O."""
from __future__ import annotations

import copy
import hashlib
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from model import Server, Token


def digest(data):
    return hashlib.sha256(data).hexdigest()


class AdversarialTests(unittest.TestCase):
    def setUp(self):
        self.s = Server(b"BASE")

    def begin(self, owner="A-secret", request="A-request", s=None):
        s = s or self.s
        result = s.begin(owner, request, s.epoch)
        self.assertEqual(result["code"], "OK", result)
        return result

    def prepared(self, owner="A-secret", request="A-request", data=b"EDIT", s=None):
        s = s or self.s
        r = self.begin(owner, request, s)
        t, g, rev = r["token"], r["generation"], r["revision"]
        for out in [s.stage(owner, t, g, rev, len(data), digest(data)),
                    s.write(owner, t, g, 0, data),
                    s.validate(owner, t, g)]:
            self.assertEqual(out["code"], "OK", out)
        return t, g, rev

    def assert_live(self, data, rev, s=None):
        s = s or self.s
        self.assertEqual((s.live, s.revision), (data, rev))

    def test_two_clients_and_stale_cached_revision(self):
        a = self.begin()
        self.assertEqual(self.s.begin("B-secret", "B-request", self.s.epoch)["code"], "BUSY")
        self.assertEqual(self.s.abort("A-secret", a["token"], 0)["code"], "ABORTED")
        t, g, rev = self.prepared("B-secret", "B-commit", b"BBBB")
        self.assertEqual(self.s.commit("B-secret", t, g, rev, digest(b"BBBB"))["code"], "COMMITTED")
        a2 = self.begin(request="A-new")
        self.assertEqual(self.s.stage("A-secret", a2["token"], 0, a["revision"], 4, digest(b"AAAA"))["code"], "CONFLICT")
        self.assert_live(b"BBBB", 1)

    def test_foreign_attempts_do_not_touch_owner_or_lease(self):
        a = self.begin()
        t = a["token"]
        baseline = copy.deepcopy(self.s.transactions[t])
        calls = [lambda: self.s.read("B-secret", t, 0, 0, 4),
                 lambda: self.s.stage("B-secret", t, 0, 0, 4, digest(b"BBBB")),
                 lambda: self.s.write("B-secret", t, 0, 0, b"BBBB"),
                 lambda: self.s.validate("B-secret", t, 0),
                 lambda: self.s.commit("B-secret", t, 0, 0, digest(b"BBBB")),
                 lambda: self.s.abort("B-secret", t, 0),
                 lambda: self.s.status("B-secret", t),
                 lambda: self.s.resume("B-secret", t, 0, "steal")]
        for call in calls:
            self.assertEqual(call()["code"], "FOREIGN")
            self.assertEqual(self.s.transactions[t], baseline)
            self.assert_live(b"BASE", 0)

    def test_foreign_cancel_cannot_cancel_owner(self):
        a = self.begin()
        self.s.cancel("B-secret", "A-request", self.s.epoch)
        self.assertEqual(self.s.active, a["token"])
        self.assertEqual(self.s.status("A-secret", a["token"])["code"], "ACTIVE")
        self.assert_live(b"BASE", 0)

    def test_legacy_ops_do_not_change_or_renew_v2(self):
        a = self.begin()
        self.s.advance(14)
        baseline = copy.deepcopy(self.s.transactions[a["token"]])
        for op in ["QUERY", "BEGIN", "WRITE", "COMMIT", "ABORT"]:
            self.assertEqual(self.s.legacy(op, b"EVIL")["code"], "BUSY")
            self.assertEqual(self.s.transactions[a["token"]], baseline)
        self.s.advance(1)
        self.assertEqual(self.s.status("A-secret", a["token"])["code"], "EXPIRED")
        self.assert_live(b"BASE", 0)

    def test_v2_does_not_release_legacy_hold(self):
        self.assertEqual(self.s.legacy("BEGIN")["code"], "OK")
        self.assertEqual(self.s.begin("A-secret", "A", self.s.epoch)["code"], "BUSY")
        self.s.cancel("A-secret", "A", self.s.epoch)
        self.assertEqual(self.s.begin("B-secret", "B", self.s.epoch)["code"], "BUSY")
        self.assert_live(b"BASE", 0)

    def test_expire_then_foreign_owner_then_late_old_frames(self):
        a = self.begin()
        self.s.advance(15)
        b = self.begin("B-secret", "B-request")
        baseline = copy.deepcopy(self.s.transactions[b["token"]])
        late = [self.s.abort("A-secret", a["token"], 0),
                self.s.write("A-secret", a["token"], 0, 0, b"AAAA"),
                self.s.commit("A-secret", a["token"], 0, 0, digest(b"AAAA")),
                self.s.resume("A-secret", a["token"], 0, "late")]
        self.assertTrue(all(x["code"] == "EXPIRED" for x in late), late)
        self.assertEqual(self.s.active, b["token"])
        self.assertEqual(self.s.transactions[b["token"]], baseline)
        self.assert_live(b"BASE", 0)

    def test_authorized_keepalive_cannot_defeat_hard_expiry(self):
        a = self.begin()
        for _ in range(5):
            self.s.advance(10)
            self.assertEqual(self.s.keepalive("A-secret", a["token"], 0)["code"], "OK")
        self.s.advance(10)
        self.assertEqual(self.s.status("A-secret", a["token"])["code"], "EXPIRED")
        self.assert_live(b"BASE", 0)

    def test_lost_begin_reply_retries_same_transaction(self):
        a = self.begin()
        same = self.begin()
        self.assertEqual(same["token"], a["token"])
        self.assertEqual(len(self.s.transactions), 1)
        self.assertEqual(self.s.abort("A-secret", a["token"], 0)["code"], "ABORTED")
        self.assertEqual(self.s.begin("A-secret", "A-request", self.s.epoch)["code"], "ABORTED")
        self.assertIsNone(self.s.active)

    def test_reconnect_rebind_idempotent_and_old_frames_revoked(self):
        t, g, rev = self.prepared()
        result = self.s.resume("A-secret", t, g, "resume-one")
        self.assertEqual(result["code"], "OK")
        ng = result["generation"]
        self.assertGreater(ng, g)
        retry = self.s.resume("A-secret", t, g, "resume-one")
        self.assertEqual(retry, result)
        for out in [self.s.read("A-secret", t, g, 0, 1),
                    self.s.write("A-secret", t, g, 0, b"EDIT"),
                    self.s.abort("A-secret", t, g),
                    self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))]:
            self.assertEqual(out["code"], "STALE_BINDING", out)
        self.assertEqual(self.s.commit("A-secret", t, ng, rev, digest(b"EDIT"))["code"], "COMMITTED")
        self.assert_live(b"EDIT", 1)

    def test_lost_commit_reply_and_cancel_after_commit(self):
        t, g, rev = self.prepared()
        first = self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))
        self.assertEqual(first["code"], "COMMITTED")
        self.assertEqual(self.s.commit("A-secret", t, g, rev, digest(b"EDIT")), first)
        self.assertEqual(self.s.abort("A-secret", t, g)["code"], "COMMITTED")
        self.assertEqual(self.s.cancel("A-secret", "A-request", self.s.epoch)["code"], "COMMITTED")
        self.assertEqual(self.s.status("A-secret", t)["code"], "COMMITTED")
        self.assert_live(b"EDIT", 1)

    def test_committed_receipt_rejects_different_intent(self):
        t, g, rev = self.prepared()
        self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))
        self.assertEqual(self.s.commit("A-secret", t, g, rev, digest(b"EVIL"))["code"], "INTENT_MISMATCH")
        self.assertEqual(self.s.commit("A-secret", t, g, rev + 1, digest(b"EDIT"))["code"], "INTENT_MISMATCH")
        self.assert_live(b"EDIT", 1)

    def test_cancel_before_delayed_begin_is_tombstone(self):
        self.assertEqual(self.s.cancel("A-secret", "A-request", self.s.epoch)["code"], "ABORTED")
        self.assertEqual(self.s.begin("A-secret", "A-request", self.s.epoch)["code"], "ABORTED")
        self.assertIsNone(self.s.active)
        self.assert_live(b"BASE", 0)

    def test_abort_before_commit_prevents_late_commit(self):
        t, g, rev = self.prepared()
        self.assertEqual(self.s.abort("A-secret", t, g)["code"], "ABORTED")
        self.assertEqual(self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))["code"], "ABORTED")
        self.assert_live(b"BASE", 0)

    def test_local_edit_during_staging_causes_final_conflict(self):
        t, g, rev = self.prepared()
        self.assertEqual(self.s.local_edit(b"LOCL")["code"], "OK")
        self.assertEqual(self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))["code"], "CONFLICT")
        self.assert_live(b"LOCL", 1)

    def test_snapshot_remains_original_despite_local_edit(self):
        a = self.begin()
        self.assertEqual(self.s.local_edit(b"LOCL")["code"], "OK")
        self.assertEqual(self.s.read("A-secret", a["token"], 0, 0, 4)["data"], b"BASE")
        self.assertEqual(a["snapshot_digest"], digest(b"BASE"))
        self.assert_live(b"LOCL", 1)

    def test_local_aba_edit_still_conflicts(self):
        t, g, rev = self.prepared()
        self.s.local_edit(b"TEMP")
        self.s.local_edit(b"BASE")
        self.assertEqual(self.s.commit("A-secret", t, g, rev, digest(b"EDIT"))["code"], "CONFLICT")
        self.assert_live(b"BASE", 2)

    def test_failed_validation_does_not_change_music(self):
        s = Server(b"BASE", validator=lambda data: data != b"EVIL")
        a = self.begin(s=s)
        self.assertEqual(s.stage("A-secret", a["token"], 0, 0, 4, digest(b"EVIL"))["code"], "OK")
        self.assertEqual(s.write("A-secret", a["token"], 0, 0, b"EVIL")["code"], "OK")
        self.assertEqual(s.validate("A-secret", a["token"], 0)["code"], "FORMAT")
        self.assertNotEqual(s.commit("A-secret", a["token"], 0, 0, digest(b"EVIL"))["code"], "COMMITTED")
        self.assert_live(b"BASE", 0, s)

    def test_incomplete_changed_duplicate_and_out_of_order_chunks(self):
        a = self.begin()
        t = a["token"]
        self.assertEqual(self.s.stage("A-secret", t, 0, 0, 4, digest(b"EDIT"))["code"], "OK")
        self.assertNotEqual(self.s.write("A-secret", t, 0, 2, b"IT")["code"], "OK")
        self.assertEqual(self.s.write("A-secret", t, 0, 0, b"ED")["code"], "OK")
        self.assertEqual(self.s.write("A-secret", t, 0, 0, b"ED")["code"], "OK")
        self.assertNotEqual(self.s.write("A-secret", t, 0, 0, b"XX")["code"], "OK")
        self.assertNotEqual(self.s.validate("A-secret", t, 0)["code"], "OK")
        self.assertNotEqual(self.s.commit("A-secret", t, 0, 0, digest(b"EDIT"))["code"], "COMMITTED")
        self.assertNotEqual(self.s.write("A-secret", t, 0, 2, b"ITX")["code"], "OK")
        self.assert_live(b"BASE", 0)
        self.assertEqual(self.s.write("A-secret", t, 0, 2, b"IT")["code"], "OK")
        self.assertEqual(self.s.validate("A-secret", t, 0)["code"], "OK")
        self.assertEqual(self.s.commit("A-secret", t, 0, 0, digest(b"EDIT"))["code"], "COMMITTED")
        self.assert_live(b"EDIT", 1)

    def test_foreign_epoch_token_cannot_alias_new_serial(self):
        a = self.begin()
        fresh = Server(b"BASE", epoch="boot-2")
        b = self.begin("B-secret", "B", fresh)
        baseline = copy.deepcopy(fresh.transactions[b["token"]])
        for out in [fresh.abort("A-secret", a["token"], 0),
                    fresh.status("A-secret", a["token"]),
                    fresh.resume("A-secret", a["token"], 0, "old")]:
            self.assertEqual(out["code"], "STALE_EPOCH", out)
        self.assertEqual(fresh.transactions[b["token"]], baseline)
        self.assert_live(b"BASE", 0, fresh)

    def test_terminal_capacity_fails_closed(self):
        s = Server(b"BASE", max_transactions=2)
        a = self.begin(s=s)
        s.abort("A-secret", a["token"], 0)
        b = self.begin("B-secret", "B", s)
        s.abort("B-secret", b["token"], 0)
        self.assertEqual(s.begin("C-secret", "C", s.epoch)["code"], "EXHAUSTED")
        self.assertEqual(s.begin("A-secret", "A-request", s.epoch)["code"], "ABORTED")
        self.assert_live(b"BASE", 0, s)

    def test_revision_exhaustion_never_wraps(self):
        s = Server(b"BASE", max_revision=1)
        s.local_edit(b"LOCL")
        self.assertEqual(s.local_edit(b"LOSS")["code"], "EXHAUSTED")
        a = s.begin("A-secret", "A", s.epoch)
        if a["code"] == "OK":
            t = a["token"]
            s.stage("A-secret", t, 0, 1, 4, digest(b"EDIT"))
            s.write("A-secret", t, 0, 0, b"EDIT")
            s.validate("A-secret", t, 0)
            self.assertEqual(s.commit("A-secret", t, 0, 1, digest(b"EDIT"))["code"], "EXHAUSTED")
        else:
            self.assertEqual(a["code"], "EXHAUSTED")
        self.assert_live(b"LOCL", 1, s)

    def test_unsupported_never_acquires_or_mutates(self):
        s = Server(b"BASE", supported=False)
        self.assertEqual(s.begin("A-secret", "A", s.epoch)["code"], "UNSUPPORTED")
        self.assertIsNone(s.active)
        self.assert_live(b"BASE", 0, s)

    def test_malformed_revision_cannot_alias_zero(self):
        for bad in [False, True, 0.0, "0", None, -1]:
            with self.subTest(revision=repr(bad)):
                s = Server(b"BASE")
                a = self.begin(s=s)
                self.assertEqual(s.stage("A-secret", a["token"], 0, bad, 4, digest(b"EDIT"))["code"], "FORMAT")
                self.assert_live(b"BASE", 0, s)

    def test_malformed_commit_revision_cannot_publish(self):
        for bad in [False, 0.0, "0", None, -1]:
            with self.subTest(revision=repr(bad)):
                s = Server(b"BASE")
                t, g, _ = self.prepared(s=s)
                self.assertEqual(s.commit("A-secret", t, g, bad, digest(b"EDIT"))["code"], "FORMAT")
                self.assert_live(b"BASE", 0, s)

    def test_noncanonical_token_serials_do_not_alias(self):
        a = self.begin()
        for serial in [True, 1.0, False, 0, -1, None, "1", [], 1 << 56]:
            with self.subTest(serial=repr(serial)):
                before = copy.deepcopy(self.s.transactions[a["token"]])
                bad = Token(self.s.epoch, serial)
                self.assertEqual(self.s.abort("A-secret", bad, 0)["code"], "FORMAT")
                self.assertEqual(self.s.transactions[a["token"]], before)
                self.assert_live(b"BASE", 0)

    def test_resume_capacity_and_stale_retry_fail_closed(self):
        a = self.begin()
        t, g = a["token"], 0
        for i in range(8):
            out = self.s.resume("A-secret", t, g, f"resume-{i}")
            self.assertEqual(out["code"], "OK")
            g = out["generation"]
        before = copy.deepcopy(self.s.transactions[t])
        self.assertEqual(self.s.resume("A-secret", t, g, "ninth")["code"], "EXHAUSTED")
        self.assertEqual(self.s.transactions[t], before)
        self.assertEqual(self.s.resume("A-secret", t, 0, "resume-0")["code"], "STALE_BINDING")
        self.assertEqual(self.s.transactions[t], before)
        self.assert_live(b"BASE", 0)

    def test_bad_numeric_fields_do_not_renew_lease(self):
        a = self.begin()
        t = a["token"]
        baseline = copy.deepcopy(self.s.transactions[t])
        for bad in [False, True, 0.0, "0", None, -1]:
            self.assertNotEqual(self.s.read("A-secret", t, 0, bad, 1)["code"], "OK")
            self.assertNotEqual(self.s.read("A-secret", t, 0, 0, bad)["code"], "OK")
            self.assertNotEqual(self.s.stage("A-secret", t, 0, 0, bad, digest(b"EDIT"))["code"], "OK")
            self.assertNotEqual(self.s.advance(bad)["code"], "OK")
            self.assertEqual(self.s.transactions[t], baseline)

    def test_full_project_sized_opaque_bytes_and_hidden_tail_copy(self):
        baseline = bytes(i % 256 for i in range(36452))
        intended = bytearray(baseline)
        for offset in [0, 1023, len(intended) - 1]:
            intended[offset] ^= 0xFF
        intended = bytes(intended)
        s = Server(baseline)
        a = self.begin(s=s)
        t = a["token"]
        read_back = b""
        for offset in range(0, len(baseline), 224):
            out = s.read("A-secret", t, 0, offset, min(224, len(baseline) - offset))
            self.assertEqual(out["code"], "OK")
            read_back += out["data"]
        self.assertEqual(read_back, baseline)
        self.assertEqual(s.stage("A-secret", t, 0, 0, len(intended), digest(intended))["code"], "OK")
        for offset in range(0, len(intended), 224):
            part = intended[offset:offset + 224]
            self.assertEqual(s.write("A-secret", t, 0, offset, part)["code"], "OK")
            self.assertEqual(s.write("A-secret", t, 0, offset, part)["code"], "OK")
        self.assertEqual(s.validate("A-secret", t, 0)["code"], "OK")
        self.assertEqual(s.commit("A-secret", t, 0, 0, digest(intended))["code"], "COMMITTED")
        self.assert_live(intended, 1, s)

    def test_validate_retry_does_not_renew_lease(self):
        t, g, _ = self.prepared()
        deadline = self.s.transactions[t].idle_deadline
        self.s.advance(14)
        self.assertEqual(self.s.validate("A-secret", t, g)["code"], "OK")
        self.assertEqual(self.s.transactions[t].idle_deadline, deadline)
        self.s.advance(1)
        self.assertEqual(self.s.status("A-secret", t)["code"], "EXPIRED")
        self.assert_live(b"BASE", 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
