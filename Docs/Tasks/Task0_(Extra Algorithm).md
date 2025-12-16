### **Task: Implement and Validate an Additional Scheduling Algorithm**

#### **Objective**
1. **Explore and implement** a **new scheduling algorithm** (preemptive or non-preemptive) to complement our existing suite:
   - Round Robin (RR)
   - Preemptive Highest Priority First (HPFS)
   - Shortest Job Next (SJN)

2. **Ensure each algorithm** (including the new one) outputs a `.perf` file with **reliable, accurate metrics** for performance comparison.

---

#### **Algorithm Suggestions**
Choose **one** of the following (or propose another):

- **Preemptive Shortest Job First (SJF)**: Preempts the current job if a shorter one arrives.
- **Multilevel Feedback Queue (MLFQ)**: Uses multiple queues with varying priorities and time slices.
- **First-Come, First-Served (FCFS)**: Non-preemptive, simple but may cause convoy effect.
- **Lottery Scheduling**: Randomly assigns CPU time via lottery tickets.

**Consider**:

- **Preemptive vs. Non-preemptive**: Preemptive algorithms are more complex but often more responsive.
- **Fairness vs. Efficiency**: Balance between minimizing waiting time and maximizing throughput.

---

#### **Action Items**

1. **Research and Select an Algorithm**:
   - Review the theory and pseudocode for your chosen algorithm.
   - Document its **pros/cons** and **expected use cases** in [`Algorithms`](../Algorithms/).

2. **Implement the Algorithm**:
   - Add the new scheduler to the codebase.
   - Ensure it integrates with the existing simulation framework.
   - Use **modular design** for easy comparison with other algorithms.

3. **Generate `.perf` Files**:
   - For **each algorithm**, log the following metrics to a `.perf` file:
     ```
     CPU utilization = XX.XX%
     Avg WTA (Waiting Time Average) = XX.XX
     Avg Waiting = XX.XX
     Std WTA = XX.XX
     ```
   - **Validation**: Cross-check calculations manually for small test cases.

4. **Test and Compare**:
   - Run all algorithms (including the new one) on the **same set of test cases**.
   - Compare `.perf` files to analyze trade-offs (e.g., throughput vs. fairness).

---

#### **Example `.perf` File Format**
```
CPU utilization = 89.13%
Avg WTA = 1.00
Avg Waiting = 80.60
Std WTA = 0.00
```

---
**Question for the team**:

- Which algorithm should we prioritize for implementation? (SJF, MLFQ, FCFS, or another?)
- Should we create a script to automate `.perf` file generation and comparison?