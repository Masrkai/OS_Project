# Functions & Programming Practices Reference

## Inter-Process Communication (IPC) Functions

### Shared Memory

#### `shmget(key, size, flags)`
**Purpose**: Allocate shared memory segment

**Parameters**:

- `key`: Unique identifier (from `ftok()` which generate an IPC key)
- `size`: Memory size in bytes
- `flags`: Creation flags (`IPC_CREAT | 0644` = create with permissions)

**Returns**: Shared memory ID or -1 on error

**Usage in Code**:
```c
// In clk.c - create 4 bytes for integer
shmid = shmget(key, 4, IPC_CREAT | 0644);
```

#### `shmat(shmid, addr, flags)`
**Purpose**: Attach shared memory to process address space

**Parameters**:

- `shmid`: Shared memory ID
- `addr`: Desired address (NULL = let OS choose)
- `flags`: Attachment options (0 = read-write)

**Returns**: Pointer to shared memory or -1

**Usage in Code**:
```c
shmaddr = (int *)shmat(shmid, (void *)0, 0);
```

#### `shmdt(addr)`
**Purpose**: Detach shared memory from process

**Practice**: Always detach before process exits to prevent memory leaks

#### `shmctl(shmid, cmd, buf)`
**Purpose**: Control operations on shared memory

**Common Commands**:

- `IPC_RMID`: Remove shared memory segment

**Usage in Code**:
```c
shmctl(shmid, IPC_RMID, NULL); // Destroy shared memory
```

#### `ftok(path, proj_id)`
**Purpose**: Generate unique key from file path and project ID

**Parameters**:

- `path`: Existing file path
- `proj_id`: Project identifier (single char)

**Returns**: Unique key_t value

**Practice**: File must exist before calling ftok()

**Usage in Code**:
```c
key_t key = ftok(".osclock_marker", 'C');
```

### Message Queues

#### `msgget(key, flags)`
**Purpose**: Create or access message queue

**Returns**: Message queue ID

**Usage in Code**:
```c
msgqid = msgget(msgkey, IPC_CREAT | 0644);
```

#### `msgsnd(msgqid, msgp, size, flags)`
**Purpose**: Send message to queue

**Parameters**:

- `msgqid`: Queue ID
- `msgp`: Pointer to message structure
- `size`: Size of message data (excluding mtype)
- `flags`: 0 = blocking, IPC_NOWAIT = non-blocking

**Practice**: Message structure must start with `long mtype`

**Usage in Code**:
```c
Message msg;
msg.mtype = 1;
msg.process = processes[i];
msgsnd(msgqid, &msg, sizeof(Process), 0);
```

#### `msgrcv(msgqid, msgp, size, type, flags)`
**Purpose**: Receive message from queue

**Parameters**:

- `type`: Message type to receive (0 = any, >0 = specific, <0 = range)
- `flags`: IPC_NOWAIT = return immediately if no message

**Returns**: Number of bytes received, or -1

**Practice**: Use IPC_NOWAIT for polling pattern

**Usage in Code**:
```c
// Non-blocking receive of type 1 messages
while (msgrcv(msgqid, &msg, sizeof(msg.process), 1, IPC_NOWAIT) != -1) {
    // Process message
}
```

#### `msgctl(msgqid, cmd, buf)`
**Purpose**: Control message queue

**Commands**:

- `IPC_RMID`: Remove queue

**Usage in Code**:
```c
msgctl(msgqid, IPC_RMID, NULL);
```

## Process Management Functions

### `fork()`
**Purpose**: Create child process (exact copy of parent)

**Returns**:

- Child process: 0
- Parent process: child's PID
- Error: -1

**Practice**: Check return value to determine which process you're in

**Usage in Code**:
```c
pid_t pid = fork();
if (pid == 0) {
    // Child process code
    execl("./program.out", "program.out", arg1, NULL);
} else if (pid > 0) {
    // Parent process code
    // Save pid for later control
}
```

### `execl(path, arg0, arg1, ..., NULL)`
**Purpose**: Replace current process image with new program

**Parameters**:

- `path`: Path to executable
- `arg0`: Program name (by convention)
- `arg1...`: Arguments to program
- `NULL`: Terminator (required!)

**Practice**: Only returns on error (doesn't return on success)

**Usage in Code**:
```c
execl("./process.out", "process.out", remainingTimeStr, NULL);
perror("Error executing process"); // Only reached if execl fails
exit(-1);
```

### `wait(status)` / `waitpid(pid, status, options)`
**Purpose**: Wait for child process to change state

**Parameters**:

- `pid`: Specific child (-1 = any child)
- `status`: Exit status output
- `options`: 0 = blocking, WNOHANG = non-blocking

**Practice**: Prevents zombie processes

**Usage in Code**:
```c
waitpid(schedulerPid, NULL, 0); // Wait for specific child
```

### `kill(pid, signal)`
**Purpose**: Send signal to process

**Common Signals**:

- `SIGSTOP`: Pause process execution
- `SIGCONT`: Resume paused process
- `SIGKILL`: Force terminate (cannot be caught)
- `SIGINT`: Interrupt (Ctrl+C)
- `SIGUSR1`: User-defined signal

**Usage in Code**:
```c
kill(pcb->pid, SIGSTOP);  // Preempt process
kill(pcb->pid, SIGCONT);  // Resume process
kill(pcb->pid, SIGKILL);  // Terminate process
```

### `signal(signum, handler)`
**Purpose**: Register signal handler function

**Practice**: Handler should be simple and set flags, not do heavy work

**Usage in Code**:
```c
signal(SIGINT, cleanup);  // Handle Ctrl+C
signal(SIGUSR1, handleProcessFinish);
```

### `getpid()` / `getpgrp()`
**Purpose**: Get process ID or process group ID

**Usage in Code**:
```c
printf("Process PID: %d\n", getpid());
killpg(getpgrp(), SIGINT); // Kill entire process group
```

## Data Structures & Design Patterns

### Process Control Block (PCB)
**Purpose**: Maintain all information about a process

**Practice**: Central data structure for process management

**Fields**:
```c
typedef struct {
    int id;                 // Unique identifier
    int arrivalTime;        // When process arrives
    int runtime;            // Total execution time needed
    int priority;           // Priority level
    int remainingTime;      // Time left to execute
    int waitingTime;        // Total time spent waiting
    int executionTime;      // Time actually executed
    int startTime;          // First execution time
    int finishTime;         // Completion time
    int lastStopTime;       // Last preemption time
    ProcessState state;     // READY, RUNNING, BLOCKED, FINISHED
    pid_t pid;              // OS process ID
    bool started;           // Has process been forked?
} PCB;
```

### Queue Implementation (Linked List)

#### Node Structure
```c
typedef struct QueueNode {
    PCB* pcb;              // Pointer to process data
    struct QueueNode* next; // Next node in queue
} QueueNode;
```

#### Queue Structure
```c
typedef struct {
    QueueNode* head;  // Front of queue
    QueueNode* tail;  // Back of queue
    int size;         // Number of elements
} Queue;
```

### Queue Operations

#### `initQueue(Queue* q)`
**Purpose**: Initialize empty queue

**Practice**: Always call before using queue

```c
void initQueue(Queue* q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}
```

#### `enqueue(Queue* q, PCB* pcb)`
**Purpose**: Add element to back of queue

**Algorithm**:

1. Allocate new node
2. Set node data
3. If empty: head = tail = node
4. Else: tail->next = node, tail = node
5. Increment size

**Practice**: O(1) operation with tail pointer

#### `dequeue(Queue* q)`
**Purpose**: Remove and return front element

**Algorithm**:

1. Save head node
2. Move head to next node
3. If empty: set tail = NULL
4. Free old head node
5. Decrement size
6. Return PCB

**Practice**: O(1) operation, check for empty queue

#### `peek(Queue* q)`
**Purpose**: View front element without removing

**Practice**: Read-only operation, useful for lookahead

#### `removeFromQueue(Queue* q, PCB* pcb)`
**Purpose**: Remove specific element from anywhere in queue

**Algorithm**:

1. Handle head removal specially
2. Traverse list to find element
3. Update previous node's next pointer
4. Handle tail update if needed
5. Free node

**Practice**: O(n) operation, used for preemption

## Scheduling Algorithm Implementations

### Highest Priority First (HPF)

```c
PCB* selectHPF() {
    QueueNode* node = readyQueue.head;
    PCB* highest = NULL;
    
    while (node != NULL) {
        if (highest == NULL || 
            node->pcb->priority < highest->priority) {
            highest = node->pcb;
        } else if (node->pcb->priority == highest->priority) {
            // Tie-breaking by arrival time
            if (node->pcb->arrivalTime < highest->arrivalTime) {
                highest = node->pcb;
            }
        }
        node = node->next;
    }
    return highest;
}
```

**Practice**: Linear scan finds minimum, O(n) complexity

### Shortest Job Next (SJN/SRTN)

```c
PCB* selectSJN() {
    QueueNode* node = readyQueue.head;
    PCB* shortest = NULL;
    
    while (node != NULL) {
        if (shortest == NULL || 
            node->pcb->remainingTime < shortest->remainingTime) {
            shortest = node->pcb;
        } else if (node->pcb->remainingTime == shortest->remainingTime) {
            // Tie-breaking
            if (node->pcb->arrivalTime < shortest->arrivalTime) {
                shortest = node->pcb;
            }
        }
        node = node->next;
    }
    return shortest;
}
```

**Practice**: Greedy algorithm, minimizes average waiting time

### Round Robin (RR)

```c
PCB* selectRR() {
    if (isEmpty(&readyQueue)) {
        return NULL;
    }
    
    PCB* selected = peek(&readyQueue);
    
    // Rotate queue for circular behavior
    QueueNode* head = readyQueue.head;
    if (readyQueue.size > 1) {
        readyQueue.head = head->next;
        readyQueue.tail->next = head;
        head->next = NULL;
        readyQueue.tail = head;
    }
    
    return selected;
}
```

**Practice**: FIFO with rotation ensures fairness

### Multi-Level Feedback Queue (MLFQ)

**Structure**:
```c
#define NUM_QUEUES 3
static Queue mlfqQueues[NUM_QUEUES];
static int processQueueLevel[100];
```

**Quantum Mapping**:
```c
int getMLFQQuantum(int queueLevel) {
    switch (queueLevel) {
        case 0: return 2;  // Highest priority
        case 1: return 4;  // Medium priority
        case 2: return 8;  // Lowest priority
        default: return 8;
    }
}
```

**Selection** (Priority-based):
```c
PCB* selectMLFQ() {
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (!isEmpty(&mlfqQueues[i])) {
            return dequeue(&mlfqQueues[i]);
        }
    }
    return NULL;
}
```

**Demotion** (on quantum expiration):
```c
void demoteProcess(PCB* pcb) {
    int currentLevel = processQueueLevel[pcb->id];
    if (currentLevel >= NUM_QUEUES - 1) {
        enqueueMLFQ(pcb, currentLevel); // Stay at lowest
        return;
    }
    int newLevel = currentLevel + 1;
    enqueueMLFQ(pcb, newLevel);
}
```

**Practice**: Adaptive priority prevents starvation while favoring I/O-bound processes

## File I/O Practices

### File Opening
```c
FILE* file = fopen("filename.txt", "r");
if (file == NULL) {
    perror("Error opening file");
    return -1;
}
```

**Modes**:

- `"r"`: Read
- `"w"`: Write (truncate)
- `"a"`: Append

### Reading Formatted Input
```c
fscanf(file, "%d", &value);
fgets(line, sizeof(line), file);  // Read line
sscanf(line, "%d\t%d\t%d", &a, &b, &c);  // Parse line
```

**Practice**: Check return values to detect errors

### Writing Output
```c
fprintf(file, "At time %d process %d started\n", time, id);
fflush(file);  // Force write to disk
```

**Practice**: Flush after important writes for immediate visibility

### Closing Files
```c
fclose(file);
```

**Practice**: Always close files to prevent resource leaks

## Error Handling Practices

### Checking System Calls
```c
if (result == -1) {
    perror("Operation description");
    exit(-1);
}
```

**Practice**: Every system call should be checked

### `perror()` Function
**Purpose**: Print error message with system error description

**Practice**: Provides errno-based error details

### Defensive Programming
```c
if (argc < 2) {
    fprintf(stderr, "Usage: %s <argument>\n", argv[0]);
    return -1;
}
```

**Practice**: Validate all inputs and assumptions

## Time & Sleep Functions

### `sleep(seconds)`
**Purpose**: Suspend execution for specified seconds

**Practice**: Used by clock to create 1-second ticks

### `usleep(microseconds)`
**Purpose**: Suspend execution for microseconds

**Practice**: Shorter delays for polling loops

**Usage**:
```c
usleep(100000);  // 100ms = 0.1 seconds
usleep(10000);   // 10ms
```

## Logging & Metrics Practices

### Event Logging Pattern
```c
void writeLog(const char* state, PCB* pcb) {
    currentTime = getClk();
    fprintf(logFile, "At time %d process %d %s arr %d total %d remain %d wait %d",
            currentTime, pcb->id, state, pcb->arrivalTime,
            pcb->runtime, pcb->remainingTime, pcb->waitingTime);
    
    if (strcmp(state, "finished") == 0) {
        int TA = pcb->finishTime - pcb->arrivalTime;
        double WTA = (double)TA / pcb->runtime;
        fprintf(logFile, " TA %d WTA %.2f", TA, WTA);
    }
    fprintf(logFile, "\n");
    fflush(logFile);
}
```

**Practice**: Structured logging with consistent format

### Performance Metrics Calculation
```c
// CPU Utilization
double cpuUtil = ((double)totalRuntime / totalTime) * 100;

// Average Weighted Turnaround Time
double avgWTA = totalWTA / finishedCount;

// Standard Deviation
double variance = (totalWTASquared / finishedCount) - (avgWTA * avgWTA);
double stdWTA = sqrt(variance);
```

**Practice**: Accumulate statistics incrementally for efficiency

## Memory Management Practices

### Dynamic Allocation
```c
QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
if (node == NULL) {
    perror("Memory allocation failed");
    exit(-1);
}
```

**Practice**: Always check malloc return value

### Deallocation
```c
free(node);
```

**Practice**: Free every malloc to prevent memory leaks

### Static Arrays
```c
PCB processes[MAX_PROCESSES];
```

**Practice**: Fixed-size arrays for bounded resources

## String Handling

### String Formatting
```c
char buffer[20];
sprintf(buffer, "%d", value);  // Convert int to string
```

### String Comparison
```c
if (strcmp(str1, str2) == 0) {
    // Strings are equal
}
```

**Practice**: Never use `==` for string comparison

## Preprocessor Practices

### Include Guards
```c
#pragma once
```

**Practice**: Prevents multiple inclusion of headers

### Macros
```c
#define MAX_PROCESSES 100
#define SHM_PROJ 'C'
#define KEY_FILE ".osclock_key"
```

**Practice**: Constants as macros for easy modification

## Testing & Debugging Practices

### Test Generator
**Purpose**: Create randomized test cases

**Practice**: Seeded randomness for reproducibility

```c
srand(seed);  // Use same seed for repeated tests
```

### Debug Printing
```c
printf("Process %d: remaining=%d\n", id, remaining);
```

**Practice**: Verbose logging during development

### Profiling Scripts
**Purpose**: Performance analysis with flamegraphs

**Tools Used**:

- `perf record`: Capture execution profile
- `stackcollapse-perf.pl`: Convert to flamegraph format
- `flamegraph.pl`: Generate SVG visualization

**Practice**: Automated profiling for optimization

## Security Considerations

### Kernel Parameter Bypass Script
**Purpose**: Enable profiling by relaxing kernel restrictions

**Parameters Modified**:

- `kernel.kptr_restrict=0`: Allow kernel pointer access
- `kernel.perf_event_paranoid=0`: Allow performance monitoring

**Practice**: Restore original values on exit (trap handler)

**Warning**: Only for development, never in production