# Task: Building a Robust Test Suite for the OS Scheduler

## 1. Objective

Your goal is to implement a comprehensive unit and integration testing suite for the provided OS simulation. You must ensure the scheduling algorithms, IPC mechanisms, and process synchronization logic are functionally correct and resilient to edge cases.

## 2. Targeted Areas for Testing

### A. Scheduler Logic (Unit Tests)

The scheduling logic is currently tightly coupled with the main loop. You are required to:

* **Refactor for Testability:** Isolate the `selectNextProcess()` logic so it can be tested without forking real processes.
* **Algorithm Validation:** * **HPF:** Assert that the process with the highest priority is always selected, even if a lower-priority process arrived earlier.
* **SJN:** Assert that the process with the shortest total runtime is selected next.
* **Round Robin:** Verify the "Round Robin" property: when , the process is preempted and moved to the tail of the `readyQueue`.


* **Metric Accuracy:** Validate the math for Turnaround Time (TA) and Weighted Turnaround Time (WTA).
* 
* 



### B. IPC & Shared Memory (Integration Tests)

Test the "plumbing" of the system:

* **Clock Synchronization:** Verify that the `process.out` correctly reads the shared memory incremented by `clk.out`.
* **Message Queue Integrity:** Test the `process_generator`'s ability to handle the "End of File" signal (Message Type 2). Ensure no processes are "lost" if they arrive simultaneously.
* **Resource Cleanup:** Write a test that triggers `SIGINT` and verifies that `shmid` and `msgqid` are properly removed from the system using `ipcs` commands.

### C. Edge Case Scenarios

Construct a `processes.txt` file to test:

1. **Zero-time arrival:** Multiple processes arriving at `Time 0`.
2. **Long gaps:** Large idle times between process arrivals.
3. **Maximum Load:** Stress test the system with `MAX_PROCESSES`.

---

## 3. Recommended Tools

* **Testing Framework:** [Unity](http://www.throwtheswitch.org/unity) or [Check](https://libcheck.github.io/check/) (C Unit Testing Frameworks).
* **Mocking:** Create a "Mock Clock" that allows you to manually increment time without waiting for `sleep(1)`.
* **Memory Leak Detection:** Run your test suite through **Valgrind** to ensure no `QueueNode` or `PCB` allocations are leaked during scheduling.

---

## 4. Deliverables

1. **`test_suite.c`**: A standalone file containing unit tests for `queue.h` and the selection algorithms.
2. **Mock Objects**: A version of `headers.h` that allows for simulated IPC.
3. **Automated Test Script**: A bash script that:
* Compiles all modules.
* Runs the test suite.
* Reports a **Pass/Fail** percentage.



---

## 5. Success Criteria

* **Code Coverage:** At least 80% of the logic in `scheduler.c` and `process_generator.c` should be exercised by tests.
* **Determinism:** Tests must produce the same result regardless of CPU load.
* **Cleanup:** The system must not leave orphaned shared memory segments or message queues after a test failure.
