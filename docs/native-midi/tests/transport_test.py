#!/usr/bin/env python3
"""Independent no-device checks of the actual native updater transport guards.

The in-memory backend speaks the updater's existing identity/read-request wire
format. No mido import, MIDI endpoint access, firmware rebuild, or installation.
"""
import io
import sys
import time
import unittest
from collections import deque
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(sys.argv.pop(1)).resolve() / "tools"))
import fm1_install as I

PRODUCT = "SMK-37 Elite_900"


def identity(text):
    raw = text.encode()
    payload = raw + bytes(27 - len(raw))
    return b"\xf0" + I.pack7(bytes([0, 0x59, 0x11, 27, 0, 0]) + payload + bytes([~sum(payload) & 255])) + b"\xf7"


def request(address, size=8):
    raw = bytearray([0, 0x59, 0x30, 0, 0, 0, 0])
    raw += address.to_bytes(4, "little") + size.to_bytes(3, "little")
    raw.append(~sum(raw[6:14]) & 255)
    return b"\xf0" + I.pack7(raw) + b"\xf7"


class Link:
    def __init__(self, backend, name):
        self.backend, self.name, self.generation = backend, name, backend.generation
        self.q, self.closed, self.fail_upgrade = deque(), False, False

    @property
    def lost(self):
        return self.generation != self.backend.generation or self.name not in self.backend.inputs

    def send(self, data):
        if self.lost:
            return False
        if data == I.HS_QUERY:
            self.q.append(identity(self.backend.ident))
        elif data == I.UPGRADE:
            if self.fail_upgrade:
                return False
            self.backend.upgrades.append(self.backend.ident)
            self.q.append(request(I.FINISH_WRITE if self.backend.ident.startswith("ota-") else I.FINISH_CHECK))
        elif data[:1] == b"\xf0":
            raw = I.unpack7(data[1:-1])
            if raw[:3] == b"\0\x59\x30":
                address = int.from_bytes(raw[7:11], "little")
                if address == I.FINISH_CHECK:
                    self.backend.boot(self.backend.loader, "Felucca Update")
                    if self.backend.ambiguous_loader:
                        self.backend.inputs.append("SMK-37 Pro")
                        self.backend.outputs.append("SMK-37 Pro")
                elif address == I.FINISH_WRITE:
                    self.backend.boot(self.backend.after, "Felucca")
        return True

    def read(self, timeout):
        if self.q:
            return self.q.popleft()
        return None

    def drain(self):
        self.q.clear()

    def close(self):
        self.closed = True


class Backend:
    def __init__(self, ident=PRODUCT, after=PRODUCT, loader="ota-" + PRODUCT):
        self.generation, self.upgrades, self.links = 0, [], []
        self.after, self.loader, self.ambiguous_loader = after, loader, False
        self.boot(ident, "X0X FM-1")

    def boot(self, ident, name):
        self.ident = ident
        self.inputs = [name] if ident is not None else []
        self.outputs = list(self.inputs)
        self.generation += 1

    def input_names(self):
        return list(self.inputs)

    def output_names(self):
        return list(self.outputs)

    def open(self, name, output):
        assert name in self.inputs and output in self.outputs
        link = Link(self, name)
        self.links.append(link)
        return link


class Independent(unittest.TestCase):
    def setUp(self):
        self.delay = patch.dict(I.DELAY, open=0, start=0, reply=0, loader=0, reboot=0,
                                retry=0.001, poll=0.001, hs=0.003, idle_check=0.02,
                                idle_write=0.02, wait_loader=0.025, wait_reboot=0.025)
        self.delay.start()

    def tearDown(self):
        self.delay.stop()

    def install(self, backend):
        updater = I.Updater(backend)
        dev = updater.find()
        result = updater.install(dev, b"existing wire fixture", PRODUCT)
        return updater, result

    def test_same_target_normal_mode_transition(self):
        backend = Backend()
        updater, result = self.install(backend)
        self.assertEqual(result, PRODUCT)
        self.assertEqual(backend.upgrades, [PRODUCT, "ota-" + PRODUCT])

    def test_pro_device_rejected_for_elite_before_upgrade(self):
        backend = Backend(ident="SMK-37 Pro_900")
        with self.assertRaises(I.InstallError):
            self.install(backend)
        self.assertEqual(backend.upgrades, [])

    def test_pro_loader_rejected_before_loader_upgrade(self):
        backend = Backend(loader="ota-SMK-37 Pro_900")
        with self.assertRaises(I.InstallError):
            self.install(backend)
        self.assertEqual(backend.upgrades, [PRODUCT])

    def test_two_compatible_ports_rejected_before_upgrade(self):
        backend = Backend()
        backend.inputs.append("SMK-37 Pro")
        backend.outputs.append("SMK-37 Pro")
        with self.assertRaises(I.InstallError):
            self.install(backend)
        self.assertEqual(backend.upgrades, [])

    def test_duplicate_native_port_names_are_ambiguous(self):
        backend = Backend()
        backend.inputs *= 2
        backend.outputs *= 2
        with self.assertRaises(I.InstallError):
            self.install(backend)
        self.assertEqual(backend.upgrades, [])

    def test_unrelated_sole_output_is_not_paired(self):
        self.assertIsNone(I.pair_output("X0X FM-1", ["Other Keyboard"]))

    def test_new_compatible_port_blocks_loader_write(self):
        backend = Backend()
        backend.ambiguous_loader = True
        with self.assertRaises(I.InstallError):
            self.install(backend)
        self.assertEqual(backend.upgrades, [PRODUCT])

    def test_stale_initial_handle_rejected_before_upgrade(self):
        backend = Backend()
        updater = I.Updater(backend)
        dev = updater.find()
        backend.boot(PRODUCT, "X0X FM-1")
        with self.assertRaises(I.InstallError):
            updater.install(dev, b"existing wire fixture", PRODUCT)
        self.assertEqual(backend.upgrades, [])

    def test_wrong_version_after_reboot_is_failure(self):
        backend = Backend(after="SMK-37 Elite_016")
        with self.assertRaises(I.InstallError) as caught:
            self.install(backend)
        self.assertIn(caught.exception.code, ("mismatch", "noreturn"))

    def test_absent_reboot_is_failure(self):
        backend = Backend(after=None)
        start = time.monotonic()
        with self.assertRaises(I.InstallError) as caught:
            self.install(backend)
        self.assertEqual(caught.exception.code, "noreturn")
        self.assertLess(time.monotonic() - start, 1)

    def test_verified_port_change_refused_for_readback_or_restore(self):
        backend = Backend()
        updater, _ = self.install(backend)
        backend.boot(PRODUCT, "SMK-37 Elite renamed")
        with self.assertRaises(I.InstallError):
            updater.wait_for(lambda ident: not ident.loader, 0.025)

    def test_verified_stale_version_refused_before_readback_or_restore(self):
        backend = Backend()
        updater, _ = self.install(backend)
        backend.boot("SMK-37 Elite_016", "Felucca")
        with self.assertRaises(I.InstallError):
            updater.wait_for(lambda ident: not ident.loader, 0.025)

    def test_controller_readback_transport_failure_is_not_success(self):
        backend = Backend()
        link = backend.open(backend.inputs[0], backend.outputs[0])
        backend.boot(None, "gone")
        with patch.dict(I.CTL_READ, timeout=0.001, tries=1):
            with self.assertRaises(I.InstallError) as caught:
                I.check_ctl(link, b"controller fixture", io.StringIO())
        self.assertEqual(caught.exception.code, "ctlcrc")

    def test_malformed_packet_stream_cannot_defeat_idle_timeout(self):
        class Noise:
            lost = False
            reads = 0

            def read(self, timeout):
                self.reads += 1
                if self.reads > 100:
                    raise AssertionError("malformed traffic defeated the idle timeout")
                return b"\xf0\x7d\x01\xf7"

        clock = [0.0]

        def monotonic():
            clock[0] += 0.005
            return clock[0]

        link = Noise()
        with patch.object(I.time, "monotonic", monotonic):
            served, finished, lost = I.Updater(Backend()).serve(
                link, b"fixture", I.FINISH_CHECK, 0.02, lambda *args: None)
        self.assertEqual(served, 0)
        self.assertFalse(finished)
        self.assertFalse(lost)

    def test_wrong_stage_finish_is_not_acknowledged(self):
        class WrongFinish:
            lost = False

            def __init__(self):
                self.packets, self.sent = deque([request(I.FINISH_WRITE)]), []

            def read(self, timeout):
                return self.packets.popleft() if self.packets else None

            def send(self, data):
                self.sent.append(data)
                return True

        link = WrongFinish()
        try:
            _, finished, _ = I.Updater(Backend()).serve(
                link, b"fixture", I.FINISH_CHECK, 0.005, lambda *args: None)
            self.assertFalse(finished)
        except I.InstallError:
            pass
        self.assertEqual(link.sent, [])

    def test_valid_repeated_requests_cannot_defeat_total_timeout(self):
        class Repeats:
            lost = False
            reads = 0

            def read(self, timeout):
                self.reads += 1
                if self.reads > 100:
                    raise AssertionError("valid repeated traffic defeated the total timeout")
                return request(0, 1)

            def send(self, data):
                return True

        clock = [0.0]

        def monotonic():
            clock[0] += 0.005
            return clock[0]

        link = Repeats()
        with patch.object(I.time, "monotonic", monotonic), patch.dict(I.DELAY, total_check=0.04):
            served, finished, lost = I.Updater(Backend()).serve(
                link, b"fixture", I.FINISH_CHECK, 0.02, lambda *args: None)
        self.assertGreater(served, 0)
        self.assertFalse(finished)
        self.assertFalse(lost)

    def test_failed_upgrade_send_stops_and_closes_link(self):
        backend = Backend()
        updater = I.Updater(backend)
        dev = updater.find()
        dev.link.fail_upgrade = True
        with self.assertRaises(I.InstallError) as caught:
            updater.install(dev, b"fixture", PRODUCT)
        self.assertEqual(caught.exception.code, "lost")
        self.assertEqual(backend.upgrades, [])
        self.assertTrue(backend.links[0].closed)


if __name__ == "__main__":
    unittest.main(verbosity=2)
