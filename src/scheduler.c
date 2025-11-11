#include "../include/headers.h"
#include <math.h>
#include <string.h>

#define MAX_PROCESSES 100

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

// Ready queue
typedef struct {
    QueueNode* head;
    QueueNode* tail;
    int size;
} Queue;

// Message structure for IPC
typedef struct {
    long mtype;
    struct {
        int id;
        int arrivalTime;
        int runtime;
        int priority;
    } process;
} Message;

// Global variables
int algorithm;
int quantum;
int msgqid;
PCB processes[MAX_PROCESSES];
int processCount = 0;
Queue readyQueue;
PCB* runningProcess = NULL;
int currentTime = 0;
int totalWaitingTime = 0;
int totalRuntime = 0;
double totalWTA = 0;
double totalWTASquared = 0;
int finishedCount = 0;
int quantumCounter = 0;
FILE* logFile;
FILE* perfFile;

// Function declarations
void initQueue(Queue* q);
void enqueue(Queue* q, PCB* pcb);
PCB* dequeue(Queue* q);
PCB* peek(Queue* q);
bool isEmpty(Queue* q);
void removeFromQueue(Queue* q, PCB* pcb);
void scheduleNext();
void startProcess(PCB* pcb);
void stopProcess(PCB* pcb);
void resumeProcess(PCB* pcb);
void finishProcess(PCB* pcb);
void handleProcessFinish(int signum);
void receiveProcesses();
void selectNextProcess();
void writeLog(const char* state, PCB* pcb);
void writePerformanceMetrics();
void cleanup();
PCB* selectHPF();
PCB* selectSJN();
PCB* selectRR();

int main(int argc, char * argv[])
{
    initClk();

    // Get parameters from command line
    if (argc < 4) {
        printf("Error: Scheduler needs algorithm, quantum, and msgqid arguments!\n");
        return -1;
    }

    algorithm = atoi(argv[1]);
    quantum = atoi(argv[2]);
    msgqid = atoi(argv[3]);

    printf("Scheduler started: Algorithm=%d, Quantum=%d, MsgQID=%d\n",
           algorithm, quantum, msgqid);

    // Open log files
    logFile = fopen("scheduler.log", "w");
    if (logFile == NULL) {
        perror("Error opening log file");
        return -1;
    }
    fprintf(logFile, "#At time x process y state arr w total z remain y wait k\n");

    // Initialize ready queue
    initQueue(&readyQueue);

    // Set up signal handler for process completion
    signal(SIGUSR1, handleProcessFinish);

    // Main scheduling loop
    bool allProcessesArrived = false;

    while (!allProcessesArrived || !isEmpty(&readyQueue) || runningProcess != NULL) {
        currentTime = getClk();

        // Receive new processes
        receiveProcesses();

        // Check for process completion
        if (runningProcess != NULL && runningProcess->remainingTime <= 0) {
            finishProcess(runningProcess);
            runningProcess = NULL;
        }

        // Handle Round Robin quantum expiration
        if (algorithm == 3 && runningProcess != NULL && runningProcess->state == RUNNING) {
            quantumCounter++;
            if (quantumCounter >= quantum && runningProcess->remainingTime > 0) {
                stopProcess(runningProcess);
                enqueue(&readyQueue, runningProcess);
                runningProcess = NULL;
                quantumCounter = 0;
            }
        }

        // Schedule next process if CPU is idle
        if (runningProcess == NULL && !isEmpty(&readyQueue)) {
            selectNextProcess();
        }

        // Check if all processes have arrived (received termination message)
        Message msg;
        if (msgrcv(msgqid, &msg, sizeof(msg.process), 2, IPC_NOWAIT) != -1) {
            allProcessesArrived = true;
            printf("All processes have arrived\n");
        }

        // Update waiting time for processes in ready queue
        QueueNode* node = readyQueue.head;
        while (node != NULL) {
            if (node->pcb->state == READY) {
                node->pcb->waitingTime++;
            }
            node = node->next;
        }

        // Avoid busy waiting
        usleep(10000); // 10ms
    }

    printf("All processes completed\n");

    // Write performance metrics
    writePerformanceMetrics();

    // Close log files
    fclose(logFile);

    // Clean up
    cleanup();
    destroyClk(true);

    return 0;
}

void initQueue(Queue* q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void enqueue(Queue* q, PCB* pcb) {
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    node->pcb = pcb;
    node->next = NULL;

    if (q->tail == NULL) {
        q->head = q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
    q->size++;
}

PCB* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;

    QueueNode* node = q->head;
    PCB* pcb = node->pcb;
    q->head = q->head->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }

    free(node);
    q->size--;
    return pcb;
}

PCB* peek(Queue* q) {
    if (q->head == NULL) return NULL;
    return q->head->pcb;
}

bool isEmpty(Queue* q) {
    return q->size == 0;
}

void removeFromQueue(Queue* q, PCB* pcb) {
    if (q->head == NULL) return;

    if (q->head->pcb == pcb) {
        dequeue(q);
        return;
    }

    QueueNode* prev = q->head;
    QueueNode* curr = q->head->next;

    while (curr != NULL) {
        if (curr->pcb == pcb) {
            prev->next = curr->next;
            if (curr == q->tail) {
                q->tail = prev;
            }
            free(curr);
            q->size--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void receiveProcesses() {
    Message msg;

    // Non-blocking receive of all arrived processes
    while (msgrcv(msgqid, &msg, sizeof(msg.process), 1, IPC_NOWAIT) != -1) {
        // Create PCB for new process
        PCB* pcb = &processes[processCount++];
        pcb->id = msg.process.id;
        pcb->arrivalTime = msg.process.arrivalTime;
        pcb->runtime = msg.process.runtime;
        pcb->priority = msg.process.priority;
        pcb->remainingTime = msg.process.runtime;
        pcb->waitingTime = 0;
        pcb->executionTime = 0;
        pcb->state = READY;
        pcb->pid = -1;
        pcb->started = false;
        pcb->startTime = -1;
        pcb->lastStopTime = -1;

        totalRuntime += pcb->runtime;

        printf("Received process %d at time %d\n", pcb->id, currentTime);

        // Add to ready queue
        enqueue(&readyQueue, pcb);
    }
}

void selectNextProcess() {
    PCB* selected = NULL;

    switch (algorithm) {
        case 1: // HPF
            selected = selectHPF();
            break;
        case 2: // SJN
            selected = selectSJN();
            break;
        case 3: // RR
            selected = selectRR();
            quantumCounter = 0;
            break;
    }

    if (selected != NULL) {
        runningProcess = selected;
        removeFromQueue(&readyQueue, selected);

        if (!selected->started) {
            startProcess(selected);
        } else {
            resumeProcess(selected);
        }
    }
}

PCB* selectHPF() {
    // Find process with highest priority (lowest priority number)
    QueueNode* node = readyQueue.head;
    PCB* highest = NULL;

    while (node != NULL) {
        if (highest == NULL || node->pcb->priority < highest->priority) {
            highest = node->pcb;
        } else if (node->pcb->priority == highest->priority) {
            // Tie-breaking: choose the one that arrived first
            if (node->pcb->arrivalTime < highest->arrivalTime) {
                highest = node->pcb;
            }
        }
        node = node->next;
    }

    return highest;
}

PCB* selectSJN() {
    // Find process with shortest remaining time
    QueueNode* node = readyQueue.head;
    PCB* shortest = NULL;

    while (node != NULL) {
        if (shortest == NULL || node->pcb->remainingTime < shortest->remainingTime) {
            shortest = node->pcb;
        } else if (node->pcb->remainingTime == shortest->remainingTime) {
            // Tie-breaking: choose the one that arrived first
            if (node->pcb->arrivalTime < shortest->arrivalTime) {
                shortest = node->pcb;
            }
        }
        node = node->next;
    }

    return shortest;
}

PCB* selectRR() {
    // Round Robin: simply take the first process in queue (FCFS)
    return peek(&readyQueue);
}

void startProcess(PCB* pcb) {
    currentTime = getClk();

    // Fork the process
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        char remainingTimeStr[20];
        sprintf(remainingTimeStr, "%d", pcb->remainingTime);
        execl("./process.out", "process.out", remainingTimeStr, NULL);
        perror("Error executing process");
        exit(-1);
    } else if (pid > 0) {
        pcb->pid = pid;
        pcb->started = true;
        pcb->state = RUNNING;
        pcb->startTime = currentTime;
        pcb->executionTime = 0;

        printf("Started process %d with PID %d at time %d\n", pcb->id, pid, currentTime);

        writeLog("started", pcb);
    } else {
        perror("Error forking process");
    }
}

void stopProcess(PCB* pcb) {
    currentTime = getClk();

    // Send SIGSTOP to pause the process
    kill(pcb->pid, SIGSTOP);

    pcb->state = READY;
    pcb->lastStopTime = currentTime;

    printf("Stopped process %d at time %d\n", pcb->id, currentTime);

    writeLog("stopped", pcb);
}

void resumeProcess(PCB* pcb) {
    currentTime = getClk();

    // Send SIGCONT to resume the process
    kill(pcb->pid, SIGCONT);

    pcb->state = RUNNING;

    // Update waiting time
    if (pcb->lastStopTime != -1) {
        int waitTime = currentTime - pcb->lastStopTime;
        pcb->waitingTime += waitTime;
    }

    printf("Resumed process %d at time %d\n", pcb->id, currentTime);

    writeLog("resumed", pcb);
}

void finishProcess(PCB* pcb) {
    currentTime = getClk();

    pcb->state = FINISHED;
    pcb->finishTime = currentTime;

    // Calculate metrics
    int turnaroundTime = pcb->finishTime - pcb->arrivalTime;
    double wta = (double)turnaroundTime / pcb->runtime;

    totalWaitingTime += pcb->waitingTime;
    totalWTA += wta;
    totalWTASquared += (wta * wta);
    finishedCount++;

    printf("Finished process %d at time %d (TA=%d, WTA=%.2f)\n",
           pcb->id, currentTime, turnaroundTime, wta);

    writeLog("finished", pcb);

    // Terminate the process
    kill(pcb->pid, SIGKILL);
    waitpid(pcb->pid, NULL, 0);
}

void handleProcessFinish(int signum) {
    // This signal handler is called when a process sends SIGUSR1
    // We handle the actual finishing in the main loop
    if (runningProcess != NULL) {
        runningProcess->remainingTime = 0;
    }
}

void writeLog(const char* state, PCB* pcb) {
    currentTime = getClk();

    fprintf(logFile, "At time %d process %d %s arr %d total %d remain %d wait %d",
            currentTime, pcb->id, state, pcb->arrivalTime,
            pcb->runtime, pcb->remainingTime, pcb->waitingTime);

    if (strcmp(state, "finished") == 0) {
        int turnaroundTime = pcb->finishTime - pcb->arrivalTime;
        double wta = (double)turnaroundTime / pcb->runtime;
        fprintf(logFile, " TA %d WTA %.2f", turnaroundTime, wta);
    }

    fprintf(logFile, "\n");
    fflush(logFile);
}

void writePerformanceMetrics() {
    perfFile = fopen("scheduler.perf", "w");
    if (perfFile == NULL) {
        perror("Error opening performance file");
        return;
    }

    // CPU utilization
    int totalTime = currentTime;
    double cpuUtilization = (totalTime > 0) ? ((double)totalRuntime / totalTime) * 100 : 0;

    // Average WTA
    double avgWTA = (finishedCount > 0) ? totalWTA / finishedCount : 0;

    // Average waiting time
    double avgWaiting = (finishedCount > 0) ? (double)totalWaitingTime / finishedCount : 0;

    // Standard deviation of WTA
    double variance = (finishedCount > 0) ?
                      (totalWTASquared / finishedCount) - (avgWTA * avgWTA) : 0;
    double stdWTA = sqrt(variance);

    fprintf(perfFile, "CPU utilization = %.2f%%\n", cpuUtilization);
    fprintf(perfFile, "Avg WTA = %.2f\n", avgWTA);
    fprintf(perfFile, "Avg Waiting = %.2f\n", avgWaiting);
    fprintf(perfFile, "Std WTA = %.2f\n", stdWTA);

    fclose(perfFile);

    printf("\nPerformance Metrics:\n");
    printf("CPU utilization = %.2f%%\n", cpuUtilization);
    printf("Avg WTA = %.2f\n", avgWTA);
    printf("Avg Waiting = %.2f\n", avgWaiting);
    printf("Std WTA = %.2f\n", stdWTA);
}

void cleanup() {
    printf("Scheduler cleanup complete\n");
}