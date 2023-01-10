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


/*Min and max count up done by every sensor*/
#define IPC_SENSOR_1_MIN_COUNT		100U
#define IPC_SENSOR_1_MAX_COUNT		199U

#define IPC_SENSOR_2A_MIN_COUNT		200U
#define IPC_SENSOR_2A_MAX_COUNT		249U

#define IPC_SENSOR_2B_MIN_COUNT		250U
#define IPC_SENSOR_2B_MAX_COUNT		299U



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

void main_exercise( void )
{
	/*Controller tasks*/
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_MAIN].funcPtr = &ipcControllerTaskMain;
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_MAIN].priority = IPC_TASK_PRIORITY_2;  // todo: decide the final priority, keep 2 for now


	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_SEC].funcPtr = &ipcControllerTaskSecondary;
	ipcControllerTasks[IPC_TASK_TYPE_CONTROLLER_SEC].priority = IPC_TASK_PRIORITY_2;  // todo: decide the final priority, keep 2 for now


	/*Sensor Tasks*/
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].funcPtr = &ipcSensorTask1;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_1;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_1].priority = IPC_TASK_PRIORITY_3;				// todo: decide the final priority, keep 3 for now

	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].funcPtr = &ipcSensorTask2a;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_2A;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2A].priority = IPC_TASK_PRIORITY_3;				// todo: decide the final priority, keep 3 for now

	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].funcPtr = &ipcSensorTask2b;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].outputFrequency = IPC_SENSOR_FREQ_MS_SENSOR_2B;
	ipcSensorTasks[IPC_TASK_TYPE_SENSOR_2B].priority = IPC_TASK_PRIORITY_3;				// todo: decide the final priority, keep 3 for now


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
	uint8_t taskCount = 0;
	uint8_t taskCountMax = (IPC_TASK_TYPE_SENSOR_MAX);

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

	}
}
/*-----------------------------------------------------------*/


/* TODO */
/*-----------------------------------------------------------*/


