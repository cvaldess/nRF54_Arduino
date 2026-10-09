/* Common linker script for nRF54L Arduino applications */

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(Reset_Handler)

/* InternalFS (LittleFS) region, carved out just above the app FLASH and below
 * the bootloader settings page. Single source of truth for
 * InternalFileSystem.cpp and flash_nrf5x.c. The start follows from the FLASH
 * region; the size is set by each chip script as __flash_arduino_size, since
 * it depends on the gap the bootloader leaves below its settings page:
 *   nRF54L05:   0x047000-0x04E000  (28 KB)
 *   nRF54L10:   0x0C7000-0x0CE000  (28 KB)
 *   nRF54L15:   0x147000-0x14E000  (28 KB)
 *   nRF54LM20A: 0x1C9000-0x1D1000  (32 KB)
 * Plain assignments rather than PROVIDE so the symbols are in every image's
 * symbol table, whether or not InternalFS is linked. */
__flash_arduino_start = ORIGIN(FLASH) + LENGTH(FLASH);
__flash_arduino_end   = __flash_arduino_start + __flash_arduino_size;

/* __bootloader_peer_data: the handover area the bootloader reads after a
 * reset into OTA DFU -- the NOINIT region in the chip scripts. BLEDfu.cpp
 * writes 60 bytes of peer data (bond keys, or the peer address) followed by
 * their 16-bit CRC; dfu_ble_svc.c in nRF54_Bootloader places the same two
 * variables at its own ORIGIN(NOINIT), so the address is a contract between
 * the two images and not a free choice. The bootloader's double-reset word
 * sits directly above, at ORIGIN(NOINIT) + LENGTH(NOINIT). Nothing in the
 * application may be loaded or zeroed over either, which is why application
 * RAM has to end at or below ORIGIN(NOINIT) (checked below). */
__bootloader_peer_data = ORIGIN(NOINIT);

/* __softdevice_start__: base of the SoftDevice image in RRAM. It cannot be
 * derived from FLASH -- the SoftDevice sits near the top of RRAM at a
 * chip-specific address that depends on the s145 version -- so each chip
 * script sets it explicitly. nRF52 code that reads the SD info struct at
 * MBR_SIZE is wrong here: on nRF54L that address is application code.
 *   nRF54L05: 0x05A800   nRF54L10: 0x0DA800
 *   nRF54L15: 0x15A800   nRF54LM20A: 0x1DA800
 * Consumed by sd_isr.c (interrupt forwarding) and utilities.c
 * (SD_ID_GET / SD_VERSION_GET). */

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)
        *(.rodata*)
        KEEP(*(.eh_frame*))

        /* RRAM faults on unaligned word reads, keep the constructor tables aligned */
        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;
    _sidata = __etext;

    /* SoftDevice SVC data */
    .svc_data :
    {
        PROVIDE(__start_svc_data = .);
        KEEP(*(svc_data))
        PROVIDE(__stop_svc_data = .);
    } > NOINIT

    /* NOINIT is the bootloader handover area (see __bootloader_peer_data), so
     * anything placed in it would be overwritten by, or overwrite, the peer
     * data. Nothing in the tree emits svc_data today; keep it that way. */
    ASSERT(SIZEOF(.svc_data) == 0, "svc_data would land on the bootloader peer data in NOINIT")
    ASSERT(ORIGIN(RAM) + LENGTH(RAM) <= ORIGIN(NOINIT),
           "application RAM overlaps the bootloader handover area in NOINIT")
    ASSERT(LENGTH(NOINIT) >= 62, "NOINIT is too small for the 60-byte DFU peer data and its CRC")

    /* Filesystem data */
    .fs_data :
    {
        PROVIDE(__start_fs_data = .);
        KEEP(*(fs_data))
        PROVIDE(__stop_fs_data = .);
    } > RAM

    .data : AT (__etext)
    {
        __data_start__ = .;
        _sdata = .;
        *(vtable)
        *(.data*)
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);
        KEEP(*(.jcr*))
        . = ALIGN(4);
        __data_end__ = .;
        _edata = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss = .;
    } > RAM

    .noinit (NOLOAD) :
    {
        PROVIDE(__start_noinit_data = .);
        KEEP(*(.noinit))
        PROVIDE(__stop_noinit_data = .);
    } > RAM

    .heap (COPY):
    {
        __HeapBase = .;
        __end__ = .;
        PROVIDE(end = .);
        KEEP(*(.heap*))
        __HeapLimit = .;
    } > RAM

    .stack_dummy (COPY):
    {
        KEEP(*(.stack*))
    } > RAM

    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
    DataInitFlashUsed = __bss_start__ - __data_start__;
    CodeFlashUsed = __etext - ORIGIN(FLASH);
    TotalFlashUsed = CodeFlashUsed + DataInitFlashUsed;
    ASSERT(TotalFlashUsed <= LENGTH(FLASH), "region FLASH overflowed with .data and user data")
}
