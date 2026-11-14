"""Focused executable contract cases. Only synthetic project bytes are used."""
import unittest
from model import Server, Token, digest


class ContractTests(unittest.TestCase):
    def opened(self, s, owner="owner-A", request="request-A"):
        reply = s.begin(owner, request, s.epoch)
        self.assertEqual(reply["code"], "OK")
        return reply["token"]

    def prepared(self, s, token, payload=b"A123", owner="owner-A", generation=0):
        revision = s.transactions[token].base_revision
        self.assertEqual(s.stage(owner, token, generation, revision, len(payload), digest(payload))["code"], "OK")
        self.assertEqual(s.write(owner, token, generation, 0, payload)["code"], "OK")
        self.assertEqual(s.validate(owner, token, generation)["code"], "OK")
        return revision, digest(payload)

    def test_snapshot_and_publication_are_distinct(self):
        s = Server(b"old!")
        token = self.opened(s)
        self.prepared(s, token)
        self.assertEqual(s.live, b"old!")
        self.assertEqual(s.read("owner-A", token, 0, 0, 4)["data"], b"old!")
        self.assertEqual(s.commit("owner-A", token, 0, 0, digest(b"A123"))["code"], "COMMITTED")
        self.assertEqual((s.live, s.revision, len(s.journal)), (b"A123", 1, 1))

    def test_busy_begin_grants_no_abort_authority(self):
        s = Server(b"old!")
        token = self.opened(s)
        self.assertEqual(s.begin("owner-B", "request-B", s.epoch)["code"], "BUSY")
        for response in [s.abort("owner-B", token, 0), s.cancel("owner-B", "request-A", s.epoch),
                         s.commit("owner-B", token, 0, 0, digest(b"old!"))]:
            self.assertEqual(response["code"], "FOREIGN")
        self.assertEqual(s.active, token)
        self.assertEqual(s.live, b"old!")

    def test_local_edit_after_validation_stales_commit(self):
        s = Server(b"old!")
        token = self.opened(s)
        base, candidate = self.prepared(s, token)
        self.assertEqual(s.local_edit(b"B234")["code"], "OK")
        self.assertEqual(s.commit("owner-A", token, 0, base, candidate)["code"], "CONFLICT")
        self.assertEqual(s.live, b"B234")
        self.assertEqual(s.read("owner-A", token, 0, 0, 4)["data"], b"old!")
        self.assertEqual(s.transactions[token].staged, b"A123")

    def test_aba_edit_is_still_stale(self):
        s = Server(b"old!")
        token = self.opened(s)
        base, candidate = self.prepared(s, token)
        s.local_edit(b"B234")
        s.local_edit(b"old!")
        self.assertEqual(s.commit("owner-A", token, 0, base, candidate)["code"], "CONFLICT")
        self.assertEqual(s.revision, 2)

    def test_lost_begin_and_commit_replies_are_retryable(self):
        s = Server(b"old!")
        first = s.begin("owner-A", "request-A", s.epoch)  # reply lost
        retry = s.begin("owner-A", "request-A", s.epoch)
        self.assertEqual(first, retry)
        self.prepared(s, first["token"])
        result = s.commit("owner-A", first["token"], 0, 0, digest(b"A123"))  # reply lost
        self.assertEqual(s.commit("owner-A", first["token"], 0, 0, digest(b"A123")), result)
        self.assertEqual(s.status("owner-A", first["token"]), result)
        self.assertEqual(s.abort("owner-A", first["token"], 0), result)
        self.assertEqual(s.cancel("owner-A", "request-A", s.epoch), result)
        self.assertEqual(len(s.journal), 1)

    def test_cancellation_tombstone_precedes_delayed_begin(self):
        s = Server(b"old!")
        self.assertEqual(s.cancel("owner-A", "request-A", s.epoch)["code"], "ABORTED")
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "ABORTED")
        self.assertIsNone(s.active)
        self.assertEqual(s.live, b"old!")

    def test_expiry_then_foreign_begin_then_old_abort(self):
        s = Server(b"old!")
        a = self.opened(s)
        self.prepared(s, a)
        s.advance(15)
        b = self.opened(s, "owner-B", "request-B")
        self.assertEqual(s.abort("owner-A", a, 0)["code"], "EXPIRED")
        self.assertEqual(s.active, b)
        self.assertEqual(s.live, b"old!")

    def test_reconnect_fences_delayed_frames_and_lost_resume_reply(self):
        s = Server(b"old!")
        a = self.opened(s)
        self.prepared(s, a)
        first = s.resume("owner-A", a, 0, "resume-1")
        self.assertEqual(first, s.resume("owner-A", a, 0, "resume-1"))
        self.assertEqual(first["generation"], 1)
        for response in [s.commit("owner-A", a, 0, 0, digest(b"A123")), s.abort("owner-A", a, 0),
                         s.write("owner-A", a, 0, 0, b"A123")]:
            self.assertEqual(response["code"], "STALE_BINDING")
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "STALE_BINDING")
        self.assertEqual(s.commit("owner-A", a, 1, 0, digest(b"A123"))["code"], "COMMITTED")

    def test_terminal_identity_cannot_resurrect(self):
        s = Server(b"old!")
        a = self.opened(s)
        s.abort("owner-A", a, 0)
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "ABORTED")
        self.assertEqual(s.resume("owner-A", a, 0, "resume-1")["code"], "ABORTED")
        self.assertEqual(s.live, b"old!")

    def test_foreign_malformed_and_legacy_packets_do_not_renew(self):
        s = Server(b"old!")
        a = self.opened(s)
        s.advance(14)
        for op in ["QUERY", "BEGIN", "WRITE", "COMMIT", "ABORT", "???"]:
            self.assertEqual(s.legacy(op, b"B234")["code"], "BUSY")
        s.keepalive("owner-B", a, 0)
        s.read("owner-A", a, 0, -1, 4)
        s.write("owner-A", a, 0, 0, b"A123")  # No manifest: invalid.
        self.assertEqual(s.transactions[a].idle_deadline, 15)
        s.advance(1)
        self.assertEqual(s.status("owner-A", a)["code"], "EXPIRED")

    def test_hard_deadline_wins_over_keepalives(self):
        s = Server(b"old!", idle_ttl=15, hard_ttl=30)
        a = self.opened(s)
        for _ in range(2):
            s.advance(14)
            self.assertEqual(s.keepalive("owner-A", a, 0)["code"], "OK")
        s.advance(2)
        self.assertEqual(s.keepalive("owner-A", a, 0)["code"], "EXPIRED")

    def test_chunk_retries_are_exact(self):
        s = Server(b"old!")
        a = self.opened(s)
        s.stage("owner-A", a, 0, 0, 4, digest(b"A123"))
        self.assertEqual(s.write("owner-A", a, 0, 2, b"23")["code"], "STATE")
        self.assertEqual(s.write("owner-A", a, 0, 0, b"A1")["code"], "OK")
        self.assertEqual(s.write("owner-A", a, 0, 0, b"A1")["code"], "OK")
        self.assertEqual(s.write("owner-A", a, 0, 0, b"B2")["code"], "INTENT_MISMATCH")
        self.assertEqual(s.write("owner-A", a, 0, 1, b"123")["code"], "INTENT_MISMATCH")
        self.assertEqual(s.write("owner-A", a, 0, 2, b"23")["code"], "OK")
        self.assertEqual(s.validate("owner-A", a, 0)["code"], "OK")

    def test_manifest_and_semantic_validation_both_required(self):
        s = Server(b"old!", validator=lambda b: b != b"bad!")
        a = self.opened(s)
        s.stage("owner-A", a, 0, 0, 4, digest(b"bad!"))
        s.write("owner-A", a, 0, 0, b"bad!")
        self.assertEqual(s.validate("owner-A", a, 0)["code"], "FORMAT")
        self.assertEqual(s.commit("owner-A", a, 0, 0, digest(b"bad!"))["code"], "STATE")
        self.assertEqual(s.live, b"old!")

    def test_no_silent_receipt_eviction_or_counter_wrap(self):
        s = Server(b"old!", max_transactions=1, max_revision=0)
        a = self.opened(s)
        self.prepared(s, a)
        self.assertEqual(s.commit("owner-A", a, 0, 0, digest(b"A123"))["code"], "EXHAUSTED")
        s.abort("owner-A", a, 0)
        self.assertEqual(s.begin("owner-B", "request-B", s.epoch)["code"], "EXHAUSTED")
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "ABORTED")
        self.assertEqual(s.live, b"old!")

    def test_epoch_change_never_replays_uncertain_transaction(self):
        old = Server(b"old!", epoch="old-boot")
        a = self.opened(old)
        self.prepared(old, a)
        new = Server(b"old!", epoch="new-boot")
        self.assertEqual(new.commit("owner-A", a, 0, 0, digest(b"A123"))["code"], "STALE_EPOCH")
        self.assertEqual(new.live, b"old!")

    def test_legacy_resource_interlock_and_revision_coverage(self):
        s = Server(b"old!")
        self.assertEqual(s.legacy("BEGIN")["code"], "OK")
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "BUSY")
        s.legacy("WRITE", b"B234")
        s.legacy("COMMIT")
        self.assertEqual((s.live, s.revision), (b"B234", 1))
        a = self.opened(s)
        self.assertEqual(s.transactions[a].base_revision, 1)

    def test_unsupported_server_is_fail_closed(self):
        s = Server(b"old!", supported=False)
        self.assertEqual(s.capabilities()["code"], "UNSUPPORTED")
        self.assertEqual(s.begin("owner-A", "request-A", s.epoch)["code"], "UNSUPPORTED")
        self.assertEqual(s.cancel("owner-A", "request-A", s.epoch)["code"], "UNSUPPORTED")
        self.assertFalse(s.journal)

    def test_counter_configuration_is_bounded_and_canonical(self):
        for limit in (-1, 1 << 56, True, 1.0):
            with self.assertRaises(ValueError):
                Server(b"old!", max_revision=limit)


if __name__ == "__main__":
    unittest.main()
