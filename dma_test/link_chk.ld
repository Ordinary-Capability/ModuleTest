
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{

	TextBase =0xA0000000;
    . = TextBase;

    . = ALIGN(4);
    .stage1_text : 
    {
    INCLUDE stage1.ld
    }

    __stage_2_size = .;

    __stage_2_start = .;
    .rodata : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) *(.eh_frame) }

    .stage2_text :
    {
    *(.text .text.*)
    }
    .ctors :
    {
        PROVIDE(__ctors_start__ = .);
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
        PROVIDE(__ctors_end__ = .);
    }

    .dtors :
    {
        PROVIDE(__dtors_start__ = .);
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        PROVIDE(__dtors_end__ = .);
    }

    __stage_2_end = .;      /* must be aligned to 4K boundary */

    . = ALIGN(4);
    .nobss : { *(.nobss) }      /* svc stack here. */

    . = ALIGN(4);
    __bss_start = .;
    .bss : { *(.bss) }
    __bss_end = .;

    /* stabs debugging sections. */
    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_aranges 0 : { *(.debug_aranges) }

    . = ALIGN(0x1000);
    _end = .;
}
