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
ISO_NAME = asvarius.iso
ISO_DIR = iso

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
	rm -f $(ASM_OBJ) $(C_OBJ) $(KERNEL) ${build} $(ISO_NAME)
	rm -rf $(ISO_DIR)

rebuild: clean all

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

# Gera uma imagem ISO inicializável (GRUB) contendo o kernel
iso: $(KERNEL)
	@set -e; \
	if ! command -v grub-mkrescue >/dev/null 2>&1; then \
	  echo "Erro: 'grub-mkrescue' não encontrado. Instale os pacotes: grub-pc-bin grub-common xorriso mtools"; \
	  exit 1; \
	fi; \
	if ! command -v xorriso >/dev/null 2>&1; then \
	  echo "Erro: 'xorriso' não encontrado. Instale o pacote xorriso"; \
	  exit 1; \
	fi; \
	if ! command -v mformat >/dev/null 2>&1; then \
	  echo "Erro: 'mformat' (mtools) não encontrado. Instale o pacote mtools"; \
	  exit 1; \
	fi; \
	rm -rf $(ISO_DIR); \
	mkdir -p $(ISO_DIR)/boot/grub; \
	cp $(KERNEL) $(ISO_DIR)/boot/kernel; \
	printf "%s\n" \
	"set timeout=0" \
	"set default=0" \
	"menuentry \"ASVARIUS Kernel\" {" \
	"    multiboot /boot/kernel" \
	"    boot" \
	"}" > $(ISO_DIR)/boot/grub/grub.cfg; \
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR); \
	echo "ISO gerada: $(ISO_NAME)"; \
	ls -lh $(ISO_NAME)

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make clean   - Remove arquivos compilados"
	@echo "  make rebuild - Limpa e recompila tudo"
	@echo "  make run     - Executa com QEMU"
	@echo "  make iso     - Gera ISO bootável (GRUB) em $(ISO_NAME)"
	@echo "  make help    - Mostra esta ajuda"

# Declarar alvos que não são arquivos
.PHONY: all clean rebuild run iso info debug help
