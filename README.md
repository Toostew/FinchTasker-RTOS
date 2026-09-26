# FinchRTOS

A from-scratch preemptive RTOS kernel for the STM32G474RE (ARM Cortex-M4).

---

## What's an Operating System?

You'd probably picture something like Windows, Apple OS, or maybe Linux (which is technically a kernel). At its core, an OS is itself software that manages a computer system's hardware and software resources, like memory, CPU processing time, storage, devices, everything. An OS acts like a middle-man between the user, apps, and hardware. If a user wants to run an app, or an app needs hardware (memory, storage), they all have to talk to the OS, and the OS talks to everything.

Now, there are a few distinct types of OSes. The one most relevant to us right now is the **Real-Time Operating System (RTOS)**, which is a type of OS specifically made with strict data processing and system response time requirements. RTOSes are designed to guarantee a response within a set time limit and are usually found in critical components, think avionics, medical devices, guidance systems, etc. Where a typical operating system would allow delays or be "slow", since doing so would at worst result in a bad user experience, a delay in a RTOS environment would result in catastrophic failures which are simply non-negotiable.

With that in mind, let me introduce you to my latest project, an RTOS kernel for the STM32G474RE, which I've called **FinchRTOS**, after Finches (because I like Finches).

<p align="center">
  <img src="https://github.com/user-attachments/assets/5e5287ff-e12c-4c57-a973-03b39f8bed29" title="finch" width="80%">
  <br>
  <sub>Finches are easily in the top 10 birds ever</sub>
</p>





Now, at its core RTOSes do the same job as most other OSes, it's just that they are time-strict. The definition of which is purely arbitrary, between different programmers it could mean anything! 1ms, 5ms, 15 seconds? time-strict is only as restrictive as the mission needs it to be. For example, if a device REQUIRES deterministic decisions every second, then you're strict to a second or less.

Another quick distinction but the terms "RTOS" and "Kernel" are used interchangeably here, but technically, a **Kernel** is the core program that actually does all the resource management, and facilitates the communication between hardware and software. An **OS** is essentially the kernel + everything else needed to run the system, think peripheral drivers, filesystems, etc.   an RTOS being an extension of an OS with time-strict elements. Since my project is quite simple in its design both terms are used to refer to it.

I'll stop boring you with requirements and definitions and go straight into the meat: How do we make an RTOS and what is needed? well, a simple RTOS must:

- Allow the creation of distinct **"tasks"**: individual programs that can be run
- Be able to run, pause, replace and resume different tasks: **"context switching"**
- Allow tasks to be delayed and/or synchronized with other tasks: **delays and semaphores**
- Order task execution using specific **scheduling strategies**

---

## Task Control Blocks

Tasks are programs that the RTOS can execute (give CPU processing time). They require memory to be allocated to store their own data. In the kernel, we represent tasks using **Task Control Blocks (TCBs)**. These are C-structs that contain information on tasks.

```c
typedef struct {
	uint32_t * stackPointer;
	uint32_t * basePointer;
	uint32_t * topOfStackPointer;
	uint32_t * taskFunction;
	enum taskStateTypes taskState;
	uint32_t wakeTick; //this is the tick wherein the tick for the task to wake, if WAITING

} TransferControlBlock_def;
```

> **Note:** notice I've called it the `TransferControlBlock_def`, it's the same as `TaskControlBlock` it's just I called it differently.

Now, another thing to understand is that memory is grouped into 2 types: **stack** and **heap**. Heap memory is raw memory that persists for as long until explicitly freed and isn't assigned to one particular task. Stack memory on the other hand is essentially task-specific memory, they only remain for as long as the task is. If the task terminates, the stack terminates. Understand that both of these types of memory function differently, but memory at its core is just a very long, sequential series of memory blocks that are byte addressable.

In the TCB above, we store 3 different pointers to the stack: `stackPointer`, which is the pointer to the next free address within the stack to be used; the `basePointer`, which points to the base of the stack; and the `topOfStackPointer`, which points to the top of the stack. Understand that the stack region is bounded by the `topOfStackPointer` down to the `basePointer`   stack memory grows downward starting from the top of the stack to the base.

<p align="center">
  <img src="https://github.com/user-attachments/assets/aad85a07-d153-497a-9f53-55402b75d401" title="Stack memory growth" width="50%">
  <br>
  <sub>The Stack grows downward, starting from a high address and ending on a lower address.</sub>
</p>

### Creating a Task

Tasks are created by calling the `createTask` function, which initializes the task's TCB, and adds it into a global "task list", which is, well, a list of every task.

```c
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
	  task->taskState = taskState; 
	//add to the TCB list
	transferControlBlockList[transferControlBlockListIndex] = task;
	transferControlBlockListIndex++; //once this fill completely, this becomes the index of the last element, assuming it doesnt overflow out
	//and conveniently, also the number of tasks currently registered

}
```

Now, for context, there are 16 registers in an ARM Cortex M4. R0–R12 are General Purpose Registers (GPRs), R13 is the Main Stack Pointer (MSP) OR Process Stack Pointer (PSP), R14 is the Link Register (LR), R15 is the Program Counter (PC) and the 16th register is the Program Status Register (xPSR). During a context switch, xPSR, PC (R15), LR (R14), R12, and R0–R3 are all saved automatically. Each register is a word (32 bits, 4 bytes) large, so a total of 32 bytes are stored automatically. Additionally, we also save the remaining 8 registers manually, so a total of 64 bytes is stored on the stack. *(More on this later)*

```c
	*(uint32_t *)((uint8_t *)fakeStackPointer + 0x3C) = 0x01000000; //xPSR, offset by 60 bytes, not 64 since technically topofTaskStack is 4 bytes (1 element) out
	*(uint32_t *)((uint8_t *)fakeStackPointer + 0x38) = (uint32_t)taskFunction; //PC
```

When stored on the stack, xPSR and PC are the closest to `topOfStackPointer`. Looking at the above code snippet, notice the offset from `fakeStackPointer` by `0x3C` and `0x38`, this is 60 and 56 in decimal, which in this context means 60 and 56 bytes above `fakeStackPointer`, which corresponds to the first and second words in the stack from a top-down perspective.

Notice how we explicitly set the values for xPSR and PC; this is important since although we can safely leave every other register value at 0 (a result of the `memset` function that was called), for the task to be called properly we need to set the xPSR and PC to an appropriate value. PC is set to the address of a function we want to associate with this task, while xPSR is set to `0x01000000`, which specifically sets bit 24 to 1, which is the **Thumb State Bit**. If not set, any instructions run causes a fault/lockup.

---

## Scheduler Configuration

Before we can run anything we also need to configure the scheduler. The function `schedulerConfig` is called:

```c
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
```

This function creates the idle task, which is a special task that exclusively runs when there are no free tasks. It is not found within the task list and is always in the `READY` state. It also populates the `currentTask` and `nextTask`.

### The Scheduler's Core Loop

Now we go into the central driver of the RTOS, `schedulerCompute`. This function runs every "tick", which you can think of as the heartbeat of the RTOS   at a predictable interval, which is powered by the SysTick, which is sorta like a countdown timer interrupt. It counts down from a certain number you set, and when it hits zero, it fires a SysTick interrupt which invokes a SysTick handler function, which calls the necessary logic. So through this we can run code at a set interval.

```c
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
}
```

As explained mostly by the comments, `schedulerCompute`'s job is figuring out who runs next. It will iterate through the task list to find who can run next, and if found, does the swap. If it doesn't, it puts the idle task to run next. Pretty straightforward!

---

## PSP / MSP and the SVC Bootstrap

Before we go into the swapping logic (context switching), we need to get up to speed with **Main Stack Pointer (MSP)** and **Process Stack Pointer (PSP)** configuration. By default, the CPU runs on MSP config, which means everything uses the same shared stack, and thus share the same stack pointer. In the RTOS environment, each task has its own dedicated stack region within memory, and hence needs its own stack pointer. So how does the CPU know when to use the MSP or PSP? well, we tell it! Within `schedulerConfig` you might have noticed this line:

```c
	  __set_PSP((uint32_t)firstTask->topOfStackPointer);
```

This is us setting up the location of the PSP in memory, which is stored within a task's TCB. `__set_PSP` is a dedicated function that configures R13 (the stack pointer register) to hold the new address. But as I have mentioned, the CPU by default starts off from MSP, so we must manually get it into PSP mode. This is done within the Supervisor Call handler. (Supervisor Call (SVC) is itself a software-invokable interrupt):

```c
//SuperVisor Call
void SVC_Handler(void)
{
	//we need to return an EXC_RETURN value of 0xFFFFFFFD,
	//returns to thread mode, non-FPU, from PSP, uses PSP on return

	__asm__ volatile (
			"LDR R0, =0xFFFFFFFD\n\t" //this loads the value direct into R0
			"MOV LR, R0\n\t" //load value in R0 to LR
			 "LDR r0, =currentTask\n\t"//load the absolute address of the current task, r0 holds the absolute address of the variable nextTask
			 "LDR r0, [r0]\n\t" //dereference the address, r0 now contains the pointer that points to the TCB (first element)
			 "LDR r0, [r0]\n\t" //dereference AGAIN so that, r0 now contains the stack pointer (actual memory region)
			 "LDMIA r0!, {R4-R11}\n\t"//load, increment after, starting at base address stored in r0, for registers R4 to R11
			 "MSR PSP, r0\n\t" //load address stored in r0 to PSP

			 "BX LR\n\t"			//Branch to address stored at register LR (BX: Branch Indirect, give register and get address within register). lr is the link register (R14) that stores
			//this causes it to load LR value, return to PSP in thread mode
	);
}
```

SVC is responsible for the initial "switch" needed to get from MSP to PSP. This is done by loading a special value into the Link Register (LR), which is `0xFFFFFFFD`, which is a special code to return to thread mode, and use the PSP on return. We also load in the `currentTask` state into the register in preparation for the switch, as seen in loading r0 above. SVC is triggered one time, within the main function. After it is triggered, execution never returns back to `main()`.

**within `main.c`:**

```c
  systick_config();



  createTask(512, &taskOne, READY); //taskZero
  createTask(256, &taskTwo, READY); //taskOne
  createTask(128, &taskThree, READY); //taskTwo
  createTask(128, &taskTwo, READY); //taskThree
  createTask(128, &taskThree, READY); //taskFour

  //this will set currentTask, NextTask, and the idleTask globals
  schedulerConfig(transferControlBlockList[0], transferControlBlockList[1], (void *)idleTaskFunction);

  systick_toggle(1);
  //invoke SVC
	__asm__ volatile (
		"SVC 0x0" //SuperVisor Call with attached 8 bit value (we're not gonna use it)
	);

	//past this point, the code will never return here.
```

---

## The Context Switch   PendSV

Now, the swapping logic itself. This is done in ARM assembly (Thumb-2), and the code can be found within the interrupt handlers in `stm32g4xx_it.c`:

```c
//Context Switch is split between STORE and LOAD. we STORE the current task state into memory, then LOAD the next task's state into the CPU regs
void PendSV_Handler(void)  //this macro declares that this function is naked, as in, C will not treat it normally, and will not generate function entry and exit code. As a consequence, we must write the body in assembly
{
	 __asm__ volatile (
			 //STORE the current task into memory
			 "MRS r0, PSP\n\t"		//load address stored at psp into register 0 (MRS: Move from special register to Regular register)
			 //C compiler goes through it as a single line so use the new line and tab (only new line needed tab is just for QOL)
			 "STMDB r0!, {R4-R11}\n\t" //Store into memory, using address stored in r0, Registers 4 to 11. ! declares writeback, meaning that the address at r0 is updated with the new values in memory
			 "LDR r1, =currentTask\n\t" //load into register, absolute address of the variable currentTask itself, not the pointer that it stores
			 "LDR r1, [r1]\n\t" //dereference the value at r1, so the absolute variable address, we are targetting the pointer stored at that address
			 "STR r0, [r1]\n\t" //dereference currentTask, store value of r0 in the stack pointer member

			 //we push 2 Registers, total 64 bits (8 bytes) to match with the AAPCS rule for 8-byte allignment
			 //The rule is that the stack pointer needs to be 8-byte alligned before a function call
			 //we are calling schedularCompute by using BL, so by then, the SP MUST be 8 byte-alligned
			 //hence, we push a random register just for padding, we're not actually gonna use R4 or whatever
			 //we choose R4 because it's the safest bet, because certain registers like r0-r3/r12 are part of a set
			 //of registers stated in AAPCS that cannot be guaranteed to be untouched. Function might or might no utilize them
			 //this is the reason why interrupts auto-stack these registers. R4 is not part of this so there is no issue
			 //R4 and other Callee saved registers (the function getting called) are obliged to be restored to their original state
			 //Caller saved registers (the function-caller) are not under any obligation to be saved
			 "PUSH {LR,R4}\n\t"
			 //we no longer swap the next and current task here, that's handled in standard C code, elsewhere, we just invoke the function
			 "BL schedulerCompute\n\t" //scheduler compute will handle the task ordering

			 "POP {LR,R4}\n\t"
			 //LOAD the new task into the CPU
			 "LDR r0, =nextTask\n\t"//load the absolute address of the next task, r0 holds the absolute address of the variable nextTask
			 "LDR r0, [r0]\n\t" //dereference the address, r0 now contains the pointer that points to the TCB (first element)
			 "LDR r0, [r0]\n\t" //dereference AGAIN so that, r0 now contains the stack pointer (actual memory region)
			 "LDMIA r0!, {R4-R11}\n\t"//load, increment after, starting at base address stored in r0, for registers R4 to R11
			 "MSR PSP, r0\n\t" //load address stored in r0 to PSP

			 "BX LR\n\t"			//Branch to address stored at register LR (BX: Branch Indirect, give register and get address within register). lr is the link register (R14) that stores
			 //the link register stores the return address of a function. When invoked with branch, it goes to that address
			 //since pendSV is an interrupt, NVIC will handle the routing, sending the cpu to the proper address of the next instruction to run (theres more nuance but thats the idea)
			 //since we are within an exception, LR is actually populated with a specific calue noted EXC_RETURN
	 );
}
```

`PendSV_Handler` is invoked every time we trigger the PendSV interrupt in the Interrupt Control and State Register (ICSR), located within the System Control Register (SCB). We do this every tick, which we can see within the SysTick handler, also in the same file:

```c
void SysTick_Handler(void)
{
	SCB->ICSR |= (1 << 28); //fire pendSV
}
```

PendSV handles the context switching, by pushing registers and saving them and loading registers from memory. It also invokes `schedulerCompute`.

You might be wondering, why does the SysTick Handler have to trigger PendSV, rather than just do all the stuff that PendSV does, within the SysTick Handler? I mean, you COULD do that, but this actually introduces an issue you might not immediately spot: **interrupt priority**. SysTick has a much higher interrupt priority within the vector table, while PendSV has the lowest possible. A big part of context switching is that it needs to be low priority, because imagine if an interrupt fires while we're mid context switch   if the context switch has a higher priority, the interrupt would have to wait until AFTER the switch concludes, which kinda defeats the whole purpose of interrupts on account of them being time-critical and important. So to fix this, we purposefully do the context switch within PendSV, which has the lowest possible interrupt priority, meaning any other interrupt will be given priority over it. Hence the context switch can only occur when no other interrupt occurs. If we did it within the SysTick Handler, which has a much higher interrupt priority, any interrupt with a priority below the SysTick will have to wait, which is suboptimal. *(I'm repeating myself)*

---

## Delays and Semaphores

These are functionality meant to synchronize functions, allowing us to effectively pause and resume tasks when needed, and even have tasks wait on other tasks.

### Delays

Of the two, delays are conceptually and technically easier to implement. You may have noticed within `schedulerCompute()` that we increment a value, `OSTickCount`.

```c
OSTickCount++;
```

We track this value, `OSTickCount`, which is a 32-bit unsigned int that stores the number of ticks (the number of times SysTick has occurred). The idea is that we can pause tasks for a certain number of ticks. We store the current tick count, and for each paused task, store the tick count it should be awoken. Observe in the TCB:

```c
typedef struct {
	uint32_t * stackPointer;
	uint32_t * basePointer;
	uint32_t * topOfStackPointer;
	uint32_t * taskFunction;
	enum taskStateTypes taskState;
	uint32_t wakeTick; //this is the tick wherein the tick for the task to wake, if WAITING

} TransferControlBlock_def;
```

Every time `schedulerCompute()` is invoked, at the start of the function we also invoke `schedulerComputeTaskState()`, whose sole job is to figure out if a task should be `READY`, or still `WAITING`:

```c
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
```

Notice the `WAITING_SEMAPHORE` block is empty? That's because the logic was moved to a more appropriate location   more on that in a bit.

### Semaphores

Semaphores are slightly more complicated in that they tie multiple tasks together. Semaphores allow for a task to claim it, and have so that other tasks will wait until that specific semaphore is freed before continuing.

> **Note:** this is a **binary semaphore**   its status can only be 0 (taken) or 1 (available), representing a single shared resource that either one task holds or nobody does. This is different from a *counting semaphore*, which tracks an integer count and allows multiple tasks to hold the resource at the same time. A binary semaphore is the simpler of the two, and I chose to implement this first. Perhaps I'll add in counting semaphores somewhere down the line

This is done using a special semaphore struct:

```c
typedef struct {
	uint8_t status; // 1 for available, 0 for unavailable
	TransferControlBlock_def * waiting_task; //pointer to any task currently waiting for the semaphore, if any
} semaphoreBinary_def;
```

Semaphores are created by the `semaphoreCreate()` function (duh):

```c
semaphoreBinary_def * semaphoreCreate(){
	//allocate memory of size of the semaphore struct
	semaphoreBinary_def * semaphorePointer = (semaphoreBinary_def *)malloc(sizeof(semaphoreBinary_def));
	semaphorePointer->status = 1; //available on the spot
	semaphorePointer->waiting_task = NULL; //no waiting tasks


	return semaphorePointer;
}
```

And the give and take logic:

```c
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
```

You'll notice the use of `__disable_irq()` and `__set_PRIMASK()`. `__disable_irq()` is a special function that deliberately disables interrupts. I know I droned about the importance of interrupts being given priority earlier but in this specific scenario, it is important that interrupts do NOT occur during semaphore operation. This is because semaphores are global, shared resources. Interrupts that fire during these critical semaphore sections could potentially modify semaphores, leaving outdated values or corrupting them. Hence, we need to ensure that both semaphore taking and giving action occur top to bottom with NO interruption. This concept is formally called **"Critical Sections"**.

Now, we use `__set_PRIMASK()` paired with `__get_PRIMASK()` as essentially the reverse of `__disable_irq()`. It re-enables the interrupts. But why don't we use `__enable_irq()`, which is a more aptly named function? This is another niche issue but when we use either disable or enable irq it does just one job: toggling interrupts. When we call `__disable_irq()`, it globally disables interrupts. When we call `__enable_irq()` it does the opposite, globally enabling interrupts. Simple! But here's the thing: let's say we have 2 tasks, both running sequentially. One of them calls `__disable_irq()`. Then the other task gets switched to and calls `__enable_irq()`. This means that interrupts are GLOBALLY enabled, even to the first task, which explicitly needed them off. This means when the second task is completed and the first task begins running where it left off, it assumes it is safely running without the risk of interrupts, but in reality it is not. We solve this issue by using `__get_PRIMASK()`, which is the register that stores interrupt request handling, among other things. In order to prevent the issue mentioned above we need to inherit the state of the PRIMASK, which is exactly what `__get_PRIMASK()` does. We get the current state of the PRIMASK, disable interrupts, then once we're done we set it back to whatever state it was initially.
