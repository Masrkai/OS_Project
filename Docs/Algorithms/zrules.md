The scheduling algorithm only works on the processes in the ready queue. (Processes
that have already arrived.)
The scheduler should be able to

1.  Start a new process. (Fork it and give it its parameters.)

2.  Switch between two processes according to the scheduling algorithm. (Stop the
old process and save its state and start/resume another one.)

3.  Keep a process control block (PCB) for each process in the system. A PCB should
keep track of the state of a process; running/waiting, execution time, remaining
time, waiting time, etc.

4.  Delete the data of a process when it gets notifies that it finished. When a process
finishes it should notify the scheduler on termination, the scheduler does NOT
terminate the process.

5.  Report the following information
    (a)  CPU utilization.
    (b)  Average weighted turnaround time.
    (c)  Average waiting time.
    (d)  Standard deviation for average weighted turnaround time.