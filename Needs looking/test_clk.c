#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define SHKEY 300  // same key used in headers.h

int main() {
    int shmid;
    int *shmaddr;

    printf("Starting clk integration test...\n");

    // Give clk time to start
    sleep(1);

    // Attach to shared memory
    shmid = shmget(SHKEY, 4, 0444);
    if (shmid == -1) {
        perror("Test failed: shmget");
        return 1;
    }

    shmaddr = (int *)shmat(shmid, NULL, 0);
    if ((long)shmaddr == -1) {
        perror("Test failed: shmat");
        return 1;
    }

    int t1 = *shmaddr;
    sleep(2);
    int t2 = *shmaddr;

    printf("Clock values: %d -> %d\n", t1, t2);

    if (t2 <= t1) {
        printf("❌ FAIL: Clock did not increment\n");
    } else {
        printf("✅ PASS: Clock increments correctly\n");
    }

    shmdt(shmaddr);
    return 0;
}
