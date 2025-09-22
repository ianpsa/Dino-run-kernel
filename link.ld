/*
* link.ld
*/

OUTPUT_FORMAT(elf32-i386) /* seta como 32 bits */
ENTRY(start) /* Entry point do executável */
SECTIONS /* definição do layout do executável */
{
    . = 0x1000000;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}