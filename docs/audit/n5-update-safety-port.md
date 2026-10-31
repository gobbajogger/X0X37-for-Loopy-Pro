# N5 update safety port

Base: `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
Source lane: `lane/opt-preserve-909-init-20261021`.

Both source patches were compared before editing. Neither complete patch is
already present: reverse-apply checks fail against this base. Stable patch IDs
are `6c7dece4a3abb3bf773054157c0dc9b74edae92d` for `35e3c046` and
`f453e2b5b84953f5ff8c4bc6e1703997a23c825f` for `1006085b`.

| Source | Ported | Already present or deliberately omitted |
| --- | --- | --- |
| `35e3c046aecae74c7ee457b6488bfb844d492f8e` | Original `x0x_bundle_gate.h`; OTA session/stage deny gates, bundle-loader capability check, occupied-record refusal and cleanup preservation; real OTA regression adapted and extended. | Base already has `OTA_MAINTENANCE_ADMIT`, returning -20 before staging and cleanup. Keep it. The source's `ble_vendor.c` backend and relocation exporter are absent here. Do not transplant them or add startup hooks to `main_fm1.c`/`x0x.c`. Without a TEST admission hook, the original default-deny behavior applies. Add `BOARD_SMK37` to OTA conditions to restrict this port to SMK. |
| `1006085bfea3095e9aed9ed80a8b13a385b8e0f1` | Original TEST-profile store/loader allowlist exclusions for `[0xAB000,0xAF000)` and storage owner-migration end at `0xAB000`. Real allowlist and migration tests. | Omit UI-data descriptor/admission, recovery UI changes, segmented BLE ABI/layout and relocation study helper: they enable a separate experimental profile and are unnecessary for these write exclusions. Omit its transaction model; test actual OTA/store code instead. |

Permanent head, BTIF, USRTRIM, stock-resource and key_mac exclusions already
exist in the base's explicit allowlists and independent `FL_PROTECTED` guard.
They remain unchanged. No bootloader, efuse, device data or hardware operation is
changed or performed. The shared file names retain their historical `fm1`
prefix; only SMK branches change. Package/model identity remains unchanged.

`X0X_SMC_VENDOR_TEST` remains off in normal Elite/Pro builds. The extra
`0xAB000..0xAEFFF` exclusion is conditional exactly as in the source: normal
builds retain the existing boot-screen store. This port does not relocate fonts,
enable a TEST BLE backend, implement a new update protocol, or certify the
experimental profile as installable. It adds no normal-build static state.

Registered tests:

- `smk-update-protection`: every permanent protected sector, page/erase requests,
  immutable TEST sectors, straddling/wrapping ranges, valid neighboring ranges,
  erased/foreign owner preservation and real settings save/load.
- `smk-vendor-update-gate`: real `ota.c`, Pro/Elite package identities, explicit
  recovery gate and missing-hook default denial, controller-attempt revocation,
  stage/session denial before requests, unchanged flash during cleanup,
  missing bundle capability and occupied recovery record refusal.
- Existing `smk-flash` remains unchanged and covers the normal-build layout.

The new strict test compiles suppress existing `-Wtype-limits` in
`FL_PROTECTED` and `-Wsign-compare` in the included baseline OTA fixture only.
No existing check, assertion or compiler flag is removed or weakened.
