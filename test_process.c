
#include "../include/headers.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

volatile sig_atomic_t processFinished = 0;

/* Signal handler for SIGUSR1 */
void processFinishedHandler(int signum)
{
    processFinished = 1;
}

int main()
{
    printf("=== Integration Test: CPU-bound Process ===\n");

    // Initialize clock
    initClk();

    // Register signal handler
    signal(SIGUSR1, processFinishedHandler);

    int runtime = 3;   // test runtime (3 clock ticks)

    int pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        destroyClk(true);
        return 1;
    }

    if (pid == 0)
    {
        // Child: execute the process
        char runtimeStr[10];
        sprintf(runtimeStr, "%d", runtime);

        execl("./process.out", "process.out", runtimeStr, NULL);

        perror("execl failed");
        exit(1);
    }

    // Parent: scheduler simulation
    printf("[TEST] Process forked with PID = %d\n", pid);

    int startTime = getClk();

    // Wait until process signals completion
    while (!processFinished)
    {
        // Busy wait (scheduler behavior)
    }

    int endTime = getClk();

    printf("[TEST] SIGUSR1 received from process\n");

    // Ensure child actually terminated
    waitpid(pid, NULL, 0);

    int elapsed = endTime - startTime;

    printf("[TEST] Expected runtime: %d\n", runtime);
    printf("[TEST] Actual elapsed time: %d\n", elapsed);

    // Simple validation
    if (elapsed >= runtime)
        printf("[PASS] Process ran for correct duration\n");
    else
        printf("[FAIL] Process finished too early\n");

    destroyClk(true);

    printf("=== Integration Test Finished ===\n");
    return 0;
}
