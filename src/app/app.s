
.global _start
_start:

  # Perform the FACE syscall
  mov $0xFACE, %rax
  mov $0xBEEFD00D, %rcx
  int $0xA0

  # Enter into an infinite loop
.app_loop:
  pause
  jmp .app_loop
