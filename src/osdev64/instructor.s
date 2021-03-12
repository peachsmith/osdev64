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


# Executes the IN instruction to read a 16-bit value from a 16-bit port.
# The argument is a 16-bit port number.
# This procedure does not modify the stack or base pointers.
#
# Params:
#   RDI - a 16-bit port number
#
# Returns:
#   RAX - an 16-bit value
.global k_inw
k_inw:
  mov %di, %dx
  in %dx, %ax
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


# Executes the BTS instruction to set bit 0 of a 64-bit value.
# If bit 0 was already set, then this procedure loops until
# bit 0 is no longer set, at which point it restarts execution
# from the beginning.
#
# C function signature:
#   void k_lock_spin(k_regn*);
#
# Params:
#   RDI - a memory location pointing to the byte whose bit 0 will be set
#
.global k_lock_spin
k_lock_spin:
  lock btsq $0, (%rdi) # Attempt to set bit 0.
  jc .lock_spin_loop   # If bit 0 was already set, loop until it isn't set.
  retq

.lock_spin_loop:
  pause
  testq $0x1, (%rdi)  # Check if the bit is set.
  jnz .lock_spin_loop # If the bit is set, repeat the loop.
  jmp k_lock_spin     # If the bit is not set, jump back to k_lock_spin.


# Executes the BTS instruction to set bit 0 of a 64-bit value.
# If bit 0 was already set, then this procedure puts the current task
# to sleep until bit 0 is no longer set, at which point it restarts
# execution from the beginning.
#
# C function signature:
#   void k_lock_sleep(k_regn*);
#
# Params:
#   RDI - a memory location pointing to the byte whose bit 0 will be set
#
.global k_lock_sleep
k_lock_sleep:
  lock btsq $0, (%rdi) # Attempt to set bit 0.
  jc .lock_sleep_loop  # If bit 0 was already set, sleep until it isn't set.
  retq

.lock_sleep_loop:
  push %rdi
  mov $3, %rax   # syscall ID is 3 (for SLEEP_SYNC)
  mov $1, %rcx   # synchronization type is 1 (for lock)
  mov %rdi, %rdx # address of lock
  int $0xA0      # Raise INT 160 to do the syscall
  pop %rdi
  jmp k_lock_sleep # Restart the procedure.


# Attempts to decrement a semaphore.
# If the value is <= 0, this procedure loops until it is > 0,
# at which point it restarts execution from the beginning.
#
# Params:
#   RDI - the memory location of a sempahore
.global k_sem_wait
k_sem_wait:

  # Add -1 to the value to decrement it.
  mov $-1, %rax
  lock xadd %rax, (%rdi)

  # If the value was <= 0, increment the value and loop until the value
  # is > 0.
  test %rax, %rax
  jz .sem_wait_rollback
  js .sem_wait_rollback

  # If the value was > 0 before decrementing, then return from the
  # procedure and access the synchronized resource.
  retq

.sem_wait_rollback:
  mov $1, %rax
  lock xadd %rax, (%rdi)

.sem_wait_loop:
  pause
  mov (%rdi), %rax
  test %rax, %rax
  jz .sem_wait_loop
  js .sem_wait_loop

  # If the value is > 0, restart the procedure.
  jmp k_sem_wait


# Attempts to decrement a semaphore.
# If the value is less than 0, this procedure puts the current task
# to sleep until it is >= 0, at which point it restarts execution from
# the beginning.
#
# Params:
#   RDI - the memory location of a sempahore
.global k_sem_sleep
k_sem_sleep:

  # Add -1 to the value to decrement it.
  mov $-1, %rax
  lock xadd %rax, (%rdi)

  # If the value was <= 0, increment the value and loop until the value
  # is > 0.
  test %rax, %rax
  jz .sem_sleep_rollback
  js .sem_sleep_rollback

  # If the value was > 0 before decrementing, then return from the
  # procedure and access the synchronized resource.
  retq

.sem_sleep_rollback:
  mov $1, %rax
  lock xadd %rax, (%rdi)

  # Put the task to sleep until the value is > 0.
  push %rdi

  mov $3, %rax   # syscall ID is 3 (for SLEEP_SYNC)
  mov $2, %rcx   # synchronization type is 2 (for semaphore)
  mov %rdi, %rdx # address of semaphore
  int $0xA0      # Raise INT 160 to do the syscall

  pop %rdi

  # The value should be > 0, so restart the procedure.
  jmp k_sem_sleep


# Invokes the FACE syscall.
.global k_syscall_face
k_syscall_face:
  push %rbp
  mov %rsp, %rbp

  mov $0xFACE, %rax # syscall ID is 0xFACE (for FACE)
  mov %rdi, %rcx
  int $0xA0

  leaveq
  retq


# Invokes the STOP syscall.
.global k_syscall_stop
k_syscall_stop:
  push %rbp
  mov %rsp, %rbp

  mov $2, %rax # syscall ID is 2 (for STOP)
  int $0xA0

  leaveq
  retq

.global k_syscall_sleep
k_syscall_sleep:
  push %rbp
  mov %rsp, %rbp

  mov $4, %rax   # syscall ID is 2 (for SLEEP_TICK)
  mov %rdi, %rcx # number of ticks
  int $0xA0

  leaveq
  retq

.global k_syscall_write
k_syscall_write:
  push %rbp
  mov %rsp, %rbp

  mov $5, %rax   # syscall ID is 5 (for WRITE)
  mov %rdi, %rcx # file pointer
  mov %rdx, %r8  # save the contents of RDX
  mov %rsi, %rdx # source buffer
  mov %r8, %rsi  # number of bytes to write
  int $0xA0

  # RAX should now contain the number of bytes written

  leaveq
  retq

.global k_syscall_read
k_syscall_read:
  push %rbp
  mov %rsp, %rbp

  mov $6, %rax   # syscall ID is 6 (for READ)
  mov %rdi, %rcx # file pointer
  mov %rdx, %r8  # save the contents of RDX
  mov %rsi, %rdx # destination buffer
  mov %r8, %rsi  # number of bytes to read
  int $0xA0

  # RAX should now contain the number of bytes read

  leaveq
  retq
