#include "../../include/schedule.h"
#include <stdio.h>

PCB* selectRR() {
    // In Round Robin, we simply pick the process at the front of the ready queue
    if (isEmpty(&readyQueue)) {
        return NULL;
    }

    PCB* selected = peek(&readyQueue);
    
    // Rotate the queue: move the head to the tail for next time
    // (This simulates the circular nature of RR)
    QueueNode* head = readyQueue.head;
    if (readyQueue.size > 1) {
        readyQueue.head = head->next;
        readyQueue.tail->next = head;
        head->next = NULL;
        readyQueue.tail = head;
    }

    return selected;
}