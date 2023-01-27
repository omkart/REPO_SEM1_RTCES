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

#define PCP_TASKS_MAX					4U
#define PCP_TASK_PERIOD					pdMS_TO_TICKS( 10000UL )		// 10s

#define TIME_UNIT_TO_MS_RES					100	//Resolution of 100ms 
#define mainNUMBER_OF_SEMAPHORS					( 3 )

#define TICKS_DEVIATION_MARGIN		10

// TODO

#define workersUSELESS_CYCLES_PER_TIME_UNIT		( 1000000UL)
/*
	Priorities at which the tasks are created.
 */
typedef enum
{
	PCP_TASK_PRIORITY_1 = (tskIDLE_PRIORITY + 1), //Lowest priority
	PCP_TASK_PRIORITY_2,
	PCP_TASK_PRIORITY_3,
	PCP_TASK_PRIORITY_4,
	PCP_TASK_PRIORITY_5,
	PCP_TASK_PRIORITY_6,
	PCP_TASK_PRIORITY_UNDEFINED,
}e_pcpTaskPriority;


typedef enum
{
	PCP_TASK_NUMBER_1 = 0,
	PCP_TASK_NUMBER_2,
	PCP_TASK_NUMBER_3,
	PCP_TASK_NUMBER_4,
	PCP_TASK_NUMBER_MAX,

}e_pcpTaskNumber;


typedef enum
{
	PCP_RESOURCE_A = 0,
	PCP_RESOURCE_B,
	PCP_RESOURCE_C,
};
/*
  *  data structure
  */


typedef union
{
	uint32_t arrayVal[3];
	struct
	{
		uint32_t ceilingPriority : 32;
		uint32_t lockingTime : 32;
		uint32_t unlockingTime : 32;

	};

}u_resource;


typedef struct
{
	e_pcpTaskPriority priority;
	uint32_t relativeReleaseTime;
	uint32_t relativeExecTime;
	unsigned long period;
	void (*funcPtr)(void);

	//Every task can access 3 such resources
	u_resource resources[mainNUMBER_OF_SEMAPHORS];
	e_pcpTaskNumber taskNb;
	TaskHandle_t taskHandle;
}s_pcpTasks;




/*-----------------------------------------------------------*/

// TODO

static void vUselessLoad(uint32_t ulCycles);
static void prvTask1(void* pvParameters);
static void prvTask2(void* pvParameters);
static void prvTask3(void* pvParameters);
static void prvTask4(void* pvParameters);

void usPrioritySemaphoreWait(s_pcpTasks* pcpTask,uint32_t startTimeOfTask);
void usPrioritySemaphoreSignal(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);


static void accessSharedResourceA(s_pcpTasks* pcpTask, uint32_t ulTimeUnits);
static void accessSharedResourceB(s_pcpTasks* pcpTask, uint32_t ulTimeUnits);
static void accessSharedResourceC(s_pcpTasks* pcpTask, uint32_t ulTimeUnits);


inline void usPrioritySemaphoreSignalResourceHandlerA(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);
inline void usPrioritySemaphoreSignalResourceHandlerB(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);
inline void usPrioritySemaphoreSignalResourceHandlerC(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);

inline void usPrioritySemaphoreSignalHandlerA(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);
inline void usPrioritySemaphoreSignalHandlerB(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);
inline void usPrioritySemaphoreSignalHandlerC(s_pcpTasks* pcpTask, uint32_t startTimeOfTask);


/*-----------------------------------------------------------*/



s_pcpTasks pcpTasks[PCP_TASKS_MAX] = {

	//Priority,					release		exec		period					function,		Resource accessed
	{PCP_TASK_PRIORITY_5,		10U,		5U,			PCP_TASK_PERIOD,		&prvTask1,		{{ PCP_TASK_PRIORITY_4,NULL,NULL},			{ PCP_TASK_PRIORITY_5,1,2 },				{ PCP_TASK_PRIORITY_5,3,4 }			},		PCP_TASK_NUMBER_1},
	{PCP_TASK_PRIORITY_4,		3U,			7U,			PCP_TASK_PERIOD,		&prvTask2,		{{ PCP_TASK_PRIORITY_4,5,6 },				{ PCP_TASK_PRIORITY_5,NULL,NULL },			{ PCP_TASK_PRIORITY_5,1,3 }			},		PCP_TASK_NUMBER_2},
	{PCP_TASK_PRIORITY_3,		5U,			8U,			PCP_TASK_PERIOD,		&prvTask3,		{{ PCP_TASK_PRIORITY_4,3,5 },				{ PCP_TASK_PRIORITY_5,2,7 },				{ PCP_TASK_PRIORITY_5,NULL,NULL }	},		PCP_TASK_NUMBER_3},
	{PCP_TASK_PRIORITY_2,		0U,			9U,			PCP_TASK_PERIOD,		&prvTask4,		{{ PCP_TASK_PRIORITY_4,2,8 },				{ PCP_TASK_PRIORITY_5,4,6 },				{ PCP_TASK_PRIORITY_5,NULL,NULL }	},		PCP_TASK_NUMBER_4},
};

static SemaphoreHandle_t semaphoreResourceA, semaphoreResourceB, semaphoreResourceC;

static uint32_t globalTimeProfile;


void main_exercise(void)
{
	//resources[0].xSemaphore = xSemaphoreCreateBinary();

	semaphoreResourceA = xSemaphoreCreateMutex();
	semaphoreResourceB = xSemaphoreCreateMutex();
	semaphoreResourceC = xSemaphoreCreateMutex();


	if (semaphoreResourceA == NULL)
	{
		printf("There was insufficient FreeRTOS heap available for the semaphore to be created successfully\n");
	}
	else
	{
		/* The semaphore can now be used. Its handle is stored in the
		xSemahore variable.  Calling xSemaphoreTake() on the semaphore here
		will fail until the semaphore has first been given. */
	}


	uint8_t taskCount = 0;

	for (taskCount = 0; taskCount < PCP_TASKS_MAX; taskCount++)
	{
		//Convert unit time to 100ms resolution and convert from ms to ticks
		pcpTasks[taskCount].relativeReleaseTime = pdMS_TO_TICKS(pcpTasks[taskCount].relativeReleaseTime * TIME_UNIT_TO_MS_RES);
		pcpTasks[taskCount].relativeExecTime = pdMS_TO_TICKS(pcpTasks[taskCount].relativeExecTime * TIME_UNIT_TO_MS_RES);


		uint8_t resourceCount = 0;
		for (resourceCount = 0; resourceCount < 3;resourceCount++)
		{
			if (pcpTasks[taskCount].resources[resourceCount].lockingTime != NULL &&
				pcpTasks[taskCount].resources[resourceCount].unlockingTime != NULL)
			{
				//Convert unit time to 100ms resolution
				pcpTasks[taskCount].resources[resourceCount].lockingTime = pdMS_TO_TICKS(pcpTasks[taskCount].resources[resourceCount].lockingTime * TIME_UNIT_TO_MS_RES);;
				pcpTasks[taskCount].resources[resourceCount].unlockingTime = pdMS_TO_TICKS(pcpTasks[taskCount].resources[resourceCount].unlockingTime * TIME_UNIT_TO_MS_RES);;

				//Convert relative locking unlocking timings to absolute timings from the point of release time
				//pcpTasks[taskCount].resources[resourceCount].lockingTime += pcpTasks[taskCount].relativeReleaseTime;
				//pcpTasks[taskCount].resources[resourceCount].unlockingTime += pcpTasks[taskCount].relativeReleaseTime;
			}

		}


		/*
		 * Create the task instances.
		 */
		xTaskCreate(pcpTasks[taskCount].funcPtr,					/* The function that implements the task. */
			"PCP_TASKS", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 								/* The size of the stack to allocate to the task. */
			NULL, 													/* The parameter passed to the task - not used in this simple case. */
			pcpTasks[taskCount].priority,							/* The priority assigned to the task. */
			&pcpTasks[taskCount].taskHandle);						/* The task handle is not required, so NULL is passed. */

	}

	globalTimeProfile = xTaskGetTickCount();

	/*
	* Start the task instances.
	*/
	vTaskStartScheduler();


	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created. */
	while (1);
}
/*-----------------------------------------------------------*/

static void vUselessLoad(uint32_t ulTimeUnits) {
	uint32_t ulUselessVariable = 0;
	uint32_t currentTick = xTaskGetTickCount();
	while ((xTaskGetTickCount() - currentTick) < ulTimeUnits)
	{
		ulUselessVariable++;
	}
}

// TODO

static void prvTask1(void* pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_1].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	/*Delay the task until the block time*/
	vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	static uint32_t startTimeOfTaskOfTaskA; //get the current tick count once the task is started
	startTimeOfTaskOfTaskA = xTaskGetTickCount();
	printf("Task 1 started\n");
	while (1)
	{
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_1], startTimeOfTaskOfTaskA);

		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_1], startTimeOfTaskOfTaskA);

		//Continue to execute the normal task
		vUselessLoad(1);
	}

}

static void prvTask2(void* pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_2].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	/*Delay the task until the block time*/
	vTaskDelayUntil(&xNextWakeTime, xBlockTime);

	static uint32_t startTimeOfTaskOfTaskB; //get the current tick count once the task is started
	startTimeOfTaskOfTaskB = xTaskGetTickCount();
	printf("Task 2 started\n");
	while (1)
	{
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_2], startTimeOfTaskOfTaskB);

		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_2], startTimeOfTaskOfTaskB);

		//Continue to execute the normal task
		vUselessLoad(1);
	}
}

static void prvTask3(void* pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_3].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	/*Delay the task until the block time*/
	vTaskDelayUntil(&xNextWakeTime, xBlockTime);

	static uint32_t startTimeOfTaskOfTaskC; //get the current tick count once the task is started
	startTimeOfTaskOfTaskC = xTaskGetTickCount();
	printf("Task 3 started\n");
	while (1)
	{
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_3], startTimeOfTaskOfTaskC);

		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_3], startTimeOfTaskOfTaskC);

		//Continue to execute the normal task
		vUselessLoad(1);
	}
}

static void prvTask4(void* pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_4].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	/*Delay the task until the block time*/
	//no delay required here and will start as soon as its available

	uint32_t startTimeOfTask = xTaskGetTickCount(); //get the current tick count once the task is started
	printf("Task 4 started\n");
	while (1)
	{
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_4], startTimeOfTask);

		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_4], startTimeOfTask);

		//Continue to execute the normal task
		vUselessLoad(1);
	}
}


//Wrapper for take() function of semaphore
void usPrioritySemaphoreWait(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//For Resource A
	usPrioritySemaphoreSignalResourceHandlerA(pcpTask, startTimeOfTask);

	//For Resource B
	usPrioritySemaphoreSignalResourceHandlerB(pcpTask, startTimeOfTask);


	//For Resource C
	usPrioritySemaphoreSignalResourceHandlerC(pcpTask, startTimeOfTask);

}


//Wrapper for give() function of semaphore
void usPrioritySemaphoreSignal(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{

	//For Resource A
	usPrioritySemaphoreSignalHandlerA(pcpTask, startTimeOfTask);

	//For Resource B
	usPrioritySemaphoreSignalHandlerB(pcpTask, startTimeOfTask);

	//For Resource C
	usPrioritySemaphoreSignalHandlerC(pcpTask, startTimeOfTask);


}


inline void usPrioritySemaphoreSignalResourceHandlerA(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);
	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_A].lockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_A].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_A].lockingTime - TICKS_DEVIATION_MARGIN) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_A].lockingTime + TICKS_DEVIATION_MARGIN)
			&& xSemaphoreGetMutexHolder(semaphoreResourceA) != pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreTake(semaphoreResourceA, (TickType_t)1) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->resources[PCP_RESOURCE_A].ceilingPriority);

				printf("Task %d acquired resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_A + 1, pcpTask->priority, pcpTask->resources[PCP_RESOURCE_A].ceilingPriority);

				//Send the value in terms of ticks
				accessSharedResourceA(pcpTask, (pcpTask->resources[PCP_RESOURCE_A].unlockingTime - pcpTask->resources[PCP_RESOURCE_A].lockingTime));

			}
			else
			{
				printf("Task %d could NOT acquire resource %d with priority of %d\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_A + 1, pcpTask->priority);
			}
		}
	}
	
}
inline void usPrioritySemaphoreSignalResourceHandlerB(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_B].lockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_B].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_B].lockingTime - TICKS_DEVIATION_MARGIN) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_B].lockingTime + TICKS_DEVIATION_MARGIN)
			&& xSemaphoreGetMutexHolder(semaphoreResourceB) != pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreTake(semaphoreResourceB, (TickType_t)1) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->resources[PCP_RESOURCE_B].ceilingPriority);

				printf("Task %d acquired resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_B + 1, pcpTask->priority, pcpTask->resources[PCP_RESOURCE_B].ceilingPriority);

				//Send the value in terms of ticks
				accessSharedResourceB(pcpTask, (pcpTask->resources[PCP_RESOURCE_B].unlockingTime - pcpTask->resources[PCP_RESOURCE_B].lockingTime));

			}
			else
			{
				printf("Task %d could NOT acquire resource %d with priority of %d\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_B + 1, pcpTask->priority);
			}
		}
	}

}

inline void usPrioritySemaphoreSignalResourceHandlerC(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_C].lockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_C].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_C].lockingTime - TICKS_DEVIATION_MARGIN) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_C].lockingTime + TICKS_DEVIATION_MARGIN)
			&& xSemaphoreGetMutexHolder(semaphoreResourceC) != pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreTake(semaphoreResourceC, (TickType_t)1) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->resources[PCP_RESOURCE_C].ceilingPriority);

				printf("Task %d acquired resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_C + 1, pcpTask->priority, pcpTask->resources[PCP_RESOURCE_C].ceilingPriority);

				//Send the value in terms of ticks
				accessSharedResourceC(pcpTask, (pcpTask->resources[PCP_RESOURCE_C].unlockingTime - pcpTask->resources[PCP_RESOURCE_C].lockingTime));

			}
			else
			{
				printf("Task %d could NOT acquire resource %d with priority of %d\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_C + 1, pcpTask->priority);
			}
		}
	}
}


inline void usPrioritySemaphoreSignalHandlerA(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_A].unlockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_A].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_A].unlockingTime - 10) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_A].unlockingTime + 10)
			&& xSemaphoreGetMutexHolder(semaphoreResourceA) == pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreGive(semaphoreResourceA, (TickType_t)1) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->priority);

				printf("Task %d released resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_A + 1, pcpTask->resources[PCP_RESOURCE_A].ceilingPriority, pcpTask->priority);

			}
			else
			{
				printf("Task %d could NOT release resource %d with priority %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_A + 1, pcpTask->priority);
			}
		}
	}
}

inline void usPrioritySemaphoreSignalHandlerB(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_B].unlockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_B].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_B].unlockingTime - 10) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_B].unlockingTime + 10)
			&& xSemaphoreGetMutexHolder(semaphoreResourceB) == pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreGive(semaphoreResourceB, (TickType_t)1) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->priority);

				printf("Task %d released resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_B + 1, pcpTask->resources[PCP_RESOURCE_B].ceilingPriority, pcpTask->priority);

			}
			else
			{
				printf("Task %d could NOT release resource %d with priority %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_B + 1, pcpTask->priority);
			}
		}
	}
}


inline void usPrioritySemaphoreSignalHandlerC(s_pcpTasks* pcpTask, uint32_t startTimeOfTask)
{
	//In ticks
	uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

	//Check if the task is supposed to access this resource or not
	if (pcpTask->resources[PCP_RESOURCE_C].unlockingTime != NULL &&
		pcpTask->resources[PCP_RESOURCE_C].unlockingTime != NULL)
	{
		if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_C].unlockingTime - 10) && \
			currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_C].unlockingTime + 10)
			&& xSemaphoreGetMutexHolder(semaphoreResourceC) == pcpTask->taskHandle)
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 5 ticks to see if it becomes free. */
			if (xSemaphoreGive(semaphoreResourceC, (TickType_t)5) == pdTRUE)
			{
				//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
				//vTaskPrioritySet(pcpTask->taskHandle, pcpTask->priority);

				printf("Task %d released resource %d and changed its priority from %d to %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_C + 1, pcpTask->resources[PCP_RESOURCE_C].ceilingPriority, pcpTask->priority);

			}
			else
			{
				printf("Task %d could NOT release resource %d with priority %d.\n",
					pcpTask->taskNb + 1, PCP_RESOURCE_C + 1, pcpTask->priority);
			}
		}
	}
}


static void accessSharedResourceA(s_pcpTasks* pcpTask, uint32_t ulTimeUnits)
{
	//Accessing the shared Resource A
	printf("Shared Resource being accessed by Task %d\n", pcpTask->taskNb + 1);
	uint32_t ulUselessVariable = 0;
	uint32_t currentTick = xTaskGetTickCount();
	while ((xTaskGetTickCount() - currentTick) < ulTimeUnits)
	{
		ulUselessVariable++;
	}

}

static void accessSharedResourceB(s_pcpTasks* pcpTask, uint32_t ulTimeUnits)
{
	//Accessing the shared Resource A
	printf("Shared Resource being accessed by Task %d\n", pcpTask->taskNb + 1);
	uint32_t ulUselessVariable = 0;
	uint32_t currentTick = xTaskGetTickCount();
	while ((xTaskGetTickCount() - currentTick) < ulTimeUnits)
	{
		ulUselessVariable++;
	}

}


static void accessSharedResourceC(s_pcpTasks* pcpTask, uint32_t ulTimeUnits)
{
	//Accessing the shared Resource A
	printf("Shared Resource being accessed by Task %d\n", pcpTask->taskNb + 1);
	uint32_t ulUselessVariable = 0;
	uint32_t currentTick = xTaskGetTickCount();
	while ((xTaskGetTickCount() - currentTick) < ulTimeUnits)
	{
		ulUselessVariable++;
	}

}

