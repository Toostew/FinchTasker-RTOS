/*
 * semaphore.h
 *
 *  Created on: 18 Aug 2026
 *      Author: tooka
 */

#ifndef INC_SEMAPHORE_H_
#define INC_SEMAPHORE_H_

typedef struct {
	uint8_t status; // 1 for available, 0 for unavailable
	TransferControlBlock_def * waiting_task; //pointer to any task currently waiting for the semaphore, if any
} semaphoreBinary_def;


semaphoreBinary_def * semaphoreCreate(); //create a semaphore
void semaphoreTake(); //take a semaphore, rendering it unavailable
void semaphoreGive(); //give back semaphore, it can now be used by others


#endif /* INC_SEMAPHORE_H_ */
