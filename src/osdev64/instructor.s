# The instructor contains various procedures that send instructions to the CPU
# for which there is no standard C interface.
# For example: IN, OUT, LGDT, etc.

.section .text

# procedures
.global k_disable_interrupts
.global k_enable_interrupts
.global k_outb
.global k_inb
.global k_lgdt
.global k_lidt
.global k_ltr
.global k_cause_exception

.global k_xor
.global k_get_cr0
.global k_set_cr0
.global k_get_cr3
.global k_set_cr3
.global k_get_cr4
.global k_set_cr4
.global k_get_rflags
.global k_set_rflags
.global k_cpuid_rax
.global k_cpuid_rdx
.global k_cpuid_vendor
.global k_get_pat
.global k_get_mtrrcap
.global k_get_msr



# disabled interrupts
#
# Featured instruction: CLI
k_disable_interrupts:
  cli
  retq

# enables interrupts
#
# Featured instruction: STI
k_enable_interrupts:
  sti
  retq


# Writes an 8-bit value to a 16-bit port.
# The first argument is a 16-bit port number, and the second argument
# is an 8-bit value to be written.
# This procedure does not modify the stack or base pointers.
#
# Featured instructions: OUT
#
# Params:
#   %rdi - a 16-bit port number
#   %rsi - an 8-bit number to be written
k_outb:
  mov %di, %dx
  mov %sil, %al

  out %al, %dx

  retq


# Reads an 8-bit value from a 16-bit port.
# The argument is a 16-bit port number.
# This procedure does not modify the stack or base pointers.
#
# Featured instructions: IN
#
# Params:
#   %rdi - a 16-bit port number
#
# Returns:
#   %rax - an 8-bit value
k_inb:

  mov %di, %dx

  in %dx, %al
  
  retq


# Loads the GDT) into GDTR.
#
# Featured instructions: LGDT
#
# Params:
#   %rdi - a 16-bit number representing the number of bytes in the GDT - 1
#   %rsi - a 64-bit number representing the address of the GDT
k_lgdt:

  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp # allocate 16 bytes on the stack

  mov %di, -0x10(%rbp)  # put the limit on the stack frame
  mov %rsi, -0xE(%rbp) # put the address of the GDT on the stack frame

  # Our stack frame now has the following structure:
  # 
  # base pointer
  # +-----------------+
  # | address of GDT  |
  # |                 |
  # |                 |
  # |                 |
  # |                 |
  # |                 |
  # |                 |
  # |                 |
  # +-----------------+
  # | size of GDT - 1 |
  # |                 |
  # +-----------------+
  # stack pointer

  lea -0x10(%rbp), %rax # load the address of our stack frame into %rax

  lgdt (%rax) # load the GDT into the GDTR

  # Put the offset of the data segment in DS, ES, FS, GS, and SS
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  
  # Put the offset of the code segment in CS using the LRET instruction.
  # The LRET instruction pops a value off the top of the stack into the
  # instruction pointer register, then pops another value off the stack
  # into the CS register.
  # This feels like cheating.
  push $0x8
  lea end_gdt(%rip), %rax
  push %rax
  lretq

end_gdt:
  leaveq
  retq


# Load the offset of the TSS selector into the TR register.
#
# Features instructions: LTR
#
# Params:
#   %rdi - 16 bits representing the offset of the TSS descriptor inthe GDT
k_ltr:

  push %rbp
  mov %rsp, %rbp

  mov %di, %ax  # offset of TSS descriptor in GDT

  ltr %ax

  leaveq
  retq


# Loads the interrupt descriptor table (IDT) into the global descriptor table
# register (IDTR). This p
#
# Featured instructions: LIDT
#
# Params:
#   %rdi - a 16-bit number representing the number of bytes in the IDT - 1
#   %rsi - a 64-bit number representing the address of the IDT
k_lidt:

  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp # allocate 16 bytes on the stack

  mov %di, -0x10(%rbp)  # IDT size limit
  mov %rsi, -0xE(%rbp) # IDT address

  lea -0x10(%rbp), %rax # address of the stack frame

  lidt (%rax) # load the IDT into the IDTR
  
  leaveq
  retq


# A procedure used to test exception handling with ISRs
k_cause_exception:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  # int $32

  # divide error
  mov $0x0, %rdx
  mov $0x4, %rax
  div %rdx

  # page fault (assumes only first 4GiB of RAM are identity mapped)
  # mov $0xFFFFFFFF00000000, %rax
  # mov (%rax), %rbx

  leaveq
  retq

# executes the XOR instruction
#
# Params:
#   %rdi - a 64-bit unsigned integer
#   %rsi - a 64-bit unsigned integer
#
# Returns:
#   %rax - the result of the XOR instruction
k_xor:

  push %rbp
  mov %rsp, %rbp

  xor %rdi, %rsi
  mov %rsi, %rax

  leaveq
  retq


# Reads the value from CR0
k_get_cr0:
  push %rbp
  mov %rsp, %rbp

  mov %cr0, %rax

  leaveq
  retq


# Writes a value into CR0
k_set_cr0:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr0

  leaveq
  retq


# Reads the value from CR3
k_get_cr3:
  push %rbp
  mov %rsp, %rbp

  mov %cr3, %rax

  leaveq
  retq

# Writes a value into CR3
k_set_cr3:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr3

  leaveq
  retq


# Reads the value from CR4
k_get_cr4:
  push %rbp
  mov %rsp, %rbp

  mov %cr4, %rax

  leaveq
  retq


# Writes a value into CR3
k_set_cr4:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr4

  leaveq
  retq


k_get_rflags:
  push %rbp
  mov %rsp, %rbp

  pushfq
  pop %rax

  leaveq
  retq


k_set_rflags:
  push %rbp
  mov %rsp, %rbp

  push %rdi
  popfq

  leaveq
  retq


# Executes the CPUID instruction and returns the value
# in RAX.
#
# Params:
#   %rdi - the input provided to CPUID
#
# Returns:
#   %rax - the result out the CPUID instruction
k_cpuid_rax:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %rax
  cpuid

  leaveq
  retq


k_cpuid_rdx:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %rax
  cpuid
  mov %rdx, %rax

  leaveq
  retq


k_cpuid_vendor:
  push %rbp
  mov %rsp, %rbp

  mov $0x0, %rax
  cpuid
  mov %rbx, (%rdi)
  mov %rdx, 4(%rdi)
  mov %rcx, 8(%rdi)

  leaveq
  retq

k_get_pat:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  mov $0x277, %ecx
  rdmsr
  mov %eax, -0x10(%rbp) # lo 32 bits
  mov %edx, -0xC(%rbp)  # hi 32 bits
  mov -0x10(%rbp), %rax

  leaveq
  retq


k_get_mtrrcap:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  mov $0xFE, %ecx
  rdmsr
  mov %eax, -0x10(%rbp) # lo 32 bits
  mov %edx, -0xC(%rbp)  # hi 32 bits
  mov -0x10(%rbp), %rax

  leaveq
  retq

k_get_msr:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  mov %rdi, %rcx
  rdmsr
  mov %eax, -0x10(%rbp) # lo 32 bits
  mov %edx, -0xC(%rbp)  # hi 32 bits
  mov -0x10(%rbp), %rax

  leaveq
  retq
