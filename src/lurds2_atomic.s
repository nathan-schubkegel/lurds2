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
  //; Arguments: (rdi) - 64-bit pointer to 32-bit value to increment
  //; Return: (eax) - the incremented 32-bit value
  mov $1, %eax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %eax, (%rdi) //; store sum of these in destination [rdi] and old value in source eax - see https://www.felixcloutier.com/x86/xadd
  inc %eax //; return the incremented value
  ret

.globl AtomicIncrement64
AtomicIncrement64:
.globl AtomicIncrement64s
AtomicIncrement64s:
  //; Arguments: (rdi) - 64-bit pointer to 64-bit value to increment
  //; Return: (rax) - the incremented 64-bit value
  mov $1, %rax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %rax, (%rdi) //; store sum of these in destination [rdi] and old value in source rax - see https://www.felixcloutier.com/x86/xadd
  inc %rax //; return the incremented value
  ret

.globl AtomicDecrement32
AtomicDecrement32:
.globl AtomicDecrement32s
AtomicDecrement32s:
  //; Arguments: (edi) - 64-bit pointer to 32-bit value to decrement
  //; Return: (rax) - the decremented 32-bit value in a 64-bit register
  mov $-1, %eax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %eax, (%rdi) //; store sum of these in destination [rdi] and old value in source eax - see https://www.felixcloutier.com/x86/xadd
  dec %eax //; return the decremented value
  ret

.globl AtomicDecrement64
AtomicDecrement64:
.globl AtomicDecrement64s
AtomicDecrement64s:
  //; Arguments: (rdi) - 64-bit pointer to 64-bit value to decrement
  //; Return: (rax) - the decremented 64-bit value
  mov $-1, %rax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xadd %rax, (%rdi) //; store sum of these in destination [rdi] and old value in source rax - see https://www.felixcloutier.com/x86/xadd
  dec %rax //; return the decremented value
  ret

.globl AtomicExchange32
AtomicExchange32:
.globl AtomicExchange32s
AtomicExchange32s:
  //; Arguments: (rdi) - 64-bit pointer to 32-bit value to overwrite
  //;            (esi) - 32-bit value to write at that pointer
  //; Return: (eax) - the old 32-bit value at that pointer
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xchg (%rdi), %esi  //; swap contents of [rdi] and esi - see https://www.felixcloutier.com/x86/xchg
  mov $0, %rax
  mov %esi, %eax
  ret

.globl AtomicExchange64
AtomicExchange64:
.globl AtomicExchange64s
AtomicExchange64s:
  //; Arguments: (rdi) - 64-bit pointer to 64-bit value to overwrite
  //;            (rsi) - 64-bit value to write at that pointer
  //; Return: (rax) - the old 64-bit value at that pointer
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  xchg (%rdi), %rsi  //; swap contents of [rdi] and rsi - see https://www.felixcloutier.com/x86/xchg
  mov %rsi, %rax
  ret

.globl AtomicCompareExchange32
AtomicCompareExchange32:
.globl AtomicCompareExchange32s
AtomicCompareExchange32s:
  //; Arguments: (rdi) - 64-bit pointer to 32-bit value to overwrite
  //;            (esi) - 32-bit value to write at pointer if pointer currently holds edx's value
  //;            (edx) - 32-bit value to compare with value at pointer
  //; Return: (eax) - the old 32-bit value at that pointer
  mov %edx, %eax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  cmpxchg %esi, (%rdi)  //; compare eax and [rdi]
                    //; if same, then Z = 1 and [rdi] = esi
                    //; if different, then Z = 0 and eax = [rdi]
                    //; see https://www.felixcloutier.com/x86/cmpxchg
  //; either way, eax is left populated with what is in [rdi]
  ret

.globl AtomicCompareExchange64
AtomicCompareExchange64:
.globl AtomicCompareExchange64s
AtomicCompareExchange64s:
  //; Arguments: (rdi) - 64-bit pointer to 64-bit value to overwrite
  //;            (rsi) - 64-bit value to write at pointer if pointer currently holds rdx's value
  //;            (rdx) - 64-bit value to compare with value at pointer
  //; Return: (rax) - the old 64-bit value at that pointer
  mov %rdx, %rax
  lock //; so the next instruction is atomic - see https://www.felixcloutier.com/x86/lock
  cmpxchg %rsi, (%rdi)  //; compare rax and [rdi]
                    //; if same, then Z = 1 and [rdi] = rsi
                    //; if different, then Z = 0 and rax = [rdi]
                    //; see https://www.felixcloutier.com/x86/cmpxchg
  //; either way, rax is left populated with what is in [rdi]
  ret
