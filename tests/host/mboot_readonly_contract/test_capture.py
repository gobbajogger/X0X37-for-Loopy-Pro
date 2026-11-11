#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Only deterministic synthetic bytes; never reads a device or user file."""
from dataclasses import replace
from hashlib import sha256
import unittest
from capture_model import Binding, Capture, Refusal, SIZE, CHUNK

B = Binding('synthetic-unit-A', 'elite', 'synthetic-session', 'nonce-1', 7, 'synthetic-pins')
DATA = bytes(range(256)) * (SIZE // 256)
FULL, HEAD = sha256(DATA).hexdigest(), sha256(DATA[:0x4000]).hexdigest()


def fresh():
    return Capture(B, supported=True, now=0, deadline=100)


def complete():
    c = fresh()
    for offset in range(0, SIZE, CHUNK):
        c.chunk(B, offset, DATA[offset:offset+CHUNK], 1)
    c.finish(B, FULL, HEAD, 2)
    return c


class Tests(unittest.TestCase):
    def test_current_capability_refuses(self):
        with self.assertRaisesRegex(Refusal, 'UNSUPPORTED'):
            Capture(B)

    def test_exact_raw_stream_digests_and_held_lease(self):
        c = complete()
        self.assertEqual(c.digest, (FULL, HEAD))
        self.assertEqual(c.state, 'COMPLETE_HELD')
        c.revalidate(B, 'nonce-2', FULL, HEAD, 3)
        self.assertEqual(c.eligibility_only(B, 4), 'READONLY_EVIDENCE_CURRENT_NOT_WRITE_AUTHORITY')
        self.assertFalse(hasattr(c, 'write'))

    def test_invalid_binding(self):
        for field, value in [('unit',''), ('model','fm1'), ('session',''), ('challenge',''),
                             ('epoch',-1), ('selection','')]:
            with self.subTest(field=field), self.assertRaises(Refusal):
                Capture(replace(B, **{field:value}), supported=True)

    def test_expired_initial_lease(self):
        with self.assertRaises(Refusal):
            Capture(B, supported=True, now=1, deadline=1)

    def test_gaps_overlap_empty_oversize_negative(self):
        for off, data in [(1,b'x'), (-1,b'x'), (0,b''), (0,b'x'*(CHUNK+1))]:
            c=fresh()
            with self.subTest(off=off,n=len(data)), self.assertRaisesRegex(Refusal,'COVERAGE'):
                c.chunk(B,off,data,1)
        c=fresh(); c.chunk(B,0,b'x',1)
        with self.assertRaises(Refusal): c.chunk(B,0,b'x',1)

    def test_partial_finish(self):
        c=fresh(); c.chunk(B,0,b'x',1)
        with self.assertRaisesRegex(Refusal,'INCOMPLETE'): c.finish(B,FULL,HEAD,2)

    def test_foreign_binding_at_chunk(self):
        for field,value in [('unit','same-product-other-unit'),('model','pro'),('session','reboot'),
                            ('challenge','replay-other'),('epoch',8),('selection','other-package')]:
            c=fresh()
            with self.subTest(field=field), self.assertRaises(Refusal):
                c.chunk(replace(B,**{field:value}),0,b'x',1)

    def test_corrupt_stream_fails_target_digest(self):
        c=fresh()
        for off in range(0,SIZE,CHUNK):
            c.chunk(B,off,b'\0'*min(CHUNK,SIZE-off),1)
        with self.assertRaisesRegex(Refusal,'TARGET_DIGEST'): c.finish(B,FULL,HEAD,2)

    def test_wrong_head_or_full_revalidation(self):
        for full,head in [('0'*64,HEAD),(FULL,'0'*64)]:
            c=complete()
            with self.assertRaises(Refusal): c.revalidate(B,'nonce-2',full,head,3)

    def test_nonce_replay(self):
        c=complete()
        with self.assertRaisesRegex(Refusal,'REPLAY'): c.revalidate(B,B.challenge,FULL,HEAD,3)

    def test_lease_expiry_at_each_boundary(self):
        for phase in ['chunk','finish','revalidate','eligibility']:
            c=fresh() if phase in ['chunk','finish'] else complete()
            with self.subTest(phase=phase), self.assertRaises(Refusal):
                if phase=='chunk': c.chunk(B,0,b'x',100)
                elif phase=='finish': c.finish(B,FULL,HEAD,100)
                elif phase=='revalidate': c.revalidate(B,'nonce-2',FULL,HEAD,100)
                else:
                    c.revalidate(B,'nonce-2',FULL,HEAD,3)
                    c.eligibility_only(B,100)

    def test_invalidation_between_revalidate_and_use(self):
        for event in ['mutation-attempt','disconnect','reboot','package-change','core-fault']:
            c=complete(); c.revalidate(B,'nonce-2',FULL,HEAD,3); c.invalidate(event)
            with self.subTest(event=event), self.assertRaises(Refusal): c.eligibility_only(B,4)

    def test_changed_unit_epoch_selection_after_revalidate(self):
        for field,value in [('unit','other'),('epoch',8),('selection','other')]:
            c=complete(); c.revalidate(B,'nonce-2',FULL,HEAD,3)
            with self.subTest(field=field), self.assertRaises(Refusal):
                c.eligibility_only(replace(B,**{field:value}),4)

    def test_foreign_cancel_cannot_release_owner(self):
        c=complete()
        with self.assertRaisesRegex(Refusal,'FOREIGN_OWNER'): c.cancel(replace(B,unit='other'))
        self.assertEqual(c.state,'COMPLETE_HELD')
        c.cancel(B)
        with self.assertRaises(Refusal): c.revalidate(B,'nonce-2',FULL,HEAD,3)

    def test_extra_chunk_after_finish_and_double_finish_refused(self):
        c=complete()
        with self.assertRaises(Refusal): c.chunk(B,SIZE,b'x',3)
        c=complete()
        with self.assertRaises(Refusal): c.finish(B,FULL,HEAD,3)

    def test_invalid_state_cannot_resume(self):
        c=fresh(); c.invalidate('disconnect')
        with self.assertRaises(Refusal): c.chunk(B,0,b'x',1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
