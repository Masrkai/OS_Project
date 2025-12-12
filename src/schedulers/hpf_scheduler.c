#include "../../include/schedule.h"
#include <stdio.h>

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