#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdbool.h>

#pragma once

#define SHM_PROJ 'C'
#define KEY_FILE ".osclock_key"

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk() { return *shmaddr; }

/*
 * All process call this function at the beginning to establish communication
 * between them and the clock module. The clock must be started first.
 * This function reads the shared memory key from a file created by clk.c
 */
void initClk() {
  FILE *keyfile = NULL;
  key_t key;
  int attempts = 0;
  const int MAX_ATTEMPTS = 30; // Wait up to 30 seconds
  
  // Wait for the clock to create the key file
  while (!keyfile && attempts < MAX_ATTEMPTS) {
    keyfile = fopen(KEY_FILE, "r");
    if (!keyfile) {
      if (attempts == 0) {
        printf("Waiting for clock to initialize...\n");
      }
      sleep(1);
      attempts++;
    }
  }
  
  if (!keyfile) {
    fprintf(stderr, "Error: Clock not initialized after %d seconds!\n", MAX_ATTEMPTS);
    fprintf(stderr, "Make sure to start clk first.\n");
    exit(-1);
  }
  
  // Read the key from file
  if (fscanf(keyfile, "%d", &key) != 1) {
    fclose(keyfile);
    fprintf(stderr, "Error: Could not read key from %s\n", KEY_FILE);
    exit(-1);
  }
  fclose(keyfile);
  
  // Get the shared memory segment
  int shmid = shmget(key, 4, 0444);
  if (shmid == -1) {
    perror("Error in shmget");
    exit(-1);
  }
  
  // Attach to shared memory
  shmaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)shmaddr == -1) {
    perror("Error in attaching the shm!");
    exit(-1);
  }
  
  printf("Successfully connected to clock (key: %d)\n", key);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Input: terminateAll: a flag to indicate whether this is the end of
 * simulation. It terminates the whole system and releases resources.
 */
void destroyClk(bool terminateAll) {
  shmdt(shmaddr);
  if (terminateAll) {
    killpg(getpgrp(), SIGINT);
  }
}