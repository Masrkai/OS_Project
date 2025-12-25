# Task: Implementation of Advanced Scheduling Algorithms

## 1. Overview

The goal of this task is to extend our current OS scheduler framework. Currently, the system supports basic algorithms (**HPF**, **SJN**, and **RR**). To improve system throughput and reduce starvation, you are required to implement three advanced scheduling strategies: **Shortest Remaining Time Next (SRTN)**, **Highest Response Ratio Next (HRRN)**, and **Multi-Level Feedback Queue (MLFQ)**.

---

## 2. Technical Specifications

### 2.1 Shortest Remaining Time Next (SRTN)

SRTN is the **preemptive** counterpart of SJN. The scheduler must re-evaluate the optimal process every time a new process arrives in the system or the clock ticks.

* **Logic:** Select the process with the smallest `remainingTime`.
* **Preemption Trigger:** If a newly arrived process has a `runtime` less than the current process’s `remainingTime`, the current process must be context-switched out.
* **Data Structure:** Requires a sorted list or a min-heap based on `remainingTime`.

### 2.2 Highest Response Ratio Next (HRRN)

HRRN is a **non-preemptive** strategy designed to balance the favorability of short jobs while preventing the starvation of long jobs by accounting for their "age" in the ready queue.

* **Formula:** At each scheduling decision, calculate the Response Ratio () for all processes in the `readyQueue`:


* : Waiting time (current time - arrival time).
* : Service time (`runtime`).


* **Logic:** The process with the highest  is selected to run next.

### 2.3 Multi-Level Feedback Queue (MLFQ)

MLFQ is a dynamic scheduling algorithm that adjusts process priority based on its execution behavior.

* **Architecture:** Define three priority levels:
* **Queue 0 (Q0):** Round Robin with .
* **Queue 1 (Q1):** Round Robin with .
* **Queue 2 (Q2):** FCFS (First Come First Served).


* **Rules:**
1. New processes enter **Q0**.
2. If a process exhausts its quantum in **Q0**, move it to **Q1**.
3. If it exhausts its quantum in **Q1**, move it to **Q2**.
4. Processes in **Q1** only run if **Q0** is empty; **Q2** only runs if **Q0** and **Q1** are empty.



---

## 3. Implementation Requirements

### 3.1 Structural Changes

You must modify `schedule.h` to include the following function prototypes:

```c
PCB* selectSRTN(); // Preemptive: Check every tick/arrival
PCB* selectHRRN(); // Non-preemptive: Check when a process finishes
PCB* selectMLFQ(); // Preemptive: Handle multi-queue promotion/demotion

```

### 3.2 Metric Tracking

For each algorithm, ensure the following fields in the `PCB` are updated accurately to maintain the integrity of the `writePerformanceMetrics()` function:

* `waitingTime`: Total time spent in the `READY` state.
* `startTime`: The clock cycle the process first moved to `RUNNING`.
* `finishTime`: The clock cycle the process reached `FINISHED`.
* **Weighted Turnaround Time (WTA):** 

---

## 4. Evaluation Criteria

Success will be measured by generating the log files and comparing the following:

1. **Average Weighted Turnaround Time:** Lower is better.
2. **CPU Utilization:** Percentage of time the CPU was not IDLE.
3. **Starvation Prevention:** (Specifically for HRRN and MLFQ) ensuring no process remains in the queue indefinitely.

**Would you like me to provide the specific C implementation for the `selectMLFQ()` logic, including the management of the three sub-queues?**