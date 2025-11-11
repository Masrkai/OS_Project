#include "../include/headers.h"

/* 
 * This file simulates a CPU-bound process
 * Each process runs for its specified runtime and notifies the scheduler when finished
 */

int remainingtime;

int main(int argc, char * argv[])
{
    initClk();
    
    // Get remaining time from command line arguments
    if (argc < 2) {
        printf("Error: Process needs remaining time argument!\n");
        return -1;
    }
    
    remainingtime = atoi(argv[1]);
    
    if (remainingtime <= 0) {
        printf("Error: Invalid remaining time!\n");
        return -1;
    }
    
    printf("Process started with remaining time: %d\n", remainingtime);
    
    // Simulate CPU-bound execution
    // The process runs until remaining time reaches 0
    int lastTime = getClk();
    
    while (remainingtime > 0)
    {
        int currentTime = getClk();
        
        // Check if one time unit has passed
        if (currentTime > lastTime) {
            int elapsed = currentTime - lastTime;
            remainingtime -= elapsed;
            lastTime = currentTime;
            
            // Debug output (can be removed in production)
            // printf("Process running... Remaining: %d\n", remainingtime);
        }
    }
    
    printf("Process finished execution!\n");
    
    // Notify parent (scheduler) that this process has finished
    // The scheduler will handle termination
    kill(getppid(), SIGUSR1);
    
    // Clean up clock resources (don't terminate the whole system)
    destroyClk(false);
    
    return 0;
}