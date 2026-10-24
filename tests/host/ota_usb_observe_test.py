#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Observation resource/selection tests; fake libusb, no USB traffic."""
from contextlib import contextmanager
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from ota_usb_observe import Descriptor, NativeUSB, USBError, observe  # noqa: E402


class API:
    def __init__(self, locators=('1:2', '1:3'), fail=False):
        self.locators, self.fail = locators, fail
        self.opened, self.released = [], False

    @contextmanager
    def devices(self):
        try:
            yield list(range(len(self.locators)))
        finally:
            self.released = True

    def describe(self, dev):
        return {'locator': self.locators[dev], 'serial_descriptor_present': False}

    def check_open_object(self, dev):
        self.opened.append(dev)
        if self.fail:
            raise USBError('libusb_open: -4')
        return True


class Tests(unittest.TestCase):
    def test_native_open_closes_on_pointer_mismatch(self):
        api, closed = NativeUSB.__new__(NativeUSB), []
        def opened(dev, target):
            target._obj.value = 99
            return 0
        api._open = opened
        api._get_device = lambda handle: 2
        api._close = lambda handle: closed.append(handle.value)
        with self.assertRaises(USBError):
            api.check_open_object(1)
        self.assertEqual(closed, [99])

    def test_native_open_closes_success_and_never_retries_failure(self):
        api, calls = NativeUSB.__new__(NativeUSB), []
        def opened(dev, target):
            calls.append(('open', dev))
            target._obj.value = 99
            return 0
        api._open = opened
        api._get_device = lambda handle: 1
        api._close = lambda handle: calls.append(('close', handle.value))
        self.assertTrue(api.check_open_object(1))
        self.assertEqual(calls, [('open', 1), ('close', 99)])
        api._open = lambda dev, target: -4
        with self.assertRaises(USBError):
            api.check_open_object(1)
        self.assertEqual(calls, [('open', 1), ('close', 99)])

    def test_native_list_releases_references_before_context(self):
        api, calls = NativeUSB.__new__(NativeUSB), []
        api._init = lambda ctx: 0
        api._get_device_list = lambda ctx, listing: 0
        api._free_device_list = lambda listing, unref: calls.append(('free', unref))
        api._exit = lambda ctx: calls.append(('exit',))
        with self.assertRaisesRegex(RuntimeError, 'consumer'):
            with api.devices() as devices:
                self.assertEqual(devices, [])
                raise RuntimeError('consumer')
        self.assertEqual(calls, [('free', 1), ('exit',)])

    def test_native_failed_list_exits_context(self):
        api, calls = NativeUSB.__new__(NativeUSB), []
        api._init = lambda ctx: 0
        api._get_device_list = lambda ctx, listing: -1
        api._free_device_list = lambda listing, unref: calls.append('free')
        api._exit = lambda ctx: calls.append('exit')
        with self.assertRaises(USBError):
            with api.devices():
                self.fail('failed listing yielded devices')
        self.assertEqual(calls, ['exit'])

    def test_descriptor_abi(self):
        import ctypes
        self.assertEqual(ctypes.sizeof(Descriptor), 18)
        self.assertEqual(Descriptor.iSerialNumber.offset, 16)

    def test_no_implicit_open(self):
        api = API()
        report = observe(api)
        self.assertEqual(api.opened, [])
        self.assertTrue(api.released)
        self.assertEqual(report['destructive_messages'], 0)
        self.assertFalse(report['serial_values_read'])
        self.assertEqual(report['cross_reboot_identity'], 'UNESTABLISHED')
        self.assertTrue(all(not d['selected_open_object_checked'] for d in report['devices']))

    def test_only_explicit_selection(self):
        api = API()
        report = observe(api, '1:3', 'loader')
        self.assertEqual(api.opened, [1])
        self.assertEqual(report['phase_label'], 'loader')
        self.assertEqual([d['selected_open_object_checked'] for d in report['devices']], [False, True])
        self.assertEqual(report['cross_reboot_identity'], 'UNESTABLISHED')

    def test_absent_or_ambiguous_never_opens(self):
        for locators in [('1:2',), ('1:3', '1:3')]:
            api = API(locators)
            with self.assertRaises(USBError):
                observe(api, '1:3')
            self.assertEqual(api.opened, [])
            self.assertTrue(api.released)

    def test_failed_open_does_not_fall_back(self):
        api = API(fail=True)
        with self.assertRaises(USBError):
            observe(api, '1:2')
        self.assertEqual(api.opened, [0])
        self.assertTrue(api.released)


if __name__ == '__main__':
    unittest.main()
