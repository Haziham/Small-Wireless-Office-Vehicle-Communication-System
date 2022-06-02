/*
 *  UART.h
 *	Version: 28 May 2022
 *
 *	This file holds the specification of UART communications.
 *
 */ 


#ifndef UART_H
#define UART_H

extern queue waypoints;
extern char vehicle_paused;

void UART_init_comms(unsigned int ubrr);
void UART_transmit_KVP(char* label, float value);

#endif