/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE: Windows will not be running the FreeRTOS project threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this project application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 *
 * NOTE 2:  This file only contains the source code that is specific to exercise 2
 * Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * NOTE:  Console input and output relies on Windows system calls, which can
 * interfere with the execution of the FreeRTOS Windows port.  This demo only
 * uses Windows system call occasionally.  Heavier use of Windows system calls
 * can crash the port.
 */

/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

/* TODO: Priorities at which the tasks are created.
 */

typedef enum
{
	CHATTERBOX_TASK_PRIORITY_1 = (tskIDLE_PRIORITY + 1),
	CHATTERBOX_TASK_PRIORITY_2,
	CHATTERBOX_TASK_PRIORITY_3,
	EX_TASK_PRIORITY_UNDEFINED,
	_TASK_PRIORITY_UNDEFINED,
}e_chatterboxTaskPriority;

/* TODO: output frequencey
 */

#define mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS pdMS_TO_TICKS( 200UL )

/*-----------------------------------------------------------*/

/*
  * TODO: data structure
  */

typedef struct
{
	e_chatterboxTaskPriority priority;
	unsigned long outputFrequency;
	char outputData[10];

}s_chatterboxTasks;

/*
 * TODO: C function (prototype) for task
 */
void chatterboxTask1(void);

void chatterboxTask2(void);

void chatterboxTask3(void);

void chatterboxTask(void);
/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;
/*
 * TODO: initialize data structures
 */
s_chatterboxTasks chatterboxTasks[3] =
{
											{CHATTERBOX_TASK_PRIORITY_1,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task1"	},
											{CHATTERBOX_TASK_PRIORITY_1,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task2"	},
											{CHATTERBOX_TASK_PRIORITY_1,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task3"	},
};

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_exercise( void )
{


	/* 
	 * TODO: Create the task instances.
     */	

	 /* Start the two tasks as described in the comments at the top of this
	 file. */
		xTaskCreate(chatterboxTask1,			/* The function that implements the task. */
			"Task1", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
			NULL, 							/* The parameter passed to the task - not used in this simple case. */
			CHATTERBOX_TASK_PRIORITY_1,/* The priority assigned to the task. */
			NULL);							/* The task handle is not required, so NULL is passed. */


		xTaskCreate(chatterboxTask2,			/* The function that implements the task. */
			"Task2", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
			NULL, 							/* The parameter passed to the task - not used in this simple case. */
			CHATTERBOX_TASK_PRIORITY_2,/* The priority assigned to the task. */
			NULL);


		xTaskCreate(chatterboxTask3,			/* The function that implements the task. */
			"Task3", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
			NULL, 							/* The parameter passed to the task - not used in this simple case. */
			CHATTERBOX_TASK_PRIORITY_3,/* The priority assigned to the task. */
			NULL);


	 /*
	  * TODO: Start the task instances.
	  */

	  /* Start the tasks and timer running. */
		vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* 
 * TODO: C function for tasks
 */
void chatterboxTask1(void)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[0].outputData);

	}

	
}

void chatterboxTask2(void)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[1].outputData);

	}


}


void chatterboxTask3(void)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[2].outputData);

	}


}


void chatterboxTask(void)