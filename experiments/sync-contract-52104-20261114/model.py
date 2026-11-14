"""Isolated semantic prototype; no firmware, MIDI, storage or device adapter.

Opaque owner strings stand for bearer capabilities, not authenticated principals.
All methods execute as indivisible events on a single deterministic model clock.
"""
from dataclasses import dataclass, field
from hashlib import sha256


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


@dataclass(frozen=True)
class Token:
    epoch: str
    serial: int


@dataclass
class Transaction:
    owner: str
    request: str
    token: Token
    base_revision: int
    snapshot: bytes
    phase: str
    generation: int
    idle_deadline: int
    hard_deadline: int
    manifest: tuple | None = None
    staged: bytes = b""
    resumes: dict = field(default_factory=dict)
    outcome: str | None = None
    result_revision: int | None = None
    result_digest: str | None = None


class Server:
    CHUNK_MAX = 224
    RESUMES_MAX = 8
    COUNTER_MAX = (1 << 56) - 1

    def __init__(self, initial: bytes, *, epoch="boot-1", supported=True,
                 max_transactions=32, idle_ttl=15, hard_ttl=60,
                 max_revision=(1 << 56) - 1, validator=None):
        if not isinstance(initial, bytes) or not 0 < len(initial) < (1 << 21):
            raise ValueError("initial must be bounded, immutable bytes")
        if (type(epoch) is not str or not epoch
                or type(idle_ttl) is not int or idle_ttl <= 0
                or type(hard_ttl) is not int or hard_ttl < idle_ttl
                or type(max_transactions) is not int or not 1 <= max_transactions <= 65535
                or type(max_revision) is not int or not 0 <= max_revision <= self.COUNTER_MAX):
            raise ValueError("invalid epoch or resource bounds")
        self.live = initial
        self.epoch = epoch
        self.supported = supported
        self.revision = 0
        self.now = 0
        self.max_transactions = max_transactions
        self.idle_ttl = idle_ttl
        self.hard_ttl = hard_ttl
        self.max_revision = max_revision
        self.validator = validator or (lambda value: len(value) == len(initial))
        self.active = None
        self.transactions = {}
        self.requests = {}
        self.journal = []
        self.legacy_active = False
        self.legacy_staged = b""
        self.serial = 0

    @staticmethod
    def _name(value):
        # Symbolic names in the model; the wire proposal specifies 128-bit values.
        return isinstance(value, str) and 0 < len(value) <= 128

    def capabilities(self):
        return dict(code="OK" if self.supported else "UNSUPPORTED", version=2,
                    features=("owner", "snapshot", "cas", "resume", "receipt", "cancel")
                    if self.supported else (), epoch=self.epoch, revision=self.revision,
                    max_chunk=self.CHUNK_MAX, idle_ttl=self.idle_ttl,
                    hard_ttl=self.hard_ttl, max_transactions=self.max_transactions,
                    durability=False, samples=False)

    def _finish(self, tx, outcome):
        tx.outcome = outcome
        tx.phase = "TERMINAL"
        # Only bounded receipt metadata persists after a terminal outcome.
        tx.snapshot = b""
        tx.staged = b""
        if self.active == tx.token:
            self.active = None

    def _expire(self):
        if self.active is not None:
            tx = self.transactions[self.active]
            if self.now >= min(tx.idle_deadline, tx.hard_deadline):
                self._finish(tx, "EXPIRED")

    def advance(self, delta):
        if type(delta) is not int or delta < 0:
            return dict(code="FORMAT")
        self.now += delta
        self._expire()
        return dict(code="OK")

    def _lookup(self, owner, token):
        self._expire()
        if not self.supported:
            return None, dict(code="UNSUPPORTED")
        if (type(token) is not Token or type(token.epoch) is not str or not token.epoch
                or type(token.serial) is not int or not 1 <= token.serial <= self.COUNTER_MAX):
            return None, dict(code="FORMAT")
        if token.epoch != self.epoch:
            return None, dict(code="STALE_EPOCH")
        tx = self.transactions.get(token)
        if tx is None:
            return None, dict(code="UNKNOWN")
        if owner != tx.owner:
            return None, dict(code="FOREIGN")
        return tx, None

    @staticmethod
    def _terminal(tx):
        result = dict(code=tx.outcome)
        if tx.outcome == "COMMITTED":
            result.update(revision=tx.result_revision, digest=tx.result_digest)
        return result

    def _bound(self, owner, token, generation):
        tx, error = self._lookup(owner, token)
        if error:
            return None, error
        if tx.outcome:
            return tx, self._terminal(tx)
        if type(generation) is not int or generation != tx.generation:
            return tx, dict(code="STALE_BINDING")
        return tx, None

    def _renew(self, tx):
        tx.idle_deadline = min(self.now + self.idle_ttl, tx.hard_deadline)

    def begin(self, owner, request, epoch):
        self._expire()
        if not self.supported:
            return dict(code="UNSUPPORTED")
        if not self._name(owner) or not self._name(request):
            return dict(code="FORMAT")
        if epoch != self.epoch:
            return dict(code="STALE_EPOCH")
        # Request IDs are globally unique within an epoch. Never overwrite a
        # known request, including cancellation before a delayed BEGIN arrives.
        known = self.requests.get(request)
        if known is not None:
            tx = self.transactions[known]
            if owner != tx.owner:
                return dict(code="FOREIGN")
            if tx.outcome:
                return self._terminal(tx)
            if tx.generation != 0:
                return dict(code="STALE_BINDING")
            return self._opened(tx)  # Does not renew the lease on retry.
        if self.active is not None or self.legacy_active:
            return dict(code="BUSY")
        if len(self.transactions) >= self.max_transactions or self.serial >= self.COUNTER_MAX:
            return dict(code="EXHAUSTED")
        self.serial += 1
        token = Token(self.epoch, self.serial)
        tx = Transaction(owner, request, token, self.revision, self.live,
                         "SNAPSHOT", 0, self.now + self.idle_ttl,
                         self.now + self.hard_ttl)
        self.transactions[token] = tx
        self.requests[request] = token
        self.active = token
        return self._opened(tx)

    @staticmethod
    def _opened(tx):
        return dict(code="OK", token=tx.token, revision=tx.base_revision,
                    snapshot_digest=digest(tx.snapshot), length=len(tx.snapshot),
                    generation=0)

    def read(self, owner, token, generation, offset, count):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        if (type(offset) is not int or type(count) is not int or offset < 0
                or not 0 < count <= self.CHUNK_MAX or offset + count > len(tx.snapshot)):
            return dict(code="FORMAT")
        # READ is repeatable and immutable, even after staging starts. READ does
        # not extend a lease; KEEPALIVE is explicit and subject to a hard limit.
        return dict(code="OK", data=tx.snapshot[offset:offset + count])

    def stage(self, owner, token, generation, revision, length, expected_digest):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        if type(revision) is not int or not 0 <= revision <= self.COUNTER_MAX:
            return dict(code="FORMAT")
        if revision != tx.base_revision or revision != self.revision:
            return dict(code="CONFLICT", revision=self.revision)
        if (type(length) is not int or length != len(tx.snapshot)
                or not isinstance(expected_digest, str) or len(expected_digest) != 64
                or any(c not in "0123456789abcdef" for c in expected_digest)):
            return dict(code="FORMAT")
        manifest = (length, expected_digest)
        if tx.manifest is not None:
            return (dict(code="OK", next=len(tx.staged)) if tx.manifest == manifest
                    else dict(code="INTENT_MISMATCH"))
        tx.manifest = manifest
        tx.phase = "WRITING"
        self._renew(tx)
        return dict(code="OK", next=0)

    def write(self, owner, token, generation, offset, data):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        if tx.manifest is None:
            return dict(code="STATE")
        if (type(offset) is not int or offset < 0 or not isinstance(data, bytes)
                or not 0 < len(data) <= self.CHUNK_MAX or offset + len(data) > tx.manifest[0]):
            return dict(code="FORMAT")
        if offset < len(tx.staged):
            if offset + len(data) <= len(tx.staged) and tx.staged[offset:offset + len(data)] == data:
                return dict(code="OK", next=len(tx.staged))
            return dict(code="INTENT_MISMATCH")
        if offset != len(tx.staged):
            return dict(code="STATE")
        tx.staged += data
        tx.phase = "WRITING"
        self._renew(tx)
        return dict(code="OK", next=len(tx.staged))

    def validate(self, owner, token, generation):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        if tx.manifest is None or len(tx.staged) != tx.manifest[0]:
            return dict(code="STATE")
        if digest(tx.staged) != tx.manifest[1] or not self.validator(tx.staged):
            return dict(code="FORMAT")
        if tx.phase == "VALIDATED":
            return dict(code="OK", digest=tx.manifest[1])
        tx.phase = "VALIDATED"
        self._renew(tx)
        return dict(code="OK", digest=tx.manifest[1])

    def _publish(self, actor, data, token=None, validated=False):
        if self.revision >= self.max_revision:
            return dict(code="EXHAUSTED")
        before = self.live
        base = self.revision
        self.live = data
        self.revision += 1
        self.journal.append(dict(actor=actor, token=token, before=before, after=data,
                                 base_revision=base, revision=self.revision,
                                 validated=validated))
        return dict(code="OK")

    def commit(self, owner, token, generation, revision, expected_digest):
        tx, error = self._lookup(owner, token)
        if error:
            return error
        if type(revision) is not int or not 0 <= revision <= self.COUNTER_MAX:
            return dict(code="FORMAT")
        if tx.outcome:
            if tx.outcome == "COMMITTED" and (revision != tx.base_revision
                                              or expected_digest != tx.result_digest):
                return dict(code="INTENT_MISMATCH")
            return self._terminal(tx)
        if type(generation) is not int or generation != tx.generation:
            return dict(code="STALE_BINDING")
        if revision != tx.base_revision or revision != self.revision:
            return dict(code="CONFLICT", revision=self.revision)
        if tx.manifest is None or expected_digest != tx.manifest[1]:
            return dict(code="INTENT_MISMATCH")
        if tx.phase != "VALIDATED":
            return dict(code="STATE")
        # Model linearization point: revision comparison, publish and receipt
        # are ONE event. A firmware port must establish its own real barrier.
        result = self._publish(owner, tx.staged, token, validated=True)
        if result["code"] != "OK":
            return result
        tx.result_revision = self.revision
        tx.result_digest = tx.manifest[1]
        self._finish(tx, "COMMITTED")
        return self._terminal(tx)

    def abort(self, owner, token, generation):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        self._finish(tx, "ABORTED")
        return dict(code="ABORTED")

    def cancel(self, owner, request, epoch):
        self._expire()
        if not self.supported:
            return dict(code="UNSUPPORTED")
        if not self._name(owner) or not self._name(request):
            return dict(code="FORMAT")
        if epoch != self.epoch:
            return dict(code="STALE_EPOCH")
        known = self.requests.get(request)
        if known is not None:
            tx = self.transactions[known]
            if tx.owner != owner:
                return dict(code="FOREIGN")
            if tx.outcome:
                return self._terminal(tx)
            self._finish(tx, "ABORTED")
            return dict(code="ABORTED")
        if len(self.transactions) >= self.max_transactions or self.serial >= self.COUNTER_MAX:
            return dict(code="EXHAUSTED")
        self.serial += 1
        token = Token(self.epoch, self.serial)
        tx = Transaction(owner, request, token, self.revision, b"", "TERMINAL", 0, 0, 0,
                         outcome="ABORTED")
        self.transactions[token] = tx
        self.requests[request] = token
        return dict(code="ABORTED")

    def status(self, owner, token):
        tx, error = self._lookup(owner, token)
        if error:
            return error
        return self._terminal(tx) if tx.outcome else dict(
            code="ACTIVE", phase=tx.phase, next=len(tx.staged), revision=tx.base_revision)

    def resume(self, owner, token, generation, resume_request):
        tx, error = self._lookup(owner, token)
        if error:
            return error
        if tx.outcome:
            return self._terminal(tx)
        if not self._name(resume_request) or type(generation) is not int:
            return dict(code="FORMAT")
        prior = tx.resumes.get(resume_request)
        if prior is not None:
            old, new = prior
            if old != generation or new != tx.generation:
                return dict(code="STALE_BINDING")
            return dict(code="OK", generation=new)
        if generation != tx.generation:
            return dict(code="STALE_BINDING")
        if len(tx.resumes) >= self.RESUMES_MAX:
            return dict(code="EXHAUSTED")
        tx.generation += 1
        tx.resumes[resume_request] = (generation, tx.generation)
        self._renew(tx)
        return dict(code="OK", generation=tx.generation)

    def keepalive(self, owner, token, generation):
        tx, error = self._bound(owner, token, generation)
        if error:
            return error
        self._renew(tx)
        return dict(code="OK", idle_deadline=tx.idle_deadline,
                    hard_deadline=tx.hard_deadline)

    def local_edit(self, data):
        self._expire()
        if not isinstance(data, bytes) or len(data) != len(self.live) or not self.validator(data):
            return dict(code="FORMAT")
        if self.legacy_active:
            return dict(code="BUSY")
        # Trusted local writer; represented explicitly so a concurrent edit
        # invalidates the outstanding snapshot. No persistence semantics.
        return self._publish("LOCAL", data, validated=True)

    def legacy(self, op, data=b""):
        self._expire()
        if self.active is not None:
            return dict(code="BUSY")  # No v2 lease renewal, even for QUERY/ABORT.
        # Abstract legacy resource interlock only, NOT a v1 firmware emulator.
        if op == "QUERY":
            return dict(code="OK", active=self.legacy_active)
        if op == "BEGIN":
            if self.legacy_active:
                return dict(code="BUSY")
            self.legacy_active = True
            self.legacy_staged = self.live
            return dict(code="OK")
        if op == "ABORT":
            self.legacy_active = False
            self.legacy_staged = b""
            return dict(code="OK")
        if op == "WRITE":
            if not self.legacy_active:
                return dict(code="STATE")
            if not isinstance(data, bytes) or len(data) != len(self.live):
                return dict(code="FORMAT")
            self.legacy_staged = data
            return dict(code="OK")
        if op == "COMMIT":
            if not self.legacy_active:
                return dict(code="STATE")
            if not self.validator(self.legacy_staged):
                return dict(code="FORMAT")
            result = self._publish("LEGACY", self.legacy_staged, validated=True)
            if result["code"] == "OK":
                self.legacy_active = False
                self.legacy_staged = b""
            return result
        return dict(code="FORMAT")
