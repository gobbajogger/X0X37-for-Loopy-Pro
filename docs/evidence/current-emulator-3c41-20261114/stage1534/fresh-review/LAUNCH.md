# Local current-package launch route

Use this only for the authorized source/design audit. Do not upload firmware, SDK/toolchain bytes or the runtime to a public service. These BUILD_ONLY packages are not install/release-qualified.

From the workspace root, with Node v24.22.1:

```sh
node stage1534/fresh-review/boot-current.cjs elite
node stage1534/fresh-review/boot-current.cjs pro
```

This explicitly loads the local current packages, not `emulator-web-poc/audio/elite.fwsc`:

- `stage1444/native-3c41/artifacts/DnBOX-0.0.26-BUILD_ONLY-3c41ef4b-elite.fwsc`, 651732 bytes, SHA-256 `62a35cb1883cc24dbe3149f37583d154ea212dbddfa5afced536b8c5eb895eb2`.
- `stage1444/native-3c41/artifacts/DnBOX-0.0.26-BUILD_ONLY-3c41ef4b-pro.fwsc`, 651730 bytes, SHA-256 `c38bff0a661e8b4aaaf6b16b153e4ff9ad34fe5e32c4daa5f043db6818ea72e0`.
- WASM `emulator-web-poc/audio/fm1_web.wasm`, SHA-256 `4fe2aa05511c32d6e230afa8f4027323df41e358d1922ae8a44f47e2b0d5fd14`.
- Worklet `emulator-web-poc/audio/worklet.js`, SHA-256 `d2e950a8e721877480389d2707886b0c376587f5dd9c8f841413f457f5d7799c`.

For a relevant bounded equivalent-state audit, use one named plan and a **new output tag** so this review's receipts remain intact:

```sh
node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/settings-held-plan.json owner-settings-audit
node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/continuity-running-plan.json owner-continuity-audit
node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/musical-qualified-plan.json owner-musical-audit
python stage1534/fresh-review/render-lcd.py 'elite-owner-musical-audit-*.png'
```

Each run creates a separate WebAssembly instance, reads and hashes the exact package, boots board 1 through the existing AudioWorklet interface, advances the emulated clock with 128-frame render quanta, captures actual RGB565 LCD memory, and releases held inputs at the end. It does not persist flash or copy firmware/runtime bytes to any service. The LCD renderer requires Pillow.

Input IDs are not browser/panel guesses: official upstream SMK code documents keybed IDs 0–36 and BTN0=37. Exact firmware `panel.c` and `smk_input.h` map physical buttons. Thus GLOBE=47, PATCH=49, PARA=50, SCALE=45, SEQ=52, PLAY=53, KNOB BANK=37; F4=17. Encoders K2/K3/K5 are 1/2/4. Upstream mapping provenance is in `../emulator-mapping/SOURCE.json`; its current upstream source does not establish byte-for-byte rebuild provenance of this older local WASM.

No browser is required. Do not retry the blocked Chromium route or weaken its protections. Keep result review and publication with the existing owners and root coordinator.
