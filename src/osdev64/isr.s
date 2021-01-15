.section .text

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



# handler functions implemented in C
.extern div0_handler
.extern gp_fault_handler
.extern page_fault_handler
.extern generic_handler
.extern pic_handler

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
  call pic_handler
  cld
  mov $0x0, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_1:
  cld
  call pic_handler
  cld
  mov $0x1, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_2:
  cld
  call pic_handler
  cld
  mov $0x2, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_3:
  cld
  call pic_handler
  cld
  mov $0x3, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_4:
  cld
  call pic_handler
  cld
  mov $0x4, %rdi
  call k_pic_send_eoi
  iretq
  
pic_irq_5:
  cld
  call pic_handler
  cld
  mov $0x5, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_6:
  cld
  call pic_handler
  cld
  mov $0x6, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_7:
  cld
  call pic_handler
  cld
  mov $0x7, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_8:
  cld
  call pic_handler
  cld
  mov $0x8, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_9:
  cld
  call pic_handler
  cld
  mov $0x9, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_10:
  cld
  call pic_handler
  cld
  mov $0xA, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_11:
  cld
  call pic_handler
  cld
  mov $0xB, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_12:
  cld
  call pic_handler
  cld
  mov $0xC, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_13:
  cld
  call pic_handler
  cld
  mov $0xD, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_14:
  cld
  call pic_handler
  cld
  mov $0xE, %rdi
  call k_pic_send_eoi
  iretq

pic_irq_15:
  cld
  call pic_handler
  cld
  mov $0xF, %rdi
  call k_pic_send_eoi
  iretq
