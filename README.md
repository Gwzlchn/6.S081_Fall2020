# Fall2020/6.S081 实验笔记（三） Lab3: Page Tables





## 实验一： `Print a page table `

第一个实验要求打印进程1的页表，结果应该如下所示：

做实验之前可以先看一下 [xv6-book](https://pdos.csail.mit.edu/6.S081/2020/xv6/book-riscv-rev1.pdf) 的第三章，其中介绍了 `RISC-V` 的页表结构。  
对于每个进程，其第一级页表地址位于 `proc` 结构体的 `pagetable` 字段。页表的每一项称为 `PTE, page table entries`, 每级页表有512 个`PTE`。


题目要求打印进程1的页表，首先在 `exec.c` 中的 `exec` 函数中插入 `if(p->pid==1) vmprint(p->pagetable)` 。

其次，在`vm.c` 中实现 `vmprint` 函数。为了方便递归，额外需要一个辅助函数。  
第一行打印 进程1 第一级页表的地址。之后每行打印页表的有效`PTE`，以及这一项`PTE` 对应的物理地址。(`PTE` 低10位为一些Flags，所以 `PTE[53:10]` 这44位表示下一级页表的页号`(PPN, Physical page number)`，而Sv39 这个RISC-V 中，物理地址56位，低12位补0才能得到物理页号对应的物理地址。综上 `PTE - > PA => (PTE>>10)<<12)`


将上述思路写成代码，如下所示：

注意，用`%p`打印的 `pte` 并不是 `pte` 的地址，而是 `pte` 本身的值，下一级页表的起始地址为 `(PTE>>10)<<12)`。

```cpp
int
vmprint_helper(pagetable_t pagetable, uint depth)
{
  
  for(int i=0;i<512;i++){
    // dereference of the pionter of page, equals to pagetable[i]
    pte_t pte = *(pagetable + i);
    if((pte & PTE_V) ){
      for(int j=0;j<depth;j++) printf(".. ");
      printf("..%d: pte %p pa %p\n",i, pte, PTE2PA(pte));
      if((pte & (PTE_R|PTE_W|PTE_X)) == 0)
        vmprint_helper((pagetable_t)PTE2PA(pte), depth+1);
    }
  }
  return 0;
}

// LAB3 print a page table
int 
vmprint(pagetable_t pagetable)
{
  printf("page table %p\n", pagetable);
  vmprint_helper(pagetable, 0);
  return 0;
}
```

最后输出结果如下所示：
```
page table 0x0000000087f6e000
..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
.. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
.. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000       // for text and data sec
.. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000       // for guard page
.. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000       // for user stack
..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
.. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
.. .. ..510: pte 0x0000000021fdd807 pa 0x0000000087f76000     // for trapframe
.. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000     // for trampoline
```
接下来我们分析，这五个页都是什么？（参考 xv6-book 中的 Chap3, section 3.6）

最顶端的两个页，在 `exec` 函数中 `pagetable = proc_pagetable(p)` 处申请的两个页。用处如注释所示。


最底端的第一个页，在 `exec` 函数中 ` if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)`  处申请，目的是加载`exec`要执行程序的代码段和数据段(对于进程1，一个页就够了，但不是所有进程的代码段和数据段均占用一个页)。


下面的代码申请了最底端的第二页、第三页。
```cpp
 kerenel/exec.c


  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  sz = sz1;
  uvmclear(pagetable, sz-2*PGSIZE);
  sp = sz;
  stackbase = sp - PGSIZE;
```
可以看到，第二页用作 `guard page`, 页表项`PTE`中的 `PTE_U` 为0，用户进程无权限访问。

第三页用作用户栈 `stack`， 可以看到，栈大小为`PGSIZE`, 栈顶为`sp` ,栈基址为`stackbase`，栈是从高地址向低地址增长的。如果用户栈溢出，则会访问到`guard page`的地址，进而会触发一个`page-fault` 例外。