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

/*
 Q3 b )

 - Yes there could be significant differences between a system call and a wrapper function developed below on application level.
 The system calls could be atomic in nature ensuring there are no incorrect context switch taking place whereas that isnt the 
 case with wrapper function (usPrioritySemaphoreWait() )
 - It could essentially be more optimised and would reduce the deviation in time at which the resource access request has been made
 and the time at which the resource access request is actually processed when the resource is available for a given task
 - There could be edge cases when changing the priority and its impact on the overall scheduling which may not be covered
 in the current wrapper function. System calls should be able to take care of these cases

*/




#define PCP_TASKS_MAX					4U
#define PCP_TASK_PERIOD					pdMS_TO_TICKS( 20000UL )		// 10s

#define TIME_UNIT_TO_MS_RES						100	//Resolution of 100ms 
#define mainNUMBER_OF_SEMAPHORS					( 3 )

#define TICKS_DEVIATION_MARGIN					10

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
}e_pcpResource;
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
	SemaphoreHandle_t semaphoreHandle;
	TaskHandle_t* taskAssigned;
}s_semaphoreData;

typedef struct
{
	e_pcpTaskPriority originalPriority;
	uint32_t relativeReleaseTime;
	uint32_t relativeExecTime;
	unsigned long period;
	void (*funcPtr)(void);

	//Every task can access 3 such resources
	u_resource resources[mainNUMBER_OF_SEMAPHORS];
	e_pcpTaskNumber taskNb;
	TaskHandle_t taskHandle;
	e_pcpTaskPriority previousPriority;
}s_pcpTasks;

s_semaphoreData semaphoreData[mainNUMBER_OF_SEMAPHORS];


/*-----------------------------------------------------------*/

// TODO

static void vUselessLoad(uint32_t ulCycles);
static void prvTask1(void* pvParameters);
static void prvTask2(void* pvParameters);
static void prvTask3(void* pvParameters);
static void prvTask4(void* pvParameters);

void usPrioritySemaphoreWait(s_pcpTasks* pcpTask, e_pcpResource resource);
void usPrioritySemaphoreSignal(s_pcpTasks* pcpTask, e_pcpResource resource);



/*-----------------------------------------------------------*/



s_pcpTasks pcpTasks[PCP_TASKS_MAX] = {

	//Priority,					release		exec		period					function,		Resource accessed
	{PCP_TASK_PRIORITY_5,		10U,		5U,			PCP_TASK_PERIOD,		&prvTask1,		{{ PCP_TASK_PRIORITY_4,NULL,NULL},			{ PCP_TASK_PRIORITY_5,1,2 },				{ PCP_TASK_PRIORITY_5,3,4 }			},		PCP_TASK_NUMBER_1},
	{PCP_TASK_PRIORITY_4,		3U,			7U,			PCP_TASK_PERIOD,		&prvTask2,		{{ PCP_TASK_PRIORITY_4,5,6 },				{ PCP_TASK_PRIORITY_5,NULL,NULL },			{ PCP_TASK_PRIORITY_5,1,3 }			},		PCP_TASK_NUMBER_2},
	{PCP_TASK_PRIORITY_3,		5U,			8U,			PCP_TASK_PERIOD,		&prvTask3,		{{ PCP_TASK_PRIORITY_4,3,5 },				{ PCP_TASK_PRIORITY_5,2,7 },				{ PCP_TASK_PRIORITY_5,NULL,NULL }	},		PCP_TASK_NUMBER_3},
	{PCP_TASK_PRIORITY_2,		0U,			9U,			PCP_TASK_PERIOD,		&prvTask4,		{{ PCP_TASK_PRIORITY_4,2,8 },				{ PCP_TASK_PRIORITY_5,4,6 },				{ PCP_TASK_PRIORITY_5,NULL,NULL }	},		PCP_TASK_NUMBER_4},
};

static uint32_t globalTimeProfile;


void main_exercise(void)
{
	/*Creating the Semaphores for Resource*/

	semaphoreData[PCP_RESOURCE_A].semaphoreHandle = xSemaphoreCreateMutex();
	semaphoreData[PCP_RESOURCE_B].semaphoreHandle = xSemaphoreCreateMutex();
	semaphoreData[PCP_RESOURCE_C].semaphoreHandle = xSemaphoreCreateMutex();

	/*Verify if the creating of Semaphore was a success or not*/
	if (semaphoreData[PCP_RESOURCE_A].semaphoreHandle == NULL || \
		semaphoreData[PCP_RESOURCE_B].semaphoreHandle == NULL || \
		semaphoreData[PCP_RESOURCE_C].semaphoreHandle == NULL )
	{
		printf("There was insufficient FreeRTOS heap available for the semaphore to be created successfully\n");
	}
	else
	{
		/* The semaphore can now be used. */
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

		/*Previous priority will be the original priority initally*/
		pcpTasks[taskCount].previousPriority = pcpTasks[taskCount].originalPriority;


		/*
		 * Create the task instances.
		 */
		xTaskCreate(pcpTasks[taskCount].funcPtr,					/* The function that implements the task. */
			"PCP_TASKS", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 								/* The size of the stack to allocate to the task. */
			NULL, 													/* The parameter passed to the task - not used in this simple case. */
			pcpTasks[taskCount].originalPriority,							/* The priority assigned to the task. */
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
	TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_1].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	

	static uint32_t startTimeOfTaskOfTaskA; //get the current tick count once the task is started
	startTimeOfTaskOfTaskA = xTaskGetTickCount();
	
	while (1)
	{
		/*Delay the task until the release time on every period*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		//printf("Task 1 started\n");

		/*Accessing the Resource B*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_B].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_1], PCP_RESOURCE_B);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_B].unlockingTime - pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_B].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_1], PCP_RESOURCE_B);

		/*Accessing the Resource C*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_C].lockingTime - pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_B].unlockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_1], PCP_RESOURCE_C);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_C].unlockingTime - pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_C].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_1], PCP_RESOURCE_C);


		vUselessLoad( pcpTasks[PCP_TASK_NUMBER_1].relativeExecTime - pcpTasks[PCP_TASK_NUMBER_1].resources[PCP_RESOURCE_C].unlockingTime);
		
		xBlockTime = pcpTasks[PCP_TASK_NUMBER_1].period;
		/*Execute the next cycle*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	}

}

static void prvTask2(void* pvParameters)
{
	TickType_t xNextWakeTime;
	TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_2].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	static uint32_t startTimeOfTaskOfTaskB; //get the current tick count once the task is started
	startTimeOfTaskOfTaskB = xTaskGetTickCount();

	while (1)
	{
		/*Delay the task until the release time on every period*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		//printf("Task 2 started\n");


		/*Accessing the Resource C*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_C].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_2], PCP_RESOURCE_C);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_C].unlockingTime - pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_C].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_2], PCP_RESOURCE_C);

		/*Accessing the Resource A*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_A].lockingTime - pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_C].unlockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_2], PCP_RESOURCE_A);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_A].unlockingTime - pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_A].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_2], PCP_RESOURCE_A);


		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_2].relativeExecTime - pcpTasks[PCP_TASK_NUMBER_2].resources[PCP_RESOURCE_A].unlockingTime);

		xBlockTime = pcpTasks[PCP_TASK_NUMBER_2].period;
		/*Execute the next cycle*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	}
}

static void prvTask3(void* pvParameters)
{
	TickType_t xNextWakeTime;
	TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_3].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	static uint32_t startTimeOfTaskOfTaskC; //get the current tick count once the task is started
	startTimeOfTaskOfTaskC = xTaskGetTickCount();

	while (1)
	{
		/*Delay the task until the release time on every period*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		//printf("Task 3 started\n");


		/*Accessing the Resource B & A*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_B].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_3], PCP_RESOURCE_B);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_A].lockingTime - pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_B].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_3], PCP_RESOURCE_A);

		/*Releasing the Resource A & B*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_A].unlockingTime - pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_A].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_3], PCP_RESOURCE_A);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_B].unlockingTime - pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_A].unlockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_3], PCP_RESOURCE_B);


		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_3].relativeExecTime - pcpTasks[PCP_TASK_NUMBER_3].resources[PCP_RESOURCE_B].unlockingTime);

		xBlockTime = pcpTasks[PCP_TASK_NUMBER_3].period;
		/*Execute the next cycle*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	
	}
}

static void prvTask4(void* pvParameters)
{
	TickType_t xNextWakeTime;
	TickType_t xBlockTime = pcpTasks[PCP_TASK_NUMBER_4].relativeReleaseTime;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	/*Delay the task until the block time*/
	//no delay required here and will start as soon as its available

	uint32_t startTimeOfTask = xTaskGetTickCount(); //get the current tick count once the task is started
	while (1)
	{
		//printf("Task 4 started\n");

		/*Accessing the Resource B & A*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_A].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_4], PCP_RESOURCE_A);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_B].lockingTime - pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_A].lockingTime);
		usPrioritySemaphoreWait(&pcpTasks[PCP_TASK_NUMBER_4], PCP_RESOURCE_B);

		/*Releasing the Resource A & B*/
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_B].unlockingTime - pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_B].lockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_4], PCP_RESOURCE_B);
		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_A].unlockingTime - pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_B].unlockingTime);
		usPrioritySemaphoreSignal(&pcpTasks[PCP_TASK_NUMBER_4], PCP_RESOURCE_A);


		vUselessLoad(pcpTasks[PCP_TASK_NUMBER_4].relativeExecTime - pcpTasks[PCP_TASK_NUMBER_4].resources[PCP_RESOURCE_A].unlockingTime);

		xBlockTime = pcpTasks[PCP_TASK_NUMBER_4].period;
		/*Execute the next cycle*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	}
}


//Wrapper for take() function of semaphore
void usPrioritySemaphoreWait(s_pcpTasks* pcpTask, e_pcpResource pcpResource)
{
	uint32_t startTimeOfTask = pcpTask->relativeReleaseTime;

	if (pcpTask != NULL)
	{
		//In ticks
		uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);


		//Check if the task is supposed to access this resource or not
		if (pcpTask->resources[pcpResource].lockingTime != NULL &&
			pcpTask->resources[pcpResource].unlockingTime != NULL)
		{
			//Checking its time to access the given resource
			//if (currentRelativeTime >= (pcpTask->resources[pcpResource].lockingTime - TICKS_DEVIATION_MARGIN) && \
			//	currentRelativeTime < (pcpTask->resources[pcpResource].lockingTime + TICKS_DEVIATION_MARGIN) && \

				//Checking if the task does not have already accessed the resource already
			if(xSemaphoreGetMutexHolder(semaphoreData[pcpResource].semaphoreHandle) != pcpTask->taskHandle)
			{
				/* See if we can obtain the semaphore.  If the semaphore is not
				available wait 5 ticks to see if it becomes free. */
				uint32_t priorityBeforeApplyingCeilingFunc = uxTaskPriorityGet(pcpTask->taskHandle);
				if (xSemaphoreTake(semaphoreData[pcpResource].semaphoreHandle, (TickType_t)1) == pdTRUE)
				{
					//Once we access the semaphore, we will update the priority of the existing task to the ceiling function priority of the resource
					
					if (pcpTask->resources[pcpResource].ceilingPriority > priorityBeforeApplyingCeilingFunc)
					{
						vTaskPrioritySet(pcpTask->taskHandle, pcpTask->resources[pcpResource].ceilingPriority);

						printf("Task %d acquired resource %d and changed its priority from %d to %d.\n",
							pcpTask->taskNb + 1, pcpResource + 1, priorityBeforeApplyingCeilingFunc, pcpTask->resources[pcpResource].ceilingPriority);

					}
					else
					{
						
						printf("Task %d acquired resource %d and its priority is not changed & remains as %d.\n",
							pcpTask->taskNb + 1, pcpResource + 1, priorityBeforeApplyingCeilingFunc);
					}
					/*Retain the priority of the task and store it*/
					pcpTask->previousPriority = priorityBeforeApplyingCeilingFunc;

				}
				else
				{
					//printf("Task %d could NOT acquire resource %d with priority of %d\n",
						//pcpTask->taskNb + 1, pcpResource + 1, currentTaskPriority);
				}
			}
		}
	}

}


//Wrapper for give() function of semaphore
void usPrioritySemaphoreSignal(s_pcpTasks* pcpTask, e_pcpResource pcpResource)
{
	uint32_t startTimeOfTask = pcpTask->relativeReleaseTime;

	if (pcpTask != NULL)
	{
		//In ticks
		uint32_t currentRelativeTime = (xTaskGetTickCount() - startTimeOfTask);

		//Check if the task is supposed to access this resource or not
		if (pcpTask->resources[pcpResource].unlockingTime != NULL &&
			pcpTask->resources[pcpResource].unlockingTime != NULL)
		{
			//if (currentRelativeTime >= (pcpTask->resources[PCP_RESOURCE_A].unlockingTime - TICKS_DEVIATION_MARGIN) && \
				//currentRelativeTime < (pcpTask->resources[PCP_RESOURCE_A].unlockingTime + TICKS_DEVIATION_MARGIN) &&

				//Check if the task is currently accessing the Resource and only then it will release
				if(xSemaphoreGetMutexHolder(semaphoreData[pcpResource].semaphoreHandle) == pcpTask->taskHandle)
				{
					taskENTER_CRITICAL();
					if (xSemaphoreGive(semaphoreData[pcpResource].semaphoreHandle) == pdTRUE)
					{
						//Once we access the semaphore, we will restore the priority of the existing task

						/*If the task is not locking on any of the resource then it should restore the priority to its original priority*/
						if (xSemaphoreGetMutexHolder(semaphoreData[PCP_RESOURCE_A].semaphoreHandle) != pcpTask->taskHandle &&
							xSemaphoreGetMutexHolder(semaphoreData[PCP_RESOURCE_B].semaphoreHandle) != pcpTask->taskHandle &&
							xSemaphoreGetMutexHolder(semaphoreData[PCP_RESOURCE_C].semaphoreHandle) != pcpTask->taskHandle)
						{
							printf("Task %d released resource %d and changed its priority from %d to %d.\n",
								pcpTask->taskNb + 1, pcpResource + 1, uxTaskPriorityGet(pcpTask->taskHandle), pcpTask->originalPriority);

							vTaskPrioritySet(pcpTask->taskHandle, pcpTask->originalPriority);
							
						}
						else
						{
							printf("Task %d released resource %d and changed its priority from %d to %d.\n",
								pcpTask->taskNb + 1, pcpResource + 1, uxTaskPriorityGet(pcpTask->taskHandle), pcpTask->previousPriority);

							vTaskPrioritySet(pcpTask->taskHandle, pcpTask->previousPriority);
							
						}
					}
					else
					{
						printf("Task %d could NOT release resource %d with priority %d.\n",
							pcpTask->taskNb + 1, pcpResource + 1, uxTaskPriorityGet(pcpTask->taskHandle));
					}

					taskEXIT_CRITICAL();
			}
		}
	}
}




