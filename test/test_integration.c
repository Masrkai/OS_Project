#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/msg.h>

void suite_init(void) {
    if (chdir("../release") != 0) {
        perror("Failed to chdir to ../release");
        cr_assert_fail("Could not switch to ../release directory");

        sleep(30);
    }
}

TestSuite(integration_tests,
         .init = suite_init, .timeout = 20,
         .disabled = false
);


// Helper: Create test processes file
void create_integration_test_file(const char *filename, int num) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "#id arrival runtime priority\n");
    for (int i = 1; i <= num; i++) {
        fprintf(f, "%d\t%d\t%d\t%d\n", i, i * 2, 2, i % 5);
    }
    fclose(f);
}

// Helper: Clean up test artifacts
void cleanup_test_files() {
    remove("processes.txt");
    remove("scheduler.log");
    remove("scheduler.perf");
    remove(".osclock_marker");
    remove(".clock_key");
}

// Test: Complete system with HPF algorithm
Test(integration_tests, test_hpf_complete_run, .init = cleanup_test_files, .fini = cleanup_test_files) {
    create_integration_test_file("processes.txt", 3);

    // This would require a modified process_generator that exits after completion
    // For now, we test the components can start together

    pid_t gen_pid = fork();
    if (gen_pid == 0) {
        // Redirect stdin to provide algorithm choice
        FILE *input = fopen("test_input.txt", "w");
        fprintf(input, "1\n"); // HPF
        fclose(input);

        freopen("test_input.txt", "r", stdin);
        execl("process_generator.out", "process_generator.out", NULL);
        exit(1);
    }

    sleep(5); // Let system run

    // Check if log file was created
    FILE *log = fopen("scheduler.log", "r");
    cr_assert_not_null(log, "Scheduler log should be created in integration test");

    char line[256];
    int process_count = 0;
    while (fgets(line, sizeof(line), log) != NULL) {
        if (strstr(line, "started") != NULL) {
            process_count++;
        }
    }
    fclose(log);

    cr_assert_gt(process_count, 0, "At least one process should have started");

    // Cleanup
    kill(gen_pid, SIGKILL);
    waitpid(gen_pid, NULL, 0);
    remove("test_input.txt");
}

// Test: Complete system with Round Robin
Test(integration_tests, test_rr_complete_run, .init = cleanup_test_files, .fini = cleanup_test_files) {
    create_integration_test_file("processes.txt", 3);

    pid_t gen_pid = fork();
    if (gen_pid == 0) {
        FILE *input = fopen("test_input_rr.txt", "w");
        fprintf(input, "3\n2\n"); // RR with quantum 2
        fclose(input);

        freopen("test_input_rr.txt", "r", stdin);
        execl("process_generator.out", "process_generator.out", NULL);
        exit(1);
    }

    sleep(5);

    FILE *log = fopen("scheduler.log", "r");
    cr_assert_not_null(log, "Scheduler log should exist");

    char line[256];
    bool found_stopped = false;
    while (fgets(line, sizeof(line), log) != NULL) {
        if (strstr(line, "stopped") != NULL) {
            found_stopped = true;
            break;
        }
    }
    fclose(log);

    cr_assert(found_stopped, "Round Robin should have stopped at least one process");

    kill(gen_pid, SIGKILL);
    waitpid(gen_pid, NULL, 0);
    remove("test_input_rr.txt");
}

// Test: Performance metrics are generated
Test(integration_tests, test_performance_metrics_generated, .init = cleanup_test_files, .fini = cleanup_test_files) {
    create_integration_test_file("processes.txt", 2);

    pid_t gen_pid = fork();
    if (gen_pid == 0) {
        FILE *input = fopen("test_input_sjn.txt", "w");
        fprintf(input, "2\n"); // SJN
        fclose(input);

        freopen("test_input_sjn.txt", "r", stdin);
        execl("process_generator.out", "process_generator.out", NULL);
        exit(1);
    }

    sleep(8); // Give enough time for processes to complete

    // Check performance file
    FILE *perf = fopen("scheduler.perf", "r");
    if (perf) {
        char line[256];
        bool found_cpu = false, found_wta = false;

        while (fgets(line, sizeof(line), perf) != NULL) {
            if (strstr(line, "CPU utilization")) found_cpu = true;
            if (strstr(line, "Avg WTA")) found_wta = true;
        }

        cr_assert(found_cpu, "Performance file should contain CPU utilization");
        cr_assert(found_wta, "Performance file should contain Avg WTA");

        fclose(perf);
    }

    kill(gen_pid, SIGKILL);
    waitpid(gen_pid, NULL, 0);
    remove("test_input_sjn.txt");
}

// Test: MLFQ aging mechanism
Test(integration_tests, test_mlfq_system, .init = cleanup_test_files, .fini = cleanup_test_files) {
    // Create processes with varying runtimes
    FILE *f = fopen("processes.txt", "w");
    fprintf(f, "#id arrival runtime priority\n");
    fprintf(f, "1\t0\t5\t5\n");  // Longer process
    fprintf(f, "2\t1\t2\t3\n");  // Shorter process
    fprintf(f, "3\t2\t3\t4\n");
    fclose(f);

    pid_t gen_pid = fork();
    if (gen_pid == 0) {
        FILE *input = fopen("test_input_mlfq.txt", "w");
        fprintf(input, "4\n"); // MLFQ
        fclose(input);

        freopen("test_input_mlfq.txt", "r", stdin);
        execl("process_generator.out", "process_generator.out", NULL);
        exit(1);
    }

    sleep(10);

    // Check that log shows MLFQ behavior
    FILE *log = fopen("scheduler.log", "r");
    if (log) {
        char line[256];
        int state_changes = 0;

        while (fgets(line, sizeof(line), log) != NULL) {
            if (strstr(line, "stopped") || strstr(line, "resumed")) {
                state_changes++;
            }
        }

        cr_log_info("MLFQ state changes: %d", state_changes);
        cr_assert_gt(state_changes, 0, "MLFQ should show process state changes");

        fclose(log);
    }

    kill(gen_pid, SIGKILL);
    waitpid(gen_pid, NULL, 0);
    remove("test_input_mlfq.txt");
}

// Test: Process arrival time handling
Test(integration_tests, test_arrival_time_ordering, .init = cleanup_test_files, .fini = cleanup_test_files) {
    // Create processes with staggered arrivals
    FILE *f = fopen("processes.txt", "w");
    fprintf(f, "#id arrival runtime priority\n");
    fprintf(f, "1\t0\t2\t1\n");
    fprintf(f, "2\t5\t2\t2\n");
    fprintf(f, "3\t10\t2\t3\n");
    fclose(f);

    pid_t gen_pid = fork();
    if (gen_pid == 0) {
        FILE *input = fopen("test_input_arr.txt", "w");
        fprintf(input, "1\n"); // HPF
        fclose(input);

        freopen("test_input_arr.txt", "r", stdin);
        execl("process_generator.out", "process_generator.out", NULL);
        exit(1);
    }

    sleep(12);

    FILE *log = fopen("scheduler.log", "r");
    if (log) {
        char line[256];
        int process_1_start = -1, process_2_start = -1, process_3_start = -1;

        while (fgets(line, sizeof(line), log) != NULL) {
            int time, id;
            char state[20];
            if (sscanf(line, "At time %d process %d %s", &time, &id, state) == 3) {
                if (strcmp(state, "started") == 0) {
                    if (id == 1) process_1_start = time;
                    if (id == 2) process_2_start = time;
                    if (id == 3) process_3_start = time;
                }
            }
        }

        cr_log_info("Process start times: P1=%d, P2=%d, P3=%d",
                    process_1_start, process_2_start, process_3_start);

        // Processes should start at or after their arrival time
        if (process_2_start > 0) {
            cr_assert_geq(process_2_start, 5, "Process 2 should start at or after t=5");
        }

        fclose(log);
    }

    kill(gen_pid, SIGKILL);
    waitpid(gen_pid, NULL, 0);
    remove("test_input_arr.txt");
}