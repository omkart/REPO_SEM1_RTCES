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


#define CHATTERBOX_TASKS_MAX		3


/* 
	Priorities at which the tasks are created.
 */

typedef enum
{
	CHATTERBOX_TASK_PRIORITY_1 = (tskIDLE_PRIORITY + 1),
	CHATTERBOX_TASK_PRIORITY_2,
	CHATTERBOX_TASK_PRIORITY_3,
	CHATTERBOX_TASK_PRIORITY_UNDEFINED,
}e_chatterboxTaskPriority;

/* 
	Task Execution type
 */

typedef enum
{
	CHATTERBOX_TASK_EXEC_TYPE_1 = 0,		// infinite task instance executions
	CHATTERBOX_TASK_EXEC_TYPE_2,			//	task instance shall only be executed 5 times
}e_chatterboxTaskExecType;

/* 
	output frequencey
 */

#define mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS pdMS_TO_TICKS( 200UL )

/*-----------------------------------------------------------*/

/*
  *  data structure
  */

typedef struct
{
	e_chatterboxTaskPriority priority;
	unsigned long outputFrequency;
	char outputData[10];
	e_chatterboxTaskExecType execType;
	void (*taskHandlerFuncptr)(void);

}s_chatterboxTasks;

/*
 * C function (prototype) for task
 */
void chatterboxTask1(void* taskParameters);
void chatterboxTask2(void* taskParameters);
void chatterboxTask3(void* taskParameters);
/*-----------------------------------------------------------*/


/*
 * initialize data structures - Defined global for now ( could be placed as local)
 */
s_chatterboxTasks chatterboxTasks[3] =
{									// Priority						Frequency for calls,				OP String	Execution Type				Function
									{CHATTERBOX_TASK_PRIORITY_1,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task1",	CHATTERBOX_TASK_EXEC_TYPE_1, &chatterboxTask1},
									{CHATTERBOX_TASK_PRIORITY_2,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task2",	CHATTERBOX_TASK_EXEC_TYPE_1, &chatterboxTask2},
									{CHATTERBOX_TASK_PRIORITY_3,mainTASK_CHATTERBOX_OUTPUT_FREQUENCY_MS,"Task3",	CHATTERBOX_TASK_EXEC_TYPE_2, &chatterboxTask3},
};

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_exercise( void )
{
	uint8_t taskCount = 0;

	for (taskCount = 0; taskCount < CHATTERBOX_TASKS_MAX; taskCount++)
	{
		/*
		 * Create the task instances.
		 */
		xTaskCreate(chatterboxTasks[taskCount].taskHandlerFuncptr,			/* The function that implements the task. */
			"Task1", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 							/* The size of the stack to allocate to the task. */
			NULL, 												/* The parameter passed to the task - not used in this simple case. */
			chatterboxTasks[taskCount].priority,				/* The priority assigned to the task. */
			NULL);												/* The task handle is not required, so NULL is passed. */

	}

	/*
	* Start the task instances.
	*/
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	while(1);
}
/*-----------------------------------------------------------*/

/* 
 *  C function for tasks
 */
void chatterboxTask1(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = chatterboxTasks[0].outputFrequency;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	while (1)
	{
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[0].outputData);

	}
}

void chatterboxTask2(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = chatterboxTasks[1].outputFrequency;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	while (1)
	{
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[1].outputData);

	}

}


void chatterboxTask3(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = chatterboxTasks[2].outputFrequency;
	static uint64_t task3ExecutionCount = 0;
	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();



	while (1)
	{
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		printf("%s\n", chatterboxTasks[2].outputData);

		if (chatterboxTasks[2].execType == CHATTERBOX_TASK_EXEC_TYPE_2)
		{
			task3ExecutionCount++;
			if (task3ExecutionCount >= 5)
			{
				printf("Terminating Task 3\n");
				vTaskDelete(NULL); //Will delete the existing task
			}
		}
		else
		{
			//Do nothing
		}

	}
}