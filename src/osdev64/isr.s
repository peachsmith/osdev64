.section .text

# Callee-saved registers:
# rbx
# rsp
# rbp
# r12
# r13
# r14
# r15

# Caller-saved registers:
# rax
# rcx
# rdx
# rsi
# rdi
# r8
# r9
# r10
# r11
# r11


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

# Generic handler for IRQs.
.macro handle_generic_irq n:req handler:req
  cld
  push_caller_saved
  mov $\n, %rdi
  call \handler
  pop_caller_saved
.endm



# Pushes the registers onto the stack in preparation for a task switch.
.macro push_task_regs
  push %rax
  push %rbx
  push %rcx
  push %rdx
  push %rsi
  push %rdi
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  push %rbp
  push %rbp # padding (for 16-byte alignment)
.endm

# Pops the register values from the stack after a task switch.
.macro pop_task_regs
  pop %rbp # padding (for 16-byte alignment)
  pop %rbp
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rdi
  pop %rsi
  pop %rdx
  pop %rcx
  pop %rbx
  pop %rax
.endm




# interrupt service routine (ISR) entry points
.global isr0
.global isr1
.global isr2
.global isr3
.global isr4
.global isr5
.global isr6
.global isr7
.global isr8
.global isr9
.global isr10
.global isr11
.global isr12
.global isr13
.global isr14
.global isr15
.global isr16
.global isr17
.global isr18
.global isr19
.global isr20
.global isr21
.global isr22
.global isr23
.global isr24
.global isr25
.global isr26
.global isr27
.global isr28
.global isr29
.global isr30
.global isr31


.global generic_isr
.global apic_generic_isr
.global apic_spurious_isr
.global apic_generic_legacy_isr


# debug handler
.global debug_isr


# PIC IRQ handlers
.global pic_irq_0
.global pic_irq_1
.global pic_irq_2
.global pic_irq_3
.global pic_irq_4
.global pic_irq_5
.global pic_irq_6
.global pic_irq_7
.global pic_irq_8
.global pic_irq_9
.global pic_irq_10
.global pic_irq_11
.global pic_irq_12
.global pic_irq_13
.global pic_irq_14
.global pic_irq_15

# APIC IRQ handlers
.global apic_irq_0
.global apic_irq_1
.global apic_irq_2
.global apic_irq_3
.global apic_irq_4
.global apic_irq_5
.global apic_irq_6
.global apic_irq_7
.global apic_irq_8
.global apic_irq_9
.global apic_irq_10
.global apic_irq_11
.global apic_irq_12
.global apic_irq_13
.global apic_irq_14
.global apic_irq_15



# handler functions implemented in C
.extern div0_handler
.extern gp_fault_handler
.extern page_fault_handler
.extern generic_handler
.extern pic_handler
.extern apic_generic_handler
.extern apic_spurious_handler
.extern apic_pit_handler
.extern apic_generic_legacy_handler

# debug handler
.extern debug_handler

# used to send the EOI to the PIC
.extern k_pic_send_eoi



isr0:
  cld
  call div0_handler
  iretq


isr1:
  iretq


isr2:
  iretq


isr3:
  iretq


isr4:
  iretq


isr5:
  iretq


isr6:
  iretq


isr7:
  iretq


isr8:
  iretq


isr9:
  iretq


isr10:
  iretq


isr11:
  iretq


isr12:
  iretq


isr13:
  cld
  call gp_fault_handler
  iretq


isr14:
  cld
  call page_fault_handler
  iretq


isr15:
  iretq


isr16:
  iretq


isr17:
  iretq


isr18:
  iretq


isr19:
  iretq


isr20:
  iretq


isr21:
  iretq


isr22:
  iretq


isr23:
  iretq


isr24:
  iretq


isr25:
  iretq


isr26:
  iretq


isr27:
  iretq


isr28:
  iretq


isr29:
  iretq


isr30:
  iretq


isr31:
  iretq



generic_isr:
  cld
  call generic_handler
  iretq



pic_irq_0:
  cld
  mov $0x0, %rdi
  call pic_handler
  iretq

pic_irq_1:
  cld
  mov $0x1, %rdi
  call pic_handler
  iretq

pic_irq_2:
  cld
  mov $0x2, %rdi
  call pic_handler
  iretq

pic_irq_3:
  cld
  mov $0x3, %rdi
  call pic_handler
  iretq

pic_irq_4:
  cld
  mov $0x4, %rdi
  call pic_handler
  iretq
  
pic_irq_5:
  cld
  mov $0x5, %rdi
  call pic_handler
  iretq

pic_irq_6:
  cld
  mov $0x6, %rdi
  call pic_handler
  iretq

pic_irq_7:
  cld
  mov $0x7, %rdi
  call pic_handler
  iretq

pic_irq_8:
  cld
  mov $0x8, %rdi
  call pic_handler
  iretq

pic_irq_9:
  cld
  mov $0x9, %rdi
  call pic_handler
  iretq

pic_irq_10:
  cld
  mov $0xA, %rdi
  call pic_handler
  iretq

pic_irq_11:
  cld
  mov $0xB, %rdi
  call pic_handler
  iretq

pic_irq_12:
  cld
  mov $0xC, %rdi
  call pic_handler
  iretq

pic_irq_13:
  cld
  mov $0xD, %rdi
  call pic_handler
  iretq

pic_irq_14:
  cld
  mov $0xE, %rdi
  call pic_handler
  iretq

pic_irq_15:
  cld
  mov $0xF, %rdi
  call pic_handler
  iretq





apic_irq_0:
  cld

  # RSP is now assumed to be 16-byte aligned
  # The stack should have this structure:
  # +------------+
  # | SS         |
  # | RSP        | <- RSP from before entering this ISR
  # | RFLAGS     |
  # | CS         |
  # | RIP        |
  # +------------+ <- RSP currently points to the top of this stack frame

  # Push the task registers onto the stack.
  push_task_regs

  # After pushing the current task's registers onto the stack,
  # the local stack frame should have this structure:
  #
  # +------------+
  # | SS         |
  # | RSP        |
  # | RFLAGS     |
  # | CS         |
  # | RIP        |
  # |------------|
  # | RAX        |
  # | RBX        |
  # | RCX        |
  # | RDX        |
  # | RSI        |
  # | RDI        |
  # | r8         |
  # | r9         |
  # | r10        |
  # | r11        |
  # | r12        |
  # | r13        |
  # | r14        |
  # | r15        |
  # | RBP        |
  # | padding    |
  # +------------+ <- RSP now points to the top of this stack frame

  # Call the timer handler.
  mov %rsp, %rdi
  call apic_pit_handler
  mov %rax, %rsp

  # Pop the task registers from the stack.
  pop_task_regs

  iretq

apic_irq_1:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_2:
  handle_generic_irq 2, apic_generic_legacy_handler
  iretq

apic_irq_3:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_4:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq
  
apic_irq_5:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_6:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_7:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_8:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_9:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_10:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_11:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_12:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_13:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_14:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq

apic_irq_15:
  handle_generic_irq 1, apic_generic_legacy_handler
  iretq






apic_generic_isr:
  cld
  push_caller_saved
  call apic_generic_handler
  pop_caller_saved
  iretq

apic_spurious_isr:
  cld
  push_caller_saved
  call apic_spurious_handler
  pop_caller_saved
  iretq

apic_generic_legacy_isr:
  cld
  push_caller_saved
  call apic_generic_legacy_handler
  pop_caller_saved
  iretq


debug_isr:
  cld
  push_caller_saved
  call debug_handler
  pop_caller_saved
  iretq




# The system call ISR
#
# Currently supported system calls:
# 2 STOP  stops the current task
# 3 SLEEP puts the current task to sleep
# 0xFACE FACE  writes a number somewhere
.global k_syscall_isr
k_syscall_isr:

  # Upon entering this procedure, the following registers should
  # have the following values:
  # RAX: syscall ID
  # RCX: syscall data 1
  # RDX: syscall data 2
  # RSI: syscall data 3
  # RDI: syscall data 4

  # Prepare the arguments for k_syscall
  # ARG 1: RDI syscall ID
  # ARG 2: RSI syscall data 1
  # ARG 3: RDX syscall data 2
  # ARG 4: RCX syscall data 3
  # ARG 5: R8  syscall data 4

  cld

  cmpq $2, %rax # check for STOP syscall
  je .sc_stop

  cmpq $3, %rax # check for SLEEP_SYNC syscall
  je .sc_sleep_sync

  cmpq $4, %rax # check for SLEEP_TICK syscall
  je .sc_sleep_tick

  cmpq $5, %rax # check for WRITE syscall
  je .sc_write

  cmpq $6, %rax # check for READ syscall
  je .sc_read

  cmpq $0xFACE, %rax # check for FACE syscall
  je .sc_face

  iretq # Unrecognized syscall ID

.sc_stop:
  # TODO: make a macro for swapping all the argument registers
  push_task_regs # Save the task register stack.
  mov %rax, %rdi # ARG 1 (syscall ID)
  mov %rsp, %rsi # ARG 2 (register stack)
  call k_syscall # Invoke the syscall.
  mov %rax, %rsp # Get the new register stack.
  pop_task_regs  # Restore the task register stack.
  iretq          # return from ISR

.sc_sleep_sync:
  push_task_regs # Save the task register stack.
  mov %rax, %rdi # ARG 1 (syscall ID)
  mov %rsp, %rsi # ARG 2 (register stack)
  mov %rcx, %r11 # Store RCX in scratch register
  mov %rdx, %rcx # ARG 4 (address of synchronization value)
  mov %r11, %rdx # ARG 3 (synchronization type)
  call k_syscall # Invoke the syscall.
  mov %rax, %rsp # Get the new register stack.
  pop_task_regs  # Restore the task register stack.
  iretq          # return from ISR

.sc_sleep_tick:
  push_task_regs # Save the task register stack.
  mov %rax, %rdi # ARG 1 (syscall ID)
  mov %rsp, %rsi # ARG 2 (register stack)
  mov %rcx, %rdx # ARG 3 (number of ticks)
  call k_syscall # Invoke the syscall.
  mov %rax, %rsp # Get the new register stack.
  pop_task_regs  # Restore the task register stack.
  iretq          # return from ISR

.sc_face:
  push_caller_saved # Save caller-saved registers.
  mov %rax, %rdi    # ARG 1 (syscall ID)
  mov %rcx, %rsi    # ARG 2 (a number)
  call k_syscall    # Invoke the syscall.
  pop_caller_saved  # Restore caller-saved registers.
  iretq             # return from ISR

.sc_write:
  # RDX already contains ARG 3 (source buffer)
  mov %rax, %rdi    # ARG 1 (syscall ID)
  mov %rcx, %r8     # Store RCX
  mov %rsi, %rcx    # ARG 4 (number of bytes to write) 
  mov %r8, %rsi     # ARG 2 (file pointer)
  call k_syscall    # Invoke the syscall.
  iretq             # return from ISR

.sc_read:
  # RDX already contains ARG 3 (destination buffer)
  mov %rax, %rdi    # ARG 1 (syscall ID)
  mov %rcx, %r8     # Store RCX
  mov %rsi, %rcx    # ARG 4 (number of bytes to read) 
  mov %r8, %rsi     # ARG 2 (file pointer)
  call k_syscall    # Invoke the syscall.
  iretq             # return from ISR
