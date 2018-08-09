
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>




#include "tpool.h"


#define TASKS_PER_THREAD 100







// -----------------------------------------------------------------------------------------------------------------------------------------
//										Structures
// -----------------------------------------------------------------------------------------------------------------------------------------


/* - - binary semaphore - - */
typedef struct bnrySm {

	int value;
	pthread_mutex_t mutex;
	pthread_cond_t cnVrbl;

} bnrySm;



/* - - thread - - */
typedef struct thread{
	int id;
	pthread_t pthread;
	
} thread;


/* - - task queue - - */
typedef struct tskQue{

	int *tasks;
	int mxSize, start, end;
	bnrySm *inhbtd;

} tskQue;


/* - - thread pool - - */ 
typedef struct tpool{

	void (*process_task)(int);
	thread** threads;
	tskQue queue;
	bnrySm *frThrds;
	int CPUnum;
	
}tpool;



// -----------------------------------------------------------------------------------------------------------------------------------------
//										Function ProtoTypes
// -----------------------------------------------------------------------------------------------------------------------------------------


static int bnrySm_init(struct bnrySm *bnrySm_Ptr, int value);
static void bnrySm_post(struct bnrySm *bnrySm_Ptr);
static void bnrySm_wait(struct bnrySm *bnrySm_Ptr);

static int thread_init (struct thread** thread_Ptr, int id);
static void* thread_run(struct thread* thread_Ptr);

static int tskQue_init (struct tskQue* tskQue_Ptr, int size );
static int tskQue_enqueue(struct tskQue *queue, int task);
static int tskQue_dequeue(struct tskQue *tskQue_Ptr);



// -----------------------------------------------------------------------------------------------------------------------------------------
//										ThreadPool
// -----------------------------------------------------------------------------------------------------------------------------------------
struct tpool thrdPl;

int tpool_init(void (*process_task_Ptr)(int)){

	if((thrdPl.CPUnum = ( sysconf(_SC_NPROCESSORS_ONLN) ) ) < 1 ){ 		//defines the thread number based on available cores
		perror("tpool_init()::sysconf() failed:\n");
		return -1;
	}
	

	
	
	//if (( &thrdPl =(struct tpool *)malloc(sizeof(struct tpool)) )  == NULL ){ 	// allocate thread pool memory space
	//	perror("tpool_init()::malloc( 'thread pool' ) failed:\n");			
	//	return -1;
	//}
	

	if(( thrdPl.threads =( struct thread**)malloc(thrdPl.CPUnum*sizeof(struct thread *))) == NULL){ //allocate thread pointer memory space
		perror("tpool_init()::malloc( 'threads pointer' ) failed:\n");
		return -1;
	}
	
	if((tskQue_init(&thrdPl.queue, thrdPl.CPUnum * TASKS_PER_THREAD)) == -1){ 	//create task queue
		perror("tpool_init()::tskQue_init() failed:\n");
		return -1;
	}

	if( (thrdPl.frThrds = (struct bnrySm*)malloc(sizeof(struct bnrySm))) == NULL){
			perror("thrdPl_init()::malloc( 'binary semaphore pointer free threads' ) failed:\n");
			return -1;
		}

	if( (bnrySm_init(thrdPl.frThrds, 0) )== -1){
			perror("tskQue_init()::bnrySm_init() failed:\n");
			return -1;
		}
	

	
	thrdPl.process_task = process_task_Ptr;					// function to handle incoming file descriptors
	
	for (int i=0; i < thrdPl.CPUnum;i++){							//iterates through threads in thread pool
		
		if( thread_init(&(thrdPl.threads[i]), i ) == -1){				//instantiates each thread
		perror("tpool_init()::thread_init(' thread ') failed:\n");
		return -1;

		}
	}
	


	return 0;
	
}
int tpool_add_task(int newtask){

	if((tskQue_enqueue(&(thrdPl.queue),newtask) ) == -1){
		perror("tpool_add_task()::tskQue_enqueue() failed:\n");
		return -1;
	}


	return 0;
	
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//										Thread
// -----------------------------------------------------------------------------------------------------------------------------------------
static int thread_init (struct thread** thread_Ptr, int id){
	
	*thread_Ptr = (struct thread*)malloc(sizeof(struct thread));
	
	if(thread_Ptr == NULL){
		perror("thread_init().malloc() failed");
		return -1;
	}

	(*thread_Ptr)->id = id;

	pthread_create( &(*thread_Ptr)->pthread, NULL, (void *)thread_run, (*thread_Ptr));
	pthread_detach((*thread_Ptr)->pthread);
	return 0;
}


static void* thread_run(struct thread* thread_Ptr){
	int fd;									// represents the task being threaded through the core



	void (*prcss_Bfr)(int);
	

	while(1){
		if( (fd = tskQue_dequeue(&(thrdPl.queue)) ) == -1){		// check for tasks in queue
			printf("thread: %d stopped, waiting...\n", thread_Ptr->id);
			bnrySm_wait(thrdPl.queue.inhbtd);			// wait till a new task is queued
			printf("thread started...\n");
		} else {

		bnrySm_post(thrdPl.frThrds);					//

		prcss_Bfr = thrdPl.process_task;
		prcss_Bfr(fd);
		
		}
			
	}

	return NULL;
	

} 



// -----------------------------------------------------------------------------------------------------------------------------------------
//										Task Queue
// -----------------------------------------------------------------------------------------------------------------------------------------

static int tskQue_init (struct tskQue *tskQue_Ptr, int size ){

		
		tskQue_Ptr->mxSize = size;
		tskQue_Ptr->start=0;
		tskQue_Ptr->end = 0;

		

		if( (tskQue_Ptr->inhbtd = (struct bnrySm*)malloc(sizeof(struct bnrySm))) == NULL){
			perror("tskQue_init()::malloc( 'binary semaphore pointer inhabited' ) failed:\n");
			return -1;
		}
		

		if( (bnrySm_init(tskQue_Ptr->inhbtd, 0) )== -1){
			perror("tskQue_init()::bnrySm_init() failed:\n");
			return -1;
		}

		if( (tskQue_Ptr->tasks = (int *)malloc(sizeof(int) * size) ) == NULL){
			perror("tskQue_init()::malloc( 'tasks' ) failed:\n");
			return -1;
		}	
		
		for (int i=0; i < size;i++){	
			tskQue_Ptr->tasks[i] = -1;	
		}	
		

		return 0;
}

static int tskQue_enqueue(struct tskQue *tskQue_Ptr, int task){
	printf("trying to enqueue\n");
	if((tskQue_Ptr->tasks[tskQue_Ptr->end]) != -1) bnrySm_wait(thrdPl.frThrds);  		//queue is full cannot enqueue
										//should wait untill a thread has cleared up
	printf("enqueue\n");
	tskQue_Ptr->tasks[tskQue_Ptr->end]=task;				//enqueue
	tskQue_Ptr->end++;   	    						//move along	 
	if ((tskQue_Ptr->end) == (tskQue_Ptr->mxSize)) tskQue_Ptr->end=0;		//wrap around
	

	bnrySm_post( tskQue_Ptr->inhbtd);

	return 0; 								//successful completion
	
}

static int tskQue_dequeue(struct tskQue *tskQue_Ptr){
	printf("trying to dequeue\n");
	int task;								// create return value

	if( (task = tskQue_Ptr->tasks[tskQue_Ptr->start]) == -1)		//try to assign next task to return value
		return -1; 							//queue is empty nothing to dequeue
	printf("dequeue\n");
	tskQue_Ptr->tasks[tskQue_Ptr->start] = -1;				//dequeue
	tskQue_Ptr->start++;							//move along
	if ((tskQue_Ptr->start) == (tskQue_Ptr->mxSize)) tskQue_Ptr->start=0;	//wrap around
	

	
	return task;								//successful completion

}



// -----------------------------------------------------------------------------------------------------------------------------------------
//										Binary Semaphore
// -----------------------------------------------------------------------------------------------------------------------------------------
static int bnrySm_init(struct bnrySm *bnrySm_Ptr, int value){
	
	if(value != 0 || value !=0){
		perror("bnrySm_init() value must be 1 or 0:\n");
		return -1;
	}

	pthread_mutex_init( &(bnrySm_Ptr->mutex) , NULL );
	pthread_cond_init( &(bnrySm_Ptr->cnVrbl) , NULL );
	
	bnrySm_Ptr->value = value;

	return 0;
}

static void bnrySm_post(struct bnrySm *bnrySm_Ptr){

	pthread_mutex_lock( &bnrySm_Ptr->mutex );

    	bnrySm_Ptr->value = 1;

    	pthread_cond_signal( &bnrySm_Ptr->cnVrbl );
    	pthread_mutex_unlock( &bnrySm_Ptr->mutex );	
}

static void bnrySm_wait(struct bnrySm *bnrySm_Ptr){

	pthread_mutex_lock( &bnrySm_Ptr->mutex );

    	while ( !bnrySm_Ptr->value )
        	pthread_cond_wait( &bnrySm_Ptr->cnVrbl, &bnrySm_Ptr->mutex );

   	bnrySm_Ptr->value = 0;

    	pthread_mutex_unlock( &bnrySm_Ptr->mutex );
	
	
}






