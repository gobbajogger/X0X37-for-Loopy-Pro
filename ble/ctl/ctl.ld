/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Linker script of the Bluetooth controller image (core 1). The addresses come from the app's
 * interface (firmware/hal/x0x_ble_abi.h): flash XB_FLASH_XIP 0x02087000, 64 KiB; RAM
 * XB_RAM_LO..XB_RAM_HI 0x01C01800..0x01C07F00. The app checks them again before it starts the
 * image. */
CTL_HEAP_SIZE      = DEFINED(CTL_HEAP_SIZE)      ? CTL_HEAP_SIZE      : 0x2980;
CTL_ISR_STACK_SIZE = DEFINED(CTL_ISR_STACK_SIZE) ? CTL_ISR_STACK_SIZE : 0x800;
CTL_RUN_STACK_SIZE = DEFINED(CTL_RUN_STACK_SIZE) ? CTL_RUN_STACK_SIZE : 0x800;

MEMORY
{
    CFLASH (rx)  : ORIGIN = 0x02087000, LENGTH = 0x10000
    CRAM   (rwx) : ORIGIN = 0x01C01800, LENGTH = 0x6700
}

ENTRY(ctl_entry_init)

SECTIONS
{
    .ctl_header : { KEEP(*(.ctl_header)) } > CFLASH

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

    ASSERT(__ctl_image_end - 0x02087000 <= 0x10000, "the image is larger than the 64 KiB Bluetooth block")
    /DISCARD/ : { *(.comment) *(.note*) }
}
