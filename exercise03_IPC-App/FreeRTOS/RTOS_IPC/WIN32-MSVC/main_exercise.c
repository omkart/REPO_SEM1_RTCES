/******************************************************************************
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html

/* Standard includes. */
#include <stdio.h>
#include <assert.h>
#include <conio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/*Macros*/
#define IPC_MAX_TASKS_CONTROLLER	2
#define IPC_MAX_TASKS_SENSORS		3

/*Sensing and polling Frequency*/
#define IPC_SENSOR_FREQ_MS_SENSOR_1 pdMS_TO_TICKS( 200UL )			// 200ms
#define IPC_SENSOR_FREQ_MS_SENSOR_2A pdMS_TO_TICKS( 500UL )			// 500ms
#define IPC_SENSOR_FREQ_MS_SENSOR_2B pdMS_TO_TICKS( 1400UL )		// 1400ms

#define IPC_CONTROLLER_FREQ_MS pdMS_TO_TICKS( 1410UL )		// 1500ms used only for testing purposes


/*Min and max count up done by every sensor*/
#define IPC_SENSOR_1_MIN_COUNT		100U
#define IPC_SENSOR_1_MAX_COUNT		199U

#define IPC_SENSOR_2A_MIN_COUNT		200U
#define IPC_SENSOR_2A_MAX_COUNT		249U

#define IPC_SENSOR_2B_MIN_COUNT		250U
#define IPC_SENSOR_2B_MAX_COUNT		299U

/*Queue Size for Sensors*/
#define QUEUE_SENSORS_LENGTH				200U
#define QUEUE_SENSORS_ITEM_SIZE				sizeof( uint16_t ) 

/*
	Priorities at which the tasks are created.
 */
typedef enum
{
	IPC_TASK_PRIORITY_1 = (tskIDLE_PRIORITY + 1), //Lowest priority
	IPC_TASK_PRIORITY_2,
	IPC_TASK_PRIORITY_3,
	IPC_TASK_PRIORITY_UNDEFINED,
}e_ipcTaskPriority;

typedef enum
{
	IPC_TASK_TYPE_CONTROLLER_MAIN = 0,
	IPC_TASK_TYPE_CONTROLLER_SEC,
	IPC_TASK_TYPE_CONTROLLER_MAX,
}e_ipcControllerTaskType;

typedef enum
{
	IPC_TASK_TYPE_SENSOR_1 = 0,
	IPC_TASK_TYPE_SENSOR_2A,
	IPC_TASK_TYPE_SENSOR_2B,
	IPC_TASK_TYPE_SENSOR_MAX,
}e_ipcSensorTaskType;



/*
  *  data structure
  */

typedef struct
{
	e_ipcTaskPriority priority;
	unsigned long outputFrequency;
	void (*funcPtr)(void);
	TaskHandle_t taskHandle;
	QueueHandle_t queueHandle;
}s_ipcTasks;


/*Callback Functions*/
//Controller functions
void ipcControllerTaskMain(void* taskParameters);
void ipcControllerTaskSecondary(void* taskParameters);

//Sensor functions
void ipcSensorTask1(void* taskParameters);
void ipcSensorTask2a(void* taskParameters);
void ipcSensorTask2b(void* taskParameters);


/*IPC Tasks*/
static s_ipcTasks ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_MAX];	//Improvement : could be clubbed into single array and accessed through single enum type as index
static s_ipcTasks ipcSensorTasks[IPC_TASK_TYPE_SENSOR_MAX];
static QueueSetHandle_t xQueueSet2;

/*Additional Functions*/
void getSensorData(e_ipcControllerTaskType controllerType,QueueSetMemberHandle_t* xActivatedMember2);

void main_exercise( void )
{
	/*Controller tasks*/
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_MAIN].funcPtr = &ipcControllerTaskMain;
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_MAIN].priority = IPC_TASK_PRIORITY_3;  // todo: decide the final priority, keep 2 for now


	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_SEC].funcPtr = &ipcControllerTaskSecondary;
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_SEC].priority = IPC_TASK_PRIORITY_3;  // todo: decide the final priority, keep 2 for now


	/*Sensor Tasks*/
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].funcPtr = &ipcSensorTask1;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_1;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].priority = IPC_TASK_PRIORITY_2;				// todo: decide the final priority, keep 3 for now

	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].funcPtr = &ipcSensorTask2a;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_2A;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].priority = IPC_TASK_PRIORITY_2;				// todo: decide the final priority, keep 3 for now

	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].funcPtr = &ipcSensorTask2b;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_2B;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].priority = IPC_TASK_PRIORITY_2;				// todo: decide the final priority, keep 3 for now


	/*Create Controller tasks*/
	uint8_t taskCount = 0;
	uint8_t taskCountMax = (IPC_TASK_TYPE_CONTROLLER_MAX);

	for (taskCount = 0; taskCount < taskCountMax; taskCount++)
	{
		/*
		 * Create the task instances.
		 */
		xTaskCreate(ipcControllerTasks[taskCount].funcPtr,				/* The function that implements the task. */
			"ControllerTasks", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 									/* The size of the stack to allocate to the task. */
			NULL, 														/* The parameter passed to the task - not used in this simple case. */
			ipcControllerTasks[taskCount].priority,						/* The priority assigned to the task. */
			&ipcControllerTasks[taskCount].taskHandle);						

	}

	/*Create Sensor tasks*/
	taskCount = 0;
	taskCountMax = (IPC_TASK_TYPE_SENSOR_MAX);

	for (taskCount = 0; taskCount < taskCountMax; taskCount++)
	{
		/*
		 * Create the task instances.
		 */
		xTaskCreate(ipcSensorTasks[taskCount].funcPtr,					/* The function that implements the task. */
			"Sensor Tasks", 											/* The text name assigned to the task - for debug only as it is not used by the kernel. */
			configMINIMAL_STACK_SIZE, 									/* The size of the stack to allocate to the task. */
			NULL, 														/* The parameter passed to the task - not used in this simple case. */
			ipcSensorTasks[taskCount].priority,							/* The priority assigned to the task. */
			&ipcSensorTasks[taskCount].taskHandle);

		ipcSensorTasks[taskCount].queueHandle = xQueueCreate(QUEUE_SENSORS_LENGTH, QUEUE_SENSORS_ITEM_SIZE);

		/*check if queue was created*/
		configASSERT(ipcSensorTasks[taskCount].queueHandle);

	}

	
	/*Create queue sets on which the Controller tasks will keep a check*/

	//Sensor 2A and 2B data is added to a set
	/*
		Using Queue Sets as a way of establishing the logic required instead of using the signal flags
		With queue set for Sensor 2A and Sensor 2B, any new data in either of the sensor would enable the 
		Controller 1 to start the sensing of the data in queue. Since Sensor 1 is at high frequency compared 
		to either of Sensor 2, whenever data from Sensor 2 is available, Sensor 1 data will always be the latest data.
	*/
	xQueueSet2 = xQueueCreateSet(2*QUEUE_SENSORS_LENGTH);
	xQueueAddToSet(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].queueHandle, xQueueSet2);
	xQueueAddToSet(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].queueHandle, xQueueSet2);

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

/*
 *  Functions for Sensor Tasks
 */
void ipcSensorTask1(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = IPC_SENSOR_FREQ_MS_SENSOR_1;
	static uint16_t counterSensor1 = IPC_SENSOR_1_MIN_COUNT;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	while (1)
	{

		/*Delay the task until the block time*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		counterSensor1++;
		printf("Sensor 1 counted : %d\n", counterSensor1);
		xQueueSend(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].queueHandle, (void*)&counterSensor1, (TickType_t)10);

		if (counterSensor1 >= IPC_SENSOR_1_MAX_COUNT)
		{
			//Handle roll over
			counterSensor1 = IPC_SENSOR_1_MIN_COUNT;
		}

		

	}

}

void ipcSensorTask2a(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = IPC_SENSOR_FREQ_MS_SENSOR_2A;
	static uint16_t counterSensor2a = IPC_SENSOR_2A_MIN_COUNT;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	while (1)
	{
		/*Delay the task until the block time*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		counterSensor2a++;
		printf("Sensor 2A counted : %d\n", counterSensor2a);
		xQueueSend(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].queueHandle, (void*)&counterSensor2a, (TickType_t)10);

		if (counterSensor2a >= IPC_SENSOR_2A_MAX_COUNT)
		{
			//Handle roll over
			counterSensor2a = IPC_SENSOR_2A_MIN_COUNT;
		}

		
	}

}

void ipcSensorTask2b(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = IPC_SENSOR_FREQ_MS_SENSOR_2B;
	static uint16_t counterSensor2b = IPC_SENSOR_2B_MIN_COUNT;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	while (1)
	{
		/*Delay the task until the block time*/
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		counterSensor2b++;
		printf("Sensor 2B counted : %d\n", counterSensor2b);
		xQueueSend(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].queueHandle, (void*)&counterSensor2b, (TickType_t)10);

		if (counterSensor2b >= IPC_SENSOR_2B_MAX_COUNT)
		{
			//Handle roll over
			counterSensor2b = IPC_SENSOR_2B_MIN_COUNT;
		}

		
	}

}


/*Controller Tasks*/
void ipcControllerTaskMain(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = IPC_CONTROLLER_FREQ_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	QueueSetMemberHandle_t xActivatedMember2;

	/*Delay the task until the block time*/
	//vTaskDelayUntil(&xNextWakeTime, xBlockTime);
	while (1)
	{
		

		getSensorData(IPC_TASK_TYPE_CONTROLLER_MAIN, &xActivatedMember2);

		/*Once 2000ms is passed a failure is emulated in Controller 1 and controller 2 now takes over the sensing operation*/
		if ((xTaskGetTickCount() / portTICK_PERIOD_MS) >= 2000)
		{
			printf("Controller 1 has had an error at %ld\n", (xTaskGetTickCount() / portTICK_PERIOD_MS));
			/* Send notification to start execution of Controller 2 as Controller 1 has failed*/
			xTaskNotifyGive(ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_SEC].taskHandle);
			vTaskDelete(NULL);
			
		}
		else
		{
			
		}
	}
}

void getSensorData(e_ipcControllerTaskType controllerType,QueueSetMemberHandle_t* xActivatedMember2)
{

	auto uint16_t dataFromQueueSensor1 = 0;
	auto uint16_t dataFromQueueSensor2A = 0;
	auto uint16_t dataFromQueueSensor2B = 0;
	char controllerValueForPrint[20];
	if (controllerType == IPC_TASK_TYPE_CONTROLLER_MAIN)
	{
		strcpy(controllerValueForPrint, "Controller 1") ;
	}
	else if (controllerType == IPC_TASK_TYPE_CONTROLLER_SEC)
	{
		strcpy(controllerValueForPrint, "Controller 2");
	}

	/* Block to wait for something to be available from the queues that have been added to the set.  Don't block longer than 1500ms. */
	xActivatedMember2 = xQueueSelectFromSet(xQueueSet2, pdMS_TO_TICKS(1500UL));

	/*Once we reach here means there was a data available from either of the sensors 2A or 2B*/
	printf("%s received data at %ld; ", controllerValueForPrint, xTaskGetTickCount() / portTICK_PERIOD_MS);

	/*Lets first get the data from Sensor 1*/
	xQueueReceive(ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].queueHandle, &dataFromQueueSensor1, 5);
	printf("Sensor 1: %d;\t", dataFromQueueSensor1);

	if (xActivatedMember2 == ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].queueHandle)
	{
		if (xQueueReceive(xActivatedMember2, &dataFromQueueSensor2A, 5) == pdPASS) { printf("Sensor 2A: %d;\n", dataFromQueueSensor2A); }
		else { printf("Unable to get data from queue 2A"); }

	}
	else if (xActivatedMember2 == ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].queueHandle)
	{
		if (xQueueReceive(xActivatedMember2, &dataFromQueueSensor2B, 5) == pdPASS) { printf("Sensor 2B: %d;\n", dataFromQueueSensor2B); }
		else { printf("Unable to get data from queue 2B"); }

	}
	else
	{
		printf("Timed out without any data\n");
	}

}

void ipcControllerTaskSecondary(void* taskParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = IPC_CONTROLLER_FREQ_MS;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	QueueSetMemberHandle_t xActivatedMember2;

	/*Block until other task notifies to start working*/
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	while (1)
	{
		
		
		/*controller 2 will start sensing as controller 1 has failed*/
		getSensorData(IPC_TASK_TYPE_CONTROLLER_SEC, &xActivatedMember2);
	}

}


