# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic read-only lease/stream semantics, NOT a device protocol or backend."""
from dataclasses import dataclass
from hashlib import sha256

SIZE = 0x100000
CHUNK = 224
CURRENT_SUPPORTED = False


class Refusal(ValueError):
    pass


@dataclass(frozen=True)
class Binding:
    unit: str
    model: str
    session: str
    challenge: str
    epoch: int
    selection: str


class Capture:
    """One synthetic owner lease. No I/O, writer, raw export or auto-resume."""
    def __init__(self, binding, *, supported=CURRENT_SUPPORTED, now=0, deadline=1):
        if not supported:
            raise Refusal('UNSUPPORTED')
        if (binding.model not in ('pro', 'elite') or binding.epoch < 0 or
                not all((binding.unit, binding.session, binding.challenge, binding.selection)) or
                deadline <= now):
            raise Refusal('BINDING_OR_DEADLINE')
        self.binding = binding
        self.deadline = deadline
        self.state = 'CAPTURING'
        self.offset = 0
        self.full = sha256()
        self.head = sha256()
        self.digest = None
        self.used_challenges = {binding.challenge}

    def fail(self, reason):
        self.state = 'INVALID'
        raise Refusal(reason)

    def live(self, binding, now):
        if self.state not in ('CAPTURING', 'COMPLETE_HELD', 'REVALIDATED_HELD'):
            self.fail('STATE')
        if binding != self.binding or now >= self.deadline:
            self.fail('STALE_OR_FOREIGN')

    def chunk(self, binding, offset, data, now):
        self.live(binding, now)
        if self.state != 'CAPTURING':
            self.fail('STATE')
        if (not isinstance(data, bytes) or offset != self.offset or
                not 0 < len(data) <= CHUNK or offset + len(data) > SIZE):
            self.fail('COVERAGE')
        self.full.update(data)
        if offset < 0x4000:
            self.head.update(data[:min(len(data), 0x4000-offset)])
        self.offset += len(data)

    def finish(self, binding, full_digest, head_digest, now):
        self.live(binding, now)
        if self.state != 'CAPTURING' or self.offset != SIZE:
            self.fail('INCOMPLETE')
        if (full_digest, head_digest) != (self.full.hexdigest(), self.head.hexdigest()):
            self.fail('TARGET_DIGEST_MISMATCH')
        self.digest = (full_digest, head_digest)
        self.state = 'COMPLETE_HELD'
        # No raw bytes retained/exported; host private backup is a separate future component.
        return self.digest

    def revalidate(self, binding, challenge, full_digest, head_digest, now):
        self.live(binding, now)
        if self.state != 'COMPLETE_HELD' or not challenge or challenge in self.used_challenges:
            self.fail('STATE_OR_REPLAY')
        self.used_challenges.add(challenge)
        if (full_digest, head_digest) != self.digest:
            self.fail('TARGET_DIGEST_MISMATCH')
        self.state = 'REVALIDATED_HELD'

    def eligibility_only(self, binding, now):
        # Atomic live check at any downstream use; NEVER a commit grant or operation.
        self.live(binding, now)
        if self.state != 'REVALIDATED_HELD':
            self.fail('STATE')
        return 'READONLY_EVIDENCE_CURRENT_NOT_WRITE_AUTHORITY'

    def invalidate(self, reason):
        # Disconnect, reboot, package change, attempted mutation, expiry or core fault.
        self.state = 'INVALID'

    def cancel(self, binding):
        if binding != self.binding:
            raise Refusal('FOREIGN_OWNER')  # foreign cancellation must not release this lease
        self.state = 'CANCELLED'
