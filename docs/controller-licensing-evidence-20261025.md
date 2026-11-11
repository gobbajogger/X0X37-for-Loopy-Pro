# Bundled NIGHTLY controller licensing evidence — 2026-10-25

Scope: read-only examination of the existing controller, exact SDK inputs, build/link scripts and primary license sources. No vendor binary or source is included in this report or Git. This is an evidence assessment, not a legal clearance or finding of prohibition.

## Verified inputs

The clean local checkout `verified owner-local input` records official upstream `https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git`, commit `d179b4484759423312073f5fbb232501aa491047`, root tree `52a35efdc37762cba6c45d12bde4137a11458d05`, commit subject `AC79NN_SDK_V1.2.1_2023-12-13`. Every inspected file matches `git show HEAD:path` bytes. `sdk-pins.json` records full lengths and SHA256 for all six linked archives, RF source and license.

The exact SDK `LICENSE` is 11,357 bytes, SHA256 `c71d239df91726fc519c6eb72d318ec65820627232b2f796219e87dcf35d0ab4`; its text is Apache License 2.0. The pinned README does not have an explicit licensing section. The later official [README at e30b1ee375d1f2993fc23bf92c8b99006a6e5f9d](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/e30b1ee375d1f2993fc23bf92c8b99006a6e5f9d/README-en.md) states the SDK is open sourced under Apache 2.0. That supports project-level intent, not a complete third-party archive inventory.

Local exact historical controller: `verified owner-local input`, 65,492 bytes, SHA256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; current `add_ctl.ctl_problem` returns None. Thirty-four local copies have that length/hash. A matching copy and link map coexist at `verified owner-local input`.

Current public Elite package exists locally at `verified owner-local input`: 651,396 bytes, SHA256 `d14877d6592e6dc158460e960ac5aefd1b2b775588b9f896d3c83f672d61c86c`, product `SMK-37 Elite_900`. Decryption with current package tooling finds exactly 65,536 erased bytes in the controller block; controller_of returns None. This matches the published-catalog fixture. No local file matching current Pro package hash `cde289d422b61ee8b1f0a9f04ca92d244f07856667c85d757b9563e0479d3f3c` was found in the bounded search; no fresh HTTP claim is made.

## Component assessment

`build_in_container.sh` explicitly links `btctrler.a`, `wl_rf_common.a`, `crypto_toolbox_Osize.a`, `lib_ccm_aes.a`, `libc.a`, `libcompiler_rt.a` using LTO and `--gc-sections`. Archive input alone does not establish which routines survive in the final image.

| Component | Evidence | Remaining condition or gap |
| --- | --- | --- |
| btctrler.a | Exact pinned upstream archive, 5,157,416 bytes, SHA in manifest; root Apache license. | No adjacent archive-specific license/notice or readable embedded grant found. SDK-level grant scope and complete third-party inventory need confirmation. No independent prohibition found. |
| wl_rf_common.a | Exact pinned archive, 321,802 bytes; root Apache license. | Same scope/notice gap. |
| crypto_toolbox_Osize.a | Exact pinned archive, 421,662 bytes; related SDK headers explicitly show mixed origins. | micro-ecc headers state Kenneth MacKay BSD 2-clause; rijndael header says Public Domain; crypto/sha256/hmac/endian headers carry Oryx Embedded GPL-2.0-or-later notices. This does not prove archive code versions or retained final code. Must map included members to exact source/notices and satisfy any corresponding-source terms. |
| lib_ccm_aes.a | Exact pinned archive, 33,128 bytes. Debug metadata names tomcrypt.h and aes/ccm sources; root Apache license. | A filename is not a license grant. No exact component notice/source mapping established. |
| libc.a | Exact pinned archive, 188,988 bytes; metadata points to newlib libc. SDK newlib.h declares 2.2.0. | SDK headers retain several different notices. Official newlib licensing says individual files can have their own terms. Need notices for actual linked routines plus any JieLi modifications; cannot infer blanket Apache or blanket incompatibility. |
| libcompiler_rt.a | Exact pinned archive, 111,588 bytes. | No version-specific copyright/license notice or authenticated source mapping found locally. Name alone does not establish LLVM provenance/version or runtime exception terms. |
| X0X shim and ABI | Explicit MPL-2.0 SPDX/copyright headers in ctl source, ABI header and build scripts. | For executable distribution make covered source available and tell recipients how to obtain it, retain notices and comply with MPL 3.1–3.4. The linked other files keep their own terms. |
| Generated RF tables | Generator validates wifi_conf.c SHA `697c798b6a9c992c75ee3959aa58d121d4eb3015474250ad8f9156a9a22fe6bb`, extracts its disabled WIFI RX branch and marks generated file Apache-2.0 with origin. | Original pinned wifi_conf.c lacks a per-file license header. Root Apache evidence supports its treatment; retain source provenance, applicable notices and changed/generated notice. Need confirm no further origin notices apply. |

No component-specific notice was found next to the six archives or as a readable copyright/license string inside them. This is limited negative evidence, not proof that a grant or notice does not exist. The toolchain's local README provides usage instructions and no runtime notice inventory was found there. BlueKitchen BTstack is not linked by this build; a header's historical BTstack configuration comment does not establish use of BlueKitchen's stack.

The coexistence link map lists crypto archive members uECC_new, bigint, Crypto_hash, hmac, sha256, aes_cmac, endian, rijndael; AES members aes/ccm_memory; libc members memcmp/memcpy/memset/strcpy/strlen; compiler runtime members divdf3/fixdfsi/floatsidf/floatunsidf/muldf3/subdf3/adddf3. These are useful source-mapping leads, not proof that every member contributes live final-image bytes after LTO and section garbage collection.

## Applicable distribution conditions and decision

Where the Apache grant applies, section 4 requires the license copy, changed-file notices, retention of applicable source notices, and inclusion of relevant NOTICE content if supplied. Sections 2–3 grant covered copyright and patent rights subject to the terms; section 6 does not generally grant trademark rights. The absence of a top-level NOTICE does not remove component-specific notices.

MPL executable distribution requires availability of covered source and notice to recipients under sections 3.1–3.4. X0X's GPL app still requires corresponding-source and license compliance for its distribution. Newlib is a collection with individual file terms; preserve applicable binary notices and disclaimers once exact routine origins are mapped. No exact compiler-runtime license/exception has been established for these bytes, so none is asserted.

Do not describe all six archives as proprietary, closed or not GPL compatible merely because they are archives. Equally, the SDK root Apache text is insufficient here to attest every archive/linked component as freely redistributable. No verified vendor prohibition has been established. Current project no-shipping language is a conservative project policy, not independently verified vendor terms.

Public controller distribution remains undecided. Before publication obtain an authenticated component inventory/grant or exact source mapping for these pinned archives (including newlib/compiler runtime and crypto origins), assemble the required notice/source package, then have the existing independent rights/release owner review the exact final artifact. Private transfer to another recipient also triggers any applicable distribution conditions; privacy alone is not an exemption. Existing owner-local file checks/composition can proceed without claiming public clearance.

## Primary sources checked

- Local exact Git-pinned SDK license and files above; official Gitee pinned tree URLs were not accessible through the web reader during this run.
- [Official later JieLi README](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/e30b1ee375d1f2993fc23bf92c8b99006a6e5f9d/README-en.md).
- [Official newlib licensing inventory](https://sourceware.org/newlib/COPYING.NEWLIB), confirms per-file origins and source/binary notice conditions. This current inventory is context, not an exact mapping to the pinned 2.2.0 libc archive.
- [Mozilla MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/), sections 3.1–3.4.
- [LLVM 3.7 release license](https://releases.llvm.org/3.7.0/LICENSE.TXT), demonstrates version-specific notice obligations but is not mapped to this archive and is not asserted as its license.
