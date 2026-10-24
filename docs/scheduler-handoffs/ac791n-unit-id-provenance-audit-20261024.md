# AC791N app/loader unit-ID provenance audit — 2026-10-24

Status: primary-source investigation only. No device command, flash read, firmware edit/build, private identifier capture, SDK-byte publication or hardware uniqueness claim. Repository inspected at `b0da44f2cdbb4a19ad36b008e83f2a1613975b0c` during this audit. Parent separately coordinates actual Mac evidence.

## Established primary facts

**Puya P25Q80H:** [official V1.7 PDF](https://www.puyasemi.com/download_path/%E6%95%B0%E6%8D%AE%E6%89%8B%E5%86%8C/Flash/P25Q80H_Datasheet_V1.7.pdf), 70 pages, internal revision history dates V1.7 to 2021-01-01. Page54 §10.41 specifies a manufacturer-set, read-only 128-bit UID. Standard SPI transaction: CS low, opcode `0x4B`, **four dummy bytes**, 16 output bytes, CS high. Output is MSB first. Diagram wraps the dummy period as three bytes plus one, not three total. Page50 §10.35 assigns RDID `85 60 14` as manufacturer/type/density, not a per-unit ID. This is a supportable candidate transaction for an independently identified covered part; it establishes no fact about the actual keyboard's flash.

**Puya P25Q80SH:** [official filename V1.4 PDF](https://www.puyasemi.com/download_path/%E6%95%B0%E6%8D%AE%E6%89%8B%E5%86%8C/Flash/P25Q80SH_Datasheet_V1.4.pdf), 102 pages. Page89 §10.58 also specifies read-only factory 128-bit UID and `0x4B` followed by four dummy bytes and 16 output bytes. Page77 §10.46 also assigns RDID `85 60 14`. Thus these two documented parts share JEDEC identity. Page18 §8 requires readiness checking before a command. Treat the described standard-SPI transaction independently of this part's QPI/continuous-read modes.

The official Puya product pages and datasheets are primary manufacturer material. Third-party copies were discovery leads only, not qualification evidence. The PDFs were inspected through web text and screenshot actions; **raw downloaded bytes/SHA256 were not available**. A native fetch to Puya failed at the network proxy (CONNECT403). Do not report a verified PDF byte pin; the native receiving stage must retrieve and hash the exact revision used.

**JieLi AC79:** [official release_v1.2.0 flash API documentation](https://doc.zh-jieli.com/AC79/zh-cn/release_v1.2.0/module_example/peripherals/flash_api.html), §2.19.2, documents `norflash_ioctl(struct device*, u32 cmd, u32 arg)`, `IOCTL_GET_UNIQUE_ID`, and a 16-byte available output buffer; return0 indicates success. It describes `norflash_open` as hardware initialization. This documents a genuine UID API candidate but not its pinned implementation, supported part table, opcode/dummy-count details, all-ones fallback behavior, locking or standalone-loader availability. Do not execute its sample program: it includes erase/write testing. The same API contains destructive commands, which are outside this investigation.

Repository `ble/ctl/fetch_sdk.sh` names official SDK commit `d179b4484759423312073f5fbb232501aa491047`; `tools/build.py` names `AC79NN_SDK_V1.2.1_2023-12-13` input hashes. These are repository pins, not freshly verified SDK bytes here. Exact Gitee source/header/tree retrieval failed: native `git ls-remote` returned128, CONNECT403; web raw/tree/API endpoints were inaccessible. Official GitHub repository probe returned128, Repository not found. Consequently neither the exact `IOCTL_GET_UNIQUE_ID` implementation nor an eFuse unit-ID API was audited at those pins.

## Existing repository constraints

`fm1_flash_map.h` accepts `0x856014` and, on SMK, `0xEB6014`; its latter comment records a historical UBOOT observation, not a manufacturer/part identification. `fl_jedec_ram` sends only `0x9F`. No reviewed manufacturer primary source establishes UID semantics for the EB part. Do not send an unqualified `0x4B` to it or assume identical behavior because ordinary NOR commands work.

`fm1_flash.h` is shared by app and loader and supports RAM-only transactions with XIP disabled and interrupts off. A bounded candidate implementation could share one carefully reviewed reader, but existing `fl_tx`/`fl_rx` **discard `fl_spi_wait` errors**. A trusted UID reader must propagate transaction failures rather than return stale/plausible bytes; every exit must deassert CS and restore SFC/pin/controller state. Calls must stay entirely in RAM, including buffers/constants/helpers, while XIP is disabled. Account separately for the second core's possible XIP access. Early app initialization before core1/audio and before USB starts, and loader initialization before USB starts, are candidate integration points requiring target validation.

Do not repurpose `ldr_chip_key`: it derives a 16-bit package decryption key from isd_config, not a manufacturer-guaranteed unit identifier. Official docs use `get_chip_id()` in decryption examples and show a model-style CHIP_ID log, without establishing unique-unit semantics. No primary public eFuse API contract found in the bounded search establishes a read-safe, nonsecret unique identifier available to both this standalone app and loader. This is an unknown, not proof that the silicon lacks one.

## Stock serial and bootstrap limits

A stock CoreMIDI SerialNumber of sixteen characters has no proven relationship to flash UID. A 128-bit value normally needs32 hexadecimal characters; length alone proves neither origin nor uniqueness. Driver-cached serial metadata is not a live same-unit attestation. Current app/loader USB iSerialNumber remains zero. Even a valid future new-firmware UID protocol cannot authenticate the initial old-app transition, or prevent a wrong output from receiving a destructive legacy command.

Factory UID uniqueness is a manufacturer claim about a specified flash component. It supplies an accidental-replacement discriminator once observed correctly; it is not cryptographic proof against malicious firmware replaying another public UID. A flash replacement also changes that component identity. Keep the threat model and unit definition explicit.

## Required bounded next evidence

1. Identify actual installed flash part/die/revision through authoritative BOM/marking/manufacturer evidence and version-specific protocol coverage; JEDEC alone is insufficient. Obtain separate EB6014 documentation or keep it unsupported.
2. Read the exact pinned SDK UID header/implementation and verify error paths, length, opcode/dummy counts, locking, readiness and linkage. Do not copy vendor libraries into public Git or assume the standalone loader can call SDK APIs.
3. Review a default-disabled candidate transaction with strict status propagation, zero/FF rejection, repeated-read consistency, full-length preservation and all-failure cleanup. A plausible nonzero pattern alone is not uniqueness evidence. No writes, status-register changes, WREN, reset or OTP access are required by the cited UID transaction.
4. In a separately authorized native/hardware stage, establish stable same-unit results across app/loader/cold restart and different-unit discrimination while withholding raw UID values from public logs. Preserve namespace/version and full identity strength; avoid truncating to a display suffix. Physical reading and firmware execution were not performed here.
5. Separately prove endpoint input/output binding, disconnect invalidation and no B destructive delivery before send, plus the legacy bootstrap. Neither a UID primitive nor a discovery pairing observation establishes those transport properties.

No firmware is enabled, installable or physically qualified by this audit.
