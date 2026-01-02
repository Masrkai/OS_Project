 C-LOOK Disk Scheduling Algorithm Documentation

 1. Introduction
C-LOOK (Circular LOOK) is a disk scheduling algorithm that improves seek time efficiency by servicing requests in one direction, jumping to the furthest request when needed, and then continuing again. It is an optimized version of LOOK that avoids unnecessary movement.

2. How C-LOOK Works
1. The disk head moves in a single direction (e.g., towards higher cylinder numbers).
2. It services all the requests in that direction until reaching the highest requested cylinder.
3. Instead of moving to the physical end of the disk, it **jumps directly** to the lowest requested cylinder.
4. It continues servicing requests again in the same direction.

This approach reduces total head movement compared to SCAN and LOOK.

3. Example Flow
Given requests: `98, 183, 37, 122, 14, 124, 65, 67`  
Head at: `53`

 Steps
- Sort requests: `14, 37, 65, 67, 98, 122, 124, 183`
- Move right from 53 → service `65, 67, 98, 122, 124, 183`
- Jump to lowest (`14`)
- Continue → service `14, 37`

 4. Advantages
- Reduces unnecessary movement to disk edges.
- More efficient than SCAN and LOOK in many scenarios.
- Predictable and fair ordering.

5. Disadvantages
- Not always optimal depending on request distribution.
- Jumping can cause uneven delay for some requests.

 6. Pseudocode
```
1. Take list of requests and head position.
2. Sort the request list.
3. Split requests into:
   - Right-side: requests ≥ head
   - Left-side: requests < head
4. Service right-side in order.
5. Jump to smallest request.
6. Service left-side in order.
7. Calculate total head movement.
```

 7. C Implementation (Core Logic)
```
#include <stdio.h>
#include <stdlib.h>

int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int main() {
    int requests[] = {98,183,37,122,14,124,65,67};
    int size = 8;
    int head = 53;
    int totalMovement = 0;

    qsort(requests, size, sizeof(int), compare);

    int i, startIndex = 0;
    for (i = 0; i < size; i++) {
        if (requests[i] >= head) {
            startIndex = i;
            break;
        }
    }

    int current = head;

    for (i = startIndex; i < size; i++) {
        totalMovement += abs(requests[i] - current);
        current = requests[i];
    }

    for (i = 0; i < startIndex; i++) {
        totalMovement += abs(requests[i] - current);
        current = requests[i];
    }

    printf("Total Head Movement: %d\n", totalMovement);
    return 0;
}
```

 8. Simulation Input Format
Example (to integrate in your project):
```
Enter number of requests:
Enter request queue:
Enter head position:
```

 9. Conclusion
C-LOOK is an efficient disk scheduling algorithm that minimizes total head movement by skipping unnecessary traversal to disk ends and servicing requests in a circular manner.
