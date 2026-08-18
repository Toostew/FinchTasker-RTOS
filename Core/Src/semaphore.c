/*
 * semaphore.c
 *
 *  Created on: 18 Aug 2026
 *      Author: tooka
 */
#include "main.h"


//create a semaphore, add it to global list of semaphores, return pointer to said semaphore struct
semaphoreBinary_def * semaphoreCreate(){
	//allocate memory of size of the semaphore struct
	semaphoreBinary_def * semaphorePointer = (semaphoreBinary_def *)malloc(sizeof(semaphoreBinary_def));
	semaphorePointer->status = 1; //available on the spot
	semaphorePointer->waiting_task = NULL; //no waiting tasks


	return semaphorePointer;
}

//take the semaphore
//initially, I was thinking of adding the task's TCB as a parameter, but logically, the only time when this function
//runs is the currently running task, hence the currentTask, so the function doesn't inherently need to ask
//we need to absolutely guarantee this function isn't interrupted, or else it might cause race conditions, hence corrupted data
void semaphoreTake(semaphoreBinary_def * semaphorePointer){
	uint32_t primask_save = __get_PRIMASK();


	//disable interrupts
	__disable_irq();



	//if it's 1, give it to whoever
	if(semaphorePointer->status){
		semaphorePointer->status  = 0;


	} else {
		//semaphore is NOT available, mark the task as waiting for semaphore
		currentTask->taskState = WAITING_SEMAPHORE;
		semaphorePointer->waiting_task = currentTask;

		//manually fire pendSV since we cannot advance without the semaphore
		SCB->ICSR |= (1 << 28); //fire pendSV

	}


	//enable interrupts, we reinstate the old state of PRIMASK before we disabled interrupts
	__set_PRIMASK(primask_save);

}

void semaphoreGive(semaphoreBinary_def * semaphorePointer){
	uint32_t primask_save = __get_PRIMASK();


	//disable interrupts
	__disable_irq();

	semaphorePointer->status = 1;



	//check if the semaphore is being waited on by any task
	if(!(semaphorePointer->waiting_task == NULL)){
		//we need to allow the waiting task to be READY again
		semaphorePointer->waiting_task->taskState = READY;
		semaphorePointer->waiting_task = NULL;
	}


	__set_PRIMASK(primask_save);
}


