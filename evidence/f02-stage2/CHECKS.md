# Stage 2 local checks (F02)

Branch: `cursor/f02-loader-bind-stage2-fb90`
Audited source SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`
No real MIDI. No device write. No firmware / controller / package byte change.

| Check | Result |
| --- | --- |
| `python3 tests/f02_loader_bind_test.py` | pass |
| `node web/test_f02_loader_bind.mjs` | pass |
| `python3 tests/install_test.py` | pass |
| `python3 tests/install_slots_test.py` | pass |
| `python3 tests/install_ctl_test.py` | pass |
| `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | pass |
| Minimal `fm1ota.js` install against FakeFM1 | pass |

GitHub Actions were not used as tests.
