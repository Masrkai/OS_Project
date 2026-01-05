/*
 * This file represents an emulated clock for simulation purpose only.
 * It is not a real part of operating system!
 * This file must be started FIRST before any other process.
 */

#include "../include/headers.h"

int shmid;

/* Clear the resources before exit */
void cleanup(int signum) {
  printf("Received signal %d, cleaning up...\n", signum);

  shmctl(shmid, IPC_RMID, NULL);

  // Remove the key file
  if (remove(KEY_FILE) == 0) {
    printf("Removed key file: %s\n", KEY_FILE);
  }

  printf("Clock terminating!\n");
  exit(0);
}

/* This file represents the system clock for ease of calculations */
int main(int argc, char *argv[]) {

  if (argc > 1) {
    printf("Usage: %s\n", argv[0]);
    printf("no arguments needed\n");
  }

  printf("Clock starting\n");
  signal(SIGINT, cleanup);

  // Create a marker file for ftok (it must exist!)
  const char *marker_file = ".osclock_marker";
  FILE *marker = fopen(marker_file, "a");
  if (!marker) {
    perror("Error creating marker file");
    exit(-1);
  }
  fclose(marker);

  // Generate key using ftok with the marker file
  key_t key = ftok(marker_file, SHM_PROJ);
  if (key == -1) {
    perror("Error in ftok");
    exit(-1);
  }

  printf("Generated SHM key: %d\n", key);

  // Write the key to a file for other processes to read
  FILE *keyfile = fopen(KEY_FILE, "w");
  if (!keyfile) {
    perror("Error creating key file");
    exit(-1);
  }
  fprintf(keyfile, "%d", key);
  fclose(keyfile);
  printf("Key written to: %s\n", KEY_FILE);

  int clk = 0;

  // Create shared memory for one integer variable (4 bytes)
  shmid = shmget(key, 4, IPC_CREAT | 0644);
  if (shmid == -1) {
    perror("Error in creating shm!");
    remove(KEY_FILE); // Clean up key file
    exit(-1);
  }

  int *shmaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)shmaddr == -1) {
    perror("Error in attaching the shm in clock!");
    shmctl(shmid, IPC_RMID, NULL);
    remove(KEY_FILE); // Clean up key file
    exit(-1);
  }

  *shmaddr = clk; /* initialize shared memory */
  printf("Clock initialized successfully. Starting tick...\n");

  while (1) {
    sleep(1);
    (*shmaddr)++;
  }

  return 0;
}