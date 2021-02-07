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
  jc .bts_spin_loop    # If bit 0 was already set, loop until it isn't set.
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
# Params:
#   RDI - a memory location pointing to the byte whose bit 0 will be set
#
.global k_lock_sleep
k_lock_sleep:
  lock btsq $0, (%rdi) # Attempt to set bit 0.
  jc .lock_sleep_loop  # If bit 0 was already set, sleep until it isn't set.
  retq

.lock_sleep_loop:
  mov $1, %rdx     # Indicate that the task is waiting on a lock.
  int $0x40        # Raise interrupt 64 to put the task to sleep.
  jmp k_lock_sleep # Restart the procedure.