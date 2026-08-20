# PrismScheduler - System Workflow

## Overview

This is a **process scheduling simulator** that emulates operating system scheduling algorithms. It creates a simulated OS environment with three main components: a clock process, a scheduler process, and user processes, all communicating through Inter-Process Communication (IPC).

## System Architecture

```
┌─────────────────┐
│ Process         │
│ Generator       │ (Main Coordinator)
└────────┬────────┘
         │
    ┌────┴────┬──────────────┐
    │         │              │
    ▼         ▼              ▼
┌───────┐ ┌──────────┐ ┌──────────┐
│ Clock │ │Scheduler │ │Processes │
│Process│ │ Process  │ │(Dynamic) │
└───────┘ └──────────┘ └──────────┘
    │         │              │
    └────┬────┴──────────────┘
         │
    IPC Mechanisms:
    - Shared Memory (Clock)
    - Message Queues (Processes)
    - Signals (Process Control)
```

## Execution Flow

### Phase 1: Initialization

1. **Process Generator Starts** (`process_generator.c`)
   - Reads process definitions from `processes.txt`
   - Each process has: ID, arrival time, runtime, priority
   - Prompts user to select scheduling algorithm:
     - **HPF** (Highest Priority First - Preemptive)
     - **SJN** (Shortest Job Next - Preemptive SRTN)
     - **RR** (Round Robin)
     - **MLFQ** (Multi-Level Feedback Queue)
   - If RR selected, asks for time quantum

2. **IPC Setup**
   - Creates message queue using `ftok()` and `msgget()`
   - This queue will carry process information to the scheduler

3. **Clock Process Creation** (`clk.c`)
   - Forks and executes `clk.out`
   - Clock creates shared memory segment
   - Writes shared memory key to `.osclock_key` file
   - Increments integer in shared memory every second (simulated time)
   - **Critical**: Must start before other processes

4. **Clock Initialization**
   - Process generator calls `initClk()` from `headers.h`
   - Reads key from `.osclock_key` file
   - Attaches to shared memory
   - Now can read simulated time with `getClk()`

5. **Scheduler Process Creation** (`scheduler.c`)
   - Forks and executes `scheduler.out`
   - Passes three arguments:
     - Algorithm number
     - Quantum value
     - Message queue ID
   - Scheduler also calls `initClk()` to access time
   - Opens log files: `scheduler.log` and `scheduler.perf`
   - Initializes appropriate queue structure based on algorithm

### Phase 2: Process Submission

6. **Process Arrival Loop** (in `process_generator.c`)
   - Continuously monitors simulated clock
   - When `currentTime >= process.arrivalTime`:
     - Packs process data into Message structure
     - Sends message (type 1) to scheduler via message queue
     - Message contains: id, arrivalTime, runtime, priority
   - After all processes sent, sends termination message (type 2)

### Phase 3: Scheduling Execution

7. **Scheduler Main Loop** (`scheduler.c`)
   - Runs until: all processes arrived AND queues empty AND no running process
   - Each iteration:
     - Gets current time from shared memory
     - Calculates elapsed time since last iteration
     - Receives new processes from message queue
     - Updates running process remaining time
     - Handles process completion
     - Handles quantum expiration (RR/MLFQ)
     - Selects next process if CPU idle
     - Updates waiting times for ready processes

8. **Receiving Processes** (`receiveProcesses()`)
   - Non-blocking receive on message queue (type 1)
   - Creates PCB (Process Control Block) for each process
   - Initializes all PCB fields
   - Adds process to appropriate queue:
     - HPF/SJN/RR: Single ready queue
     - MLFQ: Queue 0 (highest priority)

9. **Process Selection** (`selectNextProcess()`)
   - Routes to algorithm-specific selector:
     - **HPF**: `selectHPF()` - finds lowest priority number
     - **SJN**: `selectSJN()` - finds shortest remaining time
     - **RR**: `selectRR()` - FIFO from ready queue
     - **MLFQ**: `selectMLFQ()` - checks queues 0→1→2
   - Removes selected process from queue
   - Calls `startProcess()` or `resumeProcess()`

### Phase 4: Process Execution

10. **Starting a Process** (`startProcess()`)
    - Forks new process
    - Child executes `process.out` with remaining time argument
    - Parent stores child PID in PCB
    - Sets process state to RUNNING
    - Records start time
    - Logs "started" event

11. **Process Simulation** (`process.c`)
    - Receives remaining time as argument
    - Performs busy-wait CPU work (infinite loop with volatile counter)
    - Scheduler controls lifetime via signals:
      - SIGSTOP: Pause execution
      - SIGCONT: Resume execution
      - SIGKILL: Terminate

12. **Time Accounting**
    - Each scheduler iteration:
      - Decrements `runningProcess->remainingTime` by elapsed time
      - Increments `runningProcess->executionTime` by elapsed time
      - Increments waiting time for ready processes

### Phase 5: Quantum Management (RR & MLFQ)

13. **Quantum Tracking**
    - `quantumCounter` accumulates elapsed time
    - When counter >= quantum and process still has work:
      - Call `stopProcess()` to send SIGSTOP
      - **RR**: Re-enqueue to back of ready queue
      - **MLFQ**: Call `handleMLFQQuantumExpired()` to demote

14. **MLFQ Specific Behavior**
    - Three queues with increasing quantums: 2, 4, 8
    - New processes start in Queue 0
    - If quantum expires: demote to next lower queue
    - If process finishes early: stays at same level (if re-enters)
    - Selection prioritizes Queue 0 > Queue 1 > Queue 2

### Phase 6: Process Completion

15. **Finishing a Process** (`finishProcess()`)
    - Triggered when `remainingTime <= 0`
    - Records finish time
    - Calculates metrics:
      - Turnaround Time = finish time - arrival time
      - WTA = Turnaround Time / runtime
    - Accumulates global statistics
    - Logs "finished" event with TA and WTA
    - Sends SIGKILL and waits for child termination

### Phase 7: Termination

16. **Scheduler Completion**
    - Loop exits when all processes finished
    - Calls `writePerformanceMetrics()`:
      - CPU Utilization = (total runtime / total time) × 100%
      - Average WTA
      - Average Waiting Time
      - Standard Deviation of WTA
    - Writes to `scheduler.perf`
    - Calls `cleanup()` and `destroyClk(true)`

17. **Process Generator Cleanup** (`clearResources()`)
    - Waits for scheduler to finish
    - Removes message queue
    - Kills clock and scheduler processes
    - Destroys shared memory
    - Removes `.osclock_key` file

## IPC Mechanisms Used

### Shared Memory
- **Purpose**: Simulated clock time
- **Key Generation**: `ftok()` with marker file
- **Access**: All processes read shared integer
- **Lifecycle**: Created by clock, destroyed by process generator

### Message Queues
- **Purpose**: Send process data to scheduler
- **Message Types**:
  - Type 1: New process arrival
  - Type 2: All processes sent (termination signal)
- **Direction**: Process generator → Scheduler (one-way)

### Signals
- **SIGSTOP**: Preempt running process
- **SIGCONT**: Resume stopped process
- **SIGKILL**: Terminate completed process
- **SIGINT**: Cleanup signal for graceful shutdown
- **SIGUSR1**: Custom signal for process finish notification (declared but not fully utilized)

## Scheduling Algorithms

### HPF (Highest Priority First)
- Preemptive priority scheduling
- Lower priority number = higher priority
- Tie-breaking: earliest arrival time
- No quantum management

### SJN (Shortest Job Next / SRTN)
- Preemptive shortest remaining time
- Dynamically selects process with minimum remaining time
- Tie-breaking: earliest arrival time
- Can preempt currently running process if shorter job arrives

### RR (Round Robin)
- Processes execute for fixed quantum
- After quantum: moved to back of queue
- Fair allocation of CPU time
- FIFO queue with rotation

### MLFQ (Multi-Level Feedback Queue)
- Three priority queues with adaptive quantums
- Queue 0 (quantum=2) > Queue 1 (quantum=4) > Queue 2 (quantum=8)
- Demote on quantum expiration
- Prevents starvation while prioritizing I/O-bound processes

## File Outputs

### `scheduler.log`
- Line-by-line event trace
- Format: `At time X process Y STATE arr A total T remain R wait W [TA ta WTA wta]`
- States: started, stopped, resumed, finished

### `scheduler.perf`
- Summary statistics after completion
- CPU utilization percentage
- Average WTA (weighted turnaround time)
- Average waiting time
- Standard deviation of WTA

## Running the System

```bash
# Generate test processes
./test_generator.out

# Run the simulation
./process_generator.out

# System automatically:
# 1. Starts clock
# 2. Starts scheduler
# 3. Submits processes at arrival times
# 4. Executes scheduling
# 5. Outputs logs and metrics
```

## Key Design Decisions

1. **Separate Clock Process**: Simulates OS time independently
2. **Shared Memory for Clock**: Efficient, read-only access pattern
3. **Message Queue for Processes**: Clean separation of generator and scheduler
4. **Signal-Based Process Control**: Mimics real OS process management
5. **PCB Array + Queue Pointers**: Efficient memory management
6. **Non-blocking Message Receive**: Allows scheduler to make progress even without new arrivals
7. **Elapsed Time Calculation**: Handles variable-speed execution gracefully

## Potential Issues & Solutions

### Race Conditions
- Clock must start before others attach
- Solution: `initClk()` waits up to 30 seconds with retry logic

### Busy Waiting
- Scheduler could spin rapidly
- Solution: `usleep(10000)` introduces 10ms delay per iteration

### Resource Cleanup
- Processes/IPC might leak on crash
- Solution: Signal handlers and explicit cleanup functions

### Time Precision
- Sleep-based clock has 1-second granularity
- Solution: Adequate for educational simulation; real OS uses hardware timers