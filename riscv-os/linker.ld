/* RISC-V Linker Script for QEMU virt machine */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    . = 0x80000000;
    
    .text : {
        *(.text.boot)
        *(.text*)
    } > RAM
    
    .rodata : {
        *(.rodata*)
    } > RAM
    
    .data : {
        *(.data*)
    } > RAM
    
    .bss : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > RAM
    
    . = ALIGN(4096);
    _heap_start = .;
    . += 0x4000000;  /* 64MB heap */
    _heap_end = .;
    
    . = ALIGN(4096);
    . += 0x400000;   /* 4MB stack */
    _stack_top = .;
}