
# cross compiler built without red zone
CC_DIR=/home/john/opt/cross-compiler-wrz/bin

# repos built from source
REPOS=/home/john/development

# build tools
CC=$(CC_DIR)/x86_64-elf-gcc
AS=$(CC_DIR)/x86_64-elf-as
LD=$(CC_DIR)/x86_64-elf-ld
OBJCOPY=objcopy

# GNU-EFI utilities
GNUEFI_DIR=$(REPOS)/gnu-efi

# OVMF firmware used by QEMU
OVMF_DIR=$(REPOS)/edk2/Build/OvmfX64/DEBUG_GCC5/FV

# C compilation flags
CFLAGS=-fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args

# C additional include directories
CINCLUDES=-I$(GNUEFI_DIR)/inc -I$(GNUEFI_DIR)/inc/x86_64 -Iinclude

# object files generated during build
OBJECTS=\
$(GNUEFI_DIR)/x86_64/gnuefi/crt0-efi-x86_64.o \
instructor.o \
main.o \
firmware.o \
graphics.o \
serial.o \
console.o \
memory.o \
paging.o \
acpi.o \
gdt.o \
idt.o \
isr.o \
pic.o \
pit.o \
exceptions.o \
mtrr.o \
util.o \
apic.o \
task.o \
klibc.o


all: myos.iso
	xorriso -as mkisofs -R -f -e myos.img -no-emul-boot -o myos.iso iso


myos.iso: myos.efi
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dysym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so myos.efi

	cp myos.efi BOOTX64.EFI

	dd if=/dev/zero of=myos.img bs=1k count=16384
	mformat -i myos.img -t 32 -h 2 -s 1000 ::
	mmd -i myos.img ::/EFI
	mmd -i myos.img ::/EFI/BOOT
	mcopy -i myos.img BOOTX64.EFI ::/EFI/BOOT
	mcopy -i myos.img zap-vga16.psf ::/

	cp myos.img iso


myos.efi: klibc.o
	$(AS) --64 src/osdev64/instructor.s -o instructor.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/main.c -o main.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/firmware.c -o firmware.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/graphics.c -o graphics.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/serial.c -o serial.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/console.c -o console.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/memory.c -o memory.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/paging.c -o paging.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/acpi.c -o acpi.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/gdt.c -o gdt.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/idt.c -o idt.o
	$(AS) --64 src/osdev64/isr.s -o isr.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/pic.c -o pic.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/pit.c -o pit.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/exceptions.c -o exceptions.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/mtrr.c -o mtrr.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/util.c -o util.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/apic.c -o apic.o
	$(CC) $(CINCLUDES) $(CFLAGS) -c src/osdev64/task.c -o task.o

	$(LD) -shared -Bsymbolic -L$(GNUEFI_DIR)/x86_64/gnuefi -L$(GNUEFI_DIR)/x86_64/lib -T$(GNUEFI_DIR)/gnuefi/elf_x86_64_efi.lds $(OBJECTS) -o main.so -lgnuefi -lefi


klibc.o:
	$(CC) -Iklibc -Iinclude $(CFLAGS) -c src/klibc/klibc.c -o klibc.o


# starts the VM and boots the kernel
# use the following to see CPU state:
# -no-reboot -d int,cpu_reset
run:
	qemu-system-x86_64 -serial stdio -L $(OVMF_DIR) -bios OVMF.fd -cdrom myos.iso -m 512M


.PHONY : clean
clean:
	rm *.o *.so *.iso *.img *.efi *.EFI iso/myos.img
