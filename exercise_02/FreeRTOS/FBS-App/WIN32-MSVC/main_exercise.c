/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, software written by omkar, including without limitation the rights to
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
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 * 
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
#include "semphr.h"


#define FBS_MAX_TASKS_IN_FRAME			4
#define FBS_MAX_FRAMES					5
#define FBS_MAX_TASKS_WORKER			6


#define FBS_SCHEDULER_FREQUENCY_MS pdMS_TO_TICKS( 200UL )



/*
	Priorities at which the tasks are created.
 */
typedef enum
{
	FBS_TASK_PRIORITY_1 = (tskIDLE_PRIORITY + 1), //Lowest priority
	FBS_TASK_PRIORITY_2,
	FBS_TASK_PRIORITY_3,
	FBS_TASK_PRIORITY_UNDEFINED,
}e_fbsTaskPriority;

typedef enum
{
	FBS_TASK_TYPE_SCHEDULER = 0,
	FBS_TASK_TYPE_WORKER,
	FBS_TASK_TYPE_UNDEFINED,

}e_fbsTaskType;


typedef struct
{
	e_fbsTaskType taskType;
	e_fbsTaskPriority fbsTaskPriority;
	TaskHandle_t taskHandle;
	void (*funcPtr)(void);
}s_fbsTasks;


typedef struct
{
	uint8_t frameStart;
	uint8_t frameEnd;
	uint8_t tasksInFrame[FBS_MAX_TASKS_IN_FRAME];

}s_fbsTaskLookupTable;


void fbsSchedulerTask(void* taskParameters);
void fbsWorkerTask0(void* taskParameters);
void fbsWorkerTask1(void* taskParameters);
void fbsWorkerTask2(void* taskParameters);
void fbsWorkerTask3(void* taskParameters);
void fbsWorkerTask4(void* taskParameters);
void fbsWorkerTask5(void* taskParameters);

/*WIP : update the table as per the question at the moment kept this table for testing the scheduler task*/
s_fbsTaskLookupTable fbsTaskLookupTable[FBS_MAX_FRAMES] =
{
	//frameStart				//frameEnd			//tasksInFrame
	{0U,						120U,				{0,1,2,3}			},
	{120U,						240U,				{4,5,0,1}			},
	{240U,						360U,				{0,1,2,3}			},
	{360U,						480U,				{4,5,0,1}			},
	{480U,						600U,				{0,1,2,3}			},

};


static s_fbsTasks schedulerTask;
static s_fbsTasks workerTasks[FBS_MAX_TASKS_WORKER];

void main_exercise( void )
{


	schedulerTask.taskType = FBS_TASK_TYPE_SCHEDULER;
	schedulerTask.fbsTaskPriority = FBS_TASK_PRIORITY_3;
	schedulerTask.funcPtr = &fbsSchedulerTask;
	
	/*
	* Create the task instances.
	*/
	xTaskCreate(schedulerTask.funcPtr,			/* The function that implements the task. */
		"SchedularTask", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
		configMINIMAL_STACK_SIZE, 							/* The size of the stack to allocate to the task. */
		NULL, 												/* The parameter passed to the task - not used in this simple case. */
		schedulerTask.fbsTaskPriority,				/* The priority assigned to the task. */
		schedulerTask.taskHandle);												/* The task handle is not required, so NULL is passed. */



	workerTasks[0].funcPtr = &fbsWorkerTask0;
	workerTasks[1].funcPtr = &fbsWorkerTask1;
	workerTasks[2].funcPtr = &fbsWorkerTask2;
	workerTasks[3].funcPtr = &fbsWorkerTask3;
	workerTasks[4].funcPtr = &fbsWorkerTask4;
	workerTasks[5].funcPtr = &fbsWorkerTask5;

	uint8_t taskCount = 0;

	for (taskCount = 0; taskCount < FBS_MAX_TASKS_WORKER; taskCount++)
	{
		workerTasks[taskCount].taskType = FBS_TASK_TYPE_WORKER;
		workerTasks[taskCount].fbsTaskPriority = FBS_TASK_PRIORITY_1;

		/*
		 * Create the task instances.
		 */
		xTaskCreate(workerTasks[taskCount].funcPtr,			/* The function that implements the task. */
			"WorkerTasks", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 							/* The size of the stack to allocate to the task. */
			&workerTasks[taskCount].taskType, 												/* The parameter passed to the task - not used in this simple case. */
			workerTasks[taskCount].fbsTaskPriority,				/* The priority assigned to the task. */
			workerTasks[taskCount].taskHandle);												/* The task handle is not required, so NULL is passed. */

	}

	/*
	* Start the task instances.
	*/
	vTaskStartScheduler();
    /*
     * TODO
     */
	for( ;; );
}
/*-----------------------------------------------------------*/


/*
 *  C function for tasks
 */

void fbsSchedulerTask(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = FBS_SCHEDULER_FREQUENCY_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	static frameCountNb = 0;

	printf("Scheduling Frame %d \n", frameCountNb);


	while (1)
	{
		 
		uint8_t workerTaskIterator = 0;
		uint8_t workerTaskToScheduleIterator = 0;
		for (workerTaskIterator = 0;workerTaskIterator < FBS_MAX_TASKS_WORKER;workerTaskIterator++)
		{
			workerTaskToScheduleIterator = 0;
			for (workerTaskToScheduleIterator = 0;workerTaskToScheduleIterator <= FBS_MAX_TASKS_IN_FRAME;workerTaskToScheduleIterator++)
			{
				if (fbsTaskLookupTable->tasksInFrame[workerTaskToScheduleIterator] == workerTaskIterator)
				{
					/*This worker task should be scheduled in this frame*/
					if (eTaskGetState(workerTasks[workerTaskIterator].taskHandle) != eReady &&
						eTaskGetState(workerTasks[workerTaskIterator].taskHandle) != eRunning)
					{
						//vTaskResume(workerTasks[workerTaskIterator].taskHandle);
					}
				
				}
				else
				{
					/*This worker task should not be scheduled in this frame*/
					vTaskSuspend(workerTasks[workerTaskIterator].taskHandle);
					printf("Suspended Worker Task Nb : %d", workerTaskIterator);
				}
			}
			
		}



		/*Delay the task until the block time*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		
		/*Checking the Frame Execution*/
		printf("Checking Frame %d \n", frameCountNb);
		vTaskResume(workerTasks[0].taskHandle);

		/*Handle Roll over*/
		frameCountNb++;
		if (frameCountNb >= FBS_MAX_FRAMES)
		{
			
			frameCountNb = 0;
		}
	}
}

void fbsWorkerTask0(void* taskParameters)
{
	static uint64_t fbsWorkTask0Counter;

	while (1)
	{
		fbsWorkTask0Counter++;

		printf("This is Worker Task 0 & I counted %d cycles\n", fbsWorkTask0Counter);
	}
	

}

void fbsWorkerTask1(void* taskParameters)
{
	static uint64_t fbsWorkTask1Counter;

	while (1)
	{
		fbsWorkTask1Counter++;

		printf("This is Worker Task 1 & I counted %d cycles\n", fbsWorkTask1Counter);

	}

	
}

void fbsWorkerTask2(void* taskParameters)
{
	static uint64_t fbsWorkTask2Counter;

	while (1)
	{
		fbsWorkTask2Counter++;

		printf("This is Worker Task 2 & I counted %d cycles\n", fbsWorkTask2Counter);
	}

}

void fbsWorkerTask3(void* taskParameters)
{
	static uint64_t fbsWorkTask3Counter;

	while (1)
	{
		fbsWorkTask3Counter++;

		printf("This is Worker Task 3 & I counted %d cycles\n", fbsWorkTask3Counter);
	}

}

void fbsWorkerTask4(void* taskParameters)
{
	static uint64_t fbsWorkTask4Counter;

	while (1)
	{
		fbsWorkTask4Counter++;

		printf("This is Worker Task 4 & I counted %d cycles\n", fbsWorkTask4Counter);
	}

}

void fbsWorkerTask5(void* taskParameters)
{
	static uint64_t fbsWorkTask5Counter;

	while (1)
	{
		fbsWorkTask5Counter++;

		printf("This is Worker Task 5 & I counted %d cycles\n", fbsWorkTask5Counter);
	}

}