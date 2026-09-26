/*
 * stack.c
 *
 *  Created on: 22 Jul 2026
 *      Author: tooka
 */

#include "main.h" //master header file


//this function switches from Main Stack Pointer to Process Stack Pointer
//ARM's core has two physical stack pointers. MSP is used by exception/interrupt handlers
//(and by everything, before a scheduler exists) - this is fixed, not task-specific.
//PSP is for our own tasks - whichever task is currently running uses PSP as its active stack.
//we need to provide a region of memory, and provide the location of the topmost address
//During task execution, PSP holds the address one word past the last valid stack element
//(since ARM stacks grow downward, the initial SP points just past the array's top end)
//This region of memory is part of a larger whole, and is usually part of a Task Control block
//this region of memory will store everything local to the task like variables, constants, etc.
//During a context switch, the CPU will automatically push certain Register states into this region of memory. Keep in mind
//that there is no "special region" to store it within this region, they load the values to wherever the stack pointer was pointing within this stack region
//Each task gets its own separate stack region so its local data never collides
//with another task's stack, or with the kernel/handler stack (MSP).
//I say "certain" registers, it is not enough. We need to manually push other registers ourselves

//understand byte alignment: When you specify a variable to be, say, of 8, 16, 32, or 64 bits (1,2,4,8 bytes respectively),
//the compiler enforces memory allignment such that the address of the given variable is always divisible by it's size in bytes.
//so, an 8 bit and 16 bit value (1 and 2 bytes), the 8 bit value can be placed at any address (since any address is divisible by 1)
//but the 16 bit value (2 bytes) will only be placed on addresses that are divisible by 2.
//so an 8 bit value could exist on addresses 0x00000001, 0x00000002, 0x00000003.. so on
//but a 16 bit value can only exist on 0x00000002, 0x00000004, ... so on
//this applies to the higher bit numbers
//32 bit numbers are alligned to 4-divisible addresses, so 0x00000004, 0x00000008, 0x0000000C.. so on
//64 bit number are alligned to 8-divisible addresses, like 0x00000008, 0x00000010, 0x00000018.. so on

//provide the beginning address of the stack already set to 512 elements (512 * 32 bits (4 bytes))
//CAUTION: at the current moment, this function WILL fail. this is because the function, which initially starts off
//in MSP, converts to PSP before the function can terminate. This will cause hardfaults that are not immediately obvious
void psp_switchConfig(uint32_t * taskStack, uint32_t sizeOfStack){

	  //__get_CONTROL() function is provided by ARM to get the CONTROL register
	  uint32_t controlRegister = __get_CONTROL();

	  //calculate the top of the stack, it is the tip of the array, plus 1 (so technically, 1 element outside)
	  //
	  uint32_t topOfTask_Stack = ((uint32_t)(taskStack) + (sizeOfStack * sizeof(uint32_t)));

	  //another ARM function to set the location of top of stack of PSP
	  __set_PSP(topOfTask_Stack);

	  //set SPSEL active stack pointer selection to PSP (bit 1)
	  controlRegister |= (1 << 1);


	  NVIC_SetPriority(PendSV_IRQn, 15);
	  //write the changes to the control register ()
	  //do  not trigger on
	  //__set_CONTROL(controlRegister);

	  // Instruction Synchronization Barrier (ISB) instruction. It flushes the processor's pipeline and
	  //fetch buffers, ensuring that all subsequent instructions are fetched from cache or memory after
	  //previous system changes take effect. In short, big config changes, invoke __ISB for safety
	  __ISB();
}


//this is the separate config function to set the first task into sequence
//we do not set control here ourselves, we'll do this in the supervisor call with arm assembly
void schedulerConfig(TransferControlBlock_def * firstTask, TransferControlBlock_def * secondTask,
		void * idleTaskFunction){

		//set the first and second tasks as current and next
	  currentTask = firstTask;
	  nextTask = secondTask;

	  //this creates the idleTask; it's a special global so we need to do it here
		uint32_t * stackRegion = (uint32_t *)malloc(256 * sizeof(uint32_t));

		if(stackRegion == NULL){
			return;
		}
		memset(stackRegion, 0, (256 * sizeof(uint32_t))); //set every byte to 0
		uint32_t topOfTask_Stack = ((uint32_t)(stackRegion) + (256 * sizeof(uint32_t)));
		uint32_t fakeStackPointer = ((uint32_t)(topOfTask_Stack) - 64);

		*(uint32_t *)((uint8_t *)fakeStackPointer + 0x3C) = 0x01000000; //xPSR, offset by 60 bytes, not 64 since technically topofTaskStack is 4 bytes (1 element) out
		*(uint32_t *)((uint8_t *)fakeStackPointer + 0x38) = (uint32_t)idleTaskFunction; //PC




		//create the TCB with malloc
		//(remember: local variables are destroyed on function termination, malloc allocates memory in the heap)
		TransferControlBlock_def * task = (TransferControlBlock_def *)malloc(sizeof(TransferControlBlock_def));



		//this is the creation of the idle task, which is a special task not found within the task list
		task->stackPointer = (uint32_t *)fakeStackPointer;
		task->basePointer = stackRegion;
		task->topOfStackPointer = (uint32_t *)topOfTask_Stack;
		task->taskFunction = (uint32_t *)idleTaskFunction;
		task->taskState = READY; //since it is idle it will ALWAYS be ready


		  idleTask = task;

	  __set_PSP((uint32_t)firstTask->topOfStackPointer);


	  NVIC_SetPriority(PendSV_IRQn, 15);

	  __ISB();
}


//this function figures out who's up next, using round robin (simple)
//when it is invoked from pendSV, currentTask will already contain
//on first run 1 has already been computed as the next task
//initially we just swap current and nextTask, which we compute for the next invokation of schedulerCompute
//to allow blocking to occur we now need to check the task state before moving on
void schedulerCompute(void){
	OSTickCount++;
	//compute the task states of all tasks that are WAITING
	//determines if currently waiting tasks can be promoted to READY
	schedulerComputeTaskState();

	uint8_t candidateFound = 0;
	//we will check who in the list isn't blocked or waiting
	//the first task that is ready will be swapped and done
	//if none of the tasks are free it switches to an idle tasks that is always ready

	//check every single item
	//check the currently running task, if it's state is anything other than RUNNING, do not set to ready
	if(currentTask->taskState == RUNNING){
		currentTask->taskState = READY; //set ready
	}
	//if the taskstate is WAITING, then we must not allow it to be promoted to READY or else it will invalidate the delay

	//this will run the length of the TCB list
	for(int i = 0; i < transferControlBlockListLength; i++){
		transferControlBlockListNextIndex++;
		if(transferControlBlockListNextIndex >= transferControlBlockListLength){
			transferControlBlockListNextIndex = 0; //reset to 0
		}
		if(transferControlBlockList[transferControlBlockListNextIndex]->taskState == READY){
			//found a ready task


			nextTask = transferControlBlockList[transferControlBlockListNextIndex];
			currentTask = nextTask;
			nextTask->taskState = RUNNING;
			candidateFound = 1;
			break; //terminate early

		}
	}
	//the for loop terminates automatically if it could not find another task despite checking them all
	//we will check the candidateFound flag to see if the for loop terminated with or without a candidate task
	if(!candidateFound){
		nextTask = idleTask;
		currentTask = nextTask;
		currentTask->taskState = RUNNING;

	}


	/*
	currentTask = nextTask;

	transferControlBlockListNextIndex++;

	if(transferControlBlockListNextIndex >= transferControlBlockListLength){
		transferControlBlockListNextIndex = 0; //reset to 0
	}
	nextTask = transferControlBlockList[transferControlBlockListNextIndex];
	*/
	/*
	//count the number each task runs for debug
	switch(transferControlBlockListNextIndex){
		case 0:
			taskZeroRuns++;
			break;
		case 1:
			taskOneRuns++;
			break;
		case 2:
			taskTwoRuns++;
			break;
		case 3:
			taskThreeRuns++;
			break;
		case 4:
			taskFourRuns++;
			break;
	}
	*/
}

//this function will go through the ENTIRE list of tasks, check if their wakeCount has been exceeded by the tick count.
//if yes, promote from blocked to ready again
void schedulerComputeTaskState(){
	for(int i = 0; i < transferControlBlockListLength; i++){
		if(transferControlBlockList[i]->taskState == WAITING_DELAY){

			//check if the OSTickCount exceed or equal wakeTick
			//if yes, set the task state to READY
			//instead of directly comparing wakeTick <= OSTickCount, we can subtract them, and see if <= 0
			//this is better since if either overflows it's 32 bit register, it will loop back.
			//by minusing we reduce the chance of errors happening during an overflow
			//keep in mind at 1ms tickrate, 32-bit register will overflow in 49 days
			//we use signed int because we will deal with negatives, if we used uint negatives would overflow and wrap forwards
			if((int32_t)(transferControlBlockList[i]->wakeTick - OSTickCount) <= 0){
				transferControlBlockList[i]->taskState = READY;
			}

		} else if (transferControlBlockList[i]->taskState == WAITING_SEMAPHORE){
			//this handles the semaphore condition, check to see if tasks waiting on semaphore can now run
		}
	}
}







//creates task for you
void createTask(uint32_t stackSizeInWords, void * taskFunction, enum taskStateTypes taskState){
	uint32_t * stackRegion = (uint32_t *)malloc(stackSizeInWords * sizeof(uint32_t));

	if(stackRegion == NULL){
		return;
	}
	memset(stackRegion, 0, (stackSizeInWords * sizeof(uint32_t))); //set every byte to 0
	uint32_t topOfTask_Stack = ((uint32_t)(stackRegion) + (stackSizeInWords * sizeof(uint32_t)));
	uint32_t fakeStackPointer = ((uint32_t)(topOfTask_Stack) - 64);

	*(uint32_t *)((uint8_t *)fakeStackPointer + 0x3C) = 0x01000000; //xPSR, offset by 60 bytes, not 64 since technically topofTaskStack is 4 bytes (1 element) out
	*(uint32_t *)((uint8_t *)fakeStackPointer + 0x38) = (uint32_t)taskFunction; //PC


	  //check if there's any space left in the taskList
	  if(transferControlBlockListIndex >= transferControlBlockListLength){
			return;
	  }

	  //create the TCB with malloc
	  //(remember: local variables are destroyed on function termination, malloc allocates memory in the heap)
	  TransferControlBlock_def * task = (TransferControlBlock_def *)malloc(sizeof(TransferControlBlock_def));

	  task->stackPointer = (uint32_t *)fakeStackPointer;
	  task->basePointer = stackRegion;
	  task->topOfStackPointer = (uint32_t *)topOfTask_Stack;
	  task->taskFunction = (uint32_t *)taskFunction;
	  task->taskState = taskState; //actually come to think of it you could just default to ready on task creation, but hey leave the door open

	//add to the TCB list
	transferControlBlockList[transferControlBlockListIndex] = task;
	transferControlBlockListIndex++; //once this fill completely, this becomes the index of the last element, assuming it doesnt overflow out
	//and conveniently, also the number of tasks currently registered

}

//this function, when invoked, delays execution of whatever task that calls it for the set number of ticks
//called inside the task function
//as set in config.c, systick fires every 1 ms
void taskDelay(uint32_t ticks){
	currentTask->wakeTick = OSTickCount + ticks;
	currentTask->taskState = WAITING_DELAY;

	//manually fire pendSV
	SCB->ICSR |= (1 << 28); //fire pendSV
}



//PB1, far right
void taskOne(void){
	while(1){
		taskOneRuns++;
		toggleBlink(0);
		taskDelay(100);
	}
}
//PB4, far left
void taskTwo(void){
	  while(1){
		  taskTwoRuns++;
		  toggleBlink(1);
		  taskDelay(200);
	  }
}

//PB5, middle
void taskThree(){
	while(1){
		taskThreeRuns++;
		toggleBlink(2);
		taskDelay(1000);
	}
}

void idleTaskFunction(void){
	while(1){

	}
}



int assemblyAdd(int a, int b){
	int value;

	/*
 __asm__ volatile (
    " ASSEMBLY CODE ZONE "   // Raw Assembly goes here, %x are positional placeholders
    : OUTPUT OPERANDS ZONE   // Map C variables you want to WRITE to
    : INPUT OPERANDS ZONE    // Map C variables you want to READ from
    : CLOBBER ZONE           // Tell the compiler which registers you modified (optional)
	);

	  */

	__asm__ volatile (
			"ADD %0, %1, %2" //ADD 0(value), 1(a), 2(b)  %0, %1, %2 are placeholders that are evaluated in order
			: "=r" (value) //the characters inside "" like "=r" are called constraints, and do some small operations or conditions
			  //all output must have "=". and "r" is a contraint to use a general-purpose register r0-r12
			: "r"  	(a), // bind the value of a to the 2nd operand
			  "r"	(b) //bind the value of b to the 3rd operand; for more info look up GCC Extensions

	);

	return value;
}











