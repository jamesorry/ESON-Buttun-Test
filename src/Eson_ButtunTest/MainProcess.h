#ifndef _MAIN_PROCESS_H_
#define _MAIN_PROCESS_H_

#include "Arduino.h"

#define	EXTIO_NUM			4 //8個IO為一組
#define	INPUT_6_NUMBER		1
#define OUTPUT_6_NUMBER		1

#define	OUTPUT_NONE_ACTIVE	0
#define	OUTPUT_ACTIVE		1

#define	INPUT_NONE_ACTIVE	0
#define	INPUT_ACTIVE		1

#define WORKINDEX_TOTAL 	4

#define INPUT_START_BTN     0
#define INPUT_SET_BTN       1
typedef struct _DigitalIO_
{
	uint8_t	Input[4];
	uint8_t	Output[4];
	uint8_t PreOutput[4];
}DigitalIO;

typedef struct _MainDataStruct_
{
//	此處的變數值會寫到EEPROM
	char 		Vendor[10];
	uint8_t 	HMI_ID;
    int         TotalTimes;
    int         Speed;
}MainDataStruct;

#define workindex_RunMode       0
#define RunMode_Stop            0
#define RunMode_Normal          1
#define RunMode_Start           2
#define RunMode_ChooseMode      3
#define RunMode_SetTimes        4
#define RunMode_SetSpeed        5
#define RunMode_SetOK           6
#define RunMode_CheckSpeed      7
#define RunMode_StartStop       8
#define workindex_StartProcess   1
typedef struct _RuntimeStruct_
{
//	此處為啟動後才會使用的變數
	int  	Workindex[WORKINDEX_TOTAL];
	
	uint8_t sensor[INPUT_6_NUMBER*8 + EXTIO_NUM*8];
	uint8_t outbuf[(OUTPUT_6_NUMBER+EXTIO_NUM)*8];

	bool 		UpdateEEPROM = false;
	uint16_t	NowTimes = 0;
    long        single_delay_time = 0; //單次移動需要延遲多久
    uint8_t     chooseMode = 0;
    bool        NeedStopTest = false;
    bool        test = false;
    int         test_time = 0;
    int         test_count = 0;
}RuntimeStatus;

void calculateDelayTime();
void setOutput(uint8_t index, uint8_t hl);
uint8_t getInput(uint8_t index);
void SetTimesProcess();
void SetSpeedProcess();
bool StartProcess();
void ChooseModeProcess();
void MainPorcess_Timer();
void MainProcess_Task();
void MainProcess_Init();
void buzzerPlay(int playMS);
void Page_Start();
void Page_ChooseMode(uint8_t mode);

#endif	//_MAIN_PROCESS_H_

