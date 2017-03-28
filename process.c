#include "3140_concur.h"
#include <stdlib.h>
#include <fsl_device_registers.h>

/* PCB process control block */
struct process_state {
	unsigned int *sp;
	struct process_state *next;
	/* the stack pointer for the process */
};

/* current process is NULL before scheduler starts and after it end */
process_t * current_process = NULL;
process_t * process_one = NULL; // initialize queue's global variable keeping track of first process in queue

/* Add new process to queue */
void add_to_queue (process_t *new_process) 
{
	// list is empty, add to beginning
	if (process_one == NULL) {
		process_one = new_process;
		new_process->next = NULL;
	} 
	// list is not empty, add to end
	else {
		process_t *tmp = process_one;
		// traverse the list to the end
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}
		// add new_process as the next of the current last
		tmp->next = new_process;
		new_process->next = NULL;
	}
}

/* Remove first element from queue and returns; we only ever remove first element of list
When first element is only element, next is null. Therefore, the first element is null */
process_t *remove_from_queue (void) {
	if (process_one == NULL) return NULL;
	process_t *elem = process_one;
	process_one = process_one->next;
	return elem;
}

/* Start process with the given amount of time */
void process_start (void)
{
	SIM->SCGC6 = SIM_SCGC6_PIT_MASK;
	PIT_MCR &= 0;
	PIT->CHANNEL[0].LDVAL = 0x1E8480; // load 2 million cycles (somewhere close to 10 Hz, >10 Hz)
	PIT->CHANNEL[0].TFLG |= 1;
	NVIC_EnableIRQ(PIT0_IRQn);
	process_begin();
}


/* Creates a new process that starts at function f, initial stack size n 
Returns 0 on success, -1 if error 
May require allocating memory for a process_t structure, use malloc() 
Should called process_stack_init() to allocate the stack 
Should allocate a process_t 
Should add the process to your scheduler's data structures */
int process_create (void (*f) (void), int n)
{
	process_t *process = (process_t*) malloc(sizeof(process_t));
	if (process == NULL) return -1;
	unsigned int *new_sp = process_stack_init(f, n);
	if (new_sp == NULL) return -1;
	process->sp = new_sp;
	//process->next = NULL;
	//struct process_queue *new_process;
	//new_process->val = process;
	add_to_queue(process);
	return 0;
}


/* Updates process_t
process_t is NULL until process_start is called
process_t is also NULL when a process terminates
current_process is process_t
cursp is null if no process is running or when process just finished */
unsigned int * process_select(unsigned int * cursp)
{
	if (cursp == NULL) {
		if (process_one == NULL) return NULL;
		// remove first process in queue if it exists
		// free the stack memory
		// save process in a temp variable
		//struct process_state *tmp = current_process;
		// free the stack for the previous process and remove from the queue
		process_t *tmp = current_process;
		current_process = remove_from_queue();
		process_stack_free(tmp->sp, sizeof(tmp));
		free(tmp);
		// update current_process; null if queue is empty
		return current_process->sp;
	}
	// process was not done
	else {
		// kick current process to the back of the queue and remove from the beginning
		current_process->sp = cursp;
		// current_process->next = NULL;
		add_to_queue(current_process);
		current_process = remove_from_queue();
		// the current process is now what is first in the queue
		return current_process->sp;
	}
			
}
