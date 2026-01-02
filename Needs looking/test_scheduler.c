#include "../include/headers.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    int arrivalTime;
    int runtime;
    int priority;
} Process;

typedef struct {
    long mtype;
    Process process;
} Message;

int main()
{
    printf("=== Scheduler Integration Test ===\n");

    /* Create message queue */
    key_t key = ftok("integration_test.c", 65);
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        return 1;
    }

    printf("[TEST] Message Queue ID = %d\n", msgqid);

    /* Fork scheduler */
    pid_t schedulerPID = fork();

    if (schedulerPID == -1) {
        perror("fork failed");
        return 1;
    }

    if (schedulerPID == 0) {
        /* Child: run scheduler */
        char msgqidStr[10], algoStr[10], quantumStr[10];

        sprintf(algoStr, "%d", 3);     // Round Robin
        sprintf(quantumStr, "%d", 2);  // Quantum = 2
        sprintf(msgqidStr, "%d", msgqid);

        execl("./scheduler.out", "scheduler.out",
              algoStr, quantumStr, msgqidStr, NULL);

        perror("execl scheduler failed");
        exit(1);
    }

    /* Parent: acts as process generator */
    sleep(1); // allow scheduler to start

    Message msg;

    /* Send test processes */
    for (int i = 0; i < 3; i++) {
        msg.mtype = 1;
        msg.process.id = i + 1;
        msg.process.arrivalTime = i;
        msg.process.runtime = 3 + i;
        msg.process.priority = 1;

        msgsnd(msgqid, &msg, sizeof(msg.process), 0);
        printf("[TEST] Sent process %d\n", msg.process.id);

        sleep(1);
    }

    /* Send termination message */
    msg.mtype = 2;
    msgsnd(msgqid, &msg, sizeof(msg.process), 0);
    printf("[TEST] Sent termination message\n");

    /* Wait for scheduler */
    waitpid(schedulerPID, NULL, 0);

    /* Cleanup message queue */
    msgctl(msgqid, IPC_RMID, NULL);

    printf("=== Integration Test Finished Successfully ===\n");
    return 0;
}
