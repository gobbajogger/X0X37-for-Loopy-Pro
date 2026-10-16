# JieLi files: reference only

NOT FOR DISTRIBUTION. Do not ship, publish, push outside the box, or put these files in a release, package, installer or bundle for others. JieLi's Bluetooth controller is closed code. End users must get it from JieLi's official source and build the controller image on their own computer (USER_PREFERENCES.md).

- lib/: the exact libraries our Bluetooth controller image links (copy of flash_review/ble/ctl/lib on 2026-10-16). btstack.a and btctrler.a are the same files as in the AC79 SDK cpu/wl82/liba (md5 da773040... and f5578045...).
- sdk/include_lib/ and sdk/LICENSE: headers from the JieLi AC79 AIoT SDK, commit d179b448 "AC79NN_SDK_V1.2.1_2023-12-13", https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git.
- The full SDK (1 GB) is not in git. It is at ~/fw-AC79_AIoT_SDK on the Mac, and as a git bundle on the box at /workspace/x0x37-refs/jieli/fw-AC79_AIoT_SDK.bundle.
- The toolchain is at ~/.jieli/jieli-linux-toolchains-20260730.1 on the Mac (not in git).
- The user asked to keep these in git, 2026-10-16 17:45 (Vladivostok). Branch refs-oscuro only. Do not merge this branch into ble-a56, integration or smk37.
