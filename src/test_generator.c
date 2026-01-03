#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define null 0

struct processData {
  int arrivaltime;
  int priority;
  int runningtime;
  int id;
};

// Testable function that generates process data
// Returns 0 on success, -1 on error
int generate_processes(FILE *output, int no, unsigned int seed) {
  if (output == null || no <= 0) {
    return -1;
  }

  srand(seed);

  fprintf(output, "#id arrival runtime priority\n");

  struct processData pData;
  pData.arrivaltime = 1;

  for (int i = 1; i <= no; i++) {
    pData.id = i;
    pData.arrivaltime += rand() % (11);
    pData.runningtime = rand() % (30);
    pData.priority = rand() % (11);
    fprintf(output, "%d\t%d\t%d\t%d\n", pData.id, pData.arrivaltime,
            pData.runningtime, pData.priority);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  FILE *pFile;
  pFile = fopen("processes.txt", "w");
  if (pFile == null) {
    fprintf(stderr, "Error: Could not open processes.txt for writing\n");
    return 1;
  }

  int no;
  printf("Please enter the number of processes you want to generate: ");
  scanf("%d", &no);

  int result = generate_processes(pFile, no, time(null));

  fclose(pFile);

  return result == 0 ? 0 : 1;
}