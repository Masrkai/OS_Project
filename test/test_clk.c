#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <sys/shm.h>
#include <signal.h>

#include "../include/headers.h" // Assume your code is here

#define TEST_SHKEY 300

int temp_shmid = -1;

// This runs BEFORE every test
void suite_setup(void) {
    // Create the shared memory segment so initClk doesn't loop forever
    temp_shmid = shmget(TEST_SHKEY, sizeof(int), IPC_CREAT | 0666);
    cr_assert(temp_shmid != -1, "Failed to create shared memory for test");
    
    // Initialize the clock value to 0
    int *addr = shmat(temp_shmid, NULL, 0);
    *addr = 0;
    shmdt(addr);

    // Redirect stdout so we can check printfs if needed
    cr_redirect_stdout();
}

// This runs AFTER every test
void suite_teardown(void) {
    // Clean up the shared memory segment from the OS
    shmctl(temp_shmid, IPC_RMID, NULL);
}

// --------------------------------------------------------
// TESTS
// --------------------------------------------------------

Test(clk_suite, test_init_and_get, .init = suite_setup, .fini = suite_teardown) {
    initClk();
    
    // Check if shmaddr was actually assigned
    cr_assert_not_null(shmaddr, "shmaddr should not be null after initClk");
    
    // Verify initial value
    cr_assert_eq(getClk(), 0, "getClk should return 0 initially");
    
    // Manually change memory and check if getClk reflects it
    *shmaddr = 42;
    cr_assert_eq(getClk(), 42, "getClk should reflect changes in shared memory");
    
    destroyClk(false);
}

Test(clk_suite, test_destroy_signal, .init = suite_setup, .fini = suite_teardown, .signal = SIGINT) {
    initClk();
    
    // This should trigger SIGINT to the process group
    // Criterion will catch this and mark the test as passed because of .signal = SIGINT
    destroyClk(true);
}