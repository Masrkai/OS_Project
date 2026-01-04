#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "../include/headers.h"

pid_t clock_pid;

// This setup runs before each test to start the clock process
void setup(void) {
    clock_pid = fork();
    if (clock_pid == 0) {
        // Child process: Execute the clock logic
        // For testing, we can use execl if clk is compiled,
        // or just call a modified main-like function.
        char *args[] = {"./../release/clk.out", NULL};
        execv("./../release/clk.out", args);
        exit(0);
    }
    // Give the clock a moment to create the file and SHM
    sleep(1);
}

// This teardown runs after each test to clean up
void teardown(void) {
    if (clock_pid > 0) {
        kill(clock_pid, SIGINT);
        waitpid(clock_pid, NULL, 0);
    }
    // Cleanup files just in case the signal handler missed them
    remove(KEY_FILE);
    remove(".osclock_marker");
}

Test(clock_suite, test_clock_initialization, .init = setup, .fini = teardown) {
    // 1. Test initClk
    initClk();
    cr_assert_not_null(shmaddr, "shmaddr should be attached after initClk");

    // 2. Test initial value
    int initial_time = getClk();
    cr_expect(initial_time >= 0, "Clock should start at 0 or greater");

    // 3. Test progression (Wait 2 seconds and check if time advanced)
    sleep(2);
    int later_time = getClk();
    cr_assert(later_time > initial_time, "Clock should increment over time. Expected > %d, got %d", initial_time, later_time);

    destroyClk(false);
}

Test(clock_suite, test_key_file_creation, .init = setup, .fini = teardown) {
    FILE *file = fopen(KEY_FILE, "r");
    cr_assert_not_null(file, "Key file should exist after clock starts");

    int key;
    cr_assert(fscanf(file, "%d", &key) == 1, "Key file should contain a valid integer key");
    fclose(file);
}