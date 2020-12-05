
CC=/home/john/opt/cross-compiler-wrz/bin/x86_64-elf-gcc
AS=/home/john/opt/cross-compiler-wrz/bin/x86_64-elf-as
LD=/home/john/opt/cross-compiler-wrz/bin/x86_64-elf-ld
OBJCOPY=objcopy

GNUEFI_DIR=/home/john/development/gnu-efi

OVMF_DIR=/home/john/development/edk2/Build/OvmfX64/DEBUG_GCC5/FV

CFLAGS=-fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args

OBJECTS=$(GNUEFI_DIR)/x86_64/gnuefi/crt0-efi-x86_64.o main.o uefi.o acpi.o gdt.o idt.o isr.o exceptions.o serial.o instructor.o graphics.o

all:
	$(AS) --64 src/instructor.s -o instructor.o
	$(AS) --64 src/isr.s -o isr.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/main.c -o main.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/serial.c -o serial.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/gdt.c -o gdt.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/idt.c -o idt.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/uefi.c -o uefi.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/acpi.c -o acpi.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/graphics.c -o graphics.o
	$(CC) -I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude $(CFLAGS) -c src/exceptions.c -o exceptions.o

	$(LD) -shared -Bsymbolic -L$(GNUEFI_DIR)/x86_64/gnuefi -L$(GNUEFI_DIR)/x86_64/lib -T$(GNUEFI_DIR)/gnuefi/elf_x86_64_efi.lds $(OBJECTS) -o main.so -lgnuefi -lefi
	
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dysym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so myos.efi

	cp myos.efi BOOTX64.EFI

	dd if=/dev/zero of=myos.img bs=1k count=16384
	mformat -i myos.img -t 32 -h 2 -s 1000 ::
	mmd -i myos.img ::/EFI
	mmd -i myos.img ::/EFI/BOOT
	mcopy -i myos.img BOOTX64.EFI ::/EFI/BOOT
	mcopy -i myos.img jep.txt ::/

	cp myos.img iso
	xorriso -as mkisofs -R -f -e myos.img -no-emul-boot -o myos.iso iso


# starts the VM and boots the kernel
# use the following to see CPU state:
# -no-reboot -d int,cpu_reset
run:
	qemu-system-x86_64 -serial stdio -L $(OVMF_DIR) -bios OVMF.fd -cdrom myos.iso -m 512M


clean-all:
	rm *.o *.so *.iso *.img *.efi *.EFI iso/myos.img
