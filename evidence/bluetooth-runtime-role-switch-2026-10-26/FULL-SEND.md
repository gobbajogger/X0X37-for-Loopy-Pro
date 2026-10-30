# Full Send — Bluetooth runtime role switching

The owner requested Full Send after commit `6424e20ca6b676bd6a3a5c7d7eec8c0f7c9e92d4`
was pushed. Production `main` is hardware proven; this isolated source lane targets
firmware development `nightly`. The original pushed branch is preserved.

## Candidate and decisions

- Base: `3e7fed606ec3b2858811c080a39a976c87d1b117` (`nightly`).
- Checked source: `38b4905752cba8c475c151ccedc93605fd766743`.
- Changed behavior: switch Bluetooth DEVICE/HOST without restarting the unit.
  Stop and drain the old role before replacing the shared host state; fail
  closed after a three-second unresolved stop.
- Ported only this task's change onto the development branch. Preserved newer
  BLE overflow/transport and input-queue behavior. Updated extracted-service
  fixture dependencies accordingly.
- Shared development target: Elite, per current `skills/x0x37-check/SKILL.md`.
  No Pro package or Pro hardware claim. No device access, flash, release,
  catalog or Pages change.

## Proof

`tools/dev_validate.py run` returned **DEV CHECKED** for the exact source above,
model `elite`, development configuration, application and matching controller.
The plan ran host-build and all scoped BLE/SMC, transition, receive, capture,
feedback and overflow regressions. Each check, app build and controller build
returned zero. The role suite uses the production TIMER5 service bodies and
real host/mailbox implementations under ASan/UBSan.

Native input recovery verified the official toolchain archive SHA-256
`a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`, compiler
SHA-256 `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`, SDK
commit `d179b4484759423312073f5fbb232501aa491047` and all three packaging/eight
controller dependency hashes.

Independent executable QA reran `dev_validate.py verify`, the role-transition
suite and controller check. Package/model/loader/CRCs and in-memory composition
with the exact controller passed. App free margins: code 229948 B, static RAM
336 B, pool 21092 B (required pool minimum 8192 B). Controller: ABI 2, CRC32
`77b7a496`, 44 B flash and 544 B RAM free.

## Sol review ledger

Reviewed with sol-loop: 1 round with gpt-6.1-sol on high. Sol verdict: approved.
No blocker/should findings. Usage was not exposed.

- S1, nit, deferred: `plat_ble_name()` can briefly display a suffix of the old
  name if TIMER5 replaces the host union during UI string use. A stable copy
  would add buffer/locking work for a transient cosmetic effect; it is outside
  the requested connection transition fix. No connection/data safety finding.

Behavior/test diff before and after review: 13 files, 374 insertions,
19 deletions. No behavior changes after approval. Markdown evidence does not
change the validation tool's source-input digest; verify the final tip before
handoff.

## Private artifacts and limits

Artifacts and raw logs are retained locally in
`/workspace/scratch/bt-role-native-38b49057-elite`. Main Library tools were not
available in this session, so no Library upload/readback is claimed. No binary,
SDK or toolchain is committed or publicly uploaded.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Elite `.fwsc` | 651455 | `19b7e32d2360aea998a948e098c86b44abd5742c26156c113f2ce00e96bf05bb` |
| `ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

The package is unversioned `SMK-37 Elite_900` with an empty controller block;
its pairing with the separately built controller was verified in memory.
`installable=false`, DEVICE NOT TESTED, PACKAGE NOT PUBLISHED. This is not a
qualified NIGHTLY download, installed repair, Pro validation or production PASS.
Enabling Bluetooth after an OFF boot still requires a restart.

## Native validation record

The following generated record contains no firmware or proprietary source:

```json
{
  "schema": 2,
  "validation_inputs": {
    "schema": 1,
    "sha256": "6ba37952b154219cb5c3de894b3f7efe318cc1f7b5c75388d5820e27cb3f79c0",
    "inputs": {
      "schema": 1,
      "mode": "dev",
      "source": "6b4d8a179b100a6a598061d398048010b28aa4cb7389bfbab37fdbb773da93bb",
      "configuration": {
        "model": "elite",
        "config": "dev",
        "controller": true,
        "plan": {
          "base": "3e7fed606ec3b2858811c080a39a976c87d1b117",
          "scope": "Bluetooth device/host runtime role switch; shared SMK-37 development validation, Elite target per current scoped check policy. No install or production promotion.",
          "checks": [
            {
              "reason": "Exercise production role services and HCI lifecycle, existing BLE/MIDI/SMC consumers, and host integration.",
              "argv": [
                "sh",
                "tests/run_tests.sh",
                "ble",
                "ble-role-switch",
                "ble-midi-receive",
                "ble-capture-replay",
                "ble-out-noteoff-overflow",
                "ble-out-rt-overflow",
                "smk-clip-ble-clear",
                "smk-clip-ble-notify",
                "ble-central-diag",
                "smc-link",
                "smc-queue",
                "smc-protocol",
                "host-build"
              ]
            }
          ]
        },
        "dependencies": []
      },
      "environment": {
        "CTL_NO_CACHE": "1",
        "CTL_NATIVE": "1",
        "NODE_PATH": "/opt/codex/runtimes/codex-primary-runtime/dependencies/node/node_modules",
        "MODEL": "elite",
        "BOARD": "smk37",
        "X0X_DEV_PADCAL": "1"
      },
      "platform": [
        "Linux",
        "x86_64",
        "6.18.55"
      ],
      "validator": {
        "validation_inputs.py": {
          "sha256": "88771086fc5daeae12b9fe56617eed458f83c3e5a12d3f3825361d5ddc493b9d",
          "executable": false
        },
        "dev_validate.py": {
          "sha256": "39c5956f49de3ca9aa6aa3edab43aa9a481264136ed0dbb029ae377de3c965a6",
          "executable": false
        }
      },
      "tools": {
        "bash": {
          "sha256": "223fe8564b60636bc738b6178d3ba9a50ef7d791266b0efae6363bb716e4c47f",
          "executable": true
        },
        "sh": {
          "sha256": "a6f559e00b69a4aa4d8cb607be18d9386c5aee55c509e2c075549dcf00e00fc7",
          "executable": true
        },
        "git": {
          "sha256": "9dae295c1a4ffae4ee772e6205fcf22b26e1bfeeb5ee98f278b5139bda7ca22f",
          "executable": true
        },
        "make": {
          "sha256": "8a229baf64671988cfb5af43ba3070c6ce7214cf32bcf96155115559d1466fa7",
          "executable": true
        },
        "node": {
          "sha256": "bc17c508ffeed0ec622934f9b7fa72f8e78da65350e63c3eceb56fa688aa5e12",
          "executable": true
        },
        "cc": {
          "sha256": "a23ecab8ff08f09ad8c80602c2c5df7f49e09c25905cb8975902e101bf72635f",
          "executable": true
        },
        "c++": {
          "sha256": "6b3696e4dcb85e1c949c732a02befa50e3983ecf94ce7e8e58d9d503b954b79d",
          "executable": true
        },
        "ld": {
          "sha256": "d2766988b6a1b1d1518cdc09242e726d1f24e151e16410ada3092b11ffd9081b",
          "executable": true
        },
        "ar": {
          "sha256": "f8fe09d485b40b1d47c534c96f5cbaaa9148e247aa0494c087104421f48ac5eb",
          "executable": true
        },
        "objcopy": {
          "sha256": "403f90bbd5d26c8ab9a2ad51f56255b812e8858a83aa8740ee71791071ddb580",
          "executable": true
        },
        "python": {
          "sha256": "fa67443527ed9647f760d807e2a38f26340757123e643c4639cf273ed15d5ea7",
          "executable": true
        }
      },
      "python_packages": {
        "numpy": {
          "version": "2.3.5",
          "sha256": "33899bddedcc34ed6dff45f16493b38af2d4a2dd7f768d209a3bd6b31d4e855a"
        },
        "pillow": {
          "version": "12.3.0",
          "sha256": "2e0581398ad83af176995b57b45112bd7fe22579e326e58e7c8420eb009cd220"
        }
      },
      "dependencies": {
        "toolchain": {
          "sha256": "50a35938b8c15394e2a5d6bbd9284dcb3a9ccfc14796a15f820ff5f8b5308ba5"
        },
        "sdk": {
          "sha256": "0652c371941e828920c1db1c8a6f6f1cd95df3411b0e4418f989b107f7dc0e2d"
        }
      },
      "references": {}
    }
  },
  "result": "DEV CHECKED",
  "config": "dev",
  "model": "elite",
  "commit": "38b4905752cba8c475c151ccedc93605fd766743",
  "tree": "6b4d8a179b100a6a598061d398048010b28aa4cb7389bfbab37fdbb773da93bb",
  "plan": {
    "base": "3e7fed606ec3b2858811c080a39a976c87d1b117",
    "scope": "Bluetooth device/host runtime role switch; shared SMK-37 development validation, Elite target per current scoped check policy. No install or production promotion.",
    "checks": [
      {
        "reason": "Exercise production role services and HCI lifecycle, existing BLE/MIDI/SMC consumers, and host integration.",
        "argv": [
          "sh",
          "tests/run_tests.sh",
          "ble",
          "ble-role-switch",
          "ble-midi-receive",
          "ble-capture-replay",
          "ble-out-noteoff-overflow",
          "ble-out-rt-overflow",
          "smk-clip-ble-clear",
          "smk-clip-ble-notify",
          "ble-central-diag",
          "smc-link",
          "smc-queue",
          "smc-protocol",
          "host-build"
        ]
      }
    ]
  },
  "changed_files": [
    "ble/README.md",
    "evidence/bluetooth-runtime-role-switch-2026-10-26/CHECKS.md",
    "firmware/src/app/ui.c",
    "firmware/src/ble.c",
    "firmware/src/ble/ble_role.h",
    "tests/host/ble_midi_deadline_test.c",
    "tests/host/ble_midi_stop_test.c",
    "tests/host/ble_out_noteoff_overflow_test.py",
    "tests/host/ble_out_rt_overflow_test.py",
    "tests/host/ble_role_switch_test.c",
    "tests/host/ble_role_switch_test.py",
    "tests/host/ble_role_switch_test.sh",
    "tests/run_tests.sh"
  ],
  "controller": true,
  "budgets": {
    "code_free": 229948,
    "ram_free": 336,
    "pool_free": 21092
  },
  "firmware_version": null,
  "firmware": {
    "firmware_version": null,
    "release_display": null,
    "version_label": "Unversioned legacy DEV",
    "build_id": "g38b4905752cb-h19b7e32d2360",
    "display_label": "X0X37 Unversioned legacy DEV \u00b7 SMK-37 Elite \u00b7 DEV \u00b7 DEVICE NOT TESTED",
    "model": "elite",
    "product": "SMK-37 Elite_900",
    "source_commit": "38b4905752cba8c475c151ccedc93605fd766743",
    "package_sha256": "19b7e32d2360aea998a948e098c86b44abd5742c26156c113f2ce00e96bf05bb",
    "package_bytes": 651455,
    "status": "package-inspected",
    "device_tested": false,
    "installable": false
  },
  "logs": {
    "check-0": {
      "argv": [
        "sh",
        "tests/run_tests.sh",
        "ble",
        "ble-role-switch",
        "ble-midi-receive",
        "ble-capture-replay",
        "ble-out-noteoff-overflow",
        "ble-out-rt-overflow",
        "smk-clip-ble-clear",
        "smk-clip-ble-notify",
        "ble-central-diag",
        "smc-link",
        "smc-queue",
        "smc-protocol",
        "host-build"
      ],
      "returncode": 0,
      "sha256": "a207b6faf2d5829d6ea52f1bf5aa3b434a96af7fc8a7f5296236806db761b295"
    },
    "build": {
      "argv": [
        "sh",
        "build.sh",
        "--board",
        "smk37"
      ],
      "returncode": 0,
      "sha256": "8841b68ba2afdbf1776ed1b549a18735226bd5401c3cc749fc4cfe68bb229e22"
    },
    "controller": {
      "argv": [
        "sh",
        "ble/ctl/build_ctl.sh"
      ],
      "returncode": 0,
      "sha256": "a48e6b6b1a217e8dd691310599c2b13cd3b82f9c6516331c2862c37eb911f540"
    }
  },
  "artifacts": {
    "package": {
      "file": "x0x-smk37-elite.fwsc",
      "sha256": "19b7e32d2360aea998a948e098c86b44abd5742c26156c113f2ce00e96bf05bb"
    },
    "controller": {
      "file": "ctl.img",
      "sha256": "956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a"
    }
  }
}
```
