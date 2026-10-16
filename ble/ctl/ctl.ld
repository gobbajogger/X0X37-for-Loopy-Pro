/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Linker script of the Bluetooth image (core 1: controller and JieLi's BLE host). The addresses
 * come from the app's interface (firmware/hal/x0x_ble_abi.h, ABI 3):
 *   piece 1  XB_FLASH_XIP  0x02087000, 64 KiB (the Bluetooth block, in every install)
 *   piece 2a XB_FLASH2_XIP 0x020A7000, 16 KiB (flash 0xAB000, plain)
 *   piece 2b 0x020AC000, 28 KiB (flash 0xB0000): around the store owner tag at 0xAF000
 *   RAM      XB_RAM_LO..XB_RAM_HI   0x01C01480..0x01C07F00
 *   RAM 2    XB_RAM2_LO..XB_RAM2_HI 0x01C7CDC0..0x01C7D400 (the top of the app's BLERAM); its last
 *            XB_NV_SIZE bytes are the bond store, which the app fills before init (not cleared)
 * The app checks them again before it starts the image. */
CTL_HEAP_SIZE      = DEFINED(CTL_HEAP_SIZE)      ? CTL_HEAP_SIZE      : 0x2780;
CTL_ISR_STACK_SIZE = DEFINED(CTL_ISR_STACK_SIZE) ? CTL_ISR_STACK_SIZE : 0x800;
CTL_RUN_STACK_SIZE = DEFINED(CTL_RUN_STACK_SIZE) ? CTL_RUN_STACK_SIZE : 0x800;

MEMORY
{
    CFLASH (rx)  : ORIGIN = 0x02087000, LENGTH = 0x10000
    CFLASH2A (rx) : ORIGIN = 0x020A7000, LENGTH = 0x4000
    CFLASH2B (rx) : ORIGIN = 0x020AC000, LENGTH = 0x7000
    CRAM   (rwx) : ORIGIN = 0x01C01480, LENGTH = 0x01C07F00 - 0x01C01480
    CRAM2  (rw)  : ORIGIN = 0x01C7CDC0, LENGTH = 0x01C7D300 - 0x01C7CDC0
    CNV    (rw)  : ORIGIN = 0x01C7D300, LENGTH = 0x100
}

ENTRY(ctl_entry_init)

SECTIONS
{
    .ctl_header : { KEEP(*(.ctl_header)) } > CFLASH

    /* piece 2: the host's code (JieLi's named sections), so piece 1 fits its 64 KiB */
    .text2a :
    {
        *(.ble_sm_code .ble_sm_code.*)
        *(.classic_rf_code .classic_rf_code.*)
        . = ALIGN(4);
    } > CFLASH2A
    .text2b :
    {
        *(.bt_stack_code .bt_stack_code.*)
        *(.classic_lmp_code .classic_lmp_code.*)
        *(.ble_att_code .ble_att_code.*)
        . = ALIGN(4);
        __ctl_p2_end = .;
    } > CFLASH2B

    .text :
    {
        *(.text .text.*)
        *(.*_code)
        *(.*_code.*)
        . = ALIGN(4);
    } > CFLASH

    .rodata :
    {
        *(.rodata .rodata.*)
        *(.*_const)
        *(.LOG_TAG_CONST)
        . = ALIGN(4);
    } > CFLASH

    /* Code that must run with the flash off (RF trim, delays). */
    .ramtext :
    {
        __ctl_ram_lo = .;
        __ctl_ramtext_start = .;
        *(.ctl_ram_text)
        *(.volatile_ram_code)
        *(*.rf.text.cache.L2.rf_trim)
        . = ALIGN(4);
        __ctl_ramtext_end = .;
    } > CRAM AT > CFLASH
    __ctl_ramtext_lma = LOADADDR(.ramtext);

    .data :
    {
        __ctl_data_start = .;
        *(.data .data.*)
        *(.*_data)
        *(.volatile_ram)
        . = ALIGN(4);
        __ctl_data_end = .;
    } > CRAM AT > CFLASH
    __ctl_data_lma = LOADADDR(.data);
    __ctl_image_end = LOADADDR(.data) + SIZEOF(.data);

    .bss (NOLOAD) :
    {
        __ctl_bss_start = .;
        *(.bss .bss.*)
        *(.*_bss)
        *(.bd_base)
        *(COMMON)
        . = ALIGN(4);
        __ctl_bss_end = .;
    } > CRAM

    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __ctl_heap_start = .;
        . += CTL_HEAP_SIZE;
        __ctl_heap_end = .;
        . = ALIGN(8);
        __ctl_run_stack_lo = .;           /* init() and poll() */
        . += CTL_RUN_STACK_SIZE;
        __ctl_run_stack_hi = .;
        __ctl_isr_stack_lo = .;           /* core 1's interrupts (its ssp) */
        . += CTL_ISR_STACK_SIZE;
        __ctl_isr_stack_hi = .;
        __ctl_ram_hi = .;
    } > CRAM

    .ram2 (NOLOAD) :
    {
        __ctl_ram2_lo = .;
        *(.ctl_ram2 .ctl_ram2.*)
        . = ALIGN(4);
        __ctl_ram2_hi = .;
    } > CRAM2
    .nv (NOLOAD) : { __ctl_nv = .; . += 0x100; } > CNV

    ASSERT(__ctl_image_end - 0x02087000 <= 0x10000, "piece 1 is larger than the 64 KiB Bluetooth block")
    /DISCARD/ : { *(.comment) *(.note*) }
}
