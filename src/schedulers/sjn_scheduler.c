#include "../../include/schedule.h"
#include <stdio.h>

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