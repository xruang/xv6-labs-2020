#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\nxv6 kernel is booting\n\n");

    kinit();
    printf("kinit ok\n");

    kvminit();
    printf("kvminit ok\n");

    kvminithart();
    printf("kvminithart ok\n");

    procinit();
    printf("procinit ok\n");

    trapinit();
    printf("trapinit ok\n");

    trapinithart();
    printf("trapinithart ok\n");

    plicinit();
    printf("plicinit ok\n");

    plicinithart();
    printf("plicinithart ok\n");

    binit();
    printf("binit ok\n");

    iinit();
    printf("iinit ok\n");

    fileinit();
    printf("fileinit ok\n");

    virtio_disk_init();
    printf("virtio_disk_init ok\n");

    userinit();
    printf("userinit ok\n");

    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();
    trapinithart();
    plicinithart();
  }

  scheduler();
}
