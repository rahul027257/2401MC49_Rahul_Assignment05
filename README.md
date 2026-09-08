# Assignment 5: Process Synchronization in xv6

**Name:** R.Rahul 
**Roll Number:** 2401MC49

## 1. General Design & Shared Memory Setup
Since xv6 processes do not share memory by default after a `fork()`, I implemented a custom shared memory mechanism to facilitate synchronization across all four questions.

* **Shared Page Mechanism (`shm_get`):** I added a new system call `shm_get()` in `kernel/sysproc.c`. It allocates a single physical page using `kalloc()` and maps it to a fixed user-space virtual address (`0x30000000`) using `mappages()`. 
* **Parent-Child Sharing:** Because xv6's `fork()` does not copy page table entries above the process size (`p->sz`), the child process must also call `shm_get()` immediately after forking. The kernel uses a `static` pointer to ensure the exact same physical page is mapped into both the parent's and child's page tables.
* **Memory Cleanup:** To prevent kernel panics (`freewalk: leaf`) during process exit, I added `uvmunmap(pagetable, 0x30000000, 1, 0);` inside `proc_freepagetable()` in `kernel/proc.c`.
* **Synchronization Primitives:** Instead of creating new kernel-level semaphores, I used **user-space spinlocks** (busy-waiting). A critical design decision was marking all shared variables as `volatile`. Because xv6 is compiled with the `-O` (optimize) flag, the compiler caches variables in CPU registers. The `volatile` keyword forces the compiler to read the actual RAM on every iteration, allowing processes to see updates made by other processes without OS intervention.

---

## 2. Question 1: Mutual Exclusion using Peterson's Algorithm
* **Implementation:** I created `user/peterson.c` which forks one child. The shared memory contains `flag[2]`, `turn`, and `shared_counter`. 
* **Logic:** Each process runs a loop of exactly 10 iterations. They set their flag, yield the turn, and busy-wait using the exact Peterson's condition (`while (flag[other] == 1 && turn == other)`). 
* **Verification:** The output log confirms that no two "in CS" prints are interleaved. The `shared_counter` reaches exactly **20** (10 iterations × 2 processes), proving mutual exclusion and zero lost updates.

---

## 3. Question 2: Producer-Consumer Problem (Bounded Buffer)
* **Implementation:** I created `user/prodcons.c` with a configurable circular buffer of size **5**. 
* **Logic:** The shared memory contains the buffer array, `in`/`out` pointers, a `count` variable (acting as the state for `empty` and `full` semaphores), and a `mutex` variable. 
* **Blocking Behavior:** The producer busy-waits when `count >= 5` (buffer full), and the consumer busy-waits when `count <= 0` (buffer empty). The `mutex` ensures only one process modifies the buffer pointers at a time.
* **Verification:** The producer generates integers 1 to 20. The output log shows the producer blocking when the buffer is full, the consumer blocking when empty, and the final count reaching exactly **0** with no lost or duplicated items.

---

## 4. Question 3: Readers-Writers Problem
* **Implementation:** I created `user/readwrite.c` implementing the **First Readers-Writers problem (Readers Priority)**.
* **Logic:** I spawned **3 reader processes** and **2 writer processes** using `fork()`. A `read_count` variable tracks active readers, protected by a `mutex`. The first reader to enter acquires a `writelock` (blocking all writers). Subsequent readers can enter concurrently. When the last reader exits, it releases the `writelock`.
* **Verification:** The output log demonstrates multiple readers accessing `shared_data` simultaneously (e.g., `active_readers=3`), while writers get strictly exclusive access. The final data reaches exactly **10** (2 writers × 5 iterations).

---

## 5. Question 4: Dining Philosophers Problem (Deadlock Avoidance Write-Up)

**Strategy Used:** Resource Ordering (Lock Ordering)

**Explanation of the Technique and Why it Prevents Deadlock:**
The Dining Philosophers problem is prone to deadlock if all 5 philosophers simultaneously pick up their left fork and wait indefinitely for their right fork. This creates a "Circular Wait," which is one of the four necessary Coffman conditions for deadlock.

To prevent this, I implemented the **Resource Ordering** strategy. Instead of every philosopher blindly picking up their "left" fork and then their "right" fork, the code calculates the numerical IDs of the two forks they need and **always picks up the lower-numbered fork first**, followed by the higher-numbered fork. 

For example, Philosopher 0 needs forks 0 and 1 (picks 0, then 1). However, Philosopher 4 needs forks 4 and 0. Under normal rules, they would pick 4 then 0. Under Resource Ordering, Philosopher 4 picks fork 0 first, then fork 4. 

**Why it works:** By enforcing a strict global ordering on resource acquisition, we mathematically eliminate the Circular Wait condition. Philosopher 0 and Philosopher 4 will never simultaneously hold their "first" forks and wait for each other. At least one philosopher (in this case, Philosopher 4) will always be able to acquire both forks, eat, and release them. This guarantees forward progress, prevents permanent hangs, and ensures no philosopher starves for the duration of the test.

**Verification:** The output log shows all 5 philosophers successfully cycling through THINKING → HUNGRY → EATING → THINKING for 3 complete cycles without any permanent deadlock.

---

## 6. How to Build and Run

**1. Compile the kernel:**
```bash
make clean
make qemu
