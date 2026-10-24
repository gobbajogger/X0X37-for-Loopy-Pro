#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only libusb observation for the OTA transport investigation.

No interface claim, driver detach, configuration/reset, serial-string read,
control/bulk transfer, MIDI message, firmware byte or automatic device selection.
BUS:ADDRESS identifies only an object in this enumeration, never a physical unit
across reconnect. This is deliberately NOT an updater backend.
"""
import argparse
from contextlib import contextmanager
import ctypes as C
import ctypes.util
import json
import sys


class USBError(RuntimeError):
    pass


class Descriptor(C.Structure):
    _fields_ = [(name, typ) for name, typ in (
        ('bLength', C.c_uint8), ('bDescriptorType', C.c_uint8),
        ('bcdUSB', C.c_uint16), ('bDeviceClass', C.c_uint8),
        ('bDeviceSubClass', C.c_uint8), ('bDeviceProtocol', C.c_uint8),
        ('bMaxPacketSize0', C.c_uint8), ('idVendor', C.c_uint16),
        ('idProduct', C.c_uint16), ('bcdDevice', C.c_uint16),
        ('iManufacturer', C.c_uint8), ('iProduct', C.c_uint8),
        ('iSerialNumber', C.c_uint8), ('bNumConfigurations', C.c_uint8))]


class NativeUSB:
    """Only libusb's enumeration/open/close API is exposed here."""
    def __init__(self):
        if sys.platform != 'linux':
            raise USBError('this observation tool supports Linux only; other native ABIs are unvalidated')
        path = ctypes.util.find_library('usb-1.0')
        if not path:
            raise USBError('libusb-1.0 is unavailable')
        self.lib = C.CDLL(path)
        ptr = C.c_void_p
        signatures = {
            'init': (C.c_int, [C.POINTER(ptr)]),
            'exit': (None, [ptr]),
            'get_device_list': (C.c_ssize_t, [ptr, C.POINTER(C.POINTER(ptr))]),
            'free_device_list': (None, [C.POINTER(ptr), C.c_int]),
            'get_bus_number': (C.c_uint8, [ptr]),
            'get_device_address': (C.c_uint8, [ptr]),
            'get_port_numbers': (C.c_int, [ptr, C.POINTER(C.c_uint8), C.c_int]),
            'get_device_descriptor': (C.c_int, [ptr, C.POINTER(Descriptor)]),
            'open': (C.c_int, [ptr, C.POINTER(ptr)]),
            'get_device': (ptr, [ptr]),
            'close': (None, [ptr]),
        }
        for name, (result, args) in signatures.items():
            try:
                fn = getattr(self.lib, 'libusb_' + name)
            except AttributeError as exc:
                raise USBError('required libusb symbol is unavailable: ' + name) from exc
            fn.restype, fn.argtypes = result, args
            setattr(self, '_' + name, fn)

    @contextmanager
    def devices(self):
        ctx, listing = C.c_void_p(), C.POINTER(C.c_void_p)()
        rc = self._init(C.byref(ctx))
        if rc:
            raise USBError(f'libusb_init: {rc}')
        try:
            count = self._get_device_list(ctx, C.byref(listing))
            if count < 0:
                raise USBError(f'libusb_get_device_list: {count}')
            try:
                # The list retains device references until all optional open
                # handles have been checked and closed below.
                yield [listing[i] for i in range(count)]
            finally:
                self._free_device_list(listing, 1)
        finally:
            self._exit(ctx)

    def describe(self, dev):
        desc, ports = Descriptor(), (C.c_uint8 * 8)()
        rc = self._get_device_descriptor(dev, C.byref(desc))
        if rc:
            raise USBError(f'libusb_get_device_descriptor: {rc}')
        n = self._get_port_numbers(dev, ports, len(ports))
        return {
            'locator': f'{self._get_bus_number(dev)}:{self._get_device_address(dev)}',
            'port_path': list(ports[:n]) if n > 0 else None,
            'vid': f'{desc.idVendor:04x}', 'pid': f'{desc.idProduct:04x}',
            'bcd_device': f'{desc.bcdDevice:04x}',
            'serial_descriptor_present': desc.iSerialNumber != 0,
        }

    def check_open_object(self, dev):
        # This pointer association is a library-level observation. It does
        # not authenticate kernel open races, endpoint ownership, or a unit.
        handle = C.c_void_p()
        rc = self._open(dev, C.byref(handle))
        if rc:
            raise USBError(f'libusb_open: {rc}')
        try:
            if self._get_device(handle) != dev:
                raise USBError('opened handle does not refer to selected enumeration object')
            return True
        finally:
            self._close(handle)


def observe(api, selected=None, phase='unspecified'):
    """Record descriptors; optionally check ONE explicitly selected open object."""
    report = {
        'schema': 'x0x37-ota-usb-observation-v1', 'phase_label': phase,
        'status': 'OBSERVATION ONLY / NOT PHYSICAL CONTINUITY PROOF',
        'destructive_messages': 0, 'serial_values_read': False,
        'cross_reboot_identity': 'UNESTABLISHED', 'devices': [],
    }
    with api.devices() as devices:
        rows = [(dev, api.describe(dev)) for dev in devices]
        matches = [dev for dev, row in rows if row['locator'] == selected]
        if selected is not None:
            if len(matches) != 1:
                raise USBError('explicit locator is absent or ambiguous in this enumeration')
            api.check_open_object(matches[0])
        for dev, row in rows:
            row['selected_open_object_checked'] = selected is not None and dev == matches[0]
            report['devices'].append(row)
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', metavar='BUS:ADDRESS',
                        help='optionally open this exact enumerated object; not unit identity')
    parser.add_argument('--phase', choices=('application', 'loader', 'unspecified'),
                        default='unspecified', help='operator annotation only')
    args = parser.parse_args(argv)
    if args.device is not None:
        try:
            bus, address = (int(n) for n in args.device.split(':'))
            if not (1 <= bus <= 255 and 1 <= address <= 127):
                raise ValueError
            args.device = f'{bus}:{address}'
        except ValueError:
            parser.error('--device must be BUS:ADDRESS (bus 1..255, address 1..127)')
    try:
        report = observe(NativeUSB(), args.device, args.phase)
    except (USBError, OSError) as exc:
        print(json.dumps({'status': 'OBSERVATION UNAVAILABLE', 'error': str(exc)}))
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
