#ifndef _HMI_CMD_H_
#define _HMI_CMD_H_

#include "Arduino.h"
#include "SoftwareSerial.h"

#define	VERSTR	"2021081901"
#define VENDOR	"LH"


#define setbit(value,x) (value |=(1<<x))
#define getbit(value,x) ((value>>x)&1)
#define clrbit(value,x) (value &=~(1<<x))
#define revbit(value,x) (value ^=(1<<x))

#define OUT1         24
#define OUT2         25
#define OUT3         26
#define OUT4         27
#define OUT5         28
#define OUT6         29
#define IN1          A0
#define IN2          A1
#define IN3          A2
#define IN4          A3
#define IN5          A4
#define IN6          A5

#define BT_PWRC      A6
#define BUZZ         A7

static const uint8_t OutputPin[] = {24, 25, 26, 27, 28, 29};

static const uint8_t InputPin[] = {A0, A1, A2, A3, A4, A5};

#define	CMD_PORT		Serial	
#define	CMD_PORT_BR		115200


#endif
