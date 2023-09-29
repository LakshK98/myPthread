// File:	mypthread.c

// List all group members' names:
// Laksh Kotian
// Arnab Halder
// iLab machine tested on:cp.cs.rutgers.edu

#include "mypthread.h"
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#define STACK_SIZE 64000
#define MAX_THREADS 200
#define TIME_QUANTUM_MICROSECONDS 5000
#define NUL_MLFQ 3
#define PROMOTION_DELAY 5

//64 rr 100 parellel_cal


typedef enum {RUN_NEXT, BLOCK, REMOVE} scheduler_event;

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
queue* ready_queue = NULL;
queue* blocked_queue = NULL;
tcb* current_tcb = NULL;
tcb* main_tcb = NULL;
int threadCount = 1;
int sched_event = -1; // 0 : timer ; 1 : thread start ; 2 thread_exit; 3 block cur thread
bool firstExec = true;
tcb* threads[MAX_THREADS];
int* sched_counter;
scheduling_algo sched = RR;


queue* mlf_queues[3];

static void schedule();
int get_num_quant_for_q_lvl(int);

static void sched_MLFQ();
static void sched_RR_SJF();
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

// returns whether t1 is greater than or equal to t2
bool priority_compare(tcb* t1, tcb* t2)
{
	if(t1->quant_count < t2->quant_count)
		return true;

	if(t1->quant_count > t2->quant_count)
		return false;
	
	time_t cur_time = time(NULL);
	
	if((difftime(cur_time, t1->entry_time)) >= (difftime(cur_time, t2->entry_time) ))
		return true;
	
	return false;

}

void q_push_fcfs(queue *q, q_node *qn)
{
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
}

void q_push_priority_based(queue *q, q_node *qn)
{
	if(q->head == NULL)
	{
		q->head = qn;
		q->tail = qn;
		q->size = 1;
		
	}
	else 
	{
		q_node* cur = q->head;

		if(priority_compare(qn->t, q->head->t))
		{
			qn->next = q->head;
			q->head = qn;
		}
		else
		{
			while(cur->next!=NULL)
			{
				if(priority_compare(qn->t,cur->next->t))
				{
					break;
				}
				cur = cur->next;
			}
			q_node* temp = cur->next;
			cur->next = qn;
			qn->next = temp;
			if(cur == q->tail)
				q->tail = qn;
		}
		q->size++;

	}

}


void q_push(queue *q, tcb *t, bool priority_based)
{
	q_node* qn = (q_node*) malloc(sizeof(q_node));
	qn->t = t;
	qn->next = NULL;

	if(priority_based)
	{
		q_push_priority_based(q, qn);
	}
	else
	{
		q_push_fcfs(q, qn);
	}

	// printf("PRINTING after push\n");
	// print_q(q);
}



queue* q_init()
{
	queue* ret_q = (queue*)malloc(sizeof(queue));
	ret_q->head = NULL;
	ret_q->tail = NULL;
	ret_q->size = 0;
	return ret_q;
}

// tcb* init_tcb()
// {

// 	queue* ret_q = (queue*)malloc(sizeof(queue));
// 	ret_q->head = NULL;
// 	ret_q->tail = NULL;
// 	ret_q->size = 0;
// 	return ret_q;
// }
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
	
	return qn;
}

void* disable_signal()
{

	sigset_t a;
	sigset_t b;
	
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);  
	sigprocmask(SIG_BLOCK, &a, &b); 
}
void enable_signal()
{
	sigset_t a;
	sigset_t b;
	
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);  
	sigprocmask(SIG_UNBLOCK, &a, &b); 
}

// wrapper 
void return_wrapper(void *(*function)(void*),  void * arg)
{
	void * ret_val = (void*)(*function)(arg);
	mypthread_exit(ret_val);

}
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
	if(firstExec)
	{
		firstExec = false;
		#ifdef PSJF
		sched = SJF;
		#endif
		#ifdef MLFQ
		sched = MLF;
		#endif
		ready_queue = q_init();
		for(int i =0 ;i <MAX_THREADS;i++)
		{
			threads[i] = NULL;
		}
		for(int i =0 ;i <NUL_MLFQ; i++)
		{
			mlf_queues[i] = q_init();
		}
		sched_counter = (int*) malloc(sizeof(int));
	}


	
	ucontext_t *new_thread_context = (ucontext_t*)malloc(sizeof(ucontext_t));

	getcontext(new_thread_context);
	init_context(new_thread_context);
	new_thread_context->uc_link = NULL;

	void (*function_ptr) () = (void*)function;
	void (*wrapped_func_ptr) () = &return_wrapper;
	makecontext(new_thread_context, wrapped_func_ptr, 2, function, arg);
	
	// creating tcb for new thread
	tcb * new_thread_tcb = (tcb*)malloc(sizeof(tcb));
	*new_thread = threadCount;
	new_thread_tcb->id = (mypthread_t*) malloc(sizeof(mypthread_t));
	*new_thread_tcb->id = threadCount;
	new_thread_tcb->return_val = NULL;
	new_thread_tcb->join_queue = q_init();
	new_thread_tcb->quant_count = 0;
	new_thread_tcb->entry_time = time(NULL);
	new_thread_tcb->is_blocking = false;
	threads[(*new_thread_tcb->id)] = new_thread_tcb;
	threadCount++;
	new_thread_tcb->context = new_thread_context;
	new_thread_tcb->status = READY;
	new_thread_tcb->mlfq_lvl = 0;



	disable_signal();
	
	if(sched == MLF)
		q_push(mlf_queues[0], new_thread_tcb, true);
	else
		q_push(ready_queue, new_thread_tcb, sched!= RR);


	// in case of first execution initialize context and tcb for main thread and store it in current tcb
	if(main_tcb == NULL)
	{
		ucontext_t *main_thread_context = (ucontext_t*)malloc(sizeof(ucontext_t));
		main_thread_context->uc_stack.ss_sp = malloc(STACK_SIZE);
		main_thread_context->uc_stack.ss_size = STACK_SIZE;

		getcontext(main_thread_context);

		main_thread_context->uc_link = NULL;
		main_tcb = (tcb*)malloc(sizeof(tcb));
		// reserving id 0 for main
		main_tcb->id = (mypthread_t*) malloc(sizeof(mypthread_t));
		*main_tcb->id = 0;
		main_tcb->context = main_thread_context;
		main_tcb->join_queue = q_init();
		main_tcb->status = RUNNING;
		main_tcb->return_val =NULL;
		main_tcb->quant_count = 0;
		main_tcb->entry_time = time(NULL);
		main_tcb->mlfq_lvl = 0;
		main_tcb->is_blocking = false;
		threads[0] = main_tcb;
		current_tcb = main_tcb;
	}



	sched_event = 0;
	schedule();
		
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

	sched_event = 0;
	schedule();

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// int * x = (int *) value_ptr;
	// printf("Value %d", (*x));

	if(current_tcb->return_val!=NULL)
	{
		(*current_tcb->return_val) = value_ptr; 
	}
	// printf("Exit called \n");
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
	if(threads[thread]!=NULL)
	{
		disable_signal();
		threads[thread]->return_val = value_ptr;
		q_push(threads[thread]->join_queue, current_tcb, false);
		sched_event = 3;
		schedule();
	}


	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	// YOUR CODE HERE
	mutex->lock = 0;
	mutex->owner_id = NULL;
	mutex->blocked_queue = q_init();
    q_node * qn = mutex->blocked_queue->head;
	// printf("fine here");
	//initialize data structures for this mutex

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
		// YOUR CODE HERE
		disable_signal();
		// printf("thread %d entered lock %d\n", (*current_tcb->id),mutex->lock  );
		if(mutex->lock == 0)
		{
			mutex->lock = 1;
			mutex->owner_id = current_tcb->id;
			current_tcb->is_blocking = true;
		
		}
		else
		{
			q_node * qn = (q_node*) malloc(sizeof(q_node));
			qn->t = current_tcb;
			q_push(mutex->blocked_queue, current_tcb, false);
			sched_event = 3;
			schedule();
		}
		enable_signal();

		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread

		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{		
	disable_signal();
	if(current_tcb->id == mutex->owner_id)
	{
		if(mutex->blocked_queue->size > 0)
		{
			tcb* next_owner = q_pop(mutex->blocked_queue)->t;
			if(sched == MLF)
			{
				next_owner->mlfq_lvl = 0;
				q_push(mlf_queues[0], next_owner, true);

			}

			else
				q_push(ready_queue, next_owner, sched!=RR);
			mutex->owner_id = next_owner->id;
		}
		else
			mutex->lock = 0;
	}

	enable_signal();


	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE
	
	// deallocate dynamic memory allocated during mypthread_mutex_init
	if(mutex->lock == 1)
	{
		//EBUSY
		return -1;
	}
	mutex->blocked_queue->head = NULL;
	mutex->blocked_queue->tail = NULL;
	mutex->blocked_queue->size = 0;
	free(mutex->blocked_queue);
	mutex->owner_id = NULL;

	return 0;
};

int sigco = 0;
void sighandler(int sig)
{
 
	//  write(STDOUT_FILENO, "handler called \n", 16);
//  printf("signal occurred %d times\n", ++sigco);
	sched_event = 0;

	#ifdef MLFQ
	current_tcb->quant_count = current_tcb->quant_count + get_num_quant_for_q_lvl(current_tcb->mlfq_lvl);
	#endif
	#ifndef MLFQ
	current_tcb->quant_count++;
	#endif
	current_tcb->entry_time = time(NULL);
	schedule();
}

void set_timer(int num_quants)
{
	// printf("%d ", num_quants);

	struct itimerval it;
	struct sigaction act, oact;
	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGALRM, &act, &oact); 

	// Start itimer
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = TIME_QUANTUM_MICROSECONDS * num_quants;
	setitimer(ITIMER_REAL, &it, NULL);	
	enable_signal();
}

/* scheduler */
static void schedule()
{
	if(sched == MLF)
	{
		sched_MLFQ();
	}
	else
	{
		sched_RR_SJF();
	}
}

// This function is used as scheduler for both RR and SJF.
// In case of SJF we use a sorted insert strategy while pushing threads into the queue so that whenever we pop from the queue we get the
// thread that has run the shortest amount of time. 
static void sched_RR_SJF()
{

	disable_signal();
	

	if(current_tcb!=NULL)
	{	
		// printf("current_tcb %d i can see stack %d thread_count \n" , (*current_tcb->id), threadCount);


	}
	 if(sched_event == 0)
	{
		if(current_tcb->status == RUNNING)
		{
			if(ready_queue->size == 0)
			{
				// printf("No one waiting in queue contiue exec current tcb id:%d \n",(* current_tcb->id));
				return;
			}

			tcb* temp_tcb  = current_tcb;
			current_tcb->status = READY;

			current_tcb = q_pop(ready_queue)->t;

			current_tcb->status = RUNNING;
			
			q_push(ready_queue, temp_tcb, sched!= RR);

			set_timer(1);
			swapcontext(temp_tcb->context, current_tcb->context);

		}
		else
		{
			// error case
		}

	}
	else if(sched_event == 1)
	{
		// this should only be null for first start
		if(current_tcb==NULL)
		{
			if(ready_queue->head == NULL)
				return;

			current_tcb = q_pop(ready_queue)->t;
		}
	

		if(current_tcb->status == READY)
		{
			// printf("ready current tcb\n");
			
			current_tcb->status = RUNNING;
			ucontext_t* ready_thread_context = current_tcb->context;
		

			set_timer(1);
			setcontext(ready_thread_context);

		}
	}
	else if (sched_event == 2)
	{		



		if((current_tcb->join_queue->size)>0)
		{
			while((current_tcb->join_queue->size)!=0)
			{
				q_push(ready_queue, q_pop(current_tcb->join_queue)->t, sched!=RR);
			}
		}
		current_tcb->status = FINISHED;

		threads[(*current_tcb->id)] = NULL;

		free (current_tcb->context);
		free(current_tcb);
		
		current_tcb = q_pop(ready_queue)->t;


		if(current_tcb == NULL)
		{

		}
		else
		{
			current_tcb->status = RUNNING;

			set_timer(1);
			setcontext(current_tcb->context);
		}

	}
	else if (sched_event == 3)
	{

		// printf("Blocking code\n");

		tcb* temp_tcb  = current_tcb;
		current_tcb->status = BLOCKED;
		
		// if(blocked_queue == NULL)
		// {
		// 	blocked_queue = q_init();
		// }
		// q_push(blocked_queue, current_tcb);


		// Assumption: readyqueue wont be empty cuz either
		// this is blocked by mutex in which case owner will be in ready q
		// this is blocked by join in which case the thread it is joining on will be in ready q

		current_tcb = q_pop(ready_queue)->t;

		current_tcb->status = RUNNING;
		// printf(" Running tcb with id  : %i\n", (* current_tcb->id));
		// printf("  Blocked tcb with id  : %i\n", (* temp_tcb->id));

		// printf(" BLOCKED \n");

		set_timer(1);
		swapcontext(temp_tcb->context, current_tcb->context);
	}
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

int  get_highest_available_prority()
{
	for(int i=0;i<NUL_MLFQ;i++)
	{
		if(mlf_queues[i]->size>0)
			return i;

	}	
	return -1;
}

int get_num_quant_for_q_lvl(int mlfq_num)
{
	return (mlfq_num == (NUL_MLFQ -1)) ? 1 :  (current_tcb->mlfq_lvl+1);
}

void update_MLFQ()
{
	for(int i=1;i<NUL_MLFQ;i++)
	{

		time_t cur_time = time(NULL);
		
		while ( mlf_queues[i]->size>0 && difftime(cur_time, mlf_queues[i]->head->t->entry_time)  >=( PROMOTION_DELAY*TIME_QUANTUM_MICROSECONDS) )
		{
			
			q_push_priority_based(mlf_queues[i-1], q_pop( mlf_queues[i]) );
		}
	}
}
/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ() {
	// YOUR CODE HERE
	
	(*sched_counter)++;
	if((*sched_counter) == PROMOTION_DELAY)
	{
		update_MLFQ();
		(*sched_counter) = 0;
	}
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)
	int highest_available_priority = get_highest_available_prority();

 	if(sched_event == 0)

	{

		if(!current_tcb->is_blocking)
		{

			current_tcb->mlfq_lvl = (current_tcb->mlfq_lvl  == NUL_MLFQ-1) ? current_tcb->mlfq_lvl : current_tcb->mlfq_lvl+1;
		}
		else
		{
			current_tcb->mlfq_lvl = 0;
		}
		if(current_tcb->status == RUNNING)
		{

			if((highest_available_priority == -1) || (highest_available_priority > current_tcb->mlfq_lvl) || (highest_available_priority == (NUL_MLFQ-1)))
			{
				// printf("No one waiting in queue contiue exec current tcb id:%d \n",(* current_tcb->id));
				return;
			}
			// printf("running current tcb\n");

			tcb* temp_tcb  = current_tcb;
			current_tcb->status = READY;

			current_tcb = q_pop(mlf_queues[highest_available_priority])->t;
			current_tcb->status = RUNNING;

			q_push(mlf_queues[current_tcb->mlfq_lvl ], temp_tcb, true);
			// printf(" Running tcb with id  : %i\n", (* current_tcb->id));
			// printf("  temp with id  : %i\n", (* temp_tcb->id));

			// printf(" SWAPPING \n");

			set_timer(get_num_quant_for_q_lvl(current_tcb->mlfq_lvl));
			swapcontext(temp_tcb->context, current_tcb->context);

		}
		else
		{
			// printf(" ERROR: no running running current tcb\n");

		}

	}
	else if (sched_event == 2)
	{		


		// printf("current tcb id : %i is exiting\n", (*current_tcb->id));

		if((current_tcb->join_queue->size)>0)
		{
			while((current_tcb->join_queue->size)!=0)
			{
				tcb* waiting_thread =  q_pop(current_tcb->join_queue)->t;
				q_push(mlf_queues[waiting_thread->mlfq_lvl], waiting_thread, true);
			}
		}

		highest_available_priority = get_highest_available_prority();
		current_tcb->status = FINISHED;

		threads[(*current_tcb->id)] = NULL;

		free (current_tcb->context);
		free(current_tcb);
		current_tcb = NULL;

		if(highest_available_priority != -1)
		{
			current_tcb = q_pop(mlf_queues[highest_available_priority])->t;

			current_tcb->status = RUNNING;

			set_timer(get_num_quant_for_q_lvl(current_tcb->mlfq_lvl));
			setcontext(current_tcb->context);
		}



		if(current_tcb == NULL)
		{
			// printf("All threads done executing \n");


		}

	}
	else if (sched_event == 3)
	{

		// printf("Blocking code\n");

		tcb* temp_tcb  = current_tcb;
		current_tcb->status = BLOCKED;


		// Assumption: readyqueue wont be empty cuz either
		// this is blocked by mutex in which case owner will be in ready q
		// this is blocked by join in which case the thread it is joining on will be in ready q

		current_tcb = q_pop(mlf_queues[highest_available_priority])->t;

		current_tcb->status = RUNNING;
		

		set_timer(get_num_quant_for_q_lvl(current_tcb->mlfq_lvl));
		swapcontext(temp_tcb->context, current_tcb->context);
	}
	return;
}
