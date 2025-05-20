# CSC 501 - Spring 2025

## PA1: Process Scheduling

---

### 1. Objective

The objectives of this lab are to get familiar with the concepts of process management like:

- Process priorities  
- Scheduling  
- Context switching

---

### 2. Readings

Study the Xinu source code in the `sys/` directory, especially:

- `create.c` – process creation  
- `resched.c`, `resume.c`, `suspend.c` – scheduling and context switching  
- `kill.c` – process termination  
- `chprio.c` – change process priority  
- `ready.c`, `initialize.c` – related utilities and initialization

---

### 3. What to Do

Start with `csc501-lab0.tgz`, rename it to `csc501-lab1`.

You need to implement two new scheduling policies to avoid **starvation** in Xinu. Remember: the **NULL process** should only run when no other ready process exists.

> Valid priority range: `0–99` (99 = highest)

---

#### 3.1 Aging-Based Scheduler

- Increase priority of **waiting** processes by 2 on every `resched()` call.
- Use round-robin when priorities are equal.

**Example:**

Assume three processes:
- P1 (priority 1)  
- P2 (priority 2)  
- P3 (priority 4, running)

| Call | Ready Queue (after increment) | Running | Scheduled | New Ready Queue |
|------|-------------------------------|---------|-----------|------------------|
| 1st  | P1(3), P2(4)                  | P3(4)   | P2(4)     | P1(3), P3(4)     |
| 2nd  | P1(5), P3(6)                  | P2(4)   | P3(6)     | P1(5), P2(4)     |

---

#### 3.2 Linux-like Scheduler (based on Linux 2.2)

- Uses **epochs**, where each process gets a time quantum (`counter`) per epoch.
- If a process uses all of its quantum:  
  `quantum = priority`  
- If a process has leftover quantum:  
  `quantum = floor(counter / 2) + priority`
- Scheduling priority is determined by:  
  `goodness = counter + priority`
- Round-robin is used for equal goodness.

> Priority changes using `create()` or `chprio()` take effect in the **next** epoch.

**Example:**

Quantum:
- P1 = 10, P2 = 20, P3 = 15  
- Epoch = 45

Valid schedule:
```
P2(20) → P3(15) → P1(10)
```

Invalid schedule:
```
P2(20) → P3(15) → P2(20) → P1(10)
```

---

#### 3.3 Required Functions

```c
void setschedclass(int sched_class);
int getschedclass();
```

```c
#define AGESCHED    1
#define LINUXSCHED  2
```

---

### 4. Files of Interest

- `create.c`
- `resched.c`
- `resume.c`
- `suspend.c`
- `ready.c`
- `proc.h`
- `kernel.h`

Test files:
- `test1.c`, `test2.c`, `test3.c`, `test4.c`
- You may also use `testmain.c` for your own tests

---

### 5. Expected Results

#### Aging-Based Scheduler

Processes A (10), B (20), C (30) increase a variable during CPU time.

**Expected ratio (approximately 1:1:1):**
```
Start... B
Start... C
Start... A

Test Result: A = 110709, B = 106170, C = 110942
```

#### Linux-like Scheduler

Processes A (5), B (50), C (90) print characters over time.

**Sample Output:**
```
MCCCCCCCCCCCCCBBBBBBBMMMACCCCCCCCCCCCCBB
BBBBBMMMACCCCCCCCCCCCBBBBBBBMMMACCCCCCCC
CCCCBBBBBBBMMABBBBBBBMMMABBBBBBBMMMABBBB
BBBMMMBMMAMMMAMMMMMAMMMAMMMAMMMMMMAMMAMM
MMMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

---

### 6. Additional Questions

Write your answers in a plain text file: `sys/Lab1Answers.txt`

#### Questions:

1. What are the **advantages and disadvantages** of:
    - Aging-Based Scheduler  
    - Linux-like Scheduler  
    - Round Robin Scheduler (default Xinu)

2. When does each scheduler run the **NULL process**?

3. Suggest **two improvements** to the Aging-Based Scheduler that improve fairness while maintaining the idea of increasing waiting process priority.

---
