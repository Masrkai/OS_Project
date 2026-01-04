#include "../include/headers.h"
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return -1;
  }

  int remainingtime = atoi(argv[1]);
  
  printf("Process started with remaining time: %d (PID: %d)\n", 
         remainingtime, getpid());

  // Simulate CPU-bound work
  // The scheduler manages our actual remaining time via SIGSTOP/SIGCONT/SIGKILL
  while (1) {
    // Just do busy work - scheduler will kill us when done
    for (volatile long i = 0; i < 1000000; i++);
  }

  return 0;
}