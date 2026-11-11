/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca memory map.
 * Code + constants execute in place from XIP (app.bin offset 0 = 0x02000120).
 * 0x01C00000..0x01C01800 holds .ram_text (code that must run while the flash
 * is busy: the flash driver) and .c1_text (the second core's loop).
 * 0x01C01800..0x01C07F00 is the SMK-37's Bluetooth controller image RAM (hal/x0x_ble_abi.h);
 * 0x01C07F00.. holds crt0's status words.
 * RAM  : .data/.bss (96 KiB). Core 1's mailbox (fm1_c1_mb, 64-byte aligned) opens .data at the RAM
 *        start (a zero image copied at boot like the rest of .data): in .bss its alignment left a 60-byte hole.
 * POOL : big zero-initialised buffers (FX lines, canvases), section .pool
 * C1STK: core 1's user and exception stacks (fm1_cpu1.S), below core 0's stack guard: not in
 *        RAM (.bss) any more, and an overflow runs into the unused pool tail, not into .data.
 * XPOOL: 9 KiB of the old main stack, above C1STK: the audio jobs' buffers either core writes
 *        (section .xpool, zeroed at boot). Core 1 may not write core 0's stacks (fm1_guard.h).
 * STACKS with 256-byte guard bands; NOINIT survives resets and UBOOT entry.
 * BLERAM: the Bluetooth mailbox and the app's BLE host (section .ble_ram, not cleared).
 * 0x01C7FD50.. is reserved: boot_info, boot-state mailbox, vector table. */
MEMORY
{
    XIP    (rx)  : ORIGIN = 0x02000120, LENGTH = 0x8DFBC
    RAMTEXT (rwx): ORIGIN = 0x01C00000, LENGTH = 0x1800
    RAM    (rwx) : ORIGIN = 0x01C08000, LENGTH = 96K
    POOL   (rw)  : ORIGIN = 0x01C20000, LENGTH = 0x54000
    C1STK  (rw)  : ORIGIN = 0x01C74000, LENGTH = 0x1200
    XPOOL  (rw)  : ORIGIN = 0x01C75200, LENGTH = 0x2400
    NOINIT (rw)  : ORIGIN = 0x01C7C000, LENGTH = 0x400
    BLERAM (rw)  : ORIGIN = 0x01C7C400, LENGTH = 0x1000
}
SECTIONS
{
    .text : { KEEP(*(.text.crt0)) *(.text*) *(.rodata*) . = ALIGN(4); _etext = .; } > XIP
    /* 1 KiB at the start of RAM that nothing uses: the Bluetooth start checks whether the radio writes
     * there (src/ble.c ble_sink_*). The RAM code starts after it. */
    .ram_sink (NOLOAD) : { _ram_sink = .; . += 0x400; _ram_sink_end = .; } > RAMTEXT
    .ram_text : { _rt_start = .; KEEP(*(.ram_text*)) . = ALIGN(4); } > RAMTEXT AT > XIP
    _rt_load = LOADADDR(.ram_text);
    .c1_text : { _c1_start = .; KEEP(*(.c1_text*)) . = ALIGN(4); _c1_end = .; } > RAMTEXT AT > XIP
    _c1_load = LOADADDR(.c1_text);
    _rt_end = _c1_end;                  /* the guard's RAM code window covers both */
    .data : { . = ALIGN(4); _data_start = .; KEEP(*(.bss.fm1_c1_mb)) *(.data*) *(.sdata*) . = ALIGN(4); _data_end = .; } > RAM AT > XIP
    _data_load = LOADADDR(.data);
    .c1stack (NOLOAD) : { *(.bss.c1stack) } > C1STK
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM
    .pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL
    .xpool (NOLOAD) : { _xpool_start = .; *(.xpool*) . = ALIGN(4); _xpool_end = .; } > XPOOL
    .noinit (NOLOAD) : { *(.noinit*) } > NOINIT
    .ble_ram (NOLOAD) : { . = ALIGN(64); *(.ble_ram*) } > BLERAM
    ASSERT(_c1_end <= 0x01C01800, "RAM code reaches the Bluetooth controller RAM")
    /* main user stack 10,496 B (was 24,320; 4,608 B went to C1STK and 9,216 B to XPOOL;
     * fm1-emu PERF TEST peak 4,520 B) */
    _guard0 = 0x01C77600; _ustack_lo = 0x01C77700; _ustack_top = 0x01C7A000;
    ASSERT(_c1_sstack_top <= ORIGIN(XPOOL), "core 1's stacks reach XPOOL")
    ASSERT(_xpool_end <= _guard0, "XPOOL reaches core 0's stack guard")
    _guard1 = 0x01C7A000; _sstack_lo = 0x01C7A100; _sstack_top = 0x01C7C000;
    /DISCARD/ : { *(.comment) *(.note*) }
}
