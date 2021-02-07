# The instructor contains various procedures that send instructions to the CPU
# for which there is no standard C interface.
# For example: IN, OUT, LGDT, etc.
.section .text

# Used to put the current task to sleep
.extern k_task_sleep
.extern k_task_resume

# Pushes the caller-saved registers onto the stack.
.macro push_caller_saved
  push %rax
  push %rcx
  push %rdx
  push %rsi
  push %rdi
  push %r8
  push %r9
  push %r10
  push %r11
  push %r11 # padding (for 16-byte alignment)
.endm

# Pops the caller-saved registers from the stack.
.macro pop_caller_saved
  pop %r11 # padding (for 16-byte alignment)
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rdi
  pop %rsi
  pop %rdx
  pop %rcx
  pop %rax
.endm





# Executes the CLI instruction to disable interrupts.
.global k_disable_interrupts
k_disable_interrupts:
  cli
  retq


# Executes the STI instruction to enable interrupts.
.global k_enable_interrupts
k_enable_interrupts:
  sti
  retq


# Executes the OUT isntruction to write an 8-bit value to a 16-bit port.
# The first argument is a 16-bit port number, and the second argument
# is an 8-bit value to be written.
# This procedure does not modify the stack or base pointers.
#
# Params:
#   RDI - a 16-bit port number
#   RSI - an 8-bit number to be written
.global k_outb
k_outb:
  mov %di, %dx
  mov %sil, %al
  out %al, %dx
  retq


# Executes the IN instruction to read an 8-bit value from a 16-bit port.
# The argument is a 16-bit port number.
# This procedure does not modify the stack or base pointers.
#
# Params:
#   RDI - a 16-bit port number
#
# Returns:
#   RAX - an 8-bit value
.global k_inb
k_inb:
  mov %di, %dx
  in %dx, %al
  retq


# Executes the LGDT instruction to load the address of the GDT into the GDTR.
#
# Params:
#   RDI - a 16-bit number representing the number of bytes in the GDT - 1
#   RSI - a 64-bit number representing the address of the GDT
.global k_lgdt
k_lgdt:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp      # allocate 16 bytes on the stack
  mov %di, -0x10(%rbp) # put the limit on the stack frame
  mov %rsi, -0xE(%rbp) # put the address of the GDT on the stack frame

  lea -0x10(%rbp), %rax # get the address of the top of the stack frame
  lgdt (%rax)           # load the GDT into the GDTR

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


# Executes the LTR instruction to load the offset of the TSS selector
# into the TR register.
#
# Features instructions: LTR
#
# Params:
#   RDI - 16 bits representing the offset of the TSS descriptor in the GDT
.global k_ltr
k_ltr:
  push %rbp
  mov %rsp, %rbp

  mov %di, %ax # offset of TSS descriptor in GDT
  ltr %ax      # Load the offset of the TSS into TR.

  leaveq
  retq


# Loads the interrupt descriptor table (IDT) into the global descriptor table
# register (IDTR). This p
#
# Featured instructions: LIDT
#
# Params:
#   RDI - a 16-bit number representing the number of bytes in the IDT - 1
#   RSI - a 64-bit number representing the address of the IDT
.global k_lidt
k_lidt:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp      # Allocate 16 bytes on the stack.
  mov %di, -0x10(%rbp) # IDT size limit
  mov %rsi, -0xE(%rbp) # IDT address

  lea -0x10(%rbp), %rax # Get the address of the top of the stack frame.
  lidt (%rax)           # Load the IDT into the IDTR.
  
  leaveq
  retq


# A procedure used to test exception handling with ISRs
.global k_cause_exception
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


# Executes the XOR instruction.
#
# Params:
#   RDI - a 64-bit unsigned integer
#   RSI - a 64-bit unsigned integer
#
# Returns:
#   RAX - the result of the XOR instruction
.global k_xor
k_xor:
  push %rbp
  mov %rsp, %rbp

  xor %rdi, %rsi
  mov %rsi, %rax

  leaveq
  retq


# Reads the value of CR0.
#
# Returns:
#   RAX - the value read from CR0
.global k_get_cr0
k_get_cr0:
  push %rbp
  mov %rsp, %rbp

  mov %cr0, %rax

  leaveq
  retq


# Writes a value into CR0.
#
# Params:
#   RDI - the value to write to CR0
.global k_set_cr0
k_set_cr0:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr0

  leaveq
  retq


# Reads the value of CR3.
#
# Returns:
#   RAX - the value read from CR3
.global k_get_cr3
k_get_cr3:
  push %rbp
  mov %rsp, %rbp

  mov %cr3, %rax

  leaveq
  retq

# Writes a value into CR3.
# Params:
#
#   RDI - the value to write to CR3
.global k_set_cr3
k_set_cr3:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr3

  leaveq
  retq


# Reads the value of CR4.
#
# Returns:
#   RAX - the value read from CR4
.global k_get_cr4
k_get_cr4:
  push %rbp
  mov %rsp, %rbp

  mov %cr4, %rax

  leaveq
  retq


# Writes a value into CR4.
#
# Params:
#   RDI - the value to write to CR4
.global k_set_cr4
k_set_cr4:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %cr4

  leaveq
  retq


# Reads the value of RFLAGS.
#
# Returns:
#   RAX - the value read from RFLAGS
.global k_get_rflags
k_get_rflags:
  push %rbp
  mov %rsp, %rbp

  pushfq
  pop %rax

  leaveq
  retq


# Writes a value into RFLAGS.
#
# Params:
#   RDI - the value to write to RFLAGS
.global k_set_rflags
k_set_rflags:
  push %rbp
  mov %rsp, %rbp

  push %rdi
  popfq

  leaveq
  retq


# Executes the CPUID instruction and returns the value that was placed
# in RAX.
#
# Params:
#   RDI - the input provided to CPUID
#
# Returns:
#   RAX - the value placed in RAX by the CPUID instruction
.global k_cpuid_rax
k_cpuid_rax:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %rax
  cpuid

  leaveq
  retq


# Executes the CPUID instruction and returns the value that was placed
# in RDX.
#
# Params:
#   RDI - the input provided to CPUID
#
# Returns:
#   RAX - the value placed in RDX by the CPUID instruction
.global k_cpuid_rdx
k_cpuid_rdx:
  push %rbp
  mov %rsp, %rbp

  mov %rdi, %rax
  cpuid
  mov %rdx, %rax

  leaveq
  retq


# Executes the CPUID instruction to get the vendor identification string.
# At the start of the procedure, RDI is expected to contain the address
# of at least 12 bytes of memory.
#
# Params:
#   RDI - address of 12 bytes of memory
.global k_cpuid_vendor
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

# Executes the RDMSR isntruction to read the value of an MSR.
#
# Params:
#   RDI - the address of an MSR
#
# Returns:
#   RAX - the contents of an MSR
.global k_msr_get
k_msr_get:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  mov %rdi, %rcx        # MSR address

  rdmsr                 # Read the MSR

  mov %eax, -0x10(%rbp) # lo 32 bits
  mov %edx, -0xC(%rbp)  # hi 32 bits

  mov -0x10(%rbp), %rax

  leaveq
  retq


# Executes the WRMSR isntruction to write a value into an MSR.
#
# Params:
#   RDI - the address of an MSR
#   RSI - the value to write to the MSR
.global k_msr_set
k_msr_set:
  push %rbp
  mov %rsp, %rbp

  sub $0x10, %rsp

  mov %rdi, %rcx         # MSR address
  mov %rsi, -0x10(%rbp)  # MSR contents

  mov -0x10(%rbp), %eax  # lo 32 bits
  mov -0xC(%rbp), %edx   # hi 32 bits

  wrmsr                  # Write the MSR

  leaveq
  retq


# Used for debugging stuff.
.global k_nonsense
k_nonsense:
  push %rbp
  mov %rsp, %rbp

  int $0x30
  
  leaveq
  retq


# Executes the XCHG instruction to swap the value of RDI with the
# value pointed to by RSI. The XCHG instruction automatically locks the bus.
#
# Params:
#   RDI - a value to be written to the memory location
#   RSI - a memory location containing a value to be read
#
# Returns:
#   RAX - the value formerly held at the memory location
.global k_xchg
k_xchg:
  xchg %rdi, (%rsi)
  mov %rdi, %rax
  retq


# Executes the XADD instruction to add the value of RDI to the
# value pointed to by RSI. The previous value held at the memory
# location is returned. The lock prefix is added to lock the bus.
#
# Params:
#   RDI - a value to be added to the value at a memory location
#   RSI - a memory location containing a value to be added
#
# Returns:
#   RAX - the value formerly held at the memory location
.global k_xadd
k_xadd:
  lock xadd %rdi, (%rsi)
  mov %rdi, %rax
  retq


.global k_bts
k_bts:
  lock bts %rdi, (%rsi)
  jc .bts_carry
  movq $0x0, %rax
  retq
.bts_carry:
  movq $0x1, %rax
  retq


.global k_btr
k_btr:
  lock btr %rdi, (%rsi)
  jc .btr_carry
  movq $0x0, %rax
  retq
.btr_carry:
  movq $0x1, %rax
  retq


# TODO: come up with better names for these procedures


# Attempts to set bit 0 of the value pointed to by RDI.
# If bit 0 is already set, then this procedure enters a loop and
# repeatedly tests the value until it is 0. Once bit 0 is cleared,
# the procedure makes another attempt to set it.
#
# Params:
#   RDI - a memory location pointing to the byte whose bit 0 will be set
#
.global k_bts_spin
k_bts_spin:
  lock bts $0, (%rdi) # attempt to set a bit
  jc .bts_spin_loop     # if the bit was already set, loop until it isn't set
  retq

.bts_spin_loop:
  pause
  testq $0x1, (%rdi) # check if the bit is set
  jnz .bts_spin_loop # if the bit is set, repeat the loop
  jmp k_bts_spin     # if the bit is not set, jump back to k_bts_spin


# Executes the BTS instruction to set bit 0 of a 64-bit value.
# If the carry flag is set after executing the BTS instruction,
# then this procedure puts the current task to sleep until
# the lock has a value of 0.
#
# Params:
#   RDI - a memory location pointing to the byte whose bit 0 will be set
#
.global k_bts_sleep
k_bts_sleep:
  lock bts $0, (%rdi) # attempt to set a bit
  jc .bts_sleep       # If the bit was already set, then sleep.
  retq

.bts_sleep:
  mov $1, %rdx    # Indicate that the task is waiting on a lock.
  int $0x40       # raise interrupt 64 to put the task to sleep
  jmp k_bts_sleep # restart the procedure


# Attempts to decrement a semaphore.
# If the value is less than 0, this procedure loops until it is >= 0,
# at which point it restarts execution from the beginning.
#
# Params:
#   RDI - the memory location of a sempahore
.global k_sem_wait
k_sem_wait:
  mov (%rdi), %rax
  test %rax, %rax        # Set the sign flag if the semaphore is < 0.
  js .sem_wait_loop      # If the value is < 0, loop until it's >= 0.
  mov $-1, %rdx          # Store -1 in RDX so it can be used with XADD.
  lock xadd %rdx, (%rdi) # Add -1 to the value to decrement it.
  retq

.sem_wait_loop:
  pause
  mov (%rdi), %rax
  test %rax, %rax   # Set the sign flag if the semaphore is < 0.
  js .sem_wait_loop # If the value is < 0, repeat the loop.
  jmp k_sem_wait    # If the value is >= 0, restart the procedure.


# Attempts to decrement a semaphore.
# If the value is less than 0, this procedure loops puts the current task
# to sleep until it is >= 0, at which point it restarts execution from
# the beginning.
#
# Params:
#   RDI - the memory location of a sempahore
.global k_sem_sleep
k_sem_sleep:
  mov (%rdi), %rax
  test %rax, %rax        # Set the sign flag if the semaphore is < 0.
  js .sem_sleep          # If the value is < 0, put the current task to sleep.
  mov $-1, %rdx          # Store -1 in RDX so it can be used with XADD.
  lock xadd %rdx, (%rdi) # Add -1 to the value to decrement it.
  retq

.sem_sleep:
  mov $2, %rdx     # Indicate that the task is waiting on a semaphore.
  int $0x40        # Raise interrupt 64 to put the current task to sleep.
  jmp k_sem_sleep  # Restart the procedure.
