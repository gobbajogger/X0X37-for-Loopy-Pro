# Native BREAK storage on the two stock models

The five owner zones have 109,274 original IMA ADPCM bytes. One 20-byte bank header and five 20-byte zone headers give 109,394 bytes. The bank SHA-256 is `8df1316157fc8ce8dcb87cfd267fea83790702ccd1963ae91c0de60ca255b582`. All five zones, and their independent decoded-PCM/preview hashes, passed the original asset verifier.

## Placement enforced by the running firmware

The SMK app has 552,672 bytes before the Bluetooth controller. `firmware/src/app/engine.c:3063` passes that limit to the native reader. `firmware/src/app/brk_pack.h:51` validates contiguous headers, full zone lengths, the native sample rate, the CRC and erased bytes after the pack. The reader searches on 16-byte boundaries. `tools/break_pack.py:114` uses this same native placement when composing an application. To retain the 1,024-byte XIP reserve, the five-zone bank must start at or below app offset 442,240. The installed app and current model-specific shortfalls are measured in pack-fit.json. Embedding only the raw arrays in the linked application would save at most the 120-byte pack header overhead before accounting for its own startup and descriptors.

## Stock flash layout

| Flash range | Bytes | Existing contents or limitation |
|---|---:|---|
| 0x00000–0x03FFF | 16,384 | Boot head, protected from stock app update |
| 0x04000–0x0411F | 288 | Application directory |
| 0x04120–0x8AFFF | 552,672 | Application code and the native bank share this region |
| 0x8B000–0x9AFFF | 65,536 | One fixed Bluetooth controller block |
| 0x9B000–0x9CFFF | 8,192 | End of app slot, SDK config and slot map |
| 0x9D000–0xAFFFF | 77,824 | Seven A/B storage objects, two boot screen copies and owner tag. Two 4 KiB sectors are spare. |
| 0xB0000–0xBAFFF | 45,056 | Three 4 KiB sample shadow sectors and a 32 KiB user sample slot |
| 0xBB000–0xBFFFF | 20,480 | Update loader staging and records |
| 0xC0000–0xC1FFF | 8,192 | Bluetooth pairing and calibration |
| 0xC2000–0xF3FFF | 204,800 | Two user sample slots of 80 KiB and 120 KiB |
| 0xF4000–0xFDFFF | 40,960 | Protected stock resources for rollback |
| 0xFE000–0xFEFFF | 4,096 | Unused sector outside the stock update range |
| 0xFF000–0xFFFFF | 4,096 | Protected key_mac sector |

`firmware/hal/fm1_flash_map.h:24` defines the layout. `firmware/src/storage.c:42` identifies the two spare sectors. `firmware/src/app/slots.c:23` locates all three user slots and their sector-zero shadows. `firmware/loader/ldr_core.c:109` copies package pages only from 0x04000 up to 0x9D000. Its later scan clears recognized update records. Adding extra bytes to the package does not populate flash beyond that copy loop.

Even counting **every byte** of the 8 KiB post-controller config range, 8 KiB of VM spare and the 4 KiB off-map sector adds only 20,480 bytes to the measured application tail. That deliberately overstates capacity because the stock bank reader cannot address these regions and part of the config range is in use. The smallest tested lossless compression was 97,776 bytes before decoder code. The 44 KiB VM range marked “spare” in an historical name is fully occupied by the sample shadows and slot 3.

The shipping contiguous-bank and fixed-map routes therefore have a measured capacity deficit. This bound applies to the existing layout and compiled app sizes. It does not establish a lower bound for all possible future implementations of the firmware.

The existing exporter can transfer all five complete zones into user slots 1 and 2 by an explicit sample-restore operation. The dated integration ledger records 31 selection and restore cases. Treating that model as the default changes ownership of user-storage capacity and introduces the cases of populated or partially populated slots. The installer behavior, preservation of existing user recordings, available capacity, and power-loss recovery would need a product decision and separate tests. No slot operation, partition change or device operation was performed.
