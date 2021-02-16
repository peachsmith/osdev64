OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
  . = 0x1000;          /* location counter starts at 4096 */
  .text : { *(.text) } /* include .text section from all input files */
  . = 0xFA000;         /* location counter updated to 0xFA000 */
  .data : { *(.data) } /* include .data section from all input files */
  .bss : { *(.bss) }   /* include .bss section from all input files */
} 
