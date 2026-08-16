/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32g4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */

//SuperVisor Call
void SVC_Handler(void)
{
	//we need to return an EXTI_RETURN value of 0xFFFFFFFD,
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

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */

/*
//this is the handler function that runs when pendSV is triggered
//in this function you'll notice I dereference, assign etc but it seeem inconsistent, but this is actually just a feature of assembly
//assembly has its own structure and ruleset of when to treat values in registers as addresses directly or if it requires dereferencing
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

			 //LOAD the new task into the CPU
			 "LDR r0, =nextTask\n\t"//load the absolute address of the next task, r0 holds the absolute address of the variable nextTask
			 "LDR r0, [r0]\n\t" //dereference the address, r0 now contains the pointer that points to the TCB (first element)
			 "LDR r0, [r0]\n\t" //dereference AGAIN so that, r0 now contains the stack pointer (actual memory region)
			 "LDMIA r0!, {R4-R11}\n\t"//load, increment after, starting at base address stored in r0, for registers R4 to R11
			 "MSR PSP, r0\n\t" //load address stored in r0 to PSP
			 //due to LDR, STR requiring [], which means dereferencing, we need 4 registers to do this swap
			 "LDR r1, =nextTask\n\t"
			 "LDR r2, =currentTask\n\t"
			 "LDR r3, [r1]\n\t" //dereference the absolute address into r3, r3 has address to the TCB
			 "LDR r4, [r2]\n\t"
			 "STR r3, [r2]\n\t" //dereference r2 so it contains the address of the TCB, store that at address at r1
			 "STR r4, [r1]\n\t" //dereference r1 so it contains the address of the TCB, store that at address at r2

			 "BX LR"			//Branch to address stored at register LR (BX: Branch Indirect, give register and get address within register). lr is the link register (R14) that stores
			 //the link register stores the return address of a function. When invoked with branch, it goes to that address
			 //since pendSV is an interrupt, NVIC will handle the routing, sending the cpu to the proper address of the next instruction to run (theres more nuance but thats the idea)
	 );
} */

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

//we don't perform anything huge inside systick handler because it has a VERY high priority
//the whole point of a context switch is so that it can run when nothing important needs CPU time
//Hence, ideally we keep it a pyramid layout; wherein lower priority interrupts do the heavy lifting (so they can be interrupted)
//higher priority interrupts get very easy and lightweight tasks
void SysTick_Handler(void)
{
	SCB->ICSR |= (1 << 28); //fire pendSV
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  BSP_PB_IRQHandler(BUTTON_USER);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
