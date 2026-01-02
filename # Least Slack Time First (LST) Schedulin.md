# Least Slack Time First (LST) Scheduling Algorithm Documentation

## 1. Introduction

Least Slack Time First (LST), also known as **Least Slack Scheduling (LSS)**, is a dynamic real-time scheduling algorithm used to decide which task should run next based on the amount of *slack time* it has left.

Slack time represents how much extra time a task has before it risks missing its deadline.

---

## 2. What Is Slack Time?

Slack time for a task is calculated as:

**Slack = (Deadline - Current Time - Remaining Execution Time)**

Meaning:

* A task with **less slack** is **closer to missing its deadline**.
* LST picks the task with the **minimum slack** to run next.

---

## 3. How LST Works (Step‑by‑Step)

1. At every scheduling decision, calculate each task's slack time.
2. Choose the task with the **least slack**.
3. Run that task until the next event (new task arrival, completion, or timer update).
4. Recalculate slack times dynamically as time changes.

This makes LST a **preemptive** and **dynamic** real-time scheduling algorithm.

---

## 4. Example

Suppose the tasks are:

| Task | Arrival | Execution | Deadline |
| ---- | ------- | --------- | -------- |
| T1   | 0       | 3         | 10       |
| T2   | 2       | 4         | 12       |
| T3   | 4       | 2         | 7        |

At time = 4:

### Slack calculations:

* **T1:** 10 - 4 - 3 = **3**
* **T2:** 12 - 4 - 4 = **4**
* **T3:** 7 - 4 - 2 = **1** ← Least slack

→ LST chooses **T3** to run.

---

## 5. Advantages

* Ensures that tasks close to missing deadlines are prioritized.
* Dynamic, adapts to system changes.
* Good for **hard real-time systems**.

## 6. Disadvantages

* Requires continuous recalculation → can be complex.
* High overhead due to preemption.
* Performance depends heavily on accurate time tracking.

---

## 7. Pseudocode

```
loop:
    update current_time
    for each task i in ready_queue:
        slack[i] = deadline[i] - current_time - remaining_time[i]
    choose task with minimum slack
    run that task for 1 time unit
    if new tasks arrive or task finishes:
        update ready_queue
end loop
```

---

## 8. Example C Implementation (Core Logic)

```
#include <stdio.h>
#include <limits.h>

struct Task {
    int id;
    int deadline;
    int remaining;
};

int main() {
    struct Task tasks[] = {
        {1, 10, 3},
        {2, 12, 4},
        {3, 7, 2}
    };

    int n = 3;
    int time = 4;  // Example current time

    int minSlack = INT_MAX;
    int chosenTask = -1;

    for (int i = 0; i < n; i++) {
        int slack = tasks[i].deadline - time - tasks[i].remaining;

        if (slack < minSlack) {
            minSlack = slack;
            chosenTask = tasks[i].id;
        }
    }

    printf("Task with least slack: T%d (Slack = %d)\n", chosenTask, minSlack);

    return 0;
}
```

---

## 9. Simulation Input Format (Suggested)

```
Enter number of tasks:
Enter arrival times:
Enter execution times:
Enter deadlines:
```

---

## 10. Conclusion

Least Slack Time First (LST) is a powerful real-time scheduling algorithm that selects the task closest to missing its deadline. While it offers excellent deadline handling, it comes with higher computational cost and requires continuous monitoring.
