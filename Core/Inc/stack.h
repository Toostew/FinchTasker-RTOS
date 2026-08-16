/*
 * stack.h
 *
 *  Created on: 22 Jul 2026
 *      Author: tooka
 */

#ifndef INC_STACK_H_
#define INC_STACK_H_

enum taskStateTypes {
	READY,
	RUNNING,
	WAITING_DELAY,
	WAITING_SEMAPHORE
};


typedef struct {
	uint32_t * stackPointer;
	uint32_t * basePointer;
	uint32_t * topOfStackPointer;
	uint32_t * taskFunction;
	enum taskStateTypes taskState;
	uint32_t wakeTick; //this is the tick wherein the tick for the task to wake, if WAITING

} TransferControlBlock_def;




void psp_switchConfig(uint32_t * taskStack, uint32_t sizeOfStack);
void schedulerConfig(TransferControlBlock_def * firstTask, TransferControlBlock_def * secondTask);
void createTask(uint32_t stackSizeInWords, void * taskFunction, enum taskStateTypes taskState);
int assemblyAdd(int a, int b);
void schedulerComputeTaskState(void);
void schedulerCompute(void);
void taskOne(void);
void taskTwo(void);
void taskThree(void);


#endif /* INC_STACK_H_ */
