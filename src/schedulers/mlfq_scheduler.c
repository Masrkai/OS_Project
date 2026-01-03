#include "../../include/schedule.h"
#include <stdlib.h>
#include <stdio.h>

// MLFQ Configuration
#define NUM_QUEUES 3
#define QUEUE_0_QUANTUM 2
#define QUEUE_1_QUANTUM 4
#define QUEUE_2_QUANTUM 8

// MLFQ queues - high priority (0) to low priority (NUM_QUEUES-1)
static Queue mlfqQueues[NUM_QUEUES];
static bool mlfqInitialized = false;

// Track which queue each process belongs to
static int processQueueLevel[100]; // Assumes MAX_PROCESSES = 100

void initMLFQ() {
    if (mlfqInitialized) return;

    for (int i = 0; i < NUM_QUEUES; i++) {
        initQueue(&mlfqQueues[i]);
    }

    // Initialize all process queue levels to -1 (not assigned)
    for (int i = 0; i < 100; i++) {
        processQueueLevel[i] = -1;
    }

    mlfqInitialized = true;
    printf("MLFQ initialized with %d queues\n", NUM_QUEUES);
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

    // Store which queue this process is in
    processQueueLevel[pcb->id] = queueLevel;

    // Add to appropriate queue
    enqueue(&mlfqQueues[queueLevel], pcb);

    printf("Process %d added to MLFQ queue %d\n", pcb->id, queueLevel);
}

void demoteProcess(PCB* pcb) {
    int currentLevel = processQueueLevel[pcb->id];

    // Don't demote if already at lowest priority
    if (currentLevel >= NUM_QUEUES - 1) {
        enqueueMLFQ(pcb, currentLevel);
        printf("Process %d remains at lowest queue %d\n", pcb->id, currentLevel);
        return;
    }

    // Move to next lower priority queue
    int newLevel = currentLevel + 1;
    enqueueMLFQ(pcb, newLevel);
    printf("Process %d demoted from queue %d to queue %d\n",
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
    return processQueueLevel[processId];
}

void handleMLFQNewProcess(PCB* pcb) {
    // New processes start at highest priority queue (queue 0)
    enqueueMLFQ(pcb, 0);
}

void handleMLFQQuantumExpired(PCB* pcb) {
    // Process used its full quantum - demote it
    demoteProcess(pcb);
}

void handleMLFQProcessYielded(PCB* pcb) {
    // Process yielded before quantum expired - keep at same priority
    int currentLevel = processQueueLevel[pcb->id];
    enqueueMLFQ(pcb, currentLevel);
    printf("Process %d yielded, stays at queue %d\n", pcb->id, currentLevel);
}