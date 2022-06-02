/*
 *  main.c
 *	Version: 28 May 2022
 *
 *	Driver file for the vehicle communications system.
 *  Defines callback functions for navigation and drive systems.
 */ 

#define F_CPU 16000000

/* Included header files */
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "queue.h"
#include "UART.h"

/* Constants */
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define DISTANCE_X 0
#define DISTANCE_Y 1
#define SPEED 2

/* Global variables */
char *dummyNavStr[] = {"Optical vision accuracy","Inertia manipulator power"};
char *dummyDirStr[] = {"Light speed flux", "Jet engine capacity"};
float dummyNavFloat[] = {-3.45, 100};
float dummyDriFloat[] = {3.45, 1.67};

//Turn off watchdog timer on start up
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
	MCUSR = 0;
	wdt_disable();
}

/* Dummy navigation and drive function definitions */

/*
 * Dummy navigation telemetry function definition
 * Param callback disp_telemetry function
 */
void readTelemetryNav(void (*callback)(char **, float *, int))
{
	callback(dummyNavStr, dummyNavFloat, 2);
}

/*
 * Dummy drive telemetry function definition
 * Param callback disp_telemetry function
 */
void readTelemetryDrive(void (*callback)(char **, float *, int))
{
	callback(dummyDirStr, dummyDriFloat, 2);
}

/*
 * Dummy navigation movement function definition
 * Param callback waypointReached function
 */
void moveTo(float distance_x, float distance_y, float speed, void (*callback)(int))
{
	_delay_ms(2000);
	callback(0);
}


/* Callback functions */

/*
 * Transmits high-level telemetry (vehicle status) data back to user
 * Param labels string (char*) array containing message 
 * Param values float array containing telemetry values
 * Param length value length of labels and values arrays
 */
void disp_telemetry(char **labels, float *values, int length)
{
	for (int i = 0; i<length; i++)
	{
		UART_transmit_KVP(labels[i], values[i]);
	}
}

/*
 * Handles reaching waypoint success or failure and displays telemetry to user
 * Param labels string (char*) array containing message
 */
void waypointReached(int failure)
{
	if (failure == 0)
	{
		//Display success message on user interface 
		UART_transmit_KVP("Waypoint reached", 0);
	}
	else
	{
		//Display fail message on user interface
		UART_transmit_KVP("Waypoint not reached. Paused", 0);
		
		vehicle_paused = 1;
	}
	readTelemetryNav(disp_telemetry);
	readTelemetryDrive(disp_telemetry);
}


/* 
 * Main function
 * Initialises communication system and updates waypoint instructions and queue when needed
 */
int main()
{
	UART_init_comms(MYUBRR);
	UART_transmit_KVP("Starting up", 0);

	while (1)
	{
		if (!vehicle_paused && !queue_isEmpty(waypoints))
		{
			float* waypoint = (float*) queue_get(waypoints);
			moveTo(waypoint[DISTANCE_X], waypoint[DISTANCE_Y], waypoint[SPEED], waypointReached);
			queue_remove(waypoints);
		}
	}
	return 0;
}




