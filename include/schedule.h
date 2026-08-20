#pragma once

/* ==================== PrismScheduler Header ==================== */

/* ==================== Includes ==================== */

#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>

/* ==================== Data Structures ==================== */

// Process states
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} ProcessState;

// Process Control Block (PCB)
typedef struct {
    int id;
    int arrivalTime;
    int runtime;
    int priority;
    int remainingTime;
    int waitingTime;
    int executionTime;
    int startTime;
    int finishTime;
    int lastStopTime;
    ProcessState state;
    pid_t pid;
    bool started;
} PCB;

// Queue node for ready queue
typedef struct QueueNode {
    PCB* pcb;
    struct QueueNode* next;
} QueueNode;

// Ready queue (singly-linked FIFO)
typedef struct {
    QueueNode* head;
    QueueNode* tail;
    int size;
} Queue;

// Message structure for IPC (message queue)
typedef struct {
    long mtype;
    struct {
        int id;
        int arrivalTime;
        int runtime;
        int priority;
    } process;
} Message;

/* ==================== Global Variables (extern) ==================== */

extern int algorithm;          // Scheduling algorithm identifier
extern int quantum;            // Time quantum (used in RR, MLFQ)
extern int msgqid;             // Message queue ID

extern PCB processes[];        // Array of all processes
extern int processCount;       // Total number of processes

extern Queue readyQueue;       // Main ready queue (used in non-MLFQ algorithms)
extern PCB* runningProcess;    // Currently executing process

extern int currentTime;        // Global simulated clock
extern int finishedCount;      // Count of completed processes

extern int totalWaitingTime;   // Sum of waiting times (for avg)
extern int totalRuntime;       // Sum of runtimes (for utilization)
extern double totalWTA;        // Sum of WTAs (Waiting Time / Runtime)
extern double totalWTASquared; // Sum of squared WTAs (for StdDev)

extern int quantumCounter;     // Tracks ticks in current quantum
extern int currentMLFQLevel;   // Current MLFQ queue level of running process
extern FILE* logFile;          // Log file for trace output
extern FILE* perfFile;         // Performance metrics output file

/* ==================== Core Utilities & Initialization ==================== */

void initClk(void);
void destroyClk(bool cleanupShm);

int getClk(void);              // Get current simulated time

void initQueue(Queue* q);
void initMLFQ(void);

/* ==================== Queue Operations ==================== */

bool isEmpty(Queue* q);
int size(Queue* q);            // Optional but useful
PCB* peek(Queue* q);          // View head without removal

void enqueue(Queue* q, PCB* pcb);
PCB* dequeue(Queue* q);
void removeFromQueue(Queue* q, PCB* pcb);  // Remove arbitrary PCB

/* ==================== Process Lifecycle Management ==================== */

void startProcess(PCB* pcb);
void stopProcess(PCB* pcb);    // Preempt or yield
void resumeProcess(PCB* pcb);
void finishProcess(PCB* pcb);

void handleProcessFinish(int signum);  // Signal handler for SIGCHLD (or custom)

/* ==================== Scheduling Core ==================== */

void receiveProcesses(void);      // Read/process incoming messages
void scheduleNext(void);          // Trigger scheduler
void selectNextProcess(void);     // Dispatch based on `algorithm`

// Algorithm-specific selectors (return next PCB or NULL)
PCB* selectRR(void);
PCB* selectHPF(void);
PCB* selectSJN(void);
PCB* selectMLFQ(void);

/* ==================== Logging & Metrics ==================== */

void writeLog(const char* state, PCB* pcb);
void writePerformanceMetrics(void);

/* ==================== Cleanup ==================== */

void cleanup(void);

/* ==================== MLFQ-Specific (Multi-Level Feedback Queue) ==================== */

// Configuration/state helpers
int getMLFQQuantum(int queueLevel);
int getProcessQueueLevel(int processId);
bool isMLFQEmpty(void);

// Queue operations per level (assumes internal MLFQ structure, e.g., array of Queues)
void enqueueMLFQ(PCB* pcb, int queueLevel);

// Event handlers
void handleMLFQNewProcess(PCB* pcb);
void handleMLFQQuantumExpired(PCB* pcb);
void handleMLFQProcessYielded(PCB* pcb);

// Process promotion/demotion
void demoteProcess(PCB* pcb);
void promoteProcess(PCB* pcb);

// Aging and starvation prevention
void updateMLFQAging(int currentTime, int elapsedTime);
void performPriorityBoost(void);

// Debugging and monitoring
void printMLFQStats(void);