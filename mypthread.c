// File:	mypthread.c

// List all group members' names:
// iLab machine tested on:

#include "mypthread.h"
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>

#define STACK_SIZE 50240
// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
queue ready_queue;
tcb* current_tcb = NULL;
tcb* main_tcb = NULL;
int threadCount = 1;
int sched_event = -1; // 0 : timer ; 1 : thread start ; 2 thread_exit

static void schedule();



void print_q(queue *q)
{
	printf("printing queue of size : %d \n", q->size);
	q_node* cur = q->head;
	while(cur!=NULL)
	{
		printf("%d ", (*cur->t->id));
		cur = cur->next;
	}
	printf("\n");
}

void q_push(queue *q, tcb *t)
{
	q_node* qn = (q_node*) malloc(sizeof(q_node));
	qn->t = t;
	qn->next = NULL;
	if(q->head == NULL)
	{
		q->head = qn;
		q->tail = qn;
		q->size = 1;
	}
	else
	{
		q->tail->next = qn;
		q->tail = q->tail->next;
		q->size++;
	}
	printf("PRINTING after push\n");
	print_q(q);
}

q_node* q_pop(queue *q)
{
	if(q->size == 0)
		return NULL;
	q_node* qn = q->head;
	if(q->head != NULL)
	{
		q->head = q->head->next;
	}
	q->size--;
	
	printf("PRINTING after pop\n");
	print_q(q);
	return qn;
}

// void return_wrapper(void *(*function)(void*),  void * arg)
// {
// 	mypthread_exit((*function)(arg));

// }
// void return_wrapper_test(void *(*function)(void*) , void *arg)
// {

// 	int* ret = (int*) (*function)(arg);
// 	printf("return_wrapper_test %d \n", (*ret) );
// }

void init_context(ucontext_t * context)
{
	context->uc_link = NULL;
	context->uc_stack.ss_sp = malloc(STACK_SIZE);
	context->uc_stack.ss_size = STACK_SIZE;
	context->uc_stack.ss_flags = 0;
}

/* create a new thread */
int mypthread_create(mypthread_t * new_thread, pthread_attr_t * attr, void *(*function)(void*), void * arg)
{
		// YOUR CODE HERE	


		bool * run_once = (bool*) malloc (sizeof(bool));
		*run_once = false;

		printf("reached mypthread_create\n");
		
		ucontext_t *new_thread_context = (ucontext_t*)malloc(sizeof(ucontext_t));
		ucontext_t *exit_context = (ucontext_t*)malloc(sizeof(ucontext_t));

		getcontext(new_thread_context);
		getcontext(exit_context);

		void (*exit_func_ptr) () = &mypthread_exit;
		init_context(exit_context);
		makecontext(exit_context, exit_func_ptr, 0);

		init_context(new_thread_context);
		new_thread_context->uc_link = exit_context;

		void (*function_ptr) () = (void*)function;
		makecontext(new_thread_context, function_ptr, 1, arg);
		
		tcb * new_thread_tcb = (tcb*)malloc(sizeof(tcb));
		*new_thread = threadCount;
		new_thread_tcb->id = (mypthread_t*) malloc(sizeof(mypthread_t));
		*new_thread_tcb->id = threadCount;
		threadCount++;
		new_thread_tcb->context = new_thread_context;
		new_thread_tcb->status = READY;

		// sigset_t a,b;
		// sigemptyset(&a);
		// sigaddset(&a, SIGALRM);   // i want to block SIGPROF...

		// sigprocmask(SIG_BLOCK, &a, &b); 

		q_push(&ready_queue, new_thread_tcb);


		// sigprocmask(SIG_SETMASK, &b, NULL);
		printf("pushed thread %d . queue size %d \n", (*new_thread_tcb->id) , (ready_queue.size));

		if(main_tcb == NULL)
		{
			ucontext_t *main_thread_context = (ucontext_t*)malloc(sizeof(ucontext_t));
			main_thread_context->uc_stack.ss_sp = malloc(STACK_SIZE);
			main_thread_context->uc_stack.ss_size = STACK_SIZE;

			getcontext(main_thread_context);

			if(*run_once)
			{
				printf("Already run once . Returning \n");
				return 0;
			}
			else{
				printf("did not work\n");

			}
			main_thread_context->uc_link = exit_context;
			main_tcb = (tcb*)malloc(sizeof(tcb));
			int five = 5;
			// reserving id 0 for main
			main_tcb->id = (mypthread_t*) malloc(sizeof(mypthread_t));
			*main_tcb->id = 0;
			main_tcb->context = main_thread_context;
			main_tcb->status = READY;
			printf("pushed main thread %d . queue size %d \n", (*main_tcb->id) , (ready_queue.size));

			q_push(&ready_queue, main_tcb);
			// threadCount++;
			printf("pushed main thread %d . queue size %d \n", (*main_tcb->id) , (ready_queue.size));
					print_q(&ready_queue);


		}



		// void (*return_wrapper_ptr) () = &return_wrapper; 
		// void (*return_wrapper_test_ptr) () = &return_wrapper_test;
 		// makecontext(new_thread_context, return_wrapper_test_ptr, 2, function, arg);




			if(*run_once)
			{
				printf("Already run once . Returning \n");
				return 0;
			}
			else{
				printf("did not work\n");

			}

		*run_once = true;

		printf("Calling schedule from start");
		sched_event = 1;
		schedule();
		 
		// sigprocmask(SIG_SETMASK, &b, NULL);
	   // create a Thread Control Block
	   // create and initialize the context of this thread
	   // allocate heap space for this thread's stack
	   // after everything is all set, push this thread into the ready queue	
		
	return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	// YOUR CODE HERE
	
	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// int * x = (int *) value_ptr;
	// printf("Value %d", (*x));

	printf("Exit called ");
	sched_event = 2;
	schedule();
	// YOUR CODE HERE

	// preserve the return value pointer if not NULL
	// deallocate any dynamic memory allocated when starting this thread
	
	return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// YOUR CODE HERE

	// wait for a specific thread to terminate
	// deallocate any dynamic memory created by the joining thread

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	// YOUR CODE HERE
	
	//initialize data structures for this mutex

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
		// YOUR CODE HERE
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
		
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE	
	
	// update the mutex's metadata to indicate it is unlocked
	// put the thread at the front of this mutex's blocked/waiting queue in to the run queue

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE
	
	// deallocate dynamic memory allocated during mypthread_mutex_init

	return 0;
};

int sigco = 0;
void sighandler(int sig)
{
 
//  printf("signal occurred %d times\n", ++sigco);
	sched_event = 0;
	schedule();
}

void set_timer()
{

	struct itimerval it;
	struct sigaction act, oact;
	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGALRM, &act, &oact); 

	// Start itimer
	it.it_interval.tv_sec = 7;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = 3;
	it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);	
}

/* scheduler */
static void schedule()
{
	// YOUR CODE HERE
	
	// each time a timer signal occurs your library should switch in to this context
	
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ

	printf("scheduler invokded \n");
	
	if(sched_event == 1)
	{
		if(current_tcb==NULL)
		{
			if(ready_queue.head == NULL)
				return;

			current_tcb = q_pop(&ready_queue)->t;
		}
		printf("current tcb not null id : %i\n", (*current_tcb->id));
		printf("current tcb status : %i\n", (current_tcb->status));
		
		set_timer();
		
		if(current_tcb->status == READY)
		{
			printf("ready current tcb\n");
			
			current_tcb->status = RUNNING;
			ucontext_t* ready_thread_context = current_tcb->context;
		
			setcontext(ready_thread_context);

			// for ( ; ; ) ;
		}

	

	}
	else if(sched_event == 0)
	{
		if(current_tcb->status == RUNNING)
		{
			printf("running current tcb\n");
			tcb* temp_tcb  = current_tcb;
			current_tcb->status = READY;
			print_q(&ready_queue);
			q_push(&ready_queue, current_tcb);
			print_q(&ready_queue);

			current_tcb = q_pop(&ready_queue)->t;

			current_tcb->status = RUNNING;
			printf(" Running tcb with id  : %i\n", (* current_tcb->id));
			printf("  temp with id  : %i\n", (* temp_tcb->id));

			printf(" SWAPPING \n");

			swapcontext(temp_tcb->context, current_tcb->context);

		}
		else
		{
			printf(" ERROR: no running running current tcb\n");

		}



	}
	else if (sched_event = 2)
	{		
		printf("current tcb id : %i is exiting\n", (*current_tcb->id));
		current_tcb->status = FINISHED;
		free (current_tcb->context);
		free(current_tcb);

		current_tcb = q_pop(&ready_queue)->t;

		if(current_tcb == NULL)
		{
			printf("All threads done executing \n");

		}
		else
		{
			current_tcb->status = RUNNING;
			setcontext(current_tcb->context);
		}

	}
	
	return;
}

/* Round Robin scheduling algorithm */
static void sched_RR()
{
	// YOUR CODE HERE
	
	// Your own implementation of RR
	// (feel free to modify arguments and return types)
	
	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE

	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	return;
}

/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ() {
	// YOUR CODE HERE
	
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	return;
}

// Feel free to add any other functions you need

// YOUR CODE HERE
