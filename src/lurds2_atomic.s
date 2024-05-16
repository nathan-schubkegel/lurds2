//;
//; This is free and unencumbered software released into the public domain under The Unlicense.
//; You have complete freedom to do anything you want with the software, for any purpose.
//; Please refer to <http://unlicense.org/>
//;
//;

.text

.globl AtomicIncrement32
AtomicIncrement32:
.globl AtomicIncrement32s
AtomicIncrement32s:
  //; Arguments: (rax) - 64-bit pointer to 32-bit value to increment
  //; Return: (rax) - the incremented 32-bit value in a 64-bit register
  push %rdi
  mov $1, %edi
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %edi, (%rax) //; store sum of these in destination [rax] and old value in source edi - see https://www.felixcloutier.com/x86/xadd
  inc %edi
  mov $0, %rax
  mov %edi, %eax
  pop %rdi
  ret

.globl AtomicIncrement64
AtomicIncrement64:
.globl AtomicIncrement64s
AtomicIncrement64s:
  //; Arguments: (rax) - 64-bit pointer to 64-bit value to increment
  //; Return: (rax) - the incremented 64-bit value
  push %rdi
  mov $1, %rdi
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %rdi, (%rax) //; store sum of these in destination [rdi] and old value in source rax - see https://www.felixcloutier.com/x86/xadd
  inc %rdi
  mov %rdi, %rax
  pop %rdi
  ret

.globl AtomicDecrement32
AtomicDecrement32:
.globl AtomicDecrement32s
AtomicDecrement32s:
  //; Arguments: (rax) - 64-bit pointer to 32-bit value to decrement
  //; Return: (rax) - the decremented 32-bit value in a 64-bit register
  push %rdi
  mov $-1, %edi
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %edi, (%rax) //; store sum of these in destination [rax] and old value in source edi - see https://www.felixcloutier.com/x86/xadd
  dec %edi
  mov $0, %rax
  mov %edi, %eax
  pop %rdi
  ret

.globl AtomicDecrement64
AtomicDecrement64:
.globl AtomicDecrement64s
AtomicDecrement64s:
  //; Arguments: (rax) - 64-bit pointer to 64-bit value to decrement
  //; Return: (rax) - the decremented 64-bit value
  push %rdi
  mov $-1, %rdi
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %rdi, (%rax) //; store sum of these in destination [rdi] and old value in source rax - see https://www.felixcloutier.com/x86/xadd
  dec %rdi
  mov %rdi, %rax
  pop %rdi
  ret
