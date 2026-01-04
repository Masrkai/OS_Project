#include <criterion/criterion.h>
#include <criterion/hooks.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#define PROCESS_PATH "./../release/process.out"

// Helper to check if a process is still alive
bool is_process_running(pid_t pid) {
    return kill(pid, 0) == 0;
}

TestSuite(process_behavior);

// 1. Test Argument Validation
Test(process_behavior, test_no_arguments) {
    pid_t pid = fork();
    if (pid == 0) {
        // Run with no arguments
        execl(PROCESS_PATH, PROCESS_PATH, NULL);
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);

    // argc < 2 returns -1, which manifests as exit code 255
    cr_assert(WIFEXITED(status));
    cr_assert_eq(WEXITSTATUS(status), 255, "Process should exit with -1 (255) when no args provided");
}

// 2. Test Execution and Signal Handling (The "Scheduler" perspective)
Test(process_behavior, test_lifecycle_and_signals) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(PROCESS_PATH, PROCESS_PATH, "5", NULL);
        exit(1);
    }

    // Give it a moment to start
    usleep(100000);

    // Verify it's running
    cr_assert(is_process_running(pid), "Process should be running in its busy loop.");

    // Simulate Scheduler STOP (SIGSTOP)
    cr_log_info("Sending SIGSTOP to process %d", pid);
    kill(pid, SIGSTOP);

    int status;
    // WUNTRACED allows us to see if it stopped
    waitpid(pid, &status, WUNTRACED);
    cr_assert(WIFSTOPPED(status), "Process should have stopped upon receiving SIGSTOP.");

    // Simulate Scheduler RESUME (SIGCONT)
    cr_log_info("Sending SIGCONT to process %d", pid);
    kill(pid, SIGCONT);
    usleep(1000);
    cr_assert(is_process_running(pid), "Process should be running again after SIGCONT.");

    // Simulate Scheduler TERMINATION (SIGKILL)
    cr_log_info("Terminating process %d", pid);
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);

    cr_assert(WIFSIGNALED(status), "Process should have been terminated by a signal.");
    cr_assert_eq(WTERMSIG(status), SIGKILL);
}