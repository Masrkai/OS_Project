#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "../include/headers.h"

static bool signal_received = false;

// Signal handler to catch the process's completion notification
void handle_sigusr1(int sig) {
    signal_received = true;
}

// Setup: Start the clock process so process.c can connect to it
void setup_proc(void) {
    signal_received = false;
    signal(SIGUSR1, handle_sigusr1);

    if (fork() == 0) {
        execl("./../release/clk.out", "./../release/clk.out", NULL);
        exit(0);
    }
    sleep(1); // Wait for clock SHM to exist
}

void teardown_proc(void) {
    // Kill the clock and any leftover processes
    system("pkill -9 clk");
    remove(KEY_FILE);
    remove(".osclock_marker");
}

TestSuite(process_logic, .init = setup_proc, .fini = teardown_proc);

Test(process_logic, test_process_execution_flow) {
    int runtime = 3;
    cr_log_info("Starting test: Process with %d unit runtime.", runtime);

    // 1. Fork the process under test
    pid_t worker_pid = fork();
    if (worker_pid == 0) {
        char runtime_str[10];
        sprintf(runtime_str, "%d", runtime);
        execl("./../release/process.out", "./../release/process.out", runtime_str, NULL);
        exit(1);
    }

    // 2. Connect to the clock in the test process to manipulate time
    initClk();
    int start_time = getClk();
    cr_log_info("Initial Clock: %d. Manually ticking the clock...", start_time);

    // 3. Simulate time passing by incrementing SHM directly
    // Process expects 3 units. We tick 4 times to be sure.
    for (int i = 1; i <= 4; i++) {
        sleep(1); // Real sleep so process loop can catch the change
        (*shmaddr)++; 
        cr_log_info("Tick %d -> Clock is now %d", i, getClk());
    }

    // 4. Verify the process sent the finish signal (SIGUSR1)
    cr_expect(signal_received, "Process should have sent SIGUSR1 to parent upon completion.");

    // 5. Verify the process actually terminated
    int status;
    waitpid(worker_pid, &status, WNOHANG);
    cr_assert(WIFEXITED(status), "Process should have exited by now.");
    
    cr_log_info("Process successfully tracked time and notified parent.");
    destroyClk(false);
}