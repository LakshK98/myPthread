# myPthread
A user level thread library that provides mutual exclusion between threads and a custom scheduler. 
Custom implementation of the pthread library. Finer details and performance comparison can be found in report.pdf
## Running steps:
1. Set the SCHED parameter in the Makefile to one of RR, PSJF or MLFQ to set the scheduling algorithm
2. Run Make
3. Import myprthread library to use available funtions 

## Functions
1. mypthread_create(): create a new thread
2. mypthread_yield(): Current thread voluntarily surrenders its remaining runtime for other threads to use
3. mypthread_exit(): Terminate a thread
4. mypthread_join(): Wait for thread termination
5. mypthread_mutex_init(): initialize the mutex lock 
6. mypthread_mutex_lock(): acquire a mutex lock 
7. mypthread_mutex_unlock(): release the mutex lock
8. mypthread_mutex_destroy(): destroy the mutex

## Custom scheduler (MLFQ) details:
Our implementation takes the following parameters.  
**NUM_MLFQ** Number of queues in the MLFQ (Default 3)  
**PROMOTION_DELAY** Number of quants after which promotion will be attempted (Default 10)  
**TIME_QUANTUM** quantum for the first queue (Default 500 ms)  
All levels of the queue follow the same strategy as SJF for their insert i.e. they take into account
quant_count and entry time and the queue is always kept in sorted order  
The first NUM_MLFQ-1 follow non-preemptive RR for scheduling and the time quantum for a
level is given by level_number * TIME_QUANTUM.  
The last level is a preemptive FCFS queue in which threads will run as long as a higher priority
threads comes along.

promotion/demotion:  
After a thread is executed for its assigned quantum, it gets demoted to the next level of the
queue.  
If a thread is holding a lock , it does not get demoted till it releases the lock . This is to ensure it
gets high priority so that other threads can access the locked
