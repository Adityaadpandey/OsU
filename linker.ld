OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
    . = 0x00100000;

    .text : ALIGN(4096) {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4096) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4096) {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4096) {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        _bss_end = .;
    }

    _kernel_end = .;
}
