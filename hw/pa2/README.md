# PA2: Readers/Writer Locks with Priority Inheritance

**CSC 501: Spring 2025**

## 1. Introduction

In this assignment, you will implement **readers/writer locks** in XINU, along with a **priority inheritance mechanism** to prevent the **priority inversion** problem.

Please download and untar a fresh version of the XINU for QEMU source at: `csc501-lab2-qemu.tgz`.

### Background

- Readers/writer locks allow multiple readers to share a lock, but writers require exclusive access.
- You will **not** modify XINU's existing semaphore implementation. Instead, you'll build your lock system on top of it.
- You will also fix existing issues with XINU semaphores, including:
  - No distinction between read/write access.
  - Improper behavior when deleting a semaphore with waiting processes.
  - Lack of handling for priority inversion.

## 2. Interfaces to Implement

### Basic Lock Operations

You must implement the following functions in corresponding source files:

- `linit()` (in `linit.c`)
- `lcreate()` (in `lcreate.c`)
- `ldelete()` (in `ldelete.c`)
- `lock()` (in `lock.c`)
- `releaseall()` (in `releaseall.c`)

Use a header file `lock.h` to define constants and data structures:
- `READ`, `WRITE`, `DELETED`, etc.
- Set `#define NLOCKS 50`

### Function Details

#### `int lcreate(void)`
Creates a new lock. Returns a lock descriptor, or `SYSERR` if none are available.

#### `int ldelete(int lockdescriptor)`
Deletes the specified lock. Any waiting process should return `DELETED`, not `OK`.

#### `int lock(int ldes1, int type, int priority)`
Acquires a lock with the given type and wait priority.

#### `int releaseall(int numlocks, ...)`
Releases multiple locks simultaneously. Uses variable arguments (not `va_list`), and directly accesses the stack like in `create.c`.

### Lock Deletion Rules

- Deleted locks must not be reused inappropriately.
- A process waiting on a deleted lock must return `DELETED`.
- A process should not acquire a new lock that happens to reuse an old lock ID.

### Locking Policy

- Readers should **not** be blocked unless:
  1. A writer already holds the lock.
  2. A writer with **equal or higher priority** is waiting.
- When releasing, the lock should be granted to the highest-priority waiting process (or group of readers, if applicable).
- If a reader and a writer have equal priority, and the writer’s wait time is no more than 0.5s longer than the reader’s, **prefer the writer**.
- All eligible readers with priority greater than any waiting writer should be granted the lock together.

### Lock Wait Priority

- `lock()` allows a process to specify a **wait priority**, which is **independent** from scheduling priority.
- Larger value = higher priority.
- Processes should be inserted into the wait queue in order of wait priority.

### Release Policy

- `releaseall()` must release only the locks that the calling process actually holds.
- If any listed lock is not held, return `SYSERR` but still release the others.

## 3. Priority Inheritance

### Concept

If a low-priority process holds a lock needed by a high-priority process, it should temporarily **inherit** the higher priority to prevent priority inversion.

Maintain this invariant:
prio(p) = max(prio(p_i)) for all p_i waiting on any lock held by process p

### Transitive Inheritance

If:
- A (prio 10) holds L1
- B (prio 20) holds L2
- A waits on L2
- C (prio 30) waits on L1

Then **both A and B** must inherit priority 30.

### Suggested Implementation

#### In Process Table
- `pprio`: original priority
- `pinh`: inherited priority (0 if none)
- `lock_list[]`: locks held
- `waiting_lock_id`: lock the process is waiting for, or -1

#### In Lock Table
- `lprio`: max priority among waiters
- `holding_list[]`: processes holding this lock

#### When Locking
- If lock is held and the waiter’s priority > holder’s, elevate holder’s priority (`pinh`)
- Handle transitivity recursively

#### When Releasing
- Reset process priority to max of all waiters across currently held locks

#### When `chprio`, `kill`, etc.
- Recalculate lock wait priorities and adjust holding processes' `pinh` as needed

## 4. Additional Tasks

Write your answers to the following questions in `TMP/Lab2Answers.txt`:

1. Describe another approach (besides priority inheritance) to solve priority inversion.
2. Design a test case (in `task1.c`) to demonstrate the priority inversion problem and how your implementation solves it.
   - Compare XINU semaphore vs your lock system
3. Examine the pseudo-code in `task2_sync_issue.c`:
   - Identify a potential synchronization bug
   - Explain why it occurs

---

Place all files (`Lab2Answers.txt`, `task1.c`, etc.) in the `TMP/` directory for submission.
