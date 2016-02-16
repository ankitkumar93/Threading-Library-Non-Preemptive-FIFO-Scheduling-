#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "mythread.h"
#include "mythreadextra.h"

/* Data Structures */
typedef struct _Thread{
	ucontext_t context;
	struct _Thread* parent;
	struct _Thread* next;
}_MyThread;
typedef struct{
	_MyThread* front;
	_MyThread* rear;
}Queue;
typedef struct _Semaphore{
	int val;
	struct _Semaphore* next;
	Queue waiting;
}_MySemaphore;
typedef struct{
	_MySemaphore* front;
	_MySemaphore* rear;
}QueueSem;
/* Global Variables */
Queue *ready, *blocking, *terminated;
QueueSem *terminatedsem;
_MyThread *running, *exit_thread;

/* Private Functions */
//Queue Management Functions for Thread Queue
//Initialize the Queue
Queue* initQueue(){
	Queue* Q = malloc(sizeof(Queue));
	if(Q == NULL)
		return NULL;
	Q->front = NULL;
	Q->rear = NULL;
	return Q;
}

//Fix the Queue if Empty
void fixQueueEmpty(Queue* Q){
        if(Q->front == NULL){
		Q->rear = NULL;
	}
}


//Insert a thread in Queue
void insertQueue(Queue* Q, _MyThread* thread){
	if(Q == NULL){
		return;
	}
	if(thread == NULL){
		return;
	}
	if(Q->rear == NULL){
		Q->front = thread;
		Q->rear = thread;	
	}else{
		Q->rear->next = thread;
		Q->rear = Q->rear->next;
	}
}

//Remove a thread from Queue
_MyThread* removeQueue(Queue* Q){
	if(Q->front == NULL)
		return NULL;
	_MyThread* return_thread = Q->front;
	Q->front = Q->front->next;
	//Fix for Empty Queue
	fixQueueEmpty(Q);
	return_thread->next = NULL;
	return return_thread;
}

void removeQueueElement(Queue* Q, _MyThread* thread){
	if(Q == NULL || thread == NULL)
		return;
	_MyThread* temp = Q->front;
	_MyThread* prev = NULL;
	while(temp != NULL){
		if(temp == thread){
			if(prev != NULL)
				prev->next = temp->next;
			else
				Q->front = temp->next;
			if(temp == Q->rear)
				Q->rear = prev;
			fixQueueEmpty(Q);
			temp->next = NULL;
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

int findQueue(Queue* Q, _MyThread* thread){
	if(Q == NULL || thread == NULL)
		return -1;
	_MyThread* temp = Q->front;
	while(temp != NULL){
		if(temp == thread)
			return 0;
		temp = temp->next;
	}
	return -1;
}

_MyThread* findQueueChild(Queue* Q, _MyThread* parentthread){
	_MyThread* childthread = NULL;
	_MyThread* temp = Q->front;
	while(temp != NULL){
		if(temp->parent == parentthread){
			childthread = temp;
			break;
		}
		temp = temp->next;
	}
	return childthread;
}

void cleanQueue(Queue* Q){
	if(Q == NULL){
		return;
	}
	
	//Clean all the Queue Data
	while(Q->front != NULL){
		_MyThread* qdata = Q->front;
		Q->front = Q->front->next;
		free(qdata);
		qdata = NULL;
	}
	

	//Clear up the Queue
	Q->rear = NULL;
	free(Q);
	Q = NULL;
}

int checkQueueEmpty(Queue* Q){
	if(Q->rear == NULL)
		return 0;
	return -1;
}

//Semaphore Queue Management Functions
QueueSem* initQueueSem(){
	QueueSem* Q = malloc(sizeof(QueueSem));
        Q->front = NULL;
        Q->rear = NULL;
        return Q;
}
void insertQueueSem(QueueSem* Q, _MySemaphore* sem){
	if(Q == NULL){
                return;
        }
        if(sem == NULL){
                return;
        }
        if(Q->rear == NULL){
                Q->front = sem;
                Q->rear = sem;
        }else{
                Q->rear->next = sem;
                Q->rear = Q->rear->next;
        }

}

int findQueueSem(QueueSem* Q, _MySemaphore* sem){
	 if(Q == NULL || sem == NULL)
                return -1;
        _MySemaphore* temp = Q->front;
        while(temp != NULL){
                if(temp == sem)
                        return 0;
                temp = temp->next;
        }
        return -1;

}

void cleanQueueSem(QueueSem* Q){
	if(Q == NULL){
                return;
        }

        //Clean all the Queue Data
        while(Q->front != NULL){
                _MySemaphore* qdata = Q->front;
                Q->front = Q->front->next;
                free(qdata);
                qdata = NULL;
        }


        //Clear up the Queue
        Q->rear = NULL;
        free(Q);
        Q = NULL;
	
}

/* Thread Helper Functions */
_MyThread* createThread(void (*start_func)(void*), void* args){
	_MyThread* mythread = malloc(sizeof(_MyThread));
	//If memory cannot be allocated return NULL
	if(mythread == NULL)
		return NULL;
        //Thread Data
        mythread->parent = running;
        mythread->next = NULL;
        //Init Context
        getcontext(&mythread->context);
        //Context Stack
        char* mystack = NULL;
        mystack = malloc(sizeof(char)*8192);
        mythread->context.uc_stack.ss_sp = mystack;
        mythread->context.uc_stack.ss_size = 8192;
        //Context Link  
        mythread->context.uc_link = &exit_thread->context;

        //Make context for the new thread
        makecontext(&mythread->context, (void(*)(void))start_func, 1, args);
	
	return mythread;
}

//Change Execution to a Different Thread
void executeThread(_MyThread* current_thread, _MyThread* ready_thread){
	if(ready_thread != NULL){
		running = ready_thread;
		if(current_thread != NULL)
			swapcontext(&current_thread->context, &ready_thread->context);
		else
			setcontext(&ready_thread->context);
	}
}

//Handle Thread Exit
void exitThread(){
	_MyThread* currentthread = running;
        _MyThread* parentthread = currentthread->parent;
        insertQueue(terminated, currentthread);
        running = NULL;
        if(parentthread != NULL && findQueue(terminated, parentthread) == -1){
                if(findQueue(blocking, parentthread) != -1){
                        setcontext(&parentthread->context);
                }
        }

        //Clean up if no thread in ready queue
        if(checkQueueEmpty(ready) != -1){
                cleanQueue(blocking);
                cleanQueue(ready);
                cleanQueue(terminated);
                cleanQueueSem(terminatedsem);
        }else{
                _MyThread* ready_thread = removeQueue(ready);
                executeThread(NULL, ready_thread);
        }

}

/* Public Functions */

/* Thread Functions */
//Create a new Thread
MyThread MyThreadCreate(void (*start_func)(void*), void* args){
	//Create a new thread
	_MyThread* mythread = createThread(start_func, args);
	
	//If created thread is NULL return NULL
	if(mythread == NULL)
		return NULL;

	//Add new thread to the ready queue
	insertQueue(ready, mythread);
	
	//Return the created thread	
	MyThread return_thread = (MyThread) mythread;
	return return_thread;	
}

//Yield invoking thread
void MyThreadYield(void){
	//Get the head of the Ready Queue	
	_MyThread* ready_thread = removeQueue(ready);
	
	//Get the current thread(running)
	_MyThread* current_thread = running;
	
	//Insert current thread back into the Ready Queue
	if(current_thread != NULL)
		insertQueue(ready, current_thread);
			
	//Switch to the ready thread
	executeThread(current_thread, ready_thread);
}
	

//Join with a child thread
int MyThreadJoin(MyThread thread){
	_MyThread* childthread = (_MyThread*) thread;
	_MyThread* parentthread = running;
	if(parentthread == NULL)
		return -1;
	if(childthread == NULL)
		return 0;
	if(childthread->parent != parentthread)
		return -1;
	insertQueue(blocking, parentthread);
	running = NULL;
	getcontext(&parentthread->context);		
	//If child not terminated shift execution to another thread
	if(findQueue(terminated, childthread) == -1){
		_MyThread* ready_thread = removeQueue(ready);
		executeThread(NULL, ready_thread);
	}	
	running = parentthread;	
	removeQueueElement(blocking, parentthread);
	
	//Return when the child thread has terminated
	return 0;
}

//Join with all children
void MyThreadJoinAll(void){
	_MyThread* parentthread = running;
	_MyThread* childthread = NULL;
	insertQueue(blocking, parentthread);
	running = NULL;
	getcontext(&parentthread->context);
	childthread = findQueueChild(ready, parentthread);
	if(childthread == NULL)
		childthread = findQueueChild(blocking, parentthread);	
	if(childthread != NULL){
		_MyThread* ready_thread = removeQueue(ready);
        	executeThread(NULL, ready_thread);
	}
	running = parentthread;
        removeQueueElement(blocking, parentthread);
}

//Terminate the invoking thread
void MyThreadExit(void){
	exitThread();
}

//Create and run the main thread
void MyThreadInit(void (*start_func)(void*), void* args){
	//If already called before return(check using the state of ready queue)
	if(ready != NULL)
		return;
	//Initialize the Queues
	ready = initQueue();
	blocking = initQueue();
	terminated = initQueue();
	terminatedsem = initQueueSem();
	
	//If allocation error in Queues return
	if(ready == NULL || blocking == NULL || terminated == NULL || terminatedsem == NULL)
		return;
	
	//Make a context holder for exit thread to return to
	exit_thread = malloc(sizeof(_MyThread));

	//Make running threads NULL
	running = NULL;

	//Create a new thread
	_MyThread* mythread = createThread(start_func, args);

	//Shift Execution to the new thread
	executeThread(exit_thread, mythread);	
}

/* Semaphore Functions */
//Create a Semaphore
MySemaphore MySemaphoreInit(int initialValue){
	if(initialValue < 0)
		return NULL;
	_MySemaphore* sem = malloc(sizeof(_MySemaphore));
	//If memory cannot be allocated return error
	if(sem == NULL)
		return NULL;
	sem->val = initialValue;
	sem->next  =  NULL;
	MySemaphore return_sem = (MySemaphore)sem;
	return return_sem;
}

//Signal a Semaphore
void MySemaphoreSignal(MySemaphore mysem){
	_MySemaphore* sem = (_MySemaphore*) mysem;
	if(sem == NULL)
		return;
	
	sem->val++;
	if(sem->val <= 0){
		_MyThread* mythread = removeQueue(&sem->waiting);
		if(mythread == NULL)
			return;
		insertQueue(ready, mythread);
	}
}

//Wait on a Semaphore
void MySemaphoreWait(MySemaphore mysem){
	_MySemaphore* sem = (_MySemaphore*) mysem;
	if(sem == NULL)
		return;
	sem->val--;
	if(sem->val < 0){
		_MyThread* current_thread = running;
		_MyThread* ready_thread = removeQueue(ready);
		if(ready_thread == NULL)
			return;
		insertQueue(&sem->waiting, current_thread);
		executeThread(current_thread, ready_thread);	
	}
}

//Destory a Semaphore
int MySemaphoreDestroy(MySemaphore mysem){
	_MySemaphore* sem = (_MySemaphore*) mysem;
	if(sem == NULL)
		return -1;
	if(findQueueSem(terminatedsem, sem) == 0)
		return -1;
	if(checkQueueEmpty(&sem->waiting) == -1)
		return -1;
	//Put the Semaphore in Terminated Sem Queue
	insertQueueSem(terminatedsem, sem);
	return 0;
}

//Extra Credit
//MyThread Init Extra for Extra Credit
void MyThreadInitExtra(void){
	//If already called before return(check using the state of ready queue)
        if(ready != NULL)
                return;
        //Initialize the Queues
        ready = initQueue();
        blocking = initQueue();
        terminated = initQueue();
        terminatedsem = initQueueSem();

        //If allocation error in Queues return
        if(ready == NULL || blocking == NULL || terminated == NULL || terminatedsem == NULL)
                return;

	//Initialize running as NULL
	running = NULL;
        //Create the Exit Thread to Fix Thread Termination
        exit_thread = malloc(sizeof(_MyThread));
	exit_thread->parent = NULL;
	exit_thread->next = NULL;
	getcontext(&exit_thread->context);
	//Context Stack
        char* mystack = NULL;
        mystack = malloc(sizeof(char)*8192);
       	exit_thread->context.uc_stack.ss_sp = mystack;
        exit_thread->context.uc_stack.ss_size = 8192;
        //Context Link  
        exit_thread->context.uc_link = NULL;
	
        //Make context for the new thread
        makecontext(&exit_thread->context, exitThread, 0);
	//Create a new thread descriptor for the current thread
        _MyThread* mythread = malloc(sizeof(_MyThread));
	mythread->parent = NULL;
	mythread->next = NULL;
	getcontext(&mythread->context);
	//Context Stack
        char* mystack_2 = NULL;
        mystack_2 = malloc(sizeof(char)*8192);
        mythread->context.uc_stack.ss_sp = mystack_2;
        mythread->context.uc_stack.ss_size = 8192;
        //Context Link  
        mythread->context.uc_link = &exit_thread->context;
	
	//Make running as the mythread (current thread)
	running = mythread;
}
/*------------------------------End of Program--------------------------------------*/
