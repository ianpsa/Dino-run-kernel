ASM = nasm
CC = gcc
LD = ld

ASMFLAGS = -f elf32
CFLAGS = -m32 -c -ffreestanding -fno-stack-protector -fno-pic -nostdlib
LDFLAGS = -m elf_i386 -T link.ld

# Arquivos
KERNEL = kernel
ASM_SRC = kernel.asm
C_SRC = kernel.c
ASM_OBJ = kasm.o
C_OBJ = kc.o
build = build

all: $(KERNEL)

# Linkagem do kernel
$(KERNEL): $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Compilação do assembly
$(ASM_OBJ): $(ASM_SRC)
	$(ASM) $(ASMFLAGS) $< -o $@

# Compilação do C
$(C_OBJ): $(C_SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(ASM_OBJ) $(C_OBJ) $(KERNEL) ${build}

rebuild: clean all

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make clean   - Remove arquivos compilados"
	@echo "  make rebuild - Limpa e recompila tudo"
	@echo "  make run     - Executa com QEMU"
	@echo "  make help    - Mostra esta ajuda"

# Declarar alvos que não são arquivos
.PHONY: all clean rebuild run iso info debug help
