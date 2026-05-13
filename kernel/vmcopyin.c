#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//
// This file contains copyin_new() and copyinstr_new(), the
// replacements for copyin and coyinstr in vm.c.
//

static struct stats {
  int ncopyin;
  int ncopyinstr;
} stats;

static int
appendint(char *buf, int sz, int n, int x)
{
  char tmp[16];
  int i = 0;

  if(sz <= 0)
    return n;

  if(x == 0){
    if(n < sz - 1)
      buf[n++] = '0';
    buf[n] = 0;
    return n;
  }

  while(x > 0){
    tmp[i++] = '0' + x % 10;
    x /= 10;
  }

  while(i > 0 && n < sz - 1)
    buf[n++] = tmp[--i];

  buf[n] = 0;
  return n;
}

int 
statscopyin(char *buf, int sz) 
{
  int n = 0;

  if(sz <= 0)
    return 0;

  buf[0] = 0;

  char *s1 = "copyin: ";
  for(; *s1 && n < sz - 1; s1++)
    buf[n++] = *s1;
  buf[n] = 0;

  n = appendint(buf, sz, n, stats.ncopyin);

  if(n < sz - 1)
    buf[n++] = '\n';
  buf[n] = 0;

  char *s2 = "copyinstr: ";
  for(; *s2 && n < sz - 1; s2++)
    buf[n++] = *s2;
  buf[n] = 0;

  n = appendint(buf, sz, n, stats.ncopyinstr);

  if(n < sz - 1)
    buf[n++] = '\n';
  buf[n] = 0;

  return n;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  stats.ncopyin++;
  
  struct proc *p = myproc();

  if (srcva >= p->sz || srcva+len >= p->sz || srcva+len < srcva)
    return -1;
  memmove((void *) dst, (void *)srcva, len);
  

  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
 stats.ncopyinstr++;
  struct proc *p = myproc();
  char *s = (char *) srcva;
  
  for(int i = 0; i < max && srcva + i < p->sz; i++){
    dst[i] = s[i];
    if(s[i] == '\0')
      return 0;
  }
  return -1;
}
