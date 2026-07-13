DOSSIER_UEFI_KERNEL = sortie
DOSSIER_EFI = EFI/BOOT
OBJ_DIR = obj
TARGET = $(DOSSIER_UEFI_KERNEL)/$(DOSSIER_EFI)/BOOTX64.EFI
KERNEL_BIN = $(DOSSIER_UEFI_KERNEL)/$(DOSSIER_EFI)/kernel.bin
KERNEL_ELF = $(OBJ_DIR)/kernel.elf
SOURCE_UEFI  = src/uefi/uefi.c src/font.c
SOURCE_KERNEL = src/kernel.c src/console.c  src/font.c src/timer.c src/string.c src/commandes.c \
				src/interruptions/interruptions.c  src/processus/processus.c \
				src/memoire/memoire.c src/memoire/memoire_physique.c src/memoire/memoire_virtuelle.c\
				src/io/affichage.c src/io/clavier.c src/io/io.c

# Flags pour l'uefi (Format Windows PE)
CFLAGS_UEFI     = -target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -Wall -mno-stack-arg-probe
LDFLAGS_UEFI    = -target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main -Wl,-subsystem:efi_application -fuse-ld=lld

# Flags pour le KERNEL (Format ELF brut calé à 0x100000)
CFLAGS_KERNEL = -target x86_64-unknown-none-elf -ffreestanding -fno-builtin \
                -mgeneral-regs-only -mno-red-zone -Wall \
                -mcmodel=kernel -fno-pic -fno-common -mno-stack-arg-probe \
				-fstack-protector-all -MMD -MP
				

LDFLAGS_KERNEL  = -target x86_64-unknown-none-elf -nostdlib -T linker.ld -fuse-ld=lld -O3


# MODIFIÉ : On ajoute le préfixe 'obj/' à chaque fichier .o pour qu'ils pointent vers le bon dossier
OBJECTS_KERNEL = $(addprefix $(OBJ_DIR)/, $(SOURCE_KERNEL:.c=.o)) \
                 $(OBJ_DIR)/interruptions_asm.o \
                 $(OBJ_DIR)/processus_asm.o

# La cible "all" compile désormais le bootloader ET le kernel
all: $(TARGET) $(KERNEL_BIN)

# Compilation du Bootloader uefi
$(TARGET): $(SOURCE_UEFI)
	@mkdir -p $(DOSSIER_UEFI_KERNEL)/$(DOSSIER_EFI)
	clang $(CFLAGS_UEFI) $(LDFLAGS_UEFI) $(SOURCE_UEFI) -o $(TARGET)

$(KERNEL_ELF): $(OBJECTS_KERNEL) linker.ld
	@mkdir -p $(OBJ_DIR)
	ld.lld -T linker.ld -o $@ $(OBJECTS_KERNEL)
    
# MODIFIÉ : La règle générique indique maintenant que les .o vont dans OBJ_DIR
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	clang $(CFLAGS_KERNEL) -c $< -o $@

# MODIFIÉ : Même chose pour l'assembleur
$(OBJ_DIR)/interruptions_asm.o: src/interruptions/interruptions.asm
	@mkdir -p $(dir $@)
	nasm -f elf64 $< -o $@

$(OBJ_DIR)/processus_asm.o: src/processus/processus.asm
	@mkdir -p $(dir $@)
	nasm -f elf64 $< -o $@

# Compilation du Kernel en binaire pur (.bin)
$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary \
            -j .text -j .rodata -j .data -j .bss \
            --set-section-flags .bss=alloc,load,contents \
            $< $@
    

# Mode CONSOLE
run: all
	rm -f OVMF_VARS.fd
	rm -rf sortie_qemu
	cp -r $(DOSSIER_UEFI_KERNEL) sortie_qemu
	env -u LD_LIBRARY_PATH -u LD_PRELOAD qemu-system-x86_64 \
		-bios /usr/share/ovmf/OVMF.fd \
		-nographic \
		-net none \
		-no-reboot -d int \
		-drive file=fat:rw:sortie_qemu,format=raw,media=disk

# Mode GRAPHIQUE
rung: all
	rm -f OVMF_VARS.fd
	rm -rf sortie_qemu
	cp -r $(DOSSIER_UEFI_KERNEL) sortie_qemu
	env -u LD_LIBRARY_PATH -u LD_PRELOAD qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-bios /usr/share/ovmf/OVMF.fd \
		-display sdl \
		-net none \
		-no-reboot -d int \
		-drive file=fat:rw:sortie_qemu,format=raw,media=disk

# MODIFIÉ : On nettoie aussi le dossier jetable
clean:
	rm -rf $(OBJ_DIR) $(DOSSIER_UEFI_KERNEL) sortie_qemu

.PHONY: reset

reset:
	$(MAKE) clean
	$(MAKE) rung

-include $(OBJECTS_KERNEL:.o=.d)