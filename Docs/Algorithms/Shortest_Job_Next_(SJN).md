## Key SJN Algorithm Features:

1. **Non-Preemptive Nature**: 
   - If a process is already running → Let it continue until completion
   - No interruption of running processes (major difference from preemptive HPF)

2. **Shortest Burst Time Selection**: 
   - Scans all processes in the ready queue
   - Compares **total burst time** (total required CPU time)
   - Selects the process with the shortest execution time

3. **Tie-Breaking**: If multiple processes have the same burst time, applies **FCFS** by selecting the one with the earliest arrival time

4. **Run to Completion**: Once a process is selected and starts running, it executes without interruption until it finishes

5. **Only Schedules When Idle**: The algorithm only makes a scheduling decision when:
   - No process is currently running (CPU is idle)
   - A process just completed

## Key Differences from Other Algorithms:

- **vs HPF**: Uses burst time instead of priority, and is non-preemptive
- **vs RR**: No time quantum, processes run to completion
- **vs SRTN**: Uses total burst time (not remaining time), and is non-preemptive

**Advantage**: Minimizes average waiting time when you know job lengths in advance

**Disadvantage**: Can cause starvation for long jobs if short jobs keep arriving!


```mermaid

graph TD
    Start([SJN Algorithm Called]) --> Input[Input: Ready Queue<br/>Current Running Process]
    
    Input --> CheckRunning{Is There a<br/>Running Process?}
    
    CheckRunning -->|Yes| Continue[Continue Current Process<br/>Non-Preemptive Algorithm<br/>Run Until Completion]
    Continue --> Return1[Return: Current Process<br/>Preemption Flag = FALSE]
    Return1 --> End
    
    CheckRunning -->|No| CheckQueue{Ready Queue<br/>Empty?}
    
    CheckQueue -->|Yes| NoProcess[No Process Available<br/>CPU Idle]
    NoProcess --> ReturnNull[Return: NULL<br/>Preemption Flag = FALSE]
    ReturnNull --> End
    
    CheckQueue -->|No| ScanQueue[Scan All Processes<br/>in Ready Queue]
    
    ScanQueue --> InitShortest[Initialize:<br/>Shortest Time = +∞<br/>Selected Process = NULL]
    
    InitShortest --> LoopStart{More Processes<br/>to Check?}
    
    LoopStart -->|Yes| GetNext[Get Next Process<br/>from Ready Queue]
    
    GetNext --> ReadBurst[Read Process Burst Time<br/>Total Required CPU Time<br/>from PCB]
    
    ReadBurst --> Compare{Burst Time <<br/>Shortest Time?}
    
    Compare -->|Yes| UpdateShortest[Update:<br/>Shortest Time = This Burst Time<br/>Selected Process = This Process]
    
    Compare -->|No| LoopStart
    
    UpdateShortest --> CheckTie{Multiple Processes<br/>with Same Burst Time?}
    
    CheckTie -->|Yes| FCFS[Apply FCFS Tie-Breaking<br/>Select Process with<br/>Earliest Arrival Time]
    
    CheckTie -->|No| LoopStart
    
    FCFS --> LoopStart
    
    LoopStart -->|No| ValidateSelection{Selected Process<br/>Found?}
    
    ValidateSelection -->|No| Error[Error: No Process Selected<br/>Return NULL]
    Error --> End
    
    ValidateSelection -->|Yes| RemoveFromQueue[Remove Selected Process<br/>from Ready Queue]
    
    RemoveFromQueue --> UpdatePCB[Update Selected Process PCB:<br/>- State = RUNNING<br/>- Start Time<br/>- Will Run Until Completion]
    
    UpdatePCB --> Return2[Return: Selected Process<br/>Preemption Flag = TRUE<br/>Run to Completion]
    
    Return2 --> Note[Note: Process Will Run<br/>Without Interruption<br/>Until It Completes<br/>Non-Preemptive Behavior]
    
    Note --> End([Algorithm Complete])
    
    style Start fill:#2e7d32,color:#fff
    style End fill:#c62828,color:#fff
    style Compare fill:#1565c0,color:#fff
    style Continue fill:#f57c00,color:#fff
    style Return1 fill:#6a1b9a,color:#fff
    style Return2 fill:#6a1b9a,color:#fff
    style Note fill:#558b2f,color:#fff

```