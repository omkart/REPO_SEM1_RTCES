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
#include <stdint.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

//#define FBS_DEBUG_PRINTS				
#define FBS_PRINT_WORKER_TASK_LOGS			

#define FBS_MAX_TASKS_IN_FRAME			20
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
	uint8_t* framesToExecTasks;
}s_fbsTasks;


typedef struct
{
	uint8_t frameStart;
	uint8_t frameEnd;
	uint8_t nbTasksInFrame;
	uint8_t tasksInFrame[FBS_MAX_TASKS_IN_FRAME];


}s_fbsTaskLookupTable;


void fbsSchedulerTask(void* taskParameters);
void fbsWorkerTask0(void* taskParameters);
void fbsWorkerTask1(void* taskParameters);
void fbsWorkerTask2(void* taskParameters);
void fbsWorkerTask3(void* taskParameters);
void fbsWorkerTask4(void* taskParameters);
void fbsWorkerTask5(void* taskParameters);

void checkPreviousFrame(uint8_t frameCountNb);
void scheduleNewFrame(uint8_t frameCountNb);


/*
Table can also be initialised through initLookupTable for future use wherein that could become and API exposed for the 
algo that generates the scheduling details of the frames
Keeping hardcoded as of now for simplicity
*/
s_fbsTaskLookupTable fbsTaskLookupTable[FBS_MAX_FRAMES] =
{
	//frameStart				//frameEnd		//nbTasksInFrame			//tasksInFrame
	{0U,						120U,			6,							{0,1,2,3,4,5}	},
	{120U,						240U,			0,							{0}				},
	{240U,						360U,			3,							{0,1,4}			},
	{360U,						480U,			2,							{2,3}			},
	{480U,						600U,			3,							{0,1,4}			},
};


static s_fbsTasks schedulerTask;
static s_fbsTasks workerTasks[FBS_MAX_TASKS_WORKER];
static bool scheduleStartedForTheFirstTime = true;

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
			NULL, 												/* The parameter passed to the task - not used in this simple case. */
			workerTasks[taskCount].fbsTaskPriority,				/* The priority assigned to the task. */
			&workerTasks[taskCount].taskHandle);												/* The task handle is not required, so NULL is passed. */

		/*Find the frame number for worker task*/
		uint8_t frameNbFound[FBS_MAX_FRAMES] = {0};
		uint8_t frameNb, taskNbItr,i = 0;
		for (frameNb = 0;frameNb < FBS_MAX_FRAMES; frameNb++)
		{
			for (taskNbItr = 0;taskNbItr < fbsTaskLookupTable[frameNb].nbTasksInFrame;taskNbItr++)
			{
				/*
					This check suggests the worker task is to be executed in this particular frame
					If thats the case then we just capture that frame number and store it
				*/
				if (fbsTaskLookupTable[frameNb].tasksInFrame[taskNbItr] == taskCount)
				{
					frameNbFound[i] = frameNb;
					i++;
				}
			}
			
		}

		/*Once all the frame numbers are found for the current worker task, we will create an array of the frames
		where this worker task needs to be executed*/
		/*First set all the array elements with a NULL and then copy the required data at appropriate locations*/
		workerTasks[taskCount].framesToExecTasks = malloc(FBS_MAX_FRAMES * sizeof(uint8_t));

		/*0xFF used as an invalid data*/
		memset(workerTasks[taskCount].framesToExecTasks, 0xFF, FBS_MAX_FRAMES * sizeof(uint8_t));
		memcpy(workerTasks[taskCount].framesToExecTasks, frameNbFound, (i * sizeof(uint8_t)));
	}


	#ifdef FBS_DEBUG_PRINTS == 1
	 
		uint8_t id = 0;
		for (id = 0;id < FBS_MAX_FRAMES; id++)
		{
			printf("Frame numbers attached : %d\n", workerTasks[0].framesToExecTasks[id]);
		}
	#endif



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
	static uint8_t currentFrameCountNb = 0;
	static uint8_t previousFrameCountNb = 0;

	while (1)
	{

		if (scheduleStartedForTheFirstTime == false)
		{
			
			/*Checking the Frame Execution*/
			printf("Checking Frame %d \n\n\n", previousFrameCountNb);

			checkPreviousFrame(previousFrameCountNb);
		}
		else
		{
			/*Scheduling for the first time*/
			printf("Its the very first frame \n", currentFrameCountNb);
			scheduleStartedForTheFirstTime = false;
		}

		printf("Scheduling Frame %d \n", currentFrameCountNb);

		scheduleNewFrame(currentFrameCountNb);


		

		/*Delay the task until the block time*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		
		

		
		previousFrameCountNb = currentFrameCountNb;
		currentFrameCountNb++;
		
		/*Handle Roll over*/
		if (currentFrameCountNb >= FBS_MAX_FRAMES)
		{
			
			currentFrameCountNb = 0;
		}
	}
}

void fbsWorkerTask0(void* taskParameters)
{
	static uint64_t fbsWorkTask0Counter;

	while (1)
	{
		fbsWorkTask0Counter++;

		#ifdef FBS_PRINT_WORKER_TASK_LOGS == 1
				printf("This is Worker Task 0 & I counted %d cycles\n", fbsWorkTask0Counter);
		#endif


		/*After count up the task will suspend itself*/
		vTaskSuspend(NULL);
	}
	

}

void fbsWorkerTask1(void* taskParameters)
{
	static uint64_t fbsWorkTask1Counter;

	while (1)
	{
		fbsWorkTask1Counter++;
		#ifdef FBS_PRINT_WORKER_TASK_LOGS 
			printf("This is Worker Task 1 & I counted %d cycles\n", fbsWorkTask1Counter);
		#endif

		/*After count up the task will suspend itself*/
		vTaskSuspend(NULL);

	}

	
}

void fbsWorkerTask2(void* taskParameters)
{
	static uint64_t fbsWorkTask2Counter;

	while (1)
	{
		fbsWorkTask2Counter++;
		#ifdef FBS_PRINT_WORKER_TASK_LOGS
				printf("This is Worker Task 2 & I counted %d cycles\n", fbsWorkTask2Counter);
		#endif
		
				
		/*After count up the task will suspend itself*/
		vTaskSuspend(NULL);
	}

}

void fbsWorkerTask3(void* taskParameters)
{
	static uint64_t fbsWorkTask3Counter;

	while (1)
	{
		fbsWorkTask3Counter++;
		#ifdef FBS_PRINT_WORKER_TASK_LOGS 
				printf("This is Worker Task 3 & I counted %d cycles\n", fbsWorkTask3Counter);
		#endif

		/*After count up the task will suspend itself*/
		vTaskSuspend(NULL);
	}

}

void fbsWorkerTask4(void* taskParameters)
{
	static uint64_t fbsWorkTask4Counter;

	while (1)
	{
		fbsWorkTask4Counter++;
		#ifdef FBS_PRINT_WORKER_TASK_LOGS 
				printf("This is Worker Task 4 & I counted %d cycles\n", fbsWorkTask4Counter);
		#endif
		

		/*After count up the task will suspend itself*/
		vTaskSuspend(NULL);
	}

}

void fbsWorkerTask5(void* taskParameters)
{
	static uint64_t fbsWorkTask5Counter;

	while (1)
	{
		fbsWorkTask5Counter++;
		#ifdef FBS_PRINT_WORKER_TASK_LOGS 
		/*commenting print logs because the log window gets stuck after running this printf*/
		//printf("This is Worker Task 5 & I counted %d cycles\n", fbsWorkTask5Counter);
		#endif


		/*After count up the task will suspend itself*/
		/*This task will misbehave and will not suspend itself*/
		//vTaskSuspend(NULL);
	}

}



void checkPreviousFrame(uint8_t frameCountNb)
{
	uint8_t taskCount = 0;
	for (taskCount = 0; taskCount < FBS_MAX_TASKS_WORKER; taskCount++)
	{
		/*Check for overruns*/
		if (eTaskGetState(workerTasks[taskCount].taskHandle) != eSuspended)
		{
			printf("\nTask %d in Frame %d was not suspended. Sad.\nSuspending task %d for good.\n\n", taskCount, frameCountNb,taskCount);
			/*Suspending the overrun task*/
			vTaskSuspend(workerTasks[taskCount].taskHandle);
		}
		else
		{
			/*Everything is running correctly*/
		}
	}

}


void scheduleNewFrame(uint8_t frameCountNb)
{
	uint8_t iterator = 0;
	bool breakCondition = 0;

	uint8_t taskCount = 0;

	for (taskCount = 0; taskCount < FBS_MAX_TASKS_WORKER; taskCount++)
	{

		breakCondition = false;
		for (iterator = 0; (iterator < FBS_MAX_FRAMES) && (breakCondition == false);iterator++)
		{
			if (workerTasks[taskCount].framesToExecTasks[iterator] == frameCountNb)
			{
				/*We have found that this worker task should be executed in the current frame number
				So we will schedule it*/
				breakCondition = true;
			}
			else
			{
				/*do nothing*/
			}
		}

		if (breakCondition == true)
		{
			/*We schedule this task*/
			printf("Schedule worker task %d\n", taskCount);
			vTaskResume(workerTasks[taskCount].taskHandle);
		}
		else
		{
			/*We will not schedule this task*/

		}
	}
}