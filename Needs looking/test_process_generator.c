#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <msgqid>\n", argv[0]);
        return 1;
    }
    
    int msgqid = atoi(argv[1]);
    Message msg;
    
    printf("Test scheduler started with msgqid=%d...\n", msgqid);
    
    while (1) {
        // Receive message of any type (mtype = 0 means any message)
        if (msgrcv(msgqid, &msg, sizeof(Process), 0, 0) == -1) {
            perror("msgrcv failed");
            return 1;
        }
        
        // Check for termination message
        if (msg.mtype == 2) {
            printf("Received termination message\n");
            break;
        }
        
        // Log received process
        printf("Received process: ID=%d arrival=%d runtime=%d priority=%d (mtype=%ld)\n",
               msg.process.id,
               msg.process.arrivalTime,
               msg.process.runtime,
               msg.process.priority,
               msg.mtype);
    }
    
    printf("Test scheduler exiting normally\n");
    return 0;
}