/*
 *  UART.h
 *	Version: 28 May 2022
 *
 *	This file holds the definition of UART communication protocol.
 *
 */ 


#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "queue.h"

/* Const variables */
#define WAYPOINT 10
#define START 11
#define PAUSE 12
#define STOP 13
#define SENDBACKTELEMETRY 14 
#define MAXSENDLIMIT 208
#define MESSAGESTART UDR0 == 2
#define MESSAGEEND message[13] == 3
#define PARITYERROR UCSR0A & (1<<UPE0)
#define EXPECTEDMESSGAELENGTH rx_count == 14
#define ENABLEWATCHDOGTIMER WDTCSR |= (1<<WDE) | (1<<WDIE);
#define WAIT while (1) {};
#define INSTRUCTION message[0]

unsigned char message[14];  
char receiving_waypoint = 0; 
char valid_waypoint = 1; 
uint8_t rx_count = 0; 
unsigned char vehicle_paused = 1; 
queue waypoints; 
queue keyValuePairs; 

typedef struct keyValuePair {
	char* label;
	float value;
	unsigned char bytelength;
} *keyValuePairPtr;

/*
 * Initialises receiver, transmitter and queues
 * Param ubrr value to be set as UART baud rate
 */
void UART_init_comms(unsigned int ubrr)
{
	//UART
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0); //enable transmitter, receiver, receive complete interrupt
	UCSR0C |= (1<<UPM01); //set even parity bit
	
	queue_init(&waypoints);
	queue_init(&keyValuePairs);
	
	sei(); //enable interrupts
}

/*
 * Transmit packet to PC
 * Param byte char to be transmitted
 */
void transmit_byte(char byte)
{
	while (!( UCSR0A & (1<<UDRE0))); // Wait for empty transmit buffer
	UDR0 = byte;				// Put data into buffer, sends the data
}

/*
 * Transmit message to PC
 * Param string char* to be transmitted
 */
void transmit_string(char *string)
{
	unsigned char length = strlen(string);
	for (int i = 0; i < length; i++){
		transmit_byte(string[i]);
	}
}

/*
 * Transmit data to PC
 * Param value float to be transmitted
 */
void transmit_float(float value)
{
	char * tempByteArray = (char*) & value;
	for (int i = 0; i<4; i++)
	{
		transmit_byte(tempByteArray[i]);
	}
}

/*
 * Transmit telemetry key value pair
 * Param label char* containing message
 * Param value float containing telemetry
 */
void transmit_KVP(char *label, float value)
{
	transmit_byte(2);
	transmit_byte(strlen(label));
	transmit_string(label);
	transmit_float(value);
	transmit_byte(3);
}

/*
 * Add key value pairs to the queue
 * This allows the master MCU (laptop) to specify when data
 * should be sent back.
 * 
 * Param label char* containing the label of the KVP
 * Param value float containing the value of the KVP
 */
void UART_transmit_KVP(char* label, float value)
{
	//ensuring label does not exceed max size
	if (strlen(label)+7<MAXSENDLIMIT)
	{
		//queue new key value pair
		keyValuePairPtr toAdd;
		toAdd = malloc(sizeof(*toAdd));
		toAdd->label = label;
		toAdd->value = value;
		toAdd->bytelength = strlen(label)+7;
		queue_add(keyValuePairs, toAdd);
	}
}

/*
 * Transmit queued key value pairs via UART0
 * If there are no KVP's to be sent a status OK message will be queued and sent instead. 
 * No more than 207 bytes will be sent at a time to prevent overloading GT-38 transceivers
 */
void send_back_telemetry()
{
	uint16_t bytesSent = 0; //Keeps track of bytes sent
	
	//If there are no KVP's send status OK
	if (queue_isEmpty(keyValuePairs))
	{
		UART_transmit_KVP("Status OK", 0);
	}
	
	//Send KVP's until max byte send limit is reach or there are no KVP's left
	while (!queue_isEmpty(keyValuePairs) && bytesSent <= MAXSENDLIMIT)
	{
		keyValuePairPtr current = (keyValuePairPtr)(queue_get(keyValuePairs));
		bytesSent += current->bytelength;
		if(bytesSent <= MAXSENDLIMIT)
		{
			transmit_KVP(current->label, current->value);
			queue_remove(keyValuePairs);
		}
	}
}

/*
 * Decodes message received into a waypoint (3 float values)
 * Param message char* to be decoded
 */
float* message_to_waypoint(unsigned char* message)
{
	volatile float floatType = 0;
	char * tempByteArray = (char*) & floatType;
	float* waypoint = malloc(3*sizeof(float));
	
	for (int i=0; i<3; i++)
	{
		for (int j=0; j<4; j++)
		{
			tempByteArray[j] = message[i*4+j+1];
		}
		waypoint[i] = floatType;
	}
	
	return waypoint;
}

/*
 * UART0 Receive Interrupt Service Routine (ISR)
 * Conducts error checking, stores waypoints and updates communication status based on instruction from user
 * If errors occur within the data received, an error message is transmitted back to the user
 */
ISR(USART0_RX_vect)
{
	if (receiving_waypoint)
	{
		//Check error flags
		if (PARITYERROR)
		{
			valid_waypoint = 0;
		}
		
		//Read message
		message[rx_count] = UDR0;
		rx_count++;
		
		if(EXPECTEDMESSGAELENGTH)
		{
			if (valid_waypoint && MESSAGEEND) //Check message is whole, valid and has termination character where expected
			{
				switch (INSTRUCTION)
				{
					case WAYPOINT:
					queue_add(waypoints, message_to_waypoint(message));
					break;
					case START:
					vehicle_paused = 0;
					break;
					case PAUSE:
					vehicle_paused = 1;
					break;
					case STOP:
					ENABLEWATCHDOGTIMER
					WAIT
					case SENDBACKTELEMETRY:
					send_back_telemetry();
					break;
				}
			} else
			{
				UART_transmit_KVP("Waypoint Not Processed: Error", 0); //Send error message back to PC
			}
			rx_count = 0;
			receiving_waypoint = 0;
		}
	}
	else if (MESSAGESTART) 
	{
		receiving_waypoint = 1;
	}
}

