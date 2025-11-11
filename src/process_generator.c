#include "../include/headers.h"
#include <string.h>

#define MAX_PROCESSES 100

// Process structure to hold process data
typedef struct {
    int id;
    int arrivalTime;
    int runtime;
    int priority;
} Process;

// Message structure for IPC with scheduler
typedef struct {
    long mtype;
    Process process;
} Message;

void clearResources(int);
int readProcesses(const char* filename, Process processes[]);
void createSchedulerAndClock(int algorithm, int quantum);
void sendProcessesToScheduler(Process processes[], int count, int msgqid);

// Global variables for cleanup
int msgqid = -1;
pid_t schedulerPid = -1;
pid_t clockPid = -1;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    
    Process processes[MAX_PROCESSES];
    int processCount = 0;
    int algorithm;
    int quantum = 0;
    
    // 1. Read the input files
    printf("Reading processes from file...\n");
    processCount = readProcesses("processes.txt", processes);
    
    if (processCount == 0) {
        printf("No processes found or error reading file!\n");
        return -1;
    }
    
    printf("Successfully read %d processes\n", processCount);
    
    // 2. Ask the user for the chosen scheduling algorithm and its parameters
    printf("\nChoose the scheduling algorithm:\n");
    printf("1. Preemptive Highest Priority First (HPF)\n");
    printf("2. Shortest Job Next (SJN)\n");
    printf("3. Round Robin (RR)\n");
    printf("Enter choice (1-3): ");
    scanf("%d", &algorithm);
    
    if (algorithm < 1 || algorithm > 3) {
        printf("Invalid algorithm choice!\n");
        return -1;
    }
    
    // If Round Robin, ask for quantum
    if (algorithm == 3) {
        printf("Enter time quantum for Round Robin: ");
        scanf("%d", &quantum);
        if (quantum <= 0) {
            printf("Invalid quantum value!\n");
            return -1;
        }
    }
    
    // 3. Create message queue for IPC
    key_t msgkey = ftok(".", 'M');
    msgqid = msgget(msgkey, IPC_CREAT | 0644);
    if (msgqid == -1) {
        perror("Error creating message queue");
        return -1;
    }
    printf("Message queue created with ID: %d\n", msgqid);
    
    // 4. Create the clock process
    clockPid = fork();
    if (clockPid == 0) {
        // Child process - run clock
        execl("./clk.out", "clk.out", NULL);
        perror("Error executing clock");
        exit(-1);
    } else if (clockPid == -1) {
        perror("Error forking clock");
        clearResources(0);
        return -1;
    }
    
    printf("Clock process created with PID: %d\n", clockPid);
    
    // 5. Initialize clock communication
    initClk();
    printf("Clock initialized\n");
    
    // 6. Create the scheduler process
    schedulerPid = fork();
    if (schedulerPid == 0) {
        // Child process - run scheduler
        char algoStr[10], quantumStr[10], msgqStr[10];
        sprintf(algoStr, "%d", algorithm);
        sprintf(quantumStr, "%d", quantum);
        sprintf(msgqStr, "%d", msgqid);
        
        execl("./scheduler.out", "scheduler.out", algoStr, quantumStr, msgqStr, NULL);
        perror("Error executing scheduler");
        exit(-1);
    } else if (schedulerPid == -1) {
        perror("Error forking scheduler");
        clearResources(0);
        return -1;
    }
    
    printf("Scheduler process created with PID: %d\n", schedulerPid);
    
    // Give scheduler time to initialize
    sleep(1);
    
    // 7. Main loop - send processes to scheduler at appropriate arrival times
    printf("\nStarting process generation...\n");
    sendProcessesToScheduler(processes, processCount, msgqid);
    
    // 8. Wait for scheduler to finish
    printf("Waiting for scheduler to complete...\n");
    waitpid(schedulerPid, NULL, 0);
    
    // 9. Clear resources
    printf("Process generator finished\n");
    clearResources(0);
    
    return 0;
}

// Read processes from input file
int readProcesses(const char* filename, Process processes[]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening processes file");
        return 0;
    }
    
    char line[256];
    int count = 0;
    
    while (fgets(line, sizeof(line), file) != NULL && count < MAX_PROCESSES) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        // Parse process data
        int result = sscanf(line, "%d\t%d\t%d\t%d", 
                           &processes[count].id,
                           &processes[count].arrivalTime,
                           &processes[count].runtime,
                           &processes[count].priority);
        
        if (result == 4) {
            printf("Process %d: arrival=%d, runtime=%d, priority=%d\n",
                   processes[count].id,
                   processes[count].arrivalTime,
                   processes[count].runtime,
                   processes[count].priority);
            count++;
        }
    }
    
    fclose(file);
    return count;
}

// Send processes to scheduler at their arrival times
void sendProcessesToScheduler(Process processes[], int count, int msgqid) {
    int currentProcess = 0;
    int currentTime;
    
    while (currentProcess < count) {
        currentTime = getClk();
        
        // Send all processes that have arrived at current time
        while (currentProcess < count && 
               processes[currentProcess].arrivalTime <= currentTime) {
            
            Message msg;
            msg.mtype = 1; // Message type for new process
            msg.process = processes[currentProcess];
            
            if (msgsnd(msgqid, &msg, sizeof(Process), 0) == -1) {
                perror("Error sending process to scheduler");
            } else {
                printf("Sent process %d to scheduler at time %d\n", 
                       processes[currentProcess].id, currentTime);
            }
            
            currentProcess++;
        }
        
        // Small sleep to avoid busy waiting
        if (currentProcess < count) {
            usleep(100000); // 100ms
        }
    }
    
    // Send termination signal to scheduler (message type 2)
    Message msg;
    msg.mtype = 2;
    msg.process.id = -1; // Special ID indicating end of processes
    msgsnd(msgqid, &msg, sizeof(Process), 0);
    printf("Sent termination signal to scheduler\n");
}

// Clear all IPC resources
void clearResources(int signum) {
    printf("\nCleaning up resources...\n");
    
    // Remove message queue
    if (msgqid != -1) {
        if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
            perror("Error removing message queue");
        } else {
            printf("Message queue removed\n");
        }
    }
    
    // Terminate clock and scheduler if still running
    if (clockPid > 0) {
        kill(clockPid, SIGINT);
    }
    if (schedulerPid > 0) {
        kill(schedulerPid, SIGINT);
    }
    
    // Destroy clock resources
    destroyClk(true);
    
    if (signum == SIGINT) {
        printf("Process generator interrupted!\n");
        exit(0);
    }
}