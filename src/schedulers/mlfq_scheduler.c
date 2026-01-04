#include "../../include/schedule.h"
#include <stdlib.h>
#include <stdio.h>

// MLFQ Configuration
#define NUM_QUEUES 3
#define QUEUE_0_QUANTUM 2
#define QUEUE_1_QUANTUM 4
#define QUEUE_2_QUANTUM 8

// Aging parameters
#define AGING_THRESHOLD 10  // Time units before promoting
#define BOOST_INTERVAL 50   // Periodic boost interval to prevent starvation

// MLFQ queues - high priority (0) to low priority (NUM_QUEUES-1)
static Queue mlfqQueues[NUM_QUEUES];
static bool mlfqInitialized = false;

// Enhanced tracking for each process
typedef struct {
    int queueLevel;           // Current queue level
    int timeInQueue;          // Time spent waiting in current queue
    int totalQuantumUsed;     // Total quantum used in current queue level
    int timeSinceLastBoost;   // Time since last priority boost
    bool wasPreempted;        // Whether process was preempted or yielded
} MLFQProcessInfo;

static MLFQProcessInfo processInfo[100]; // Assumes MAX_PROCESSES = 100
static int lastBoostTime = 0;            // Last time we did a global boost

void initMLFQ() {
    if (mlfqInitialized) return;

    for (int i = 0; i < NUM_QUEUES; i++) {
        initQueue(&mlfqQueues[i]);
    }

    // Initialize all process info
    for (int i = 0; i < 100; i++) {
        processInfo[i].queueLevel = -1;
        processInfo[i].timeInQueue = 0;
        processInfo[i].totalQuantumUsed = 0;
        processInfo[i].timeSinceLastBoost = 0;
        processInfo[i].wasPreempted = false;
    }

    mlfqInitialized = true;
    lastBoostTime = 0;
    printf("MLFQ initialized with %d queues (aging enabled)\n", NUM_QUEUES);
}

int getMLFQQuantum(int queueLevel) {
    switch (queueLevel) {
        case 0: return QUEUE_0_QUANTUM;
        case 1: return QUEUE_1_QUANTUM;
        case 2: return QUEUE_2_QUANTUM;
        default: return QUEUE_2_QUANTUM;
    }
}

void enqueueMLFQ(PCB* pcb, int queueLevel) {
    if (!mlfqInitialized) {
        initMLFQ();
    }

    // Clamp queue level to valid range
    if (queueLevel < 0) queueLevel = 0;
    if (queueLevel >= NUM_QUEUES) queueLevel = NUM_QUEUES - 1;

    // Update process info
    processInfo[pcb->id].queueLevel = queueLevel;
    processInfo[pcb->id].timeInQueue = 0;  // Reset waiting time in new queue

    // Add to appropriate queue
    enqueue(&mlfqQueues[queueLevel], pcb);

    printf("Process %d added to MLFQ queue %d\n", pcb->id, queueLevel);
}

// Promote a process to a higher priority queue (aging)
void promoteProcess(PCB* pcb) {
    int currentLevel = processInfo[pcb->id].queueLevel;

    // Don't promote if already at highest priority
    if (currentLevel <= 0) {
        return;
    }

    int newLevel = currentLevel - 1;
    
    // Reset quantum tracking for new level
    processInfo[pcb->id].totalQuantumUsed = 0;
    processInfo[pcb->id].timeSinceLastBoost = 0;
    
    enqueueMLFQ(pcb, newLevel);
    printf("Process %d PROMOTED from queue %d to queue %d (aging)\n",
           pcb->id, currentLevel, newLevel);
}

// Demote a process to a lower priority queue
void demoteProcess(PCB* pcb) {
    int currentLevel = processInfo[pcb->id].queueLevel;

    // Don't demote if already at lowest priority
    if (currentLevel >= NUM_QUEUES - 1) {
        enqueueMLFQ(pcb, currentLevel);
        printf("Process %d remains at lowest queue %d\n", pcb->id, currentLevel);
        return;
    }

    // Move to next lower priority queue
    int newLevel = currentLevel + 1;
    
    // Reset quantum tracking for new level
    processInfo[pcb->id].totalQuantumUsed = 0;
    
    enqueueMLFQ(pcb, newLevel);
    printf("Process %d DEMOTED from queue %d to queue %d\n",
           pcb->id, currentLevel, newLevel);
}

PCB* selectMLFQ() {
    if (!mlfqInitialized) {
        initMLFQ();
    }

    // Check each queue from highest to lowest priority
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (!isEmpty(&mlfqQueues[i])) {
            PCB* selected = dequeue(&mlfqQueues[i]);
            printf("MLFQ selected process %d from queue %d\n", selected->id, i);
            return selected;
        }
    }

    return NULL; // No process available
}

bool isMLFQEmpty() {
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (!isEmpty(&mlfqQueues[i])) {
            return false;
        }
    }
    return true;
}

int getProcessQueueLevel(int processId) {
    return processInfo[processId].queueLevel;
}

void handleMLFQNewProcess(PCB* pcb) {
    // New processes start at highest priority queue (queue 0)
    processInfo[pcb->id].totalQuantumUsed = 0;
    processInfo[pcb->id].timeSinceLastBoost = 0;
    processInfo[pcb->id].wasPreempted = true;
    enqueueMLFQ(pcb, 0);
}

void handleMLFQQuantumExpired(PCB* pcb) {
    // Process used its full quantum - demote it
    int currentLevel = processInfo[pcb->id].queueLevel;
    int quantumForLevel = getMLFQQuantum(currentLevel);
    
    processInfo[pcb->id].totalQuantumUsed += quantumForLevel;
    processInfo[pcb->id].wasPreempted = true;
    
    // Demote if process consistently uses full quantum
    // This identifies CPU-bound processes
    demoteProcess(pcb);
}

void handleMLFQProcessYielded(PCB* pcb) {
    // Process yielded before quantum expired (e.g., I/O or completion)
    // This might indicate an I/O-bound process
    int currentLevel = processInfo[pcb->id].queueLevel;
    
    processInfo[pcb->id].wasPreempted = false;
    
    // Keep at same priority or even promote if consistently yielding
    // This helps I/O-bound processes get quick responses
    if (currentLevel > 0 && processInfo[pcb->id].totalQuantumUsed < getMLFQQuantum(currentLevel)) {
        // Process is being cooperative, keep at current level
        enqueueMLFQ(pcb, currentLevel);
        printf("Process %d yielded cooperatively, stays at queue %d\n", 
               pcb->id, currentLevel);
    } else {
        enqueueMLFQ(pcb, currentLevel);
    }
}

// Update aging for all waiting processes
void updateMLFQAging(int currentTime, int elapsedTime) {
    if (!mlfqInitialized) return;
    
    // Check if it's time for a global priority boost
    if (currentTime - lastBoostTime >= BOOST_INTERVAL) {
        performPriorityBoost();
        lastBoostTime = currentTime;
        return;
    }
    
    // Update waiting time for all processes in queues
    for (int queueLevel = 0; queueLevel < NUM_QUEUES; queueLevel++) {
        QueueNode* node = mlfqQueues[queueLevel].head;
        
        while (node != NULL) {
            PCB* pcb = node->pcb;
            
            // Increment time spent waiting in this queue
            processInfo[pcb->id].timeInQueue += elapsedTime;
            processInfo[pcb->id].timeSinceLastBoost += elapsedTime;
            
            // Check if process should be promoted due to aging
            if (processInfo[pcb->id].timeInQueue >= AGING_THRESHOLD && 
                queueLevel > 0) {
                
                // Remove from current queue and promote
                QueueNode* nextNode = node->next;
                removeFromQueue(&mlfqQueues[queueLevel], pcb);
                promoteProcess(pcb);
                
                node = nextNode;
                continue;
            }
            
            node = node->next;
        }
    }
}

// Perform a global priority boost to prevent starvation
void performPriorityBoost() {
    printf("Performing global priority boost to prevent starvation\n");
    
    // Move all processes to highest priority queue
    for (int queueLevel = 1; queueLevel < NUM_QUEUES; queueLevel++) {
        while (!isEmpty(&mlfqQueues[queueLevel])) {
            PCB* pcb = dequeue(&mlfqQueues[queueLevel]);
            
            // Reset tracking info
            processInfo[pcb->id].totalQuantumUsed = 0;
            processInfo[pcb->id].timeSinceLastBoost = 0;
            
            // Add to highest priority queue
            enqueueMLFQ(pcb, 0);
        }
    }
}

// Get statistics about MLFQ state
void printMLFQStats() {
    printf("\n=== MLFQ Statistics ===\n");
    for (int i = 0; i < NUM_QUEUES; i++) {
        printf("Queue %d (quantum=%d): %d processes\n", 
               i, getMLFQQuantum(i), mlfqQueues[i].size);
    }
    printf("======================\n\n");
}