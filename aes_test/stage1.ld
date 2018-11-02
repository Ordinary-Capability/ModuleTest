        *(.init)
        *libkernel_gcc.a:(.text .text.*)
        *libdriver_gcc.a:(.text .text.*)
        build/rt-thread/extlib/*(.text .text.*)
        build/rt-thread/platform/*(.text .text.*)
        *libvmm_rtt.a:(.text .text.*)
        */libc.a:(.text .text.*)
        */libgcc.a:(.text .text.*)
        *(.gnu.linkonce.t*)
        
        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        . = ALIGN(4);   

        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        . = ALIGN(4);

        /* section information for modules */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(RTMSymTab))
        __rtmsymtab_end = .;

        *libkernel_gcc.a:(.rodata .rodata*)
        *libdriver_gcc.a:(.rodata .rodata*)
        build/rt-thread/extlib/*(.rodata .rodata*)
        build/rt-thread/platform/*(.rodata .rodata*)
        */libc.a:(.rodata .rodata*)
        */libgcc.a:(.rodata .rodata*)
        *(.gnu.linkonce.t*)
        . = ALIGN(4);
        *libvmm_rtt.a:(.data)
        *libkernel_gcc.a:(.data .data*)
        *libdriver_gcc.a:(.data .data*)
        *(.mmutbl)
        build/rt-thread/platform/*(.data .data.*)
