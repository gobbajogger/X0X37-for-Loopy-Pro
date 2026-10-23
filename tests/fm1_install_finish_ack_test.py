#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only OTA finish reply reliability test. No real device, MIDI, or flash.

    python3 tests/fm1_install_finish_ack_test.py
    python3 tests/fm1_install_finish_ack_test.py /path/to/extracted_source.py
"""
import importlib.util
from pathlib import Path
import sys
import unittest

source_path = (Path(sys.argv.pop(1)) if len(sys.argv) > 1
               else Path(__file__).resolve().parents[1] / 'tools' / 'fm1_install.py')
spec = importlib.util.spec_from_file_location('x0x_install_ota_finish_under_test', source_path)
install = importlib.util.module_from_spec(spec)
spec.loader.exec_module(install)


class Link:
    def __init__(self, requests, *, reject=None, drop_when_empty=True):
        self.requests = list(requests)
        self.reject = set(reject or [])
        self.sent = []
        self.drop_when_empty = drop_when_empty

    @property
    def lost(self):
        return not self.requests and self.drop_when_empty

    def read(self, timeout):
        return self.requests.pop(0) if self.requests else None

    def send(self, packet):
        self.sent.append(packet)
        # The live MidoLink.send() returns False when MIDI send raises.
        # Source-matched extracted fixtures and the complete source both use
        # this interface; no real port is opened here.
        if isinstance(packet, tuple):
            addr = packet[0]
        else:
            # In complete-source mode, response() returns the MIDI/SysEx
            # packet. The marker is recorded from its data payload, not
            # otherwise changed or interpreted.
            addr = self.sent_addr.pop(0)
        return addr not in self.reject


class FinishAckTest(unittest.TestCase):
    @staticmethod
    def packet(addr, n=0, flags=0):
        return flags, addr, n

    def run_serve(self, requests, finish, reject=(), image=b'abcdef'):
        # Updater.serve reads parse_request() output. For the full module,
        # patch only the wire parser and response encoder: all decisions stay
        # in the actual source method. For the isolated fixture the same
        # interface is already provided; patching remains valid.
        from unittest import mock
        link = Link(requests, reject=reject)
        link.sent_addr = [v[1] for v in requests]
        # A response failure for one command must not be mistaken for success.
        with mock.patch.object(install, 'parse_request', side_effect=lambda p: p), \
             mock.patch.object(install, 'response', side_effect=lambda addr, buf, fl=0: (addr, bytes(buf), fl)), \
             mock.patch.object(install.time, 'sleep', return_value=None):
            events = []
            outcome = install.Updater.__new__(install.Updater).serve(link, image, finish,
                                                                    0.0, lambda *args: events.append(args))
        return outcome, link.sent, events

    def test_finish_check_reply_succeeds(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_CHECK)], install.FINISH_CHECK)
        self.assertEqual(got, (0, True, False))
        self.assertEqual(len(sent), 1)

    def test_finish_check_reply_failure_is_connection_lost(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_CHECK)], install.FINISH_CHECK,
                                      reject=(install.FINISH_CHECK,))
        self.assertEqual(got, (0, False, True))
        self.assertEqual(len(sent), 1)

    def test_finish_write_reply_succeeds(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_WRITE)], install.FINISH_WRITE)
        self.assertEqual(got, (0, True, False))
        self.assertEqual(len(sent), 1)

    def test_finish_write_reply_failure_is_connection_lost(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_WRITE)], install.FINISH_WRITE,
                                      reject=(install.FINISH_WRITE,))
        self.assertEqual(got, (0, False, True))
        self.assertEqual(len(sent), 1)

    def test_nonmatching_finish_reply_failure_is_terminal(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_CHECK), self.packet(install.FINISH_WRITE)],
                                      install.FINISH_WRITE, reject=(install.FINISH_CHECK,))
        self.assertEqual(got, (0, False, True))
        self.assertEqual(len(sent), 1)

    def test_nonmatching_finish_reply_success_continues(self):
        got, sent, _ = self.run_serve([self.packet(install.FINISH_CHECK), self.packet(install.FINISH_WRITE)],
                                      install.FINISH_WRITE)
        self.assertEqual(got, (0, True, False))
        self.assertEqual(len(sent), 2)

    def test_bad_data_reply_failure_still_aborts(self):
        got, sent, events = self.run_serve([self.packet(0, 2)], install.FINISH_WRITE, reject=(0,))
        self.assertEqual(got, (0, False, True))
        self.assertFalse(events)
        self.assertEqual(len(sent), 1)

    def test_successful_data_response_preserves_progress(self):
        got, sent, events = self.run_serve([self.packet(0, 2), self.packet(install.FINISH_WRITE)],
                                           install.FINISH_WRITE)
        self.assertEqual(got, (1, True, False))
        self.assertEqual(len(sent), 2)
        self.assertEqual(events, [(1, 2)])

    def test_empty_link_reports_disconnection_not_success(self):
        got, sent, events = self.run_serve([], install.FINISH_CHECK)
        self.assertEqual(got, (0, False, True))
        self.assertFalse(sent)
        self.assertFalse(events)


if __name__ == '__main__':
    unittest.main(verbosity=2)
