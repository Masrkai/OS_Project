
## Key RR Algorithm Features

1. **Time Quantum Check**: First checks if the current process has used up its time quantum
   - If quantum NOT expired → Continue current process (no preemption)
   - If quantum expired → Move to ready queue and select next

2. **FIFO Selection**: Always selects the **first process** from the ready queue (strict FIFO order)
   - No priority comparison needed
   - Fair scheduling - every process gets equal CPU time

3. **Circular Queue Behavior**:
   - When a process's quantum expires, it moves to the **end** of the ready queue
   - Creates a circular/round-robin pattern through all processes

4. **Time Slice Calculation**:
   - Execute time = min(Time Quantum, Remaining Time)
   - Prevents executing more than what's needed if process finishes before quantum

5. **Timer Setup**: The scheduler sets a timer interrupt for the time quantum, which will automatically trigger the next scheduling decision when it expires

6. **Context Switching**: Only occurs when switching between different processes, not when continuing the same one

**Main Difference from HPF**: RR doesn't care about priorities it treats all processes equally and gives each one a fair time slice in rotation. This prevents starvation and ensures good response time for interactive processes!

```mermaid


graph TD
    Start([RR Algorithm Called]) --> Input[Input: Ready Queue<br/>Current Running Process<br/>Time Quantum Q]
    
    Input --> CheckRunning{Is There a<br/>Running Process?}
    
    CheckRunning -->|Yes| CheckQuantum{Time Quantum<br/>Expired?}
    
    CheckQuantum -->|No| Continue[Continue Current Process<br/>No Scheduling Needed]
    Continue --> Return1[Return: Current Process<br/>Preemption Flag = FALSE]
    Return1 --> End
    
    CheckQuantum -->|Yes| MoveToEnd[Move Current Process<br/>to End of Ready Queue<br/>Update PCB State = READY]
    
    CheckRunning -->|No| CheckQueue
    MoveToEnd --> CheckQueue
    
    CheckQueue{Ready Queue<br/>Empty?}
    
    CheckQueue -->|Yes| NoProcess[No Process Available<br/>CPU Idle]
    NoProcess --> ReturnNull[Return: NULL<br/>Preemption Flag = FALSE]
    ReturnNull --> End
    
    CheckQueue -->|No| SelectFirst[Select First Process<br/>from Ready Queue<br/>FIFO Order]
    
    SelectFirst --> RemoveFromQueue[Remove Selected Process<br/>from Ready Queue]
    
    RemoveFromQueue --> CheckRemaining{Process Remaining<br/>Time > 0?}
    
    CheckRemaining -->|No| Error[Error: Invalid Process<br/>Remaining Time ≤ 0]
    Error --> End
    
    CheckRemaining -->|Yes| CalcTimeSlice[Calculate Time Slice:<br/>Execute Time = min Q, Remaining Time]
    
    CalcTimeSlice --> SetQuantum[Set Process Time Quantum = Q<br/>Reset Quantum Timer]
    
    SetQuantum --> UpdatePCB[Update Selected Process PCB:<br/>- State = RUNNING<br/>- Quantum Start Time<br/>- Expected Preemption Time]
    
    UpdatePCB --> CheckContext{Was There a<br/>Previously Running<br/>Process?}
    
    CheckContext -->|Yes| ContextSwitch[Context Switch Required<br/>Save Old Process State<br/>Load New Process State]
    
    CheckContext -->|No| FirstRun[First Process to Run<br/>No Context Switch Needed]
    
    ContextSwitch --> Return2
    FirstRun --> Return2
    
    Return2[Return: Selected Process<br/>Preemption Flag = TRUE<br/>Time Slice = Calculated Time]
    
    Return2 --> Note[Note: Scheduler Sets Timer<br/>for Time Quantum Q<br/>Timer Interrupt Will Trigger<br/>Next Scheduling Decision]
    
    Note --> End([Algorithm Complete])
    
    style Start fill:#2e7d32,color:#fff
    style End fill:#c62828,color:#fff
    style CheckQuantum fill:#f57c00,color:#fff
    style SelectFirst fill:#1565c0,color:#fff
    style Return1 fill:#6a1b9a,color:#fff
    style Return2 fill:#6a1b9a,color:#fff
    style Note fill:#558b2f,color:#fff


```
