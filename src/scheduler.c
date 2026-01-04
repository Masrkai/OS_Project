#include "../include/schedule.h"
#include "../include/headers.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <math.h>

// Global variables (actual definitions)
#define MAX_PROCESSES 100

int msgqid;
int algorithm;

int quantum;
int currentTime = 0;
int totalRuntime = 0;
int totalWaitingTime = 0;

int processCount = 0;
int finishedCount = 0;
int quantumCounter = 0;
int currentMLFQLevel = 0;

double totalWTA = 0;
double totalWTASquared = 0;

Queue readyQueue;
PCB* runningProcess = NULL;
PCB processes[MAX_PROCESSES];

FILE* logFile;
FILE* perfFile;

// Modified main() function - key changes:
int main(int argc, char * argv[])
{
    initClk();

    if (argc < 4) {
        printf("Error: Scheduler needs algorithm, quantum, and msgqid arguments!\n");
        return -1;
    }

    algorithm = atoi(argv[1]);
    quantum = atoi(argv[2]);
    msgqid = atoi(argv[3]);

    printf("Scheduler started: Algorithm=%d, Quantum=%d, MsgQID=%d\n",
           algorithm, quantum, msgqid);

    logFile = fopen("scheduler.log", "w");
    if (logFile == NULL) {
        perror("Error opening log file");
        return -1;
    }
    fprintf(logFile, "#At time x process y state arr w total z remain y wait k\n");

    // Initialize appropriate queue structure
    if (algorithm == 4) { // MLFQ
        initMLFQ();
    } else {
        initQueue(&readyQueue);
    }

    signal(SIGUSR1, handleProcessFinish);

    bool allProcessesArrived = false;

    while (!allProcessesArrived ||
        (algorithm == 4 ? !isMLFQEmpty() : !isEmpty(&readyQueue)) ||
        runningProcess != NULL) {

        int previousTime = currentTime;
        currentTime = getClk();
        int elapsed = currentTime - previousTime;

        receiveProcesses();

        // Decrement remaining time for running process
        if (runningProcess != NULL && runningProcess->state == RUNNING && elapsed > 0) {
            runningProcess->remainingTime -= elapsed;
            runningProcess->executionTime += elapsed;

            printf("Process %d: remaining=%d\n",
                runningProcess->id, runningProcess->remainingTime);
        }

        if (runningProcess != NULL && runningProcess->remainingTime <= 0) {
            finishProcess(runningProcess);
            runningProcess = NULL;
        }

        // Handle quantum expiration for RR and MLFQ
        if ((algorithm == 3 || algorithm == 4) &&
            runningProcess != NULL &&
            runningProcess->state == RUNNING) {

            quantumCounter += elapsed;

            int currentQuantum = (algorithm == 4) ?
                getMLFQQuantum(currentMLFQLevel) : quantum;

            if (quantumCounter >= currentQuantum && runningProcess->remainingTime > 0) {
                stopProcess(runningProcess);

                if (algorithm == 4) {
                    handleMLFQQuantumExpired(runningProcess);
                } else {
                    enqueue(&readyQueue, runningProcess);
                }

                runningProcess = NULL;
                quantumCounter = 0;
            }
        }

        // Schedule next process if CPU is idle
        if (runningProcess == NULL) {
            bool hasProcesses = (algorithm == 4) ? !isMLFQEmpty() : !isEmpty(&readyQueue);
            if (hasProcesses) {
                selectNextProcess();
            }
        }

        // Check if all processes have arrived
        Message msg;
        if (msgrcv(msgqid, &msg, sizeof(msg.process), 2, IPC_NOWAIT) != -1) {
            allProcessesArrived = true;
            printf("All processes have arrived\n");
        }

        // Update waiting time
        if (algorithm != 4) {
            QueueNode* node = readyQueue.head;
            while (node != NULL) {
                if (node->pcb->state == READY) {
                    node->pcb->waitingTime += elapsed;
                }
                node = node->next;
            }
        }

        usleep(10000); // 10ms sleep
    }

    printf("All processes completed\n");
    writePerformanceMetrics();
    fclose(logFile);
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

    while (msgrcv(msgqid, &msg, sizeof(msg.process), 1, IPC_NOWAIT) != -1) {
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

        // Add to appropriate queue
        if (algorithm == 4) { // MLFQ
            handleMLFQNewProcess(pcb);
        } else {
            enqueue(&readyQueue, pcb);
        }
    }
}

// Modified selectNextProcess() function:
void selectNextProcess() {
    PCB* selected = NULL;

    switch (algorithm) {
        case 1: // HPF
            selected = selectHPF();
            break;
        case 2: // SRTN
            selected = selectSJN();
            break;
        case 3: // RR
            selected = selectRR();
            quantumCounter = 0;
            break;
        case 4: // MLFQ
            selected = selectMLFQ();
            if (selected != NULL) {
                currentMLFQLevel = getProcessQueueLevel(selected->id);
                quantumCounter = 0;
                printf("Selected process %d from MLFQ level %d (quantum=%d)\n",
                       selected->id, currentMLFQLevel,
                       getMLFQQuantum(currentMLFQLevel));
            }
            break;
    }

    if (selected != NULL) {
        runningProcess = selected;
        // Note: For MLFQ, process is already dequeued by selectMLFQ()
        if (algorithm != 4) {
            removeFromQueue(&readyQueue, selected);
        }

        if (!selected->started) {
            startProcess(selected);
        } else {
            resumeProcess(selected);
        }
    }
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