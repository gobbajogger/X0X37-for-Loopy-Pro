#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reviewed diagnostic overlay; admission and acquisition remain unchanged."""
import sys
import acquire


def describe(reply):
    # Decode only the exact framing/layouts understood by the unchanged gate.
    if not reply or any(type(x) is not int or not 0 <= x < 128 for x in reply):
        return 'DIAG: malformed reply (no state inferred)'
    n = reply[0]
    if len(reply) != 1 + 5*n or any(reply[5+5*i] >= 16 for i in range(n)):
        return 'DIAG: invalid framing (no state inferred)'
    w = [sum(reply[1+5*i+j] << (7*j) for j in range(5)) for i in range(n)]
    if n == 63 and w[-3] == 0x534D4347:
        f = w[-2]
        return (f'DIAG TEST: allowed={f&1} attempted={(f>>1)&1} '
                f'requested_on={(f>>2)&1} boot_on={(f>>3)&1} '
                f'flags=0x{f:08x} reset_raw=0x{w[-1]:08x}')
    if n == 71 or (n == 77 and w[-6] == 0x534D4301):
        f, q = w[48], w[49]
        return (f'DIAG ABI2 words={n}: image={f&255} boot_on={(f>>8)&255} '
                f'up={(f>>16)&255} dead={(f>>24)&255} want={q&255} '
                f'host_on={(q>>8)&255} state={w[51]} (OFF=1) '
                f'reset_raw=0x{w[58]:08x}')
    return f'DIAG: unsupported layout words={n} (no state inferred)'


def check_cold(link, fi):
    reply = list(fi.xs.req(fi.EditorLink(link), 33, [], fi.CTL_READ['timeout'], fi.CTL_READ['tries']))
    print(describe(reply), flush=True)
    return acquire.install.cold_state(reply)  # Sole original admission decision.


def main():
    original = acquire.install.check_cold
    acquire.install.check_cold = check_cold
    try:
        return acquire.main()
    except acquire.bundle.BundleError as exc:
        print(f'Head acquisition refused: {exc}', file=sys.stderr)
        print('If the cold check failed, nothing was staged. Save BLUETOOTH OFF in the keyboard, '
              'disconnect all power, reconnect, and leave Bluetooth OFF. '
              'If already done, send the DIAG line above; do not bypass the check.', file=sys.stderr)
        return 2
    finally:
        acquire.install.check_cold = original


if __name__ == '__main__':
    sys.exit(main())
