        # qemu -kernel loads the kernel at 0x80000000
        # and causes each CPU to jump there.
        # kernel.ld causes the following code to be
        # placed at 0x80000000.

        .section .text
        .globl _entry
_entry:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with 4096 bytes per CPU.
        la sp, stack0

        # sp = stack0 + 4096 * (hartid + 1)
        li a0, 4096
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0

        # jump to start() in start.c
        call start

spin:
        j spin
