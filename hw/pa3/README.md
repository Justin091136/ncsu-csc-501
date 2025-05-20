# CSC501: Spring 2025  
## PA3: Demand Paging

---

## 1. Introduction

Demand paging allows a program to access a larger address space than available physical memory using backing stores. This project involves implementing demand paging in Xinu, including memory mapping, page fault handling, backing store management, and page replacement policies.

---

## 2. Goal

Implement the following system calls and the infrastructure to support:

- `xmmap`
- `xmunmap`
- `vcreate`
- `vgetmem`
- `vfreemem`
- `srpolicy`

---

## 3. System Calls

```c
SYSCALL xmmap(int virtpage, bsd_t source, int npages);
SYSCALL xmunmap(int virtpage);
SYSCALL vcreate(int *procaddr, int ssize, int hsize, int priority, char *name, int nargs, long args);
WORD *vgetmem(int nbytes);
SYSCALL vfreemem(void *block_ptr, int size_in_bytes);
SYSCALL srpolicy(int policy);
```

---

## 4. System Design

### 4.1 Memory & Backing Store

- **Page Size:** 4096 bytes  
- **Backing Stores:** 16 total, each up to 128 pages  
- **Reserved Frames for Backing Store:** Pages 2048–4095  
- **User Virtual Memory:** Starts at page 4096

### 4.2 Data Structures

- **Backing Store Map:**  
  `{ pid, vpage, npages, store }`

- **Inverted Page Table:**  
  `{ frame, pid, vpno }`

### 4.3 Page Table Management

- Global page tables map the first 16MB  
- Per-process page directories  
- Page tables allocated on-demand  
- Freed when empty

---

## 5. Page Fault Handling

### 5.1 ISR

Handle interrupt 14 using `set_evec()`. Steps:

1. Get faulted address
2. Validate mapping
3. Allocate page table if missing
4. Allocate a frame
5. Read from backing store
6. Update page table and inverted page table

### 5.2 Page Replacement

#### Second-Chance (SC)

- Circular queue
- Reference bit checked and cleared
- Replaces page on second chance

#### FIFO

- Replace oldest page

Set using:

```c
srpolicy(SC);   // default
srpolicy(FIFO); // switch to FIFO
```

---

## 6. Process Management

### Creation

- `vcreate()` allocates private heap (and optionally stack)
- Initializes page directory

### Destruction

- Writes dirty pages back
- Frees mappings and page directory
- Releases heap backing store

### Context Switch

- Load new process’ page directory to CR3 (`PDBR`)

---

## 7. Initialization

- Setup segment limits in `i386.c`
- Reserve pages 2048–4095 for backing store
- Create global page tables (pages 0–4095)
- Install page fault handler
- Enable paging

---

## 8. Testing Example

```c
void A() {
    int *x = vgetmem(1000);
    *x = 100;  // causes page fault
    x++;
    *x = 200;
    vfreemem(--x, 1000);
}

main() {
    vcreate(A, 2000, 100, 20, "A", 0);
}
```

### `xmmap()` usage example

```c
get_bs(4, 100);
xmmap(7000, 4, 100);
char *x = (char*)(7000 * 4096);
*x = 'Y';

xmmap(6000, 4, 100);
x = (char*)(6000 * 4096);
char temp = *x;  // reads 'Y' from another process
```

---

## 9. Submission Checklist

✅ `xmmap` / `xmunmap`  
✅ `vcreate` with private heap  
✅ `vgetmem` / `vfreemem`  
✅ Backing store support  
✅ Demand paging  
✅ Page replacement: SC, FIFO  
✅ Page table on-demand allocation & cleanup  

---

## 10. Debugging Tips

- Use `#define` in `evec.c` for stack trace  
- Use `nm`, `objdump -d xinu.elf > xinu.dis`  
- Stack pointer errors may cause reboots

---

## 11. FAQs

- **Virtual page number:** Top 20 bits of virtual address  
- **Mapping entries:** Max 16  
- **Global page tables:** Pages 0–4095  
- **Page table/page directory location:** Pages 1024–2047  
- **xmmap usage:** Like `mmap()` for backing store access  
- **read_bs / write_bs:** Safe in ISR  
- **Replacement policy test:** Modify `NFRAMES` in `paging.h`  

---

© CSC501 Spring 2025 · North Carolina State University
