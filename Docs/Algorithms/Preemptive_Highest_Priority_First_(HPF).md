## Key Algorithm Steps:

1. **Input Processing**: Takes the ready queue and current running process as input

2. **Queue Preparation**: If there's a running process, it's added back to the ready queue for comparison (this is what makes it preemptive!)

3. **Priority Scanning**:
   - Iterates through all processes in the ready queue
   - Compares each process's priority value
   - Tracks the process with the highest priority

4. **Tie-Breaking**: If multiple processes have the same highest priority, it applies **FCFS** (First Come First Served) by selecting the one with the earliest arrival time

5. **Preemption Decision**:
   - If the selected process is already running → No preemption needed
   - If a different process has higher priority → Preemption occurs
   - The old process stays in the ready queue with state = READY

6. **Output**: Returns the selected process and a preemption flag indicating whether context switching is needed

This algorithm ensures that at every scheduling decision point, the process with the highest priority gets CPU time, even if it means preempting a currently running lower-priority process!



```mermaid

graph TD
    Start([HPF Algorithm Called]) --> Input[Input: Ready Queue<br/>Current Running Process]

    Input --> CheckRunning{Is There a<br/>Running Process?}

    CheckRunning -->|Yes| AddToQueue[Add Running Process<br/>Back to Ready Queue<br/>for Comparison]
    CheckRunning -->|No| ScanQueue

    AddToQueue --> ScanQueue[Scan All Processes<br/>in Ready Queue]

    ScanQueue --> InitHighest[Initialize:<br/>Highest Priority = -∞<br/>Selected Process = NULL]

    InitHighest --> LoopStart{More Processes<br/>to Check?}

    LoopStart -->|Yes| GetNext[Get Next Process<br/>from Ready Queue]

    GetNext --> ReadPriority[Read Process Priority<br/>from PCB]

    ReadPriority --> Compare{Process Priority<br/>> Highest Priority?}

    Compare -->|Yes| UpdateHighest[Update:<br/>Highest Priority = This Priority<br/>Selected Process = This Process]
    Compare -->|No| LoopStart

    UpdateHighest --> CheckTie{Multiple Processes<br/>with Same Priority?}

    CheckTie -->|Yes| FCFS[Apply FCFS Tie-Breaking<br/>Select Process with<br/>Earliest Arrival Time]
    CheckTie -->|No| LoopStart

    FCFS --> LoopStart

    LoopStart -->|No| ValidateSelection{Selected Process<br/>Found?}

    ValidateSelection -->|No| Error[Error: No Process Selected<br/>Return NULL]
    Error --> End

    ValidateSelection -->|Yes| CheckPreempt{Selected Process ==<br/>Currently Running?}

    CheckPreempt -->|Yes| NoPreempt[No Preemption Needed<br/>Continue Current Process]
    NoPreempt --> Return1[Return: Current Process<br/>Preemption Flag = FALSE]
    Return1 --> End

    CheckPreempt -->|No| RemoveFromQueue[Remove Selected Process<br/>from Ready Queue]

    RemoveFromQueue --> CheckWasRunning{Was There a<br/>Running Process?}

    CheckWasRunning -->|Yes| KeepInQueue[Keep Old Process<br/>in Ready Queue<br/>Update PCB State = READY]
    CheckWasRunning -->|No| NoAction[No Action Needed<br/>No Process to Preempt]

    KeepInQueue --> Return2
    NoAction --> Return2

    Return2[Return: Selected Process<br/>Preemption Flag = TRUE]

    Return2 --> End([Algorithm Complete])

    style Start fill:#2e7d32,color:#fff
    style End fill:#c62828,color:#fff
    style Compare fill:#1565c0,color:#fff
    style CheckPreempt fill:#f57c00,color:#fff
    style Return1 fill:#6a1b9a,color:#fff
    style Return2 fill:#6a1b9a,color:#fff
```





