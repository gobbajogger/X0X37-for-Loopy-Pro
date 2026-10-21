/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact logical baseline 816e0d3. Reproduce/check with rollback_fixture.py.
 * Whole main canonical CRC decrypts ONLY [4000,4000+9796e), retains plaintext
 * trailing slot-map bytes. Thus independent of device SFC key.
 * App SHA256 4b7c76e4...d0ec9; ctl SHA256 9d6855e4...a8245.
 * No original spare-byte restoration: policy2 explicitly clears unused spans. */
#define BP_RB_COMMIT 0x8B000u
#define BP_RB_IMAGE_ID 0x0816E0D3u
#define BP_RB_CRYPT_SIZE 0x9796Eu
#define BP_RB_MAIN_CRC 0x53F84577u
#define BP_FF_8K_CRC 0xB4293435u
#define BP_FF_4K_CRC 0xF154670Au
