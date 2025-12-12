#pragma once

#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>

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

// Global variables (extern declarations)
extern int algorithm;
extern int quantum;
extern int msgqid;
extern PCB processes[];
extern int processCount;
extern Queue readyQueue;
extern PCB* runningProcess;
extern int currentTime;
extern int totalWaitingTime;
extern int totalRuntime;
extern double totalWTA;
extern double totalWTASquared;
extern int finishedCount;
extern int quantumCounter;
extern FILE* logFile;
extern FILE* perfFile;

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
int getClk();
void initClk();
void destroyClk(bool);

// Algorithm-specific selection functions
PCB* selectHPF();
PCB* selectSJN();
PCB* selectRR();