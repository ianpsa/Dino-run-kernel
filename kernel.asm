;;kernel.asm
bits 32             ;nasm directive - 32 bits
section .text
        ;especificação de multiboot
        align 4
        dd 0x1BADB002
        dd 0x00
        dd - (0x1BADB002 + 0x00)

global start
extern kmain        ;kmain será definido no arquivo em c

start:
    cli     ;bloqueia interrupções
    mov esp, stack_space    ;define o ponteiro do stack
    call kmain
    hlt     ;halt na cpu

section .bss
resb 8192       ;8kb para a stack
stack_space: